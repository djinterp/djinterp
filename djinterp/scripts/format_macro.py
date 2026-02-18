#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple


_DEFINE_RE = re.compile(r"^(\s*#\s*define\s+)([A-Za-z_]\w*)\s*\(")


@dataclass
class FormatCfg:
    max_width: int
    indent: int
    paren_pad: int
    params_per_line: int  # 0 => auto
    align: str            # none|comma
    backslash_col: int    # 0 => no special alignment
    space_before_bslash: int
    body_on_newline: bool
    strip_doc_comments: bool
    start_params_new_line: bool


def _split_macro_block(lines: List[str], i: int) -> Tuple[int, List[str]]:
    """
    Given file lines and an index where a #define starts,
    return (next_index_after_block, block_lines).
    Block continues while the current line ends with a backslash (ignoring trailing spaces).
    """
    blk = [lines[i]]
    i += 1
    while blk[-1].rstrip().endswith("\\") and i < len(lines):
        blk.append(lines[i])
        i += 1
    return i, blk


def _find_param_close(block: List[str]) -> Tuple[int, int]:
    """
    Find the line/col (within that line) of the ')' that closes the define's parameter list.
    Assumes the block begins with #define NAME( ...
    """
    # Start scanning after the first '(' in the first line
    first = block[0]
    open_pos = first.find("(")
    if open_pos < 0:
        raise ValueError("internal: no '(' found in define start line")

    depth = 0
    for li, line in enumerate(block):
        s = line
        # ignore trailing backslash for parsing
        if s.rstrip().endswith("\\"):
            s = s.rstrip()
            s = s[:-1]
        start = open_pos + 1 if li == 0 else 0
        for ci in range(start, len(s)):
            ch = s[ci]
            if ch == "(":
                depth += 1
            elif ch == ")":
                if depth == 0:
                    return li, ci
                depth -= 1
    raise ValueError("could not find closing ')' for macro parameter list")


def _parse_params(block: List[str], close_li: int, close_ci: int) -> List[str]:
    # Extract everything between the first '(' and the close ')'
    parts: List[str] = []
    first = block[0]
    open_pos = first.find("(")
    if open_pos < 0:
        return []

    # Gather text from (open_pos+1) up to close
    for li, line in enumerate(block):
        s = line
        if s.rstrip().endswith("\\"):
            s = s.rstrip()[:-1]
        if li == 0:
            s = s[open_pos + 1 :]
            if li == close_li:
                # close_ci is in the original line, need to adjust for the slice
                s = s[:close_ci - open_pos - 1]
                parts.append(s)
                break
        elif li == close_li:
            s = s[:close_ci]
            parts.append(s)
            break
        else:
            parts.append(s)

    raw = " ".join(p.strip() for p in parts if p.strip())
    if not raw:
        return []
    return [p.strip() for p in raw.split(",")]


def _parse_body(block: List[str], close_li: int, close_ci: int) -> List[str]:
    """
    Body is anything after the close ')' (on that line), plus subsequent lines,
    with trailing '\' removed.
    """
    out: List[str] = []

    for li, line in enumerate(block):
        s = line.rstrip("\n")
        has_bslash = s.rstrip().endswith("\\")
        if has_bslash:
            s2 = s.rstrip()
            s2 = s2[:-1]
        else:
            s2 = s

        if li < close_li:
            continue
        if li == close_li:
            tail = s2[close_ci + 1 :].strip()
            if tail:
                out.append(tail)
        else:
            tail = s2.strip()
            if tail:
                out.append(tail)

    return out


def _align_backslashes(lines: List[str], cfg: FormatCfg, add_bslash: List[bool]) -> List[str]:
    if cfg.backslash_col <= 0:
        # just append with minimal spacing
        out = []
        for s, need in zip(lines, add_bslash):
            if need:
                out.append(s + (" " * cfg.space_before_bslash) + "\\")
            else:
                out.append(s)
        return out

    target = cfg.backslash_col  # 1-based column (visual)
    out = []
    for s, need in zip(lines, add_bslash):
        if not need:
            out.append(s)
            continue
        # we want '\' to appear at column = target
        # so characters before it should be (target-1)
        pre_len = target - 1
        if len(s) >= pre_len:
            # can't pad to that column; fall back to minimal spacing
            out.append(s + (" " * cfg.space_before_bslash) + "\\")
        else:
            pad = pre_len - len(s)
            if pad < cfg.space_before_bslash:
                pad = cfg.space_before_bslash
            out.append(s + (" " * pad) + "\\")
    return out


