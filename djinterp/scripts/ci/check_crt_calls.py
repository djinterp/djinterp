#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                              check_crt_calls.py
#
# The rule for the C runtime's calls that MSVC deprecates: no C or C++ file
# calls one where MSVC compiles the call. Every MSVC target builds with /sdl,
# which makes the deprecation (C4996) an error, so such a call breaks the
# Windows build; this check finds it on any platform, before a Windows run.
#   The calls are the runtime's "unsafe" ones (fopen, getenv, sprintf,
# strcpy, strerror, localtime and their kin) and the POSIX names MSVC wants
# underscored (open, close, fileno, strdup, unlink and their kin). Each has
# a portable form in the tree: filesys's file and directory calls, and its
# d_file_is_terminal for isatty; string_fn's d_strcpy_s and kin; dio's
# d_snprintf and d_sscanf; dtime's d_localtime and kin; dstdlib's
# d_getenv_copy and d_getenv_dup for getenv, d_setenv and d_unsetenv for
# putenv; or snprintf, which MSVC accepts.
#   A call MSVC never compiles is allowed: in a branch of #if, #ifdef or
# #ifndef that excludes MSVC (on _MSC_VER, D_ENV_COMPILER_MSVC_FAMILY or
# D_ENV_CRT_MSVC) or Windows (_WIN32, _WIN64, D_ENV_IS_OS_WINDOWS), or that
# holds only on a system MSVC does not build for (__linux__, __APPLE__,
# D_ENV_IS_OS_POSIX and their kin), and in every branch after one MSVC
# always takes. A file the build compiles only off Windows, with no such
# condition in it, keeps its line on the list. So is a call in the files that
# make the portable forms (EXEMPT): calling the runtime is what they are for,
# and they say so to MSVC themselves.
#   Comments, strings and member calls (x.read(), p->close()) are not calls
# of the runtime and are not counted.
#   The tree still has calls, until each lane replaces its own. --known
# holds a run to that list: a file may make as many calls as its line
# allows; a file making more fails the run; a file making fewer is reported,
# so that its line can come down.
#
#   usage:   python3 scripts/ci/check_crt_calls.py [options] [path ...]
#   options: --known FILE         each file's calls allowed for now
#            --write-known FILE   write this run's counts as the list
#            --tree DIR           the tree to check (default: the one this
#                                 script is in)
#            --list               print each call found, with its line
#   paths:   files or folders under the tree's root; default inc/, src/,
#            tests/
#   exit:    0 when no file makes more calls than its line allows; 1
#            otherwise
#
#
# path:      /scripts/ci/check_crt_calls.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
#                                                            revised: 2026.11.26
#*******************************************************************************
import argparse
import os
import re
import sys

TREE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))))
EXTENSIONS = ('.h', '.c', '.hpp', '.cpp', '.inl', '.ipp', '.tpp', '.inc')
DEFAULT_PATHS = ('inc', 'src', 'tests')

# the runtime's calls MSVC deprecates, by the form the tree has for each
CALLS = (
    # files and directories: filesys
    'fopen', 'freopen', 'fdopen', 'tmpnam', 'tmpfile', 'mktemp',
    'open', 'close', 'read', 'write', 'lseek', 'dup', 'dup2', 'fileno',
    'isatty', 'access', 'unlink', 'mkdir', 'rmdir', 'chdir', 'getcwd',
    'umask', 'chmod',
    # strings: string_fn
    'strcpy', 'strcat', 'strncpy', 'strncat', 'strdup', 'strtok', 'strerror',
    'strlwr', 'strupr', 'stricmp', 'strnicmp', 'wcscpy', 'wcscat', 'wcsncpy',
    'mbstowcs', 'wcstombs',
    # formatted text: snprintf, or dio
    'sprintf', 'vsprintf', 'sscanf', 'fscanf', 'scanf', 'vsscanf', 'vfscanf',
    # time: dtime
    'localtime', 'gmtime', 'ctime', 'asctime',
    # the environment
    'getenv', 'putenv', 'getpid',
)
# the POSIX names a C++ class may also give its own members: in C++ they
# count only where qualified as the runtime's (::close, std::fopen)
GENERIC = {'open', 'close', 'read', 'write', 'lseek', 'dup', 'dup2',
           'fileno', 'isatty', 'access', 'unlink', 'mkdir', 'rmdir', 'chdir',
           'getcwd', 'umask', 'chmod', 'getpid'}
