#!/usr/bin/env python3
"""
split_re_std.py -- split inc/re_std/ into inc/re_std/c/ and inc/re_std/cpp/.

A file's language decides its tree: .h and .c go to c/, everything else
(.hpp, .cpp, the extensionless umbrellas) to cpp/. config.h and config.hpp
stay at re_std's root. Then every spelling of a moved path is updated:

  1. #include "..." directives, in every C/C++ file of the repository. Each
     include is RESOLVED (against the including file's folder, then against
     inc/) and rewritten only if the file it names moved, or if the file
     doing the including moved. A relative include stays relative and one
     written from inc/ stays written from inc/. Text that merely looks like
     a re_std path (tests/re_std/cpp/any/..., src/re_std/c/cstdint/...) is never
     touched, because it does not resolve to a moved file.
  2. Templates (*.in) resolve relative to the file they generate, not to
     themselves, so their includes are rewritten by suffix: the part from
     re_std/ on, which names the moved file, and nothing before it.
  3. Path references in other text (banner path: lines, comments, docs,
     CMake, scripts): an occurrence of re_std/<old path of a moved file> is
     rewritten only where what precedes re_std/ is nothing path-like, inc/,
     or a ./ or ../ chain.
  4. Include guards: re_std's derivation skips cpp/ and adds C_ for c/, so a
     C++ header keeps its guard and a C header's changes. A guard is
     rewritten only where it equals the old derivation, and every other
     reference to it in the repository follows.
  5. Trailing include comments that were aligned in a group are realigned.

Idempotent: on a tree that is already split it reports nothing to do.

Usage:
    python3 split_re_std.py --root <repo>            # dry run, report only
    python3 split_re_std.py --root <repo> --apply    # do it
    python3 split_re_std.py --root <repo> --apply --git   # use git mv

Exit status 0 on success, 1 if verification finds an include that resolved
before the split and does not resolve to the moved file after it.
"""

import argparse
import os
import re
import subprocess
import sys
from collections import Counter, defaultdict

# ----------------------------------------------------------------------------
# settings
# ----------------------------------------------------------------------------

C_EXTS        = {".h", ".c"}
SOURCE_EXTS   = {".h", ".hpp", ".c", ".cpp", ".inl", ".ipp", ".tpp", ".cc",
                 ".cxx", ".hh", ".hxx"}
TEMPLATE_EXTS = {".in"}
TEXT_EXTS     = {".md", ".txt", ".cmake", ".sh", ".ps1", ".py", ".yml",
                 ".yaml", ".json", ".bat", ".cfg", ".ini", ".toml", ".dss"}
TEXT_NAMES    = {"CMakeLists.txt", "Makefile"}
SKIP_DIRS     = {".git", "_handoffs", "retired", "node_modules", "out",
                 "__pycache__", ".vs"}
ROOT_KEEP     = {"config.h", "config.hpp"}

INCLUDE_RE = re.compile(r'^(?P<lead>\s*#\s*include\s*)"(?P<path>[^"\n]+)"'
                        r'(?P<rest>[^\n]*)$', re.M)
PATH_CHARS = set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                 "0123456789_-./${}")


def posix(p):
    return p.replace(os.sep, "/")


def read_text(path):
    with open(path, "rb") as f:
        raw = f.read()
    if b"\0" in raw[:8192]:
        return None
    try:
        return raw.decode("utf-8")
    except UnicodeDecodeError:
        return None


