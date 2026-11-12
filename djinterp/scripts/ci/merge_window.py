#!/usr/bin/env python3
#*******************************************************************************
# djinterp [ci]                                                  merge_window.py
#
# Lays a merge window's lane zips over the tree, in order, and says what each
# would do before anything is written: the files each zip adds and changes,
# the files two zips both carry, the files a zip would take back to an older
# state because the tree changed them after the lane's base, the line
# endings, and the deletions a zip cannot carry. With --gates it then builds
# the result in a scratch checkout and runs the checks over it: the ladder
# and the style over what it changed, the fixed-width includes, the roots'
# growth and the version; with --apply it writes the result into the tree,
# and with --commit it commits each zip as its own commit, with its lane's
# message.
#   A lane zip holds whole files, at their paths under the tree's root
# (inc/, src/, tests/, ...), CRLF throughout, except .sh files, which are LF
# (ruled 2026.11.05). Where two zips carry one file, or where the tree
# changed a file after the base the lane started from, the later whole file
# would silently undo the other change. Given the lane's base (--base, or
# --base-of for one zip), each such file is merged three ways instead: the
# base's version, the tree's, and the zip's, with git merge-file. A clean
# merge keeps both changes; a conflict is reported and nothing is written
# for that file until it is resolved.
#
#   usage:   python3 scripts/ci/merge_window.py [options] ZIP[=MESSAGE] ...
#            (from anywhere in the repository; ZIPs in the order to apply,
#            each optionally with the file holding its commit message)
#   options: --base REV           the commit the lanes started from
#            --base-of ZIP=REV    one zip's base, where it differs
#            --delete ZIP=PATH    a file the zip's handoff deletes (repeat)
#            --delete ZIP=@FILE   every path FILE lists, one a line (a lane's
#                                 deletions file; # starts a comment)
#            --gates              build the result in a scratch checkout and
#                                 run the checks over it
#            --apply              write the result into the tree
#            --commit             with --apply: one commit per zip, with its
#                                 message file
#   exit:    0 when the window is clean (every file placed, every merge
#            clean, line endings right, gates passed if run); 1 when it is
#            not; 2 when the run could not start
#
#
# path:      /scripts/ci/merge_window.py
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
#                                                            revised: 2026.11.11
#*******************************************************************************
import argparse
import os
import posixpath
import shutil
import subprocess
import sys
import tempfile
import zipfile

TREE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))))
TOPS = {'inc', 'src', 'tests', 'docs', 'build', 'scripts', 'tools', 'res',
        'lib', 'bin', '.gitattributes', '.gitignore', 'README.md',
        'README.html'}
BINARY = ('.png', '.jpg', '.jpeg', '.gif', '.ico', '.bmp', '.zip', '.gz',
          '.7z', '.pdf', '.ttf', '.otf', '.woff', '.woff2', '.exe', '.dll',
          '.so', '.a', '.lib', '.obj', '.o', '.bin', '.dat', '.db', '.sqlite',
          '.wasm', '.xlsx', '.docx', '.pptx')


