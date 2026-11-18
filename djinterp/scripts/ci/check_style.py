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
#        before path:; its tag, the one its path derives (ruled 2026.11.17):
#        [<language>][<module>], the language the c or cpp folder in the path
#        (config/c/ counts), else the extension, the module the first folder
#        under it without modules/ or core/, util/ as utility; env, tools/
#        (every tool, and inc/ui) and config/'s own files their own module;
#        a file directly in a language folder its language alone, and re_std
#        [re_std]; tests and runners as their subject's path. Until the tree's
#        banners are converted (banner_tags.py), a one-bracket tag is still
#        accepted
#     G  a header's include guard: missing, or not the one its path derives
#        (djinterp's headers: DJINTERP_, then C_ for the C tree and nothing for
#        the C++ tree, CONFIG_ and C_ or CPP_ for config/, the folders without
#        modules/, the stem, _H or _HPP; the roots DJINTERP and DJINTERP_CPP;
#        re_std's: RE_STD_, C_ for its C tree, its path with the extension;
#        inc/ui's: DJINTERP_UI_ and its path (ruled 2026.11.17);
#        then, by ruling #71, each repeated run of words merged, as
#        DJINTERP_C_TEST_TEST_COMMON_H to DJINTERP_C_TEST_COMMON_H, unless
#        that is another header's guard, merged or not, when all are kept);
#        its #define carrying 1, and its #endif naming it
#     R  an include of djinterp.h or djinterp.hpp whose comment is not
#        "// framework root" (a source's own header says
#        "// corresponding header")
#     V  a header's (under inc/) variadic macro defined outside env's pair,
#        D_ENV_PP_VARIADIC_QUIET_BEGIN to D_ENV_PP_VARIADIC_QUIET_END, so
#        that a pedantic C++98 or C90 build warns about it (ruled
#        2026.11.17: every header that defines one quiets its own)
#   Warnings, which are counted and shown but do not fail a run:
#     W  a line over 80 characters (the limit is a target: a long path or
#        string may pass it)
#     C  an #include without a trailing comment
#     F  a function body over 100 lines (ruling cli a), unless the body is
#        constant data (ruling E8): its first line is the comment
#        "// constant data: exempt from the 100-line limit", and each of its
#        statements is a static or const declaration or a return; a test
#        (a file under tests/, or a runner under build/cmake/config/testing/)
#        is exempt (ruled 2026.11.11)
#     M  an include guard with every segment kept where ruling #71 merges
#        them: still accepted, while the tree's headers move
#     H  a header under inc/ whose banner lacks HOW TO USE, OPTIONS or
#        CONFIGURATION, each a heading over its own row of dashes, in that
#        order (ruled 2026.11.17: every header carries them). The punch
#        list tracks it: a count above its line is shown, and does not
#        fail a run
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
#            --show N             lines shown per file and code (default 3;
#                                 0 shows them all); a summary line counts
#                                 the ones left out
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
#                                                            revised: 2026.11.18
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
ERRORS = 'TSUXBGRV'
WARNINGS = 'WCFMH'
# warnings the punch list tracks: one above its line is shown, not failed
TRACKED = 'H'
SECTIONS = ('HOW TO USE', 'OPTIONS', 'CONFIGURATION')
GENERATED = re.compile(r'(auto-?generated|generated (by|from|with)\b|'
                       r'do not (edit|modify))', re.I)
RULE_OPEN = '/' + '*' * 79
RULE_CLOSE = '*' * 79 + '/'
TITLE = re.compile(r'^\* djinterp (\[[^\]]*\](?:\[[^\]]*\])?) +(\S+)$')
# a one-bracket tag that isn't the derived one is accepted until the tree's
# banners are converted by banner_tags.py (2026.11.20), then refused
ONE_BRACKET_ACCEPTED = True
DATE = re.compile(r'^\d{4}\.\d{2}\.\d{2}$')
INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"](.*)$')
GUARD = re.compile(r'^\s*#\s*ifndef\s+(\w+)\s*$')
MACRO = re.compile(r'^\s*#\s*define\s+(\w+)\(([^)]*)\)')
QUIET_BEGIN = 'D_ENV_PP_VARIADIC_QUIET_BEGIN'
QUIET_END = 'D_ENV_PP_VARIADIC_QUIET_END'
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
    if parts[:2] == ['inc', 'ui']:
        # the tools' interface tree (ruled 2026.11.17): DJINTERP_UI_, then
        # its path, as a tool's own headers under inc/djinterp/tools/ are
        guard = '_'.join(['DJINTERP'] + parts[1:-1] +
                         [stem, extension[1:]])
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


