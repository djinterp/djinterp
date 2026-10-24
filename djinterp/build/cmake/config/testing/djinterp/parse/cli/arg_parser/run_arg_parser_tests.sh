#!/usr/bin/env bash
#*******************************************************************************
# djinterp [parse]                                       run_arg_parser_tests.sh
#
# Builds the argument parser and runs its suites at every C and C++ level,
# from anywhere, with a compiler and nothing else: the script twin of the
# CMake leaf beside it.
#   The parser's C sources are compiled as C, at the C floor, into one
# archive, and the C suites link against it at each level from the floor
# (C99) up; then the C++ suites -- the C++ face, called from C++ across a
# real language boundary -- at each C++ level from the face's floor (C++98,
# in the framework's strict ISO mode) up, and again without exceptions at
# the lowest and the highest C++ level. Where the linker wraps symbols
# (--wrap), an allocation sweep runs the C API's scenarios with every
# allocation refused in turn, at the C floor. A level the compiler cannot
# select is left out, and said so; so are the C++ suites when there is no
# C++ compiler, and the sweep when the linker cannot wrap.
#   A suite is a table of sections, and a section is what a knob removes --
# or a language level, or exceptions turned off -- so the number run
# differs by configuration, by design. Each run in the default
# configuration must print the count expected_sections.txt gives its build:
# a section that silently stopped being compiled is a failure, not a
# smaller pass.
#   Every compile takes the ladder's flags (-pedantic-errors -Werror=undef
# -D_XOPEN_SOURCE=700) with -Wall -Wextra beside them, in testing mode, with
# re_std's knob set beside djinterp's. Warnings are counted and shown; they
# do not fail a run.
#   --sanitize builds everything with AddressSanitizer and
# UndefinedBehaviorSanitizer, the latter ending a run at its first finding;
# a leak found at exit fails the run.
#   --matrix then rebuilds everything under each configuration knob, at the
# C floor and the highest C level, the C++ floor and the highest C++ level,
# and the sweep. A knob removes sections, by design, so a matrix run is
# held to "built, ran, nothing failed, something ran" rather than to a
# count, and prints what it ran.
#
#   usage:  run_arg_parser_tests.sh [--matrix] [--sanitize] [--verbose]
#             --matrix    also every configuration knob
#             --sanitize  AddressSanitizer and UndefinedBehaviorSanitizer
#             --verbose   print each section as it runs, not only the totals
#   env:    CC          the C compiler         (default: gcc)
#           CXX         the C++ compiler       (default: g++; one that is
#                       not found leaves the C++ suites out)
#           AR          the archiver           (default: ar)
#           CFLAGS      more C compile flags   (default: none)
#           CXXFLAGS    more C++ compile flags (default: none)
#           C_LEVELS    C levels               (default: c99 c11 c17 c23)
#           CXX_LEVELS  C++ levels             (default: c++98 c++11 c++14
#                                               c++17 c++20 c++23)
#           RUN_PREFIX  what runs a test       (default: nothing; e.g.
#                       `env LC_ALL=C.UTF-8 wine` for a Windows build: the
#                       script runs in the C locale, under which Wine cannot
#                       keep a file name beyond ASCII, which the suites make)
#           EXE_SUFFIX  a test's file suffix   (default: nothing; .exe for
#                       a Windows build)
#   exit:   0 when every run passed, with its expected count where one is
#           expected; 1 when one did not; 2 when no run could be made
#
# path:      /build/cmake/config/testing/djinterp/parse/cli/arg_parser/run_arg_parser_tests.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
#                                                            revised: 2026.10.23
#*******************************************************************************
set -uo pipefail
export LC_ALL=C

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${HERE}/../../../../../../../.." && pwd)"
CC="${CC:-gcc}"
CXX="${CXX:-g++}"
AR="${AR:-ar}"
C_LEVELS="${C_LEVELS:-c99 c11 c17 c23}"
C_FLOOR="c99"
CXX_LEVELS="${CXX_LEVELS:-c++98 c++11 c++14 c++17 c++20 c++23}"
RUN_PREFIX="${RUN_PREFIX:-}"
EXE_SUFFIX="${EXE_SUFFIX:-}"

