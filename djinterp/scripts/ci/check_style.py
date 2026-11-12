#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                                   check_style.py
#
# The style guides' measurable rules, checked over the tree's C and C++
# files: the ones a machine can decide, so that no review has to.
#   Errors, which fail a run:
#     T  a tab character, anywhere
#     S  whitespace at the end of a line
#     U  not valid UTF-8, or a byte-order mark
#     X  a character two columns wide (East Asian Wide or Fullwidth)
#     B  the banner: its rules, its title line (the file's own name on
#        character 80), its fields at character 14, its path (the file's own),
#        its dates (YYYY.MM.DD, ending on character 80), the two blank lines
#        before path:; its tag is not checked while ruling #33 leaves the tags
#        as they are
#     G  a header's include guard: missing, or not the one its path derives
#        (djinterp's headers: DJINTERP_, then C_ for the C tree and nothing for
#        the C++ tree, CONFIG_ and C_ or CPP_ for config/, the folders without
#        modules/, the stem, _H or _HPP; the roots DJINTERP and DJINTERP_CPP;
#        re_std's: RE_STD_, C_ for its C tree, its path with the extension;
#        then, by ruling #71, each repeated run of words merged, as
#        DJINTERP_C_TEST_TEST_COMMON_H to DJINTERP_C_TEST_COMMON_H, unless
#        that is another header's guard, merged or not, when all are kept);
#        its #define carrying 1, and its #endif naming it
#     R  an include of djinterp.h or djinterp.hpp whose comment is not
#        "// framework root" (a source's own header says
#        "// corresponding header")
#   Warnings, which are counted and shown but do not fail a run:
#     W  a line over 80 characters (the limit is a target: a long path or
#        string may pass it)
#     C  an #include without a trailing comment
#     F  a function body over 100 lines (ruling cli a), unless the body is
#        constant data (ruling E8): its first line is the comment
#        "// constant data: exempt from the 100-line limit", and each of its
#        statements is a static or const declaration or a return
#     M  an include guard with every segment kept where ruling #71 merges
#        them: still accepted, while the tree's headers move
#   Generated files (a banner or first lines that say so) are left out, as
# the guides exempt them.
#   Most files met the rules when the guides were written, and some do not
# yet. --known holds a run to a punch list of counts, one per file and error
# code: an error count above its line fails the run, and a count below it is
# reported so that the line can come down. --write-known records a run's
# error counts as the list.
#
#   usage:   python3 scripts/ci/check_style.py [options] [path ...]
#   options: --changed REV        only the files changed since REV (and the
#                                 new ones git does not track yet)
#            --known FILE         the punch list a run is held to
#            --write-known FILE   write this run's counts as the list
#            --show N             lines shown per file and code (default 3)
#            --warnings           list the files with warnings too, not only
#                                 the totals
#            --tree DIR           the tree to check (default: the one this
#                                 script is in)
#   paths:   files or folders under the tree's root; default inc/, src/ and
#            tests/
#   exit:    0 when no error count rose (or, without --known, there is no
#            error); 1 otherwise; 2 when the run could not start
#
#
# path:      /scripts/ci/check_style.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
#                                                            revised: 2026.11.11
#*******************************************************************************
import argparse
import collections
import os
import re
import subprocess
import sys
import unicodedata

TREE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))))
EXTENSIONS = ('.h', '.c', '.hpp', '.cpp')
ERRORS = 'TSUXBGR'
WARNINGS = 'WCFM'
GENERATED = re.compile(r'(auto-?generated|generated (by|from|with)\b|'
                       r'do not (edit|modify))', re.I)
RULE_OPEN = '/' + '*' * 79
RULE_CLOSE = '*' * 79 + '/'
TITLE = re.compile(r'^\* djinterp \[[^\]]*\] +(\S+)$')
DATE = re.compile(r'^\d{4}\.\d{2}\.\d{2}$')
INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"](.*)$')
GUARD = re.compile(r'^\s*#\s*ifndef\s+(\w+)\s*$')
OPENER = re.compile(r'^\s*(namespace\b|extern\s+"C"|class\b|struct\b|'
                    r'union\b|enum\b)')


