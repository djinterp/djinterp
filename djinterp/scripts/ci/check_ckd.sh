#!/usr/bin/env bash
#*******************************************************************************
# djinterp [ci]                                                     check_ckd.sh
#
# Runs the checked-arithmetic kit's harness (c/dckdint.h) and the regression
# harness of the two heap overflows it fixed, in every configuration a build
# can be in.
#     tests/unintegrated/c/t_dckdint.c
#         as C at C99, C11, C17 and C23 (GCC's -std=c2x where it has no
#         -std=c23) and as C++ at ISO strict C++98, C++98, C++11, 14, 17, 20
#         and 23, and C++26 where the compiler has it; with GCC and Clang;
#         plain and with -DD_TESTING=1; and at each of the kit's three tiers:
#         the best the build reaches, the compiler's builtins (with
#         D_ENV_C_HAS_STDCKDINT_H pre-defined to 0) and the portable test
#         (with D_ENV_COMPILER_HAS_OVERFLOW_BUILTINS pre-defined to 0 too).
#         A forced tier must be the one reached; the best must be the
#         builtins' or <stdckdint.h>'s.
#     tests/unintegrated/c/container/t_container_overflow.c
#         d_array_common_init_fill and d_vector_common_reserve, built with
#         the container sources under AddressSanitizer and
#         UndefinedBehaviorSanitizer, as C at C99, C11, C17 and C23, with GCC
#         and Clang, at the best tier and at the portable one.
# Every unit is compiled with the ladders' flags: -pedantic-errors
# -Werror=undef -Wall -Wextra -Werror -D_XOPEN_SOURCE=700, and for ISO strict
# C++98 -DD_CFG_ENV_ISO_STRICT=1 -DRE_STD_CFG_ISO_STRICT=1.
#   A configuration passes when it builds, its harness ends "0 failed" with
# exit 0, and its tier is the one expected. A compiler that is missing fails
# the run unless ALLOW_SKIP=1 says the gap is known, and the number of
# configurations that reported is compared with the matrix's, so that a row
# lost to an edit fails too. The ladders' newest rungs, C23 and C++26, are
# noted rather than failed where a compiler has neither their name nor their
# draft's (GCC before 9 for C23, GCC 13 for C++26).
#
#   usage:   bash scripts/ci/check_ckd.sh            (from the tree's root)
#   env:     CC_LIST      the C compilers to use (default "gcc clang")
#            CXX_LIST     the C++ compilers, paired with CC_LIST by position
#                         (default "g++ clang++")
#            JOBS         parallel builds (default: the processors)
#            ALLOW_SKIP   1 lets a missing compiler or level be skipped
#            EXTRA_FLAGS  added to every compile and link (e.g. -m32, for a
#                         32-bit build where size_t is 32 bits); a
#                         configuration the flags leave the compiler unable
#                         to build at all (no 32-bit library) is skipped
#   exit:    0 when every configuration passes and none was skipped without
#            leave; 1 otherwise; 2 when the run itself could not start
#
#
# path:      /scripts/ci/check_ckd.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
#                                                            revised: 2026.11.05
#*******************************************************************************
set -u

root="$(pwd)"
if [[ ! -f "$root/inc/djinterp/c/dckdint.h" ]]; then
    echo "check_ckd.sh: run it from the tree's root (inc/, src/, tests/)" >&2
    exit 2
fi

read -r -a cc_list  <<< "${CC_LIST:-gcc clang}"
read -r -a extra    <<< "${EXTRA_FLAGS:-}"
read -r -a cxx_list <<< "${CXX_LIST:-g++ clang++}"
jobs="${JOBS:-$(getconf _NPROCESSORS_ONLN 2> /dev/null || echo 4)}"
allow_skip="${ALLOW_SKIP:-0}"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

warn="-pedantic-errors -Werror=undef -Wall -Wextra -Werror -D_XOPEN_SOURCE=700"
strict="-DD_CFG_ENV_ISO_STRICT=1 -DRE_STD_CFG_ISO_STRICT=1"
sanitize="-g -fsanitize=address,undefined -fno-omit-frame-pointer"
sanitize="$sanitize -fno-sanitize-recover=all"
no_header="-DD_ENV_C_HAS_STDCKDINT_H=0"
no_builtins="-DD_ENV_COMPILER_HAS_OVERFLOW_BUILTINS=0"
tier_flags=( ""
             "$no_header -DD_TESTS_CKD_EXPECT_TIER=2"
             "$no_header $no_builtins -DD_TESTS_CKD_EXPECT_TIER=1" )
