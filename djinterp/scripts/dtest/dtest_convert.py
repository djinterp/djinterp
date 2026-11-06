#!/usr/bin/env python3
# =============================================================================
# djinterp [test]  --  scripts/dtest/dtest_convert.py
#
#   Converts C test sources in DTest's old forms to its frozen C face,
# c/modules/test/test_suite.h (docs/djinterp_testing.md, the migration
# guide). Three old forms:
#
#   bridge   the dtest_* C ABI the filesys suite was written against
#            (dtest_suite, dtest_module, dtest_assert_expr, ...): a rename,
#            complete.
#   sa       the standalone suites (*_tests_sa*.c), in either of their two
#            styles: counter (bool f(struct dj_test_counter*), checks through
#            d_assert_standalone) and tree (struct dj_test_object* f(void),
#            leaves through D_ASSERT_*). Each function becomes a section
#            function over a struct dj_test_suite*, each check a
#            D_TEST_CHECK_MSG, and each group a test.
#   program  a standalone program under tests/unintegrated: main() becomes a
#            spec test, and a spec row for it is printed to paste into the
#            suite header.
#   paths    not a form but its commonest breakage: each quoted #include that
#            does not resolve is pointed at the file the restructure moved it
#            to, found by the longest tail of its path that names exactly one
#            file under inc/ or tests/ (c/dmemory.h -> c/memory/dmemory.h,
#            test/c/X -> c/modules/test/X, core/X -> cpp/X, ...). An include
#            with no single match is marked.
#
#   What the converter cannot decide it leaves in place and marks with a
# "DTEST-CONVERT:" comment; --report lists every mark, and a file with marks
# is not done until each is resolved by hand. The conversion is idempotent:
# a converted file converts to itself.
#
# usage:
#   dtest_convert.py --from {bridge,sa,program} [--in-place | --out DIR]
#                    [--report] FILE...
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.05
# =============================================================================
import argparse
import os
import re
import sys

MARK = 'DTEST-CONVERT:'
SUITE_H = 'djinterp/c/modules/test/test_suite.h'


# -----------------------------------------------------------------------------
# 1. helpers
# -----------------------------------------------------------------------------

def split_call(text, start):
    """Given text and the index of a call's '(' return (args, end) where args
    are the top-level comma-separated arguments (stripped) and end is the
    index just past the matching ')'. Strings, chars and comments are
    respected. Returns (None, start) when the parentheses do not close."""
    depth = 0
    i = start
    args = []
    cur = start + 1
    n = len(text)
    while i < n:
        c = text[i]
        if c in '"\'':
            q = c
            i += 1
            while i < n and text[i] != q:
                if text[i] == '\\':
                    i += 1
                i += 1
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            i = n if j < 0 else j + 1
        elif text.startswith('//', i):
            j = text.find('\n', i)
            i = n if j < 0 else j
        elif c in '([{':
            depth += 1
        elif c in ')]}':
            depth -= 1
            if depth == 0:
                args.append(text[cur:i].strip())
                return args, i + 1
        elif c == ',' and depth == 1:
            args.append(text[cur:i].strip())
            cur = i + 1
        i += 1
    return None, start


def rewrite_calls(text, name, fn):
    """Replace every call `name(...)` with fn(args, column) (a string), or
    leave it when fn returns None. The column is where the call begins."""
    out = []
    pos = 0
    pat = re.compile(r'\b' + re.escape(name) + r'\s*\(')
    while True:
        m = pat.search(text, pos)
        if not m:
            break
        args, end = split_call(text, m.end() - 1)
        if args is None:
            break
        # the call's own column: arguments align under its first
        indent = m.start() - (text.rfind('\n', 0, m.start()) + 1)
        repl = fn(args, indent)
        if repl is None:
            out.append(text[pos:end])
        else:
            out.append(text[pos:m.start()])
            out.append(repl)
        pos = end
    out.append(text[pos:])
    return ''.join(out)


