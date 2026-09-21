#!/bin/sh
# Run the one-true-awk T.* conformance suite.  The suite is NOT vendored here:
# it is 6MB and it is not ours.  Fetch it with:
#
#   curl -sL https://codeload.github.com/onetrueawk/awk/tar.gz/refs/heads/master \
#     | tar xz -C /tmp && SUITE=/tmp/awk-master/testdir sh tests/djinterp/tools/dawk/conformance.sh
#
# Run it against gawk first.  gawk fails 9 of the 33 scripts, because parts of
# the suite test one-true-awk's exact error text and its own extensions, so
# "gawk is clean here and we are not" is the only signal worth chasing.
set -u
AWKBIN="${1:-$(pwd)/build/dawk}"
# The suite is copied into a scratch directory and run from there, so a
# relative path to the interpreter would stop resolving.  Absolutise it first;
# getting this wrong makes every test fail with 127 and looks like a real
# regression.
case "$AWKBIN" in /*) ;; *) AWKBIN="$(cd "$(dirname "$AWKBIN")" && pwd)/$(basename "$AWKBIN")" ;; esac
[ -x "$AWKBIN" ] || { echo "no executable at $AWKBIN"; exit 2; }
SUITE="${SUITE:-/tmp/awk-master/testdir}"
[ -d "$SUITE" ] || { echo "suite not found at $SUITE; see the header"; exit 2; }
WORK=$(mktemp -d); cp -r "$SUITE"/* "$WORK"/ 2>/dev/null; cd "$WORK" || exit 2
pass=0; bad=0; crash=0
for t in T.*; do
  out=$(awk="$AWKBIN" timeout 20 sh "$t" </dev/null 2>&1); rc=$?
  n=$(printf '%s' "$out" | grep -c 'BAD' || true)
  if [ "$rc" -ne 0 ] && [ "$n" -eq 0 ]; then crash=$((crash+1)); printf '  %-14s exit %s\n' "$t" "$rc"
  elif [ "$n" -gt 0 ];   then bad=$((bad+1));   printf '  %-14s %s BAD\n' "$t" "$n"
  else pass=$((pass+1)); fi
done
printf '\n%d clean, %d with BAD, %d nonzero-exit\n' "$pass" "$bad" "$crash"
cd /; rm -rf "$WORK"
