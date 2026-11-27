#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                              check_lane_names.py
#
# No lane named in the tree, and no decision cited by a lane's code (ruling
# 13, B): a decision is cited by its module and number ("the event module's
# decision 15"). The merge window runs it as a gate over the lines a window
# adds and over each commit message; a lane runs it over its own zip before
# it sends it. It reads only what a change adds, so what the tree already
# holds never fails it, and a line a change only moves (one it removes from a
# file it deletes or edits) is not added; --all reports the whole tree.
#   Nor is a name a change only carries into new words around it: a document
# made HTML from its Markdown, or a paragraph wrapped again, has new lines and
# the old text. A finding in an added line is carried when the words a
# reader sees beside it, three on one side, stand with it in what the change
# removes (a file it deletes, whole, or the lines it takes out of a file it
# edits). The words are the letters and digits a reader sees: an HTML
# file's tags, a Markdown link's target and an entity are not read.
#   It refuses, in a line it reads:
#     a lane named: "lane F", "lanes B and E", "Lane-C", "LaneF", "the wasm
#       lane", "lane math"; and a message's subject that starts with a
#       lane's letter, as "H: ..." ("math:" and "wasm:" name modules). The
#       word "lane" alone passes (SIMD has lanes).
#     a lane-coded id in a citation. An id is A to H, J, O, W, X or WM, an
#       optional hyphen, 1 to 3 digits and an optional letter (C82, E15,
#       F-7, O46, W17, WM1). It is refused after a citing word (decision,
#       ruling, ruled, item, ask, per, see, under), in parentheses alone or
#       in a list ("(C66)", "(E81, E82)"), with a possessive ("W7's floor"),
#       and as an alias marker's tag ("// O2: old name"). The C standards
#       (C89 to C23, C2x, C2y) are never ids, nor are Arm's instruction sets
#       (A32, A64) on a line that names Arm; F1 to F24 are keys in the last
#       two forms; and parentheses straight after a name hold an argument,
#       not a citation ("K(O1)").
#
#   usage:   python3 scripts/ci/check_lane_names.py [options]
#   options: --base REV        the lines the work tree adds to REV
#            --zip ZIP         the lines a lane zip's files add to the tree's
#                              copies at HEAD (a new file is read whole), and
#                              its COMMIT_MESSAGE.txt if it carries one;
#                              repeat for more
#            --deleted LIST    the paths a zip deletes, one a line, whose
#                              lines a file it moves may carry; repeat
#            --message FILE    a commit message, every line; repeat
#            --all             every line of the tree (a report)
#            --tree DIR        the tree (default: the one this script is in)
#   exit:    0 when nothing is found; 1 when something is; 2 when the run
#            could not start
#
#
# path:      /scripts/ci/check_lane_names.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
#                                                            revised: 2026.11.26
#*******************************************************************************
import argparse
import difflib
import os
import re
import subprocess
import sys
import zipfile

LANE = re.compile(
    r"(?<![A-Za-z])[Ll]anes?[ _-]?[A-H](?![A-Za-z])"
    r"|\b(?:[Mm]ath|[Ww]asm) lanes?\b"
    r"|(?<![A-Za-z])[Ll]anes? (?:math|wasm)\b")
ID = r"(?:WM|[A-HJOWX])-?\d{1,3}[a-z]?"
CITE = re.compile(
    r"(?i:\b(?:decisions?|rulings?|ruled|item|ask|per|see|under))\s+"
    r"(" + ID + r")\b")
SEP = r"(?:\s*[,/;&]\s*(?:and\s+|or\s+)?|\s+(?:and|or)\s+)"
PAREN = re.compile(r"(?<![A-Za-z0-9_])\(\s*(" + ID + r"(?:" + SEP + ID +
                   r")*)\s*\)")
POSS = re.compile(r"\b(" + ID + r")['\u2019]s\b")
MARK = re.compile(r"(?://|/\*|#)\s*(" + ID + r"):")
STANDARD = re.compile(r"^C(?:89|90|95|99|11|17|18|23|2x|2y)$")
ARM_SET = re.compile(r"^A(?:32|64)$")
ARM_LINE = re.compile(r"\b(?:ARM|Arm|AArch32|AArch64)\b")
KEY = re.compile(r"^F(?:[1-9]|1[0-9]|2[0-4])$")
TEXT = ('.c', '.h', '.cpp', '.hpp', '.cc', '.hh', '.inl', '.ipp', '.txt',
        '.md', '.html', '.htm', '.cmake', '.py', '.sh', '.ps1', '.awk',
        '.dawk', '.js', '.mjs', '.json', '.yml', '.yaml', '.in', '.mk',
        '.rst', '.xml', '.css', '.cfg', '.ini', '.toml', '.1', '.3', '.7')
