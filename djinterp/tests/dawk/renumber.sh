#!/bin/sh
# renumber.sh -- tools/renumber_headings.py against the files it should
# leave.
#
#   tests/dawk/renumber/before/   headers with Roman headings, in each shape
#   tests/dawk/renumber/after/    the same headers, as they should come out:
#                                 seven changed -- headings converted, a
#                                 table renumbered or written, one not
#                                 written where a section's number is
#                                 missing, a list counted in letters left as
#                                 it is among them -- and six left exactly
#                                 as they were
#
# The tool is run over a copy of before/ and the copy compared with after/,
# byte for byte.  A second run must convert nothing; each file must be the
# same to the compiler as it was, by tools/same_tokens.py, since only
# comments may change; and nothing in it may have moved, by
# tools/moved_lines.py.  The comparison is by tokens where there is a Clang
# to lex with and by characters where there is not, and the last line says
# which; without one the last check is not made.
#
#   sh tests/dawk/renumber.sh
dir=tests/dawk/renumber
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
fail=0

mkdir -p "$work/inc/djinterp" "$work/src/djinterp"
cp "$dir"/before/* "$work/inc/djinterp/"

first=$(python3 tools/renumber_headings.py --base "$work") || fail=1
second=$(python3 tools/renumber_headings.py --base "$work") || fail=1

if ! diff -r "$work/inc/djinterp" "$dir/after" > "$work/diff"; then
    echo "FAIL renumber: the converted files are not the expected ones"
    sed 's/^/    /' "$work/diff"
    fail=1
fi

if ! printf '%s\n' "$second" | grep -q '^converted: 0 file'; then
    echo "FAIL renumber: a second run found more to convert"
    fail=1
fi

how=$(python3 tools/same_tokens.py --mode)
for before in "$dir"/before/*; do
    name=$(basename "$before")
    if ! python3 tools/same_tokens.py "$before" \
            "$work/inc/djinterp/$name" 2> "$work/why"; then
        echo "FAIL renumber: $name is not what it was to the compiler"
        sed 's/^/    /' "$work/why"
        fail=1
    fi
    if [ "$how" = "tokens" ] && \
       ! python3 tools/moved_lines.py "$before" \
            "$work/inc/djinterp/$name" 2> "$work/why"; then
        echo "FAIL renumber: something in $name moved"
        sed 's/^/    /' "$work/why"
        fail=1
    fi
done

[ $fail -eq 0 ] && echo "renumber: $(printf '%s\n' "$first" | head -1);" \
    "six left as they were, nothing for a second run, $how unchanged," \
    "nothing moved"
exit $fail