def rel(path):
    """A path as the tree names it: relative to its root, with slashes."""
    return os.path.relpath(path, TREE).replace(os.sep, '/')


def path_guard(path):
    """The include guard a header's path derives with every segment kept, or
    None where the path is outside the trees the guides cover."""
    parts = path.split('/')
    stem, extension = os.path.splitext(parts[-1])

    if parts[:2] == ['inc', 're_std']:
        folders = parts[2:-1]
        out = ['RE_STD']
        if folders and folders[0] in ('c', 'cpp'):
            if folders[0] == 'c':
                out.append('C')
            folders = folders[1:]
        guard = '_'.join(out + folders + [parts[-1]])
        return re.sub(r'[^A-Za-z0-9]+', '_', guard).upper()
    if parts[:2] != ['inc', 'djinterp']:
        return None
    if parts[2:] == ['c', 'djinterp.h']:
        return 'DJINTERP'
    if parts[2:] == ['cpp', 'djinterp.hpp']:
        return 'DJINTERP_CPP'

    folders = parts[2:-1]
    out = ['DJINTERP']
    if folders and folders[0] == 'config':
        out.append('CONFIG')
        folders = folders[1:]
    if folders and folders[0] in ('c', 'cpp'):
        if folders[0] == 'c' or out[-1] == 'CONFIG':
            out.append(folders[0].upper())
        folders = folders[1:]
        # the structural folder right under a language tree adds nothing
        if folders and folders[0] in ('modules', 'core'):
            folders = folders[1:]
    suffix = extension[1:].upper()
    guard = '_'.join(out + folders + [stem, suffix])
    return re.sub(r'[^A-Za-z0-9]+', '_', guard).upper()


def merge_repeats(guard):
    """A guard with each run of words that repeats the run before it merged
    into one (ruling #71): DJINTERP_C_TEST_TEST_COMMON_H is
    DJINTERP_C_TEST_COMMON_H, and a run of two or three words merges the same
    way."""
    words = guard.split('_')
    merged = True
    while merged:
        merged = False
        for size in (3, 2, 1):
            for start in range(len(words) - 2 * size + 1):
                if (words[start:start + size] ==
                        words[start + size:start + 2 * size]):
                    del words[start + size:start + 2 * size]
                    merged = True
                    break
            if merged:
                break
    return '_'.join(words)


GUARDS = None


def guard_index():
    """Every header's guard in the tree, merged, with the headers that would
    take it, and every guard with its segments kept: what decides whether a
    merge collides. Built once, from inc/."""
    global GUARDS
    if GUARDS is None:
        merged = collections.defaultdict(set)
        kept = set()
        for directory, _, entries in os.walk(os.path.join(TREE, 'inc')):
            for entry in entries:
                if not entry.endswith(('.h', '.hpp')):
                    continue
                path = rel(os.path.join(directory, entry))
                full = path_guard(path)
                if full:
                    merged[merge_repeats(full)].add(path)
                    kept.add(full)
        GUARDS = (merged, kept)
    return GUARDS


def derive_guard(path):
    """The include guard a header's path derives (ruling #71): path_guard's,
    its repeated segments merged, unless the merged guard is another header's,
    merged or not; then every segment is kept. None where path_guard has
    none."""
    full = path_guard(path)
    if full is None:
        return None
    merged = merge_repeats(full)
    if merged == full:
        return full
    takers, kept = guard_index()
    if (takers.get(merged, set()) - {path}) or merged in kept:
        return full
    return merged


