#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                         check_stdint_includes.py
#
# The style guides' rule for the fixed-width integers: they come from re_std,
# so no C or C++ file includes <stdint.h>, <cstdint>, <inttypes.h> or
# <cinttypes> itself. A C file includes re_std's cstdint/dstdint.h, a C++
# file cstdint/cstdint.hpp. dstdint.h, which wraps the system's header, and
# its conformance harness, which compares against it, are the two files the
# rule exempts, with bitwisedom's platform checks, which compile against the
# platform's own headers alone.
#   Some files still include the system's headers, until they are swept.
# --known holds a run to that list: a file the list has is allowed, a
# new one fails the run, and a file on the list that no longer includes one
# is reported so that its line can go. --write-known records a run's files
# as the list.
#
#   usage:   python3 scripts/ci/check_stdint_includes.py [options] [path ...]
#   options: --known FILE         the list of files allowed for now
#            --write-known FILE   write this run's files as the list
#            --tree DIR           the tree to check (default: the one this
#                                 script is in)
#   paths:   files or folders under the tree's root; default inc/, src/,
#            tests/
#   exit:    0 when no file outside the list includes one; 1 otherwise
#
#
# path:      /scripts/ci/check_stdint_includes.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
#                                                            revised: 2026.11.11
#*******************************************************************************
import argparse
import os
import re
import sys

TREE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))))
EXTENSIONS = ('.h', '.c', '.hpp', '.cpp', '.inl', '.ipp', '.tpp')
DIRECT = re.compile(r'^\s*#\s*include\s*<(stdint\.h|cstdint|inttypes\.h|'
                    r'cinttypes)>', re.M)
EXEMPT = {'inc/re_std/c/cstdint/dstdint.h',
          'tests/re_std/c/cstdint/t_dstdint.c'}
# bitwisedom's platform checks compile against the platform's own headers
# alone (check_platform.cmake gives them nothing of djinterp), so they may
# include <stdint.h> as the platform has it
EXEMPT_DIRS = ('tests/djinterp/tools/bitwisedom/platform/',)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('paths', nargs='*')
    parser.add_argument('--known')
    parser.add_argument('--write-known')
    parser.add_argument('--tree')
    options = parser.parse_args()

    tree = os.path.abspath(options.tree) if options.tree else TREE
    found = {}
    for item in options.paths or ['inc', 'src', 'tests']:
        full = os.path.join(tree, item)
        walk = ([(os.path.dirname(full), [], [os.path.basename(full)])]
                if os.path.isfile(full) else os.walk(full))
        for directory, names, entries in walk:
            names.sort()
            for entry in sorted(entries):
                if not entry.endswith(EXTENSIONS):
                    continue
                path = os.path.join(directory, entry)
                name = os.path.relpath(path, tree).replace(os.sep, '/')
                if (name in EXEMPT or name.startswith(EXEMPT_DIRS) or
                        (name.startswith('tests/') and '/data/' in name)):
                    continue
                with open(path, encoding='utf-8', errors='replace') as handle:
                    headers = DIRECT.findall(handle.read())
                if headers:
                    found[name] = sorted(set(headers))

    known = set()
    if options.known:
        with open(options.known, encoding='utf-8') as handle:
            known = set(line.split('\t')[0].strip() for line in handle
                        if line.strip() and not line.startswith('#'))

    new = sorted(name for name in found if name not in known)
    gone = sorted(name for name in known if name not in found and
                  os.path.isfile(os.path.join(tree, name)))
    for name in new:
        print('FAIL %s includes <%s>: take the fixed-width integers from '
              're_std' % (name, '>, <'.join(found[name])))
    for name in gone:
        print('FIXED %s: no longer includes one; remove its line from the '
              'list' % name)
    print('check_stdint_includes: %d files include one directly, %d of them '
          'new%s' % (len(found), len(new),
                     (', %d on the list' % (len(found) - len(new)))
                     if options.known else ''))

    if options.write_known:
        with open(options.write_known, 'w', encoding='utf-8',
                  newline='\n') as handle:
            handle.write('# djinterp: files that still include <stdint.h>, '
                         '<cstdint>, <inttypes.h> or <cinttypes> directly, '
                         'written by\n# scripts/ci/check_stdint_includes.py '
                         '--write-known; a line goes when its file is swept\n')
            for name in sorted(found):
                handle.write('%s\t%s\n' % (name, ', '.join(found[name])))
    return 1 if new else 0


if __name__ == '__main__':
    sys.exit(main())
