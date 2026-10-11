#!/bin/sh
# dcheck.sh -- dcheck's resolution of a sheet's claims, against a golden.
#
#   tests/dawk/dcheck/claims.dss       the sheet
#   tests/dawk/dcheck/root/            the tree it is run over
#   tests/dawk/dcheck/claims.first.expected  the findings, exactly, under
#   tests/dawk/dcheck/claims.all.expected    each cascade.resolve
#
# rules.sh goes through the engine's evaluator and never through the
# driver, so it cannot see which claims the driver lets stand or the
# severity it reports them at.  This runs the driver itself.
#
#   sh tests/dawk/dcheck.sh [build/dcheck]          check
#   sh tests/dawk/dcheck.sh [build/dcheck] --bless  rewrite the golden; read
#                                                   every changed line first
DCHECK=${1:-build/dcheck}
BLESS=$2
dir=tests/dawk/dcheck
fail=0
for mode in first all; do
    actual=$("$DCHECK" --cascade.resolve=$mode --report.findings=1000 \
                       "$dir/claims.dss" "$dir/root" 2>&1 \
             | grep "^$dir/root/")
    if [ "$BLESS" = "--bless" ]; then
        printf '%s\n' "$actual" > "$dir/claims.$mode.expected"
        continue
    fi
    expected=$(cat "$dir/claims.$mode.expected" 2>/dev/null)
    if [ "$actual" != "$expected" ]; then
        echo "FAIL dcheck claims (cascade.resolve=$mode)"
        printf '%s\n' "$expected" > /tmp/dcheck_expected.$$
        printf '%s\n' "$actual"   > /tmp/dcheck_actual.$$
        diff /tmp/dcheck_expected.$$ /tmp/dcheck_actual.$$ | sed 's/^/    /'
        rm -f /tmp/dcheck_expected.$$ /tmp/dcheck_actual.$$
        fail=1
    fi
done
if [ "$BLESS" = "--bless" ]; then
    echo "dcheck: goldens rewritten"
    exit 0
fi
[ $fail -eq 0 ] && echo "dcheck: claims resolve as the goldens say, under first and all"
exit $fail
