#!/usr/bin/env bash
#*******************************************************************************
# djinterp [ci]                                                 check_filesys.sh
#
# Runs c/filesys's runtime harnesses, which check the two widths
# (c/filesys/file_common.h) and the C++ layer that follows them, in the
# configurations a build can be in. The library -- every c/filesys source,
# with the four the C++ layer links against -- is built as a Linux build
# makes it: -std=c99 -pedantic-errors -D_XOPEN_SOURCE=700, every warning an
# error, and file_ops.c and file_stat.c with -D_GNU_SOURCE as well, as their
# banners ask. It is built three times: natively, natively without
# -D_GNU_SOURCE, so the portable copy and stat paths run too, and as a 32-bit
# build with -D_FILE_OFFSET_BITS=64 -D_TIME_BITS=64, which a 32-bit glibc
# build of the library must have.
#   Each harness then runs, against the library of its configuration's width,
# under tests/djinterp/c/filesys/t_file_shim.c (LD_PRELOAD), which gives
# directory entries named wide* inode numbers past 32 bits, gives a
# directory named fake5t a 5 TiB filesystem, and counts copy_file_range
# calls:
#     t_file_width.c    as C99 on the library's glibc paths and as C11 on
#                       its portable ones, natively; as C11 with the width
#                       pinned to 32; as C99 in a 32-bit process, at the
#                       width it takes from its own flags (32, and 64 with
#                       large files, with GCC and Clang); and as ISO strict
#                       C++98 in a 32-bit process, below the 64-bit floor,
#                       where only the 32-bit twins exist
#     t_file_width.cpp  as ISO strict C++98 and as C++17 natively, and as
#                       C++17 with the width pinned to 32; and in a 32-bit
#                       process as ISO strict C++98 (width 32), the same
#                       with large files (the width narrowed, which takes
#                       the width-bearing classes away), and C++11 with
#                       large files (width 64)
# A configuration passes when it builds and links, its harness passes, and
# the harness reports the width, and the narrowing, expected of it; and, for
# t_file_width.c, when its copy went through copy_file_range exactly where
# the library was built with -D_GNU_SOURCE. The fixtures t_file_width.cpp
# reads are made here; t_file_width.c makes its own.
#   A configuration that cannot run is a failure, not a pass: the 32-bit rows
# need the multilib packages, and a compiler that cannot link fails the run
# unless ALLOW_SKIP=1 says the gap is known. The number of configurations
# that reported is compared with the matrix's, so that a row lost to an
# edit fails too.
#
#   usage:   bash scripts/ci/check_filesys.sh
#   env:     ALLOW_SKIP      1 lets a configuration whose compiler cannot
#                            build for its target be skipped
#            EXPECT_CONFIGS  the number of configurations that must report
#                            (default: the matrix's); empty for no check
#   exit:    0 when every configuration passes, none was skipped without
#            leave and the count is right; 1 otherwise; 2 when the run
#            cannot be made
#
# path:      /scripts/ci/check_filesys.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
#                                                            revised: 2026.10.14
#*******************************************************************************
# the flag lists below are split into words on purpose, everywhere
# shellcheck disable=SC2086
set -u

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root" || exit 2

# the matrix's number of configurations: 7 of the C harness and 6 of the
# C++ one, below. Change it with the matrix.
default_configs=13
expect_configs="${EXPECT_CONFIGS-$default_configs}"
allow_skip="${ALLOW_SKIP-0}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

c_harness="tests/djinterp/c/filesys/t_file_width.c"
cpp_harness="tests/djinterp/cpp/filesys/t_file_width.cpp"
shim="tests/djinterp/c/filesys/t_file_shim.c"
# what the C++ layer links against, beyond c/filesys
links="src/djinterp/c/dstring.c src/djinterp/c/djinterp.c
       src/djinterp/c/memory/dmemory.c src/djinterp/c/string_fn.c"
warn="-Wall -Wextra -Werror"
posix="-D_XOPEN_SOURCE=700"
gnu="-D_GNU_SOURCE"
large="-D_FILE_OFFSET_BITS=64 -D_TIME_BITS=64"
strict="-pedantic-errors -DD_CFG_ENV_ISO_STRICT=1 -DRE_STD_CFG_ISO_STRICT=1"
passes=0
failures=0
skips=0

