#!/usr/bin/env bash
#*******************************************************************************
# djinterp [ci]                                              check_math_tests.sh
#
# Builds and runs the math module's tests: every C harness in
# tests/djinterp/c/modules/math/ (t_*.c) and every face test under
# tests/djinterp/cpp/modules/math/ (t_*_face.cpp), found afresh on each run, so
# a new test is in the next run. Each test prints "<name>: <n> checks, <m>
# failures" last. A build passes when it compiles (warnings are errors), exits
# 0, and its last line ends in ", 0 failures".
#   Two tiers:
#     smoke  every harness once, by gcc at C99, and every face test by g++ at
#            three rungs: ISO strict C++98, C++14 and C++20, at the host's
#            width. Needs gcc and g++ only.
#     full   the math lane's matrix. Each harness 12 ways: gcc and clang at
#            C99 and C11; g++ and clang++ compiling it as ISO strict C++98;
#            gcc and clang at C99 -m32 on SSE (-msse2 -mfpmath=sse); gcc at
#            C99 -m32 on the x87; g++ and clang++ as ISO strict C++98 -m32 on
#            SSE; and g++ so on the x87. A harness with a C++14 section (it
#            tests __cplusplus >= 201402L, where it checks that its kernels
#            are constant expressions with the run time's bits) 4 ways more:
#            g++ and clang++ at C++14 and C++20. Each face test 64 ways: g++
#            and clang++, 64- and 32-bit, the C++ ladder's 8 rungs, plain and
#            testing (-DD_TESTING=1). Needs gcc, g++, clang and clang++ and
#            their 32-bit libraries.
#   ISO strict C++98 is -std=c++98 -DD_CFG_ENV_ISO_STRICT=1
# -DRE_STD_CFG_ISO_STRICT=1; with -m32 it leaves no 64-bit integer at all.
# Harnesses build with -O2 -Wall -Wextra -Werror -pedantic-errors, face tests
# with -Wall -Wextra -Werror -Wshadow -pedantic-errors, and both link -lm.
#
#   Each build gives one line, "<name> <build> ok|BUILD-FAIL|RUN-FAIL ...". A
# build has the same label in both tiers, so a smoke run's lines are a subset
# of a full run's. Failures are printed as they happen, progress once a
# minute; then the builds and passes per test, and the time.
#
#   usage:   scripts/ci/check_math_tests.sh [smoke|full] [--save <file>] [--list]
#                                   [name ...]
#              smoke   the default
#              --save  also writes every build's line, sorted, to <file>
#              --list  prints each build's command instead of running it
#              name    only the tests named (t_decomp, t_eigen_face, ...)
#   env:     JOBS      parallel builds        (default: processor count)
#   exit:    0 when every build passes, 1 when one does not, 2 when the run
#            cannot be made (an unknown tier, option or name, a missing
#            compiler, or no 32-bit support for the full tier)
#
# path:      /scripts/ci/check_math_tests.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
#                                                            revised: 2026.10.14
#*******************************************************************************
set -u
export LC_ALL=C

here="$(pwd)"
root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root" || exit 2

tier="smoke"
save=""
list=0
names=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        smoke|full)
            tier="$1"
            ;;
        --save)
            if [[ $# -lt 2 ]]; then
                echo "--save needs a file" >&2
                exit 2
            fi
            save="$2"
            shift
            ;;
        --list)
            list=1
            ;;
        -h|--help)
            sed -n '/^#   usage/,/^#            compiler/p' "$0" | sed 's/^#//'
            exit 0
            ;;
        -*)
            echo "unknown option: $1" >&2
            exit 2
            ;;
        *)
            names+=("$1")
            ;;
    esac
    shift
