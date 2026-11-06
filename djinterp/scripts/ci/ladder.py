#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                                        ladder.py
#
# The C and C++ ladders: compiles every library unit at every language level
# a compiler offers, plain and in testing mode, and reports what fails. This
# is the check behind the first release gate (version portability): every
# core unit compiles at every rung with GCC and Clang, and with MSVC at its
# levels.
#   A unit is a library file: each .h and .c of the C tree, and each .hpp and
# .cpp of the C++ tree, under inc/djinterp/ and src/djinterp/ (the generated
# tables under c/util/macro/ excepted), as the upgrade audit counted them;
# and, in their own groups, the headers of env/, config/ and inc/re_std/.
# A C header is compiled as C and, for its C++ face, as C++ as well. A header
# is compiled through a one-line unit that includes it, so it must stand
# alone. Nothing is linked: each compile checks syntax and meaning only.
#   The rungs: C99, C11, C17 and C23 (GCC's -std=c2x where it has no -std=c23)
# for C; ISO strict C++98 (with D_CFG_ENV_ISO_STRICT and RE_STD_CFG_ISO_STRICT
# set to 1), C++98, C++11, 14, 17, 20, 23 and 26 (-std=c++2c where there is
# no -std=c++26) for C++. A compiler that offers none of a rung's spellings
# skips it, and the summary says so. Each rung runs plain and with
# -DD_TESTING=1. GCC and Clang take the ladder's flags: -fsyntax-only
# -pedantic-errors -Werror=undef -Wall -Wextra -D_XOPEN_SOURCE=700; at C++98
# without ISO strictness, -Wno-variadic-macros as well, because that is the
# rung where the strict knob lets the framework use variadic macros, which
# GCC and Clang take in C++98 as an extension (decision 4.5; the db and math
# scripts build that rung the same way), and the strict rung holds C++98 to
# the standard alone (--variadic-errors leaves the flag out). Warnings
# are counted but do not fail a unit; an error does. MSVC (cl) runs at the
# rungs it has (C11, C17, the latest C; C++14, 17, 20 and the latest) with
# /Zs /permissive- /Zc:preprocessor /Zc:__cplusplus /utf-8 /W4 and C4668
# (an undefined name in #if) as an error, as -Werror=undef makes it.
#   Many units fail today; the punch list names them. --known holds a run to
# that list: a failure the list has is reported and allowed, a new one fails
# the run, and a unit on the list that now passes everywhere is reported so
# that its line can go. --write-known records a run's failures as the list.
#
#   usage:   python3 scripts/ci/ladder.py [options] [unit ...]
#   options: --lang c|cpp|both     the ladders to run (default both)
#            --cc LIST             C compilers, comma-separated (default
#                                  gcc,clang; cl on Windows)
#            --cxx LIST            C++ compilers (default g++,clang++; cl on
#                                  Windows)
#            --levels LIST         only these rungs (e.g. c99,c++98s,c++23)
#            --modes LIST          plain,testing (default both)
#            --groups LIST         c,cpp,env,config,re_std (default all)
#            --changed REV         only the units changed since REV (and the
#                                  new ones git does not track yet), with
#                                  the units that include a changed header
#            --known FILE          the punch list a run is held to
#            --write-known FILE    write this run's failures as the list
#            --json FILE           every result, with its diagnostics
#            --extra-flags FLAGS   added to every compile (e.g. -m32)
#            --jobs N              parallel compiles (default: processors)
#            --tree DIR            the tree to run over (default: the one
#                                  this script is in)
#            --variadic-errors     no -Wno-variadic-macros at non-strict
#                                  C++98
#   units:   paths under the tree's root, or @FILE for a list, one a line
#   exit:    0 when nothing failed (or nothing failed that --known lacks);
#            1 when something did; 2 when the run could not start
#
#
# path:      /scripts/ci/ladder.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
#                                                            revised: 2026.11.05
#*******************************************************************************
import argparse
import collections
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading

TREE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))))

# the rungs, with the spellings to try in order, and their extra defines
C_RUNGS = [('c99', ['c99'], []), ('c11', ['c11'], []), ('c17', ['c17'], []),
           ('c23', ['c23', 'c2x'], [])]
CXX_RUNGS = [('c++98s', ['c++98'],
              ['-DD_CFG_ENV_ISO_STRICT=1', '-DRE_STD_CFG_ISO_STRICT=1']),
             ('c++98', ['c++98'], []), ('c++11', ['c++11'], []),
             ('c++14', ['c++14'], []), ('c++17', ['c++17'], []),
             ('c++20', ['c++20'], []), ('c++23', ['c++23'], []),
             ('c++26', ['c++26', 'c++2c'], [])]
