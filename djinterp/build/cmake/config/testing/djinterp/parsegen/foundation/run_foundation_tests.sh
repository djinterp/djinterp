#!/usr/bin/env bash
#*******************************************************************************
# djinterp [parsegen]                                    run_foundation_tests.sh
#
# Builds the parse/parsegen foundation and runs its suites at every language
# level, from anywhere, with a compiler and nothing else: the script twin of
# the CMake leaf beside it.
#   The C sources are compiled as C, at the C floor, into one archive. The
# C++ suites link against it at each level from the module floor (C++11) up,
# and the C suites at each level from the C floor (C99) up: one library, C
# whatever its consumer is, which is the boundary the foundation exists to
# serve. A level the compiler cannot select is left out, and said so.
#   A suite is a table of sections, and a section is what a level or a knob
# removes, so the number run differs by level, by design. Each run must print
# the count expected_sections.txt gives its level: a section that silently
# stopped being compiled is a failure, not a smaller pass.
#   Every compile takes the ladder's flags (-pedantic-errors -Werror=undef
# -D_XOPEN_SOURCE=700) with -Wall -Wextra beside them, in testing mode, with
# re_std's knob set beside djinterp's. Warnings are counted and shown; they
# do not fail a run.
#   --matrix then rebuilds everything under each configuration knob, at the
# lowest and the highest C++ level and at the C floor. A knob removes
# sections, by design, so a matrix run is held to "built, ran, nothing
# failed, something ran" rather than to a count, and prints what it ran.
#
#   usage:  run_foundation_tests.sh [--matrix] [--verbose]
#             --matrix   also every configuration knob
#             --verbose  print each section as it runs, not only the totals
#   env:    CC, CXX     compilers        (default: gcc, g++)
#           CXX_LEVELS  C++ levels       (default: c++11 c++14 c++17 c++20
#                                         c++23)
#           C_LEVELS    C levels         (default: c99 c11 c17 c23)
#   exit:   0 when every run passed, with its expected count where one is
#           expected; 1 when one did not; 2 when no run could be made
#
# path:      /build/cmake/config/testing/djinterp/parsegen/foundation/run_foundation_tests.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
#                                                            revised: 2026.10.04
#*******************************************************************************
set -uo pipefail
export LC_ALL=C

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${HERE}/../../../../../../.." && pwd)"
CC="${CC:-gcc}"
CXX="${CXX:-g++}"
CXX_LEVELS="${CXX_LEVELS:-c++11 c++14 c++17 c++20 c++23}"
C_LEVELS="${C_LEVELS:-c99 c11 c17 c23}"
C_FLOOR="c99"

matrix=0
verbose=0

for argument in "$@"; do
    case "${argument}" in
        --matrix)  matrix=1 ;;
        --verbose) verbose=1 ;;
        *)
            echo "usage: $(basename "$0") [--matrix] [--verbose]" >&2
            exit 2
            ;;
    esac
done

for tool in "${CC}" "${CXX}" ar; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "not found: ${tool}" >&2
        exit 2
    fi
done

# the ladder's flags, the usual warnings, and testing mode. D_TESTING changes
# the layout of struct d_parse_machine, so the library and every suite take
# it alike; re_std's knob beside it keeps both independent of include order
COMMON=( -O1 -Wall -Wextra -pedantic-errors -Werror=undef
         -D_XOPEN_SOURCE=700 -DD_TESTING=1 -DRE_STD_CFG_TESTING=1
         -I"${ROOT}/inc" )

