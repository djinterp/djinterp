#!/usr/bin/env python3
"""
functional_workbook.py - build the djinterp::functional coverage workbook.

A thin project wrapper around build_coverage_workbook.build(). It bakes the
djinterp::functional *presentation criteria* into code - the same settings
that otherwise live in _config.json - so the workbook regenerates with a
single command and without depending on a _config.json in the data dir.

This mirrors how the generic builder's docstring envisions a per-project
front-end (e.g. a restd.py): the JSON files remain the single source of
truth for the data; this file owns only the project's look + terminology.

Usage:
    python3 functional_workbook.py \
        --data-dir docs/djinterp/functional/data \
        --output   docs/djinterp/functional/functional_coverage.xlsx

    # Excel-clean output (shared-string table) via LibreOffice:
    python3 functional_workbook.py --data-dir <dir> --output <file> --normalize

The data dir must contain one *.json per header (the 19 existing + the 12
typeclass files) plus the optional _roadmap.json. _config.json is no longer
required - FUNCTIONAL_CONFIG below is deep-merged last and wins - but a stray
one is harmless.
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

from build_coverage_workbook import build


# ---------------------------------------------------------------------------
# djinterp::functional presentation criteria  (mirrors _config.json)
# ---------------------------------------------------------------------------
# build() deep-merges this LAST, on top of DEFAULT_CONFIG and any on-disk
# _config.json, so these values win. Single-tier project: the library *is*
# the thing being measured, so there is no "secondary" back-port tier and
# support cells render green (available) / red (not), with "cx" marking
# constexpr-eligibility inside the same cell.
FUNCTIONAL_CONFIG = {
    "project_name":      "djinterp::functional",
    "workbook_title":    "djinterp functional - version & constexpr coverage",
    "workbook_subtitle": ("Per-header coverage of the djinterp functional "
                          "subframework across C++ language versions (C++11 "
                          "baseline), with constexpr eligibility tracked "
                          "separately. One sheet per header file (alphabetical)."),

    "unit_label":        "Header file",
    "unit_label_plural": "header files",

    "tiers": {
        "primary":   {"label": "djinterp"},
        "secondary": None,
    },

    "columns": {
        "primary_in":              True,
        "support_matrix":          True,
        "constexpr_matrix":        False,
        "support_shows_constexpr": True,
        "t_alias":                 False,
        "v_var":                   False,
        "deprecated":              False,
        "intrinsic":               False,
        "intrinsic_names":         False,
        "detection_macro":         False,
        "notes":                   True,
    },

    "versions": ["C++11", "C++14", "C++17", "C++20", "C++23", "C++26"],

    "sheets": {
        "legend":       True,
        "todo":         True,
        "pending_deps": True,
        "failures":     True,
    },
}


def parse_args():
    here = os.path.dirname(os.path.abspath(__file__))
    p = argparse.ArgumentParser(
        description="Build the djinterp::functional coverage workbook.")
    p.add_argument("--data-dir", default=os.path.join(here, "data"),
                   help="Directory of per-header *.json (+ optional "
                        "_roadmap.json). Default: ./data next to this script.")
    p.add_argument("--output", default="functional_coverage.xlsx",
                   help="Output .xlsx path. Default: ./functional_coverage.xlsx")
    p.add_argument("--normalize", action="store_true",
                   help="Round-trip the result through LibreOffice so it is "
                        "written with a shared-string table - the form Excel "
                        "opens most reliably (openpyxl emits inline strings, "
                        "which some Excel builds prompt to 'repair'; content is "
                        "identical). Requires soffice/libreoffice on PATH.")
    return p.parse_args()


def normalize_with_libreoffice(path):
    """Rewrite `path` in place via LibreOffice headless conversion."""
    soffice = shutil.which("soffice") or shutil.which("libreoffice")
    if not soffice:
        print("  --normalize requested but soffice/libreoffice was not found "
              "on PATH; leaving the openpyxl workbook as-is.", file=sys.stderr)
        return
    # Convert into a temp dir, then move over the original, so LibreOffice
    # never has to overwrite its own input.
    with tempfile.TemporaryDirectory() as td:
        subprocess.run([soffice, "--headless", "--convert-to", "xlsx",
                        "--outdir", td, path],
                       check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        produced = os.path.join(td, os.path.basename(path))
        if not os.path.isfile(produced):
            print("  LibreOffice did not produce an output file; leaving the "
                  "openpyxl workbook as-is.", file=sys.stderr)
            return
        shutil.move(produced, path)
    print(f"  normalized via LibreOffice (shared-string table) -> {path}")


def main():
    args = parse_args()
    wb = build(args.data_dir, config=FUNCTIONAL_CONFIG)
    out = os.path.abspath(args.output)
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    wb.save(out)
    print(f"Wrote {out}")
    print(f"  data:   {args.data_dir}")
    print(f"  sheets ({len(wb.sheetnames)}): {wb.sheetnames}")
    if args.normalize:
        normalize_with_libreoffice(out)


if __name__ == "__main__":
    main()
