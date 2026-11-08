#!/usr/bin/env python3
"""gen_env_printer_features.py -- write env_printer.h's C++ feature table.

env_printer.h prints every C++ feature-test result env_cpp_features.h
detects, from a table with one line per feature. That table is generated
here, from the features env_cpp_features.h defines -- every
D_ENV_CPP_FEATURE_<X>_DESC, in the header's order, each of which has its
_NAME, _VERS and _VAL beside it -- into the block between env_printer.h's
markers:

    // BEGIN GENERATED: scripts/env/gen_env_printer_features.py
    ...
    // END GENERATED: scripts/env/gen_env_printer_features.py

Everything outside the markers is written by hand. A feature added to
env_cpp_features.h reaches the printer by running this again.

Usage:
    python3 scripts/env/gen_env_printer_features.py           rewrite the block
    python3 scripts/env/gen_env_printer_features.py --check   exit 1 if stale
"""

import pathlib
import re
import sys

ROOT     = pathlib.Path(__file__).resolve().parents[2]
FEATURES = ROOT / "inc/djinterp/env/cpp/env_cpp_features.h"
PRINTER  = ROOT / "inc/djinterp/env/env_printer.h"
BEGIN    = "// BEGIN GENERATED: scripts/env/gen_env_printer_features.py"
END      = "// END GENERATED: scripts/env/gen_env_printer_features.py"
INDENT   = "        "


def features():
    """The feature names, in env_cpp_features.h's order, each complete."""
    text  = FEATURES.read_bytes().decode("utf-8").replace("\r\n", "\n")
    names = re.findall(r"#define D_ENV_CPP_FEATURE_(\w+)_DESC ", text)

    for name in names:
        for part in ("NAME", "VERS", "VAL"):
            if f"D_ENV_CPP_FEATURE_{name}_{part} " not in text:
                sys.exit(f"{name}: no _{part} in {FEATURES.name}")

    return names


def block(names):
    """The generated lines, markers included."""
    lines = [INDENT + BEGIN]

    for i, name in enumerate(names):
        comma = "," if i + 1 < len(names) else ""
        lines.append(f"{INDENT}D_INTERNAL_ENV_PRINTER_FEATURE({name}){comma}")

    lines.append(INDENT + END)

    return "\n".join(lines)


def main():
    raw   = PRINTER.read_bytes().decode("utf-8")
    crlf  = "\r\n" in raw
    text  = raw.replace("\r\n", "\n")
    start = text.find(INDENT + BEGIN)
    stop  = text.find(END)

    if (start < 0) or (stop < 0):
        sys.exit(f"{PRINTER.name}: the generated block's markers are missing")

    stop    = stop + len(END)
    updated = text[:start] + block(features()) + text[stop:]

    if "--check" in sys.argv[1:]:
        if updated != text:
            print(f"{PRINTER.name}: the C++ feature table is stale")
            return 1

        print(f"{PRINTER.name}: the C++ feature table is current")
        return 0

    if updated != text:
        # the file's own line endings: the tree is committed with CRLF
        out = updated.replace("\n", "\r\n") if crlf else updated
        PRINTER.write_bytes(out.encode("utf-8"))
        print(f"{PRINTER.name}: rewrote the C++ feature table")

    return 0


if __name__ == "__main__":
    sys.exit(main())
