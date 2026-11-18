#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                                   banner_tags.py
#
# Converts banners to the tag their path derives (ruled 2026.11.17): the
# title line becomes "* djinterp [<language>][<module>]", the file's name on
# character 80, by check_style.py's own rule, so the two can never disagree.
# Where a banner has one blank * line before path:, it gains the second.
# Nothing else in a file changes: not its revised: date, not its line endings.
#   Idempotent: a converted file is left as it is, so the script can run over
# the whole tree once and over every zip after, as often as it likes.
#   Files check_style.py skips (generated ones, test data) are skipped; a
# file whose banner's title doesn't name the file is listed and left alone.
# Where the tag and the name can't both fit in 80 characters, one space
# separates them, which check_style.py accepts.
#
#   usage:   python3 scripts/ci/banner_tags.py [--write] [--tree DIR]
#                                              [PATH ...]
#            PATH: files or folders; the default is inc/, src/, tests/ and
#            build/cmake/config/testing/. Without --write, it only lists
#            what it would change.
#   exit:    0 when nothing is left to convert (or --write converted it all);
#            1 when files would change, or were left alone
#
#
# path:      /scripts/ci/banner_tags.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
#                                                            revised: 2026.11.18
#*******************************************************************************

import argparse
import importlib.util
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TITLE = re.compile(r'^\* (\S+)( +\[[^\]]*\](?:\[[^\]]*\])?)? +(\S+)$')


def load_check_style(tree):
    """check_style.py, from beside this script, with its tree set: the rule
    for tags lives there once."""
    spec = importlib.util.spec_from_file_location(
        'check_style', os.path.join(HERE, 'check_style.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    if tree:
        module.TREE = os.path.abspath(tree)
    return module


def title_line(tag, name):
    """The title: "* djinterp TAG", spaces, the name ending on character 80;
    where the two can't fit, one space between them, as check_style.py
    accepts."""
    head = '* djinterp ' + tag
    room = 80 - len(head) - len(name)
    return head + ' ' * max(room, 1) + name


def convert(style, path, text):
    """(new text, problem): the text with its banner converted, or None when
    nothing changes; problem says why a file was left alone."""
    newline = '\r\n' if '\r\n' in text else '\n'
    lines = text.replace('\r\n', '\n').split('\n')
    if len(lines) < 3 or lines[0] != style.RULE_OPEN:
        return None, None
    try:
        close = lines.index(style.RULE_CLOSE)
    except ValueError:
        return None, None
    if style.GENERATED.search('\n'.join(lines[:14])):
        return None, None
    tag = style.derive_tag(path)
    if tag is None:
        return None, None
    match = TITLE.match(lines[1])
    name = os.path.basename(path)
    if not match or match.group(3) != name:
        return None, 'the title does not name the file'
    title = title_line(tag, name)
    changed = False
    if lines[1] != title:
        lines[1] = title
        changed = True
    for number in range(2, close):
        if lines[number].startswith('* path:'):
            blanks = 0
            while (blanks < 2 and number - 1 - blanks > 1 and
                   lines[number - 1 - blanks] == '*'):
                blanks += 1
            if blanks == 1:
                lines.insert(number, '*')
                changed = True
            break
    if not changed:
        return None, None
    return newline.join(lines), None


def collect(style, paths):
    """The C and C++ files under paths, as check_style.py walks them."""
    found = []
    for item in paths:
        full = os.path.join(style.TREE, item)
        if os.path.isfile(full):
            if full.endswith(style.EXTENSIONS):
                found.append(style.rel(full))
            continue
        for directory, names, entries in os.walk(full):
            names.sort()
            if style.is_test_data(style.rel(directory) + '/'):
                continue
            for entry in sorted(entries):
                if entry.endswith(style.EXTENSIONS):
                    found.append(style.rel(os.path.join(directory, entry)))
    return found


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('paths', nargs='*')
    parser.add_argument('--write', action='store_true')
    parser.add_argument('--tree')
    args = parser.parse_args()
    style = load_check_style(args.tree)
    paths = args.paths or ['inc', 'src', 'tests',
                           'build/cmake/config/testing']
    converted, left = 0, []
    for path in collect(style, paths):
        full = os.path.join(style.TREE, path)
        with open(full, 'rb') as handle:
            data = handle.read()
        bom = data.startswith(b'\xef\xbb\xbf')
        try:
            text = data[3 if bom else 0:].decode('utf-8')
        except UnicodeDecodeError:
            left.append((path, 'not valid UTF-8'))
            continue
        new, problem = convert(style, path, text)
        if problem:
            left.append((path, problem))
        if new is None:
            continue
        converted += 1
        if args.write:
            with open(full, 'wb') as handle:
                handle.write((b'\xef\xbb\xbf' if bom else b'') +
                             new.encode('utf-8'))
        else:
            print('would convert %s' % path)
    for path, problem in left:
        print('left alone: %s: %s' % (path, problem))
    print('banner_tags: %d %s, %d left alone' % (
        converted, 'converted' if args.write else 'to convert', len(left)))
    return 1 if (left or (converted and not args.write)) else 0


if __name__ == '__main__':
    sys.exit(main())