MSVC_C = {'c11': '/std:c11', 'c17': '/std:c17', 'c23': '/std:clatest'}
MSVC_CXX = {'c++14': '/std:c++14', 'c++17': '/std:c++17',
            'c++20': '/std:c++20', 'c++26': '/std:c++latest'}
# the rung where variadic macros are the framework's to use
EXTENSION_RUNGS = set(['c++98'])
GNU_FLAGS = ['-fsyntax-only', '-pedantic-errors', '-Werror=undef', '-Wall',
             '-Wextra']
MSVC_FLAGS = ['/nologo', '/Zs', '/permissive-', '/Zc:preprocessor',
              '/Zc:__cplusplus', '/utf-8', '/W4', '/we4668',
              '/external:anglebrackets', '/external:W0']
GENERATED = re.compile(r'/c/util/macro/\w*\d+\.h$')
INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)
GNU_DIAG = re.compile(r'^(.*?):(\d+):(?:\d+:)? (warning|error|fatal error): '
                      r'(.*)$')
MSVC_DIAG = re.compile(r'^(.*?)\((\d+)(?:,\d+)?\): (warning|error|fatal '
                       r'error) (C\d+): (.*)$')


def rel(path):
    """A path as the tree names it: relative to its root, with slashes."""
    return os.path.relpath(path, TREE).replace(os.sep, '/')


def walk(top, extensions):
    """The files under a folder of the tree with one of the extensions."""
    found = []
    root = os.path.join(TREE, top)

    for directory, names, files in os.walk(root):
        names.sort()
        for name in sorted(files):
            if name.endswith(extensions):
                path = rel(os.path.join(directory, name))
                if not GENERATED.search('/' + path):
                    found.append(path)

    return found


def all_units(groups):
    """Every unit, as (group, language, path); a C header comes twice."""
    units = []

    if 'c' in groups:
        for path in (walk('inc/djinterp/c', ('.h',)) +
                     walk('src/djinterp/c', ('.c', '.h'))):
            units.append(('c', 'c', path))
            if path.startswith('inc/') and path.endswith('.h'):
                units.append(('c', 'cpp', path))
    if 'cpp' in groups:
        for path in (walk('inc/djinterp/cpp', ('.hpp',)) +
                     walk('src/djinterp/cpp', ('.cpp', '.hpp'))):
            units.append(('cpp', 'cpp', path))
    for group, top in (('env', 'inc/djinterp/env'),
                       ('config', 'inc/djinterp/config'),
                       ('re_std', 'inc/re_std')):
        if group in groups:
            for path in walk(top, ('.h',)):
                units.append((group, 'c', path))
                units.append((group, 'cpp', path))
            for path in walk(top, ('.hpp',)):
                units.append((group, 'cpp', path))

    return units


def includers(units):
    """For each header, the units that include it directly."""
    by_header = collections.defaultdict(set)

    for _group, _language, path in units:
        try:
            with open(os.path.join(TREE, path), encoding='utf-8',
                      errors='replace') as handle:
                text = handle.read()
        except OSError:
            continue
        for target in INCLUDE.findall(text):
            resolved = os.path.normpath(os.path.join(
                os.path.dirname(os.path.join(TREE, path)), target))
            by_header[rel(resolved)].add(path)

    return by_header


def changed_units(units, revision):
    """The units changed since a revision, and those including a changed
    header."""
    result = subprocess.run(['git', 'diff', '--name-only', '--relative',
                             revision, '--', '.'],
                            cwd=TREE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            universal_newlines=True)
    if result.returncode != 0:
        sys.exit('ladder.py: git diff ' + revision + ': ' +
                 result.stderr.strip())
    changed = set(line.strip() for line in result.stdout.splitlines()
                  if line.strip())
    # a new file is not in the diff until it is added: take those too
    result = subprocess.run(['git', 'ls-files', '--others',
                             '--exclude-standard', '--', '.'],
                            cwd=TREE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            universal_newlines=True)
    changed |= set(line.strip() for line in result.stdout.splitlines()
                   if line.strip())
    by_header = includers(units)
    wanted = set(changed)

    for path in changed:
        wanted |= by_header.get(path, set())

    return [unit for unit in units if unit[2] in wanted]