NAMES = ('CMakeLists.txt', 'Makefile', 'README', '.gitattributes',
         '.gitignore', 'LICENSE')
SKIP = ('scripts/ci/check_lane_names.py',)
SUBJECT = re.compile(r"^\s*[A-H]\s*:")
TAG = re.compile(r"<!--.*?-->|</?[A-Za-z][A-Za-z0-9-]*(?:\s[^<>]*)?/?>", re.S)
LINK = re.compile(r"\]\([^()\s]*\)")
ENTITY = re.compile(r"&(?:#[0-9]+|#[xX][0-9A-Fa-f]+|[A-Za-z][A-Za-z0-9]*);")
WORD = re.compile(r"[A-Za-z0-9]+")
# the words beside a finding, on one side, that must stand with it in what a
# change removes for it to be carried
CONTEXT = 3


def never_id(text, line):
    return bool(STANDARD.match(text) or
                (ARM_SET.match(text) and ARM_LINE.search(line)))


def ids_in(text, line=''):
    return [i for i in re.split(SEP, text)
            if i and not never_id(i, line)]


def findings(line):
    found = [m.group(0) for m in LANE.finditer(line)]
    for pattern in (CITE, POSS, MARK):
        found += [m.group(0) for m in pattern.finditer(line)
                  if not never_id(m.group(1), line) and
                  not (pattern is not CITE and KEY.match(m.group(1)))]
    for m in PAREN.finditer(line):
        if ids_in(m.group(1), line):
            found.append(m.group(0))
    return found


def is_text(path):
    base = os.path.basename(path)
    return base.endswith(TEXT) or base in NAMES


def lines_of(data):
    return data.decode('utf-8', 'replace').replace('\r\n', '\n').split('\n')


def seen(path, text):
    """text with what a reader does not see - an HTML file's tags, a Markdown
    link's target, an entity - made spaces; its newlines stay, so a word
    keeps its line"""
    def blank(match):
        return re.sub(r"[^\n]", " ", match.group(0))
    if path.endswith(('.html', '.htm')):
        text = TAG.sub(blank, text)
    if path.endswith('.md'):
        text = LINK.sub(blank, text)
    return ENTITY.sub(blank, text)


def words(path, lines):
    """the (line number, word) of each word lines show a reader"""
    out = []
    for number, line in enumerate(seen(path, '\n'.join(lines)).split('\n'),
                                  1):
        out += [(number, word) for word in WORD.findall(line)]
    return out


def run_of(path, lines):
    """lines' words as one string, each word between spaces"""
    return ' ' + ' '.join(w for _, w in words(path, lines)) + ' '


