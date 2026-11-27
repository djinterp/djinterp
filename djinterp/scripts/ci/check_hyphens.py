#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                                check_hyphens.py
#
# The owner's rule for dashes in prose: in every file of the tree, a dash
# is one hyphen with a space each side, never two hyphens; at a line's end,
# where the sentence goes on below, one hyphen after a space. Prose is a C or
# C++ comment, a document (a .md file, or a .txt under docs/, outside its
# fenced code and code spans; an .html page, outside its pre, code, script
# and style elements and its tags), and a # comment of CMake, Python, shell,
# YAML and PowerShell files. Strings and code are not prose: a program's
# messages and a command's "--" are left alone, as is data a test or a tool
# reads byte for byte (fixtures, expected output, corpora) and the punch
# lists.
#   Without --write it lists every file whose prose holds a double hyphen,
# and fails if any does. With --write it turns them, and for a C or C++
# file checks that the file's tokens, comments stripped, are unchanged,
# leaving the file alone if they are not.
#
#   usage:   python3 scripts/ci/check_hyphens.py [options] [path ...]
#   options: --write    turn every prose double hyphen into one
#            --no-tokens
#                       with --write, skip the token check (it needs GCC),
#                       where the result is checked another way, as a kit
#                       checks its tree
#            --list     with --write, each file turned and its count
#            --tree DIR the tree to check (default: the one this script
#                       is in)
#   paths:   files or folders under the tree's root (or absolute paths
#            inside it); default the whole tree. A file named is checked
#            as a walk of its folder would check it
#   exit:    0 when no prose holds a double hyphen (or --write turned them
#            all); 1 otherwise
#
#
# path:      /scripts/ci/check_hyphens.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
#                                                            revised: 2026.11.26
#*******************************************************************************
import argparse
import os
import re
import subprocess
import sys

C_EXT = ('.h', '.c', '.hpp', '.cpp', '.inl', '.ipp', '.tpp', '.inc')
DOC_EXT = ('.md',)
HTML_EXT = ('.html', '.htm')
DOC_TXT_DIRS = ('docs/',)
# data a test or a tool reads byte for byte, not prose
DATA_PARTS = ('/fixtures/', '/fixture/', '/expected/', '/golden/', '/corpus/',
              '/data/', '/samples/', '/testdata/')
HASH_EXT = ('.cmake', '.py', '.sh', '.yml', '.yaml', '.ps1')
HASH_NAMES = ('CMakeLists.txt',)
SKIP_DIRS = ('.git', 'build/out', '__pycache__')
# lists and data the tools read, not prose
SKIP_FILES = ('scripts/ci/style_known.txt', 'scripts/ci/ladder_known.txt',
              'scripts/ci/stdint_known.txt', 'scripts/ci/crt_known.txt')


# a dash of two hyphens at a line's end, after a word: 'word --'
LINE_END = re.compile(r'(?<=[^\s-]) --(?=\r?$)', re.M)


def fix(text):
    return LINE_END.sub(' -', text.replace(' -- ', ' - '))


def dashes(text):
    """How many two-hyphen dashes a text holds, mid-line and at a line's
    end, prose or not."""
    return text.count(' -- ') + len(LINE_END.findall(text))


def c_comments(text):
    """C or C++ text with its comments' double hyphens turned."""
    out = []
    i, n = 0, len(text)
    while i < n:
        if text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            out.append(fix(text[i:j]))
            i = j
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append(fix(text[i:j]))
            i = j
        elif text[i] in '"\'':
            q = text[i]
            # a raw string literal R"delim( ... )delim"
            if q == '"' and i > 0 and text[i - 1] == 'R' and \
                    (i < 2 or not (text[i - 2].isalnum() or text[i - 2] == '_')):
                m = re.match(r'"([^()\\\s]{0,16})\(', text[i:])
                if m:
                    end = text.find(')' + m.group(1) + '"', i)
                    end = n if end < 0 else end + len(m.group(1)) + 2
                    out.append(text[i:end])
                    i = end
                    continue
            j = i + 1
            while j < n and text[j] != q and text[j] != '\n':
                j += 2 if text[j] == '\\' else 1
            j = min(j + 1, n)
            out.append(text[i:j])
            i = j
        else:
            out.append(text[i])
            i += 1
    return ''.join(out)


def markdown(text):
    """A document with its double hyphens turned outside fenced code and
    code spans."""
    out = []
    fence = None
    for line in text.split('\n'):
        stripped = line.lstrip()
        m = re.match(r'(```+|~~~+)', stripped)
        if m:
            if fence is None:
                fence = m.group(1)[0] * 3
            elif stripped.startswith(fence):
                fence = None
            out.append(line)
            continue
        if fence is not None:
            out.append(line)
            continue
        parts = re.split(r'(`+[^`]*`+)', line)
        out.append(''.join(p if p.startswith('`') else fix(p) for p in parts))
    return '\n'.join(out)


