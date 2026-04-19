# gen_inc.py
import argparse
from pathlib import Path

def _is_c_ident(s: str) -> bool:
    # Simple "good enough" identifier check (covers typical C idents)
    return s and (s[0].isalpha() or s[0] == "_") and all(ch.isalnum() or ch == "_" for ch in s)

def inc_line(n: int, macro_prefix: str, offset: int, param_start: int) -> str:
    """
    Generate a single increment macro definition.
    
    Args:
        n: The parameter index (e.g., 0, 1, 2...)
        macro_prefix: The macro name prefix (e.g., "D_INTERNAL_INC_")
        offset: The offset to apply (e.g., +1 means INC_0 -> 1, -5 means INC_5 -> 0)
        param_start: The starting parameter index
    
    Returns:
        The macro definition line
    """
    param_idx = param_start + n
    value = param_idx + offset
    return f"#define {macro_prefix}{param_idx} {value}"

def main() -> None:
    ap = argparse.ArgumentParser(
        description="Generate increment/decrement macros. "
        "Example: INC_0 -> 1, INC_1 -> 2, etc. with offset +1, "
        "or INC_5 -> 0, INC_6 -> 1, etc. with offset -5"
    )
    ap.add_argument("start", type=int, help="Start N (inclusive, number of macros to generate from start)")
    ap.add_argument("max", type=int, help="Max N (inclusive, number of macros to generate through max)")
    ap.add_argument("out", type=str, help="Output header path")
    ap.add_argument("--macro-prefix", default="D_INTERNAL_INC_", help="Macro name prefix (default: D_INTERNAL_INC_)")
    ap.add_argument("--offset", type=int, default=1, help="Offset to apply to each value (default: +1). "
                    "Positive offset: INC_0 -> 1, INC_1 -> 2. "
                    "Negative offset: INC_5 -> 0 (with offset -5)")
    ap.add_argument("--param-start", type=int, default=0, help="Starting parameter index (default: 0)")
    ap.add_argument("--param-step", type=int, default=1, help="Step between parameter indices (default: 1)")
    args = ap.parse_args()

    if args.start < 0:
        raise SystemExit("start must be >= 0")
    if args.max < args.start:
        raise SystemExit("max must be >= start")
    if not _is_c_ident(args.macro_prefix.rstrip("_")):
        raise SystemExit(f"--macro-prefix must begin with a C identifier (got {args.macro_prefix!r})")
    if args.param_step < 1:
        raise SystemExit(f"--param-step must be >= 1 (got {args.param_step})")

    lines = []
    lines.append("/* Auto-generated. Do not edit by hand. */")
    lines.append("")
    lines.append(f"/* --- {args.macro_prefix}N (offset: {args.offset:+d}) --- */")

    # Generate range from start to max inclusive
    num_macros = args.max - args.start + 1
    for i in range(num_macros):
        lines.append(inc_line(i, args.macro_prefix, args.offset, args.param_start))

    Path(args.out).write_text("\n".join(lines) + "\n", encoding="utf-8")
    
    # Calculate the actual range for display
    first_param = args.param_start
    last_param = args.param_start + (num_macros - 1) * args.param_step
    first_value = first_param + args.offset
    last_value = last_param + args.offset
    
    print(f"Wrote {args.out} (N={args.start}..{args.max}, "
          f"prefix={args.macro_prefix}, offset={args.offset:+d}, "
          f"param_start={args.param_start}, param_step={args.param_step})")
    print(f"  Generated: {args.macro_prefix}{first_param} -> {first_value}, "
          f"..., {args.macro_prefix}{last_param} -> {last_value}")

if __name__ == "__main__":
    main()
