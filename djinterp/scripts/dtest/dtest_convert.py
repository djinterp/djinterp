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
#   section  the dj_tests_section tables (tests/.../parse/tests_section.h)
#            the parse, parsegen and command-line-kit suites list their
#            sections in: each table becomes a table of dj_test_case_spec
#            rows (a descriptor to write per row, marked once per table),
#            gains a dj_test_block_spec beside it, and keeps its #if lines;
#            each extern follows, and the C-face runner over every table of
#            the run is printed to merge into the suite's runner.
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
#   --final (with --from sa --in-place) gives a converted suite its final
# names: d_tests_sa_ becomes d_tests_, and X_tests_sa*.{c,h} X_tests*.{c,h},
# in the code, the includes, the guards and the banners; each file is renamed
# when its new name is free, and one whose new name a C-face suite already
# has is reported, to merge by hand. A name a register test already has, a
# runner and a CMake leaf are the suite's own work (docs/djinterp_testing.md,
# the migration guide).
#
# usage:
#   dtest_convert.py --from {bridge,sa,program,section,paths}
#                    [--in-place | --out DIR]
#                    [--report] [--final] FILE...
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
        # a macro's own definition is not a call of it
        line_start = text.rfind('\n', 0, m.start()) + 1
        if re.match(r'\s*#\s*define\s*$', text[line_start:m.start()]):
            out.append(text[pos:end])
            pos = end
            continue
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
    """An argument on one line: each run of whitespace outside string and
    character literals becomes one space; a literal is kept as it is, since
    its spaces are its value."""
    out = []
    for i, part in enumerate(re.split(r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')',
                                      s)):
        out.append(part if i % 2 else re.sub(r'\s+', ' ', part))
    return ''.join(out).strip()


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
    text = re.sub(r'\b\w+(?:\s*->\s*elements)?\s*\[\s*\w+\s*\+\+\s*\]\s*=\s*'
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
    # the groups' variables, whatever their names (group, root, ...)
    groups = set(re.findall(r'(\w+)\s*=\s*d_test_object_new_interior\s*\(',
                            text)) | {'group'}
    # the group: a test of the same name
    text = re.sub(r'(\w+)\s*=\s*d_test_object_new_interior\s*\(\s*("[^"]*")'
                  r'\s*,[^;]*\);',
                  r'd_test_suite_test(_suite, \2);', text)
    # a child group stored into its parent: a call over the same suite
    text = re.sub(r'\b\w+\s*->\s*elements\s*\[\s*\w+\s*\+\+\s*\]\s*=\s*'
                  r'(d_tests_\w+)\s*\(\s*\)\s*;',
                  r'\1(_suite);', text)
    # each group's declaration, its allocation guard (and the comment that
    # leads it) and its return go: the suite holds the test now, and a guard
    # left behind would read a pointer nothing sets
    for g in sorted(groups):
        text = re.sub(r'\n[ \t]*struct\s+dj_test_object\s*\*\s*%s\s*;' % g,
                      '', text)
        guard = (r'(?:\n[ \t]*//[^\n]*)*\n[ \t]*if\s*\(\s*!\s*%s\s*\)\s*'
                 r'\{[^{}]*\}\n' % g)
        text = re.sub(r'\n[ \t]*\n' + guard[:-2] + r'\n', '\n', text)
        text = re.sub(guard, '\n', text)
        text = re.sub(r'\breturn\s+%s\s*;' % g, 'return true;', text)
    # slots filled with failures when an earlier step failed: one failing
    # check says it (the loop counted on the store the conversion removed)
    def fill(m):
        call = m.group(2)
        args, _ = split_call(call, call.index('('))
        if not args or len(args) != 4:
            return m.group(0)
        return m.group(1) + check_msg(len(m.group(1)), args[1:]) + ';\n'
    text = re.sub(r'([ \t]*)while\s*\(\s*(?:child_idx|idx)\s*<\s*\w+\s*\)\s*'
                  r'\{\s*(D_TEST_CHECK_MSG\((?:[^;]|"[^"]*")*\));\s*\}\n',
                  fill, text)
    # a converted section returns false where its group gave up with NULL
    out, pos = [], 0
    for name, open_, close in sections(text):
        out.append(text[pos:open_])
        out.append(re.sub(r'\breturn\s+NULL\s*;', 'return false;',
                          text[open_:close]))
        pos = close
    out.append(text[pos:])
    text = ''.join(out)
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


def match_brace(text, i):
    """Given text[i] == '{', the index of its matching '}', skipping strings,
    character constants and comments; -1 when it does not close."""
    depth = 0
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
        elif c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


SECTION_DEF = re.compile(r'^(?:static\s+)?bool\s+(d_tests?_\w+)\s*\(\s*struct\s+'
                         r'dj_test_suite\s*\*\s*_suite\s*\)\s*\{', re.M)


def sections(text):
    """Each section function's (name, index of its '{', index of its '}')."""
    out = []
    for m in SECTION_DEF.finditer(text):
        open_ = m.end() - 1
        close = match_brace(text, open_)
        if close > 0:
            out.append((m.group(1), open_, close))
    return out


def label_of(name):
    """A section's test or block name: its function name without the
    suite prefix (d_tests_, d_tests_sa_, d_test_)."""
    return re.sub(r'^d_tests?_(sa_)?', '', name)


STATEMENT = re.compile(r'^\s*(if|for|while|do|switch|return|goto|break|'
                       r'continue|else)\b')
DECLARATION = re.compile(r'^\s*(?:(?:static|const|volatile|unsigned|signed|'
                         r'struct|enum|union|register)\s+)*[A-Za-z_]\w*'
                         r'(?:\s+|\s*\*+\s*)\**[A-Za-z_]\w*\s*'
                         r'(?:\[[^\]]*\]\s*)*(?:[=;,]|$)')


def body_start(text, open_, close):
    """Where a section's first statement begins: after its declarations, and
    after an `if (!_suite)` guard that leads the body."""
    i = text.find('\n', open_) + 1
    while 0 < i < close:
        end = text.find('\n', i)
        end = close if end < 0 or end > close else end
        line = text[i:end]
        s = line.strip()
        if (not s) or s.startswith(('//', '/*', '*', '#')):
            i = end + 1
            continue
        if DECLARATION.match(line) and not STATEMENT.match(line):
            # a declaration may run over several lines, a struct's members
            # or an initializer's braces among them: to its own ';'
            j = i
            depth = 0
            while j < close:
                c = text[j]
                if c in '{([':
                    depth += 1
                elif c in '})]':
                    depth -= 1
                elif c == ';' and depth == 0:
                    break
                elif c in '"\'':
                    q = c
                    j += 1
                    while j < close and text[j] != q:
                        j += 2 if text[j] == '\\' else 1
                j += 1
            i = text.find('\n', j) + 1 if j < close else close
            continue
        guard = re.match(r'[ \t]*if\s*\(\s*(!\s*_suite|_suite\s*==\s*NULL)\s*\)'
                         r'\s*\{\s*return\s+false\s*;\s*\}\s*\n', text[i:])
        if guard:
            i += guard.end()
            while text.startswith('\n', i):
                i += 1
            return i
        return i
    return i


def convert_sa_bookkeeping(text, marks):
    """What a counter-style test kept of its own count goes, since the suite
    keeps it: a snapshot of a count into a local, and a return that compares
    a count with one."""
    count = r'_suite\s*->\s*(?:tests|assertions)_(?:passed|total)'
    text = re.sub(r'\n[ \t]*(?:size_t\s+)?\w+\s*=\s*' + count + r'\s*;[^\n]*',
                  '', text)
    # a count raised under a condition: `if (ok) _suite->tests_passed++;`
    text = re.sub(r'\n[ \t]*if\s*\([^\n]*\)\s*' + count +
                  r'\s*(?:\+\+|\+=[^;]*|=[^;]*);[^\n]*', '', text)
    text = re.sub(r'\n[ \t]*if\s*\([^\n]*\)\s*\{\s*' + count +
                  r'\s*(?:\+\+|\+=[^;]*|=[^;]*);\s*\}[^\n]*', '', text)
    text = re.sub(r'\breturn\s*\(?\s*' + count + r'\s*[<>=!]=?\s*\w+\s*\)?\s*;',
                  'return true;', text)
    text = re.sub(r'\breturn\s*\(?\s*\w+\s*[<>=!]=?\s*' + count + r'\s*\)?\s*;',
                  'return true;', text)
    # a local that held a snapshot and is no longer read
    for name in set(re.findall(r'\n[ \t]*size_t\s+(initial_\w+|\w+_before|'
                               r'start_\w+)\s*;', text)):
        if len(re.findall(r'\b%s\b' % re.escape(name), text)) == 1:
            text = re.sub(r'\n[ \t]*size_t\s+%s\s*;[^\n]*' % re.escape(name),
                          '', text)
    return text


def convert_sa_aggregators(text, marks):
    """A section that keeps a counter of its own (struct dj_test_counter x)
    is an aggregator: it ran its children over that counter and printed what
    it counted. Its body becomes the calls it makes, in order, with any #if
    around them; the suite counts and reports. A report helper over a
    counter (now `const struct dj_test_suite*`) goes with them."""
    out = []
    pos = 0
    for name, open_, close in sections(text):
        body = text[open_ + 1:close]
        locals_ = re.findall(r'\bstruct\s+dj_test_counter\s+(\w+)', body)
        if not locals_:
            continue
        args = '|'.join(['_suite'] + [r'&\s*' + re.escape(v) for v in locals_])
        keep = []
        for line in body.split('\n'):
            s = line.strip()
            if re.match(r'#\s*(if|ifdef|ifndef|elif|else|endif)\b', s):
                keep.append(s)
                continue
            for m in re.finditer(r'\b(d_tests?_\w+)\s*\(\s*(?:%s)\s*\)' % args,
                                 line):
                keep.append('    %s(_suite);' % m.group(1))
            for m in re.finditer(r'\b(d_tests?_\w+_(?:setup|teardown|cleanup|'
                                 r'init))\s*\(\s*\)', line):
                keep.append('    (void)%s();' % m.group(1))
        # an #if left with nothing inside goes
        changed = True
        while changed:
            changed = False
            for k in range(len(keep) - 1):
                if (re.match(r'#\s*(if|ifdef|ifndef)\b', keep[k]) and
                        re.match(r'#\s*endif\b', keep[k + 1])):
                    del keep[k:k + 2]
                    changed = True
                    break
        new = '\n' + '\n'.join(keep) + ('\n\n' if keep else '') + \
              '    return true;\n'
        out.append(text[pos:open_ + 1])
        out.append(new)
        pos = close
    out.append(text[pos:])
    text = ''.join(out)
    # report helpers over a counter, definitions and prototypes
    helper = re.compile(r'\n(?:static\s+)?(?:void|bool|int)\s+(d_tests?_\w+)'
                        r'\s*\([^;{)]*const\s+struct\s+dj_test_suite\s*\*'
                        r'[^;{)]*\)\s*')
    gone = set()
    while True:
        m = helper.search(text)
        if not m:
            break
        gone.add(m.group(1))
        j = m.end()
        if text.startswith(';', j):
            text = text[:m.start()] + text[j + 1:]
            continue
        if not text.startswith('{', j):
            break
        close = match_brace(text, j)
        if close < 0:
            break
        # the doc comment right above it goes too
        start = m.start()
        doc = re.search(r'\n/\*[^*]?(?:[^*]|\*(?!/))*\*/\s*$', text[:start])
        if doc and m.group(1) in doc.group(0):
            start = doc.start()
        text = text[:start] + text[close + 1:]
    # and the calls to them, the summary printers a run_all kept
    for name in gone | REPORT_HELPERS:
        text = re.sub(r'\n[ \t]*(?:\(void\)\s*)?%s\s*\([^;]*\)\s*;[^\n]*'
                      % re.escape(name), '', text)
    REPORT_HELPERS.update(gone)
    return text


# REPORT_HELPERS
#   variable: the report helpers over a counter that the conversion removed,
# across the files of a run: a run_all in one file may call a printer that
# another file defined.
REPORT_HELPERS = set()

# GROUP_HELPERS
#   variable: the functions that build a standalone group from an array of
# results (a `struct dj_test_object**` parameter), across the files of a run:
# the suite builds the tree now, so they go, prototypes and doc comments too.
GROUP_HELPERS = set()


def remove_functions(text, names):
    """Each prototype and definition of the named functions, with the doc
    comment right above a definition, removed."""
    for name in names:
        decl = re.compile(r'\n(?:static\s+)?(?:const\s+)?(?:struct\s+)?\w+'
                          r'(?:\s*\*+\s*|\s+)%s\s*\([^;{]*\)\s*' % re.escape(name))
        while True:
            m = decl.search(text)
            if not m:
                break
            j = m.end()
            if text.startswith(';', j):
                text = text[:m.start()] + text[j + 1:]
                continue
            if not text.startswith('{', j):
                break
            close = match_brace(text, j)
            if close < 0:
                break
            start = m.start()
            doc = re.search(r'\n/\*(?:[^*]|\*(?!/))*\*/\s*$', text[:start])
            if doc and name in doc.group(0):
                start = doc.start()
            text = text[:start] + text[close + 1:]
        # a header's one-line note naming it
        text = re.sub(r'\n// %s\n(?://[^\n]*\n)*' % re.escape(name), '\n', text)
    return text


# CHECKERS
#   variable: the sections that check something, across every file of a run
# (filled by main before converting): a section that only calls others opens
# a block when one of them checks, and nothing when it only dispatches to
# other blocks, so that no empty heading is left.
CHECKERS = set()


def find_checkers(text):
    """The functions of a standalone source that check: each definition whose
    body asserts, in either style, or opens a group."""
    found = set()
    for m in re.finditer(r'^(?:static\s+)?(?:bool|struct\s+dj_test_object\s*\*)'
                         r'\s*(d_tests?_\w+)\s*\([^;{]*\)\s*\{', text, re.M):
        close = match_brace(text, m.end() - 1)
        body = text[m.end():close] if close > 0 else ''
        if re.search(r'\b(d_assert_standalone|D_ASSERT_[A-Z_]+|'
                     r'D_TEST_CHECK(?:_MSG)?)\s*\(', body):
            found.add(m.group(1))
    return found


def open_call(what, label):
    """`    d_test_suite_test(_suite, label);` and a blank line, laid out as the
    guide lays out a call that does not fit 80 columns: the label under the
    first argument, and a conditional label broken before its ? and :."""
    flat = '    d_test_suite_%s(_suite, %s);' % (what, label)
    if len(flat) <= 80:
        return flat + '\n\n'
    pad = ' ' * len('    d_test_suite_%s(' % what)
    cond = re.match(r'(.+?)\s+\?\s+(.+?)\s+:\s+(.+)$', label)
    if cond:
        body = (cond.group(1) + '\n' + pad + '    ? ' + cond.group(2) + '\n' +
                pad + '    : ' + cond.group(3))
    else:
        body = label
    return '    d_test_suite_%s(_suite,\n%s%s);\n\n' % (what, pad, body)


def convert_sa_item_arrays(text, marks):
    """The tree style's other form: leaves and child groups stored into a
    local array (items[idx++] = ...), and the group made from it by a
    helper, `d_tests_x_group("name", items, idx)`. A child becomes a call
    over the suite, the helper's call goes, and its name opens the section:
    a test when the section checks, a block when it calls."""
    text = re.sub(r'\b\w+\s*\[\s*\w+\s*\+\+\s*\]\s*=\s*(d_tests?_\w+)\s*\(\s*'
                  r'\)\s*;', r'\1(_suite);', text)
    helper_re = re.compile(r'(?:\b(\w+)\s*=\s*|\breturn\s+)(d_tests?_\w*group'
                           r'\w*)\s*\(')

    def helper_call(body):
        """The first group-helper call in a body: (start, end, label, var),
        its name the first of its three arguments, whatever expression."""
        for m in helper_re.finditer(body):
            args, end = split_call(body, m.end() - 1)
            if not args or len(args) != 3:
                continue
            semi = re.match(r'\s*;', body[end:])
            if not semi:
                continue
            return m.start(), end + semi.end(), one_line(args[0]), m.group(1)
        return None

    while True:
        found = None
        for name, open_, close in sections(text):
            call = helper_call(text[open_ + 1:close])
            if call:
                found = (open_, close, call)
                break
        if not found:
            return text
        open_, close, (start, end, label, var) = found
        body = text[open_ + 1:close]
        if var:
            body = body[:start] + body[end:]
            body = re.sub(r'\breturn\s+%s\s*;' % re.escape(var),
                          'return true;', body)
            body = re.sub(r'\n[ \t]*struct\s+dj_test_object\s*\*\s*%s\s*;'
                          % re.escape(var), '', body)
        else:
            body = body[:start] + 'return true;' + body[end:]
        # the array and its index, once nothing stores into them
        for arr in re.findall(r'\bstruct\s+dj_test_object\s*\*\s*(\w+)\s*\[',
                              body):
            if not re.search(r'\b%s\b' % re.escape(arr),
                             re.sub(r'\bstruct\s+dj_test_object\s*\*\s*%s\s*'
                                    r'\[[^\]]*\]\s*;' % re.escape(arr), '',
                                    body)):
                body = re.sub(r'\n[ \t]*struct\s+dj_test_object\s*\*\s*%s\s*'
                              r'\[[^\]]*\]\s*;[^\n]*' % re.escape(arr), '', body)
        for idx in ('idx', 'child_idx'):
            if len(re.findall(r'\b%s\b' % idx, body)) and not re.search(
                    r'\b%s\b(?!\s*=\s*0\s*;)' % idx,
                    re.sub(r'\n[ \t]*size_t\s+%s\s*;' % idx, '', body)):
                body = re.sub(r'\n[ \t]*size_t\s+%s\s*;[^\n]*' % idx, '', body)
                body = re.sub(r'\n[ \t]*%s\s*=\s*0\s*;[^\n]*' % idx, '', body)
        what = 'test' if re.search(r'\bD_TEST_CHECK(?:_MSG)?\s*\(', body) \
            else 'block'
        text = text[:open_ + 1] + body + text[close:]
        close = open_ + 1 + len(body)
        at = body_start(text, open_, close)
        text = text[:at] + open_call(what, label) + text[at:]


def convert_sa_open_tests(text, marks):
    """Each section opens what it is: a test when it checks, a block when it
    calls sections that check, and nothing when it only dispatches to other
    blocks; one that already opens something is left as it is."""
    out = []
    pos = 0
    for name, open_, close in sections(text):
        body = text[open_ + 1:close]
        if re.search(r'\bd_test_suite_(test|block|module)\s*\(', body):
            continue
        calls = re.findall(r'\b(d_tests?_\w+)\s*\(\s*_suite\s*\)', body)
        checks = re.search(r'\bD_TEST_CHECK(?:_MSG)?\s*\(', body)
        if checks:
            what = 'test'
        elif any(c in CHECKERS for c in calls):
            what = 'block'
        else:
            continue
        at = body_start(text, open_, close)
        out.append(text[pos:at])
        out.append(open_call(what, '"%s"' % label_of(name)))
        pos = at
    out.append(text[pos:])
    return ''.join(out)


def ensure_suite_include(text, path):
    """A file that now names the suite includes the C face."""
    if 'test_suite.h' in text:
        return text
    if not re.search(r'\bstruct\s+dj_test_suite\b|\bD_TEST_CHECK', text):
        return text
    line = '#include "%s"  // DTest\'s C face' % rel_include(path)
    incs = list(re.finditer(r'^[ \t]*#\s*include\s*"[^"]*"[^\n]*\n', text,
                            re.M))
    if incs:
        at = incs[-1].end()
        return text[:at] + line + '\n' + text[at:]
    guard = re.search(r'^#define\s+\w+[^\n]*\n', text, re.M)
    at = guard.end() if guard else 0
    return text[:at] + '\n' + line + '\n' + text[at:]


def convert_sa(text, path, marks):
    text = re.sub(r'#include\s+"[^"]*test_standalone\.h"[^\n]*',
                  '#include "%s"  // DTest\'s C face' % rel_include(path),
                  text)
    text = re.sub(r'#include\s+"[^"]*test_counter\.h"[^\n]*',
                  '#include "%s"  // DTest\'s C face' % rel_include(path),
                  text)
    # one include of the C face, where both were included
    first = text.find('test_suite.h')
    if first >= 0:
        head, tail = text[:first + 1], text[first + 1:]
        tail = re.sub(r'\n[ \t]*#include\s+"[^"]*test_suite\.h"[^\n]*', '',
                      tail)
        text = head + tail
    text = remove_functions(text, GROUP_HELPERS)
    text = convert_sa_checks(text, marks)
    text = convert_sa_counter(text, marks)
    text = convert_sa_tree(text, marks)
    text = convert_sa_bookkeeping(text, marks)
    text = convert_sa_aggregators(text, marks)
    text = convert_sa_item_arrays(text, marks)
    text = convert_sa_open_tests(text, marks)
    text = convert_sa_verdicts(text)
    text = drop_dead_locals(text)
    text = ensure_suite_include(text, path)
    return text


# a progress line opens with D_INDENT's %s or with spaces of its own
LEAD = r'(?:%s|[ ]*)'
VERDICT = re.compile(r'(?:\n[ \t]*//[^\n]*)?\n[ \t]*if\s*\(\s*(\w+)\s*\)\s*\{'
                     r'\s*printf\s*\(\s*"' + LEAD + r'\[PASS\][^;]*;\s*\}\s*'
                     r'else\s*\{\s*printf\s*\(\s*"' + LEAD + r'\[FAIL\][^;]*;'
                     r'\s*\}[^\n]*')
PROGRESS = [
    # "--- Testing X ---"
    re.compile(r'\n[ \t]*printf\s*\(\s*"' + LEAD + r'---[^"]*---\\n"\s*'
               r'(?:,\s*D_INDENT\s*)?\)\s*;[^\n]*(?=\n)'),
    # "[SECTION] X" and the rule under it: the block heading says it
    re.compile(r'\n[ \t]*printf\s*\(\s*"(?:\\n)?' + LEAD + r'\[SECTION\][^"]*"'
               r'\s*\)\s*;[^\n]*(?:\n[ \t]*printf\s*\(\s*"' + LEAD +
               r'-+\\n"\s*\)\s*;[^\n]*)?(?=\n)'),
    # "Testing `x`..."
    re.compile(r'\n[ \t]*printf\s*\(\s*"' + LEAD + r'Testing [^"]*\.\.\.\\n"\s*'
               r'\)\s*;[^\n]*(?=\n)'),
]


def convert_sa_verdicts(text):
    """The tier's own progress lines, which the suite's report now gives: a
    section's "--- Testing X ---" banner, and its [PASS]/[FAIL] print, whose
    flag becomes what the section returns."""
    out, pos = [], 0
    for name, open_, close in sections(text):
        body = text[open_:close]
        for pattern in PROGRESS:
            body = pattern.sub('', body)
        m = VERDICT.search(body)
        if m:
            flag = m.group(1)
            body = body[:m.start()] + body[m.end():]
            tail = body.rstrip()
            ret = re.search(r'\breturn\s+true\s*;\s*$', tail[:-1] if
                            tail.endswith('}') else tail)
            if ret:
                k = body.rfind('return true;')
                body = body[:k] + 'return %s;' % flag + body[k + 12:]
        body = re.sub(r'\{\n\n+', '{\n', body)
        body = re.sub(r'\n\n\n+', '\n\n', body)
        out.append(text[pos:open_])
        out.append(body)
        pos = close
    out.append(text[pos:])
    return ''.join(out)


# a declaration, not a statement: `return true;` has the same shape
LOCAL = re.compile(r'\n([ \t]+)(?!(?:return|goto|case|else|do|break|continue)'
                   r'\b)(?:(?:const|unsigned|signed|struct)\s+)*\w+'
                   r'(?:\s+|\s*\*+\s*)(?!(?:true|false|NULL)\b)(\w+)\s*;'
                   r'[^\n]*')


def code_only(text):
    """The text with its comments and literals blanked, for counting uses;
    each keeps its length, so an index into one is an index into the other."""
    return re.sub(r'//[^\n]*|/\*(?:[^*]|\*(?!/))*\*/|"(?:[^"\\\n]|\\.)*"|'
                  r"'(?:[^'\\\n]|\\.)*'", lambda m: ' ' * len(m.group(0)),
                  text)


# the tier's own bookkeeping, which the conversion leaves: no macro pastes it
TIER_LOCALS = {'initial_tests_passed', 'initial_assertions_passed'}
# a name a macro may paste together is a use no search sees
PASTE = re.compile(r'##|\bD_(?:CONCAT|PASTE|CAT)\w*\s*\(')


def dead_local(body):
    """(declaration, its sets) of the first local nothing reads, or None:
    only the function's own declarations (a struct's members, declared
    deeper, are not locals), and none where a macro pastes names."""
    pasting = bool(PASTE.search(body))
    code = code_only(body)
    for m in LOCAL.finditer(body):
        if code[:m.start()].count('{') - code[:m.start()].count('}') != 1:
            continue
        var = m.group(2)
        if pasting and var not in TIER_LOCALS:
            continue
        rest = body[:m.start()] + body[m.end():]
        sets = list(re.finditer(r'(?:\n[ \t]*//[^\n]*)*\n([ \t]*)%s\s*'
                                r'(?:[-+*/%%&|^]|<<|>>)?=(?!=)\s*([^;]*);[^\n]*'
                                % var, rest))
        uses = len(re.findall(r'\b%s\b' % var, code_only(rest)))
        if uses == len(sets):
            return m, sets
    return None


def drop_dead_locals(text):
    """A local the conversion left with nothing to do: one never read again
    (a counter's snapshot, a group's size) goes with its assignments, the
    comment that leads one, and an #if left empty by them; an assignment
    whose value is a call keeps the call, as a (void) statement."""
    out, pos = [], 0
    for name, open_, close in sections(text):
        body = text[open_:close]
        while True:
            found = dead_local(body)
            if not found:
                break
            m, sets = found
            rest = body[:m.start()] + body[m.end():]
            for st in reversed(sets):
                rhs = st.group(2).strip()
                if re.search(r'\w\s*\(', re.sub(r'\bsizeof\s*\(', '(',
                                                   rhs)):
                    keep = '\n%s(void)%s;' % (st.group(1), rhs)
                    head = st.group(0)[:st.group(0).rfind('\n')]
                    rest = rest[:st.start()] + head + keep + rest[st.end():]
                else:
                    rest = rest[:st.start()] + rest[st.end():]
            body = rest
        # an #if the removal emptied, and the comment that led it
        body = re.sub(r'(?:\n[ \t]*//[^\n]*)*\n[ \t]*#\s*if[^\n]*'
                      r'(?:\n[ \t]*#\s*(?:else|elif)[^\n]*)*\n[ \t]*#\s*endif'
                      r'[^\n]*', '', body)
        body = re.sub(r'\{\n\n+', '{\n', body)
        body = re.sub(r'\n\n\n+', '\n\n', body)
        out.append(text[pos:open_])
        out.append(body)
        pos = close
    out.append(text[pos:])
    return ''.join(out)


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
# 4b. section: dj_tests_section tables
# -----------------------------------------------------------------------------

SECTION_TABLE = re.compile(r'^([ \t]*)((?:static\s+)?const\s+)(struct\s+)?'
                           r'dj_tests_section(\s+)(\w+)(\s*\[\s*\]\s*=\s*\n?\s*\{)',
                           re.M)
SECTION_EXTERN = re.compile(r'^([ \t]*)extern(\s+)const\s+(struct\s+)?'
                            r'dj_tests_section(\s+)(\w+)\s*\[\s*\]\s*;[^\n]*$', re.M)
SECTION_ROW = re.compile(r'\{(\s*)("(?:[^"\\]|\\.)*")\s*,(\s*)([^{}]+?)\s*\}')
SECTION_INCLUDE = re.compile(r'^#\s*include\s*"[^"]*tests_section\.h"[^\n]*$', re.M)
SECTION_TABLES = []


def block_name(table):
    """The block a table becomes, named as its sections are: the table's
    name without its d_tests_ prefix."""
    return re.sub(r'^d_?tests?_', '', table)


def convert_section_rows(body, indent):
    """Each row { "name", fn } of a table body, as a C-face row
    { "name", "", fn }, one field to a line; everything between rows (their
    commas, #if lines, comments) is kept."""
    out = []
    pos = 0
    rows = 0
    for m in SECTION_ROW.finditer(body):
        out.append(body[pos:m.start()])
        col = column_of(body, m.start())
        pad = ' ' * (col + 2)
        out.append('{ %s,\n%s"",\n%s%s }' % (m.group(2), pad, pad,
                                             m.group(4).strip()))
        pos = m.end()
        rows += 1
    out.append(body[pos:])
    return ''.join(out), rows


def convert_section(text, path, marks):
    """dj_tests_section tables to the C face's spec rows, in place: each
    table keeps its name, linkage and conditions, so the code that names it
    still does, and a block spec over it is defined beside it."""
    if 'dj_tests_section' not in text:
        return text
    out = []
    pos = 0
    for m in SECTION_TABLE.finditer(text):
        open_ = m.end() - 1
        close = match_brace(text, open_)
        if close < 0:
            marks.append((path, text.count('\n', 0, m.start()) + 1,
                          'a table whose braces do not close'))
            continue
        end = text.find(';', close)
        indent, quals, table = m.group(1), m.group(2), m.group(5)
        body, rows = convert_section_rows(text[open_ + 1:close], indent)
        is_static = 'static' in quals
        block = '%s_block' % table
        out.append(text[pos:m.start()])
        out.append('%s// %s write a descriptor for each of the %d rows below '
                   '(3.5)\n' % (indent, MARK, rows))
        out.append('%s%sstruct dj_test_case_spec%s%s%s' %
                   (indent, quals, m.group(4), table,
                    text[m.start(6):open_ + 1]))
        out.append(body)
        out.append(text[close:end + 1])
        out.append('\n\n%s// %s\n%s//   constant: %s as a block of the C face\'s '
                   'spec form.\n' % (indent, block, indent, table))
        out.append('%s%sconst struct dj_test_block_spec %s =\n%s{\n'
                   % (indent, 'static ' if is_static else '', block, indent))
        out.append('%s    "%s",\n%s    "",  // %s describe the block\n'
                   % (indent, block_name(table), indent, MARK))
        out.append('%s    %s,\n%s    sizeof(%s) / sizeof(%s[0])\n%s};'
                   % (indent, table, indent, table, table, indent))
        SECTION_TABLES.append((path, table, block, is_static))
        pos = end + 1
    out.append(text[pos:])
    text = ''.join(out)

    # the externs a header declares the tables by
    def extern(m):
        indent, table = m.group(1), m.group(5)
        return ('%sextern%sconst struct dj_test_case_spec%s%s[];\n'
                '%sextern%sconst struct dj_test_block_spec %s_block;'
                % (indent, m.group(2), m.group(4), table, indent, m.group(2),
                   table))
    text = SECTION_EXTERN.sub(extern, text)

    # tests_section.h gives way to the C face
    inc = '#include "%s"  // dj_test_case_spec' % rel_include(path)
    if SECTION_INCLUDE.search(text):
        text = SECTION_INCLUDE.sub(inc if SUITE_H not in text else '', text,
                                   count=1)
        text = re.sub(r'\n#\s*include\s*"[^"]*tests_section\.h"[^\n]*', '',
                      text)

    # a runner over the old tables walks .run and .name: it is replaced, not
    # converted (the printed runner below)
    for i, line in enumerate(text.split('\n')):
        if re.search(r'\bdj_tests_section\b', line):
            marks.append((path, i + 1, 'a use of dj_tests_section left: '
                          'a runner over the old tables is replaced by '
                          'the printed one'))
    return text


def print_section_runner():
    """The C-face runner over every block the run converted: blocks gathered
    into one module at run time, since another unit's const is not a
    constant expression in C."""
    blocks = [b for (_, _, b, st) in SECTION_TABLES if not st]
    if not blocks:
        return
    rows = ',\n'.join('        %s' % b for b in blocks)
    print('  runner for the %d converted block(s): its main, to merge with '
          'the suite\'s own setup:\n'
          '    const struct dj_test_block_spec  blocks[]  =\n    {\n%s\n    };\n'
          '    const struct dj_test_module_spec module    =\n'
          '        { "MODULE", "", blocks, sizeof(blocks) / sizeof(blocks[0]) };\n'
          '    const struct dj_test_module_spec* const modules[] = { &module };\n\n'
          '    return d_test_run_modules(_argc,\n'
          '                              _argv,\n'
          '                              "TITLE",\n'
          '                              modules,\n'
          '                              1u);' % (len(blocks), rows),
          file=sys.stderr)


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
    # D_ASSERT_SAME_SIZE is dmacro.h's own compile-time check, not the tier's
    (r'\bD_ASSERT_(?!SAME_SIZE\b)\w+', 'a standalone assertion is left'),
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


def final_path(path):
    d, b = os.path.split(path)
    return os.path.join(d, re.sub(r'_tests_sa(?=[_.])', '_tests', b))


def final_names(text, path):
    """d_tests_sa_ -> d_tests_, and the file's own names without _sa."""
    text = re.sub(r'\bd_tests_sa_', 'd_tests_', text)
    text = re.sub(r'(#\s*include\s*"[^"]*_tests)_sa(?=[_.][^"]*")', r'\1',
                  text)
    for g in re.findall(r'#ifndef\s+(\w*_SA\w*)\s*\n#define\s+\1\b', text):
        text = re.sub(r'\b%s\b' % g, re.sub(r'_SA(?=_|$)', '', g), text)
    old, new = os.path.basename(path), os.path.basename(final_path(path))
    lines = text.split('\n')
    for i, line in enumerate(lines[:4]):
        if line.rstrip().endswith(old):
            head = line.rstrip()[:-len(old)].rstrip()
            lines[i] = head + ' ' * max(1, 80 - len(head) - len(new)) + new
            break
    text = '\n'.join(lines)
    return re.sub(r'(\* path:\s+\S*)' + re.escape(old), r'\g<1>' + new, text,
                  count=1)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--from', dest='form', required=True,
                    choices=('bridge', 'sa', 'program', 'section', 'paths'))
    g = ap.add_mutually_exclusive_group()
    g.add_argument('--in-place', action='store_true')
    g.add_argument('--out')
    ap.add_argument('--report', action='store_true')
    ap.add_argument('--final', action='store_true')
    ap.add_argument('files', nargs='+')
    a = ap.parse_args()
    if a.final and not (a.form == 'sa' and a.in_place):
        ap.error('--final goes with --from sa --in-place')

    conv = {'bridge': convert_bridge, 'sa': convert_sa,
            'program': convert_program, 'section': convert_section,
            'paths': convert_paths}[a.form]
    marks = []
    if a.form == 'sa':
        # which sections check, across every file of the run, before any
        # changes: a suite's sections call each other across its files
        for path in a.files:
            raw = open(path, 'rb').read().decode('utf-8', 'surrogateescape')
            raw = raw.replace('\r\n', '\n')
            CHECKERS.update(find_checkers(raw))
            # a report helper over a counter: `const struct dj_test_counter*`
            GROUP_HELPERS.update(re.findall(
                r'^(?:static\s+)?(?:struct\s+)?\w+\s*\**\s*(d_tests?_\w+)\s*\([^;{)]*'
                r'struct\s+dj_test_object\s*\*\s*\*', raw, re.M))
            REPORT_HELPERS.update(re.findall(
                r'^(?:static\s+)?(?:void|bool|int)\s+(d_tests?_\w+)\s*\([^;{)]*'
                r'const\s+struct\s+dj_test_counter\s*\*', raw, re.M))
    for path in a.files:
        raw = open(path, 'rb').read()
        crlf = b'\r\n' in raw
        text = raw.decode('utf-8', errors='surrogateescape').replace('\r\n',
                                                                     '\n')
        new = conv(text, path, marks)
        if a.form != 'paths':
            mark_leftovers(new, path, marks)
        if a.final:
            new = final_names(new, path)
        data = new.replace('\n', '\r\n') if crlf else new
        data = data.encode('utf-8', errors='surrogateescape')
        if a.in_place:
            open(path, 'wb').write(data)
            if a.final:
                dst = final_path(path)
                if dst == path:
                    pass
                elif os.path.exists(dst):
                    print('%s: %s is taken: merge by hand' % (path, dst),
                          file=sys.stderr)
                else:
                    os.rename(path, dst)
        elif a.out:
            dst = os.path.join(a.out, path)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            open(dst, 'wb').write(data)
        else:
            sys.stdout.buffer.write(data)
    if a.form == 'section':
        print_section_runner()
    if a.report or a.in_place or a.out:
        for path, line, why in marks:
            print('%s:%d: %s %s' % (path, line, MARK, why), file=sys.stderr)
        print('%d file(s), %d mark(s) to resolve by hand'
              % (len(a.files), len(marks)), file=sys.stderr)
    return 1 if marks else 0


if __name__ == '__main__':
    sys.exit(main())
