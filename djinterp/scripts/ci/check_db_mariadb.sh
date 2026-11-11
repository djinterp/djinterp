#!/usr/bin/env bash
#*******************************************************************************
# djinterp [ci]                                              check_db_mariadb.sh
#
# Builds djinterp's MariaDB/MySQL layer (c/db/db_*.c, c/db/mariadb/) and runs
# its harnesses through every client given, with every compiler, at every
# language level, against the server the environment names, and fails if any
# build or run fails:
#     mariadb_tests.c     the C suite, at each C standard, with
#                         -pedantic-errors -Wall -Wextra -Wshadow -Werror
#     t_mariadb_cpp.cpp   the C++ consumer, at each C++ standard, ISO strict
#                         C++98 included, linked against the layer built as C
#     t_mariadb_face.cpp  the C++ face, mariadb_face.hpp, likewise; and built
#                         once more for each misuse the face must refuse,
#                         from the level that refuses it, which must fail in
#                         the face's words (check_misuse)
#     t_mariadb_core.cpp core/db's mariadb_connection and mysql_connection,
#                         and their tables, mariadb_table and mysql_table
#                         (core/db/mariadb/, core/db/mysql/), database_table
#                         among them, and
#     t_database.cpp      core/db's vocabulary and utilities, each from
#                         C++17, core/db's floor, built with core/db's sources
#                         over the layer built as C
# A client is a word: `mariadb` for the installed MariaDB Connector/C, found
# by mariadb_config or pkg-config; `mysql` for the installed libmysqlclient,
# found by mysql_config or pkg-config; `custom` for one the environment
# describes (CLIENT_CFLAGS, CLIENT_LIBS, CLIENT_LIBRARY_PATH); or `stubs`,
# which builds the layer without a client (D_CFG_MARIADB=0), runs
# mariadb_stubs_tests.c in place of mariadb_tests.c, and needs no server.
# Each test asks the server its flavor and version for every check whose
# answer depends on them, so one suite covers MariaDB from 10.3 and MySQL
# from 5.7.
# The C suites are DTest's: tests/djinterp/c/db/mariadb/mariadb_tests.c and
# mariadb_stubs_tests.c, each built with its runner, in the CMake leaf
# (build/cmake/config/testing/djinterp/c/db/mariadb/), and DTest's C face, and
# run with --failures --no-table, so that a failure's log ends with what
# failed.
#   Over a client, the white-box suite of checked sizes,
# mariadb_sizes_tests.c, is built and run as well, at each C standard: it
# includes mariadb_query.c, which its build leaves out of the layer's sources.
#   Every compile defines _XOPEN_SOURCE=700, as the ladders do.
#
#   usage:   scripts/ci/check_db_mariadb.sh [mariadb | mysql | custom | stubs] ...
#            (no argument: mariadb)
#   env:     DJINTERP_TEST_MARIADB_HOST, _PORT, _USER, _PASSWORD, _SOCKET
#                          the server, as mariadb_tests.c reads them; a
#                          client run sets DJINTERP_TEST_MARIADB_REQUIRED=1,
#                          so that the C++ consumer and the face fail
#                          without one too
#            CLIENT_CFLAGS, CLIENT_LIBS
#                          the `custom` client's compile and link flags
#            CLIENT_LIBRARY_PATH
#                          where the `custom` client's shared library is, for
#                          the runs                       (default: none)
#            CC_LIST       C compilers               (default: "gcc clang")
#            C_STD_LIST    C standards            (default: "c99 c11 c17 c2x")
#            CXX_LIST      C++ compilers, each paired with the C compiler
#                          named like it (g++ -> gcc, clang++ -> clang)
#                                                     (default: "g++ clang++")
#            CXX_STD_LIST  C++ standards, c++98-strict meaning ISO strict
#                          C++98 and c++26 -std=c++2c (default: "c++98-strict
#                          c++98 c++11 c++14 c++17 c++20 c++23 c++26"); one a
#                          compiler does not have is skipped, and said to be
#            RUNNER        runs a harness, e.g. wine64         (default: none)
#            EXTRA_FLAGS   added to every compile and link, e.g.
#                          -fsanitize=address,undefined         (default: none)
#            EXE           the executable suffix               (default: none)
#   exit:    0 when everything passes or is skipped, 1 when something
#            fails, 2 when the run
#            cannot be made (a missing compiler or client)
#
# path:      /scripts/ci/check_db_mariadb.sh
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
exe="${EXE:-}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