CPP_EXTENSIONS = ('.hpp', '.cpp', '.ipp', '.tpp', '.inl')
CALL = re.compile(r'\b(' + '|'.join(CALLS) + r')\s*\(')
NOT_A_CALL_AFTER = {'return', 'else', 'case', 'do', 'sizeof', 'throw',
                    'co_return', 'co_await', 'co_yield', 'not', 'and', 'or'}

# the files that make the portable forms
EXEMPT = {
    'src/djinterp/c/string_fn.c',
    'src/djinterp/c/dstring.c',
    'src/djinterp/c/dio.c',
    'src/djinterp/c/dtime.c',
    'src/djinterp/c/dstdlib.c',
}
EXEMPT_DIRS = (
    'src/djinterp/c/filesys/',
    'inc/djinterp/c/filesys/',
    # re_std wraps the runtime itself, and its harnesses compare against it
    'inc/re_std/',
    'src/re_std/',
    'tests/re_std/',
    # bitwisedom's platform checks build against the platform alone
    'tests/djinterp/tools/bitwisedom/platform/',
)

# a condition that holds only off MSVC: off Windows, or on a system MSVC
# does not build for
NOT_MSVC = re.compile(
    r'^\s*(?:'
    r'!\s*defined\s*\(?\s*(?:_MSC_VER|_WIN32|_WIN64)\s*\)?'
    r'|!\s*\(?\s*(?:D_ENV_COMPILER_MSVC_FAMILY|D_ENV_CRT_MSVC|'
    r'D_ENV_IS_OS_WINDOWS)\s*\)?'
    r'|defined\s*\(?\s*(?:__unix__|__unix|__linux__|__APPLE__|'
    r'__FreeBSD__|__NetBSD__|__OpenBSD__|__EMSCRIPTEN__|__wasi__)\s*\)?'
    r'|\(?\s*(?:D_ENV_IS_OS_POSIX|D_ENV_IS_OS_UNIX|D_ENV_IS_OS_LINUX|'
    r'D_ENV_IS_OS_APPLE|D_ENV_IS_OS_BSD|D_ENV_IS_OS_WASM)\s*\)?'
    r')\s*$')
# a condition that holds only on MSVC, or only on Windows
ONLY_MSVC = re.compile(
    r'^\s*(?:'
    r'defined\s*\(?\s*(?:_MSC_VER|_WIN32|_WIN64)\s*\)?'
    r'|\(?\s*(?:D_ENV_COMPILER_MSVC_FAMILY|D_ENV_CRT_MSVC|'
    r'D_ENV_IS_OS_WINDOWS)\s*\)?'
    r')\s*$')


def strip(text):
    """The text with comments and string and character literals blanked,
    keeping every newline, so that line numbers stay."""
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            out.append(' ' * (j - i))
            i = j
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r'[^\n]', ' ', text[i:j]))
            i = j
        elif c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c and text[j] != '\n':
                j += 2 if text[j] == '\\' else 1
            j = min(j + 1, n)
            out.append(c + ' ' * (j - i - 2) + (c if j - i >= 2 else ''))
            i = j
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def is_call(line, match, cpp):
    """Whether a name followed by '(' calls the runtime's function, rather
    than declaring a function of that name or calling a member."""
    name = match.group(1)
    before = line[:match.start()].rstrip()
    qualified = False
    if before.endswith('::'):
        head = before[:-2].rstrip()
        scope = re.search(r'(\w*)$', head).group(1)
        if head.endswith('>') or scope not in ('', 'std'):
            return False            # another class's, namespace's or template's
        qualified = True
        before = head[:len(head) - len(scope)].rstrip()
    if before.endswith(('.', '->')):
        return False                # a member
    if cpp and name in GENERIC and not qualified:
        return False                # maybe a member of the enclosing class
    # a declaration: the name follows a type (an identifier, or a pointer or
    # reference to one), not an operator or a keyword that takes a value
    word = re.search(r'([A-Za-z_]\w*)\s*[*&]*$', before)
    if word and word.group(1) not in NOT_A_CALL_AFTER and \
            not before.endswith(('(', ',', '=', '!', '?', ':', '{', ';')):
        if re.search(r'[*&\w]$', before):
            return False
    return True


