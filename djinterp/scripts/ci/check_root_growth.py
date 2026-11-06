#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                            check_root_growth.py
#
# The framework roots only grow (decision 78 of the register, ruled
# 2026.11.04). Every lane adds to djinterp.hpp, djinterp_cpp.h (its C++
# keywords and namespace macros) and djinterp.h as it needs a keyword, a
# namespace or a macro, and a lane's whole-file copy of a root,
# written against its own view of it, can drop what another lane added since:
# relay 93 lost `namespace functional = ::djinterp;`, which five vparse
# headers need, and NS_UI, which seventeen ui headers need, and each loss was
# found only by recompiling what used it. This check finds it at the merge:
# it lists what each root declares at a base revision -- its macros, its
# namespaces and namespace aliases, its type aliases and its types -- and
# fails when the tree's root lacks one of them.
#   A declaration removed on purpose is named with --allow, in the commit
# that removes it.
#
#   usage:   python3 scripts/ci/check_root_growth.py [--base REV]
#                                                    [--allow NAME ...]
#   options: --base REV     the revision to compare with (default HEAD)
#            --allow NAME   a declaration the tree may lack (repeat)
#            --tree DIR     the tree to check (default: the one this script
#                           is in); REV is read from its repository
#   exit:    0 when each root declares everything it declared at REV; 1 when
#            one does not; 2 when the run could not start
#
#
# path:      /scripts/ci/check_root_growth.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
#                                                            revised: 2026.11.05
#*******************************************************************************
import argparse
import os
import re
import subprocess
import sys

TREE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))))
ROOTS = ['inc/djinterp/c/djinterp.h',        # the C root
         'inc/djinterp/c/djinterp_cpp.h',    # its C++ keywords, NS_* macros
         'inc/djinterp/cpp/djinterp.hpp']    # the C++ root
PATTERNS = [
    ('macro', re.compile(r'^\s*#\s*define\s+(\w+)', re.M)),
    ('namespace alias', re.compile(r'\bnamespace\s+(\w+)\s*=', re.M)),
    ('namespace', re.compile(r'\bnamespace\s+(\w+)\s*\{', re.M)),
    ('type alias', re.compile(r'\busing\s+(\w+)\s*=', re.M)),
    ('typedef', re.compile(r'\btypedef\b[^;{}]*?\b(\w+)\s*;', re.M)),
    ('type', re.compile(r'^\s*(?:template\s*<[^>]*>\s*)?(?:struct|class|'
                        r'union|enum(?:\s+class)?)\s+(\w+)\s*[:{]', re.M)),
]


def strip_comments(text):
    """The text without its comments, so that a name a comment mentions is
    not taken for a declaration."""
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def declarations(text):
    """What a root declares, as a set of (kind, name)."""
    text = strip_comments(text.replace('\r\n', '\n').replace('\\\n', ' '))
    found = set()
    for kind, pattern in PATTERNS:
        for name in pattern.findall(text):
            found.add((kind, name))
    return found


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--base', default='HEAD')
    parser.add_argument('--allow', action='append', default=[])
    parser.add_argument('--tree')
    options = parser.parse_args()

    tree = os.path.abspath(options.tree) if options.tree else TREE
    prefix = subprocess.run(['git', 'rev-parse', '--show-prefix'], cwd=tree,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            universal_newlines=True)
    if prefix.returncode != 0:
        print('check_root_growth.py: not in a git repository: %s' %
              prefix.stderr.strip(), file=sys.stderr)
        return 2

    lost_any = False
    for root in ROOTS:
        before = subprocess.run(['git', 'show', '%s:%s%s' % (
            options.base, prefix.stdout.strip(), root)], cwd=tree,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if before.returncode != 0:
            print('NOTE %s: not at %s, so nothing to hold it to' % (
                root, options.base))
            continue
        path = os.path.join(tree, root)
        if not os.path.isfile(path):
            print('FAIL %s: the root is gone' % root)
            lost_any = True
            continue
        with open(path, encoding='utf-8', errors='replace') as handle:
            now = declarations(handle.read())
        then = declarations(before.stdout.decode('utf-8', 'replace'))
        lost = sorted(item for item in then - now
                      if item[1] not in options.allow)
        for kind, name in lost:
            print('FAIL %s: lost the %s %s, which %s declared' % (
                root, kind, name, options.base))
        print('%s: %d declarations at %s, %d now, %d lost' % (
            root, len(then), options.base, len(now), len(lost)))
        lost_any = lost_any or bool(lost)

    print('check_root_growth: %s' % ('a root lost a declaration' if lost_any
                                     else 'the roots only grew'))
    return 1 if lost_any else 0


if __name__ == '__main__':
    sys.exit(main())
