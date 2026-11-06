#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                                 check_version.py
#
# The version header (inc/djinterp/c/dversion.h) states the version in five
# parts and one string, and they must agree: D_VERSION_STRING is
# MAJOR.MINOR.PATCH, with -PRERELEASE after it where the label is not empty,
# and D_VERSION_IS_RELEASE is 1 exactly when the label is empty. With --tag,
# the version must also be the one a release tag names (v0.8.0, or
# v0.8.0-rc.1 for a candidate), so that a tag and the tree it tags cannot
# disagree.
#
#   usage:   python3 scripts/ci/check_version.py [--tag TAG] [--tree DIR]
#   exit:    0 when the parts agree (and match the tag); 1 when they do not;
#            2 when the header cannot be read
#
#
# path:      /scripts/ci/check_version.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
#                                                            revised: 2026.11.05
#*******************************************************************************
import argparse
import os
import re
import sys

TREE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))))
HEADER = 'inc/djinterp/c/dversion.h'
DEFINE = re.compile(r'^\s*#\s*define\s+D_VERSION_(MAJOR|MINOR|PATCH|'
                    r'PRERELEASE|IS_RELEASE|STRING)\s+(\S+)\s*$', re.M)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tag')
    parser.add_argument('--tree')
    options = parser.parse_args()

    tree = os.path.abspath(options.tree) if options.tree else TREE
    try:
        with open(os.path.join(tree, HEADER), encoding='utf-8') as handle:
            text = handle.read()
    except OSError as error:
        print('check_version.py: %s' % error, file=sys.stderr)
        return 2

    parts = dict(DEFINE.findall(text))
    missing = [name for name in ('MAJOR', 'MINOR', 'PATCH', 'PRERELEASE',
                                 'IS_RELEASE', 'STRING') if name not in parts]
    if missing:
        print('FAIL %s defines no D_VERSION_%s' % (HEADER, missing[0]))
        return 1

    problems = []
    for name in ('MAJOR', 'MINOR', 'PATCH'):
        if not parts[name].isdigit():
            problems.append('D_VERSION_%s is %s, not a number' % (
                name, parts[name]))
    label = parts['PRERELEASE'].strip('"')
    numbers = '%s.%s.%s' % (parts['MAJOR'], parts['MINOR'], parts['PATCH'])
    want = numbers + ('-' + label if label else '')
    if parts['STRING'].strip('"') != want:
        problems.append('D_VERSION_STRING is %s; the parts make "%s"' % (
            parts['STRING'], want))
    if parts['IS_RELEASE'] != ('0' if label else '1'):
        problems.append('D_VERSION_IS_RELEASE is %s with the label "%s"' % (
            parts['IS_RELEASE'], label))
    if options.tag and options.tag.lstrip('v') != want:
        problems.append('the tag %s names %s; the header says %s' % (
            options.tag, options.tag.lstrip('v'), want))

    for problem in problems:
        print('FAIL %s: %s' % (HEADER, problem))
    print('check_version: %s %s' % (want, 'agrees' if not problems
                                    else 'does not agree'))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
