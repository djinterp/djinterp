#!/usr/bin/env bash
#*******************************************************************************
# djinterp [ci]                                                 check_ostream.sh
#
# Builds the output stream (c/util/ostream/*.c, sink_common.c), its doubles,
# printf and templates (c/text/ostream_format.c, ostream_template.c), the
# machine environment and the djin registry (c/option/option_env*.c over the
# option core) and the environment printer (env/env_printer.c), with DTest's
# C face (c/modules/test/test_suite.c), runs their suites and harnesses with
# every compiler, at every language level, and fails if any build, run, JSON
# or CSV check fails. Each DTest suite is its runner (under
# build/cmake/config/testing/djinterp) and its sections (under
# tests/djinterp):
#     ostream              the stream's C suite (c/util/ostream), at each C
#                          standard, plain and in testing mode
#                          (-DD_TESTING=1)
#     ostream_template     the template engine (c/text), likewise
#     pdf_stream           the PDF writer's numbers and save_to on the stream
#                          (c/util/pdf), likewise
#     t_option_env.c,      the machine environment and the registry
#     t_option_env_registry.c  (tests/unintegrated/c/option), likewise
#     output_stream        the C++ face and the std adapters
#                          (cpp/util/output), at each C++ standard, ISO
#                          strict C++98 included, linked against the units
#                          built as C
#     pdf_stream (C++)     the C++ PDF writer against the C one, and
#                          DPdfDocument::save_to (cpp/util/pdf), at each C++
#                          standard (C++17 and up run it; below, skipped)
#     writer_stream        writer.hpp's sinks and printer.hpp's file printer
#                          on the stream (cpp/text), at each C++ standard
#                          (C++17 and up run it; below, skipped)
#     t_env_printer.c      the environment printer, from C
#                          (tests/unintegrated/c/env), at each C
#                          standard; its JSON and CSV are read back by
#                          python3
#     t_env_printer.cpp    the printer's C++ face (tests/unintegrated/cpp/env),
#                          at each C++ standard, with
#                          the C++ feature table compiled in; its JSON and
#                          CSV too
# Every compile is -pedantic-errors -Wall -Wextra -Wconversion -Wshadow
# -Werror -Werror=undef, and defines _XOPEN_SOURCE=700, as the ladders do. A
# C++ standard a compiler does not have -- C++26 before GCC 14 and Clang 17
# -- is skipped, and said to be. Each run happens in a fresh directory.
#
#   usage:   scripts/ci/check_ostream.sh   (lane E's harnesses run when they
#            are in the tree; ONLY=stream runs the stream's three alone)
#   env:     CC_LIST       C compilers               (default: "gcc clang")
#            C_STD_LIST    C standards            (default: "c99 c11 c17 c2x")
#            CXX_LIST      C++ compilers, each paired with the C compiler
#                          named like it (g++ -> gcc, clang++ -> clang)
#                                                     (default: "g++ clang++")
#            CXX_STD_LIST  C++ standards, c++98-strict meaning ISO strict
#                          C++98 and c++26 -std=c++2c (default: "c++98-strict
#                          c++98 c++11 c++14 c++17 c++20 c++23 c++26")
#            RUNNER        runs a harness, e.g. wine64         (default: none)
#            EXTRA_FLAGS   added to every compile and link, e.g.
#                          -fsanitize=address,undefined or -m32
#                                                         (default: none)
#            EXE           the executable suffix       (default: none, or .exe
#                          for MinGW)
#   exit:    0 when everything passes or is skipped, 1 when something
#            fails, 2 when the run cannot be made (a missing compiler)
#
# path:      /scripts/ci/check_ostream.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
#                                                            revised: 2026.11.09
#*******************************************************************************
set -u

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root" || exit 2