def check_banner(path, lines, report):
    """The banner's fixed template."""
    if not lines or lines[0] != RULE_OPEN:
        report('B', 1, 'the banner does not open with / and 79 *')
        return
    try:
        close = lines.index(RULE_CLOSE)
    except ValueError:
        report('B', 1, 'the banner does not close with 79 * and /')
        return

    banner = lines[:close + 1]
    title = TITLE.match(banner[1]) if len(banner) > 1 else None
    if not title:
        report('B', 2, 'the title line is not "* djinterp [tag] <file>"')
    elif title.group(1) != os.path.basename(path):
        report('B', 2, 'the title names %s' % title.group(1))
    elif len(banner[1]) != 80:
        report('B', 2, 'the file name ends on character %d, not 80' %
               len(banner[1]))

    fields = {}
    for number, line in enumerate(banner, 1):
        for name in ('path:', 'link(s):', 'author(s):'):
            if line.startswith('* ' + name):
                fields[name] = (number, line)
    for name in ('path:', 'link(s):', 'author(s):'):
        if name not in fields:
            report('B', 1, 'no %s field' % name)
            continue
        number, line = fields[name]
        if len(line) < 14 or line[13] == ' ' or line[12] != ' ':
            report('B', number, '%s value does not begin on character 14' %
                   name)
    if 'path:' in fields:
        number, line = fields['path:']
        if line[13:].strip() != '/' + path:
            report('B', number, 'the path is %s, not /%s' % (
                line[13:].strip(), path))
        if (number < 4 or banner[number - 2] != '*' or
                banner[number - 3] != '*'):
            report('B', number, 'two blank * lines do not precede path:')
    if 'author(s):' in fields:
        number, line = fields['author(s):']
        created = re.search(r'created: (\S+)$', line)
        if not created or len(line) != 80:
            report('B', number, 'created: does not end on character 80')
        elif not DATE.match(created.group(1)):
            report('B', number, 'created: %s is not YYYY.MM.DD' %
                   created.group(1))
        revised = banner[number] if number < len(banner) else ''
        match = re.match(r'^\* +revised: (\S+)$', revised)
        if not match or len(revised) != 80:
            report('B', number + 1, 'revised: does not end on character 80 '
                   'beneath created:')
        elif not DATE.match(match.group(1)):
            report('B', number + 1, 'revised: %s is not YYYY.MM.DD' %
                   match.group(1))


def check_guard(path, lines, report):
    """A header's guard: present, derived, defined to 1, closed by name."""
    want = derive_guard(path)
    if want is None:
        return
    found = None
    for number, line in enumerate(lines, 1):
        match = GUARD.match(line)
        if match:
            found = (number, match.group(1))
            break
        if line.startswith('#') and not line.startswith('#ifndef'):
            break
    if not found:
        report('G', 1, 'no include guard (want %s)' % want)
        return
    number, guard = found
    full = path_guard(path)
    if guard == full and full != want:
        report('M', number, 'the guard is %s; ruling #71 merges it to %s' % (
            guard, want))
    elif guard != want:
        report('G', number, 'the guard is %s; its path derives %s' % (
            guard, want))
    following = lines[number] if number < len(lines) else ''
    if not re.match(r'^\s*#\s*define\s+%s\s+1\s*$' % re.escape(guard),
                    following):
        report('G', number + 1, 'the guard is not defined to 1 on the next '
               'line')
    closing = [n for n, line in enumerate(lines, 1) if line.strip()]
    last = lines[closing[-1] - 1] if closing else ''
    if not re.match(r'^\s*#\s*endif\s+//\s*%s\s*$' % re.escape(guard), last):
        report('G', closing[-1] if closing else 1,
               'the last line is not "#endif  // %s"' % guard)


def check_includes(path, lines, report):
    """Include comments, and the framework root's own summary."""
    stem = os.path.splitext(os.path.basename(path))[0]
    for number, line in enumerate(lines, 1):
        match = INCLUDE.match(line)
        if not match:
            continue
        target, rest = match.group(2), match.group(3)
        comment = rest.strip()
        if not comment.startswith('//'):
            report('C', number, 'no comment on #include %s' % target)
            continue
        name = os.path.basename(target)
        if name in ('djinterp.h', 'djinterp.hpp'):
            own = (os.path.splitext(name)[0] == stem and
                   path.endswith(('.c', '.cpp')))
            want = '// corresponding header' if own else '// framework root'
            if comment != want:
                report('R', number, 'the include of %s says "%s", not "%s"' % (
                    name, comment, want))


