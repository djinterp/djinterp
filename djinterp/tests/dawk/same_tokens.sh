#!/bin/sh
# same_tokens.sh -- tools/same_tokens.py, the check every fixer's work is put
# through, against pairs it must pass and pairs it must not.
#
#   tests/dawk/same_tokens/same/     <name>.before.* and <name>.after.*: what
#                                    a fixer of layout and comments may do
#   tests/dawk/same_tokens/differ/   pairs that differ in meaning, each by
#                                    what its name says
#
# A check that cannot fail checks nothing, so the second directory is the
# test. Most of its pairs hold, before and after, the same characters that
# are not white space in the same order: the comparison this tool replaced
# passes those, and the last line says how many it passes here. Without
# Clang there is only that comparison, and the test says so and fails, since
# it would then be vouching for a check that is not being made.
#
#   sh tests/dawk/same_tokens.sh
TOOL=tools/same_tokens.py
dir=tests/dawk/same_tokens
fail=0

if [ "$(python3 "$TOOL" --mode)" != "tokens" ]; then
    echo "FAIL same_tokens: no clang to lex with; only characters compared"
    exit 1
fi

for before in "$dir"/same/*.before.*; do
    after=$(printf '%s' "$before" | sed 's/\.before\./.after./')
    if ! python3 "$TOOL" "$before" "$after" 2>/dev/null; then
        echo "FAIL same_tokens: $(basename "$before") and its after differ"
        python3 "$TOOL" "$before" "$after" 2>&1 | sed 's/^/    /'
        fail=1
    fi
done

caught=0
weak=0
for before in "$dir"/differ/*.before.*; do
    after=$(printf '%s' "$before" | sed 's/\.before\./.after./')
    if python3 "$TOOL" "$before" "$after" 2>/dev/null; then
        echo "FAIL same_tokens: $(basename "$before") and its after pass"
        fail=1
    else
        caught=$((caught + 1))
    fi
    # the comparison by characters, which is the fallback: is it fooled?
    CLANG=/nonexistent python3 "$TOOL" "$before" "$after" 2>/dev/null \
        && weak=$((weak + 1))
done

[ $fail -eq 0 ] && echo "same_tokens: the layout pair passes; $caught of" \
    "$caught differing pairs are caught, $weak of which a comparison of" \
    "characters passes"
exit $fail
