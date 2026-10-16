#!/usr/bin/env python3
"""diff_literal.py -- the literal decoder against gcc, validity and value.

    tests/parse/diff_literal.py <decode_literals binary> [count]

For each standard below, generates literals -- valid and deliberately
broken -- and asks two questions of both the decoder and gcc:

  validity  gcc -pedantic-errors is compiled over one `(void)(literal);` per
            line and its error lines are collected; the decoder's verdict
            must agree on every line.
  value     for every literal both accept, gcc prints the value it computes
            and the decoder's value must match: integers exactly, unsuffixed
            floating literals bit for bit, character literals as the code
            unit or multicharacter value.

A few constructs are deliberately left out of the comparison, because gcc 13
is not the standard there and the unit tests cover them instead: `wb`
(gcc 14), decimal floating suffixes, `\\N{...}`, unknown escapes (a pedwarn
in C, so an error under -pedantic-errors, where the standard leaves them
conditionally supported), and user-defined suffixes, which need an operator.
Prints the mismatches and exits non-zero if there are any.
"""

import os
import random
import subprocess
import sys
import tempfile

STANDARDS = [
    ("c11", "gcc", "-std=c11"),
    ("c17", "gcc", "-std=c17"),
    ("c23", "gcc", "-std=c2x"),
    ("c++14", "g++", "-std=c++14"),
    ("c++17", "g++", "-std=c++17"),
    ("c++20", "g++", "-std=c++20"),
    ("c++23", "g++", "-std=c++23"),
]


def separate(rng, digits, std):
    """Insert digit separators where the standard has them."""
    if std in ("c11", "c17") or len(digits) < 2 or rng.random() < 0.6:
        return digits
    out = digits[0]
    for d in digits[1:]:
        out += ("'" if rng.random() < 0.25 else "") + d
    return out


def integer(rng, std):
    """A valid integer literal, with a random suffix."""
    base = rng.choice([10, 8, 16, 2 if std not in ("c11", "c17") else 16])
    value = rng.choice([rng.randrange(0, 1000), rng.randrange(0, 2 ** 32),
                        rng.randrange(0, 2 ** 64), 2 ** 64 - 1,
                        2 ** 63, 2 ** 31])
    if base == 10:
        body = str(value) if value else "1"
    elif base == 8:
        body = "0" + format(value, "o")
    elif base == 16:
        body = rng.choice(["0x", "0X"]) + format(value, rng.choice("xX"))
    else:
        body = rng.choice(["0b", "0B"]) + format(value, "b")
    prefix = body[:2] if body[:2].lower() in ("0x", "0b") else ""
    body = prefix + separate(rng, body[len(prefix):], std)
    suffixes = ["", "u", "U", "l", "L", "ul", "LU", "ll", "LL", "ull",
                "LLU", "uLL"]
    if std == "c++23":
        suffixes += ["z", "Z", "uz", "zu", "UZ"]
    return body + rng.choice(suffixes)


def floating(rng, std):
    """A valid floating literal, unsuffixed or `f`."""
    if std not in ("c++14",) and rng.random() < 0.3:
        mant = format(rng.randrange(1, 2 ** 20), "x")
        frac = format(rng.randrange(0, 2 ** 12), "x")
        lit = "0x%s.%sp%+d" % (mant, frac, rng.randrange(-40, 40))
    else:
        whole = str(rng.randrange(0, 10 ** rng.randrange(1, 12)))
        frac = str(rng.randrange(0, 10 ** rng.randrange(1, 12)))
        lit = rng.choice(["%s.%s", "%s.", ".%s", "%se%d", "%s.%se%d"])
        args = {"%s.%s": (whole, frac), "%s.": (whole,), ".%s": (frac,),
                "%se%d": (whole, rng.randrange(-30, 30)),
                "%s.%se%d": (whole, frac, rng.randrange(-300, 300))}[lit]
        lit = lit % args
    return lit + rng.choice(["", "", "f", "F"])


def character(rng, std):
    """A character literal: plain, escaped, UCN, extended, prefixed."""
    cpp = std.startswith("c++")
    body = rng.choice([
        rng.choice("abcXYZ09 ~!"), "\\n", "\\t", "\\0", "\\'", "\\\\",
        "\\x41", "\\x7f", "\\xff", "\\101", "\\377", "\\7",
        "\\u00e9", "\\u20ac", "\\U0001F600", "\u00e9", "\u20ac", "ab",
        "\\x41\\x42"])
    prefixes = ["", "L", "u", "U"]
    if std in ("c23",) or (cpp and std != "c++14"):
        prefixes.append("u8")
    if not cpp and body in ("\\u00e9",) and rng.random() < 0.5:
        body = "\\u0041"
    return rng.choice(prefixes) + "'" + body + "'"


def string(rng, std):
    """A string literal, sometimes raw in C++."""
    body = "".join(rng.choice(["a", "\\n", "\\x41", "\\u00e9", "\u20ac",
                               " ", "\\\"", "\\101"])
                   for _ in range(rng.randrange(0, 6)))
    prefix = rng.choice(["", "L", "u", "U", "u8"])
    if std.startswith("c++") and rng.random() < 0.2:
        return prefix + 'R"x(' + body.replace("\\", "/") + ')x"'
    return prefix + '"' + body + '"'


BROKEN = [
    "09", "0b2", "08u", "1e", "1e+", "0x", "0x.p1", "0x1.8", "1lL", "1uu",
    "1ul l", "1.0u", "1.0ll", "0x1p", "1x", "1'", "0x'1", "1''2", "'\\x'",
    "'\\u12'", "'\\U0000D800'", "'\\u0041'", "\"\\x\"", "u8'\u00e9'",
    "u'\\U0001F600'", "'\\x100'", "u8'\\x100'", "u'\\x10000'",
    "18446744073709551616", "0x1_", "1.0.0", "'\\u{41}'", "\"\\o{7}\"",
]