if [[ $# -eq 0 ]]; then
    set -- mariadb
fi

layer=(src/djinterp/c/db/db_*.c src/djinterp/c/db/mariadb/mariadb_*.c)
core=(src/djinterp/cpp/db/database.cpp
      src/djinterp/cpp/db/database_statement.cpp
      src/djinterp/cpp/db/mysql/mysql_common.cpp
      src/djinterp/cpp/db/mariadb/mariadb.cpp
      src/djinterp/cpp/db/mysql/mysql.cpp)
# the layer but mariadb_query.c, which the sizes suite includes
rest=()
for source in "${layer[@]}"; do
    [[ "$source" == */mariadb_query.c ]] || rest+=("$source")
done
suites=tests/djinterp/c/db/mariadb
leaf=build/cmake/config/testing/djinterp/c/db/mariadb
dtest=src/djinterp/c/modules/test/test_suite.c
sizes=("$suites/mariadb_sizes_tests.c" "$leaf/mariadb_sizes_tests_runner.c")
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

# client_flags <client> <cflags|libs>
#   a client's compile or link flags, from its own config tool, pkg-config,
# or, for `custom`, the environment; nothing for `stubs`. The compile flags
# name the client's own mysql.h, <mysql.h> in the directory its tool gives,
# which env takes by default too: named, a header that cannot be found leaves
# the layer's stubs rather than reach the other client's, where both are
# installed, and the two disagree on struct MYSQL.
client_flags() {
    local tool package
    case "$1" in
        stubs)  return 0 ;;
        custom)
            if [[ "$2" == cflags ]]; then
                echo "${CLIENT_CFLAGS:-}"
            else
                echo "${CLIENT_LIBS:-}"
            fi
            return 0
            ;;
        mariadb) tool=mariadb_config; package=libmariadb ;;
        mysql)   tool=mysql_config;   package=mysqlclient ;;
        *)       return 1 ;;
    esac
    if command -v "$tool" > /dev/null 2>&1; then
        "$tool" "--$2" || return 1
    elif pkg-config --exists "$package" 2> /dev/null; then
        pkg-config "--$2" "$package" || return 1
    else
        return 1
    fi
    [[ "$2" == cflags ]] && echo "-DD_CFG_ENV_MARIADB_C_PATH=<mysql.h>"
    return 0
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
#   builds the face test with misuse <n> (D_TESTS_MARIADB_FACE_MISUSE), from
# the level it is refused at, and passes when the build fails and its log
# has the words the face refuses it in.
misuse_case() {
    local cxx="$1" std="$2" n="$3" from="$4" words="$5"
    local name="cxx=$1 std=$2 misuse $3"
    local log="$tmp/misuse-$3-$1-$2.log"

    shift 5
    [[ $(year_of "$std") -lt $from ]] && return 0

    if "$cxx" $(std_flags "$std") $extra "$@" -D_XOPEN_SOURCE=700 -Iinc \
            ${flags[@]+"${flags[@]}"} -DD_TESTS_MARIADB_FACE_MISUSE="$n" \
            -fsyntax-only tests/unintegrated/cpp/db/mariadb/t_mariadb_face.cpp \
            > "$log" 2>&1; then
        report FAIL "$name (it compiled)"
    elif grep -q "$words" "$log"; then
        report PASS "$name: refused"
    else
        report FAIL "$name (refused, but not by the face)" "$log"
    fi
}

# check_misuse <c++ compiler> <standard>
#   each misuse the face must refuse, once per compiler and standard, from
# the level that refuses it (t_mariadb_face.cpp lists them):
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