def added(old, new, removed=None, corpus=None, path=''):
    """The (number, text) of each line in new that old lacks; the text of
    each line old has and new lacks goes into removed, a set, and the words
    of each run of them into corpus, a list."""
    out = []
    matcher = difflib.SequenceMatcher(None, old, new, autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag in ('replace', 'insert'):
            out += [(j + 1, new[j]) for j in range(j1, j2)]
        if removed is not None and tag in ('replace', 'delete'):
            removed.update(old[i].strip() for i in range(i1, i2))
        if corpus is not None and tag in ('replace', 'delete'):
            corpus.append(run_of(path, old[i1:i2]))
    return out


def carried(stream, number, found, cursor, corpus):
    """whether found, on line number of the file whose words are stream,
    stands with the words beside it in corpus; cursor holds, per line, the
    index of the word after the last finding placed on it"""
    mine = [i for i, (n, _) in enumerate(stream) if n == number]
    target = WORD.findall(found)
    if not mine or not target:
        return False
    start = cursor.get(number, mine[0])
    for i in range(start, mine[-1] + 1):
        if [w for _, w in stream[i:i + len(target)]] == target:
            cursor[number] = i + len(target)
            break
    else:
        return False
    plain = [w for _, w in stream]
    left = plain[max(0, i - CONTEXT):i]
    right = plain[i + len(target):i + len(target) + CONTEXT]
    runs = []
    if len(left) == CONTEXT:
        runs.append(left + target)
    if len(right) == CONTEXT:
        runs.append(target + right)
    if not runs:
        runs.append(left + target + right)
    return any(' ' + ' '.join(run) + ' ' in text
               for run in runs for text in corpus)


def git(tree, *arguments):
    return subprocess.run(['git', '-C', tree] + list(arguments),
                          capture_output=True, check=True).stdout


def head_copy(tree, prefix, path):
    try:
        return lines_of(git(tree, 'show', 'HEAD:' + prefix + path))
    except subprocess.CalledProcessError:
        return []


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--base')
    parser.add_argument('--zip', action='append', default=[])
    parser.add_argument('--message', action='append', default=[])
    parser.add_argument('--deleted', action='append', default=[])
    parser.add_argument('--all', action='store_true')
    parser.add_argument('--tree', default=os.path.dirname(os.path.dirname(
        os.path.dirname(os.path.abspath(__file__)))))
    options = parser.parse_args()
    if not (options.base or options.zip or options.message or options.all):
        parser.error('nothing to read: give --base, --zip, --message or --all')

    tree = options.tree
    try:
        prefix = git(tree, 'rev-parse', '--show-prefix').decode().strip()
    except (subprocess.CalledProcessError, OSError):
        prefix = None
    if prefix is None and (options.base or options.zip or options.all):
        print('check_lane_names: %s is not in a git tree' % tree,
              file=sys.stderr)
        return 2

    hits = []
    pending = []    # files' added lines, read once every removed line is known
    removed = set()
    corpus = []     # the words of what the change removes, a run a string

    def read(where, numbered, moved=False, path='', lines=None):
        if moved:
            pending.append((where, list(numbered), path, lines))
            return
        cursor = {}
        stream = None
        for number, text in numbered:
            for found in findings(text):
                if lines is not None:
                    if stream is None:
                        stream = words(path, lines)
                    if carried(stream, number, found, cursor, corpus):
                        continue
                hits.append((where, number, found, text.strip()))

    for name in options.message:
        with open(name, 'rb') as handle:
            lines = lines_of(handle.read())
        read(name, enumerate(lines, 1))
        if lines and SUBJECT.match(lines[0]):
            hits.append((name, 1, lines[0].split(':')[0] + ':',
                         lines[0].strip()))

    for name in options.deleted:
        with open(name, encoding='utf-8') as handle:
            for path in handle.read().split('\n'):
                path = path.strip().replace('\\', '/')
                if path.startswith('djinterp/'):
                    path = path[len('djinterp/'):]
                if path:
                    old = head_copy(tree, prefix, path)
                    removed.update(l.strip() for l in old)
                    corpus.append(run_of(path, old))

    for name in options.zip:
        with zipfile.ZipFile(name) as archive:
            for info in archive.infolist():
                path = info.filename.replace('\\', '/')
                if info.is_dir() or path in SKIP:
                    continue
                if path in ('COMMIT_MESSAGE.txt', 'commit_message.txt'):
                    read(name + ':' + path,
                         enumerate(lines_of(archive.read(info)), 1))
                    continue
                if path.startswith('djinterp/'):
                    path = path[len('djinterp/'):]
                if not is_text(path):
                    continue
                lines = lines_of(archive.read(info))
                read(name + ':' + path,
                     added(head_copy(tree, prefix, path), lines, removed,
                           corpus, path), True, path, lines)

    if options.base:
        # without rename detection, a moved file is a deletion and an
        # addition, so the lines it carries are in removed
        changed = git(tree, 'diff', '--no-renames', '--name-only',
                      '--diff-filter=AM', options.base, '--',
                      '.').decode().split()
        for full in git(tree, 'diff', '--no-renames', '--name-only',
                        '--diff-filter=D', options.base, '--',
                        '.').decode().split():
            try:
                old = lines_of(git(tree, 'show', options.base + ':' + full))
            except subprocess.CalledProcessError:
                continue
            removed.update(l.strip() for l in old)
            corpus.append(run_of(full, old))
        new = git(tree, 'ls-files', '--others', '--exclude-standard',
                  '--', '.').decode().split()
        for full in sorted(set(changed + [prefix + n for n in new])):
            path = full[len(prefix):] if full.startswith(prefix) else full
            if not is_text(path) or path in SKIP:
                continue
            disk = os.path.join(tree, path)
            if not os.path.isfile(disk):
                continue
            try:
                old = lines_of(git(tree, 'show', options.base + ':' + full))
            except subprocess.CalledProcessError:
                old = []
            with open(disk, 'rb') as handle:
                lines = lines_of(handle.read())
            read(path, added(old, lines, removed, corpus, path), True, path,
                 lines)

    for where, numbered, path, lines in pending:
        read(where, [(n, t) for n, t in numbered if t.strip() not in removed],
             False, path, lines)

    if options.all:
        for path in git(tree, 'ls-files', '--', '.').decode().split('\n'):
            if path and is_text(path) and path not in SKIP:
                with open(os.path.join(tree, path), 'rb') as handle:
                    read(path, enumerate(lines_of(handle.read()), 1))

    for where, number, found, text in hits:
        print('%s:%d: %s    | %s' % (where, number, found, text[:100]))
    files = len(set(h[0] for h in hits))
    print('check_lane_names: %d finding%s in %d file%s' % (
        len(hits), '' if len(hits) == 1 else 's', files,
        '' if files == 1 else 's'))
    return 1 if hits else 0


if __name__ == '__main__':
    sys.exit(main())
