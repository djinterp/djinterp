#!/bin/sh
# moved_lines.sh -- tools/moved_lines.py, which reads where a fixer put what
# it moved, against pairs it must pass and pairs it must not.
#
#   tests/dawk/moved_lines/right/   <name>.before.* and <name>.after.*: each
#                                   repair of layout, made as it should be
#   tests/dawk/moved_lines/wrong/   the same, made wrongly, each by what its
#                                   name says; <name>.says holds what the
#                                   tool must say of it, line and all
#
# A check that cannot fail checks nothing, so the second directory is the
# test. Every pair in it holds the same tokens before and after -- that is
# checked here too -- so each is something tools/same_tokens.py passes, and
# three of them are what `dcheck --fix` once wrote into the tree's own
# files: bases_laid_out_as_parameters, comment_left_behind and
# statement_left_behind.
#
#   sh tests/dawk/moved_lines.sh
TOOL=tools/moved_lines.py
SAME=tools/same_tokens.py
dir=tests/dawk/moved_lines
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
fail=0

if [ "$(python3 "$SAME" --mode)" != "tokens" ]; then
    echo "FAIL moved_lines: no clang to lex with"
    exit 1
fi

right=0
for before in "$dir"/right/*.before.*; do
    after=$(printf '%s' "$before" | sed 's/\.before\./.after./')
    right=$((right + 1))
    if ! python3 "$SAME" "$before" "$after" 2> "$work/why"; then
        echo "FAIL moved_lines: $(basename "$before") and its after do not" \
             "hold the same tokens"
        sed 's/^/    /' "$work/why"
        fail=1
    fi
    if ! python3 "$TOOL" "$before" "$after" 2> "$work/why"; then
        echo "FAIL moved_lines: $(basename "$before") was moved rightly," \
             "and is not passed"
        sed 's/^/    /' "$work/why"
        fail=1
    fi
done

caught=0
for before in "$dir"/wrong/*.before.*; do
    after=$(printf '%s' "$before" | sed 's/\.before\./.after./')
    name=$(basename "$before" | sed 's/\.before\..*//')
    if ! python3 "$SAME" "$before" "$after" 2> "$work/why"; then
        echo "FAIL moved_lines: $name is to differ in layout alone, and" \
             "differs in tokens"
        sed 's/^/    /' "$work/why"
        fail=1
    fi
    if python3 "$TOOL" "$before" "$after" 2> "$work/said"; then
        echo "FAIL moved_lines: $name is passed"
        fail=1
    elif ! grep -qF -- "$(cat "$dir/wrong/$name.says")" "$work/said"; then
        echo "FAIL moved_lines: $name is refused, but not for what it is:"
        sed 's/^/    /' "$work/said"
        echo "    expected: $(cat "$dir/wrong/$name.says")"
        fail=1
    else
        caught=$((caught + 1))
    fi
done

[ $fail -eq 0 ] && echo "moved_lines: $right pairs moved rightly pass;" \
    "$caught of $caught moved wrongly are caught, each for what it is, and" \
    "each holds the tokens it held"
exit $fail