def is_test_data(path):
    """Whether a path is in a data folder under tests/: a test's input, such
    as a lexer's corpus, keeps its language's extension but is not code."""
    return path.startswith('tests/') and '/data/' in path


CONSTANT_DATA = '// constant data: exempt from the 100-line limit'


def is_constant_data(body):
    """Whether a function body holds only constant data: each statement at
    its top level is a static or const declaration or a return.
    Preprocessor lines, comments, strings and characters are left out."""
    text = '\n'.join(line for line in body
                     if not line.lstrip().startswith('#'))
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    text = re.sub(r'//[^\n]*', ' ', text)
    text = re.sub(r'"(\\.|[^"\\\n])*"', '""', text)
    text = re.sub(r"'(\\.|[^'\\\n])*'", "''", text)
    statements, depth, current = [], 0, []
    for character in text:
        if character in '{(':
            depth += 1
        elif character in '})':
            depth -= 1
        if character == ';' and depth == 0:
            statements.append(''.join(current).strip())
            current = []
        else:
            current.append(character)
    if ''.join(current).strip():
        return False
    return bool(statements) and all(
        re.match(r'(static|const|return)\b', statement)
        for statement in statements)


def check_functions(lines, report):
    """Function bodies over 100 lines: a { alone at the start of a line to a
    } alone at the start of one, where the { does not open a namespace, a
    type or an extern block."""
    start = None
    for number, line in enumerate(lines, 1):
        if line == '{' and start is None:
            before = ''
            for back in range(number - 2, -1, -1):
                if lines[back].strip():
                    before = lines[back]
                    break
            if not OPENER.match(before):
                start = number
        elif start is not None and re.match(r'^\}\s*(//.*)?$', line):
            length = number - start - 1
            if length > 100:
                body = lines[start:number - 1]
                marked = [entry.strip() for entry in body if entry.strip()]
                if marked and marked[0] == CONSTANT_DATA:
                    if not is_constant_data(body):
                        report('F', start, 'a function body of %d lines, '
                               'marked as constant data but with other '
                               'statements' % length)
                else:
                    report('F', start, 'a function body of %d lines' % length)
            start = None
        elif start is not None and line.startswith('};'):
            start = None


def check_file(path):
    """Every check over one file: {code: [(line, message), ...]}."""
    found = collections.defaultdict(list)

    def report(code, number, message):
        found[code].append((number, message))

    with open(os.path.join(TREE, path), 'rb') as handle:
        data = handle.read()
    if data.startswith(b'\xef\xbb\xbf'):
        report('U', 1, 'a byte-order mark')
        data = data[3:]
    try:
        text = data.decode('utf-8')
    except UnicodeDecodeError as error:
        report('U', data[:error.start].count(b'\n') + 1, 'not valid UTF-8')
        text = data.decode('utf-8', 'replace')
    lines = text.replace('\r\n', '\n').split('\n')
    if lines and lines[-1] == '':
        lines.pop()
    head = '\n'.join(lines[:14])
    if GENERATED.search(head):
        return None

    for number, line in enumerate(lines, 1):
        if '\t' in line:
            report('T', number, 'a tab')
        if line != line.rstrip():
            report('S', number, 'trailing whitespace')
        if len(line) > 80:
            report('W', number, '%d characters' % len(line))
        for character in line:
            if (ord(character) > 0x7f and
                    unicodedata.east_asian_width(character) in ('W', 'F')):
                report('X', number, 'U+%04X is two columns wide' %
                       ord(character))
                break
    check_banner(path, lines, report)
    if path.endswith(('.h', '.hpp')):
        check_guard(path, lines, report)
    check_includes(path, lines, report)
    check_functions(lines, report)
    return found