def rel_include(path):
    """The relative include of test_suite.h from a file under tests/ or
    build/, found by walking up to the directory that holds inc/."""
    d = os.path.dirname(os.path.abspath(path))
    up = 0
    while d and d != os.path.dirname(d):
        if os.path.isdir(os.path.join(d, 'inc', 'djinterp')):
            return '../' * up + 'inc/' + SUITE_H
        d = os.path.dirname(d)
        up += 1
    return SUITE_H


def one_line(s):
    return re.sub(r'\s+', ' ', s).strip()


def check_msg(indent, args):
    """D_TEST_CHECK_MSG(_suite, name, cond, msg), one argument a line, aligned
    under the first, as the guide lays out a call that does not fit one line;
    on one line when it fits in 80 columns."""
    flat = 'D_TEST_CHECK_MSG(_suite, %s)' % ', '.join(one_line(a) for a in args)
    if indent + len(flat) + 1 <= 80:
        return flat
    pad = '\n' + ' ' * (indent + len('D_TEST_CHECK_MSG('))
    return 'D_TEST_CHECK_MSG(_suite,' + pad + (',' + pad).join(
        one_line(a) for a in args) + ')'


def column_of(text, index):
    """The column of text[index] on its line."""
    return index - (text.rfind('\n', 0, index) + 1)


# -----------------------------------------------------------------------------
# 2. bridge: dtest_* -> d_test_suite_*
# -----------------------------------------------------------------------------

BRIDGE_NAMES = [
    (r'\bdtest_suite\b', 'struct dj_test_suite'),
    (r'\bdtest_summary\b', 'struct dj_test_summary'),
    (r'\bdtest_create\s*\(', 'd_test_suite_new('),
    (r'\bdtest_destroy\s*\(', 'd_test_suite_free('),
    (r'\bdtest_module\s*\(', 'd_test_suite_module('),
    (r'\bdtest_block\s*\(', 'd_test_suite_block('),
    (r'\bdtest_test\s*\(', 'd_test_suite_test('),
    (r'\bdtest_assert_expr\s*\(', 'd_test_suite_check('),
    (r'\bdtest_run\s*\(', 'd_test_suite_run('),
    (r'\bDTEST_SHOW_ALL\b', 'D_TEST_SUITE_SHOW_TESTS'),
    (r'\bDTEST_SHOW_FAILURES_ONLY\b', 'D_TEST_SUITE_SHOW_FAILURES'),
]


def convert_bridge(text, path, marks):
    # the stand-in header goes; test_suite.h comes in its place
    text = re.sub(r'#include\s+"[^"]*_dtest\.h"[^\n]*',
                  '#include "%s"  // DTest\'s C face' % rel_include(path),
                  text)
    text = re.sub(r'#include\s+"djinterp/test/dtest\.h"[^\n]*',
                  '#include "%s"  // DTest\'s C face' % rel_include(path),
                  text)
    # struct dj_test_suite* written as `struct struct ...` by a typedef'd use
    for pat, rep in BRIDGE_NAMES:
        text = re.sub(pat, rep, text)
    text = text.replace('struct struct ', 'struct ')

    def console(args, indent):
        return ('/* %s set the detail through d_test_suite_set_options: '
                'show %s */ ((void)0)' % (MARK, ', '.join(args)))
    text = rewrite_calls(text, 'dtest_use_console', console)
    return text


# -----------------------------------------------------------------------------
# 3. sa: the standalone suites
# -----------------------------------------------------------------------------

ASSERT_FORMS = {
    # name: (arity, condition builder from the middle arguments)
    'D_ASSERT_TRUE': (3, lambda a: a[0]),
    'D_ASSERT_FALSE': (3, lambda a: '!(%s)' % a[0]),
    'D_ASSERT_NULL': (3, lambda a: '(%s) == NULL' % a[0]),
    'D_ASSERT_NOT_NULL': (3, lambda a: '(%s) != NULL' % a[0]),
    'D_ASSERT_EQUAL': (4, lambda a: '(%s) == (%s)' % (a[0], a[1])),
    'D_ASSERT_STR_EQUAL': (4, lambda a: 'd_test_suite_str_equal((%s), (%s))'
                                        % (a[0], a[1])),
}


