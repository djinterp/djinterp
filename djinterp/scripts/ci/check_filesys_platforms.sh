#!/usr/bin/env bash
#*******************************************************************************
# djinterp [ci]                                       check_filesys_platforms.sh
#
# Builds c/filesys for the platforms a Linux runner cannot run it on, and runs
# it where a runtime can:
#   compiled with zig cc against each platform's own C library headers, which
#     zig ships: macOS on x86_64 and arm64, at the C guide's POSIX level, with
#     no feature-test macros, and with _DARWIN_C_SOURCE too; iOS and Android
#     as env reports them, on macOS's headers and musl's; FreeBSD, NetBSD and
#     OpenBSD, at the POSIX level and without it; Linux on musl, 64- and
#     32-bit; and Windows on x86_64, x86 and arm64, on mingw-w64's headers;
#     and Windows on x86_64 and x86, and musl, with UTF-8 paths
#     (D_CFG_FILE_WIDE_PATHS), which only Windows converts.
#   compiled and linked for WASI, and run there: every source compiled, then
#     every object linked with nothing dropped, so that a call WASI's libc
#     lacks fails the link; and t_file_noplat.c and t_file_wasi.c run under
#     wasmtime (scripts/ci/wasi_run.py), with the clock emulation and without it.
#   compiled natively with neither POSIX nor Windows (D_CFG_FILE_HAS_POSIX=0),
#     the ISO C backend a target with no platform API gets, and
#     t_file_noplat.c run against it, 64- and 32-bit.
# Every source must build as C99 under -pedantic-errors and -Werror=undef with
# every warning an error, and the gates the configuration resolves to
# (tests/djinterp/c/filesys/t_file_gates.c, preprocessed by the row's own
# compiler) must be the ones recorded in the row.
#   A configuration that cannot run is a failure, not a pass: a missing zig,
# wasmtime or 32-bit multilib fails the run unless ALLOW_SKIP=1 says the gap
# is known. The number of configurations that reported is compared with the
# matrix's, so that a row lost to an edit fails too. PRINT_GATES=1 prints
# each row's gates as they resolve now, for recording a row's change.
#
#   usage:   bash scripts/ci/check_filesys_platforms.sh
#   env:     ZIG             the zig command (default: python3 -m ziglang,
#                            pip's ziglang)
#            WASI_RUNNER     runs <program.wasm> <directory> (default:
#                            python3 scripts/ci/wasi_run.py, which needs pip's
#                            wasmtime)
#            ALLOW_SKIP      1 lets a configuration whose tools are missing
#                            be skipped
#            EXPECT_CONFIGS  the number of configurations that must report
#                            (default: the matrix's); empty for no check
#            PRINT_GATES     1 prints each row's resolved gates
#   exit:    0 when every configuration passes, none was skipped without
#            leave and the count is right; 1 otherwise; 2 when the run
#            cannot be made
#
# path:      /scripts/ci/check_filesys_platforms.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
#                                                            revised: 2026.10.19
#*******************************************************************************
# the flag lists below are split into words on purpose, everywhere
# shellcheck disable=SC2086
set -u

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root" || exit 2

# the matrix's number of configurations: 21 zig rows, 2 WASI rows and 2
# native ones, below. Change it with the matrix.
default_configs=25
expect_configs="${EXPECT_CONFIGS-$default_configs}"
allow_skip="${ALLOW_SKIP-0}"
print_gates="${PRINT_GATES-0}"
zig="${ZIG-python3 -m ziglang}"
wasi_runner="${WASI_RUNNER-python3 $root/scripts/ci/wasi_run.py}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

probe="tests/djinterp/c/filesys/t_file_gates.c"
noplat="tests/djinterp/c/filesys/t_file_noplat.c"
wasi="tests/djinterp/c/filesys/t_file_wasi.c"
strict="-std=c99 -pedantic-errors -Werror=undef -Wall -Wextra -Werror"
posix="-D_XOPEN_SOURCE=700"
large="-D_FILE_OFFSET_BITS=64 -D_TIME_BITS=64"
ios="-DD_CFG_ENV_OS_ENABLED=0 -DD_ENV_DETECTED_OS_IOS"
android="-DD_CFG_ENV_OS_ENABLED=0 -DD_ENV_DETECTED_OS_ANDROID"
noplat_flag="-DD_CFG_FILE_HAS_POSIX=0"
wide="-DD_CFG_FILE_WIDE_PATHS=1"
narrow="-DD_CFG_FILE_WIDE_PATHS=0"
passes=0
failures=0
skips=0