def _format_define(prefix: str, name: str, params: List[str], body: List[str], cfg: FormatCfg) -> List[str]:
    # Parameter field formatting
    p = params[:]
    if cfg.align == "comma" and p:
        w = max(len(x) for x in p)
        p = [x.rjust(w) for x in p]

    head = f"{prefix}{name}("
    lines: List[str] = []

    if cfg.start_params_new_line and params:
        # Put opening paren on define line, start params on next line
        lines.append(head)
        cur = " " * cfg.indent
        first_on_line = True
        cols = 0

        def flush():
            nonlocal cur, first_on_line, cols
            lines.append(cur + ",")
            cur = " " * cfg.indent
            first_on_line = True
            cols = 0

        for i, tok in enumerate(p):
            is_last = (i == len(p) - 1)
            sep = "" if first_on_line else ", "
            candidate = cur + sep + tok

            if cfg.params_per_line > 0:
                # strict grouping
                if cols >= cfg.params_per_line:
                    flush()
                    candidate = cur + tok
                    cols = 0
                    sep = ""
                candidate = cur + ("" if first_on_line else ", ") + tok
                cur = candidate
                first_on_line = False
                cols += 1
                
                # If this is the last param, no comma after it
                if is_last:
                    cur = cur
                continue

            # auto wrap by width
            if (not first_on_line) and (len(candidate) > cfg.max_width):
                flush()
                cur = cur + tok
                first_on_line = False
                cols = 1
            else:
                cur = candidate
                first_on_line = False
                cols += 1

        # Add closing paren on same line as last param
        cur = cur + ")"
        lines.append(cur)

    else:
        # Original behavior: params start on same line as opening paren
        cur = head + (" " * cfg.paren_pad)
        first_on_line = True
        cols = 0

        def flush():
            nonlocal cur, first_on_line, cols
            lines.append(cur)
            cur = " " * cfg.indent
            first_on_line = True
            cols = 0

        for i, tok in enumerate(p):
            sep = "" if (first_on_line) else ", "
            candidate = cur + sep + tok

            if cfg.params_per_line > 0:
                # strict grouping
                if cols >= cfg.params_per_line:
                    flush()
                    candidate = cur + tok
                    cols = 0
                    sep = ""
                candidate = cur + ("" if first_on_line else ", ") + tok
                cur = candidate
                first_on_line = False
                cols += 1
                continue

            # auto wrap by width
            if (not first_on_line) and (len(candidate) > cfg.max_width):
                flush()
                cur = cur + tok
                first_on_line = False
                cols = 1
            else:
                cur = candidate
                first_on_line = False
                cols += 1

        cur = cur + ")"
        lines.append(cur)

    # Build body lines
    if not body:
        return lines

    if cfg.body_on_newline:
        # Body goes on separate line(s)
        body_lines = [(" " * cfg.indent) + b for b in body]
        return lines + body_lines
    else:
        # try to keep body on same line if it fits (first body token only)
        if len(body) == 1 and (len(lines[-1]) + 1 + len(body[0]) <= cfg.max_width):
            lines[-1] = lines[-1] + " " + body[0]
            return lines
        else:
            body_lines = [(" " * cfg.indent) + b for b in body]
            return lines + body_lines


def format_file(text: str, cfg: FormatCfg) -> str:
    lines = text.splitlines(True)
    out: List[str] = []

    i = 0
    while i < len(lines):
        m = _DEFINE_RE.match(lines[i])
        if not m:
            out.append(lines[i])
            i += 1
            continue

        # Optionally strip preceding // doc comments immediately above
        if cfg.strip_doc_comments:
            while out and out[-1].lstrip().startswith("//"):
                out.pop()

        prefix, name = m.group(1), m.group(2)

        next_i, blk = _split_macro_block(lines, i)
        close_li, close_ci = _find_param_close(blk)
        params = _parse_params(blk, close_li, close_ci)
        body = _parse_body(blk, close_li, close_ci)

        formatted = _format_define(prefix, name, params, body, cfg)

        # Determine which lines need trailing backslashes:
        # - If there is a body AND body_on_newline, all param lines need '\' and all body lines except last need '\'
        add_bslash = [False] * len(formatted)
        if body and cfg.body_on_newline:
            for k in range(0, len(formatted) - 1):
                add_bslash[k] = True
        elif cfg.start_params_new_line and params:
            # When params start on new line, all lines except the last need backslash
            for k in range(0, len(formatted) - 1):
                add_bslash[k] = True

        formatted = _align_backslashes([s.rstrip("\n") for s in formatted], cfg, add_bslash)
        out.extend([s + "\n" for s in formatted])

        i = next_i

    return "".join(out)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+", help="Input file(s) to format")
    ap.add_argument("--in-place", action="store_true", help="Rewrite files in place")
    ap.add_argument("--out", default="", help="Output path (only valid with a single input)")
    ap.add_argument("--max-width", type=int, default=100)
    ap.add_argument("--indent", type=int, default=4, help="Indent for continuation lines")
    ap.add_argument("--paren-pad", type=int, default=4, help="Spaces after '(' on the first line")
    ap.add_argument("--params-per-line", type=int, default=0, help="0=auto wrap, else fixed params per line")
    ap.add_argument("--align", choices=("none", "comma"), default="comma")
    ap.add_argument("--backslash-col", type=int, default=0, help="1-based column to align '\\' to (0=don't align)")
    ap.add_argument("--space-before-backslash", type=int, default=1)
    ap.add_argument("--body-on-newline", action="store_true", help="Force body to a new line and use multi-line '\\'")
    ap.add_argument("--strip-doc-comments", action="store_true", help="Remove preceding // comment blocks above formatted defines")
    ap.add_argument("--start-params-new-line", action="store_true", help="If macro wraps, put '(' on the define line and start first parameter on next line.")
    args = ap.parse_args()

    cfg = FormatCfg(
        max_width=args.max_width,
        indent=args.indent,
        paren_pad=args.paren_pad,
        params_per_line=args.params_per_line,
        align=args.align,
        backslash_col=args.backslash_col,
        space_before_bslash=args.space_before_backslash,
        body_on_newline=args.body_on_newline,
        strip_doc_comments=args.strip_doc_comments,
        start_params_new_line=args.start_params_new_line,
    )

    in_paths = [Path(p) for p in args.paths]
    if args.out and (len(in_paths) != 1):
        raise SystemExit("--out can only be used with a single input file")
    if (not args.in_place) and (not args.out):
        raise SystemExit("Specify either --in-place or --out")

    for p in in_paths:
        s = p.read_text(encoding="utf-8")
        formatted = format_file(s, cfg)

        if args.in_place:
            p.write_text(formatted, encoding="utf-8")
        else:
            Path(args.out).write_text(formatted, encoding="utf-8")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())