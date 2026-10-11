#!/bin/sh
# roots.sh -- a sheet says the same of a file whichever directory it was
# read under.
#
# The gates run every sheet over inc/djinterp and src/djinterp, but a person
# checking what they have just edited names one subsystem or one directory.
# The subsystem a banner names, the guard a header carries, the path a banner
# states and every rule that asks where a file lives are derived from the
# file's path below those two roots, so a run over a directory further down
# must work that path out and not take the directory for a root.
#
# For each sheet and each directory below, the findings of a run over the
# directory alone must be the findings the run over its whole root gives for
# the same files, line for line.
#
#   sh tests/dawk/roots.sh [build/dcheck]
DCHECK=${1:-build/dcheck}
SHEETS="banner guard include section text c_style cpp_style"
DIRS="inc/djinterp/config/core/env
inc/djinterp/c/util/macro
src/djinterp/c/container"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
fail=0
compared=0

findings() {
    # $1 a sheet, $2 a root: the run's findings, sorted
    "$DCHECK" --source.files=all --report.findings=10000000 \
              "sheets/$1.dss" "$2" 2>/dev/null \
        | grep -E '^[^ ]+:[0-9]+:[0-9]+: (error|warn|advise): ' \
        | sort
}

for sheet in $SHEETS; do
    for top in inc/djinterp src/djinterp; do
        findings "$sheet" "$top" > "$work/whole.$sheet.$(dirname "$top")"
    done
    for dir in $DIRS; do
        whole="$work/whole.$sheet.${dir%%/*}"
        grep "^$dir/" "$whole" > "$work/expected"
        findings "$sheet" "$dir" > "$work/actual"
        compared=$((compared + $(wc -l < "$work/expected")))
        if ! cmp -s "$work/expected" "$work/actual"; then
            echo "FAIL roots: $sheet says otherwise of $dir read by itself"
            diff "$work/expected" "$work/actual" | head -6 | sed 's/^/    /'
            fail=1
        fi
    done
done

[ $fail -eq 0 ] && echo "roots: seven sheets say the same of three" \
    "directories read alone as under their roots, $compared findings"
exit $fail