# The gates, in t_file_gates.c's order, which has their meanings:
#   backend posix win32 realpath symlinks pipes pread tmpfile clock flock
#   lock nsec birth dtype syncfull space width wide

# the zig rows:
#   label | target | flags | gates
zig_rows=(
    "macOS x86_64, POSIX level|x86_64-macos|$posix|2 1 0 1 1 1 1 1 1 0 2 4 1 1 1 1 64 0"
    "macOS x86_64, no feature-test macros|x86_64-macos||2 1 0 1 1 1 1 1 1 1 1 2 1 1 1 1 64 0"
    "macOS x86_64, POSIX level and _DARWIN_C_SOURCE|x86_64-macos|$posix -D_DARWIN_C_SOURCE|2 1 0 1 1 1 1 1 1 1 1 2 1 1 1 1 64 0"
    "macOS arm64, POSIX level|aarch64-macos|$posix|2 1 0 1 1 1 1 1 1 0 2 4 1 1 1 1 64 0"
    "iOS as env reports it, POSIX level|aarch64-macos|$posix $ios|2 1 0 1 1 1 1 1 1 0 2 4 1 1 1 1 64 0"
    "iOS as env reports it, no feature-test macros|aarch64-macos|$ios|2 1 0 1 1 1 1 1 1 1 1 2 1 1 1 1 64 0"
    "FreeBSD, POSIX level|x86_64-freebsd|$posix|2 1 0 1 1 1 1 1 1 0 2 1 1 1 0 1 64 0"
    "FreeBSD, no feature-test macros|x86_64-freebsd||2 1 0 1 1 1 1 1 1 1 1 1 1 1 0 1 64 0"
    "NetBSD, POSIX level|x86_64-netbsd|$posix|2 1 0 1 1 1 1 1 1 0 2 1 1 1 0 1 64 0"
    "NetBSD, no feature-test macros|x86_64-netbsd||2 1 0 1 1 1 1 1 1 1 1 1 1 1 0 1 64 0"
    "OpenBSD, POSIX level|x86_64-openbsd|$posix|2 1 0 1 1 1 1 1 1 0 2 1 0 1 0 1 64 0"
    "OpenBSD, no feature-test macros|x86_64-openbsd||2 1 0 1 1 1 1 1 1 1 1 1 0 1 0 1 64 0"
    "Android as env reports it, on musl|aarch64-linux-musl|$posix $android|2 1 0 1 1 1 1 1 1 1 1 1 0 1 0 1 64 0"
    "Linux on musl|x86_64-linux-musl|$posix|2 1 0 1 1 1 1 1 1 1 1 1 0 1 0 1 64 0"
    "Linux on musl, 32-bit|x86-linux-musl|$posix|2 1 0 1 1 1 1 1 1 1 1 1 0 1 0 1 32 0"
    "Linux on musl, UTF-8 paths|x86_64-linux-musl|$posix $wide|2 1 0 1 1 1 1 1 1 1 1 1 0 1 0 1 64 0"
    "Windows x86_64|x86_64-windows-gnu||3 0 1 0 1 1 0 1 1 0 3 3 1 0 0 1 64 1"
    "Windows x86|x86-windows-gnu||3 0 1 0 1 1 0 1 1 0 3 3 1 0 0 1 32 1"
    "Windows arm64|aarch64-windows-gnu||3 0 1 0 1 1 0 1 1 0 3 3 1 0 0 1 64 1"
    "Windows x86_64, code-page paths|x86_64-windows-gnu|$narrow|3 0 1 0 1 1 0 1 1 0 3 3 1 0 0 1 64 0"
    "Windows x86, code-page paths|x86-windows-gnu|$narrow|3 0 1 0 1 1 0 1 1 0 3 3 1 0 0 1 32 0"
)