matrix=0
sanitize=0
verbose=0

for argument in "$@"; do
    case "${argument}" in
        --matrix)   matrix=1 ;;
        --sanitize) sanitize=1 ;;
        --verbose)  verbose=1 ;;
        *)
            echo "usage: $(basename "$0")" \
                 "[--matrix] [--sanitize] [--verbose]" >&2
            exit 2
            ;;
    esac
done

for tool in "${CC}" "${AR}"; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "not found: ${tool}" >&2
        exit 2
    fi
done

# the ladder's flags, the usual warnings, and testing mode, which the
# library and every suite take alike; re_std's knob beside it keeps both
# independent of include order
COMMON=( -O1 -Wall -Wextra -pedantic-errors -Werror=undef
         -D_XOPEN_SOURCE=700 -DD_TESTING=1 -DRE_STD_CFG_TESTING=1
         -I"${ROOT}/inc" )

if [[ "${sanitize}" -eq 1 ]]; then
    COMMON+=( -g -fsanitize=address,undefined -fno-sanitize-recover=undefined
              -fno-omit-frame-pointer )
fi

# the C++ suites take the same, and their own extra flags; C++98 without
# extensions is the framework's strict ISO mode, in which its variadic
# macros, an extension there, give way to ISO forms
CXX_COMMON=( "${COMMON[@]}" )
STRICT_ISO=( -DD_CFG_ENV_ISO_STRICT=1 -DRE_STD_CFG_ISO_STRICT=1 )

# CFLAGS and CXXFLAGS are flags, split deliberately
# shellcheck disable=SC2206
COMMON+=( ${CFLAGS:-} )
# shellcheck disable=SC2206
CXX_COMMON+=( ${CXXFLAGS:-} )

# a C++ compiler that is not there leaves the C++ suites out, as the CMake
# leaf leaves them out without one
cxx_found=1

if ! command -v "${CXX}" >/dev/null 2>&1; then
    cxx_found=0
fi

# a Windows C++ suite is linked whole -- libstdc++, libgcc, winpthreads --
# so that it runs where those DLLs are not, as under Wine they are not
CXX_STATIC=()

case "$("${CC}" -dumpmachine 2>/dev/null)" in
    *mingw*|*windows*|*cygwin*|*msys*) CXX_STATIC=( -static ) ;;
esac