def write_text(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(text.encode("utf-8"))


# ----------------------------------------------------------------------------
# plan
# ----------------------------------------------------------------------------

def plan_moves(root):
    """Returns {old_abs: new_abs} for every re_std file that moves."""
    rs = os.path.join(root, "inc", "re_std")
    moves = {}
    for name in sorted(os.listdir(rs)):
        p = os.path.join(rs, name)
        if name in ("c", "cpp"):
            continue
        if os.path.isfile(p):
            if name not in ROOT_KEEP:
                print(f"  note: root file left where it is: inc/re_std/{name}")
            continue
        for dp, dn, fn in os.walk(p):
            dn.sort()
            for f in sorted(fn):
                old = os.path.join(dp, f)
                rel = os.path.relpath(old, rs)
                tree = "c" if os.path.splitext(f)[1] in C_EXTS else "cpp"
                moves[old] = os.path.join(rs, tree, rel)
    return moves


def walk_repo(root):
    for dp, dn, fn in os.walk(root):
        dn[:] = sorted(d for d in dn if d not in SKIP_DIRS)
        for f in sorted(fn):
            yield os.path.join(dp, f)


def classify(path):
    name = os.path.basename(path)
    ext = os.path.splitext(name)[1]
    if ext in SOURCE_EXTS:
        return "source"
    if ext in TEMPLATE_EXTS:
        return "template"
    if ext in TEXT_EXTS or name in TEXT_NAMES:
        return "text"
    if ext == "":
        return "maybe_source"
    return None


# ----------------------------------------------------------------------------
# include resolution
# ----------------------------------------------------------------------------

def resolve(including, target, inc_root):
    """Returns (abs_path, how) or (None, None); how is 'rel' or 'root'."""
    cand = os.path.normpath(os.path.join(os.path.dirname(including), target))
    if os.path.isfile(cand):
        return cand, "rel"
    cand = os.path.normpath(os.path.join(inc_root, target))
    if os.path.isfile(cand):
        return cand, "root"
    return None, None


def respell(old_spelling, how, new_includer, new_target, inc_root):
    if how == "root":
        return posix(os.path.relpath(new_target, inc_root))
    new = posix(os.path.relpath(new_target, os.path.dirname(new_includer)))
    if old_spelling.startswith("./") and not new.startswith("../"):
        new = "./" + new
    return new


# ----------------------------------------------------------------------------
# guards
# ----------------------------------------------------------------------------

def re_std_guard(rel_under_re_std, split):
    """re_std's guard derivation. With split=True, a leading cpp/ adds
    nothing and a leading c/ adds C_."""
    rel = posix(rel_under_re_std)
    if split and rel.startswith("cpp/"):
        rel = rel[4:]
    body = re.sub(r"[^A-Za-z0-9]+", "_", rel).strip("_").upper()
    return "RE_STD_" + body


# ----------------------------------------------------------------------------
# text references
# ----------------------------------------------------------------------------

def build_text_table(moves, rs):
    """old 're_std/<rel>' -> new 're_std/<tree>/<rel>', longest first."""
    table = {}
    for old, new in moves.items():
        o = "re_std/" + posix(os.path.relpath(old, rs))
        n = "re_std/" + posix(os.path.relpath(new, rs))
        table[o] = n
    keys = sorted(table, key=len, reverse=True)
    pattern = re.compile("|".join(re.escape(k) for k in keys))
    return table, pattern


def prefix_ok(text, i):
    """Is the text before a 're_std/' at index i an acceptable lead-in?"""
    if i == 0 or text[i - 1] not in PATH_CHARS:
        return True
    j = i
    while j > 0 and text[j - 1] in PATH_CHARS:
        j -= 1
    lead = text[j:i]
    if lead.endswith("inc/"):
        return True
    return re.fullmatch(r"(\./|\.\./)+", lead) is not None


def rewrite_text_refs(text, table, pattern):
    out, last, n = [], 0, 0
    for m in pattern.finditer(text):
        end = m.end()
        if end < len(text) and (text[end].isalnum() or text[end] in "_"):
            continue
        if not prefix_ok(text, m.start()):
            continue
        out.append(text[last:m.start()])
        out.append(table[m.group(0)])
        last, n = end, n + 1
    if not n:
        return text, 0
    out.append(text[last:])
    return "".join(out), n


# ----------------------------------------------------------------------------
# comment realignment
# ----------------------------------------------------------------------------

CONT_RE = re.compile(r"^(\s+)//")


def realign(lines, orig, changed_idx):
    """Realigns trailing // comments in include groups that contain a
    changed line. Groups and their shared column are read from the ORIGINAL
    lines, so a continuation comment follows its include even after the
    include grew; the shared column is kept where everything still fits."""
    n = len(orig)
    done = set()
    for idx in sorted(changed_idx):
        if idx in done:
            continue
        lo = idx
        while lo > 0 and _in_group(orig, lo - 1):
            lo -= 1
        hi = idx
        while hi + 1 < n and _in_group(orig, hi + 1):
            hi += 1
        done.update(range(lo, hi + 1))
        _realign_group(lines, orig, lo, hi)


def _is_include(line):
    return re.match(r'^\s*#\s*include\s*"', line) is not None


def _comment_col(line):
    q1 = line.find('"')
    q2 = line.find('"', q1 + 1)
    return line.find("//", q2 + 1)


def _in_group(lines, i):
    line = lines[i]
    if _is_include(line):
        return True
    m = CONT_RE.match(line)
    if not m:
        return False
    # a continuation sits under the comment of the include above it
    k = i - 1
    while k >= 0 and not _is_include(lines[k]):
        if not CONT_RE.match(lines[k]):
            return False
        k -= 1
    if k < 0:
        return False
    col = _comment_col(lines[k])
    return col > 0 and len(m.group(1)) == col


def _split_include(line):
    q1 = line.find('"')
    q2 = line.find('"', q1 + 1)
    body = line[:q2 + 1]
    rest = line[q2 + 1:]
    c = rest.find("//")
    if c < 0:
        return body, rest.rstrip(), None
    return body, rest[:c], rest[c:]


def _realign_group(lines, orig, lo, hi):
    incs = [i for i in range(lo, hi + 1) if _is_include(orig[i])]
    old_cols = [_comment_col(orig[i]) for i in incs
                if _comment_col(orig[i]) > 0]
    if not old_cols:
        return
    shared = len(incs) > 1 or any(not _is_include(orig[i])
                                  for i in range(lo, hi + 1))
    shared = shared and len(set(old_cols)) == 1
    target = None
    if shared:
        need = max(len(_split_include(lines[i])[0]) + 2 for i in incs
                   if _split_include(lines[i])[2] is not None)
        target = max(old_cols[0], need)
    for i in range(lo, hi + 1):
        if _is_include(orig[i]):
            body, gap, com = _split_include(lines[i])
            if com is None:
                continue
            if shared:
                lines[i] = body + " " * (target - len(body)) + com
            else:
                lines[i] = body + " " * max(2, len(gap)) + com
        elif shared:
            m = CONT_RE.match(orig[i])
            lines[i] = " " * target + orig[i][len(m.group(1)):]


# ----------------------------------------------------------------------------
# main pass
# ----------------------------------------------------------------------------

def run(root, apply, use_git):
    root = os.path.abspath(root)
    inc_root = os.path.join(root, "inc")
    rs = os.path.join(inc_root, "re_std")
    moves = plan_moves(root)
    if not moves:
        print("inc/re_std is already split: nothing to do.")
        return 0
    moved_to = dict(moves)
    stats = Counter()
    by_tree = Counter(posix(os.path.relpath(n, rs)).split("/")[0]
                      for n in moves.values())
    print(f"moves: {len(moves)} files ({dict(by_tree)})")

    # guards: old derivation -> new derivation, for headers whose guard
    # currently follows the old derivation
    guard_map = {}
    for old, new in moves.items():
        txt = read_text(old)
        if txt is None:
            continue
        m = re.search(r"^#ifndef\s+(\w+)\s*\n#define\s+\1\b", txt, re.M)
        if not m:
            continue
        g_old = re_std_guard(os.path.relpath(old, rs), split=False)
        g_new = re_std_guard(os.path.relpath(new, rs), split=True)
        if m.group(1) == g_old and g_new != g_old:
            guard_map[g_old] = g_new
    guard_re = (re.compile(r"\b(" + "|".join(map(re.escape, guard_map))
                           + r")\b") if guard_map else None)

    table, text_pat = build_text_table(moves, rs)

    # snapshot of every include's resolution before anything moves
    before = {}
    edits = {}               # old_abs -> new text (written at new location)
    long_lines = []
    unresolved_moved = []

    for path in walk_repo(root):
        kind = classify(path)
        if kind is None:
            continue
        text = read_text(path)
        if text is None:
            continue
        if kind == "maybe_source":
            if "#include" not in text:
                continue
            kind = "source"
        new_path = moved_to.get(path, path)
        orig = text
        lines = text.split("\n")
        changed = set()

        if kind in ("source", "template"):
            for li, line in enumerate(lines):
                m = INCLUDE_RE.match(line)
                if not m:
                    continue
                spelled = m.group("path")
                tgt, how = resolve(path, spelled, inc_root)
                if kind == "source":
                    before[(path, li)] = (tgt, spelled)
                if tgt is None or kind == "template":
                    # template, or stale/unresolvable: suffix rule only
                    new_sp, n = rewrite_text_refs(spelled, table, text_pat)
                    if tgt is None and path in moved_to and n == 0 \
                            and kind == "source":
                        unresolved_moved.append((path, spelled))
                    if n:
                        stats["include (suffix rule)"] += 1
                else:
                    t_new = moved_to.get(tgt, tgt)
                    if path not in moved_to and tgt not in moved_to:
                        continue
                    new_sp = respell(spelled, how, new_path, t_new, inc_root)
                    stats[f"include ({how})"] += 1
                if new_sp != spelled:
                    lines[li] = (m.group("lead") + '"' + new_sp + '"'
                                 + m.group("rest"))
                    changed.add(li)

        if changed:
            realign(lines, orig.split("\n"), changed)
        text = "\n".join(lines)

        # path references in prose, banners, build files, comments
        text2, n = rewrite_text_refs(text, table, text_pat)
        if n:
            stats["text reference"] += n
            text = text2

        if guard_re is not None:
            text2, n = guard_re.subn(lambda mm: guard_map[mm.group(1)], text)
            if n:
                stats["guard token"] += n
                text = text2

        if text != orig or path in moved_to:
            edits[path] = text
            if text != orig:
                stats["files edited"] += 1
                for ln_no, ln in enumerate(text.split("\n"), 1):
                    if len(ln) > 80 and ln not in orig.split("\n"):
                        long_lines.append(
                            (posix(os.path.relpath(new_path, root)), ln_no))

    for k in sorted(stats):
        print(f"  {k:24s} {stats[k]}")
    print(f"  guards renamed           {len(guard_map)} "
          f"{sorted(guard_map.items())}")
    if unresolved_moved:
        print(f"  WARNING: {len(unresolved_moved)} unresolved include(s) in "
              f"moved files")
        for p, s in unresolved_moved[:20]:
            print(f"    {posix(os.path.relpath(p, root))}: {s}")
    if long_lines:
        print(f"  lines newly over 80 chars: {len(long_lines)}")
        for p, n in long_lines:
            print(f"    {p}:{n}")

    if not apply:
        print("dry run: nothing written (pass --apply).")
        return 0

    # write: moved files at their new place, others in place
    for old, text in edits.items():
        new = moved_to.get(old, old)
        if old in moved_to and use_git:
            os.makedirs(os.path.dirname(new), exist_ok=True)
            subprocess.run(["git", "-C", root, "mv", old, new], check=True)
        write_text(new, text)
    for old, new in moves.items():
        if old in edits:
            if not use_git and os.path.exists(old) and old != new:
                os.remove(old)
            continue
        os.makedirs(os.path.dirname(new), exist_ok=True)
        if use_git:
            subprocess.run(["git", "-C", root, "mv", old, new], check=True)
        else:
            os.replace(old, new)
    # remove emptied module folders
    for name in os.listdir(rs):
        p = os.path.join(rs, name)
        if name in ("c", "cpp") or not os.path.isdir(p):
            continue
        for dp, dn, fn in os.walk(p, topdown=False):
            if not os.listdir(dp):
                os.rmdir(dp)

    return verify(root, before, moved_to, inc_root)


def verify(root, before, moved_to, inc_root):
    """Every include that resolved before must resolve to the moved
    counterpart of the same file now."""
    bad = 0
    lost = 0
    checked = 0
    by_file = defaultdict(list)
    for (path, li), (tgt, spelled) in before.items():
        by_file[path].append((li, tgt, spelled))
    for path, entries in by_file.items():
        new_path = moved_to.get(path, path)
        text = read_text(new_path)
        lines = text.split("\n")
        for li, tgt, spelled in entries:
            m = INCLUDE_RE.match(lines[li]) if li < len(lines) else None
            if m is None:
                lost += 1
                continue
            now, _ = resolve(new_path, m.group("path"), inc_root)
            if tgt is None:
                continue
            checked += 1
            if now != moved_to.get(tgt, tgt):
                bad += 1
                if bad <= 20:
                    print(f"  MISMATCH {posix(os.path.relpath(new_path, root))}"
                          f":{li + 1}: {m.group('path')}")
    print(f"verify: {checked} resolved includes checked, {bad} mismatched, "
          f"{lost} lines lost")
    return 1 if (bad or lost) else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--root", required=True, help="repository root")
    ap.add_argument("--apply", action="store_true", help="write the changes")
    ap.add_argument("--git", action="store_true", help="move with git mv")
    a = ap.parse_args()
    sys.exit(run(a.root, a.apply, a.git))


if __name__ == "__main__":
    main()