def collect(paths, changed):
    """The files to check."""
    if changed:
        result = subprocess.run(['git', 'diff', '--name-only', '--relative',
                                 changed, '--', '.'], cwd=TREE,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                universal_newlines=True)
        if result.returncode != 0:
            sys.exit('check_style.py: git diff %s: %s' % (
                changed, result.stderr.strip()))
        names = result.stdout.splitlines()
        result = subprocess.run(['git', 'ls-files', '--others',
                                 '--exclude-standard', '--', '.'], cwd=TREE,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                universal_newlines=True)
        names += result.stdout.splitlines()
        return sorted(set(name for name in names
                          if name.endswith(EXTENSIONS) and
                          not is_test_data(name) and
                          os.path.isfile(os.path.join(TREE, name))))
    files = []
    for item in paths or ['inc', 'src', 'tests']:
        full = os.path.join(TREE, item)
        if os.path.isfile(full):
            if full.endswith(EXTENSIONS):
                files.append(rel(full))
            continue
        for directory, names, entries in os.walk(full):
            names.sort()
            if is_test_data(rel(directory) + '/'):
                continue
            for entry in sorted(entries):
                if entry.endswith(EXTENSIONS):
                    files.append(rel(os.path.join(directory, entry)))
    return files


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('paths', nargs='*')
    parser.add_argument('--changed')
    parser.add_argument('--known')
    parser.add_argument('--write-known')
    parser.add_argument('--show', type=int, default=3)
    parser.add_argument('--warnings', action='store_true')
    parser.add_argument('--tree')
    options = parser.parse_args()

    global TREE
    if options.tree:
        TREE = os.path.abspath(options.tree)

    files = collect(options.paths, options.changed)
    counts = {}
    details = {}
    generated = 0
    for path in files:
        found = check_file(path)
        if found is None:
            generated += 1
            continue
        for code, items in found.items():
            counts[(path, code)] = len(items)
            details[(path, code)] = items

    known = {}
    if options.known:
        with open(options.known, encoding='utf-8') as handle:
            for line in handle:
                fields = line.rstrip('\r\n').split('\t')
                if len(fields) == 3 and not fields[0].startswith('#'):
                    known[(fields[0], fields[1])] = int(fields[2])

    totals = collections.Counter()
    for (path, code), count in counts.items():
        totals[code] += count
    print('djinterp style: %d files (%d generated, left out)' % (
        len(files), generated))
    print('  ' + '   '.join('%s %d' % (code, totals[code])
                            for code in ERRORS + WARNINGS))

    failed = 0
    checked = set(files)
    for (path, code), count in sorted(counts.items()):
        allowed = known.get((path, code), 0) if options.known else 0
        if count <= allowed:
            continue
        if code in ERRORS:
            failed += 1
            tag = 'FAIL'
        elif options.warnings:
            tag = 'warn'
        else:
            continue
        print('%s %s %s: %d%s' % (tag, path, code, count,
                                  (' (the punch list has %d)' % allowed)
                                  if allowed else ''))
        for number, message in details[(path, code)][:options.show]:
            print('       %d: %s' % (number, message))
    if options.known:
        for (path, code), allowed in sorted(known.items()):
            if path in checked and counts.get((path, code), 0) < allowed:
                print('FIXED %s %s: %d, the punch list has %d; lower it' % (
                    path, code, counts.get((path, code), 0), allowed))
    print('check_style: %d files with an error count%s' % (
        failed, ' above the punch list' if options.known else ''))

    if options.write_known:
        with open(options.write_known, 'w', encoding='utf-8',
                  newline='\n') as handle:
            handle.write('# djinterp style punch list: file<TAB>code<TAB>'
                         'count, written by scripts/ci/check_style.py '
                         '--write-known\n')
            # errors only: a warning fails nothing, so it needs no line
            for (path, code), count in sorted(counts.items()):
                if code in ERRORS:
                    handle.write('%s\t%s\t%d\n' % (path, code, count))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
