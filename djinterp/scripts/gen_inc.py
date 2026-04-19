#!/usr/bin/env python3
"""
gen_inc_headers.py

Generates chained increment headers:

  inc64.h   (expected to already exist)
  inc128.h  includes inc64.h
  inc256.h  includes inc128.h
  inc512.h  includes inc256.h
  inc1024.h includes inc512.h

Each header defines:
  #define D_INTERNAL_INC_N (N+1)

…but only for the new range in that file:
  inc128.h  -> 64..127   (values 65..128)
  inc256.h  -> 128..255  (values 129..256)
  inc512.h  -> 256..511  (values 257..512)
  inc1024.h -> 512..1023 (values 513..1024)

Formatting matches inc64.h style: values are aligned in a column.

Usage:
  python gen_inc_headers.py /path/to/output/dir

Notes:
- This script does not (re)generate inc64.h; it assumes it exists in output dir.
- Output headers are created/overwritten.
"""

from __future__ import annotations

from pathlib import Path
import sys


def write_inc_header(out_path: Path, include_name: str, start: int, end: int) -> None:
    assert 0 <= start <= end
    names = [f"D_INTERNAL_INC_{i}" for i in range(start, end + 1)]
    max_len = max(len(n) for n in names)

    lines: list[str] = [f'#include "{include_name}"', ""]

    for i in range(start, end + 1):
        name = f"D_INTERNAL_INC_{i}"
        spaces = " " * (max_len - len(name) + 1)
        lines.append(f"#define {name}{spaces}{i + 1}")

    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main(argv: list[str]) -> int:
    out_dir = Path(argv[1]) if len(argv) >= 2 else Path(".")
    out_dir.mkdir(parents=True, exist_ok=True)

    base = out_dir / "inc64.h"
    if not base.exists():
        print(f"error: {base} not found (this script expects inc64.h to already exist).", file=sys.stderr)
        return 2

    targets = [
        # total, start, end, include
        (128, 64, 127, "inc64.h"),
        (256, 128, 255, "inc128.h"),
        (512, 256, 511, "inc256.h"),
        (1024, 512, 1023, "inc512.h"),
    ]

    for total, start, end, include in targets:
        out_path = out_dir / f"inc{total}.h"
        write_inc_header(out_path, include, start, end)
        print(f"wrote {out_path}  ({start}..{end})")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