def git(arguments, cwd=TREE, binary=False, check=True):
    """Runs git; returns its standard output."""
    result = subprocess.run(['git'] + arguments, cwd=cwd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    if check and result.returncode != 0:
        raise RuntimeError('git %s: %s' % (' '.join(arguments),
                                           result.stderr.decode(
                                               'utf-8', 'replace').strip()))
    return result.stdout if binary else result.stdout.decode('utf-8',
                                                             'replace')


def is_text(path, data):
    """Whether a file is text: by its extension, and by having no NUL."""
    return (not path.lower().endswith(BINARY)) and (b'\0' not in data[:8192])


def lf(data):
    """A text file's bytes with CRLF made LF, for comparing and merging."""
    return data.replace(b'\r\n', b'\n')


def eol_problem(path, data):
    """What is wrong with a text file's line endings, or None."""
    if path.endswith('.sh'):
        bad = data.count(b'\r')
        return ('%d CR in a .sh file, which must be LF' % bad) if bad else None
    lines = data.count(b'\n')
    crlf = data.count(b'\r\n')
    lone = data.count(b'\r') - crlf
    if lines != crlf or lone:
        return '%d of %d lines not CRLF' % (lines - crlf + lone, lines)
    return None


def to_rule(path, data):
    """A text file's bytes in the tree's line endings: LF for .sh, CRLF for
    everything else."""
    data = lf(data)
    return data if path.endswith('.sh') else data.replace(b'\n', b'\r\n')


def read_zip(path):
    """A zip's files, as {path under the tree's root: bytes}, and what is
    wrong with its layout."""
    files = {}
    problems = []

    with zipfile.ZipFile(path) as archive:
        for info in archive.infolist():
            if info.is_dir():
                continue
            name = info.filename.replace('\\', '/')
            clean = posixpath.normpath(name)
            if (clean.startswith(('/', '../')) or clean == '..' or
                    ':' in clean.split('/')[0]):
                problems.append('%s: a path outside the tree' % name)
                continue
            files[clean] = archive.read(info)

    tops = set(name.split('/')[0] for name in files)
    if tops and not tops & TOPS:
        problems.append('top level is %s, not the tree root (inc/, src/, '
                        'tests/, ...)' % ', '.join(sorted(tops)))
    return files, problems


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('zips', nargs='+')
    parser.add_argument('--base')
    parser.add_argument('--base-of', action='append', default=[])
    parser.add_argument('--delete', action='append', default=[])
    parser.add_argument('--gates', action='store_true')
    parser.add_argument('--apply', action='store_true')
    parser.add_argument('--commit', action='store_true')
    options = parser.parse_args()

    try:
        top = git(['rev-parse', '--show-toplevel']).strip()
        prefix = git(['rev-parse', '--show-prefix']).strip()
        head = git(['rev-parse', '--short', 'HEAD']).strip()
    except RuntimeError as error:
        print('merge_window.py: %s' % error, file=sys.stderr)
        return 2

    zips = []
    for item in options.zips:
        path, _, message = item.partition('=')
        if not os.path.isfile(path):
            print('merge_window.py: no zip %s' % path, file=sys.stderr)
            return 2
        if options.commit and not message:
            print('merge_window.py: --commit needs ZIP=MESSAGE for %s' % path,
                  file=sys.stderr)
            return 2
        zips.append((path, message))
    bases = {}
    for item in options.base_of:
        name, _, revision = item.partition('=')
        bases[os.path.basename(name)] = revision
    deletions = {}
    for item in options.delete:
        name, _, path = item.partition('=')
        paths = [path]
        if path.startswith('@'):
            try:
                with open(path[1:], encoding='utf-8-sig') as handle:
                    paths = [line.strip() for line in handle
                             if line.strip() and
                             not line.lstrip().startswith('#')]
            except OSError as error:
                print('merge_window.py: %s' % error, file=sys.stderr)
                return 2
        for each in paths:
            deletions.setdefault(os.path.basename(name), []).append(
                each.replace('\\', '/'))

    dirty = git(['status', '--porcelain', '--', '.']).strip()
    print('merge window: %d zips over %s (tree %s)' % (
        len(zips), head, 'clean' if not dirty else 'with changes not '
        'committed: they count as the tree'))

    def base_blob(revision, path):
        """A file at a revision, or None where it did not exist."""
        spec = '%s:%s%s' % (revision, prefix, path)
        result = subprocess.run(['git', 'show', spec], cwd=TREE,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        return result.stdout if result.returncode == 0 else None

    def current(path, written):
        """A file as the window has it so far: an earlier zip's result, or
        the tree's."""
        if path in written:
            return written[path]
        full = os.path.join(TREE, path)
        if os.path.isfile(full):
            with open(full, 'rb') as handle:
                return handle.read()
        return None

    def merge3(base, ours, theirs):
        """git merge-file over LF copies: (merged bytes, conflicts)."""
        scratch = tempfile.mkdtemp(prefix='merge_window_')
        names = []
        for name, data in (('ours', ours), ('base', base),
                           ('theirs', theirs)):
            full = os.path.join(scratch, name)
            with open(full, 'wb') as handle:
                handle.write(lf(data))
            names.append(full)
        result = subprocess.run(['git', 'merge-file', '-p', '-L', 'tree',
                                 '-L', 'base', '-L', 'zip'] + names,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        shutil.rmtree(scratch, ignore_errors=True)
        return result.stdout, max(result.returncode, 0)

    if not options.base and not bases:
        print('  no --base: where the tree changed a file after a lane\'s '
              'base, the zip\'s whole file would undo it, and that cannot be '
              'told without the base')

    failed = False
    written = {}
    carried = {}
    plan = []
    for path, message in zips:
        name = os.path.basename(path)
        files, problems = read_zip(path)
        base = bases.get(name, options.base)
        added = changed = same = merged = 0
        notes = []
        for problem in problems:
            notes.append('LAYOUT %s' % problem)
            failed = True
        for rel in sorted(files):
            data = files[rel]
            text = is_text(rel, data)
            if text:
                problem = eol_problem(rel, data)
                if problem:
                    notes.append('EOL %s: %s; written as the rule says' % (
                        rel, problem))
                    data = to_rule(rel, data)
            now = current(rel, written)
            if now is None:
                added += 1
                result = data
            elif (lf(now) if text else now) == (lf(data) if text else data):
                same += 1
                result = data
            else:
                changed += 1
                result = data
                then = base_blob(base, rel) if base else None
                earlier = carried.get(rel)
                if earlier:
                    notes.append('OVERLAP %s: also in %s' % (rel, earlier))
                if base and then is None:
                    notes.append('CONFLICT %s: new since %s, and made both by '
                                 '%s and by this zip; not written' % (
                                     rel, base, earlier or 'the tree'))
                    failed = True
                    continue
                if text and then is not None and lf(then) != lf(now):
                    # the tree changed it after the lane's base: merge
                    body, conflicts = merge3(then, now, data)
                    if conflicts:
                        notes.append('CONFLICT %s: %d place%s changed by '
                                      'both the tree (or an earlier zip) and '
                                      'this zip; not written' % (
                                          rel, conflicts,
                                          '' if conflicts == 1 else 's'))
                        failed = True
                        continue
                    merged += 1
                    result = to_rule(rel, body)
                    notes.append('MERGED %s: the tree changed it since %s; '
                                 'both changes kept' % (rel, base))
                elif earlier and not base:
                    notes.append('UNDOES? %s: no --base, so whether %s '
                                 'undoes %s cannot be told' % (
                                     rel, name, earlier))
            written[rel] = result
            carried[rel] = name
        for rel in deletions.get(name, []):
            if current(rel, written) is None:
                notes.append('DELETE %s: not in the tree' % rel)
            written[rel] = None
        removed = len(deletions.get(name, []))
        print('  %-40s %3d file%s: %d new, %d changed (%d merged), %d the '
              'same%s' % (name, len(files), '' if len(files) == 1 else 's',
                          added, changed, merged, same,
                          ('; %d deletion%s' % (removed, '' if removed == 1
                                                else 's')) if removed else ''))
        for note in notes:
            print('    ' + note)
        plan.append((name, message, sorted(files), deletions.get(name, [])))

    if options.gates:
        failed = gates(written, head) or failed

    if options.apply and not failed:
        for name, message, rels, removed in plan:
            for rel in rels:
                if rel in written and written[rel] is not None:
                    full = os.path.join(TREE, rel)
                    os.makedirs(os.path.dirname(full), exist_ok=True)
                    with open(full, 'wb') as handle:
                        handle.write(written[rel])
            for rel in removed:
                full = os.path.join(TREE, rel)
                if os.path.isfile(full):
                    os.remove(full)
            if options.commit:
                # the paths go through standard input: a zip of several
                # hundred files passes Windows' limit on a command line
                listed = ''.join(path + '\0' for path in rels + removed)
                result = subprocess.run(['git', '--literal-pathspecs', 'add',
                                         '-A', '--pathspec-from-file=-',
                                         '--pathspec-file-nul'], cwd=TREE,
                                        input=listed.encode('utf-8'),
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
                if result.returncode != 0:
                    raise RuntimeError('git add: %s' % result.stderr.decode(
                        'utf-8', 'replace').strip())
                staged = subprocess.run(['git', 'diff', '--cached', '--quiet'],
                                        cwd=TREE)
                if staged.returncode == 0:
                    print('  %s: nothing to commit' % name)
                    continue
                git(['commit', '-q', '-F', os.path.abspath(message)])
                print('  committed %s: %s' % (
                    name, git(['log', '-1', '--format=%h %s']).strip()))
        print('applied')
    elif options.apply:
        print('not applied: resolve the lines above first')

    print('merge window: %s' % ('NOT READY' if failed else 'ready'))
    return 1 if failed else 0


def gates(written, head):
    """Builds the window's result in a scratch checkout of HEAD and runs the
    checks over it. Returns whether anything failed."""
    scratch = tempfile.mkdtemp(prefix='merge_window_gates_')
    checkout = os.path.join(scratch, 'tree')
    git(['worktree', 'add', '--detach', '-f', checkout, head])
    try:
        prefix = git(['rev-parse', '--show-prefix']).strip()
        root = os.path.join(checkout, prefix)
        for rel, data in written.items():
            full = os.path.join(root, rel)
            if data is None:
                if os.path.isfile(full):
                    os.remove(full)
                continue
            os.makedirs(os.path.dirname(full), exist_ok=True)
            with open(full, 'wb') as handle:
                handle.write(data)
        # the checks beside this script, over the scratch tree, each held to
        # the punch list the window's result has (or, failing that, this
        # tree's): the ladder and the style over the changed files, the
        # fixed-width includes, the roots' growth, and the version
        here = os.path.dirname(os.path.abspath(__file__))

        def known(name):
            mine = os.path.join(root, 'scripts', 'ci', name)
            mine = mine if os.path.isfile(mine) else os.path.join(here, name)
            return ['--known', mine] if os.path.isfile(mine) else []

        def script(name):
            return [sys.executable, '-I', os.path.join(here, name),
                    '--tree', root]

        checks = [
            ('the ladder over the changed units',
             script('ladder.py') + ['--changed', 'HEAD'] +
             known('ladder_known.txt')),
            ('the style of the changed files',
             script('check_style.py') + ['--changed', 'HEAD'] +
             known('style_known.txt')),
            ('the fixed-width includes',
             script('check_stdint_includes.py') +
             known('stdint_known.txt')),
            ('the roots only grow',
             script('check_root_growth.py') + ['--base', 'HEAD']),
            ('the version', script('check_version.py')),
        ]
        failed = False
        for title, command in checks:
            print('gates: %s' % title)
            result = subprocess.run(command, cwd=root, stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    universal_newlines=True)
            shown = 0
            for line in result.stdout.splitlines():
                if line.startswith(('FAIL', 'FIXED', 'SKIP', 'ladder:',
                                    'djinterp ladder', 'failing units',
                                    'check_')):
                    print('  ' + line)
                    shown += 1
            if result.returncode == 2 or not shown:
                for line in result.stderr.strip().splitlines()[-5:]:
                    print('  %s' % line)
            failed = failed or result.returncode != 0
        return failed
    finally:
        git(['worktree', 'remove', '--force', checkout], check=False)
        shutil.rmtree(scratch, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