class Compiler(object):
    """One compiler: how to spell its rungs, and how to run a unit."""

    def __init__(self, name, language, extra):
        self.name = name
        self.language = language
        self.extra = extra
        self.msvc = os.path.basename(name).lower() in ('cl', 'cl.exe')
        self.rungs = {}
        self.missing = []

    def probe(self, scratch, wanted):
        """Which rungs this compiler offers, and with which spelling."""
        table = C_RUNGS if self.language == 'c' else CXX_RUNGS
        empty = os.path.join(scratch, 'probe.' + ('c' if self.language == 'c'
                                                  else 'cpp'))
        with open(empty, 'w') as handle:
            handle.write('int d_ladder_probe;\n')

        for rung, spellings, defines in table:
            if wanted and rung not in wanted:
                continue
            if self.msvc:
                flag = (MSVC_C if self.language == 'c' else
                        MSVC_CXX).get(rung)
                if flag and self.accepts([flag, '/Zs', empty]):
                    self.rungs[rung] = ([flag], defines)
                else:
                    self.missing.append(rung)
                continue
            for spelling in spellings:
                if self.accepts(['-std=' + spelling, '-fsyntax-only',
                                 empty]):
                    self.rungs[rung] = (['-std=' + spelling], defines)
                    break
            else:
                self.missing.append(rung)

    def accepts(self, arguments):
        """Whether the compiler takes these arguments without complaint."""
        try:
            result = subprocess.run([self.name] + arguments,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    universal_newlines=True)
        except OSError:
            return False
        # cl only warns on an option it does not know (D9002)
        noise = result.stdout + result.stderr
        return result.returncode == 0 and 'D9002' not in noise

    def command(self, rung, mode, unit_file, is_header_c_as_cpp):
        """The command that compiles a unit at a rung, in a mode."""
        flags, defines = self.rungs[rung]
        testing = ['-DD_TESTING=1'] if mode == 'testing' else []
        include = os.path.join(TREE, 'inc')

        if self.msvc:
            language = '/TC' if self.language == 'c' else '/TP'
            return ([self.name] + MSVC_FLAGS + flags + [language] +
                    [d.replace('-D', '/D', 1) for d in defines + testing] +
                    ['/I' + include] + self.extra + [unit_file])

        warnings = GNU_FLAGS
        if rung in EXTENSION_RUNGS:
            warnings = GNU_FLAGS + ['-Wno-variadic-macros']
        command = [self.name] + flags + warnings + defines + testing
        if os.name != 'nt':
            command.append('-D_XOPEN_SOURCE=700')
        if is_header_c_as_cpp:
            command += ['-x', 'c++']
        return command + ['-I', include] + self.extra + [unit_file]


def wrap(scratch, path, language):
    """The file to compile for a unit: a source as it is, a header through a
    one-line unit that includes it."""
    if not path.endswith(('.h', '.hpp')):
        return os.path.join(TREE, path)

    name = path.replace('/', '_') + ('.c' if language == 'c' else '.cpp')
    unit = os.path.join(scratch, name)
    if not os.path.exists(unit):
        # written whole, then moved into place: another compile of the same
        # header must never see the file half written
        partial = '%s.%d' % (unit, threading.get_ident())
        with open(partial, 'w') as handle:
            handle.write('#include "%s"\nint d_ladder_unit;\n' %
                         os.path.join(TREE, path).replace('\\', '/'))
        os.replace(partial, unit)
    return unit


def compile_one(compiler, scratch, unit, rung, mode):
    """Compiles one unit at one rung in one mode."""
    _group, language, path = unit
    unit_file = wrap(scratch, path, language)
    command = compiler.command(rung, mode, unit_file,
                               language == 'cpp' and path.endswith('.h'))
    try:
        result = subprocess.run(command, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                universal_newlines=True, errors='replace',
                                timeout=600)
    except subprocess.TimeoutExpired:
        return {'ok': False, 'errors': ['timed out'], 'warnings': 0}

    errors = []
    warnings = 0
    pattern = MSVC_DIAG if compiler.msvc else GNU_DIAG
    for line in (result.stdout + '\n' + result.stderr).splitlines():
        match = pattern.match(line)
        if not match:
            continue
        where = match.group(1).replace('\\', '/')
        where = rel(where) if os.path.isabs(where) else where
        if 'ladder_' in where:
            where = '<unit>'
        if match.group(3) == 'warning':
            warnings += 1
        elif len(errors) < 3:
            errors.append('%s:%s: %s' % (where, match.group(2),
                                         match.group(match.lastindex)))
    ok = result.returncode == 0
    if not ok and not errors:
        tail = (result.stdout + result.stderr).strip().splitlines()
        errors.append(tail[-1][:200] if tail else 'failed')
    return {'ok': ok, 'errors': errors, 'warnings': warnings}


