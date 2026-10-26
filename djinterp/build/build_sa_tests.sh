#!/usr/bin/env sh
# Per-standard compile sweep for the djinterp C++ filesystem _sa suites.
# Compile-only by default (needs only headers). Set DJINTERP_LIB to link+run.
#   sh build_sa_tests.sh
#   DJINTERP_LIB=/path/to/framework sh build_sa_tests.sh
#   STDS="c++17 c++23" sh build_sa_tests.sh
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="${REPO:-$(cd "$HERE/.." && pwd)}"
INC="$REPO/inc"; TESTDIR="$REPO/src/test/cpp"
CXX="${CXX:-g++}"; STDS="${STDS:-c++98 c++11 c++14 c++17 c++20 c++23}"
DJINTERP_LIB="${DJINTERP_LIB:-}"; CXXFLAGS="-Wall -Wextra -D_GNU_SOURCE -I$INC"
count_warnings() { n="$(grep -c 'warning:' "$1" 2>/dev/null)"; n="${n%%[!0-9]*}"; [ -n "$n" ] && echo "$n" || echo 0; }
ok=0; warn=0; fail=0
for src in "$TESTDIR"/*_tests_sa.cpp; do
  name="$(basename "$src" .cpp)"
  for std in $STDS; do
    log="$(mktemp)"
    if [ -n "$DJINTERP_LIB" ]; then
      exe="$(mktemp)"
      if "$CXX" -std="$std" $CXXFLAGS "$src" "$DJINTERP_LIB" -o "$exe" 2>"$log"; then
        res="$("$exe" 2>/dev/null | tail -1)"; w="$(count_warnings "$log")"
        printf '  %-26s %-7s %-24s warnings:%s\n' "$name" "$std" "$res" "$w"
        if [ "$w" -gt 0 ]; then warn=$((warn+1)); else ok=$((ok+1)); fi
      else printf '  %-26s %-7s BUILD FAIL\n' "$name" "$std"; fail=$((fail+1)); grep -m3 -E 'error:' "$log" | sed 's/^/       /'; fi
      rm -f "$exe"
    else
      if "$CXX" -std="$std" $CXXFLAGS -c "$src" -o /dev/null 2>"$log"; then
        w="$(count_warnings "$log")"; printf '  %-26s %-7s compiles  warnings:%s\n' "$name" "$std" "$w"
        if [ "$w" -gt 0 ]; then warn=$((warn+1)); else ok=$((ok+1)); fi
      else printf '  %-26s %-7s COMPILE FAIL\n' "$name" "$std"; fail=$((fail+1)); grep -m3 -E 'error:' "$log" | sed 's/^/       /'; fi
    fi
    rm -f "$log"
  done
done
echo; echo "ok:$ok  warnings:$warn  failures:$fail"; [ "$fail" -eq 0 ]
