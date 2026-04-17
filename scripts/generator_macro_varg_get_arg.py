# gen_varg_get_arg.py
import argparse
from pathlib import Path


def _is_c_ident(s: str) -> bool:
    return s and (s[0].isalpha() or s[0] == "_") and all(ch.isalnum() or ch == "_" for ch in s)


def _chunk(items: list[str], n: int) -> list[list[str]]:
    return [items[i : i + n] for i in range(0, len(items), n)]


def _define_macro_multiline(name: str, params: list[str], body: str, per_line: int) -> list[str]:
    """
    Produces:
      #define NAME(p1, p2, ..., \
                   pK, ...) \
          body
    """
    if per_line < 1:
        per_line = 1

    prefix = f"#define {name}("
    cont_indent = " " * len(prefix)

    groups = _chunk(params, per_line)
    lines: list[str] = []

    for gi, grp in enumerate(groups):
        is_last = (gi == len(groups) - 1)
        joined = ", ".join(grp)
        if gi == 0:
            line = prefix + joined
        else:
            line = cont_indent + joined

        if is_last:
            line += ") \\"
        else:
            line += ", \\"

        lines.append(line)

    lines.append(f"    {body}")
    return lines


def make_get_arg_macro(i: int, macro_prefix: str, pad_width: int, per_line: int, param_prefix: str, param_start: int, param_step: int) -> str:
    name = f"{macro_prefix}{i:0{pad_width}d}"
    params = [f"{param_prefix}{param_start + k * param_step}" for k in range(i)] + ["..."]
    body = f"{param_prefix}{param_start + (i - 1) * param_step}"
    return "\n".join(_define_macro_multiline(name, params, body, per_line))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("start", type=int, help="Start N (inclusive)")
    ap.add_argument("max", type=int, help="Max N (inclusive)")
    ap.add_argument("out", type=str, help="Output header path")
    ap.add_argument("--macro-prefix", default="D_VARG_GET_ARG_", help="Macro name prefix")
    ap.add_argument("--per-line", type=int, default=10, help="Params per line before wrapping")
    ap.add_argument("--param-prefix", default="_", help="Prefix for parameter names")
    ap.add_argument("--param-start", type=int, default=1, help="Starting number for parameter indices")
    ap.add_argument("--param-step", type=int, default=1, help="Step amount for parameter indices")
    args = ap.parse_args()

    if args.start < 1:
        raise SystemExit("start must be >= 1")
    if args.max < args.start:
        raise SystemExit("max must be >= start")
    if not _is_c_ident(args.macro_prefix.rstrip("_")):
        raise SystemExit(f"--macro-prefix must begin with a C identifier (got {args.macro_prefix!r})")
    if not _is_c_ident(args.param_prefix):
        raise SystemExit(f"--param-prefix must be a C identifier prefix (got {args.param_prefix!r})")
    if args.param_step < 1:
        raise SystemExit(f"--param-step must be >= 1 (got {args.param_step})")

    pad_width = max(2, len(str(args.max)))

    lines: list[str] = []
    lines.append("/* Auto-generated. Do not edit by hand. */")
    lines.append("")
    lines.append("/* --- D_VARG_GET_ARG_XX --- */")
    lines.append("")

    for i in range(args.start, args.max + 1):
        lines.append(make_get_arg_macro(i, args.macro_prefix, pad_width, args.per_line, args.param_prefix, args.param_start, args.param_step))
        lines.append("")

    Path(args.out).write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    print(f"Wrote {args.out} (N={args.start}..{args.max}, prefix={args.macro_prefix}, param_prefix={args.param_prefix}, param_start={args.param_start}, param_step={args.param_step})")


if __name__ == "__main__":
    main()
