#!/bin/sh
# rules.sh -- every fixture against its sheet's golden findings.
#
#   tests/dawk/fixtures/<sheet>/<name>.<c|h>           a fixture
#   tests/dawk/fixtures/<sheet>/<name>.<c|h>.expected  its exact findings
#
# The sheet is sheets/<sheet>.dss.  A fixture with an empty .expected is a
# negative case: the rules must stay silent on it.  Exit 1 on any mismatch,
# naming the fixture, so a green run names what it checked (I-13).
#
#   sh tests/dawk/rules.sh [build/rules]          check
#   sh tests/dawk/rules.sh [build/rules] --bless  rewrite the goldens; read
#                                                 every changed line first
RULES=${1:-build/rules}
BLESS=$2
fail=0
count=0
for fixture in $(find tests/dawk/fixtures -name "*.c" -o -name "*.h" | sort); do
    [ -f "$fixture" ] || continue
    sheet=$(echo "$fixture" | cut -d/ -f4)
    actual=$("$RULES" "sheets/$sheet.dss" "$fixture")
    count=$((count + 1))
    if [ "$BLESS" = "--bless" ]; then
        printf '%s' "$actual" > "$fixture.expected"
        [ -n "$actual" ] && echo >> "$fixture.expected"
        continue
    fi
    expected=$(cat "$fixture.expected" 2>/dev/null)
    if [ "$actual" != "$expected" ]; then
        echo "FAIL $fixture"
        printf '%s\n' "$expected" > /tmp/rules_expected.$$
        printf '%s\n' "$actual"   > /tmp/rules_actual.$$
        diff /tmp/rules_expected.$$ /tmp/rules_actual.$$ | sed 's/^/    /'
        rm -f /tmp/rules_expected.$$ /tmp/rules_actual.$$
        fail=1
    fi
done
if [ "$BLESS" = "--bless" ]; then
    echo "rules: $count goldens rewritten"
    exit 0
fi
[ $fail -eq 0 ] && echo "rules: $count fixtures, all match their goldens"
exit $fail