# the WASI rows:
#   label | flags | libraries | gates
wasi_rows=(
    "WASI||-|1 0 0 0 0 0 0 0 0 0 2 0 0 0 0 0 32 0"
    "WASI, clock emulated|-D_WASI_EMULATED_PROCESS_CLOCKS|-lwasi-emulated-process-clocks|1 0 0 0 0 0 0 0 1 0 2 0 0 0 0 0 32 0"
)

# the native rows, with neither POSIX nor Windows:
#   label | machine | flags | gates
native_rows=(
    "no platform, 64-bit|||1 0 0 0 0 0 0 1 1 0 2 0 0 0 0 0 64 0"
    "no platform, 32-bit|-m32|$large|1 0 0 0 0 0 0 1 1 0 2 0 0 0 0 0 64 0"
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

# gates_of <compiler and flags...>
#   the probe's gates for a build, preprocessed by the command given: one
# line of values in the probe's order. A value the preprocessor left as a
# name, or as anything else that is not arithmetic, reads as "?", so that it
# can match no row.
gates_of() {
    local name value line=""

    while read -r _ name value; do
        if [[ "$value" =~ ^[0-9()+\ -]+$ ]]; then
            line="$line $(( value ))"
        else
            line="$line ?"
        fi
    done < <("$@" -E "$probe" 2> /dev/null | grep '^D_FILE_GATE ')

    echo "${line# }"
}

# gates_match <label> <expected> <compiler and flags...>
#   succeeds when the build's gates are the row's; with PRINT_GATES=1, prints
# them as well.
gates_match() {
    local label="$1" expected="$2" actual
    shift 2

    actual="$(gates_of "$@")"
    [[ "$print_gates" == 1 ]] && echo "gates $label: $actual"
    if [[ "$actual" != "$expected" ]]; then
        fail "$label: gates $actual, recorded $expected"
        return 1
    fi
}

# build_all <directory> <log> <compiler and flags...>
#   compiles every c/filesys source into the directory, as the command says.
build_all() {
    local dir="$1" log="$2" src
    shift 2

    rm -rf "$dir" && mkdir -p "$dir"
    for src in src/djinterp/c/filesys/*.c; do
        "$@" -Iinc -c "$src" -o "$dir/$(basename "$src" .c).o" \
            >> "$log" 2>&1 || return 1
    done
}

# zig_row <row>
#   every source compiled for the row's target, and its gates.
zig_row() {
    local label target flags gates
    local log="$tmp/zig.log"

    IFS='|' read -r label target flags gates <<< "$1"
    : > "$log"

    if ! build_all "$tmp/zig" "$log" $zig cc -target "$target" $strict \
             $flags; then
        fail "$label: does not build" "$log"
        return
    fi

    gates_match "$label" "$gates" $zig cc -target "$target" $flags -Iinc &&
        pass "$label"
}

# run_wasi <program> <log>
#   runs a WASI program in a fresh scratch directory, its report in the log.
run_wasi() {
    local scratch="$tmp/wasi_scratch"

    rm -rf "$scratch" && mkdir -p "$scratch"
    $wasi_runner "$1" "$scratch" > "$2" 2>&1
}

# wasi_row <row>
#   every source compiled for WASI and linked whole, its gates, and the two
# harnesses run under the runtime.
wasi_row() {
    local label flags libs gates harness exe
    local log="$tmp/wasi.log" obj="$tmp/wasi" whole="$tmp/whole.c"

    IFS='|' read -r label flags libs gates <<< "$1"
    [[ "$libs" == "-" ]] && libs=""
    : > "$log"

    if ! build_all "$obj" "$log" $zig cc -target wasm32-wasi $strict $flags
    then
        fail "$label: does not build" "$log"
        return
    fi

    # every object, with nothing dropped: a call the libc lacks fails here
    printf 'int main(void) { return 0; }\n' > "$whole"
    if ! $zig cc -target wasm32-wasi "$whole" "$obj"/*.o $libs \
             -Wl,--no-gc-sections -o "$tmp/whole.wasm" >> "$log" 2>&1; then
        fail "$label: does not link whole" "$log"
        return
    fi

    gates_match "$label" "$gates" $zig cc -target wasm32-wasi $flags -Iinc ||
        return

    for harness in "$noplat" "$wasi"; do
        exe="$tmp/$(basename "$harness" .c).wasm"
        if ! $zig cc -target wasm32-wasi $strict $flags -Iinc "$harness" \
                 "$obj"/*.o $libs -o "$exe" >> "$log" 2>&1; then
            fail "$label: $(basename "$harness") does not build" "$log"
            return
        fi
        if ! run_wasi "$exe" "$tmp/run.log"; then
            fail "$label: $(basename "$harness") failed" "$tmp/run.log"
            return
        fi
    done

    pass "$label"
}

# native_row <row>
#   every source compiled natively with neither POSIX nor Windows, its gates,
# and t_file_noplat.c run against it.
native_row() {
    local label machine flags gates
    local log="$tmp/native.log" obj="$tmp/native"
    local scratch="$tmp/native_scratch" exe="$tmp/t_file_noplat"

    IFS='|' read -r label machine flags gates <<< "$1"
    : > "$log"

    if ! build_all "$obj" "$log" gcc $machine $strict $posix $flags \
             $noplat_flag; then
        fail "$label: does not build" "$log"
        return
    fi

    gates_match "$label" "$gates" gcc $machine $posix $flags $noplat_flag \
        -Iinc || return

    if ! gcc $machine $strict $posix $flags $noplat_flag -Iinc "$noplat" \
             "$obj"/*.o -o "$exe" >> "$log" 2>&1; then
        fail "$label: t_file_noplat.c does not build" "$log"
        return
    fi

    rm -rf "$scratch" && mkdir -p "$scratch"
    if ! (cd "$scratch" && "$exe") > "$tmp/run.log" 2>&1; then
        fail "$label: t_file_noplat.c failed" "$tmp/run.log"
        return
    fi

    pass "$label"
}

# can_build <compiler> <machine>
#   succeeds when the compiler can build and link a program for the machine
# (the 32-bit one needs the multilib packages).
can_build() {
    printf 'int main(void) { return 0; }\n' |
        "$1" $2 -x c - -o "$tmp/probe" > /dev/null 2>&1
}


for f in "$probe" "$noplat" "$wasi"; do
    [[ -f "$f" ]] || { echo "check_filesys_platforms: $f is missing" >&2; exit 2; }
done

have_zig=0
$zig version > /dev/null 2>&1 && have_zig=1
have_runtime=0
python3 -c "import wasmtime" > /dev/null 2>&1 && have_runtime=1
[[ -n "${WASI_RUNNER-}" ]] && have_runtime=1

for row in "${zig_rows[@]}"; do
    if [[ $have_zig == 1 ]]; then
        zig_row "$row"
    else
        skip "${row%%|*} (no zig: pip install ziglang)"
    fi
done

for row in "${wasi_rows[@]}"; do
    if [[ $have_zig == 1 && $have_runtime == 1 ]]; then
        wasi_row "$row"
    else
        skip "${row%%|*} (no zig or no WASI runtime: pip install ziglang wasmtime)"
    fi
done

for row in "${native_rows[@]}"; do
    machine="$(echo "$row" | cut -d'|' -f2)"
    if can_build gcc "$machine"; then
        native_row "$row"
    else
        skip "${row%%|*} (no gcc $machine to build with)"
    fi
done

reported=$((passes + failures + skips))
echo "check_filesys_platforms: $passes passed, $failures failed, $skips skipped"

if [[ -n "$expect_configs" && "$reported" -ne "$expect_configs" ]]; then
    echo "check_filesys_platforms: $reported configurations reported," \
         "$expect_configs expected"
    exit 1
fi

if [[ $failures -gt 0 ]] || [[ $skips -gt 0 && "$allow_skip" != 1 ]]; then
    exit 1
fi

exit 0
