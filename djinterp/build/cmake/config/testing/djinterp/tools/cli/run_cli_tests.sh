#!/usr/bin/env bash
#*******************************************************************************
# djinterp [tools]                                              run_cli_tests.sh
#
# Builds the command-line kit and runs its suites at every C and C++ level,
# from anywhere, with a compiler and nothing else: the script twin of the
# CMake leaf beside it.
#   The argument parser's C sources and the kit's, with filesys, are compiled
# as C, at the C floor, into one archive -- position-independent where the
# target has such a thing, so that a module links it; then the probe
# modules the plugin suites load, `prog-KIND`, each its entry point and
# nothing else exported; then the C suites at each level from the floor
# (C99) up; where the linker wraps symbols (--wrap), the allocation sweep at
# the floor, every allocation of the kit's cases refused in turn; and the
# C++ suite -- the C++ face, called from C++ across a real language
# boundary -- at each C++ level from the face's floor (C++98, in the
# framework's strict ISO mode) up, and again without exceptions at the
# lowest and the highest. Each run is given the probes' directory. A level
# the compiler cannot select is left out, and said so; so is the C++ suite
# when there is no C++ compiler, and the sweep when the linker cannot wrap.
#   Each run in the default configuration must print the count
# expected_sections.txt gives its build: a section that silently stopped
# being compiled is a failure, not a smaller pass.
#   Every compile takes the ladder's flags (-pedantic-errors -Werror=undef
# -D_XOPEN_SOURCE=700) with -Wall -Wextra beside them, in testing mode, with
# re_std's knob set beside djinterp's. Warnings are counted and shown; they
# do not fail a run. --sanitize builds everything with AddressSanitizer and
# UndefinedBehaviorSanitizer, the latter ending a run at its first finding;
# a leak found at exit fails the run.
#   A Windows build is a cross build: its modules are DLLs, which export
# their entry point alone by its declaration; its C++ suites are linked
# statically, needing no DLL of the compiler's; its tests run under
# RUN_PREFIX.
#
#   usage:  run_cli_tests.sh [--sanitize] [--verbose]
#             --sanitize  AddressSanitizer and UndefinedBehaviorSanitizer
#             --verbose   print each section as it runs, not only the totals
#   env:    CC          the C compiler         (default: gcc)
#           CXX         the C++ compiler       (default: g++; one that is
#                       not found leaves the C++ suite out)
#           AR          the archiver           (default: ar)
#           CFLAGS      more C compile flags   (default: none)
#           CXXFLAGS    more C++ compile flags (default: none)
#           C_LEVELS    C levels               (default: c99 c11 c17 c23)
#           CXX_LEVELS  C++ levels             (default: c++98 c++11 c++14
#                                               c++17 c++20 c++23)
#           RUN_PREFIX  what runs a test       (default: nothing; e.g.
#                       `env LC_ALL=C.UTF-8 wine` for a Windows build)
#           EXE_SUFFIX  a test's file suffix   (default: nothing; .exe for
#                       a Windows build)
#   exit:   0 when every run passed with its expected count; 1 when one did
#           not; 2 when no run could be made
#
# path:      /build/cmake/config/testing/djinterp/tools/cli/run_cli_tests.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
#                                                            revised: 2026.10.25
#*******************************************************************************
set -uo pipefail
export LC_ALL=C

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${HERE}/../../../../../../.." && pwd)"
CC="${CC:-gcc}"
CXX="${CXX:-g++}"
AR="${AR:-ar}"
C_LEVELS="${C_LEVELS:-c99 c11 c17 c23}"
C_FLOOR="c99"
CXX_LEVELS="${CXX_LEVELS:-c++98 c++11 c++14 c++17 c++20 c++23}"
RUN_PREFIX="${RUN_PREFIX:-}"
EXE_SUFFIX="${EXE_SUFFIX:-}"

sanitize=0
verbose=0

for argument in "$@"; do
    case "${argument}" in
        --sanitize) sanitize=1 ;;
        --verbose)  verbose=1 ;;
        *)
            echo "usage: $(basename "$0") [--sanitize] [--verbose]" >&2
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

