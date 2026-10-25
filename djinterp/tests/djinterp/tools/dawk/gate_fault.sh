#!/bin/sh
# gate_fault.sh <root> -- proves the repair audit is load-bearing.
#
# Builds dcheck twice with a deliberately wrong repair compiled in: once with
# the audit, once without.  The test passes only if the fault demonstrably
# corrupts files when the audit is absent, and corrupts none when it is
# present.  A gate that never fires against a fault that never happens proves
# nothing, so the first condition is checked as strictly as the second.
set -u
ROOT=${1:?usage: gate_fault.sh <root>}
HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$HERE/../../../.." && pwd)
S="$TOP/src/djinterp/tools/dawk"
SHEET="$TOP/sheets/banner.dss"
TMP=${TMPDIR:-/tmp}/dawk_gate.$$
mkdir -p "$TMP"
SRCS="$S/dsymbol.c $S/dss.c $S/dnode.c $S/dmatch.c $S/dcascade.c $S/dschema.c \
      $S/dtree.c $S/dedit.c $S/daudit.c $S/dsettings.c $S/dgit.c $S/dline.c \
      $S/dbanner.c $S/dguard.c $S/dinclude.c $S/dsection.c $S/dlayout.c \
      $S/dproperty.c $S/dcheck.c $S/ext/*.c \
      $TOP/src/djinterp/parse/source/*.c $TOP/src/djinterp/parse/lex/*.c \
      $TOP/src/djinterp/parse/lang/c/*.c $TOP/src/djinterp/parse/lang/cpp/*.c \
      $TOP/src/djinterp/parse/syntax/*.c $TOP/src/djinterp/parse/c/diagnostic.c \
      $TOP/src/djinterp/parse/c/storage.c"
F="-std=c11 -D_POSIX_C_SOURCE=200809L -O1 -DD_AUDIT_FAULT_CORRUPT"
cc $F                     -o "$TMP/with_gate"    $SRCS || exit 2
cc $F -DD_AUDIT_DISABLED  -o "$TMP/without_gate" $SRCS || exit 2

marks() {   # lines ending in the fault's mark
    find "$1" \( -name '*.h' -o -name '*.hpp' \) -exec grep -l '#$' {} + \
        2>/dev/null | xargs -r grep -c '#$' 2>/dev/null \
        | awk -F: '{ n += $NF } END { print n + 0 }'
}

run() {     # $1 binary, $2 tree: fix to a fixpoint
    i=0
    while [ $i -lt 30 ]; do
        i=$((i + 1))
        r=$("$1" --fix "$SHEET" "$2" 2>/dev/null | awk '/lines repaired/ { print $1 }')
        [ "$r" = "0" ] && break
    done
}

base=$(marks "$ROOT")
for v in without_gate with_gate; do
    rm -rf "$TMP/tree"; cp -r "$ROOT" "$TMP/tree"
    run "$TMP/$v" "$TMP/tree"
    eval "$v=\$((\$(marks \"\$TMP/tree\") - base))"
done
rm -rf "$TMP"

echo "fault marks without the audit: $without_gate"
echo "fault marks with the audit:    $with_gate"
if [ "$without_gate" -le 0 ]; then
    echo "FAIL: the fault never fired, so this proves nothing"; exit 1
fi
if [ "$with_gate" -ne 0 ]; then
    echo "FAIL: corruption passed the audit"; exit 1
fi
echo "PASS: the audit stopped every corrupted line"
