# generator_macro_varg_has_args.py
#   Writes D_HAS_ARGS for C-Macro-Generator.ps1: D_INTERNAL_HAS_ARGS_HELPER
# takes the placeholders, N and the rest, and selects N; D_HAS_ARGS passes
# __VA_ARGS__ followed by one 1 per placeholder and a final 0.
#   Without --mvc there are max+1 placeholders, _0 to _max; with --mvc (MSVC's
# capped tables) there are max, _1 to _max.
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import macro_frame  # noqa: E402


def _is_c_ident(s: str) -> bool:
    return bool(s) and (s[0].isalpha() or s[0] == "_") and all(ch.isalnum() or ch == "_" for ch in s)


def _chunk(items: list[str], n: int) -> list[list[str]]:
    return [items[i : i + n] for i in range(0, len(items), n)]


def _emit_wrapped(head: str, items: list[str], per_line: int, indent: str, tail: str) -> list[str]:
    """
    Emits head, then items per_line to a line, each line but the last ending
    in ", \\", the last in tail.
    """
    lines: list[str] = [head]
    groups = _chunk(items, max(per_line, 1))
    for gi, grp in enumerate(groups):
        joined = ", ".join(grp)
        if gi == len(groups) - 1:
            lines.append(f"{indent}{joined}{tail}")
        else:
            lines.append(f"{indent}{joined}, \\")
    return lines


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("max", type=int, help="Maximum argument count supported")
    ap.add_argument("out", type=str, help="Output header path")
    ap.add_argument("--mvc", action="store_true", help="MSVC's capped form: placeholders _1.._max")
    ap.add_argument("--helper-name", default="D_INTERNAL_HAS_ARGS_HELPER", help="Helper macro name")
    ap.add_argument("--name", default="D_HAS_ARGS", help="Public macro name")
    ap.add_argument("--params-per-line", type=int, default=12, help="Helper params per line")
    ap.add_argument("--values-per-line", type=int, default=16, help="Trailing values per line")
    ap.add_argument("--param-prefix", default="_", help="Prefix for parameter names")
    args = ap.parse_args()

    if args.max < 1:
        raise SystemExit("max must be >= 1")
    for flag, value in (("--helper-name", args.helper_name), ("--name", args.name)):
        if not _is_c_ident(value):
            raise SystemExit(f"{flag} must be a C identifier (got {value!r})")
    if not _is_c_ident(args.param_prefix):
        raise SystemExit(f"--param-prefix must be a C identifier prefix (got {args.param_prefix!r})")
    if args.params_per_line < 1 or args.values_per_line < 1:
        raise SystemExit("--params-per-line and --values-per-line must be >= 1")

    first = 1 if args.mvc else 0
    params = [f"{args.param_prefix}{i}" for i in range(first, args.max + 1)]
    ones = len(params)

    lines: list[str] = []
    lines.append("/* Auto-generated. Do not edit by hand. */")
    lines.append(f"/* {args.name}: max={args.max}, placeholders={len(params)}, trailing_ones={ones} */")
    lines.append("")
    lines.append(f"/* --- {args.helper_name} + {args.name} --- */")
    lines.append("")
    lines.extend(_emit_wrapped(f"#define {args.helper_name}( \\",
                               params + ["N", "..."], args.params_per_line,
                               "    ", ") N"))
    lines.append("")
    lines.append(f"#define {args.name}(...) \\")
    lines.extend(_emit_wrapped(f"    {args.helper_name}(__VA_ARGS__, \\",
                               ["1"] * ones + ["0"], args.values_per_line,
                               "        ", ")"))

    changed = macro_frame.write(args.out, "\n".join(lines) + "\n")
    print(f"{'Wrote' if changed else 'Unchanged'} {args.out} (max={args.max}, placeholders={len(params)}, mvc={args.mvc})")


if __name__ == "__main__":
    main()