def calls_in(text, cpp=False):
    """Each call MSVC would compile: (line, name)."""
    found = []
    # per #if: [this branch is one MSVC never takes, MSVC took an earlier one]
    stack = []
    raw = strip(text).split('\n')
    lines = []
    k = 0
    while k < len(raw):
        line = raw[k]
        number = k + 1
        while line.rstrip().endswith('\\') and k + 1 < len(raw):
            k += 1
            line = line.rstrip()[:-1] + ' ' + raw[k]
        lines.append((number, line))
        k += 1

    for number, line in lines:
        directive = re.match(r'\s*#\s*(\w+)(.*)$', line)
        if directive:
            word, rest = directive.group(1), directive.group(2).strip()
            if word in ('if', 'ifdef', 'ifndef'):
                if word == 'ifdef':
                    rest = 'defined(%s)' % rest
                elif word == 'ifndef':
                    rest = '!defined(%s)' % rest
                taken = bool(ONLY_MSVC.match(rest))
                stack.append([bool(NOT_MSVC.match(rest)), taken])
            elif word == 'elif' and stack:
                earlier = stack[-1][1]
                stack[-1] = [earlier or bool(NOT_MSVC.match(rest)),
                             earlier or bool(ONLY_MSVC.match(rest))]
            elif word == 'else' and stack:
                stack[-1] = [stack[-1][1], stack[-1][1]]
            elif word == 'endif' and stack:
                stack.pop()
            elif word == 'define' and not any(e[0] for e in stack):
                body = re.sub(r'^\w+(\([^)]*\))?', '', rest)
                for match in CALL.finditer(body):
                    if is_call(body, match, cpp):
                        found.append((number, match.group(1)))
            continue
        if any(entry[0] for entry in stack):
            continue
        for match in CALL.finditer(line):
            if is_call(line, match, cpp):
                found.append((number, match.group(1)))
    return found


def exempt(rel):
    return rel in EXEMPT or rel.startswith(EXEMPT_DIRS)


def read_known(path):
    known = {}
    with open(path, encoding='utf-8') as handle:
        for line in handle:
            line = line.rstrip('\r\n')
            if not line or line.startswith('#'):
                continue
            fields = line.split('\t')
            known[fields[0]] = int(fields[1])
    return known


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('paths', nargs='*')
    parser.add_argument('--known')
    parser.add_argument('--write-known')
    parser.add_argument('--tree')
    parser.add_argument('--list', action='store_true')
    options = parser.parse_args()

    tree = os.path.abspath(options.tree) if options.tree else TREE
    roots = options.paths or list(DEFAULT_PATHS)

    files = []
    for root in roots:
        full = os.path.join(tree, root)
        if os.path.isfile(full):
            files.append(full)
            continue
        for folder, dirs, names in os.walk(full):
            dirs.sort()
            for name in sorted(names):
                if name.endswith(EXTENSIONS):
                    files.append(os.path.join(folder, name))

    counts = {}
    for path in files:
        rel = os.path.relpath(path, tree).replace(os.sep, '/')
        if exempt(rel):
            continue
        with open(path, encoding='utf-8', errors='replace') as handle:
            found = calls_in(handle.read(), path.endswith(CPP_EXTENSIONS))
        if found:
            counts[rel] = len(found)
            if options.list:
                for number, name in found:
                    print('%s:%d: %s' % (rel, number, name))

    if options.write_known:
        with open(options.write_known, 'w', encoding='utf-8',
                  newline='\r\n') as handle:
            handle.write('# check_crt_calls.py: each file\'s calls of the C '
                         'runtime that MSVC refuses\n')
            handle.write('# under /sdl, allowed until the file is moved onto '
                         'the tree\'s portable forms;\n')
            handle.write('# a line comes down as its calls go, and goes '
                         'at 0\n')
            for rel in sorted(counts):
                handle.write('%s\t%d\n' % (rel, counts[rel]))

    known = read_known(options.known) if options.known else {}
    over = []
    under = []
    for rel in sorted(counts):
        allowed = known.get(rel, 0)
        if counts[rel] > allowed:
            over.append((rel, counts[rel], allowed))
        elif counts[rel] < allowed:
            under.append((rel, counts[rel], allowed))
    if options.known:
        for rel in sorted(set(known) - set(counts)):
            under.append((rel, 0, known[rel]))

    for rel, have, allowed in over:
        print('%s: %d calls MSVC refuses, the list allows %d' % (
            rel, have, allowed))
    for rel, have, allowed in under:
        print('%s: %d calls, the list allows %d: its line can come down' % (
            rel, have, allowed))
    print('check_crt_calls: %d calls in %d files; %d files above the list' % (
        sum(counts.values()), len(counts), len(over)))
    return 1 if over else 0


if __name__ == '__main__':
    sys.exit(main())
