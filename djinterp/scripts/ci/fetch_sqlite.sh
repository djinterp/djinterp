#!/usr/bin/env bash
#*******************************************************************************
# djinterp [ci]                                                  fetch_sqlite.sh
#
# Makes the SQLite amalgamation -- sqlite3.c and sqlite3.h, and sqlite3ext.h
# for building extensions -- of each release named, from SQLite's official
# mirror on GitHub, for scripts/ci/check_db_sqlite.sh.
# Each lands in <out>/<version>/; a version already there is kept.
#   The amalgamation is generated from the release's own tree, by its own
# configure and `make sqlite3.c`, so what is tested is exactly the release.
# That needs git, a C compiler, make, and, for releases before 3.48, tclsh
# (Debian and Ubuntu: tcl).
#   The default list is the spread the layer is checked against: its floor,
# the releases the long-lived distributions ship, and the newest.
#
#   usage:   scripts/ci/fetch_sqlite.sh <out> [<version> ...]
#   exit:    0 when every release was made, 1 when one was not
#
# path:      /scripts/ci/fetch_sqlite.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
#                                                            revised: 2026.10.21
#*******************************************************************************
set -u

if [[ $# -lt 1 ]]; then
    echo "usage: $0 <out> [<version> ...]" >&2
    exit 2
fi

out="$(mkdir -p "$1" && cd "$1" && pwd)"
shift

# the floor (RHEL 7), Ubuntu 16.04, 18.04, RHEL 8, Ubuntu 20.04, RHEL 9 and
# Debian 11, Ubuntu 22.04, Debian 12, Debian 13, a 2025 release, and the newest
if [[ $# -eq 0 ]]; then
    set -- 3.7.17 3.11.0 3.22.0 3.26.0 3.31.1 3.34.1 3.37.2 3.40.1 3.46.1 \
           3.50.4 3.54.0
fi

status=0
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

for version in "$@"; do
    if [[ -f "$out/$version/sqlite3.c" && -f "$out/$version/sqlite3.h" ]]; then
        echo "have $version"
        continue
    fi

    echo "making $version"
    source="$work/src-$version"
    build="$work/build-$version"
    if ! git clone -q --depth 1 --branch "version-$version" \
             https://github.com/sqlite/sqlite "$source" 2> "$work/$version.log"
    then
        echo "  cannot fetch version-$version" >&2
        status=1
        continue
    fi

    mkdir -p "$build"
    if ! ( cd "$build" &&
           "$source/configure" --disable-tcl > configure.log 2>&1 &&
           make sqlite3.c > make.log 2>&1 ); then
        echo "  cannot make sqlite3.c for $version (see $build)" >&2
        status=1
        continue
    fi

    mkdir -p "$out/$version"
    cp "$build/sqlite3.c" "$build/sqlite3.h" "$source/src/sqlite3ext.h" \
       "$out/$version/"
    rm -rf "$source" "$build"
done

exit "$status"