def read_known(path):
    """The punch list: unit and language, to the rungs and modes allowed to
    fail."""
    known = {}

    with open(path, encoding='utf-8') as handle:
        for line in handle:
            line = line.rstrip('\r\n')
            if not line or line.startswith('#'):
                continue
            fields = line.split('\t')
            key = (fields[0], fields[1])
            known[key] = set(fields[2].split(',')) if len(fields) > 2 else set()

    return known


def write_known(path, failures, first_error):
    """Writes a run's failures as the punch list, grouped by module."""
    lines = ['# djinterp ladder punch list: unit<TAB>language<TAB>the '
             'rung:mode pairs that fail<TAB>the first error',
             '# written by scripts/ci/ladder.py --write-known; a line goes '
             'when its unit passes everywhere']
    for key in sorted(failures):
        pairs = ','.join(sorted(failures[key]))
        lines.append('\t'.join([key[0], key[1], pairs,
                                first_error.get(key, '')[:160]]))
    with open(path, 'w', encoding='utf-8', newline='\n') as handle:
        handle.write('\n'.join(lines) + '\n')


def describe(pairs, offered):
    """The rung:mode pairs a unit fails at, briefly: "every rung", or each
    rung, with its mode where only one of the two fails."""
    if pairs >= offered:
        return 'every rung'

    order = [rung for rung, _, _ in C_RUNGS + CXX_RUNGS]
    modes = collections.defaultdict(set)
    for pair in pairs:
        rung, _, mode = pair.partition(':')
        modes[rung].add(mode)
    words = []
    for rung in sorted(modes, key=lambda r: order.index(r) if r in order
                       else len(order)):
        both = len(modes[rung]) > 1
        words.append(rung if both else rung + ' ' + next(iter(modes[rung])))
    return ', '.join(words)