# the C harness's rows:
#   label | compiler | machine | flags | library | width | narrowed
# The machine is empty for a native build, and -m32 for a 32-bit one. The
# library is 64g (native, glibc paths), 64p (native, portable paths) or 32g
# (32-bit, glibc paths); a row built for the glibc paths says so with
# -D_GNU_SOURCE, since the creation time it expects follows the flag. The C
# harness built as C++ is C: `= {0}`, C's zero initializer, draws
# -Wmissing-field-initializers from g++, so that row turns it off.
c_rows=(
    "C99, glibc paths|gcc||-std=c99 -pedantic-errors $gnu|64g|64|0"
    "C11, portable paths|clang||-std=c11 -pedantic-errors|64p|64|0"
    "C11, width 32|gcc||-std=c11 -pedantic-errors $gnu -DD_CFG_FILE_WIDTH=32|64g|32|0"
    "C99, 32-bit|gcc|-m32|-std=c99 -pedantic-errors $gnu|32g|32|0"
    "C99, 32-bit, large files|gcc|-m32|-std=c99 -pedantic-errors $gnu $large|32g|64|0"
    "C11, 32-bit, large files|clang|-m32|-std=c11 -pedantic-errors $gnu $large|32g|64|0"
    "ISO strict C++98, 32-bit, below the floor|g++|-m32|-x c++ -std=c++98 $strict $gnu -Wno-missing-field-initializers|32g|32|0"
)

# the C++ harness's rows, in the same columns
cpp_rows=(
    "ISO strict C++98|g++||-std=c++98 $strict|64g|64|0"
    "C++17|clang++||-std=c++17 -pedantic-errors|64g|64|0"
    "C++17, width 32|g++||-std=c++17 -pedantic-errors -DD_CFG_FILE_WIDTH=32|64g|32|0"
    "ISO strict C++98, 32-bit|g++|-m32|-std=c++98 $strict|32g|32|0"
    "ISO strict C++98, 32-bit, large files|g++|-m32|-std=c++98 $strict -D_FILE_OFFSET_BITS=64|32g|32|1"
    "C++11, 32-bit, large files|g++|-m32|-std=c++11 -pedantic-errors $large|32g|64|0"
)


# pass <line>, fail <line> [detail file], skip <line>
#   report one configuration and count it; a skip fails the run unless
# ALLOW_SKIP=1.
pass() {
    echo "ok    $1"
    passes=$((passes + 1))
}

fail() {
    echo "FAIL  $1"
    [[ -n "${2-}" ]] && grep -E "error|FAIL|undefined" "$2" | head -8 |
        sed 's/^/        /'
    failures=$((failures + 1))
}

skip() {
    echo "skip  $1"
    skips=$((skips + 1))
}

# can_build <compiler> <machine>
#   succeeds when the compiler can build and link a program for the machine
# (the 32-bit one needs the multilib packages).
can_build() {
    printf 'int main(void) { return 0; }\n' |
        "$1" $2 -x c - -o "$tmp/probe" > /dev/null 2>&1
}