done
if [[ -n "$save" && "$save" != /* ]]; then
    save="$here/$save"
fi
jobs="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

warn_c="-O2 -Wall -Wextra -Werror -pedantic-errors"
warn_cpp="-Wall -Wextra -Werror -Wshadow -pedantic-errors"
strict="-DD_CFG_ENV_ISO_STRICT=1 -DRE_STD_CFG_ISO_STRICT=1"

# the tests, and the names asked for
find tests/djinterp/c/modules/math -maxdepth 1 -name 't_*.c' | sort > "$tmp/harnesses"
find tests/djinterp/cpp/modules/math -name 't_*_face.cpp' | sort > "$tmp/faces"
for name in ${names[@]+"${names[@]}"}; do
    if ! grep -q -E "/${name}\.(c|cpp)\$" "$tmp/harnesses" "$tmp/faces"; then
        echo "no harness or face test named $name" >&2
        exit 2
    fi
done

# wanted <name>
#   whether the test of that name is to run: every one when none was named.
wanted() {
    local name
    [[ ${#names[@]} -eq 0 ]] && return 0
    for name in "${names[@]}"; do
        [[ "$name" == "$1" ]] && return 0
    done
    return 1
}

# rung_flags <rung>
#   the flags that select one of the C++ ladder's rungs.
rung_flags() {
    case "$1" in
        c++98-strict) echo "-std=c++98 $strict" ;;
        c++98|c++03)  echo "-std=$1 -Wno-variadic-macros" ;;
        *)            echo "-std=$1" ;;
    esac
}

# the builds, one a line: "<name>|<label>|<source>|<compiler and flags>"
: > "$tmp/jobs"
host_bits="$(getconf LONG_BIT 2>/dev/null || echo 64)"
while read -r src; do
    t="$(basename "$src" .c)"
    wanted "$t" || continue
    if [[ "$tier" == smoke ]]; then
        echo "$t|gcc-c99|$src|gcc -std=c99 $warn_c" >> "$tmp/jobs"
        continue
    fi
    {
        for cc in gcc clang; do
            for std in c99 c11; do
                echo "$t|$cc-$std|$src|$cc -std=$std $warn_c"
            done
        done
        for cxx in g++ clang++; do
            echo "$t|$cxx-c++98-strict|$src|$cxx -x c++ -std=c++98 $warn_c" \
                 "$strict"
        done
        for cc in gcc clang; do
            echo "$t|$cc-c99-m32-sse|$src|$cc -std=c99 $warn_c -m32 -msse2" \
                 "-mfpmath=sse"
        done
        echo "$t|gcc-c99-m32-x87|$src|gcc -std=c99 $warn_c -m32"
        for cxx in g++ clang++; do
            echo "$t|$cxx-c++98-strict-m32-sse|$src|$cxx -x c++ -std=c++98" \
                 "$warn_c $strict -m32 -msse2 -mfpmath=sse"
        done
        echo "$t|g++-c++98-strict-m32-x87|$src|g++ -x c++ -std=c++98 $warn_c" \
             "$strict -m32"
        if grep -q '__cplusplus >= 201402L' "$src"; then
            for cxx in g++ clang++; do
                for std in c++14 c++20; do
                    echo "$t|$cxx-$std|$src|$cxx -x c++ -std=$std $warn_c"
                done
            done
        fi
    } >> "$tmp/jobs"
done < "$tmp/harnesses"
while read -r src; do
    t="$(basename "$src" .cpp)"
    wanted "$t" || continue
    if [[ "$tier" == smoke ]]; then
        for rung in c++98-strict c++14 c++20; do
            echo "$t|g++-$rung-$host_bits-plain|$src|g++" \
                 "$(rung_flags "$rung") $warn_cpp" >> "$tmp/jobs"
        done
        continue
    fi
    for cxx in g++ clang++; do
        for bits in 64 32; do
            arch=""
            [[ "$bits" == 32 ]] && arch=" -m32"
            for rung in c++98-strict c++98 c++03 c++11 c++14 c++17 c++20 \
                        c++23; do
                for mode in plain testing; do
                    flag=""
                    [[ "$mode" == testing ]] && flag=" -DD_TESTING=1"
                    echo "$t|$cxx-$rung-$bits-$mode|$src|$cxx" \
                         "$(rung_flags "$rung") $warn_cpp$arch$flag"
                done
            done
        done
    done >> "$tmp/jobs"
done < "$tmp/faces"

if [[ "$list" -eq 1 ]]; then
    awk -F'|' '{ print $1 " " $2 ": " $4 " -Iinc " $3 " -lm" }' "$tmp/jobs"
    exit 0
fi

# what the tier needs: the compilers, and for full their 32-bit libraries
if [[ "$tier" == smoke ]]; then
    compilers="gcc g++"
else
    compilers="gcc g++ clang clang++"
fi
for cc in $compilers; do
    if ! command -v "$cc" > /dev/null 2>&1; then
        echo "$cc not found: the $tier tier needs $compilers" >&2
        exit 2
    fi
done
if [[ "$tier" == full ]]; then
    printf '#include <math.h>\nint main(void) { return (int)sqrt(0.0); }\n' \
        > "$tmp/probe.c"
    for cc in $compilers; do
        lang="c"
        [[ "$cc" == *++ ]] && lang="c++"
        if ! "$cc" -m32 -x "$lang" "$tmp/probe.c" -o "$tmp/probe" -lm \
                > /dev/null 2>&1; then
            echo "$cc -m32 cannot build and link a program: the full tier" \
                 "needs 32-bit libraries. On Ubuntu:" >&2
            echo "  sudo apt-get install gcc-multilib g++-multilib" \
                 "lib32stdc++-13-dev libc6-dev-i386" >&2
            exit 2
        fi
    done
fi

# one build: compile in the tree, run, report the run's last line
cat > "$tmp/one.sh" <<'EOF'
#!/usr/bin/env bash
IFS='|' read -r name label src cmd <<< "$1"
exe="$(mktemp "$2/t.XXXXXX")"
# shellcheck disable=SC2086
if ! log="$($cmd -Iinc "$src" -o "$exe" -lm 2>&1)"; then
    echo "$name $label BUILD-FAIL $(grep -m1 'error' <<< "$log" | cut -c1-200)"
    rm -f "$exe"
    exit 0
fi
# (bash's own notice of a crash goes nowhere: the line below reports it)
{ "$exe" > "$exe.out" 2>&1; } 2> /dev/null
status=$?
last="$(tail -n 1 "$exe.out")"
rm -f "$exe" "$exe.out"
if [[ $status -eq 0 && "$last" =~ ,\ 0\ failures$ ]]; then
    echo "$name $label ok $last"
else
    echo "$name $label RUN-FAIL(exit $status) $last"
fi
EOF

total="$(wc -l < "$tmp/jobs")"
if [[ "$total" -eq 0 ]]; then
    echo "no builds to run" >&2
    exit 2
fi
for cc in $compilers; do
    echo "$cc: $("$cc" --version | head -n 1)"
done
echo "math tests, $tier tier: $total builds of" \
     "$(cut -d'|' -f1 "$tmp/jobs" | sort -u | wc -l) tests, $jobs at a time"
start="$(date +%s)"
tr '\n' '\0' < "$tmp/jobs" |
    xargs -0 -P "$jobs" -I{} bash "$tmp/one.sh" {} "$tmp" |
    {
        count=0
        report="$start"
        while IFS= read -r line; do
            echo "$line" >> "$tmp/results"
            count=$((count + 1))
            read -r _ _ verdict _ <<< "$line"
            [[ "$verdict" == ok ]] || echo "  $line"
            # progress, with no estimate of the time left: the harnesses
            # come first, and one of their builds can take minutes where a
            # face test's takes seconds
            now="$(date +%s)"
            if (( now - report >= 60 )); then
                echo "  $count of $total builds in $((now - start))s"
                report="$now"
            fi
        done
    }
spent=$(( $(date +%s) - start ))

touch "$tmp/results"
sort -o "$tmp/results" "$tmp/results"
ran="$(wc -l < "$tmp/results")"
passed="$(awk '$3 == "ok"' "$tmp/results" | wc -l)"
awk '{ n[$1]++; if ($3 == "ok") k[$1]++ }
     END { for (t in n)
               printf "  %-26s %3d builds, %3d pass\n", t, n[t], k[t] }' \
    "$tmp/results" | sort
if [[ -n "$save" ]]; then
    cp "$tmp/results" "$save"
fi
echo "$ran of $total builds ran, $passed pass, in ${spent}s with $jobs job(s)"
if [[ "$ran" -ne "$total" || "$passed" -ne "$total" ]]; then
    exit 1
fi
exit 0
