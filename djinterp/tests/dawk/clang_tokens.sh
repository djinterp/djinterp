#!/bin/sh
# clang_tokens.sh - parse's lexer, which the fixer and its two safety checks
# read with, against Clang's raw lexer, over every C and C++ file under inc,
# src and tests, the checks' own pairs among them (dawk's decision 45a).
#
# Each file is lexed both ways - `clang -Xclang -dump-raw-tokens`, and
# `dcheck --clang-tokens=<that dump>`, which lexes it again with parse's
# lexer - and the two must agree token for token: name, spelling, line and
# column. A file on tests/dawk/clang_tokens_known.txt may disagree, and one
# there that agrees must come off it. A generated file is left out, as dcheck
# leaves it out: the fixer never reads one. Without Clang there is nothing to
# compare with, and the script says so and passes.
#
#   sh tests/dawk/clang_tokens.sh [build/dcheck] [clang]
DCHECK=${1:-build/dcheck}
CLANG=${2:-clang}
KNOWN=tests/dawk/clang_tokens_known.txt
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
fail=0

if ! command -v "$CLANG" > /dev/null 2>&1; then
    echo "clang_tokens: no $CLANG to compare with; nothing checked"
    exit 0
fi

# the known paths, one a line
tr -d '\r' < "$KNOWN" | grep -v '^#' | cut -f1 > "$work/known"

checked=0
disagree=0
generated=0
find inc src tests -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' \
    -o -name '*.hpp' -o -name '*.cc' -o -name '*.hh' -o -name '*.cxx' \
    -o -name '*.hxx' -o -name '*.ipp' -o -name '*.tpp' \) | sort \
    > "$work/files"
while read -r file; do
    # dcheck's mark of a generator's file, in its first 4 KiB
    if head -c 4096 "$file" | grep -qF 'Auto-generated. Do not edit by hand.'
    then
        generated=$((generated + 1))
        continue
    fi
    case $file in
        *.cpp|*.hpp|*.cc|*.hh|*.cxx|*.hxx|*.ipp|*.tpp)
            lang="-x c++ -std=c++2b" ;;
        *)  lang="-x c -std=c2x" ;;
    esac
    # shellcheck disable=SC2086
    "$CLANG" $lang -fsyntax-only -Xclang -dump-raw-tokens "$file" \
        2> "$work/dump" > /dev/null
    "$DCHECK" --clang-tokens="$work/dump" "$file" 2> "$work/why"
    status=$?
    checked=$((checked + 1))
    listed=0
    grep -qxF -- "$file" "$work/known" && listed=1
    if [ $status -eq 1 ] && [ $listed -eq 1 ]; then
        disagree=$((disagree + 1))
    elif [ $status -eq 1 ]; then
        echo "FAIL clang_tokens: $(cat "$work/why")"
        fail=1
    elif [ $status -eq 0 ] && [ $listed -eq 1 ]; then
        echo "FAIL clang_tokens: $file agrees now; take it off $KNOWN"
        fail=1
    elif [ $status -ne 0 ]; then
        echo "FAIL clang_tokens: $file: $(cat "$work/why")"
        fail=1
    fi
done < "$work/files"

[ $fail -eq 0 ] && echo "clang_tokens: $checked files lexed both ways" \
    "($generated generated left out); $((checked - disagree)) agree, and" \
    "the $disagree that don't are on $KNOWN"
exit $fail
