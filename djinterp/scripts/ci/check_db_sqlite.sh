#!/usr/bin/env bash
#*******************************************************************************
# djinterp [ci]                                               check_db_sqlite.sh
#
# Builds djinterp's SQLite layer (c/db/db_*.c, c/db/sqlite/) and runs its
# harnesses against every SQLite given, with every compiler, at every
# language level, and fails if any build or run fails:
#     sqlite_tests.c     the C suite, at each C standard, with
#                        -pedantic-errors -Wall -Wextra -Wshadow -Werror
#     t_sqlite_cpp.cpp   the C++ consumer, at each C++ standard, ISO strict
#                        C++98 included, linked against the layer built as C
#     t_sqlite_face.cpp  the C++ face, sqlite_face.hpp, likewise; and built
#                        once more for each misuse the face must refuse, from
#                        the level that refuses it, which must fail in the
#                        face's words (check_misuse)
#     t_sqlite_core.cpp  core/db's sqlite_connection and sqlite_table
#                        (core/db/sqlite/), database_table among them, and
#     t_database.cpp     core/db's vocabulary and utilities, each from
#                        C++17, core/db's floor, built with core/db's sources
#                        over the layer built as C
# The C suite and the harnesses load an extension, sqlite_extension.c,
# built here as a shared library once per compiler and SQLite, where its
# sqlite3ext.h is found (the amalgamation's directory, or the compiler's own
# paths), and named to them in DJINTERP_TEST_SQLITE_EXTENSION; where it
# cannot be built, they check what fails without it, and say so.
# Each SQLite is an amalgamation -- a directory holding sqlite3.c and
# sqlite3.h, which scripts/ci/fetch_sqlite.sh makes from a release tag -- built
# once per compiler, or the word `system`, for the installed library and header.
# The suite asks the library's version for every check whose answer depends
# on it, so one suite covers every release from the floor, 3.7.17. The word
# `stubs` builds the layer without SQLite (D_CFG_SQLITE=0), runs
# sqlite_stubs_tests.c in place of sqlite_tests.c, and links no SQLite at
# all. A C++ standard a compiler does not have -- C++26 before GCC 14 and
# Clang 17 -- is skipped, and said to be.
#   The C suites are DTest's: tests/djinterp/c/db/sqlite/sqlite_tests.c and
# sqlite_stubs_tests.c, each built with its runner, in the CMake leaf
# (build/cmake/config/testing/djinterp/c/db/sqlite/), and DTest's C face, and
# run with --failures --no-table, so that a failure's log ends with what
# failed.
#   Every compile defines _XOPEN_SOURCE=700, as the ladders do. Each run
# happens in a fresh directory, which the suite or harness fills and
# empties: the C suite's by default, the C++ harnesses' named to them as
# their argument, `.`.
#
#   usage:   scripts/ci/check_db_sqlite.sh [<amalgamation directory> | system |
#                                   stubs] ...  (no argument: system)
#   env:     CC_LIST       C compilers               (default: "gcc clang")
#            C_STD_LIST    C standards            (default: "c99 c11 c17 c2x")
#            CXX_LIST      C++ compilers, each paired with the C compiler
#                          named like it (g++ -> gcc, clang++ -> clang,
#                          x86_64-w64-mingw32-g++ -> x86_64-w64-mingw32-gcc)
#                                                     (default: "g++ clang++")
#            CXX_STD_LIST  C++ standards, c++98-strict meaning ISO strict
#                          C++98 and c++26 -std=c++2c (default: "c++98-strict
#                          c++98 c++11 c++14 c++17 c++20 c++23 c++26")
#            RUNNER        runs a harness, e.g. wine64         (default: none)
#            EXTRA_FLAGS   added to every compile and link, e.g.
#                          -fsanitize=address,undefined         (default: none)
#            LINK_FLAGS    what an amalgamation needs at link time
#                          (default: "-lpthread -ldl -lm", none for MinGW)
#            EXE           the executable suffix       (default: none, or .exe
#                          for MinGW)
#   exit:    0 when everything passes or is skipped, 1 when something
#            fails, 2 when the run cannot be made (a missing compiler or
#            amalgamation)
#
# path:      /scripts/ci/check_db_sqlite.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
#                                                            revised: 2026.11.10
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

