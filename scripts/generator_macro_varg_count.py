# gen_varg_count.py
import argparse
from pathlib import Path


def _is_c_ident(s: str) -> bool:
    return s and (s[0].isalpha() or s[0] == "_") and all(ch.isalnum() or ch == "_" for ch in s)


def _chunk(items: list[str], n: int) -> list[list[str]]:
    return [items[i : i + n] for i in range(0, len(items), n)]


def _emit_define_with_wrapped_params(name: str, params: list[str], per_line: int, tail: str) -> list[str]:
    """
    Emits:
      #define NAME( \
          p0, p1, ... \
          pK, ...) tail
    where 'tail' is the text after the closing paren.
    """
    if per_line < 1:
        per_line = 1

    groups = _chunk(params, per_line)
    lines: list[str] = []
    lines.append(f"#define {name}( \\")
    for gi, grp in enumerate(groups):
        is_last = (gi == len(groups) - 1)
        joined = ", ".join(grp)
        if is_last:
            lines.append(f"    {joined}) {tail}")
        else:
            lines.append(f"    {joined}, \\")
    return lines


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("max", type=int, help="Maximum count supported (e.g. 127 for 0..127)")
    ap.add_argument("out", type=str, help="Output header path")
    ap.add_argument("--helper-name", default="D_INTERNAL_VARG_COUNT_HELPER", help="Helper macro name")
    ap.add_argument("--count-name", default="D_VARG_COUNT", help="Public count macro name")
    ap.add_argument("--placeholder", default="_", help="Leading placeholder argument (C identifier recommended)")
    ap.add_argument("--params-per-line", type=int, default=12, help="Helper params per line")
    ap.add_argument("--nums-per-line", type=int, default=13, help="Descending numbers per line")
    ap.add_argument("--param-prefix", default="_", help="Prefix for parameter names")
    ap.add_argument("--param-start", type=int, default=0, help="Starting number for parameter indices")
    ap.add_argument("--param-step", type=int, default=1, help="Step amount for parameter indices")
    args = ap.parse_args()

    if args.max < 0:
        raise SystemExit("max must be >= 0")
    if not _is_c_ident(args.helper_name):
        raise SystemExit(f"--helper-name must be a C identifier (got {args.helper_name!r})")
    if not _is_c_ident(args.count_name):
        raise SystemExit(f"--count-name must be a C identifier (got {args.count_name!r})")
    if args.placeholder and (not _is_c_ident(args.placeholder)):
        raise SystemExit(f"--placeholder must be a C identifier (got {args.placeholder!r})")
    if args.params_per_line < 1 or args.nums_per_line < 1:
        raise SystemExit("--params-per-line and --nums-per-line must be >= 1")
    if not _is_c_ident(args.param_prefix):
        raise SystemExit(f"--param-prefix must be a C identifier prefix (got {args.param_prefix!r})")
    if args.param_step < 1:
        raise SystemExit(f"--param-step must be >= 1 (got {args.param_step})")

    # Helper params: prefix0..prefixmax, N, ...
    helper_params = [f"{args.param_prefix}{args.param_start + i * args.param_step}" for i in range(0, args.max + 1)] + ["N", "..."]
    helper_lines = _emit_define_with_wrapped_params(
        args.helper_name,
        helper_params,
        args.params_per_line,
        tail="N",
    )

    # Descending numbers: max..0 (length max+1)
    nums = [str(i) for i in range(args.max, -1, -1)]
    num_groups = _chunk(nums, args.nums_per_line)

    lines: list[str] = []
    lines.append("/* Auto-generated. Do not edit by hand. */")
    lines.append("")
    lines.append("/* --- D_INTERNAL_VARG_COUNT_HELPER + D_VARG_COUNT --- */")
    lines.append("")
    lines.extend(helper_lines)
    lines.append("")
    lines.append(f"#define {args.count_name}(...) \\")
    lines.append(f"    {args.helper_name}({args.placeholder}, __VA_ARGS__, \\")

    for gi, grp in enumerate(num_groups):
        is_last = (gi == len(num_groups) - 1)
        joined = ", ".join(grp)
        if is_last:
            lines.append(f"        {joined})")
        else:
            lines.append(f"        {joined}, \\")

    Path(args.out).write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {args.out} (max={args.max}, helper={args.helper_name}, count={args.count_name}, param_prefix={args.param_prefix}, param_start={args.param_start}, param_step={args.param_step})")


if __name__ == "__main__":
    main()