# re_std's one C source is what the fixed-width integer header declares
C_SOURCES=( "${ROOT}"/src/djinterp/parse/c/*.c
            "${ROOT}"/src/djinterp/parsegen/c/*.c
            "${ROOT}/src/djinterp/c/re_std/dstdint.c" )

CXX_SUITES=( "${ROOT}/tests/djinterp/parse/parse_substrate_tests.cpp"
             "${ROOT}/tests/djinterp/parse/parse_program_tests.cpp"
             "${ROOT}/tests/djinterp/parsegen/parsegen_registry_tests.cpp"
             "${ROOT}/tests/djinterp/parsegen/parsegen_grammar_tests.cpp"
             "${ROOT}/tests/djinterp/parsegen/parsegen_analysis_tests.cpp"
             "${HERE}/foundation_tests_runner.cpp" )

C_SUITES=( "${ROOT}/tests/djinterp/parse/c/parse_c_tests.c"
           "${ROOT}/tests/djinterp/parsegen/c/parsegen_c_tests.c"
           "${HERE}/foundation_c_tests_runner.c" )

WORK="$(mktemp -d "${TMPDIR:-/tmp}/djinterp-foundation.XXXXXX")"
trap 'rm -rf "${WORK}"' EXIT

failures=0
warnings=0
totals=""
details=""

# std_flag <compiler> <language> <level>
#   Prints the -std= spelling a compiler takes for a level: its final name,
# or the draft name a compiler that predates the standard knows it by.
# Prints nothing when the compiler takes neither.
std_flag()
{
    local compiler="$1" language="$2" level="$3" name=""
    local -a names=( "$3" )

    case "${level}" in
        c++23) names+=( "c++2b" ) ;;
        c++20) names+=( "c++2a" ) ;;
        c23)   names+=( "c2x" ) ;;
        c17)   names+=( "c18" ) ;;
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
#   Compiles the C sources as C at the C floor into <out>/foundation.a, from
# nothing, so a stale object can never mask a failed build. Leaves what the
# caller should print in `details`.
build_library()
{
    local out="$1" source="" floor=""
    shift

    details=""
    rm -rf "${out}" && mkdir -p "${out}"
    floor="$(std_flag "${CC}" c "${C_FLOOR}")"

    for source in "${C_SOURCES[@]}"; do
        if ! "${CC}" ${floor} "${COMMON[@]}" "$@" -c "${source}" \
                -o "${out}/$(basename "${source%.c}").o" 2>>"${out}/log"; then
            details="$(grep -m 5 'error' "${out}/log" | sed 's/^/      /')"$'\n'
            return 1
        fi
    done

    ar rcs "${out}/foundation.a" "${out}"/*.o
    note_warnings "${out}/log"
}

# run_suites <compiler> <language> <level> <out directory> [knob flags...]
#   Builds one language's suites at one level against the library in <out>
# and runs them. Leaves the totals line, or why there is none, in `totals`,
# and what the caller should print beneath it in `details`. Returns 0 when
# the suites built, ran and reported no failed section; 2 when the compiler
# cannot select the level; 1 otherwise.
run_suites()
{
    local compiler="$1" language="$2" level="$3" out="$4" flag="" binary=""
    local -a sources=()
    shift 4

    totals=""
    details=""
    flag="$(std_flag "${compiler}" "${language}" "${level}")"
    binary="${out}/suites-${level}"

    if [[ -z "${flag}" ]]; then
        totals="left out"
        return 2
    fi

    if [[ "${language}" == "c" ]]; then
        sources=( "${C_SUITES[@]}" )
    else
        sources=( "${CXX_SUITES[@]}" )
    fi

    if ! "${compiler}" ${flag} "${COMMON[@]}" "$@" "${sources[@]}" \
            "${out}/foundation.a" -o "${binary}" 2>"${binary}.log"; then
        totals="BUILD FAILED"
        details="$(grep -m 5 'error' "${binary}.log" | sed 's/^/      /')"$'\n'
        return 1
    fi

    note_warnings "${binary}.log"

    if ! "${binary}" >"${binary}.out" 2>&1; then
        totals="$(tail -n 1 "${binary}.out")"
        details+="$(grep 'FAIL' "${binary}.out" | sed 's/^/    /')"$'\n'
        return 1
    fi

    totals="$(tail -n 1 "${binary}.out")"

    if [[ "${verbose}" -eq 1 ]]; then
        details+="$(sed '1d;$d' "${binary}.out" | sed 's/^/    /')"$'\n'
    fi
}

# check_level <compiler> <language> <level> <out directory>
#   One level of the default configuration: the suites must pass, and must
# have run exactly the sections expected_sections.txt gives the level.
check_level()
{
    local level="$3" verdict=0 want=""

    run_suites "$@"
    verdict=$?
    want="$(expected "${level}")"

    if [[ ${verdict} -eq 2 ]]; then
        printf "  %-7s left out -- %s cannot select it\n" "${level}" "$1"
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

# knob_level <compiler> <language> <level> <out directory> <knob flags...>
#   One level under a knob: built, ran, nothing failed, something ran. Prints
# the level and how many sections it ran, on the matrix's line.
knob_level()
{
    local level="$3" verdict=0 ran=""

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

echo "djinterp parse/parsegen foundation"
echo "  $("${CC}" --version | head -n 1)"
echo "  $("${CXX}" --version | head -n 1)"
echo
echo "== default configuration: every level =="

if ! build_library "${WORK}/default"; then
    echo "  the library did not build at ${C_FLOOR}"
    printf "%s" "${details}"
    exit 1
fi

printf "%s" "${details}"

for level in ${CXX_LEVELS}; do
    check_level "${CXX}" c++ "${level}" "${WORK}/default"
done

for level in ${C_LEVELS}; do
    check_level "${CC}" c "${level}" "${WORK}/default"
done

if [[ "${matrix}" -eq 1 ]]; then
    echo
    echo "== configuration matrix: sections run under each knob =="

    lowest="${CXX_LEVELS%% *}"
    highest="${CXX_LEVELS##* }"

    knobs=( "-DD_CFG_PARSE_ALL=0 -DD_CFG_PARSEGEN_ALL=0"
            "-DD_CFG_PARSE_ALL=0"
            "-DD_CFG_PARSEGEN_ALL=0"
            "-DD_CFG_PARSEGEN_GRAMMAR_HEAP=0"
            "-DD_CFG_PARSEGEN_REGISTRY_HEAP=0"
            "-DD_CFG_PARSE_POOL_HEAP=0"
            "-DD_CFG_PARSE_PROGRAM_HEAP=0"
            "-DD_CFG_PARSE_PROGRAM_TRANSPORT=0"
            "-DD_CFG_PARSE_OP_SET_HEAP=0"
            "-DD_CFG_PARSE_DIAG_HEAP=0"
            "-DD_CFG_PARSE_DIAG_FORMAT=0"
            "-DD_CFG_PARSE_MACHINE_TRACE=0"
            "-DD_CFG_PARSE_MACHINE_TRACE="
            "-DD_CFG_NO_TESTING_PRESET" )

    for knob in "${knobs[@]}"; do
        printf "  %-44s" "${knob}"
        verdict=0

        # word-split the knob deliberately: some entries set two flags
        # shellcheck disable=SC2086
        if build_library "${WORK}/knob" ${knob}; then
            # shellcheck disable=SC2086
            knob_level "${CXX}" c++ "${lowest}" "${WORK}/knob" ${knob} ||
                verdict=1

            if [[ "${highest}" != "${lowest}" ]]; then
                # shellcheck disable=SC2086
                knob_level "${CXX}" c++ "${highest}" "${WORK}/knob" ${knob} ||
                    verdict=1
            fi

            # shellcheck disable=SC2086
            knob_level "${CC}" c "${C_FLOOR}" "${WORK}/knob" ${knob} ||
                verdict=1
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