if [[ $# -eq 0 ]]; then
    set -- system
fi

layer=(src/djinterp/c/db/db_*.c src/djinterp/c/db/sqlite/sqlite_*.c)
core=(src/djinterp/cpp/db/database.cpp
      src/djinterp/cpp/db/database_statement.cpp
      src/djinterp/cpp/db/sqlite/sqlite.cpp)
suites=tests/djinterp/c/db/sqlite
leaf=build/cmake/config/testing/djinterp/c/db/sqlite
dtest=src/djinterp/c/modules/test/test_suite.c
warnings=(-pedantic-errors -Wall -Wextra -Wshadow -Werror -Werror=undef)
passed=0
failed=0
skipped=0
misuse_done=" "

# c_for <c++ compiler>
#   the C compiler named like a C++ one.
c_for() {
    case "$1" in
        *clang++*) echo "${1/clang++/clang}" ;;
        *g++*)     echo "${1/g++/gcc}" ;;
        *)         echo "" ;;
    esac
}

# is_mingw <compiler>
#   true for a compiler that targets Windows through MinGW.
is_mingw() {
    [[ "$1" == *mingw* ]]
}

# std_flags <standard>
#   the flags one entry of a standard list stands for.
std_flags() {
    case "$1" in
        c++98-strict)
            echo "-std=c++98 -DD_CFG_ENV_ISO_STRICT=1 -DRE_STD_CFG_ISO_STRICT=1"
            ;;
        c++98|c++03)
            echo "-std=$1 -Wno-variadic-macros"
            ;;
        c++26)
            echo "-std=c++2c"
            ;;
        *)
            echo "-std=$1"
            ;;
    esac
}

# year_of <standard>
#   the year one entry of a standard list stands for, so that levels compare.
year_of() {
    case "$1" in
        c++98*|c++03)  echo 1998 ;;
        c++11)         echo 2011 ;;
        c++14)         echo 2014 ;;
        c++17)         echo 2017 ;;
        c++20|c++2a)   echo 2020 ;;
        c++23|c++2b)   echo 2023 ;;
        c++26|c++2c)   echo 2026 ;;
        *)             echo 0 ;;
    esac
}

# supports_std <c++ compiler> <standard>
#   true when the compiler takes the standard's flags.
supports_std() {
    printf 'int main() { return 0; }\n' |
        "$1" $(std_flags "$2") -x c++ -fsyntax-only - > /dev/null 2>&1
}

# format_check_on <c++ compiler> <standard>
#   true when the face reads formats at compile time with the compiler at the
# standard (db_face.hpp's D_INTERNAL_DB_FACE_FORMAT_CHECK): C++20, with
# consteval and concepts.
format_check_on() {
    printf '%s\n' '#include "djinterp/cpp/db/db_face.hpp"' \
           '#if !D_INTERNAL_DB_FACE_FORMAT_CHECK' '#error off' '#endif' |
        "$1" $(std_flags "$2") -D_XOPEN_SOURCE=700 -Iinc -x c++ \
             -fsyntax-only - > /dev/null 2>&1
}

# report <verdict> <what> [<log>]
#   counts and prints one configuration's result, and the tail of its log on
# a failure.
report() {
    if [[ "$1" == PASS ]]; then
        passed=$((passed + 1))
        echo "PASS $2"
    elif [[ "$1" == SKIP ]]; then
        skipped=$((skipped + 1))
        echo "SKIP $2"
    else
        failed=$((failed + 1))
        echo "FAIL $2"
        [[ -n "${3:-}" && -f "$3" ]] && tail -n 15 "$3" | sed 's/^/     /'
    fi
}

