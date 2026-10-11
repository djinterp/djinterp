#!/bin/sh
# agree.sh -- the driver lists, of every fixture, what its golden holds.
#
#   tests/dawk/fixtures/<sheet>/   the fixtures of sheets/<sheet>.dss, each
#                                  with its findings in <fixture>.expected
#
# rules.sh reads a fixture through the harness, which builds the tokens of
# one file and nothing more. The gates read the tree through dcheck, which
# hangs every builder's node under the file beside the tokens -- banner,
# guard, include-block, delimiter, text -- and runs the cascade. A selector
# can tell the two trees apart, and one that does passes its fixture and
# fails in the gate. So dcheck is run over each sheet's fixtures as the gate
# runs it over the tree, and what it lists, with the severity taken out of
# each line, must be what the goldens hold, line for line.
#
#   sh tests/dawk/agree.sh [build/dcheck]
DCHECK=${1:-build/dcheck}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
fail=0
total=0
sheets=0

for dir in tests/dawk/fixtures/*/; do
    sheet=$(basename "$dir")
    root="tests/dawk/fixtures/$sheet"

    "$DCHECK" --source.files=all --report.findings=1000000 \
              "sheets/$sheet.dss" "$root" 2>/dev/null \
        | tr -d '\r' \
        | grep "^$root/" \
        | sed -E 's/^([^:]+:[0-9]+:[0-9]+): (error|warn|advise): /\1: /' \
        | LC_ALL=C sort > "$work/driver"
    find "$root" -name '*.expected' -exec cat {} + \
        | LC_ALL=C sort > "$work/goldens"

    sheets=$((sheets + 1))
    total=$((total + $(wc -l < "$work/goldens")))

    if ! cmp -s "$work/goldens" "$work/driver"; then
        echo "FAIL sheets/$sheet.dss: the driver and the goldens differ" \
             "(< the goldens, > the driver)"
        diff "$work/goldens" "$work/driver" | grep '^[<>]' | sed 's/^/    /'
        fail=1
    fi
done

[ $fail -eq 0 ] && echo "agree: the driver lists what the goldens hold," \
    "$total findings of $sheets sheets"
exit $fail
