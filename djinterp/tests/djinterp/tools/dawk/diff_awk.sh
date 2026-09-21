#!/bin/sh
# Differential test: run every program in corpus.txt through the system awk
# and through dawk, over the same input, and diff.  A program the reference
# rejects is skipped rather than counted, since the two disagreeing about
# what is *legal* is a separate question from what a legal program computes.
set -u
DAWK="${1:-./build/dawk}"
REF="${REF:-awk}"
CORPUS="${2:-tests/djinterp/tools/dawk/corpus.txt}"
DATA=$(mktemp);  printf 'a b c\n1 2 3\nx y z\n' > "$DATA"
DATA2=$(mktemp); printf 'p q\nr s\n' > "$DATA2"
# A second operand exercises FNR, FILENAME and nextfile.  Programs that open
# files of their own use $AUX, which each side truncates before reading back.
AUX=$(mktemp); export AUX
pass=0; fail=0; skip=0; known=0
while IFS= read -r line; do
  [ -z "$line" ] && continue
  case "$line" in \#*) continue ;; esac
  expect_diff=0; prog="$line"
  case "$line" in !*) expect_diff=1; prog=${line#!} ;; esac
  : > "$AUX"
  want=$("$REF" -v aux="$AUX" "$prog" "$DATA" "$DATA2" 2>/dev/null); wrc=$?
  if [ $wrc -ne 0 ]; then skip=$((skip+1)); continue; fi
  : > "$AUX"
  got=$("$DAWK" -v aux="$AUX" "$prog" "$DATA" "$DATA2" 2>&1); grc=$?
  if [ $expect_diff -eq 1 ]; then
    if [ "$want" = "$got" ]; then
      fail=$((fail+1))
      printf 'UNEXPECTED AGREEMENT  %s\n' "$prog"
    else
      known=$((known+1))
    fi
    continue
  fi
  if [ "$want" = "$got" ] && [ $grc -eq 0 ]; then
    pass=$((pass+1))
  else
    fail=$((fail+1))
    printf 'MISMATCH  %s\n' "$prog"
    printf '   ref: %s\n' "$(printf '%s' "$want" | head -3 | tr '\n' '~')"
    printf '   got: %s\n' "$(printf '%s' "$got"  | head -3 | tr '\n' '~')"
  fi
done < "$CORPUS"
rm -f "$DATA" "$DATA2" "$AUX"
printf '\n%d passed, %d failed, %d known divergences, %d skipped by the reference\n' \
       "$pass" "$fail" "$known" "$skip"
[ "$fail" -eq 0 ]