compilers="${CC_LIST:-gcc clang}"
c_standards="${C_STD_LIST:-c99 c11 c17 c2x}"
cxx_compilers="${CXX_LIST:-g++ clang++}"
all_cxx="c++98-strict c++98 c++11 c++14 c++17 c++20 c++23 c++26"
cxx_standards="${CXX_STD_LIST:-$all_cxx}"
runner="${RUNNER:-}"
extra="${EXTRA_FLAGS:-}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

warnings="-pedantic-errors -Wall -Wextra -Wconversion -Wshadow -Werror"
warnings="$warnings -Werror=undef"
sources="src/djinterp/c/util/sink_common.c
         src/djinterp/c/util/ostream/ostream.c
         src/djinterp/c/util/ostream/ostream_string.c
         src/djinterp/c/util/ostream/ostream_file.c
         src/djinterp/c/util/ostream/ostream_console.c
         src/djinterp/c/text/ostream_format.c
         src/djinterp/c/text/ostream_template.c
         src/djinterp/c/util/pdf/pdf_builtin_backend.c
         src/djinterp/c/util/pdf/pdf_document.c
         src/djinterp/c/util/pdf/pdf_backend.c
         src/djinterp/c/util/pdf/pdf_primitives.c
         src/djinterp/c/dstring.c src/djinterp/c/string_fn.c
         src/djinterp/c/djinterp.c src/djinterp/c/memory/dmemory.c
         src/djinterp/c/filesys/file_open.c
         src/djinterp/c/filesys/file_common.c
         src/djinterp/c/modules/test/test_suite.c"
# lane E's: the machine environment, the registry and the printer
env_sources="src/djinterp/c/option/option_env.c
             src/djinterp/c/option/option_env_registry.c
             src/djinterp/c/option/option_set_common.c
             src/djinterp/c/option/option_common.c
             src/djinterp/env/env_printer.c"
with_env=0
if [[ "${ONLY:-}" != stream && -f src/djinterp/env/env_printer.c &&
      -f src/djinterp/c/option/option_env_registry.c ]]; then
    with_env=1
    sources="$sources $env_sources"
fi

passed=0
failed=0
skipped=0

# is_mingw <compiler>
#   whether a compiler targets Windows through MinGW-w64.
is_mingw() {
    [[ "$1" == *mingw* ]]
}

# cxx_flags <standard>
#   the flags that select a C++ standard.
cxx_flags() {
    case "$1" in
        c++98-strict)
            echo "-std=c++98 -DD_CFG_ENV_ISO_STRICT=1 -DRE_STD_CFG_ISO_STRICT=1"
            ;;
        c++98)
            # the rung where the framework may use variadic macros, which
            # GCC and Clang take in C++98 as an extension (decision 4.5,
            # as scripts/ci/ladder.py builds it)
            echo "-std=c++98 -Wno-variadic-macros"
            ;;
        c++26)
            echo "-std=c++2c"
            ;;
        *)
            echo "-std=$1"
            ;;
    esac
}

# c_compiler_for <c++ compiler>
#   the C compiler named like a C++ one.
c_compiler_for() {
    case "$1" in
        *clang++*) echo "${1/clang++/clang}" ;;
        *g++*)     echo "${1/g++/gcc}" ;;
        *)         echo "cc" ;;
    esac
}

# exe_suffix <compiler>
#   the executable suffix a compiler's programs need.
exe_suffix() {
    if [[ -n "${EXE:-}" ]]; then
        echo "$EXE"
    elif is_mingw "$1"; then
        echo ".exe"
    fi
}

# report <ok> <what>
#   counts and prints one result.
report() {
    if [[ "$1" == 0 ]]; then
        passed=$((passed + 1))
        echo "  pass  $2"
    else
        failed=$((failed + 1))
        echo "  FAIL  $2"
    fi
}