# misuse_case <c++ compiler> <standard> <n> <from year> <words> [<flag> ...]
#   builds the face test with misuse <n> (D_TESTS_SQLITE_FACE_MISUSE), from
# the level it is refused at, and passes when the build fails and its log
# has the words the face refuses it in.
misuse_case() {
    local cxx="$1" std="$2" n="$3" from="$4" words="$5"
    local name="cxx=$1 std=$2 misuse $3"
    local log="$tmp/misuse-$3-$1-$2.log"

    shift 5
    [[ $(year_of "$std") -lt $from ]] && return 0

    if "$cxx" $(std_flags "$std") $extra "$@" -D_XOPEN_SOURCE=700 -Iinc \
            -DD_TESTS_SQLITE_FACE_MISUSE="$n" -fsyntax-only \
            tests/unintegrated/cpp/db/sqlite/t_sqlite_face.cpp > "$log" 2>&1; then
        report FAIL "$name (it compiled)"
    elif grep -q "$words" "$log"; then
        report PASS "$name: refused"
    else
        report FAIL "$name (refused, but not by the face)" "$log"
    fi
}

# check_misuse <c++ compiler> <standard>
#   each misuse the face must refuse, once per compiler and standard, from
# the level that refuses it (t_sqlite_face.cpp lists them):
#     1  a std::string through an f form's `...`, from C++11
#     2  a literal format its argument does not match, from C++20 where the
#        compiler reads formats at compile time, and skipped where it cannot
#     3  a std::string as a format, from C++11
#     4  a guard made and dropped at once, from C++17, its warning an error
check_misuse() {
    [[ "$misuse_done" == *" $1/$2 "* ]] && return 0
    misuse_done="$misuse_done$1/$2 "

    misuse_case "$1" "$2" 1 2011 "an f form takes numbers"
    if [[ $(year_of "$2") -ge 2020 ]] && ! format_check_on "$1" "$2"; then
        report SKIP "cxx=$1 std=$2 misuse 2 (formats unchecked here)"
    else
        misuse_case "$1" "$2" 2 2020 \
                    "the_format_argument_does_not_match_its_conversion"
    fi
    misuse_case "$1" "$2" 3 2011 "execf"
    misuse_case "$1" "$2" 4 2017 "nodiscard" -Werror
}

# suite_counts <log>
#   a DTest report's counts, for a PASS line: "<n> checks (<m> failed)".
suite_counts() {
    sed -n 's/^  checks: *\([0-9]*\) (\(.*\))$/\1 checks (\2)/p' "$1" |
        tail -n 1
}

# run_in_fresh_dir <binary> <log> [<argument> ...]
#   runs a harness, or a suite, in an empty directory, through the runner if
# one is set, with the extension built for its compiler and SQLite, where
# there is one (DJINTERP_TEST_SQLITE_EXTENSION, as build_extension last left
# it).
run_in_fresh_dir() {
    local binary="$1" log="$2" dir
    dir="$(mktemp -d "$tmp/run.XXXXXX")"
    shift 2
    ( cd "$dir" &&
      DJINTERP_TEST_SQLITE_EXTENSION="$extension" $runner "$binary" "$@" ) \
        >> "$log" 2>&1
}

# build_extension <c compiler> <tag> [<flag> ...]
#   builds sqlite_extension.c as a shared library for the compiler and the
# SQLite the tag names, once, with the flags that find its sqlite3ext.h, and
# sets `extension` to its path; to "" where it cannot be built.
build_extension() {
    local cc="$1" tag="$2" suffix=".so"
    shift 2
    is_mingw "$cc" && suffix=".dll"
    [[ "$(uname -s)" == Darwin ]] && suffix=".dylib"
    local out="$tmp/ext-$tag-$cc/sqlite_extension$suffix"

    extension=""
    if [[ ! -f "$out" && ! -f "$out.log" ]]; then
        mkdir -p "$(dirname "$out")"
        if ! "$cc" -shared -fPIC -O1 -w "$@" \
                 "$suites/sqlite_extension.c" -o "$out" \
                 > "$out.log" 2>&1; then
            rm -f "$out"
            report SKIP "sqlite=$tag cc=$cc extension (it does not build)"
        fi
    fi
    [[ -f "$out" ]] && extension="$out"
    return 0
}