def module_of(path):
    """The module a unit belongs to, for the summary."""
    parts = path.split('/')
    if parts[0] == 'inc' and parts[1] == 're_std':
        return 're_std'
    if len(parts) > 3 and parts[2] in ('c', 'cpp'):
        if parts[3] == 'modules' and len(parts) > 5:
            return parts[2] + '/' + parts[4]
        if len(parts) > 4:
            return parts[2] + '/' + parts[3]
        return parts[2] + '/(root)'
    return parts[2] if len(parts) > 3 else path


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--lang', default='both', choices=('c', 'cpp', 'both'))
    parser.add_argument('--cc')
    parser.add_argument('--cxx')
    parser.add_argument('--levels')
    parser.add_argument('--modes', default='plain,testing')
    parser.add_argument('--groups', default='c,cpp,env,config,re_std')
    parser.add_argument('--changed')
    parser.add_argument('--known')
    parser.add_argument('--write-known')
    parser.add_argument('--json')
    parser.add_argument('--extra-flags', default='')
    parser.add_argument('--jobs', type=int, default=os.cpu_count() or 2)
    parser.add_argument('--tree')
    parser.add_argument('--variadic-errors', action='store_true')
    parser.add_argument('units', nargs='*')
    options = parser.parse_args()

    if options.variadic_errors:
        EXTENSION_RUNGS.clear()

    global TREE
    if options.tree:
        TREE = os.path.abspath(options.tree)

    windows = os.name == 'nt'
    cc = (options.cc or ('cl' if windows else 'gcc,clang')).split(',')
    cxx = (options.cxx or ('cl' if windows else 'g++,clang++')).split(',')
    wanted = set(options.levels.split(',')) if options.levels else set()
    modes = options.modes.split(',')
    groups = set(options.groups.split(','))
    extra = options.extra_flags.split()

    units = all_units(groups)
    if options.units:
        listed = []
        for item in options.units:
            if item.startswith('@'):
                with open(item[1:], encoding='utf-8') as handle:
                    listed += [ln.strip() for ln in handle if ln.strip()]
            else:
                listed.append(item.replace('\\', '/'))
        units = [unit for unit in units if unit[2] in set(listed)]
    if options.changed:
        units = changed_units(units, options.changed)
    languages = ('c', 'cpp') if options.lang == 'both' else (options.lang,)
    units = [unit for unit in units if unit[1] in languages]

    scratch = tempfile.mkdtemp(prefix='ladder_')
    compilers = []
    notes = []
    for names, language in ((cc, 'c'), (cxx, 'cpp')):
        if language not in languages:
            continue
        for name in names:
            if not shutil.which(name):
                notes.append('SKIP %s: not found' % name)
                continue
            compiler = Compiler(name, language, extra)
            compiler.probe(scratch, wanted)
            if compiler.missing:
                notes.append('NOTE %s offers no %s' % (
                    name, ', '.join(compiler.missing)))
            if compiler.rungs:
                compilers.append(compiler)
    if not compilers:
        print('ladder.py: no compiler to run', file=sys.stderr)
        return 2

    jobs = []
    for compiler in compilers:
        for unit in units:
            if unit[1] != compiler.language:
                continue
            for rung in compiler.rungs:
                for mode in modes:
                    jobs.append((compiler, unit, rung, mode))

    results = {}
    with concurrent.futures.ThreadPoolExecutor(options.jobs) as pool:
        futures = {pool.submit(compile_one, job[0], scratch, job[1], job[2],
                               job[3]): job for job in jobs}
        for done, future in enumerate(
                concurrent.futures.as_completed(futures), 1):
            compiler, unit, rung, mode = futures[future]
            results[(compiler.name, unit, rung, mode)] = future.result()
            if done % 2000 == 0:
                print('ladder.py: %d of %d compiles' % (done, len(jobs)),
                      file=sys.stderr)
    shutil.rmtree(scratch, ignore_errors=True)

    # the totals, per compiler, rung and mode
    print('djinterp ladder: %d units, %d compiles' % (
        len(set(u[1:] for u in units)), len(jobs)))
    for note in notes:
        print(note)
    for compiler in compilers:
        for rung in compiler.rungs:
            for mode in modes:
                rows = [value for key, value in results.items()
                        if key[0] == compiler.name and key[2] == rung and
                        key[3] == mode]
                passed = sum(1 for value in rows if value['ok'])
                warned = sum(value['warnings'] for value in rows)
                print('  %-8s %-7s %-8s %5d of %5d pass   warnings: %d' % (
                    compiler.name, rung, mode, passed, len(rows), warned))

    # what each language was run at, to say "every rung" briefly
    offered = collections.defaultdict(set)
    for compiler in compilers:
        for rung in compiler.rungs:
            for mode in modes:
                offered[compiler.language].add(rung + ':' + mode)

    # the failures, by unit: the rung:mode pairs, with any compiler
    failures = collections.defaultdict(set)
    first_error = {}
    for (name, unit, rung, mode), value in sorted(
            results.items(), key=lambda item: (item[0][1][2], item[0][2])):
        if not value['ok']:
            key = (unit[2], unit[1])
            failures[key].add(rung + ':' + mode)
            first_error.setdefault(key, '%s %s: %s' % (
                name, rung, value['errors'][0] if value['errors'] else ''))

    by_module = collections.Counter(module_of(key[0]) for key in failures)
    print('failing units: %d' % len(failures))
    for module, count in sorted(by_module.items()):
        print('  %-28s %d' % (module, count))

    status = 0
    if options.known:
        known = read_known(options.known)
        new = {key: pairs - known.get(key, set())
               for key, pairs in failures.items()
               if not pairs <= known.get(key, set())}
        # a unit is fixed only where every rung it failed at ran again
        present = set((unit[2], unit[1]) for unit in units)
        fixed = sorted(key for key in known
                       if key not in failures and key in present and
                       known[key] and known[key] <= offered[key[1]])
        for key in sorted(new):
            print('FAIL %s (%s) at %s: %s' % (
                key[0], key[1], describe(new[key], offered[key[1]]),
                first_error.get(key, '')))
        for key in fixed:
            print('FIXED %s (%s): passes everywhere; remove its line from '
                  'the punch list' % key)
        print('ladder: %d new failures, %d known, %d fixed' % (
            len(new), len(failures) - len(new), len(fixed)))
        status = 1 if new else 0
    else:
        for key in sorted(failures):
            print('FAIL %s (%s) at %s: %s' % (
                key[0], key[1], describe(failures[key], offered[key[1]]),
                first_error.get(key, '')))
        status = 1 if failures else 0

    if options.write_known:
        write_known(options.write_known, failures, first_error)
    if options.json:
        with open(options.json, 'w', encoding='utf-8') as handle:
            json.dump([{'compiler': key[0], 'group': key[1][0],
                        'language': key[1][1], 'unit': key[1][2],
                        'rung': key[2], 'mode': key[3], 'ok': value['ok'],
                        'errors': value['errors'],
                        'warnings': value['warnings']}
                       for key, value in sorted(
                           results.items(),
                           key=lambda item: (item[0][1][2], item[0][0],
                                             item[0][2], item[0][3]))],
                      handle, indent=0)
    return status


if __name__ == '__main__':
    sys.exit(main())