# the parser's sources, the sinks it writes through, and re_std's one C
# source, which is what the fixed-width integer header declares
C_SOURCES=( "${ROOT}"/src/djinterp/parse/cli/arg_parser/c/*.c
            "${ROOT}/src/djinterp/c/util/sink_common.c"
            "${ROOT}/src/djinterp/c/re_std/dstdint.c" )

SUITES_DIR="${ROOT}/tests/djinterp/parse/cli/arg_parser/c"
C_SUITES=( "${SUITES_DIR}/arg_tests_support.c"
           "${SUITES_DIR}/arg_tests_files.c"
           "${SUITES_DIR}/arg_common_tests.c"
           "${SUITES_DIR}/arg_type_tests.c"
           "${SUITES_DIR}/arg_validator_tests.c"
           "${SUITES_DIR}/arg_validator_extra_tests.c"
           "${SUITES_DIR}/arg_validator_member_tests.c"
           "${SUITES_DIR}/arg_validator_unit_tests.c"
           "${SUITES_DIR}/arg_validator_path_tests.c"
           "${SUITES_DIR}/arg_option_tests.c"
           "${SUITES_DIR}/arg_command_tests.c"
           "${SUITES_DIR}/arg_formatter_tests.c"
           "${SUITES_DIR}/arg_parse_tests.c"
           "${SUITES_DIR}/arg_process_tests.c"
           "${SUITES_DIR}/arg_config_tests.c"
           "${SUITES_DIR}/arg_argv_tests.c"
           "${SUITES_DIR}/arg_parser_tests.c"
           "${SUITES_DIR}/arg_suggest_tests.c"
           "${SUITES_DIR}/arg_completion_tests.c"
           "${SUITES_DIR}/arg_smoke_tests.c"
           "${HERE}/arg_parser_c_tests_runner.c" )

# the C++ suites; the scratch directories their cases make files in are C,
# compiled with the library
CXX_DIR="${ROOT}/tests/djinterp/parse/cli/arg_parser"
CXX_SUITES=( "${CXX_DIR}/arg_tests_support.cpp"
             "${CXX_DIR}/arg_common_tests.cpp"
             "${CXX_DIR}/arg_type_tests.cpp"
             "${CXX_DIR}/arg_validator_tests.cpp"
             "${CXX_DIR}/arg_option_tests.cpp"
             "${CXX_DIR}/arg_command_tests.cpp"
             "${CXX_DIR}/arg_formatter_tests.cpp"
             "${CXX_DIR}/arg_config_tests.cpp"
             "${CXX_DIR}/arg_argv_tests.cpp"
             "${CXX_DIR}/arg_parser_tests.cpp"
             "${CXX_DIR}/arg_completion_tests.cpp"
             "${CXX_DIR}/arg_boundary_tests.cpp"
             "${CXX_DIR}/arg_examples_tests.cpp"
             "${HERE}/arg_parser_cpp_tests_runner.cpp" )

WORK="$(mktemp -d "${TMPDIR:-/tmp}/djinterp-arg-parser.XXXXXX")"
trap 'rm -rf "${WORK}"' EXIT

failures=0
warnings=0
totals=""
details=""

# std_flag <compiler> <level>
#   Prints the -std= spelling a compiler takes for a C or C++ level: its
# final name, or the draft name a compiler that predates the standard knows
# it by. Prints nothing when the compiler takes neither.
std_flag()
{
    local compiler="$1" level="$2" name="" language="c"
    local -a names=( "$2" )

    case "${level}" in
        c23)   names+=( "c2x" ) ;;
        c17)   names+=( "c18" ) ;;
        c++23) names+=( "c++2b" ) ;;
        c++20) names+=( "c++2a" ) ;;
        c++17) names+=( "c++1z" ) ;;
        c++14) names+=( "c++1y" ) ;;
        c++11) names+=( "c++0x" ) ;;
    esac

    case "${level}" in
        c++*) language="c++" ;;
    esac

    for name in "${names[@]}"; do
        if echo 'int main(void) { return 0; }' |
           "${compiler}" -std="${name}" -x "${language}" -fsyntax-only - \
               >/dev/null 2>&1; then
            echo "-std=${name}"
            return 0
        fi
    done

    return 0
}

# expected <build>
#   Prints the section count expected_sections.txt gives a build -- a level,
# and `-noexc` after a C++ one without exceptions -- or nothing.
expected()
{
    awk -v level="$1" '$1 == level { print $2 }' \
        "${HERE}/expected_sections.txt"
}

# note_warnings <log>
#   Adds a compile log's warnings to the tally, and its first few to the
# details the caller prints.
note_warnings()
{
    local count=0

    count="$(grep -c 'warning:' "$1" 2>/dev/null || true)"
    warnings=$((warnings + ${count:-0}))

    if [[ "${count:-0}" -gt 0 ]]; then
        details+="$(grep -m 3 'warning:' "$1" | sed 's/^/      /')"$'\n'
    fi
}

# build_library <out directory> [knob flags...]
#   Compiles the C sources as C at the C floor into <out>/arg_parser.a, from
# nothing, so a stale object can never mask a failed build. Leaves what the
# caller should print in `details`.
build_library()
{
    local out="$1" source="" floor=""
    shift

    details=""
    rm -rf "${out}" && mkdir -p "${out}"
    floor="$(std_flag "${CC}" "${C_FLOOR}")"

    for source in "${C_SOURCES[@]}"; do
        if ! "${CC}" ${floor} "${COMMON[@]}" "$@" -c "${source}" \
                -o "${out}/$(basename "${source%.c}").o" 2>>"${out}/log"; then
            details="$(grep -m 5 'error' "${out}/log" | sed 's/^/      /')"$'\n'
            return 1
        fi
    done

    "${AR}" rcs "${out}/arg_parser.a" "${out}"/*.o

    # the C++ suites' one C source, beside the archive rather than in it
    if [[ "${cxx_found}" -eq 1 ]]; then
        mkdir -p "${out}/cxx"

        if ! "${CC}" ${floor} "${COMMON[@]}" "$@" \
                -c "${CXX_DIR}/c/arg_tests_files.c" \
                -o "${out}/cxx/arg_tests_files.o" 2>>"${out}/log"; then
            details="$(grep -m 5 'error' "${out}/log" | sed 's/^/      /')"$'\n'
            return 1
        fi
    fi

    note_warnings "${out}/log"
}

# run_suites <level> <out directory> [knob flags...]
#   Builds the suites at one level against the library in <out> and runs
# them. Leaves the totals line, or why there is none, in `totals`, and what
# the caller should print beneath it in `details`. Returns 0 when the suites
# built, ran and reported no failed section; 2 when the compiler cannot
# select the level; 1 otherwise.
run_suites()
{
    local level="$1" out="$2" flag="" binary=""
    shift 2

    totals=""
    details=""
    flag="$(std_flag "${CC}" "${level}")"
    binary="${out}/suites-${level}${EXE_SUFFIX}"

    if [[ -z "${flag}" ]]; then
        totals="left out"
        return 2
    fi

    if ! "${CC}" ${flag} "${COMMON[@]}" "$@" "${C_SUITES[@]}" \
            "${out}/arg_parser.a" -o "${binary}" 2>"${binary}.log"; then
        totals="BUILD FAILED"
        details="$(grep -m 5 'error' "${binary}.log" | sed 's/^/      /')"$'\n'
        return 1
    fi

    note_warnings "${binary}.log"

    # RUN_PREFIX is a command and its arguments, split deliberately
    # shellcheck disable=SC2086
    if ! ${RUN_PREFIX} "${binary}" 2>&1 | tr -d '\r' >"${binary}.out"; then
        totals="$(tail -n 1 "${binary}.out")"
        details+="$(grep -v '^  \[PASS\]' "${binary}.out" | sed '1d' |
                    sed 's/^/    /')"$'\n'
        return 1
    fi

    totals="$(tail -n 1 "${binary}.out")"

    if [[ "${verbose}" -eq 1 ]]; then
        details+="$(sed '1d;$d' "${binary}.out" | sed 's/^/    /')"$'\n'
    fi
}

# run_cxx_suites <level> <out directory> [flags...]
#   Builds the C++ suites at one C++ level against the library in <out> --
# the C++98 floor in strict ISO -- and runs them, leaving `totals` and
# `details` as run_suites does. The flags are the knob's, or the switch
# that turns exceptions off. Returns as run_suites does.
run_cxx_suites()
{
    local level="$1" out="$2" flag="" binary="" tag=""
    local -a strict=()
    shift 2

    totals=""
    details=""
    flag="$(std_flag "${CXX}" "${level}")"
    tag="${level}"

    case " $* " in
        *" -fno-exceptions "*) tag="${level}-noexc" ;;
    esac

    binary="${out}/cxx/suites-${tag}${EXE_SUFFIX}"

    if [[ -z "${flag}" ]]; then
        totals="left out"
        return 2
    fi

    if [[ "${level}" == "c++98" ]]; then
        strict=( "${STRICT_ISO[@]}" )
    fi

    if ! "${CXX}" ${flag} "${CXX_COMMON[@]}" "${strict[@]+"${strict[@]}"}" \
            "$@" "${CXX_SUITES[@]}" "${out}/cxx/arg_tests_files.o" \
            "${out}/arg_parser.a" "${CXX_STATIC[@]+"${CXX_STATIC[@]}"}" \
            -o "${binary}" 2>"${binary}.log"; then
        totals="BUILD FAILED"
        details="$(grep -m 5 'error' "${binary}.log" | sed 's/^/      /')"$'\n'
        return 1
    fi

    note_warnings "${binary}.log"

    # RUN_PREFIX is a command and its arguments, split deliberately
    # shellcheck disable=SC2086
    if ! ${RUN_PREFIX} "${binary}" 2>&1 | tr -d '\r' >"${binary}.out"; then
        totals="$(tail -n 1 "${binary}.out")"
        totals="${totals:-nothing printed: it did not start, or died at once}"
        details+="$(grep -v '^  \[PASS\]' "${binary}.out" | sed '1d' |
                    sed 's/^/    /')"$'\n'
        return 1
    fi

    totals="$(tail -n 1 "${binary}.out")"

    if [[ "${verbose}" -eq 1 ]]; then
        details+="$(sed '1d;$d' "${binary}.out" | sed 's/^/    /')"$'\n'
    fi
}

# wraps_symbols
#   Whether the C compiler's linker wraps a symbol, as GNU ld's and lld's
# --wrap do, which the allocation sweep needs.
wraps_symbols()
{
    local probe="${WORK}/wrap"

    mkdir -p "${probe}"
    printf '%s\n' '#include <stddef.h>' \
        'void* __real_malloc(size_t _size);' \
        'void* __wrap_malloc(size_t _size) { return __real_malloc(_size); }' \
        'int main(void) { return 0; }' >"${probe}/wrap.c"

    "${CC}" "${probe}/wrap.c" -Wl,--wrap=malloc \
        -o "${probe}/wrap${EXE_SUFFIX}" >/dev/null 2>&1
}

# run_sweep <out directory> [knob flags...]
#   Builds the allocation sweep at the C floor against the library in
# <out>, malloc, calloc, realloc and free wrapped, and runs it, leaving
# `totals` and `details` as run_suites does. Returns as run_suites does.
run_sweep()
{
    local out="$1" flag="" binary=""
    shift

    totals=""
    details=""
    flag="$(std_flag "${CC}" "${C_FLOOR}")"
    binary="${out}/sweep${EXE_SUFFIX}"

    if ! "${CC}" ${flag} "${COMMON[@]}" "$@" \
            "${SUITES_DIR}/arg_sweep_tests.c" \
            "${HERE}/arg_parser_c_sweep_runner.c" "${out}/arg_parser.a" \
            -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc \
            -Wl,--wrap=free -o "${binary}" 2>"${binary}.log"; then
        totals="BUILD FAILED"
        details="$(grep -m 5 'error' "${binary}.log" | sed 's/^/      /')"$'\n'
        return 1
    fi

    note_warnings "${binary}.log"

    # RUN_PREFIX is a command and its arguments, split deliberately
    # shellcheck disable=SC2086
    if ! ${RUN_PREFIX} "${binary}" 2>&1 | tr -d '\r' >"${binary}.out"; then
        totals="$(tail -n 1 "${binary}.out")"
        details+="$(grep -v '^  \[PASS\]' "${binary}.out" | sed '1d' |
                    sed 's/^/    /')"$'\n'
        return 1
    fi

    totals="$(tail -n 1 "${binary}.out")"

    if [[ "${verbose}" -eq 1 ]]; then
        details+="$(sed '1d;$d' "${binary}.out" | sed 's/^/    /')"$'\n'
    fi
}

# check_level <level> <out directory>
#   One level of the default configuration: the suites must pass, and must
# have run exactly the sections expected_sections.txt gives the level.
check_level()
{
    local level="$1" verdict=0 want=""

    run_suites "$@"
    verdict=$?
    want="$(expected "${level}")"

    if [[ ${verdict} -eq 2 ]]; then
        printf "  %-7s left out -- %s cannot select it\n" "${level}" "${CC}"
        return 0
    fi

    printf "  %-7s %s\n" "${level}" "${totals}"
    printf "%s" "${details}"

    if [[ ${verdict} -ne 0 ]]; then
        failures=$((failures + 1))
    elif [[ "${totals}" != "passed: ${want}   failed: 0" ]]; then
        printf "          expected %s sections: see expected_sections.txt\n" \
               "${want:-a count that file does not give}"
        failures=$((failures + 1))
    fi
}

# check_cxx <build> <level> <out directory> [flags...]
#   One C++ build of the default configuration -- a level, or a level
# without exceptions -- held as check_level holds a C level, to the count
# expected_sections.txt gives the build.
check_cxx()
{
    local build="$1" verdict=0 want=""
    shift

    run_cxx_suites "$@"
    verdict=$?
    want="$(expected "${build}")"

    if [[ ${verdict} -eq 2 ]]; then
        printf "  %-13s left out -- %s cannot select it\n" "${build}" "${CXX}"
        return 0
    fi

    printf "  %-13s %s\n" "${build}" "${totals}"
    printf "%s" "${details}"

    if [[ ${verdict} -ne 0 ]]; then
        failures=$((failures + 1))
    elif [[ "${totals}" != "passed: ${want}   failed: 0" ]]; then
        printf "                expected %s sections: see " \
               "${want:-a count that file does not give}"
        printf "expected_sections.txt\n"
        failures=$((failures + 1))
    fi
}

# knob_level <level> <out directory> <knob flags...>
#   One level under a knob, C or C++, or `sweep` for the allocation sweep:
# built, ran, nothing failed, something ran. Prints the level and how many
# sections it ran, on the matrix's line.
knob_level()
{
    local level="$1" verdict=0 ran=""

    case "${level}" in
        c++*)  run_cxx_suites "$@" ;;
        sweep) shift
               run_sweep "$@" ;;
        *)     run_suites "$@" ;;
    esac

    verdict=$?
    ran="$(sed -n 's/^passed: \([0-9][0-9]*\)   failed: 0$/\1/p' \
               <<<"${totals}")"

    if [[ ${verdict} -eq 2 ]]; then
        printf "  %s -" "${level}"
        return 0
    fi

    if [[ ${verdict} -ne 0 || -z "${ran}" || "${ran}" -eq 0 ]]; then
        printf "  %s FAIL" "${level}"
        return 1
    fi

    printf "  %s %s" "${level}" "${ran}"
}

echo "djinterp argument parser"
echo "  $("${CC}" --version | head -n 1)"
if [[ "${sanitize}" -eq 1 ]]; then
    echo "  with AddressSanitizer and UndefinedBehaviorSanitizer"
fi
echo
echo "== default configuration: every level =="

if ! build_library "${WORK}/default"; then
    echo "  the library did not build at ${C_FLOOR}"
    printf "%s" "${details}"
    exit 1
fi

printf "%s" "${details}"

for level in ${C_LEVELS}; do
    check_level "${level}" "${WORK}/default"
done

echo
echo "== default configuration: the allocation sweep =="

sweeps=0

if ! wraps_symbols; then
    echo "  left out -- the linker of ${CC} has no --wrap"
else
    sweeps=1
    run_sweep "${WORK}/default"
    verdict=$?
    want="$(expected "${C_FLOOR}-sweep")"
    printf "  %-13s %s\n" "${C_FLOOR}-sweep" "${totals}"
    printf "%s" "${details}"

    if [[ ${verdict} -ne 0 ]]; then
        failures=$((failures + 1))
    elif [[ "${totals}" != "passed: ${want}   failed: 0" ]]; then
        printf "                expected %s sections: see " \
               "${want:-a count that file does not give}"
        printf "expected_sections.txt\n"
        failures=$((failures + 1))
    fi
fi

echo
echo "== default configuration: every C++ level =="

if [[ "${cxx_found}" -eq 0 ]]; then
    echo "  left out -- ${CXX} not found"
else
    echo "  $("${CXX}" --version | head -n 1)"
    cxx_lowest=""
    cxx_highest=""

    for level in ${CXX_LEVELS}; do
        check_cxx "${level}" "${level}" "${WORK}/default"

        # the levels the compiler selects, for the builds without exceptions
        if [[ -n "$(std_flag "${CXX}" "${level}")" ]]; then
            cxx_lowest="${cxx_lowest:-${level}}"
            cxx_highest="${level}"
        fi
    done

    for level in ${cxx_lowest} ${cxx_highest}; do
        # one level selected is one build without exceptions
        if [[ "${level}" == "${cxx_lowest}" && "${level}" == "${cxx_highest}" &&
              -n "${noexc_done:-}" ]]; then
            continue
        fi

        noexc_done=1
        check_cxx "${level}-noexc" "${level}" "${WORK}/default" -fno-exceptions
    done
fi

if [[ "${matrix}" -eq 1 ]]; then
    echo
    echo "== configuration matrix: sections run under each knob =="

    highest="${C_LEVELS##* }"
    cxx_floor="${cxx_lowest:-}"
    cxx_top="${cxx_highest:-}"
    knobs=( "-DD_CFG_ARG_PARSER_ALL=0"
            "-DD_CFG_ARG_PARSER_CONFIG_FILES=0"
            "-DD_CFG_ARG_PARSER_ENVIRONMENT=0"
            "-DD_CFG_ARG_PARSER_EXTRA_VALIDATORS=0"
            "-DD_CFG_ARG_PARSER_PATH_VALIDATORS=0"
            "-DD_CFG_ARG_PARSER_RESPONSE_FILES=0"
            "-DD_CFG_ARG_PARSER_SUGGESTIONS=0"
            "-DD_CFG_ARG_PARSER_TERMINAL_WIDTH=0"
            "-DD_CFG_ARG_PARSER_COMPLETION=0"
            "-DD_CFG_ARG_PARSER_WIDE_ARGV=0"
            "-DD_CFG_NO_TESTING_PRESET" )

    for knob in "${knobs[@]}"; do
        printf "  %-40s" "${knob}"
        verdict=0

        # shellcheck disable=SC2086
        if build_library "${WORK}/knob" ${knob}; then
            # shellcheck disable=SC2086
            knob_level "${C_FLOOR}" "${WORK}/knob" ${knob} || verdict=1

            if [[ "${highest}" != "${C_FLOOR}" ]]; then
                # shellcheck disable=SC2086
                knob_level "${highest}" "${WORK}/knob" ${knob} || verdict=1
            fi

            # the C++ suites, at their floor and their highest level
            if [[ -n "${cxx_floor}" ]]; then
                # shellcheck disable=SC2086
                knob_level "${cxx_floor}" "${WORK}/knob" ${knob} || verdict=1
            fi

            if [[ -n "${cxx_top}" && "${cxx_top}" != "${cxx_floor}" ]]; then
                # shellcheck disable=SC2086
                knob_level "${cxx_top}" "${WORK}/knob" ${knob} || verdict=1
            fi

            # the allocation sweep, where the linker wraps
            if [[ "${sweeps}" -eq 1 ]]; then
                # shellcheck disable=SC2086
                knob_level sweep "${WORK}/knob" ${knob} || verdict=1
            fi
        else
            printf " the library did not build"
            verdict=1
        fi

        if [[ ${verdict} -eq 0 ]]; then
            printf "   pass\n"
        else
            printf "   FAIL\n"
            failures=$((failures + 1))
        fi
    done
fi

echo
echo "runs failing: ${failures}   warnings: ${warnings}"

[[ ${failures} -eq 0 ]]
