#!/usr/bin/env bash
#*******************************************************************************
# djinterp [ci]                                                       run_all.sh
#
# Every check a Unix machine can run on the tree, one after another, with
# one screen at the end: what passed, what failed, and the lines that say
# why. It is what to run on a machine of your own after a merge window, and
# what to send back is that screen.
#   The checks, in order:
#     style     scripts/ci/check_style.py, held to style_known.txt
#     stdint    scripts/ci/check_stdint_includes.py, held to stdint_known.txt
#     version   scripts/ci/check_version.py: the version header agrees
#     roots     scripts/ci/check_root_growth.py: the framework roots lost
#               nothing since BASE
#     ladder    scripts/ci/ladder.py, held to ladder_known.txt: every library
#               unit at every rung, plain and in testing mode, with GCC and
#               Clang (with --quick, the lowest and the highest rungs only,
#               plain)
#     ckd       scripts/ci/check_ckd.sh, the checked-arithmetic kit and the
#               container overflow fixes
#     found.    the parse/parsegen foundation's script, with --matrix
#     leaves    the CMake test leaves that stand alone (parse expr, parse
#               overflow, the foundation), configured, built and run with
#               CTest, where cmake is installed
#   A missing tool is a skip, not a failure, and the screen says which; a
# missing compiler is the same, through each check's own ALLOW_SKIP.
#
#   usage:   bash scripts/ci/run_all.sh [--quick]   (from anywhere)
#   env:     CC, CXX       the compilers for the foundation and the leaves
#                          (default gcc, g++)
#            EXTRA_FLAGS   added to check_ckd.sh's builds and the ladder's
#                          compiles (e.g. -m32)
#            JOBS          parallel jobs (default: the processors)
#            BASE          the revision the roots are held to (default
#                          HEAD~1, the commit before the last)
#   exit:    0 when every check that ran passed; 1 otherwise
#
#
# path:      /scripts/ci/run_all.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
#                                                            revised: 2026.11.05
#*******************************************************************************
set -u

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/../.." && pwd)"
quick=0
[[ "${1:-}" == "--quick" ]] && quick=1
python="$(command -v python3 || command -v python || true)"
export JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2> /dev/null || echo 4)}"
logs="$(mktemp -d "${TMPDIR:-/tmp}/djinterp-ci.XXXXXX")"
summary=()
failed=0

# step NAME PATTERN COMMAND...: runs a check from the tree's root, keeps its
# log, and records the lines of it that match PATTERN for the screen
step()
{
    local name="$1" pattern="$2" status=0
    shift 2

    echo "== $name" >&2
    (cd "$root" && "$@") > "$logs/$name.log" 2>&1
    status=$?

    if [[ $status -eq 0 ]]; then
        summary+=( "PASS  $name" )
    else
        summary+=( "FAIL  $name (exit $status; log: $logs/$name.log)" )
        failed=1
    fi
    while IFS= read -r line; do
        summary+=( "        ${line:0:200}" )
    done < <(grep -E "$pattern" "$logs/$name.log" | head -n 40)
}

skip()
{
    summary+=( "SKIP  $1: $2" )
}

totals='^(FAIL|FIXED|SKIP|NOTE)|passed|failed|new failures|files with|'
totals="${totals}include one|runs failing|tests passed|left out"

if [[ -n "$python" ]]; then
    step style "$totals" "$python" -I scripts/ci/check_style.py \
        --known scripts/ci/style_known.txt
    step stdint "$totals" "$python" -I scripts/ci/check_stdint_includes.py \
        --known scripts/ci/stdint_known.txt
    step version "$totals|^check_version" "$python" -I \
        scripts/ci/check_version.py
    step roots "$totals|^check_root_growth" "$python" -I \
        scripts/ci/check_root_growth.py --base "${BASE:-HEAD~1}"
    if [[ $quick -eq 1 ]]; then
        step ladder "$totals|^djinterp ladder" "$python" -I \
            scripts/ci/ladder.py --known scripts/ci/ladder_known.txt \
            --levels c99,c23,c++98s,c++26 --modes plain \
            --extra-flags "${EXTRA_FLAGS:-}"
    else
        step ladder "$totals|^djinterp ladder" "$python" -I \
            scripts/ci/ladder.py --known scripts/ci/ladder_known.txt \
            --extra-flags "${EXTRA_FLAGS:-}"
    fi
else
    skip "style, stdint, version, roots, ladder" "no python3"
fi

step ckd "^(FAIL|SKIP|NOTE)|^check_ckd" env ALLOW_SKIP=1 \
    bash scripts/ci/check_ckd.sh

foundation=build/cmake/config/testing/djinterp/c/modules/parsegen/foundation
step foundation "$totals" bash "$foundation/run_foundation_tests.sh" --matrix

if command -v cmake > /dev/null 2>&1; then
    for leaf in c/modules/parse/expr c/modules/parse/overflow \
                c/modules/parsegen/foundation; do
        name="leaf-${leaf##*/}"
        build="$logs/build-${leaf##*/}"
        step "$name" "$totals|Failed|Error" bash -c \
            "cmake -S 'build/cmake/config/testing/djinterp/$leaf' \
                   -B '$build' -DCMAKE_C_COMPILER='${CC:-gcc}' \
                   -DCMAKE_CXX_COMPILER='${CXX:-g++}' > '$build.cfg' 2>&1 &&
             cmake --build '$build' -j '$JOBS' > '$build.log' 2>&1 &&
             ctest --test-dir '$build' --output-on-failure"
    done
else
    skip leaves "no cmake"
fi

echo
echo "djinterp run_all on $(uname -sm), $(date '+%Y-%m-%d %H:%M')"
echo "  tree: $root"
echo "  $("${CC:-gcc}" --version 2> /dev/null | head -n 1)"
printf '%s\n' "${summary[@]}"
echo "run_all: $([[ $failed -eq 0 ]] && echo 'every check passed' ||
                 echo 'something failed')"
exit $failed
