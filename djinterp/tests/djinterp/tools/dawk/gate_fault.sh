#!/bin/sh
# gate_fault.sh - proves the fixer's repair audit is load-bearing.
#
# The dawk leaf builds the checker twice with a deliberately wrong repair
# compiled in (D_AUDIT_FAULT_CORRUPT): a trailing comment, as the sheets add
# one to a corresponding header's #include and to an #endif, is added with
# the last character of the code before it turned into a `#`.
# dcheck_fault_audited keeps the repair audit; dcheck_fault_unaudited
# compiles it out (D_AUDIT_DISABLED).  Each is run to a fixpoint over a copy
# of tests/dawk/fix/before, whose files have findings whose rules state a
# repair.  The test passes only if the fault damages lines without the audit
# and none with it, and the audit refused something.  A gate that never
# fires against a fault that never happens proves nothing, so the first
# condition is checked as strictly as the second.  CTest runs the same gate
# as run_gate_fault.cmake, beside the leaf.
#
#   sh tests/djinterp/tools/dawk/gate_fault.sh \
#       [build/dcheck_fault_audited] [build/dcheck_fault_unaudited]
#
# Run it from the repository root.
set -u
AUDITED=${1:-build/dcheck_fault_audited}
UNAUDITED=${2:-build/dcheck_fault_unaudited}
SHEETS="res/sheets/c_style.dss res/sheets/cpp_style.dss"
BEFORE=tests/dawk/fix/before
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

for program in "$AUDITED" "$UNAUDITED"; do
    if [ ! -x "$program" ]; then
        echo "FAIL gate_fault: no program at $program"
        exit 2
    fi
done

marks() {   # lines under $1 with the fault's mark: a # before a // comment
    find "$1" -type f -exec cat {} + | grep -c '# \{1,\}// '
}

refused=0
run() {     # $1 checker, $2 tree: fix to a fixpoint, or for eight runs
    i=0
    refused=0
    while [ $i -lt 8 ]; do
        i=$((i + 1))
        n=0
        for sheet in $SHEETS; do
            out=$("$1" --fix "$sheet" "$2" 2>/dev/null)
            k=$(printf '%s\n' "$out" \
                | sed -n 's/^ *\([0-9]*\) lines repaired.*/\1/p')
            r=$(printf '%s\n' "$out" \
                | sed -n 's/^ *\([0-9]*\) repairs refused.*/\1/p')
            n=$((n + ${k:-0}))
            refused=$((refused + ${r:-0}))
        done
        [ $n -eq 0 ] && break
    done
}

base=$(marks "$BEFORE")

cp -r "$BEFORE" "$work/unaudited"
run "$UNAUDITED" "$work/unaudited"
unaudited=$(($(marks "$work/unaudited") - base))

cp -r "$BEFORE" "$work/audited"
run "$AUDITED" "$work/audited"
audited=$(($(marks "$work/audited") - base))

echo "fault marks without the audit: $unaudited"
echo "fault marks with the audit:    $audited, $refused repair(s) refused"
fail=0
if [ "$unaudited" -le 0 ]; then
    echo "FAIL gate_fault: the fault never fired, so this proves nothing"
    fail=1
fi
if [ "$audited" -ne 0 ]; then
    echo "FAIL gate_fault: damaged lines passed the audit"
    fail=1
fi
if [ "$refused" -le 0 ]; then
    echo "FAIL gate_fault: the audit refused nothing"
    fail=1
fi
[ $fail -eq 0 ] && echo "gate_fault: the audit stopped every damaged line:" \
    "$unaudited without it, none with it"
exit $fail
