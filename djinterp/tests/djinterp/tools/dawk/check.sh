#!/bin/sh
# check.sh -- build dawk with its extensions from the tree, and run every
# suite that needs nothing outside it: the unit tests, regress.sh, story.sh,
# cmake.sh, which also reads every CMake file under build/cmake and, where
# cmake is installed, compares the CMake reader with CMake itself,
# ninja.sh, which, where ninja is installed, has ninja answer as dninja does,
# msvs.sh, which reads every Visual Studio file there and, where python3 is
# installed, has Python read what it generates, wiki.sh, which proves
# every page it makes sound and, where markdown-it-py or pandoc is
# installed, has them read the pages too, and git.sh, which, where git is
# installed, has git answer as dgit does over repositories it makes.
#
# The differential and T.* suites need another awk and one-true-awk's testdir,
# so they run apart; see diff_awk.sh and conformance.sh. dcheck builds through
# CMake, and its suites are ctest's.
#
#   sh tests/djinterp/tools/dawk/check.sh [build directory]
#
# Run it from the repository root. CC names the compiler, cc by default, and
# CFLAGS adds to the flags; SAN=1 builds everything under AddressSanitizer and
# UBSan. It stops at the first build that fails, and exits 1 when any suite
# does.
set -e
OUT=${1:-build/dawk}
CC=${CC:-cc}
S=src/djinterp/tools/dawk
T=tests/djinterp/tools/dawk
W="-std=c11 -D_XOPEN_SOURCE=700 -Wall -Wextra -Wshadow -pedantic"
if [ "${SAN:-0}" = 1 ]; then
    W="$W -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer"
else
    W="$W -O2"
fi
W="$W ${CFLAGS:-}"

# the interpreter; then each extension's units, in ext/<name>/, which use
# only the interpreter's and the shared dsplice, dxml, dzip and dfs
E=$S/ext
CORE="$S/dinterp.c $S/dparse.c $S/dlex.c $S/dregex.c $S/dvalue.c $S/dtree.c
      $S/dfs.c $S/dsettings.c"
STORY="$S/dzip.c $S/dxml.c $E/story/dhost_story.c $E/story/dhost_articulate.c"
DCMAKE="$E/cmake/dcmake.c $E/cmake/dcmake_doc.c $S/dsplice.c"
CMAKE="$DCMAKE $E/cmake/dhost_cmake.c"
DNINJA="$E/ninja/dninja.c $E/ninja/dninja_eval.c $E/ninja/dninja_doc.c
        $S/dsplice.c"
NINJA="$E/ninja/dninja.c $E/ninja/dninja_eval.c $E/ninja/dninja_doc.c
       $E/ninja/dhost_ninja.c"
DMSVS="$E/msvs/dmsvs.c $E/msvs/dmsvs_doc.c $S/dxml.c $S/dsplice.c"
MSVS="$E/msvs/dmsvs.c $E/msvs/dmsvs_doc.c $E/msvs/dhost_sln.c
      $E/msvs/dhost_vcxproj.c $E/msvs/dhost_msvs.c"
DWIKI="$E/wiki/dwiki.c $E/wiki/dwiki_markdown.c $E/wiki/dwiki_mediawiki.c
       $E/wiki/dwiki_doc.c"
WIKI="$DWIKI $E/wiki/dhost_wiki.c"
DGIT="$E/git/dgit_text.c $E/git/dgit_doc.c $E/git/dgit_file.c
      $E/git/dgit_hash.c $E/git/dgit_inflate.c $E/git/dgit_object.c
      $E/git/dgit_repo.c $E/git/dgit_index.c"
GIT="$DGIT $E/git/dhost_git.c"

mkdir -p "$OUT"
$CC $W -o "$OUT/dawk" $S/main.c $CORE $STORY $CMAKE $NINJA $MSVS $WIKI \
    $GIT -lm
$CC $W -o "$OUT/test_dregex" $T/test_dregex.c $S/dregex.c
$CC $W -o "$OUT/test_dvalue" $T/test_dvalue.c $S/dvalue.c -lm
$CC $W -o "$OUT/test_dparse" $T/test_dparse.c $S/dparse.c $S/dlex.c \
    $S/dregex.c
$CC $W -o "$OUT/test_dzip" $T/test_dzip.c $S/dzip.c
$CC $W -o "$OUT/test_dxml" $T/test_dxml.c $S/dxml.c
$CC $W -o "$OUT/test_dhost_story" $T/ext/story/test_dhost_story.c $CORE \
    $STORY $CMAKE -lm
$CC $W -o "$OUT/test_dcmake" $T/ext/cmake/test_dcmake.c $DCMAKE $S/dfs.c
$CC $W -o "$OUT/test_dcmake_doc" $T/ext/cmake/test_dcmake_doc.c $DCMAKE \
    $S/dfs.c
$CC $W -o "$OUT/test_dninja" $T/ext/ninja/test_dninja.c $DNINJA $S/dfs.c
DMAKE="$E/make/dmake.c $E/make/dmake_gnu.c $E/make/dmake_posix.c
    $E/make/dmake_bsd.c $E/make/dmake_nmake.c $E/make/dmake_doc.c"
$CC $W -o "$OUT/test_dmake" $T/ext/make/test_dmake.c $DMAKE $S/dsplice.c \
    $S/dfs.c
$CC $W -o "$OUT/test_dmsvs" $T/ext/msvs/test_dmsvs.c $E/msvs/dmsvs.c \
    $S/dxml.c $S/dfs.c $S/dsplice.c
$CC $W -o "$OUT/test_dmsvs_doc" $T/ext/msvs/test_dmsvs_doc.c $DMSVS $S/dfs.c
$CC $W -o "$OUT/test_dwiki" $T/ext/wiki/test_dwiki.c $DWIKI $S/dfs.c
$CC $W -o "$OUT/test_dgit" $T/ext/git/test_dgit.c $DGIT $S/dfs.c
set +e

# the unit tests write what they need into a directory of their own
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
failed=0
for unit in test_dregex test_dvalue test_dparse test_dzip test_dxml \
            test_dhost_story test_dcmake test_dcmake_doc test_dninja \
            test_dmake test_dmsvs test_dmsvs_doc test_dwiki test_dgit; do
    printf '%-18s ' "$unit"
    if out=$("$OUT/$unit" "$work" 2>&1); then
        printf '%s\n' "$out" | tail -n 1
    else
        failed=1
        echo "FAILED"
        printf '%s\n' "$out" | grep -v '^  ok' | sed 's/^/    /'
    fi
done

# the scripts print their failures in full, and their count last; each
# takes dawk and the helper it builds its cases with
for suite in regress.sh:test_dhost_story ext/story/story.sh:test_dhost_story \
             ext/cmake/cmake.sh:test_dcmake ext/ninja/ninja.sh:test_dninja \
             ext/msvs/msvs.sh:test_dmsvs ext/wiki/wiki.sh:test_dwiki \
             ext/git/git.sh:test_dgit; do
    helper=${suite#*:}
    suite=${suite%%:*}
    printf '%-18s ' "${suite##*/}"
    if out=$(sh "$T/$suite" "$OUT/dawk" "$OUT/$helper" 2>&1); then
        printf '%s\n' "$out" | tail -n 1
    else
        failed=1
        echo "FAILED"
        printf '%s\n' "$out" | sed 's/^/    /'
    fi
done

[ "$failed" -eq 0 ] && echo "all passed" || echo "something failed"
exit "$failed"
