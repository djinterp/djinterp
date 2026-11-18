#!/usr/bin/env python3
#*******************************************************************************
# djinterp [scripts]                                              macro_frame.py
#
# Finishes a generated macro table the way the tree keeps it, so that
# regenerating the tables changes nothing that the generators didn't change.
# Every generator writes through write() here; C-Macro-Generator.ps1's alias
# headers come in on standard input.
#   A table that defines a variadic macro is framed (variadic macros at
# C++98, ruled 2026.11.17): after its first line, env.h's include, the
# D_ENV_PP_HAS_VARIADIC_MACROS guard and D_ENV_PP_VARIADIC_QUIET_BEGIN; at
# its end, D_ENV_PP_VARIADIC_QUIET_END and the guard's #endif. Without
# variadic macros (ISO strict C++98) the table is then empty, and with them
# a pedantic C++98 build stays quiet about the table's own definitions.
#   The banner is kept: a file that already has one keeps it, and when its
# text below the banner is unchanged the file isn't written at all. When the
# text changes, the banner's revised: date becomes today's. A new file gets a
# new banner, created and revised today, with the tag its path derives
# (check_style.py's rule).
#   Line endings follow the file being replaced (CRLF or LF); a new file is
# written with LF, which Git stores either way.
#
#   usage:   python3 scripts/macro_frame.py PATH < TEXT
#            writes TEXT to PATH as above; prints whether it changed
#
#
# path:      /scripts/macro_frame.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
#                                                            revised: 2026.11.18
#*******************************************************************************

import datetime
import importlib.util
import os
import re
import sys


# the tables live in inc/djinterp/c/util/macro/, wherever they are generated
TABLE_DIR = 'inc/djinterp/c/util/macro'
ENV_INCLUDE = '../../../env/env.h'

VARIADIC_DEFINE = re.compile(
    r'^[ \t]*#[ \t]*define[ \t]+\w+\([^)]*\.\.\.[ \t]*\)', re.MULTILINE)

FRAME_HEAD = [
    '/* Variadic, so absent without variadic macros (ISO strict C++98). */',
    '#include "%s"  /* D_ENV_PP_HAS_VARIADIC_MACROS */' % ENV_INCLUDE,
    '#if D_ENV_PP_HAS_VARIADIC_MACROS',
    'D_ENV_PP_VARIADIC_QUIET_BEGIN',
]
FRAME_TAIL = [
    'D_ENV_PP_VARIADIC_QUIET_END',
    '#endif  /* D_ENV_PP_HAS_VARIADIC_MACROS */',
]

BANNER_RULE = '*' * 79
BANNER_END = re.compile(r'^\*+/$')
REVISED = re.compile(r'revised: (\d{4}\.\d{2}\.\d{2}|TBA)')


def today():
    return datetime.date.today().strftime('%Y.%m.%d')


def frame(text):
    """The table's text, framed when it defines a variadic macro; LF line
    endings, one final newline."""
    lines = text.replace('\r\n', '\n').split('\n')
    while lines and lines[-1] == '':
        lines.pop()
    if (VARIADIC_DEFINE.search(text) and
            not any(l == FRAME_HEAD[2] for l in lines)):
        lines = lines[:1] + FRAME_HEAD + lines[1:] + FRAME_TAIL
    return '\n'.join(lines) + '\n'


def split_banner(text):
    """(banner, rest) for a text whose first line opens a banner; otherwise
    (None, text). Both keep LF line endings."""
    lines = text.split('\n')
    if not lines or not lines[0].startswith('/****'):
        return None, text
    for i, line in enumerate(lines[1:], 1):
        if BANNER_END.match(line):
            return ('\n'.join(lines[:i + 1]) + '\n',
                    '\n'.join(lines[i + 1:]))
    return None, text


def derived_tag(name):
    """The tag check_style.py derives for a table, read from the tree this
    script is in; [c][utility] where that can't be loaded."""
    here = os.path.dirname(os.path.abspath(__file__))
    checker = os.path.join(here, 'ci', 'check_style.py')
    try:
        spec = importlib.util.spec_from_file_location('check_style', checker)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        tag = module.derive_tag(TABLE_DIR + '/' + name)
        if tag:
            return tag
    except Exception:
        pass
    return '[c][utility]'


def right(left, right_text):
    """left, spaces, then right_text ending on character 80."""
    return left + ' ' * max(80 - len(left) - len(right_text), 1) + right_text


def new_banner(name):
    date = today()
    return '\n'.join([
        '/' + BANNER_RULE,
        right('* djinterp ' + derived_tag(name), name),
        '*',
        '*',
        '* path:      /%s/%s' % (TABLE_DIR, name),
        '* link(s):   TBA',
        right('* author(s): TBA', 'created: ' + date),
        right('*', 'revised: ' + date),
        BANNER_RULE + '/',
    ]) + '\n'


def finish(path, text):
    """(bytes to write, or None when the file already says this)."""
    body = frame(text)
    old = None
    newline = '\n'
    if os.path.exists(path):
        with open(path, 'rb') as f:
            raw = f.read()
        if b'\r\n' in raw:
            newline = '\r\n'
        old = raw.decode('utf-8').replace('\r\n', '\n')
    banner, old_body = split_banner(old) if old is not None else (None, None)
    if banner is not None:
        if old_body == body:
            return None
        banner = REVISED.sub('revised: ' + today(), banner, count=1)
    else:
        banner = new_banner(os.path.basename(path))
    return (banner + body).replace('\n', newline).encode('utf-8')


def write(path, text):
    """Writes the finished table to path; True when the file changed."""
    data = finish(path, text)
    if data is None:
        return False
    folder = os.path.dirname(path)
    if folder:
        os.makedirs(folder, exist_ok=True)
    with open(path, 'wb') as f:
        f.write(data)
    return True


def main():
    if len(sys.argv) != 2:
        sys.stderr.write('usage: macro_frame.py PATH < TEXT\n')
        return 2
    text = sys.stdin.buffer.read().decode('utf-8')
    changed = write(sys.argv[1], text)
    print('%s %s' % ('Wrote' if changed else 'Unchanged', sys.argv[1]))
    return 0


if __name__ == '__main__':
    sys.exit(main())
