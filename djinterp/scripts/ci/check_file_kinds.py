#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                             check_file_kinds.py
#
# Each top folder holds its own kinds of file (the owner's rule of
# 2026.11.25, with what he ruled kept on 2026.11.26):
#   inc/    C and C++ headers (.h, .hpp); a table included more than once
#           (.def); and, in inc/re_std/, a header with no extension, which
#           stands in for the standard's header of its name (<optional>).
#   src/    C and C++ sources (.c, .cpp); the headers only their own folder
#           includes, which are never installed (.h, .hpp); and .def tables.
#   tests/  code (.c, .cpp, .h, .hpp, .inc) and what a test needs beside it:
#           its data (.expected, .says, .expr, .dss, .json, .txt) and its
#           drivers (.sh, .py, .php, .cmake, .ninja).
#   docs/   the documents. A document is HTML, and lives in docs/ alone.
# Anywhere: no Markdown (a document is HTML, in docs/), no archive (a zip
# is how a change travels, not what the tree keeps), and Python only in
# scripts/ and tests/. A .gitignore or a .gitattributes may sit in any
# folder.
#   It reads every file git tracks and every file it would add (untracked,
# not ignored), so a file a change brings in is read before it is added.
#
#   usage:   python3 scripts/ci/check_file_kinds.py [options] [path ...]
#   options: path         only these files (from the tree's top)
#            --tree DIR   the tree (default: the one this script is in)
#            --list       print each kind each top folder holds, and how
#                         many of it (a report)
#   exit:    0 when every file is where its kind may be; 1 when one is not;
#            2 when the run could not start
#
#
# path:      /scripts/ci/check_file_kinds.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.26
#                                                            revised: 2026.11.26
#*******************************************************************************
import argparse
import collections
import os
import subprocess
import sys

TREE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))))

HEADERS = ('.h', '.hpp')
SOURCES = ('.c', '.cpp')
TABLE = ('.def',)
TEST_CODE = SOURCES + HEADERS + ('.inc',)
TEST_DATA = ('.expected', '.says', '.expr', '.dss', '.json', '.txt')
TEST_DRIVERS = ('.sh', '.py', '.php', '.cmake', '.ninja')
DOCUMENT = ('.html', '.htm')
MARKDOWN = ('.md', '.markdown', '.mdown', '.mkd')
ARCHIVE = ('.zip', '.gz', '.tgz', '.tar', '.bz2', '.tbz2', '.xz', '.txz',
           '.7z', '.rar', '.zst', '.lz', '.lzma', '.cab', '.jar')
PYTHON = ('.py', '.pyw')
ANYWHERE = ('.gitignore', '.gitattributes')


def kind(path):
    """a file's kind: its extension, lower case, or its whole name when it
    has none (a dot file, or a name with no dot)"""
    base = os.path.basename(path)
    stem, extension = os.path.splitext(base)
    if not extension or not stem:
        return base
    return extension.lower()


def refusal(path):
    """why path's folder may not hold it, or None when it may"""
    base = os.path.basename(path)
    if base in ANYWHERE:
        return None
    what = kind(path)
    top = path.split('/', 1)[0] if '/' in path else ''
    if what in MARKDOWN:
        return 'Markdown: a document is HTML, in docs/'
    if what in ARCHIVE:
        return 'an archive: the tree keeps no archive'
    if what in PYTHON and top not in ('scripts', 'tests'):
        return 'Python: in scripts/ or tests/ alone'
    if what in DOCUMENT and top != 'docs':
        return 'a document: in docs/ alone'
    if top == 'inc':
        if what in HEADERS + TABLE:
            return None
        if path.startswith('inc/re_std/') and '.' not in base:
            return None
        return 'inc/ holds .h, .hpp and .def, and re_std\'s ' \
               'headers with no extension'
    if top == 'src':
        if what in SOURCES + HEADERS + TABLE:
            return None
        return 'src/ holds .c, .cpp, its private .h and .hpp, and .def'
    if top == 'tests':
        if what in TEST_CODE + TEST_DATA + TEST_DRIVERS:
            return None
        return 'tests/ holds code, its data (%s) and its drivers (%s)' % (
            ', '.join(TEST_DATA), ', '.join(TEST_DRIVERS))
    return None


def files(tree):
    """every file git tracks under tree, and every file it would add, from
    tree's top"""
    def listed(*arguments):
        out = subprocess.run(['git', '-C', tree, 'ls-files', '-z'] +
                             list(arguments) + ['--', '.'],
                             capture_output=True, check=True).stdout
        return [p for p in out.decode('utf-8', 'replace').split('\0') if p]
    found = listed() + listed('--others', '--exclude-standard')
    # a tracked file the work tree deleted is not there to be read
    return sorted(set(p for p in found
                      if os.path.isfile(os.path.join(tree, p))))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('paths', nargs='*')
    parser.add_argument('--tree', default=TREE)
    parser.add_argument('--list', action='store_true')
    options = parser.parse_args()

    tree = os.path.abspath(options.tree)
    if options.paths:
        paths = []
        for path in options.paths:
            path = path.replace('\\', '/')
            if path.startswith('./'):
                path = path[2:]
            if not os.path.isfile(os.path.join(tree, path)):
                print('check_file_kinds: no such file: %s' % path,
                      file=sys.stderr)
                return 2
            paths.append(path)
    else:
        try:
            paths = files(tree)
        except (subprocess.CalledProcessError, OSError):
            print('check_file_kinds: %s is not in a git tree' % tree,
                  file=sys.stderr)
            return 2

    if options.list:
        tally = collections.defaultdict(collections.Counter)
        for path in paths:
            top = path.split('/', 1)[0] if '/' in path else '.'
            tally[top][kind(path)] += 1
        for top in sorted(tally):
            print('%s/' % top if top != '.' else './')
            for what, count in sorted(tally[top].items(),
                                      key=lambda item: (-item[1], item[0])):
                print('    %-16s %d' % (what, count))

    refused = [(path, refusal(path)) for path in paths]
    refused = [(path, why) for path, why in refused if why]
    for path, why in refused:
        print('FAIL %s: %s' % (path, why))
    print('check_file_kinds: %d file%s read, %d where %s folder does not '
          'hold %s kind' % (len(paths), '' if len(paths) == 1 else 's',
                            len(refused),
                            'its' if len(refused) == 1 else 'their',
                            'its' if len(refused) == 1 else 'their'))
    return 1 if refused else 0


if __name__ == '__main__':
    sys.exit(main())