# what the target is: a Windows target has DLLs and no position-independent
# code to ask for; a Linux one may keep the loader in libdl
TARGET="$("${CC}" -dumpmachine 2>/dev/null)"
WINDOWS=0
MODULE_SUFFIX=".so"
PIC=( -fPIC )
HIDE=( -fvisibility=hidden )
KEEP_IN=( -Wl,--exclude-libs,ALL )
LOADER=()
CXX_STATIC=()

# a Windows C++ suite is linked whole -- libstdc++, libgcc, winpthreads --
# so that it runs where those DLLs are not, as under Wine they are not
case "${TARGET}" in
    *mingw*|*windows*|*cygwin*|*msys*)
        WINDOWS=1
        MODULE_SUFFIX=".dll"
        PIC=()
        HIDE=()
        KEEP_IN=()
        CXX_STATIC=( -static )
        ;;
    *apple*|*darwin*)
        KEEP_IN=( -Wl,-exported_symbol,_d_cli_plugin_entry )
        ;;
    *linux*)
        LOADER=( -ldl )
        ;;
esac

# the ladder's flags, the usual warnings, and testing mode, which the
# libraries and every suite take alike
COMMON=( -O1 -Wall -Wextra -pedantic-errors -Werror=undef
         -D_XOPEN_SOURCE=700 -DD_TESTING=1 -DRE_STD_CFG_TESTING=1
         -I"${ROOT}/inc" )

if [[ "${sanitize}" -eq 1 ]]; then
    COMMON+=( -g -fsanitize=address,undefined -fno-sanitize-recover=undefined
              -fno-omit-frame-pointer )
fi

CXX_COMMON=( "${COMMON[@]}" )
STRICT_ISO=( -DD_CFG_ENV_ISO_STRICT=1 -DRE_STD_CFG_ISO_STRICT=1 )

# CFLAGS and CXXFLAGS are flags, split deliberately
# shellcheck disable=SC2206
COMMON+=( ${CFLAGS:-} )
# shellcheck disable=SC2206
CXX_COMMON+=( ${CXXFLAGS:-} )

cxx_found=1

if ! command -v "${CXX}" >/dev/null 2>&1; then
    cxx_found=0
fi