# build_units <c compiler> <dir> [flags...]
#   builds the units as C into <dir>, once per compiler and flag set.
build_units() {
    local cc="$1" dir="$2"
    shift 2
    mkdir -p "$dir"
    for source in $sources; do
        $cc -std=c99 $warnings -D_XOPEN_SOURCE=700 -Iinc $extra "$@" \
            -c "$source" -o "$dir/$(basename "$source" .c).o" || return 1
    done
}

# run_in <dir> <program> [arguments...]
#   runs a harness in a fresh directory, through the runner if there is one.
run_in() {
    local dir="$1"
    shift
    (cd "$dir" && $runner "$@")
}

# check_json <file>
#   whether a file holds one JSON value, by python3's parser.
check_json() {
    python3 -I -c 'import json, sys; json.load(open(sys.argv[1]))' "$1"
}

# check_csv <file>
#   whether a file is CSV with a header and rows of the header's width, by
#   python3's reader.
check_csv() {
    python3 -I -c '
import csv, sys
rows = list(csv.reader(open(sys.argv[1], newline="")))
sys.exit(0 if len(rows) > 1 and all(len(r) == len(rows[0]) for r in rows)
         else 1)' "$1"
}

# suite_files <slug> <stem> <extension>
#   a DTest suite's files: its runner, then its sections.
suite_files() {
    echo "build/cmake/config/testing/djinterp/$1/$2/$2_tests_runner.$3"
    ls tests/djinterp/"$1"/"$2"_tests*."$3"
}

# summary <file>
#   how a run ended: DTest's counts of tests and checks, or a harness's
#   last line.
summary() {
    if grep -q '^  checks:' "$1" 2>/dev/null; then
        sed -n -E 's/^  (tests|checks): +/\1 /p' "$1" | paste -sd ';' - |
            sed 's/;/; /g'
    else
        tail -1 "$1"
    fi
}