# run_harness <binary> <log> [<argument> ...]
#   runs a harness, or a suite, through the runner, if one is set, with the
# client's shared library findable.
run_harness() {
    local binary="$1" log="$2"
    local ld="${library_path}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    local dyld="${library_path}${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"

    shift 2
    LD_LIBRARY_PATH="$ld" DYLD_LIBRARY_PATH="$dyld" $runner "$binary" "$@" \
        >> "$log" 2>&1
}

# ${flags[@]+"${flags[@]}"} below expands an empty array to nothing under
# `set -u` on every bash, macOS's 3.2 included, where "${flags[@]}" fails
for client in "$@"; do
    if ! cflags="$(client_flags "$client" cflags | tr '\n' ' ')" ||
       ! libs="$(client_flags "$client" libs)"; then
        echo "client not found: $client" >&2
        exit 2
    fi

    # the harnesses wait on sockets with WSAPoll on Windows, which is
    # Winsock's
    if [[ "$exe" == .exe ]]; then
        libs="$libs -lws2_32"
    fi

    flags=()
    suite=("$suites/mariadb_tests.c" "$leaf/mariadb_tests_runner.c")
    library_path=""
    if [[ "$client" == stubs ]]; then
        flags=(-DD_CFG_MARIADB=0)
        suite=("$suites/mariadb_stubs_tests.c"
               "$leaf/mariadb_stubs_tests_runner.c")
    else
        # the client's own words for its headers and library
        read -r -a flags <<< "$cflags"
        [[ "$client" == custom ]] && library_path="${CLIENT_LIBRARY_PATH:-}"
        : "${DJINTERP_TEST_MARIADB_REQUIRED:=1}"
        export DJINTERP_TEST_MARIADB_REQUIRED
    fi

    for cc in $compilers; do
        if ! command -v "$cc" > /dev/null 2>&1; then
            echo "compiler not found: $cc" >&2
            exit 2
        fi

        # the C suite, at each standard
        for std in $c_standards; do
            name="client=$client cc=$cc std=$std"
            binary="$tmp/mariadb_tests-$client-$cc-$std$exe"
            log="$tmp/mariadb_tests-$client-$cc-$std.log"
            if "$cc" $(std_flags "$std") "${warnings[@]}" $extra \
                    -D_XOPEN_SOURCE=700 -Iinc -I"$suites" \
                    ${flags[@]+"${flags[@]}"} "${layer[@]}" "${suite[@]}" \
                    "$dtest" $libs -lm -o "$binary" > "$log" 2>&1 &&
               run_harness "$binary" "$log" --failures --no-table; then
                report PASS "$name: $(suite_counts "$log")"
            else
                report FAIL "$name" "$log"
            fi
        done

        # the checked sizes, over a client, at each standard
        [[ "$client" == stubs ]] && continue
        for std in $c_standards; do
            name="client=$client cc=$cc std=$std sizes"
            binary="$tmp/mariadb_sizes_tests-$client-$cc-$std$exe"
            log="$tmp/mariadb_sizes_tests-$client-$cc-$std.log"
            if "$cc" $(std_flags "$std") "${warnings[@]}" $extra \
                    -D_XOPEN_SOURCE=700 -Iinc -I"$suites" \
                    ${flags[@]+"${flags[@]}"} "${rest[@]}" "${sizes[@]}" \
                    "$dtest" $libs -lm -o "$binary" > "$log" 2>&1 &&
               run_harness "$binary" "$log" --failures --no-table; then
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

        objects="$tmp/layer-$client-$cc"
        mkdir -p "$objects"
        layer_ok=1
        for source in "${layer[@]}"; do
            "$cc" -std=c99 "${warnings[@]}" $extra -D_XOPEN_SOURCE=700 \
                  -Iinc ${flags[@]+"${flags[@]}"} -c "$source" \
                  -o "$objects/$(basename "$source" .c).o" \
                  >> "$objects/build.log" 2>&1 || layer_ok=0
        done
        if [[ $layer_ok -eq 0 ]]; then
            report FAIL "client=$client cxx=$cxx (the layer as C)" \
                   "$objects/build.log"
            continue
        fi

        for std in $cxx_standards; do
            # a standard the compiler does not have
            if ! supports_std "$cxx" "$std"; then
                report SKIP "client=$client cxx=$cxx std=$std (not in $cxx)"
                continue
            fi

            # the consumer, and the face
            for part in cpp face; do
                name="client=$client cxx=$cxx std=$std $part"
                binary="$tmp/t_mariadb_$part-$client-$cxx-$std$exe"
                log="$tmp/t_mariadb_$part-$client-$cxx-$std.log"
                if "$cxx" $(std_flags "$std") "${warnings[@]}" $extra \
                        -D_XOPEN_SOURCE=700 -Iinc \
                        ${flags[@]+"${flags[@]}"} \
                        "tests/unintegrated/cpp/db/mariadb/t_mariadb_$part.cpp" \
                        "$objects"/*.o $libs -lm -o "$binary" \
                        > "$log" 2>&1 &&
                   run_harness "$binary" "$log"; then
                    report PASS "$name: $(tail -n 1 "$log")"
                else
                    report FAIL "$name" "$log"
                fi
            done

            check_misuse "$cxx" "$std"

            # core/db, from its floor: the connections, and the vocabulary
            [[ $(year_of "$std") -lt 2017 ]] && continue

            for part in core database; do
                name="client=$client cxx=$cxx std=$std $part"
                binary="$tmp/t_mariadb_$part-$client-$cxx-$std$exe"
                log="$tmp/t_mariadb_$part-$client-$cxx-$std.log"
                if [[ "$part" == core ]]; then
                    sources=(tests/unintegrated/cpp/db/mariadb/t_mariadb_core.cpp
                             "${core[@]}")
                else
                    sources=(tests/unintegrated/cpp/db/t_database.cpp
                             src/djinterp/cpp/db/database.cpp)
                fi
                if "$cxx" $(std_flags "$std") "${warnings[@]}" $extra \
                        -D_XOPEN_SOURCE=700 -Iinc \
                        ${flags[@]+"${flags[@]}"} \
                        "${sources[@]}" "$objects"/*.o $libs -lm \
                        -o "$binary" > "$log" 2>&1 &&
                   run_harness "$binary" "$log"; then
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
