#!/bin/sh
# fix.sh -- what `dcheck --fix` does to a file, against the file it should
# leave.
#
#   tests/dawk/fix/before/   files with findings whose rules state a repair
#   tests/dawk/fix/after/    the same files, repaired
#
# The fixer is run over a copy of before/ and the copy compared with after/,
# byte for byte.  The fixer is run until a run repairs nothing, which it
# must reach.  Three more things are checked, since a fixer that reaches
# the right text is not thereby safe: no repair was refused; each repaired
# file is the same to the compiler as its original, by
# tools/same_tokens.py -- every repair here changes white space or adds a
# comment; and what was moved in it stands where its brackets put it, by
# tools/moved_lines.py, which so agrees with after/ and not only with its
# own pairs.  The comparison is by tokens where there is a Clang to lex
# with and by characters where there is not, and the last line says which;
# without one the third check is not made.
#
#   sh tests/dawk/fix.sh [build/dcheck]
DCHECK=${1:-build/dcheck}
SHEETS="sheets/c_style.dss sheets/cpp_style.dss"
dir=tests/dawk/fix
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
fail=0

cp -r "$dir/before" "$work/tree"

# until a run repairs nothing: an item moved to its column is moved alone,
# and what else its file needs waits for the next run
total=0
refused=0
runs=0
while [ $runs -lt 8 ]; do
    n=0
    for sheet in $SHEETS; do
        out=$("$DCHECK" --fix "$sheet" "$work/tree" 2>>"$work/err")
        k=$(printf '%s\n' "$out" \
            | sed -n 's/^ *\([0-9]*\) lines repaired.*/\1/p')
        r=$(printf '%s\n' "$out" \
            | sed -n 's/^ *\([0-9]*\) repairs refused.*/\1/p')
        n=$((n + ${k:-0}))
        refused=$((refused + ${r:-0}))
    done
    runs=$((runs + 1))
    total=$((total + n))
    [ $n -eq 0 ] && break
done

if ! diff -r "$work/tree" "$dir/after" > "$work/diff"; then
    echo "FAIL fix: the repaired files are not the expected ones"
    sed 's/^/    /' "$work/diff"
    fail=1
fi

if [ $refused -ne 0 ]; then
    echo "FAIL fix: a repair was refused"
    sed 's/^/    /' "$work/err"
    fail=1
fi

if [ $n -ne 0 ]; then
    echo "FAIL fix: still repairing after $runs runs"
    fail=1
fi

# each repaired file against its original, as the compiler reads them
how=$(python3 tools/same_tokens.py --mode)
for before in "$dir"/before/*; do
    name=$(basename "$before")
    if ! python3 tools/same_tokens.py "$before" "$work/tree/$name" \
            2> "$work/why"; then
        echo "FAIL fix: $name is not what it was to the compiler"
        sed 's/^/    /' "$work/why"
        fail=1
    fi
    if [ "$how" = "tokens" ] && \
       ! python3 tools/moved_lines.py "$before" "$work/tree/$name" \
            2> "$work/why"; then
        echo "FAIL fix: something in $name was moved to where it does not" \
             "belong"
        sed 's/^/    /' "$work/why"
        fail=1
    fi
done

[ $fail -eq 0 ] && echo "fix: $total repairs as expected in $((runs - 1))" \
    "run(s), none refused, the last run found nothing, $how unchanged," \
    "everything moved in its place"
exit $fail
