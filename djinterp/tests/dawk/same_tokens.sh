#!/bin/sh
# same_tokens.sh - `dcheck --same-tokens`, the check every fixer's work is
# put through, against pairs it must pass and pairs it must not.
#
#   tests/dawk/same_tokens/same/     <name>.before.* and <name>.after.*: what
#                                    a fixer of layout and comments may do
#   tests/dawk/same_tokens/differ/   pairs that differ in meaning, each by
#                                    what its name says
#
# A check that cannot fail checks nothing, so the second directory is the
# test. Most of its pairs hold, before and after, the same characters that
# are not white space in the same order: a comparison of those characters
# passes them, and the last line says how many it passes here, which is what
# comparing tokens adds.
#
#   sh tests/dawk/same_tokens.sh [build/dcheck]
DCHECK=${1:-build/dcheck}
dir=tests/dawk/same_tokens
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
fail=0

# the characters that are not white space, in order
squeeze() {
    tr -d ' \t\r\n\f\v' < "$1"
}

for before in "$dir"/same/*.before.*; do
    after=$(printf '%s' "$before" | sed 's/\.before\./.after./')
    if ! "$DCHECK" --same-tokens "$before" "$after" 2> "$work/why"; then
        echo "FAIL same_tokens: $(basename "$before") and its after differ"
        sed 's/^/    /' "$work/why"
        fail=1
    fi
done

caught=0
weak=0
for before in "$dir"/differ/*.before.*; do
    after=$(printf '%s' "$before" | sed 's/\.before\./.after./')
    "$DCHECK" --same-tokens "$before" "$after" 2> "$work/why"
    case $? in
        0)  echo "FAIL same_tokens: $(basename "$before") and its after pass"
            fail=1 ;;
        1)  caught=$((caught + 1)) ;;
        *)  echo "FAIL same_tokens: $(basename "$before") could not be read"
            sed 's/^/    /' "$work/why"
            fail=1 ;;
    esac
    # the comparison of characters: is it fooled?
    squeeze "$before" > "$work/a"
    squeeze "$after" > "$work/b"
    cmp -s "$work/a" "$work/b" && weak=$((weak + 1))
done

[ $fail -eq 0 ] && echo "same_tokens: the layout pair passes; $caught of" \
    "$caught differing pairs are caught, $weak of which a comparison of" \
    "characters passes"
exit $fail