# ${flags[@]+"${flags[@]}"} below expands an empty array to nothing under
# `set -u` on every bash, macOS's 3.2 included, where "${flags[@]}" fails
extension=""
for sqlite in "$@"; do
    tag="system"
    flags=()
    suite=("$suites/sqlite_tests.c" "$leaf/sqlite_tests_runner.c")
    if [[ "$sqlite" == stubs ]]; then
        tag="stubs"
        flags=(-DD_CFG_SQLITE=0)
        suite=("$suites/sqlite_stubs_tests.c"
               "$leaf/sqlite_stubs_tests_runner.c")
    elif [[ "$sqlite" != system ]]; then
        if [[ ! -f "$sqlite/sqlite3.c" || ! -f "$sqlite/sqlite3.h" ]]; then
            echo "no amalgamation in $sqlite" >&2
            exit 2
        fi
        tag="$(sed -n 's/^#define SQLITE_VERSION *"\(.*\)".*/\1/p' \
                   "$sqlite/sqlite3.h")"
        flags=(-I"$sqlite")
    fi

    for cc in $compilers; do
        if ! command -v "$cc" > /dev/null 2>&1; then
            echo "compiler not found: $cc" >&2
            exit 2
        fi

        exe="${EXE:-}"
        links="${LINK_FLAGS--lpthread -ldl -lm}"
        if is_mingw "$cc"; then
            exe="${EXE:-.exe}"
            links="${LINK_FLAGS:-}"
        fi

        # the library: none for the stubs, the system's, or the
        # amalgamation built once here
        library=()
        if [[ "$sqlite" == system ]]; then
            library=(-lsqlite3)
        elif [[ "$sqlite" != stubs ]]; then
            object="$tmp/sqlite3-$tag-$cc.o"
            if ! "$cc" -O1 -w $extra -c "$sqlite/sqlite3.c" -o "$object" \
                    > "$tmp/sqlite3-$tag-$cc.log" 2>&1; then
                report FAIL "sqlite=$tag cc=$cc (sqlite3.c)" \
                       "$tmp/sqlite3-$tag-$cc.log"
                continue
            fi
            library=("$object" $links)
        fi

        # the extension, where SQLite there is
        extension=""
        [[ "$sqlite" != stubs ]] &&
            build_extension "$cc" "$tag" ${flags[@]+"${flags[@]}"}

        # the C suite, at each standard
        for std in $c_standards; do
            name="sqlite=$tag cc=$cc std=$std"
            binary="$tmp/sqlite_tests-$tag-$cc-$std$exe"
            log="$tmp/sqlite_tests-$tag-$cc-$std.log"
            if "$cc" $(std_flags "$std") "${warnings[@]}" $extra \
                    -D_XOPEN_SOURCE=700 -Iinc -I"$suites" \
                    ${flags[@]+"${flags[@]}"} "${layer[@]}" "${suite[@]}" \
                    "$dtest" ${library[@]+"${library[@]}"} -o "$binary" \
                    > "$log" 2>&1 &&
               run_in_fresh_dir "$binary" "$log" --failures --no-table; then
                report PASS "$name: $(suite_counts "$log")"
            else
                report FAIL "$name" "$log"
            fi
        done
    done

    # the C++ consumer and the face, at each standard, over the layer built
    # as C
    for cxx in $cxx_compilers; do
        cc="$(c_for "$cxx")"
        if [[ -z "$cc" ]] || ! command -v "$cxx" > /dev/null 2>&1 ||
           ! command -v "$cc" > /dev/null 2>&1; then
            echo "compiler not found: $cxx, or its C partner" >&2
            exit 2
        fi

        exe="${EXE:-}"
        links="${LINK_FLAGS--lpthread -ldl -lm}"
        if is_mingw "$cxx"; then
            exe="${EXE:-.exe}"
            links="${LINK_FLAGS:-}"
        fi

        objects="$tmp/layer-$tag-$cc"
        mkdir -p "$objects"
        layer_ok=1
        for source in "${layer[@]}"; do
            "$cc" -std=c99 "${warnings[@]}" $extra -D_XOPEN_SOURCE=700 \
                  -Iinc ${flags[@]+"${flags[@]}"} -c "$source" \
                  -o "$objects/$(basename "$source" .c).o" \
                  >> "$objects/build.log" 2>&1 || layer_ok=0
        done
        if [[ $layer_ok -eq 0 ]]; then
            report FAIL "sqlite=$tag cxx=$cxx (the layer as C)" \
                   "$objects/build.log"
            continue
        fi

        library=()
        if [[ "$sqlite" == system ]]; then
            library=(-lsqlite3)
        elif [[ "$sqlite" != stubs ]]; then
            object="$tmp/sqlite3-$tag-$cc.o"
            if [[ ! -f "$object" ]] &&
               ! "$cc" -O1 -w $extra -c "$sqlite/sqlite3.c" -o "$object" \
                    > "$tmp/sqlite3-$tag-$cc.log" 2>&1; then
                report FAIL "sqlite=$tag cc=$cc (sqlite3.c)" \
                       "$tmp/sqlite3-$tag-$cc.log"
                continue
            fi
            library=("$object" $links)
        fi

        extension=""
        [[ "$sqlite" != stubs ]] &&
            build_extension "$cc" "$tag" ${flags[@]+"${flags[@]}"}

        for std in $cxx_standards; do
            # a standard the compiler does not have
            if ! supports_std "$cxx" "$std"; then
                report SKIP "sqlite=$tag cxx=$cxx std=$std (not in $cxx)"
                continue
            fi

            # the consumer, and the face
            for part in cpp face; do
                name="sqlite=$tag cxx=$cxx std=$std $part"
                binary="$tmp/t_sqlite_$part-$tag-$cxx-$std$exe"
                log="$tmp/t_sqlite_$part-$tag-$cxx-$std.log"
                if "$cxx" $(std_flags "$std") "${warnings[@]}" $extra \
                        -D_XOPEN_SOURCE=700 -Iinc \
                        "tests/unintegrated/cpp/db/sqlite/t_sqlite_$part.cpp" \
                        "$objects"/*.o ${library[@]+"${library[@]}"} \
                        -o "$binary" \
                        > "$log" 2>&1 &&
                   run_in_fresh_dir "$binary" "$log" .; then
                    report PASS "$name: $(tail -n 1 "$log")"
                else
                    report FAIL "$name" "$log"
                fi
            done

            check_misuse "$cxx" "$std"

            # core/db, from its floor: the connection, and the vocabulary
            [[ $(year_of "$std") -lt 2017 ]] && continue

            for part in core database; do
                name="sqlite=$tag cxx=$cxx std=$std $part"
                binary="$tmp/t_sqlite_$part-$tag-$cxx-$std$exe"
                log="$tmp/t_sqlite_$part-$tag-$cxx-$std.log"
                if [[ "$part" == core ]]; then
                    sources=(tests/unintegrated/cpp/db/sqlite/t_sqlite_core.cpp
                             "${core[@]}")
                else
                    sources=(tests/unintegrated/cpp/db/t_database.cpp
                             src/djinterp/cpp/db/database.cpp)
                fi
                if "$cxx" $(std_flags "$std") "${warnings[@]}" $extra \
                        -D_XOPEN_SOURCE=700 -Iinc \
                        "${sources[@]}" "$objects"/*.o \
                        ${library[@]+"${library[@]}"} -o "$binary" \
                        > "$log" 2>&1 &&
                   run_in_fresh_dir "$binary" "$log" .; then
                    report PASS "$name: $(tail -n 1 "$log")"
                else
                    report FAIL "$name" "$log"
                fi
            done
        done
    done
done

echo "$passed passed, $failed failed, $skipped skipped"
[[ $failed -eq 0 ]]
