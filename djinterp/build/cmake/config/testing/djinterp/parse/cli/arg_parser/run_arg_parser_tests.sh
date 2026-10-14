#!/usr/bin/env bash
#*******************************************************************************
# djinterp [parse]                                       run_arg_parser_tests.sh
#
# Builds the argument parser and runs its suites at every C level, from
# anywhere, with a compiler and nothing else: the script twin of the CMake
# leaf beside it.
#   The parser's C sources are compiled as C, at the C floor, into one
# archive, and the C suites link against it at each level from the floor
# (C99) up. A level the compiler cannot select is left out, and said so.
#   A suite is a table of sections, and a section is what a knob removes, so
# the number run differs by configuration, by design. Each run in the
# default configuration must print the count expected_sections.txt gives its
# level: a section that silently stopped being compiled is a failure, not a
# smaller pass.
#   Every compile takes the ladder's flags (-pedantic-errors -Werror=undef
# -D_XOPEN_SOURCE=700) with -Wall -Wextra beside them, in testing mode, with
# re_std's knob set beside djinterp's. Warnings are counted and shown; they
# do not fail a run.
#   --sanitize builds everything with AddressSanitizer and
# UndefinedBehaviorSanitizer, the latter ending a run at its first finding;
# a leak found at exit fails the run.
#   --matrix then rebuilds everything under each configuration knob, at the
# C floor and the highest level. A knob removes sections, by design, so a
# matrix run is held to "built, ran, nothing failed, something ran" rather
# than to a count, and prints what it ran.
#
#   usage:  run_arg_parser_tests.sh [--matrix] [--sanitize] [--verbose]
#             --matrix    also every configuration knob
#             --sanitize  AddressSanitizer and UndefinedBehaviorSanitizer
#             --verbose   print each section as it runs, not only the totals
#   env:    CC          the compiler           (default: gcc)
#           AR          the archiver           (default: ar)
#           CFLAGS      more compile flags     (default: none)
#           C_LEVELS    C levels               (default: c99 c11 c17 c23)
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
#                                                            revised: 2026.10.13
#*******************************************************************************
set -uo pipefail
export LC_ALL=C

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${HERE}/../../../../../../../.." && pwd)"
CC="${CC:-gcc}"
AR="${AR:-ar}"
C_LEVELS="${C_LEVELS:-c99 c11 c17 c23}"
C_FLOOR="c99"
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
            echo "usage: $(basename "$0") [--matrix] [--sanitize] [--verbose]" >&2
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

# CFLAGS is flags, split deliberately
# shellcheck disable=SC2206
COMMON+=( ${CFLAGS:-} )

if [[ "${sanitize}" -eq 1 ]]; then
    COMMON+=( -g -fsanitize=address,undefined -fno-sanitize-recover=undefined
              -fno-omit-frame-pointer )
fi

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
           "${SUITES_DIR}/arg_smoke_tests.c"
           "${HERE}/arg_parser_c_tests_runner.c" )

WORK="$(mktemp -d "${TMPDIR:-/tmp}/djinterp-arg-parser.XXXXXX")"
trap 'rm -rf "${WORK}"' EXIT

failures=0
warnings=0
totals=""
details=""

# std_flag <compiler> <level>
#   Prints the -std= spelling a compiler takes for a C level: its final name,
# or the draft name a compiler that predates the standard knows it by.
# Prints nothing when the compiler takes neither.
std_flag()
{
    local compiler="$1" level="$2" name=""
    local -a names=( "$2" )

    case "${level}" in
        c23) names+=( "c2x" ) ;;
        c17) names+=( "c18" ) ;;
    esac

    for name in "${names[@]}"; do
        if echo 'int main(void) { return 0; }' |
           "${compiler}" -std="${name}" -x c -fsyntax-only - \
               >/dev/null 2>&1; then
            echo "-std=${name}"
            return 0
        fi
    done

    return 0
}

# expected <level>
#   Prints the section count expected_sections.txt gives a level, or nothing.
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

# knob_level <level> <out directory> <knob flags...>
#   One level under a knob: built, ran, nothing failed, something ran.
# Prints the level and how many sections it ran, on the matrix's line.
knob_level()
{
    local level="$1" verdict=0 ran=""

    run_suites "$@"
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
[[ "${sanitize}" -eq 1 ]] && echo "  with AddressSanitizer and UndefinedBehaviorSanitizer"
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

if [[ "${matrix}" -eq 1 ]]; then
    echo
    echo "== configuration matrix: sections run under each knob =="

    highest="${C_LEVELS##* }"
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