# the parser's sources, the sinks it writes through, re_std's one C source;
# the kit's, and filesys
LIB_SOURCES=( "${ROOT}"/src/djinterp/c/modules/parse/cli/arg_parser/*.c
              "${ROOT}/src/djinterp/c/util/sink_common.c"
              "${ROOT}/src/djinterp/c/re_std/dstdint.c"
              "${ROOT}"/src/djinterp/tools/cli/c/*.c
              "${ROOT}"/src/djinterp/c/filesys/file_*.c )

TESTS="${ROOT}/tests/djinterp/tools/cli"
PARSER_TESTS="${ROOT}/tests/djinterp/parse/cli/arg_parser"
PROBE="${TESTS}/c/probe/cli_tests_probe.c"
PROBES=( probe other old abi short idle null none kit )
C_SUITES=( "${TESTS}/c/cli_tests_support.c"
           "${PARSER_TESTS}/c/arg_tests_files.c"
           "${TESTS}/c/cli_io_tests.c"
           "${TESTS}/c/cli_format_tests.c"
           "${TESTS}/c/cli_program_tests.c"
           "${TESTS}/c/cli_edit_tests.c"
           "${TESTS}/c/cli_operand_tests.c"
           "${TESTS}/c/cli_plugin_tests.c"
           "${HERE}/cli_c_tests_runner.c" )
SWEEP=( "${TESTS}/c/cli_sweep_tests.c"
        "${TESTS}/c/cli_tests_support.c"
        "${PARSER_TESTS}/c/arg_tests_files.c"
        "${HERE}/cli_c_sweep_runner.c" )
CXX_SUITES=( "${TESTS}/cli_tests.cpp"
             "${HERE}/cli_cpp_tests_runner.cpp" )

WORK="$(mktemp -d "${TMPDIR:-/tmp}/djinterp-cli.XXXXXX")"
trap 'rm -rf "${WORK}"' EXIT

# a compiler without the sanitizers' runtimes -- clang without its
# compiler-rt, MinGW -- can make no run of --sanitize at all: said so, rather
# than as a build failing for want of a library
if [[ "${sanitize}" -eq 1 ]] &&
   ! echo 'int main(void) { return 0; }' |
     "${CC}" -fsanitize=address,undefined -x c - -o "${WORK}/sanitizers" \
         >/dev/null 2>&1; then
    echo "--sanitize: ${CC} cannot link the sanitizers' runtimes here" >&2
    exit 2
fi

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
#   Prints the section count expected_sections.txt gives a build, or
# nothing.
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

# fail_build <log>
#   Leaves a failed build's first errors in `details`: the compiler's, and
# the linker's, not all of whose lines say "error".
fail_build()
{
    local pattern='error|cannot find|undefined reference|multiple definition'

    details="$(grep -m 6 -E "${pattern}" "$1" | sed 's/^/      /')"$'\n'
}

# build_library <out directory>
#   Compiles the libraries' sources as C at the C floor into <out>/cli.a,
# from nothing, so that a stale object can never mask a failed build; and
# the C++ suite's C sources beside it.
build_library()
{
    local out="$1" source="" floor=""

    details=""
    rm -rf "${out}" && mkdir -p "${out}/objects" "${out}/cxx"
    floor="$(std_flag "${CC}" "${C_FLOOR}")"

    for source in "${LIB_SOURCES[@]}"; do
        if ! "${CC}" ${floor} "${COMMON[@]}" "${PIC[@]+"${PIC[@]}"}" \
                -c "${source}" \
                -o "${out}/objects/$(basename "${source%.c}").o" \
                2>>"${out}/log"; then
            fail_build "${out}/log"
            return 1
        fi
    done

    "${AR}" rcs "${out}/cli.a" "${out}"/objects/*.o

    for source in "${TESTS}/c/cli_tests_support.c" \
                  "${PARSER_TESTS}/c/arg_tests_files.c"; do
        if ! "${CC}" ${floor} "${COMMON[@]}" -c "${source}" \
                -o "${out}/cxx/$(basename "${source%.c}").o" \
                2>>"${out}/log"; then
            fail_build "${out}/log"
            return 1
        fi
    done

    note_warnings "${out}/log"
}

# build_probes <out directory>
#   Builds each probe module into <out>/modules, KIT linking the archive as
# a program's module links the kit.
build_probes()
{
    local out="$1" kind=1 name="" floor=""
    local -a kit=()

    mkdir -p "${out}/modules"
    floor="$(std_flag "${CC}" "${C_FLOOR}")"

    for name in "${PROBES[@]}"; do
        kit=()

        if [[ "${name}" == "kit" ]]; then
            kit=( "${out}/cli.a" )
        fi

        if ! "${CC}" ${floor} "${COMMON[@]}" "${PIC[@]+"${PIC[@]}"}" \
                "${HIDE[@]+"${HIDE[@]}"}" -shared \
                -DD_TESTS_CLI_PROBE=${kind} "${PROBE}" \
                "${kit[@]+"${kit[@]}"}" "${KEEP_IN[@]+"${KEEP_IN[@]}"}" \
                -o "${out}/modules/prog-${name}${MODULE_SUFFIX}" \
                2>>"${out}/probes.log"; then
            fail_build "${out}/probes.log"
            return 1
        fi

        kind=$((kind + 1))
    done

    note_warnings "${out}/probes.log"
}

# run_binary <binary> <out directory>
#   Runs a built suite, given the probes' directory, and leaves `totals`
# and `details`. Returns 0 when no section failed.
run_binary()
{
    local binary="$1" out="$2"

    # RUN_PREFIX is a command and its arguments, split deliberately
    # shellcheck disable=SC2086
    if ! ${RUN_PREFIX} "${binary}" "${out}/modules" 2>&1 |
             tr -d '\r' >"${binary}.out"; then
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

# run_suites <level> <out directory>
#   Builds the C suites at one level against the archive and runs them.
# Returns 0 when they built, ran and failed no section; 2 when the compiler
# cannot select the level; 1 otherwise.
run_suites()
{
    local level="$1" out="$2" flag="" binary=""

    totals=""
    details=""
    flag="$(std_flag "${CC}" "${level}")"
    binary="${out}/suites-${level}${EXE_SUFFIX}"

    if [[ -z "${flag}" ]]; then
        totals="left out"
        return 2
    fi

    if ! "${CC}" ${flag} "${COMMON[@]}" "${C_SUITES[@]}" "${out}/cli.a" \
            "${LOADER[@]+"${LOADER[@]}"}" -o "${binary}" \
            2>"${binary}.log"; then
        totals="BUILD FAILED"
        fail_build "${binary}.log"
        return 1
    fi

    note_warnings "${binary}.log"
    run_binary "${binary}" "${out}"
}

# wraps_symbols
#   Whether the C compiler's linker wraps a symbol, as GNU ld's, lld's and
# MinGW's --wrap do, which the allocation sweep needs.
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

# run_sweep <out directory>
#   Builds the allocation sweep at the C floor against the archive, malloc,
# calloc, realloc and free wrapped, and runs it as run_suites runs a suite.
# Returns as run_suites does.
run_sweep()
{
    local out="$1" flag="" binary=""

    totals=""
    details=""
    flag="$(std_flag "${CC}" "${C_FLOOR}")"
    binary="${out}/sweep${EXE_SUFFIX}"

    if ! "${CC}" ${flag} "${COMMON[@]}" "${SWEEP[@]}" "${out}/cli.a" \
            "${LOADER[@]+"${LOADER[@]}"}" \
            -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc \
            -Wl,--wrap=free -o "${binary}" 2>"${binary}.log"; then
        totals="BUILD FAILED"
        fail_build "${binary}.log"
        return 1
    fi

    note_warnings "${binary}.log"
    run_binary "${binary}" "${out}"
}

# run_cxx_suites <level> <out directory> [-fno-exceptions]
#   Builds the C++ suite at one C++ level -- the C++98 floor in strict ISO
# -- and runs it, as run_suites does.
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
            "$@" "${CXX_SUITES[@]}" "${out}"/cxx/*.o "${out}/cli.a" \
            "${LOADER[@]+"${LOADER[@]}"}" \
            "${CXX_STATIC[@]+"${CXX_STATIC[@]}"}" \
            -o "${binary}" 2>"${binary}.log"; then
        totals="BUILD FAILED"
        fail_build "${binary}.log"
        return 1
    fi

    note_warnings "${binary}.log"
    run_binary "${binary}" "${out}"
}

# check <build> <runner> <arguments...>
#   One build: run, then held to the count expected_sections.txt gives it.
check()
{
    local build="$1" verdict=0 want=""
    shift

    "$@"
    verdict=$?
    want="$(expected "${build}")"

    if [[ ${verdict} -eq 2 ]]; then
        printf "  %-13s left out -- the compiler cannot select it\n" \
               "${build}"
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

echo "djinterp command-line kit"
echo "  $("${CC}" --version | head -n 1)"
if [[ "${sanitize}" -eq 1 ]]; then
    echo "  with AddressSanitizer and UndefinedBehaviorSanitizer"
fi
echo
echo "== default configuration: every C level =="

if ! build_library "${WORK}/default" ||
   ! build_probes "${WORK}/default"; then
    echo "  the libraries or the probes did not build at ${C_FLOOR}"
    printf "%s" "${details}"
    exit 1
fi

printf "%s" "${details}"

for level in ${C_LEVELS}; do
    check "${level}" run_suites "${level}" "${WORK}/default"
done

echo
echo "== default configuration: the allocation sweep =="

if wraps_symbols; then
    check "c99-sweep" run_sweep "${WORK}/default"
else
    echo "  left out -- the linker has no --wrap"
fi

echo
echo "== default configuration: every C++ level =="

if [[ "${cxx_found}" -eq 0 ]]; then
    echo "  left out -- ${CXX} not found"
else
    echo "  $("${CXX}" --version | head -n 1)"

    lowest=""
    highest=""

    for level in ${CXX_LEVELS}; do
        if [[ -n "$(std_flag "${CXX}" "${level}")" ]]; then
            lowest="${lowest:-${level}}"
            highest="${level}"
        fi

        check "${level}" run_cxx_suites "${level}" "${WORK}/default"
    done

    for level in ${lowest} ${highest}; do
        check "${level}-noexc" run_cxx_suites "${level}" "${WORK}/default" \
              -fno-exceptions
    done
fi

echo
echo "runs failing: ${failures}   warnings: ${warnings}"

[[ ${failures} -eq 0 ]]