def convert_sa_checks(text, marks):
    """d_assert_standalone(cond, name, msg, counter) and the D_ASSERT_* leaf
    macros become D_TEST_CHECK_MSG(_suite, name, cond, msg)."""
    # a leaf stored into its group: the store goes first, so the check that
    # replaces the leaf is laid out from the column it will stand at
    text = re.sub(r'\b\w+\s*->\s*elements\s*\[\s*\w+\s*\+\+\s*\]\s*=\s*'
                  r'(D_ASSERT_[A-Z_]+\s*\()', r'\1', text)
    def standalone(args, indent):
        if len(args) != 4:
            return None
        return check_msg(indent, [args[1], args[0], args[2]])
    text = rewrite_calls(text, 'd_assert_standalone', standalone)

    for macro, (arity, cond) in ASSERT_FORMS.items():
        def leaf(args, indent, arity=arity, cond=cond):
            if len(args) != arity:
                return None
            return check_msg(indent, [args[0], cond(args[1:-1]), args[-1]])
        text = rewrite_calls(text, macro, leaf)

    return text


def convert_sa_counter(text, marks):
    # bool f(struct dj_test_counter* _counter) -> over the suite
    sig = re.compile(r'\bstruct\s+dj_test_counter\s*\*\s*(_\w+)')
    params = set(sig.findall(text))
    text = sig.sub('struct dj_test_suite* _suite', text)
    for p in params:
        if p != '_suite':
            text = re.sub(r'\b%s\b' % re.escape(p), '_suite', text)
    # the counters' own bookkeeping goes: the suite counts for itself
    text = re.sub(r'\n[ \t]*_suite\s*->\s*(tests|assertions)_(total|passed)'
                  r'\s*(\+\+|\+=[^;]*|=[^;]*);[^\n]*', '', text)
    return text


def convert_sa_tree(text, marks):
    """struct dj_test_object* f(void) building a group -> a section that opens
    a test named after the group."""
    # the function's own signature
    text = re.sub(r'\bstruct\s+dj_test_object\s*\*\s*(d_tests_\w+)\s*\(\s*'
                  r'void\s*\)',
                  r'bool \1(struct dj_test_suite* _suite)', text)
    # the group: a test of the same name
    text = re.sub(r'(\w+)\s*=\s*d_test_object_new_interior\s*\(\s*("[^"]*")'
                  r'\s*,[^;]*\);',
                  r'd_test_suite_test(_suite, \2);', text)
    # a child group stored into its parent: a call over the same suite
    text = re.sub(r'\b\w+\s*->\s*elements\s*\[\s*\w+\s*\+\+\s*\]\s*=\s*'
                  r'(d_tests_\w+)\s*\(\s*\)\s*;',
                  r'\1(_suite);', text)
    # the group's declaration, its allocation guard and its return go: the
    # suite holds the test now
    text = re.sub(r'\n[ \t]*struct\s+dj_test_object\s*\*\s*group\s*;', '',
                  text)
    text = re.sub(r'\n[ \t]*\n([ \t]*)if\s*\(\s*!\s*group\s*\)\s*\{\s*'
                  r'return\s+NULL\s*;\s*\}\n', '\n', text)
    text = re.sub(r'\n([ \t]*)if\s*\(\s*!\s*group\s*\)\s*\{\s*return\s+'
                  r'NULL\s*;\s*\}\n', '\n', text)
    text = re.sub(r'\breturn\s+group\s*;', 'return true;', text)
    # a group of groups (an *_all aggregator) is a block, not a test: its
    # checks are its children's
    parts = text.split('\n}\n')
    for i, part in enumerate(parts):
        if re.search(r'\bd_tests_\w+\s*\(\s*_suite\s*\)\s*;', part):
            parts[i] = re.sub(r'\bd_test_suite_test\(_suite,',
                              'd_test_suite_block(_suite,', part, count=1)
    text = '\n}\n'.join(parts)
    # the child index, once nothing indexes with it
    for idx in ('child_idx', 'idx'):
        if not re.search(r'\[\s*%s\s*\+\+\s*\]' % idx, text):
            text = re.sub(r'\n[ \t]*size_t\s+%s\s*;' % idx, '', text)
            text = re.sub(r'\n[ \t]*%s\s*=\s*0\s*;' % idx, '', text)
    # a body left opening on a blank line, once its declarations went
    text = re.sub(r'\{\n\n([ \t]*d_test_suite_(?:test|block)\()', r'{\n\1',
                  text)
    return text


