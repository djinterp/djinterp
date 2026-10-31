#!/usr/bin/env python3
"""gate64.py <root> <file>...: the owner's ruling on round 3's question 1,
(a): a header that needs a 64-bit integer type compiles to nothing where
INT64_MAX is not defined -- ISO strict C++98 wherever int64_t is `long long`:
a 32-bit target, 64-bit Windows or macOS (dstdint.h's banner) -- as a header
does below its language floor (round 2's rule: never an #error).

gate64.py --reword <root>: the owner's ruling of 2026.10.23 -- every comment
under inc/, src/ and tests/ that says the floor is "ISO strict C++98 on a
32-bit target" says instead where int64_t is `long long`, 64-bit Windows
and macOS included. Only the exact old phrases are rewritten, with the rest of
their comment paragraph rewrapped to 80 columns; a phrase left over, in any
other words, stops the run with its place, to be reworded by hand.
The gate wraps the header's body, after its includes: dstdint.h, which
the test reads, stays in its // re_std group after // djinterp, so that
djinterp's config is read first (L2)."""
import os, re, sys

ROOT = sys.argv[2] if sys.argv[1] == '--reword' else sys.argv[1]
TARGET = 'inc/re_std/c/cstdint/dstdint.h'
OPEN = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b')
CLOSE = re.compile(r'^\s*#\s*endif\b')
INC = re.compile(r'^\s*#\s*include\b')
COMMENT = [
    "// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h",
    "// declares only where the build can spell one. Below it -- ISO strict",
    "// C++98 wherever int64_t is `long long`: a 32-bit target, 64-bit Windows",
    "// or macOS -- the header compiles to nothing (the owner's ruling of",
    "// 2026.10.03 on round 3's question 1, (a)).",
]

# --reword: each old phrase, whitespace-normalized, and its new words
REWORD = [
    ("ISO strict C++98 on a 32-bit target",
     "ISO strict C++98 wherever int64_t is `long long`: a 32-bit target, "
     "64-bit Windows or macOS"),
    ("Under ISO strict C++98 that is int64_t on a 32-bit target,",
     "Under ISO strict C++98 that is int64_t wherever it is `long long` -- a "
     "32-bit target, 64-bit Windows or macOS --"),
]
# what --reword must leave nowhere: the old floor, in whatever words
LEFTOVER = re.compile(r'strict\s+(?://\s*)?C\+\+98[^.;]{0,40}?\bon\s+a\s+'
                      r'(?://\s*)?32-bit\s+(?://\s*)?target', re.S)


def reword_text(text):
    """The rewording of one file's text, and how many phrases it changed."""
    import textwrap
    lines = text.split('\n')
    changed = 0
    i = 0
    while i < len(lines):
        m = re.match(r'^(\s*//) (\S.*)$', lines[i])
        if not m:
            i += 1
            continue
        prefix = m.group(1)
        # the paragraph: this line and those after it in the same prefix,
        # up to a bare "//" or anything else
        j = i
        while ( (j < len(lines)) and
                re.match('^' + re.escape(prefix) + r' \S', lines[j]) ):
            j += 1
        parts = [l[len(prefix) + 1:].strip() for l in lines[i:j]]
        words = ' '.join(parts)
        found = [words.find(o) for (o, _) in REWORD if o in words]
        if not found:
            i = j
            continue
        # the lines before the one where the first phrase begins stay
        offset, start = 0, i
        for k, part in enumerate(parts):
            if offset + len(part) >= min(found):
                start = i + k
                break
            offset += len(part) + 1
        rest = ' '.join(parts[start - i:])
        for o, n in REWORD:
            changed += rest.count(o)
            rest = rest.replace(o, n)
        width = 80 - len(prefix) - 1
        new = [prefix + ' ' + l for l in textwrap.wrap(
                   rest, width, break_on_hyphens=False,
                   break_long_words=False)]
        lines[start:j] = new
        i = start + len(new)
    return '\n'.join(lines), changed


def reword(root):
    total = 0
    left = []
    for top in ('inc', 'src', 'tests'):
        for d, _, fs in os.walk(os.path.join(root, top)):
            for f in sorted(fs):
                if not f.endswith(('.h', '.hpp', '.c', '.cpp', '.inl')) and \
                   '.' in f:
                    continue
                full = os.path.join(d, f)
                try:
                    text = open(full, encoding='utf-8').read()
                except (UnicodeDecodeError, IsADirectoryError):
                    continue
                new, n = reword_text(text)
                if n:
                    open(full, 'w', encoding='utf-8').write(new)
                    print('%s: %d' % (os.path.relpath(full, root), n))
                    total += n
                for m in LEFTOVER.finditer(new):
                    # one that names Windows already says it all
                    if 'Windows' in new[m.start():m.end() + 40]:
                        continue
                    line = new.count('\n', 0, m.start()) + 1
                    left.append('%s:%d' % (os.path.relpath(full, root), line))
    print('reworded %d' % total)
    if left:
        print('left, to reword by hand:')
        print('\n'.join(left))
        sys.exit(1)


def process(path):
    full = os.path.join(ROOT, path)
    L = open(full, encoding='utf-8').read().split('\n')
    if any('defined(INT64_MAX)' in l for l in L):
        return 'already gated'
    depth, at = 0, []
    for l in L:
        if OPEN.match(l):
            depth += 1
            at.append(depth)
        elif CLOSE.match(l):
            at.append(depth)
            depth -= 1
        else:
            at.append(depth)
    closes = [i for i, l in enumerate(L) if CLOSE.match(l) and at[i] == 1]
    if not closes:
        return 'NEEDS HUMAN: no guard'
    end = closes[-1]
    incs = [i for i, l in enumerate(L)
            if INC.match(l) and at[i] == 1 and i < end]
    if not incs:
        return 'NEEDS HUMAN: no include at the guard level'
    last = incs[-1]
    col = L[last].find('//')
    j = last + 1
    while ( (col > 0) and (j < len(L)) and L[j].startswith(' ') and
            (L[j].find('//') == col) ):
        j += 1
    note = []
    if not any('re_std/c/cstdint/dstdint.h' in L[i] for i in incs):
        rel = os.path.relpath(os.path.join(ROOT, TARGET),
                              os.path.dirname(full))
        added = ['// re_std',
                 '#include "%s"  // INT64_MAX: this header\'s floor' % rel]
        note.append('dstdint.h added')
    else:
        added = []
    L = (L[:end] + ['#endif  // defined(INT64_MAX)', ''] + L[end:])
    L = L[:j] + added + [''] + COMMENT + ['#if defined(INT64_MAX)'] + L[j:]
    open(full, 'w', encoding='utf-8').write('\n'.join(L))
    wide = [l for l in L if len(l) > 80 and 'INT64_MAX' in l]
    if wide:
        note.append('WIDE %d' % len(wide))
    return 'gated' + ('; ' + ', '.join(note) if note else '')


if sys.argv[1] == '--reword':
    reword(sys.argv[2])
else:
    for p in sys.argv[2:]:
        print('%s: %s' % (p, process(p)))