def derive_tag(path):
    """The banner tag a file's path derives (ruled 2026.11.17), or None where
    the path is outside the trees the rule covers."""
    parts = path.split('/')
    extension = os.path.splitext(parts[-1])[1]
    if extension not in EXTENSIONS:
        return None
    language = 'c' if extension in ('.h', '.c') else 'cpp'
    folders = parts[:-1]
    if folders[:4] == ['build', 'cmake', 'config', 'testing']:
        folders = folders[4:]
    elif folders[:1] in (['inc'], ['src'], ['tests']):
        folders = folders[1:]
    else:
        return None
    # tests outside djinterp/ that mirror a subject's path
    if folders[:1] in (['unintegrated'], ['bench']):
        folders = folders[1:]
    if not folders:
        return None
    top = folders[0]
    if top == 're_std':
        return '[re_std]'
    if top in ('ui', 'dawk'):
        return '[%s][tools]' % tag_language(folders[1:], language)
    if top == 'parse':
        return '[%s][parse]' % language
    if top == 'util':
        return '[%s][utility]' % language
    if top == 'djinterp':
        folders = folders[1:]
    elif top not in ('c', 'cpp'):
        return None
    if not folders:
        return '[%s]' % language
    if folders[0] == 'config':
        folders = folders[1:]
        if not folders or folders[0] not in ('c', 'cpp', 'env', 'tools'):
            return '[%s][config]' % language
    if folders[0] == 'tools':
        return '[%s][tools]' % tag_language(folders[1:], language)
    if folders[0] == 'env':
        return '[%s][env]' % language
    if folders[0] not in ('c', 'cpp'):
        return None
    language, rest = folders[0], folders[1:]
    while rest and rest[0] in ('modules', 'core'):
        rest = rest[1:]
    if not rest:
        return '[%s]' % language
    module = 'utility' if rest[0] == 'util' else rest[0]
    return '[%s][%s]' % (language, module)


def tag_language(folders, fallback):
    """The first c or cpp folder among folders, else fallback."""
    for folder in folders:
        if folder in ('c', 'cpp'):
            return folder
    return fallback


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
    elif title.group(2) != os.path.basename(path):
        report('B', 2, 'the title names %s' % title.group(2))
    elif len(banner[1]) != 80 and not (
            len(banner[1]) > 80 and
            banner[1] == '* djinterp %s %s' % (title.group(1),
                                               title.group(2))):
        # past 80 only where the tag and the name can't both fit, with one
        # space between them
        report('B', 2, 'the file name ends on character %d, not 80' %
               len(banner[1]))
    if title:
        tag, want = title.group(1), derive_tag(path)
        if (want is not None and tag != want and
                not (ONE_BRACKET_ACCEPTED and tag.count('[') == 1)):
            report('B', 2, 'the tag is %s; the path derives %s' % (tag, want))

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


def check_sections(lines, report):
    """A header's banner: HOW TO USE, OPTIONS and CONFIGURATION, each over
    its row of dashes, in that order."""
    if not lines or lines[0] != RULE_OPEN or RULE_CLOSE not in lines:
        return
    banner = lines[:lines.index(RULE_CLOSE)]
    found = []
    for number in range(len(banner) - 1):
        heading = banner[number][2:] if banner[number].startswith('* ') \
            else ''
        if (heading in SECTIONS and
                banner[number + 1] == '* ' + '-' * len(heading)):
            found.append(heading)
    if found != list(SECTIONS):
        missing = [name for name in SECTIONS if name not in found]
        report('H', 1, 'the banner lacks %s' % ', '.join(missing)
               if missing else 'the banner\'s sections are out of order')


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


def check_variadic(lines, report):
    """A header's variadic macros: each defined between env's pair."""
    quiet = False
    number = 0
    while number < len(lines):
        start = number + 1
        line = lines[number]
        while line.endswith('\\') and number + 1 < len(lines):
            number += 1
            line = line[:-1] + ' ' + lines[number]
        number += 1
        stripped = line.strip()
        if stripped == QUIET_BEGIN:
            quiet = True
        elif stripped == QUIET_END:
            quiet = False
        else:
            match = MACRO.match(line)
            if match and '...' in match.group(2) and not quiet:
                report('V', start, '%s is variadic, outside %s and %s' % (
                    match.group(1), QUIET_BEGIN, QUIET_END))


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


def is_test(path):
    """Whether a path is a test: a file under tests/, or a runner under
    build/cmake/config/testing/. A test's functions have no length limit
    (ruled 2026.11.11)."""
    return (path.startswith('tests/') or
            path.startswith('build/cmake/config/testing/'))


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
        # the library's headers: a test's header is built at C++11 and
        # later only, where a variadic macro is standard
        if path.startswith('inc/'):
            check_variadic(lines, report)
            check_sections(lines, report)
    check_includes(path, lines, report)
    if not is_test(path):
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
        elif options.warnings or (options.known and code in TRACKED):
            tag = 'warn'
        else:
            continue
        print('%s %s %s: %d%s' % (tag, path, code, count,
                                  (' (the punch list has %d)' % allowed)
                                  if allowed else ''))
        listed = details[(path, code)]
        shown = listed if options.show <= 0 else listed[:options.show]
        for number, message in shown:
            print('       %d: %s' % (number, message))
        if len(listed) > len(shown):
            print('       and %d more (--show 0 lists them all)' % (
                len(listed) - len(shown)))
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
            # errors, and the warnings the list tracks; another warning
            # fails nothing and is tracked by nothing, so it needs no line
            for (path, code), count in sorted(counts.items()):
                if code in ERRORS or code in TRACKED:
                    handle.write('%s\t%s\t%d\n' % (path, code, count))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