HTML_CODE = re.compile(r'(<(pre|code|script|style)\b[^>]*>.*?</\2\s*>|<[^>]*>)',
                       re.S | re.I)


def html_prose(text):
    """An HTML page with the double hyphens of its prose turned: text
    outside its tags and its pre, code, script and style elements."""
    out = []
    last = 0
    for m in HTML_CODE.finditer(text):
        out.append(fix(text[last:m.start()]))
        out.append(m.group(0))
        last = m.end()
    out.append(fix(text[last:]))
    return ''.join(out)


def hash_comments(text):
    """A script or CMake file with the double hyphens of its # comments
    turned:
    a # that begins a line's comment, not one in a string."""
    out = []
    for line in text.split('\n'):
        k = comment_start(line)
        out.append(line if k < 0 else line[:k] + fix(line[k:]))
    return '\n'.join(out)


def comment_start(line):
    quote = None
    i = 0
    while i < len(line):
        c = line[i]
        if quote:
            if c == '\\':
                i += 2
                continue
            if c == quote:
                quote = None
        elif c in '"\'':
            quote = c
        elif c == '#' and (i == 0 or line[i - 1] in ' \t'):
            return i
        i += 1
    return -1


def tokens(path, text):
    """A C or C++ text's tokens, comments stripped, by GCC's preprocessor."""
    lang = 'c' if path.endswith(('.c', '.h')) else 'c++'
    r = subprocess.run(['gcc', '-x', lang, '-fpreprocessed', '-dD', '-E', '-P',
                        '-w', '-'], input=text.encode('utf-8', 'surrogateescape'),
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return r.returncode, r.stdout


TREE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('paths', nargs='*')
    parser.add_argument('--tree')
    parser.add_argument('--write', action='store_true')
    parser.add_argument('--no-tokens', action='store_true')
    parser.add_argument('--list', action='store_true')
    options = parser.parse_args()
    write = options.write
    report = options.list
    tree = os.path.abspath(options.tree) if options.tree else TREE
    roots = options.paths or ['.']
    changed = []
    lines = 0
    left = 0
    refused = []
    walks = []
    for root in roots:
        path = os.path.abspath(os.path.join(tree, root))
        if os.path.relpath(path, tree).startswith('..'):
            print('check_hyphens: %s is outside the tree' % root)
            return 2
        if os.path.isfile(path):
            walks.append([(os.path.dirname(path), [], [os.path.basename(path)])])
        elif os.path.isdir(path):
            walks.append(os.walk(path))
        else:
            print('check_hyphens: no such file or folder: %s' % root)
            return 2
    for walk in walks:
        for folder, dirs, names in walk:
            rel_folder = os.path.relpath(folder, tree).replace(os.sep, '/')
            dirs[:] = sorted(d for d in dirs if d not in ('.git', '__pycache__'))
            for name in sorted(names):
                rel = os.path.relpath(os.path.join(folder, name), tree).replace(os.sep, '/')
                if rel in SKIP_FILES or any(p in '/' + rel for p in DATA_PARTS):
                    continue
                path = os.path.join(folder, name)
                if name in HASH_NAMES or name.endswith(HASH_EXT):
                    kind = 'hash'
                elif name.endswith(C_EXT):
                    kind = 'c'
                elif name.endswith(DOC_EXT) or (name.endswith('.txt') and
                                                rel.startswith(DOC_TXT_DIRS)):
                    kind = 'doc'
                elif name.endswith(HTML_EXT):
                    kind = 'html'
                else:
                    continue
                raw = open(path, 'rb').read()
                if b' --' not in raw:
                    continue
                try:
                    text = raw.decode('utf-8')
                except UnicodeDecodeError:
                    continue
                new = {'c': c_comments, 'doc': markdown, 'html': html_prose,
                       'hash': hash_comments}[kind](text)
                if write and kind == 'c' and new != text and \
                        not options.no_tokens:
                    a = tokens(path, text)
                    b = tokens(path, new)
                    if a[0] != b[0] or a[1].split() != b[1].split() or not a[1]:
                        refused.append(rel)
                        continue
                if new != text:
                    count = dashes(text) - dashes(new)
                    lines += count
                    changed.append((rel, count))
                    if write:
                        open(path, 'wb').write(new.encode('utf-8'))
                left += dashes(new)
    if write:
        print('check_hyphens: %d turned in %d files; %d files refused by the '
              'token check' % (lines, len(changed), len(refused)))
    else:
        for rel, count in changed:
            print('%s: %d double hyphens in its prose' % (rel, count))
        print('check_hyphens: %d double hyphens in the prose of %d files' % (
            lines, len(changed)))
    for rel in refused:
        print('  refused, its tokens would change: ' + rel)
    if report and write:
        for rel, count in changed:
            print('  %5d %s' % (count, rel))
    return 1 if (changed and not write) or refused else 0


if __name__ == '__main__':
    sys.exit(main())