kit_src="tests/unintegrated/c/t_dckdint.c"
overflow_src="tests/unintegrated/c/container/t_container_overflow.c"
tier_names=( best builtins portable )
container_srcs="src/djinterp/c/container/array/array_common.c"
container_srcs="$container_srcs src/djinterp/c/container/vector/vector_common.c"
container_srcs="$container_srcs src/djinterp/c/memory/dmemory.c"
container_srcs="$container_srcs src/djinterp/c/djinterp.c"

# std_ok COMPILER LANG STD: whether the compiler accepts the level
std_ok()
{
    echo 'int main(void) { return 0; }' |
        "$1" ${extra[@]+"${extra[@]}"} -x "$2" -std="$3" -fsyntax-only - \
            > /dev/null 2>&1
}

# links_ok COMPILER LANG: whether the compiler can link a program with the
# extra flags (a 32-bit build needs the multilib packages)
links_ok()
{
    echo 'int main(void) { return 0; }' |
        "$1" ${extra[@]+"${extra[@]}"} -x "$2" - -o "$work/link" \
            > /dev/null 2>&1
}

# san_ok COMPILER: whether the compiler can link a sanitized program (Clang
# needs its runtime package, libclang-rt-NN-dev on Debian and Ubuntu)
san_ok()
{
    echo 'int main(void) { return 0; }' |
        "$1" ${extra[@]+"${extra[@]}"} -x c -fsanitize=address,undefined - \
            -o "$work/san" > /dev/null 2>&1
}

# the C level's spelling for this compiler: -std=c23, or GCC's -std=c2x;
# nothing where it has neither
c_level()
{
    if std_ok "$1" c "$2"; then
        echo "$2"
    elif [[ "$2" == c23 ]] && std_ok "$1" c c2x; then
        echo c2x
    fi
}

: > "$work/jobs"
expected=0
skipped=0
for i in "${!cc_list[@]}"; do
    cc="${cc_list[$i]}"
    cxx="${cxx_list[$i]:-}"
    if ! command -v "$cc" > /dev/null 2>&1; then
        echo "SKIP $cc: not found"
        skipped=$((skipped + 1))
        continue
    fi
    if ! links_ok "$cc" c; then
        echo "SKIP $cc ${extra[*]:-}: cannot link a program with these flags"
        skipped=$((skipped + 1))
        continue
    fi
    sanitizers=1
    if ! san_ok "$cc"; then
        echo "SKIP $cc: no sanitizer runtime, so no overflow harness"
        skipped=$((skipped + 1))
        sanitizers=0
    fi
    for std in c99 c11 c17 c23; do
        level="$(c_level "$cc" "$std")"
        if [[ -z "$level" ]]; then
            if [[ "$std" == c23 ]]; then
                # C23 is the C ladder's newest rung: a compiler with neither
                # -std=c23 nor -std=c2x (GCC before 9) is noted, not failed
                echo "NOTE $cc: no -std=c23 or -std=c2x"
                continue
            fi
            echo "SKIP $cc $std: not accepted"
            skipped=$((skipped + 1))
            continue
        fi
        for t in 0 1 2; do
            for mode in plain testing; do
                testing=""
                [[ "$mode" == testing ]] && testing="-DD_TESTING=1"
                cmd="$cc ${extra[*]:-} -x c -std=$level $warn $testing"
                cmd="$cmd ${tier_flags[$t]}"
                echo "kit|$cc $std ${tier_names[$t]} $mode|$cmd|$kit_src|" \
                    >> "$work/jobs"
                expected=$((expected + 1))
            done
        done
        for t in 0 2; do
            [[ "$sanitizers" -eq 1 ]] || continue
            # the tier's defines, less the kit harness's expectation
            tier="${tier_flags[$t]%% -DD_TESTS_CKD_EXPECT_TIER=*}"
            cmd="$cc ${extra[*]:-} -x c -std=$level $warn $sanitize $tier"
            label="$cc $std ${tier_names[$t]} asan"
            echo "overflow|$label|$cmd|$overflow_src|$container_srcs" \
                >> "$work/jobs"
            expected=$((expected + 1))
        done
    done
    if [[ -z "$cxx" ]] || ! command -v "$cxx" > /dev/null 2>&1 ||
       ! links_ok "$cxx" c++; then
        echo "SKIP ${cxx:-the C++ compiler for $cc}: not found, or cannot" \
             "link with the extra flags"
        skipped=$((skipped + 1))
        continue
    fi
    for std in c++98s c++98 c++11 c++14 c++17 c++20 c++23 c++26; do
        flags=""
        level="$std"
        if [[ "$std" == c++98s ]]; then
            level=c++98
            flags="$strict"
        fi
        if ! std_ok "$cxx" c++ "$level"; then
            if [[ "$std" == c++26 ]]; then
                # C++26 is the ladder's newest rung: a compiler without it
                # (GCC 13) is noted, not failed
                echo "NOTE $cxx: no -std=c++26"
                continue
            fi
            echo "SKIP $cxx $std: not accepted"
            skipped=$((skipped + 1))
            continue
        fi
        for t in 0 1 2; do
            for mode in plain testing; do
                testing=""
                [[ "$mode" == testing ]] && testing="-DD_TESTING=1"
                cmd="$cxx ${extra[*]:-} -x c++ -std=$level $flags $warn"
                cmd="$cmd $testing"
                cmd="$cmd ${tier_flags[$t]}"
                echo "kit|$cxx $std ${tier_names[$t]} $mode|$cmd|$kit_src|" \
                    >> "$work/jobs"
                expected=$((expected + 1))
            done
        done
    done