# run_c <c compiler> <standard> <flags> <dir> <suffix> <name> <include>
#       <files...>
#   builds a C suite or harness against the units and runs it in its
#   directory.
run_c() {
    local cc="$1" std="$2" flags="$3" dir="$4" suffix="$5" name="$6"
    local include="$7"
    shift 7
    $cc -std="$std" $warnings -D_XOPEN_SOURCE=700 -Iinc -I"$include" \
        $extra $flags "$@" "$units"/*.o -lm -o "$dir/$name$suffix" &&
    run_in "$dir" "./$name$suffix" > "$dir/$name.txt"
}

# run_cxx <c++ compiler> <standard> <dir> <suffix> <name> <slug> <stem>
#   builds a C++ DTest suite against the units and runs it in its directory.
run_cxx() {
    local cxx="$1" std="$2" dir="$3" suffix="$4" name="$5" slug="$6"
    local stem="$7"
    $cxx $(cxx_flags "$std") $warnings -D_XOPEN_SOURCE=700 -Iinc \
        -I"tests/djinterp/$slug" $extra $(suite_files "$slug" "$stem" cpp) \
        "$units"/*.o -lm -o "$dir/$name$suffix" &&
    run_in "$dir" "./$name$suffix" > "$dir/$name.txt"
}

for cc in $compilers; do
    if ! command -v "$cc" > /dev/null; then
        echo "no $cc"; exit 2
    fi
    suffix="$(exe_suffix "$cc")"

    for mode in plain testing; do
        flags=""
        [[ "$mode" == testing ]] && flags="-DD_TESTING=1"
        units="$tmp/$cc-$mode"
        if ! build_units "$cc" "$units" $flags; then
            report 1 "$cc $mode: the units"
            continue
        fi

        for std in $c_standards; do
            what="$cc -std=$std $mode"
            dir="$tmp/run-$cc-$std-$mode"
            mkdir -p "$dir"
            for suite in c/util/ostream:ostream c/text:ostream_template \
                         c/util/pdf:pdf_stream; do
                slug="${suite%%:*}"
                stem="${suite##*:}"
                if run_c "$cc" "$std" "$flags" "$dir" "$suffix" "$stem" \
                         "tests/djinterp/$slug" \
                         $(suite_files "$slug" "$stem" c); then
                    report 0 "$what: $stem ($(summary "$dir/$stem.txt"))"
                else
                    report 1 "$what: $stem"
                    tail -20 "$dir/$stem.txt" 2>/dev/null
                fi
            done
            harnesses=""
            [[ $with_env == 1 ]] && harnesses="
                       tests/unintegrated/c/option/t_option_env.c
                       tests/unintegrated/c/option/t_option_env_registry.c"
            for harness in $harnesses; do
                name="$(basename "$harness" .c)"
                if run_c "$cc" "$std" "$flags" "$dir" "$suffix" "$name" \
                         "$(dirname "$harness")" "$harness"; then
                    report 0 "$what: $name ($(summary "$dir/$name.txt"))"
                else
                    report 1 "$what: $name"
                    tail -20 "$dir/$name.txt" 2>/dev/null
                fi
            done

            [[ $with_env == 1 ]] || continue
            if $cc -std="$std" $warnings -D_XOPEN_SOURCE=700 -Iinc $extra \
                   $flags tests/unintegrated/c/env/t_env_printer.c \
                   "$units"/*.o -lm -o "$dir/t_env_printer$suffix" &&
               run_in "$dir" "./t_env_printer$suffix" env.json env.csv \
                   > "$dir/env.txt" &&
               check_json "$dir/env.json" &&
               check_csv "$dir/env.csv"; then
                report 0 "$what: t_env_printer ($(summary "$dir/env.txt"))"
            else
                report 1 "$what: t_env_printer"; tail -20 "$dir/env.txt"
            fi
        done
    done
done

for cxx in $cxx_compilers; do
    if ! command -v "$cxx" > /dev/null; then
        echo "no $cxx"; exit 2
    fi
    cc="$(c_compiler_for "$cxx")"
    suffix="$(exe_suffix "$cxx")"
    units="$tmp/$cc-plain"
    [[ -d "$units" ]] || build_units "$cc" "$units" || {
        report 1 "$cxx: the units"; continue; }

    for std in $cxx_standards; do
        what="$cxx $std"
        dir="$tmp/run-$cxx-$std"
        mkdir -p "$dir"
        if ! $cxx $(cxx_flags "$std") -fsyntax-only -x c++ /dev/null \
                2> /dev/null; then
            skipped=$((skipped + 1)); echo "  skip  $what: no such standard"
            continue
        fi

        for suite in cpp/util/output:output_stream cpp/text:writer_stream \
                     cpp/util/pdf:pdf_stream; do
            slug="${suite%%:*}"
            stem="${suite##*:}"
            name="${stem}_cpp"
            if run_cxx "$cxx" "$std" "$dir" "$suffix" "$name" "$slug" \
                       "$stem"; then
                report 0 "$what: $stem ($(summary "$dir/$name.txt"))"
            else
                report 1 "$what: $stem"; tail -20 "$dir/$name.txt"
            fi
        done

        [[ $with_env == 1 ]] || continue
        if $cxx $(cxx_flags "$std") $warnings -D_XOPEN_SOURCE=700 -Iinc \
               -DD_CFG_ENV_PRINTER_CPP_FEATURES=1 $extra \
               tests/unintegrated/cpp/env/t_env_printer.cpp \
               "$units"/*.o -lm -o "$dir/t_env_printer$suffix" &&
           run_in "$dir" "./t_env_printer$suffix" env.json env.csv \
               > "$dir/env.txt" &&
           check_json "$dir/env.json" &&
           check_csv "$dir/env.csv"; then
            report 0 "$what: t_env_printer ($(summary "$dir/env.txt"))"
        else
            report 1 "$what: t_env_printer"; tail -20 "$dir/env.txt"
        fi
    done
done

echo "$passed passed, $failed failed, $skipped skipped"
[[ $failed -eq 0 ]]