# library <tag> <glibc: 0|1> <machine flags...>
#   builds c/filesys, and what it links against, into $tmp/lib<tag>.a, as a
# Linux build makes it; with glibc=1, file_ops.c and file_stat.c also get
# -D_GNU_SOURCE, as their banners ask, and no other source does.
library() {
    local tag="$1" glibc="$2" src extra
    shift 2

    mkdir -p "$tmp/obj_$tag"
    for src in src/djinterp/c/filesys/*.c $links; do
        extra=""
        case "$src" in
            */file_ops.c|*/file_stat.c) [[ "$glibc" == 1 ]] && extra="$gnu" ;;
        esac
        gcc "$@" -std=c99 -pedantic-errors $warn $posix $extra -c "$src" \
            -o "$tmp/obj_$tag/$(basename "$src" .c).o" \
            > "$tmp/lib_$tag.log" 2>&1 || return 1
    done
    ar rcs "$tmp/lib$tag.a" "$tmp/obj_$tag"/*.o
}

# fixtures <directory>
#   makes what t_file_width.cpp reads: sparse 5 and 3 GiB files, a file
# modified in 2040, a link, a tree holding a 5 GiB file, and a directory of
# five empty files, two of which the shim widens.
fixtures() {
    local d="$1" n

    mkdir -p "$d/tree/sub" "$d/wdir" &&
        truncate -s 5368709121 "$d/big5g" &&
        truncate -s 3221225472 "$d/big3g" &&
        : > "$d/future" &&
        touch -d '2040-06-01 00:00:00' "$d/future" &&
        truncate -s 5368709121 "$d/tree/big" &&
        ln -s big5g "$d/link" || return 1

    for n in a wide1 b wide2 c; do
        : > "$d/wdir/$n" || return 1
    done
}

# run <harness> <row> <n>
#   builds the harness the row names, links it against the row's library,
# runs it under the shim of the row's machine in a fresh scratch directory,
# and reports the row.
run() {
    local harness="$1" row="$2" n="$3"
    local label cc machine flags lib width narrowed line
    local out="$tmp/run_$n" scratch="$tmp/scratch_$n"

    IFS='|' read -r label cc machine flags lib width narrowed <<< "$row"
    label="$(basename "$harness"): $label"

    # a machine the compiler cannot build for is a gap, not a pass
    if ! can_build "$cc" "$machine"; then
        skip "$label (no $cc $machine to build with)"
        return
    fi

    if [[ ! -f "$tmp/lib$lib.a" ]]; then
        fail "$label: the library ($lib) did not build" "$tmp/lib_$lib.log"
        return
    fi

    # built with every warning an error, then linked as its language links
    if ! "$cc" $machine $flags $posix $warn -DT_FILE_SHIM \
             -c "$harness" -o "$out.o" > "$out.build" 2>&1 ||
       ! "$cc" $machine "$out.o" "$tmp/lib$lib.a" -o "$out" \
             >> "$out.build" 2>&1; then
        fail "$label: does not build" "$out.build"
        return
    fi

    rm -rf "$scratch" && mkdir -p "$scratch"
    if [[ "$harness" == *.cpp ]] && ! fixtures "$scratch"; then
        fail "$label: the fixtures could not be made"
        return
    fi

    # run, under the shim built for the same machine; the shim reports on
    # stderr, at exit, so it is kept apart from the harness's own report
    LD_PRELOAD="$tmp/shim$machine.so" "$out" "$scratch" \
        > "$out.log" 2> "$out.err"
    local status=$?
    line="$(head -1 "$out.log")"

    if [[ $status -ne 0 ]]; then
        cat "$out.err" >> "$out.log"
        fail "$label: the harness failed ($line)" "$out.log"
    elif [[ "$line" != *"D_FILE_WIDTH $width,"* ||
            "$line" != *"narrowed $narrowed"* ]]; then
        fail "$label: expected width $width, narrowed $narrowed; got: $line"
    elif [[ "$harness" == *.c ]] &&
         ! copies_as_expected "$out.err" "$lib"; then
        fail "$label: copy_file_range $( [[ "$lib" == *g ]] &&
              echo "never made the copy" ||
              echo "made the copy without -D_GNU_SOURCE")"
    else
        pass "$label ($(tail -1 "$out.log" | sed 's/^[^:]*: //'))"
    fi
}

# copies_as_expected <shim's report> <library tag>
#   succeeds when the shim counted copy_file_range calls exactly where the
# library was built with -D_GNU_SOURCE (a tag ending in g); a process that
# made none prints no count.
copies_as_expected() {
    local calls

    calls="$(sed -n 's/^t_file_shim: copy_file_range calls //p' "$1" |
             tail -1)"
    if [[ "$2" == *g ]]; then
        [[ -n "$calls" && "$calls" -gt 0 ]]
    else
        [[ -z "$calls" ]]
    fi
}


for f in "$c_harness" "$cpp_harness" "$shim"; do
    [[ -f "$f" ]] || { echo "check_filesys: $f is missing" >&2; exit 2; }
done

# the shims and the libraries: natively, and as 32-bit builds where the
# compiler can make them (a row that needs one and cannot have it is skipped,
# or failed, by run)
if ! gcc -shared -fPIC $gnu $warn "$shim" -o "$tmp/shim.so" -ldl; then
    echo "check_filesys: the shim does not build" >&2
    exit 2
fi
library 64g 1
library 64p 0
if can_build gcc -m32; then
    gcc -m32 -shared -fPIC $gnu $warn "$shim" -o "$tmp/shim-m32.so" -ldl &&
        library 32g 1 -m32 $large
fi

n=0
for row in "${c_rows[@]}"; do
    n=$((n + 1))
    run "$c_harness" "$row" "$n"
done
for row in "${cpp_rows[@]}"; do
    n=$((n + 1))
    run "$cpp_harness" "$row" "$n"
done

reported=$((passes + failures + skips))
echo "check_filesys: $passes passed, $failures failed, $skips skipped"

if [[ -n "$expect_configs" && "$reported" -ne "$expect_configs" ]]; then
    echo "check_filesys: $reported configurations reported," \
         "$expect_configs expected"
    exit 1
fi

if [[ $failures -gt 0 ]] || [[ $skips -gt 0 && "$allow_skip" != 1 ]]; then
    exit 1
fi

exit 0