done

cat > "$work/one.sh" << 'EOF'
#!/usr/bin/env bash
# one configuration: build, run, and report a line
IFS='|' read -r kind label cmd src libs <<< "$1"
exe="$(mktemp "$2/x.XXXXXX")"
objs=""
# the library's own sources build with the ladder's warnings but not as
# errors: their warnings are their lane's, reported by the ladders; the
# harness itself builds with every warning an error
for lib in $libs; do
    obj="$exe.$(basename "$lib" .c).o"
    # shellcheck disable=SC2086
    if ! log="$(${cmd/-Werror /} -c "$lib" -o "$obj" 2>&1)"; then
        first="$(grep -m1 -E 'error' <<< "$log" | cut -c1-200)"
        echo "FAIL $label: build $lib: $first"
        rm -f "$exe"*; exit 0
    fi
    objs="$objs $obj"
done
# shellcheck disable=SC2086
link=""
[[ -n "$objs" ]] && link="-x none $objs"
# shellcheck disable=SC2086
if ! log="$($cmd "$src" $link -lm -o "$exe" 2>&1)"; then
    first="$(grep -m1 -E 'error|undefined' <<< "$log" | cut -c1-200)"
    echo "FAIL $label: build: $first"
    rm -f "$exe"*; exit 0
fi
rm -f $objs
out="$(ASAN_OPTIONS=allocator_may_return_null=1 timeout 600 "$exe" 2>&1)"
status=$?
rm -f "$exe"
last="$(tail -n 1 <<< "$out")"
if [[ $status -ne 0 || ! "$last" =~ \ 0\ failed$ ]]; then
    first="$(grep -m1 -E 'FAIL|ERROR' <<< "$out" | cut -c1-200)"
    echo "FAIL $label: exit $status: $first"
    exit 0
fi
if [[ "$kind" == kit ]]; then
    tier="$(grep -m1 -o 'tier [0-9]' <<< "$out" | cut -d' ' -f2)"
    if [[ "$label" == *" best "* && "${tier:-0}" -lt 2 ]]; then
        echo "FAIL $label: the best tier is $tier, below the builtins'"
        exit 0
    fi
    echo "ok   $label: tier $tier, $last"
else
    echo "ok   $label: $last"
fi
EOF
chmod +x "$work/one.sh"

tr '\n' '\0' < "$work/jobs" |
    xargs -0 -P "$jobs" -I{} "$work/one.sh" {} "$work" |
    sort > "$work/results"
cat "$work/results"

passed="$(grep -c '^ok ' "$work/results")"
failed="$(grep -c '^FAIL ' "$work/results")"
echo "check_ckd: $passed passed, $failed failed, $skipped skipped, of $expected"

if [[ "$failed" -ne 0 ]]; then
    exit 1
fi
if [[ "$skipped" -ne 0 && "$allow_skip" != 1 ]]; then
    echo "check_ckd: a compiler or level was skipped;" \
         "set ALLOW_SKIP=1 if the gap is known" >&2
    exit 1
fi
if [[ "$passed" -ne "$expected" ]]; then
    echo "check_ckd: $passed configurations reported," \
         "$expected were in the matrix" >&2
    exit 1
fi
exit 0