def convert_sa(text, path, marks):
    text = re.sub(r'#include\s+"[^"]*test_standalone\.h"[^\n]*',
                  '#include "%s"  // DTest\'s C face' % rel_include(path),
                  text)
    text = re.sub(r'#include\s+"[^"]*test_counter\.h"[^\n]*',
                  '#include "%s"  // DTest\'s C face' % rel_include(path),
                  text)
    text = convert_sa_checks(text, marks)
    text = convert_sa_counter(text, marks)
    text = convert_sa_tree(text, marks)
    return text


# -----------------------------------------------------------------------------
# 4. program: a tests/unintegrated program
# -----------------------------------------------------------------------------

def convert_program(text, path, marks):
    stem = re.sub(r'\W', '_', os.path.splitext(os.path.basename(path))[0])
    name = 'd_tests_%s_main' % re.sub(r'^t_', '', stem)
    m = re.search(r'\bint\s+main\s*\(\s*(void)?\s*\)', text)
    m2 = re.search(r'\bint\s*\n\s*main\s*\(\s*(void)?\s*\)', text)
    m = m or m2
    if not m:
        marks.append((path, 0, 'no main(void) found'))
        return text
    text = text[:m.start()] + ('static int\n%s_program(void)' % name) + \
        text[m.end():]
    text += ('\n\n/*\n%s\n  The program, run as one spec test: it passes when '
             'the program would\nhave exited with 0.\n*/\nbool\n%s(void)\n{\n'
             '    D_TEST_REQUIRE(%s_program() == 0);\n\n    return true;\n}\n'
             % (name, name, name))
    if SUITE_H not in text:
        inc = '#include "%s"  // DTest\'s C face\n' % rel_include(path)
        text = re.sub(r'(\n#include[^\n]*\n)',
                      lambda m: m.group(1) + inc, text, count=1)
    print('  spec row for %s:\n    { "%s", "the %s program exits with 0", '
          '%s },' % (path, name, stem, name))
    return text


# -----------------------------------------------------------------------------
# 5. paths: includes the restructure moved
# -----------------------------------------------------------------------------

# the renames whose new tail is not the old one's (applied before the search)
MOVED = [
    ('djinterp/c/dmemory.h', 'djinterp/c/memory/dmemory.h'),
    ('djinterp/test/c/default/', 'djinterp/c/modules/test/default/'),
    ('djinterp/test/c/', 'djinterp/c/modules/test/'),
    ('djinterp/c/test/', 'djinterp/c/modules/test/'),
    ('djinterp/test/', 'djinterp/cpp/modules/test/'),
    ('djinterp/core/', 'djinterp/cpp/'),
    ('tests/djinterp/test/', 'tests/djinterp/cpp/modules/test/'),
]
_INDEX = None


def repo_root(path):
    d = os.path.dirname(os.path.abspath(path))
    while d and d != os.path.dirname(d):
        if os.path.isdir(os.path.join(d, 'inc', 'djinterp')):
            return d
        d = os.path.dirname(d)
    return None


def file_index(root):
    global _INDEX
    if _INDEX is None:
        _INDEX = []
        for top in ('inc', 'tests'):
            for dp, dn, fn in os.walk(os.path.join(root, top)):
                for f in fn:
                    _INDEX.append(os.path.relpath(os.path.join(dp, f), root))
    return _INDEX


def resolve_moved(root, target):
    """The one file under inc/ or tests/ that the moved include names."""
    t = target.replace('\\', '/')
    while t.startswith('../') or t.startswith('./'):
        t = t[3:] if t.startswith('../') else t[2:]
    for old, new in MOVED:
        if old in t:
            t = t.replace(old, new, 1)
            break
    parts = t.split('/')
    index = file_index(root)
    for k in range(len(parts), 0, -1):
        tail = '/'.join(parts[-k:])
        hits = [f for f in index if f == tail or f.endswith('/' + tail)]
        if len(hits) == 1:
            return hits[0]
        if len(hits) > 1 and k == len(parts):
            break
    return None