# per standard, broken literals gcc 13 judges by something other than the
# standard: `\o{}` before C++23 is an unknown escape; `\u{}` in C is simply
# ill-formed but gcc accepts it as an extension
SKIP = {std: ('"\\o{7}"',) for std, _, _ in STANDARDS if std != "c++23"}


# where gcc 13 lags the standard: P1854 made an ordinary character literal
# whose one character needs several code units ill-formed in C++23, and gcc
# 13 still accepts it with a warning
KNOWN = {("c++23", "'\\u20ac'"), ("c++23", "'\u20ac'"),
         ("c++23", "'\u00e9'"), ("c++23", "'\\u00e9'"),
         ("c++23", "'\\U0001F600'")}


def cases(rng, std, count):
    """The literals for one standard."""
    makers = [integer, floating, character, string]
    out = [rng.choice(makers)(rng, std) for _ in range(count)]
    # a separator that is not between digits opens a character literal that
    # never closes, and gcc's recovery from that misplaces every later error;
    # those three are left to the unit tests
    out += [b for b in BROKEN if b not in ("1'", "1''2", "0x'1")]
    return [c for c in out if " " not in c or c.startswith(("'", '"'))]


def gcc_errors(compiler, flag, literals, workdir):
    """Lines of `literals` gcc rejects."""
    ext = ".cpp" if compiler == "g++" else ".c"
    path = os.path.join(workdir, "v" + ext)
    with open(path, "w", encoding="utf-8") as handle:
        for index, lit in enumerate(literals):
            handle.write("void f%d(void){(void)(%s);}\n" % (index, lit))
    run = subprocess.run([compiler, flag, "-pedantic-errors", "-fsyntax-only",
                          "-Wno-multichar", path],
                         capture_output=True, text=True,
                         env={"LC_ALL": "C", "PATH": os.environ["PATH"]})
    bad = set()
    for line in run.stderr.splitlines():
        parts = line.split(":")
        if len(parts) > 3 and "error" in parts[3] and parts[1].isdigit():
            bad.add(int(parts[1]) - 1)
    return bad


def gcc_values(compiler, flag, entries, workdir):
    """What gcc computes for each (index, literal, category) entry."""
    ext = ".cpp" if compiler == "g++" else ".c"
    src = os.path.join(workdir, "p" + ext)
    exe = os.path.join(workdir, "p")
    with open(src, "w", encoding="utf-8") as handle:
        handle.write("#include <stdio.h>\nint main(void){\n")
        for index, lit, category in entries:
            if category == 1:
                handle.write('printf("%%d %%llx\\n", %d, '
                             '(unsigned long long)(%s));\n' % (index, lit))
            elif category == 2:
                handle.write('printf("%%d %%a\\n", %d, (double)(%s));\n'
                             % (index, lit))
            else:
                handle.write('printf("%%d %%llx\\n", %d, (unsigned long long)'
                             '(unsigned)(%s));\n' % (index, lit))
        handle.write("return 0;}\n")
    subprocess.run([compiler, flag, "-w", "-o", exe, src], check=True)
    out = subprocess.run([exe], capture_output=True, text=True).stdout
    return dict((int(a), b) for a, b in
                (line.split(" ", 1) for line in out.splitlines()))


def main(argv):
    decoder = argv[1]
    count = int(argv[2]) if len(argv) > 2 else 400
    rng = random.Random(20260922)
    mismatches = 0
    checked = 0
    with tempfile.TemporaryDirectory() as workdir:
        for std, compiler, flag in STANDARDS:
            literals = cases(rng, std, count)
            listing = os.path.join(workdir, "l.txt")
            with open(listing, "w", encoding="utf-8") as handle:
                handle.write("\n".join(literals) + "\n")
            ours = subprocess.run([decoder, std, listing], capture_output=True,
                                  text=True, check=True).stdout.splitlines()
            theirs = gcc_errors(compiler, flag, literals, workdir)
            entries = []
            for index, (lit, line) in enumerate(zip(literals, ours)):
                fields = line.split()
                ok = fields[1] == "OK"
                # a ud-suffix needs an operator, and an unknown escape is a
                # pedwarn in gcc but conditionally supported in the standard
                if ((int(fields[6]) & 0x400) or lit in SKIP.get(std, ())
                        or (std, lit) in KNOWN):
                    continue
                checked += 1
                if ok == (index in theirs):
                    mismatches += 1
                    print("%-6s validity %-28s ours %-3s gcc %s"
                          % (std, lit, fields[1],
                             "ERR" if index in theirs else "OK"))
                    continue
                category = int(fields[2])
                if ok and category in (1, 2, 3) and not (
                        category == 2 and lit[-1] in "fF"):
                    entries.append((index, lit, category, fields))
            values = gcc_values(compiler, flag,
                                [(i, l, c) for i, l, c, _ in entries], workdir)
            for index, lit, category, fields in entries:
                want = values.get(index)
                if category == 1:
                    have = fields[4]
                elif category == 2:
                    have = fields[5]
                else:
                    have = fields[4]
                    single = (fields[3] in ("0", "2") and fields[7] == "1"
                              and int(fields[4], 16) <= 0xFF)
                    if want and single:
                        want = format(int(want, 16) & 0xFF, "x")
                if want != have:
                    mismatches += 1
                    print("%-6s value    %-28s ours %s gcc %s"
                          % (std, lit, have, want))
    print("literals %d  mismatches %d" % (checked, mismatches))
    return 1 if mismatches else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