def convert_paths(text, path, marks):
    root = repo_root(path)
    if root is None:
        return text
    here = os.path.dirname(os.path.abspath(path))

    def fix(m):
        target = m.group(2)
        # resolved beside the file, or from the include root (-Iinc), which
        # is how the suites spell the framework's headers
        if ( os.path.exists(os.path.join(here, target)) or
             os.path.exists(os.path.join(root, 'inc', target)) ):
            return m.group(0)
        # a bare name resolves through the leaf's INCLUDES, as the guide
        # has the header under test included: left alone
        if '/' not in target:
            return m.group(0)
        found = resolve_moved(root, target)
        if found is None:
            if target.startswith('.'):
                marks.append((path, text.count('\n', 0, m.start()) + 1,
                              'no single file for #include "%s"' % target))
            return m.group(0)
        # an include spelled from the include root stays in that form
        if ( (not target.startswith('.')) and
             (found.startswith('inc/')) ):
            return '%s"%s"' % (m.group(1), found[len('inc/'):])
        rel = os.path.relpath(os.path.join(root, found), here)
        rel = rel.replace(os.sep, '/')
        if not rel.startswith('.'):
            rel = './' + rel
        return '%s"%s"' % (m.group(1), rel)
    # directives only: an include quoted in a comment is left as written
    return re.sub(r'^([ \t]*#\s*include\s+)"([^"]+)"', fix, text,
                  flags=re.M)


# -----------------------------------------------------------------------------
# 6. marks and driver
# -----------------------------------------------------------------------------

LEFTOVERS = [
    (r'\bdj_test_counter\b', 'a test counter is still used'),
    (r'\bdj_test_object\b', 'a standalone test object is still used'),
    (r'\bd_test_object_\w+', 'a standalone object call is left'),
    (r'\bd_test_counter_\w+', 'a counter call is left'),
    (r'\bdtest_\w+', 'a bridge name is left'),
    (r'\bD_ASSERT_\w+', 'a standalone assertion is left'),
    (r'\breturn\s+group\s*;', 'a group is still returned'),
]


def mark_leftovers(text, path, marks):
    lines = text.split('\n')
    for i, line in enumerate(lines):
        if MARK in line:
            marks.append((path, i + 1, line.strip()))
            continue
        for pat, why in LEFTOVERS:
            if re.search(pat, line) and not line.lstrip().startswith(('//',
                                                                      '*')):
                marks.append((path, i + 1, why + ': ' + one_line(line)))
                break


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--from', dest='form', required=True,
                    choices=('bridge', 'sa', 'program', 'paths'))
    g = ap.add_mutually_exclusive_group()
    g.add_argument('--in-place', action='store_true')
    g.add_argument('--out')
    ap.add_argument('--report', action='store_true')
    ap.add_argument('files', nargs='+')
    a = ap.parse_args()

    conv = {'bridge': convert_bridge, 'sa': convert_sa,
            'program': convert_program, 'paths': convert_paths}[a.form]
    marks = []
    for path in a.files:
        raw = open(path, 'rb').read()
        crlf = b'\r\n' in raw
        text = raw.decode('utf-8', errors='surrogateescape').replace('\r\n',
                                                                     '\n')
        new = conv(text, path, marks)
        if a.form != 'paths':
            mark_leftovers(new, path, marks)
        data = new.replace('\n', '\r\n') if crlf else new
        data = data.encode('utf-8', errors='surrogateescape')
        if a.in_place:
            open(path, 'wb').write(data)
        elif a.out:
            dst = os.path.join(a.out, path)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            open(dst, 'wb').write(data)
        else:
            sys.stdout.buffer.write(data)
    if a.report or a.in_place or a.out:
        for path, line, why in marks:
            print('%s:%d: %s %s' % (path, line, MARK, why), file=sys.stderr)
        print('%d file(s), %d mark(s) to resolve by hand'
              % (len(a.files), len(marks)), file=sys.stderr)
    return 1 if marks else 0


if __name__ == '__main__':
    sys.exit(main())
