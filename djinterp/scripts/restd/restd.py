#!/usr/bin/env python3
"""
restd specialization of the generic coverage tooling.

The generic scripts (build_coverage_workbook.py, scaffold_unit.py) are
project-agnostic and speak in terms of a "primary" baseline and an optional
"secondary" back-port tier. restd is the canonical two-tier consumer:
primary = "std", secondary = "restd". This wrapper pins restd's conventions
in one place so day-to-day commands are short and consistent:

    # regenerate the workbook from the data dir
    python3 restd.py build

    # scaffold a new <header> unit with restd field names (std_in/restd_min)
    python3 restd.py scaffold --header span.hpp \\
        --symbol "span<T,Extent>" "view class" \\
        --symbol "dynamic_extent"  "constant"

    # (re)write the restd _config.json into the data dir
    python3 restd.py config

Defaults assume the restd layout:
    data dir : docs/restd/data
    output   : docs/restd/restd_coverage.xlsx
Override with --data-dir / --output on any subcommand.

This file holds NO rendering logic — that all lives in
build_coverage_workbook.py. restd.py only supplies restd's config and a
restd-flavoured scaffolder that emits the std_in / restd_min field names the
existing restd corpus uses (the generic loader accepts both spellings, so
either works; matching the corpus keeps diffs clean).
"""

import argparse
import json
import os

import build_coverage_workbook as bcw


# ----------------------------------------------------------------------------
# restd presets
# ----------------------------------------------------------------------------

DEFAULT_DATA_DIR = os.path.join("docs", "restd", "data")
DEFAULT_OUTPUT   = os.path.join("docs", "restd", "restd_coverage.xlsx")

# The contents of docs/restd/data/_config.json. Kept here so `restd.py config`
# can (re)generate it, and so the restd look is reproducible from this file
# alone. Only the keys that differ from build_coverage_workbook.DEFAULT_CONFIG
# are listed; _deep_merge fills in the rest.
RESTD_CONFIG = {
    "project_name":      "restd",
    "workbook_title":    "restd coverage matrix",
    "workbook_subtitle": ("Per-std-header coverage of restd across C++ "
                          "versions. Green = restd covers it, yellow = only "
                          "std has it (a coverage gap), red = no coverage. A "
                          "\"cx\" in a cell marks versions where the symbol is "
                          "usable in a constant expression."),
    "unit_label":        "Std Header",
    "unit_label_plural": "std headers",
    "tiers": {
        "primary":   {"label": "std"},
        "secondary": {"label": "restd"},
    },
    # merge the two version blocks into one and colour it library-centrically
    "support_coloring": "coverage",
    "category_case":    "lower",
    "columns": {
        "constexpr_matrix":        False,
        "support_shows_constexpr": True,
    },
}

# restd authors symbols with these field names (the generic schema's
# canonical names are primary_in / secondary_min / constexpr_in_*; the loader
# accepts both, but matching the corpus is cleaner).
RESTD_STUB_FIELDS = {
    "name":                None,
    "group":               None,
    "std_in":              None,
    "restd_min":           None,
    "constexpr_in_std":    None,
    "constexpr_in_restd":  None,
    "intrinsic_required":  False,
    "intrinsic_names":     "",
    "detection_macro":     "",
    "t_alias_in":          None,
    "v_var_in":            None,
    "deprecated_in":       None,
    "notes":               "",
    "depends_on":          [],
    "failure_reason":      None,
}


# ----------------------------------------------------------------------------
# config
# ----------------------------------------------------------------------------

def write_config(data_dir):
    os.makedirs(data_dir, exist_ok=True)
    path = os.path.join(data_dir, "_config.json")
    with open(path, "w") as f:
        json.dump(RESTD_CONFIG, f, indent=2, ensure_ascii=False)
        f.write("\n")
    return path


def cmd_config(args):
    path = write_config(args.data_dir)
    print(f"Wrote {path}")


# ----------------------------------------------------------------------------
# build
# ----------------------------------------------------------------------------

def cmd_build(args):
    # Ensure the restd config exists / is current before building, unless the
    # caller asked to use whatever _config.json is already present.
    if not args.keep_config:
        write_config(args.data_dir)
    wb = bcw.build(args.data_dir)
    out = os.path.abspath(args.output)
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    wb.save(out)
    print(f"Wrote {args.output}")
    print(f"  data:    {args.data_dir}")
    print(f"  sheets ({len(wb.sheetnames)}): {wb.sheetnames}")


# ----------------------------------------------------------------------------
# scaffold (restd field names)
# ----------------------------------------------------------------------------

def _make_symbol(name, group, std_in):
    sym = dict(RESTD_STUB_FIELDS)
    sym["name"]             = name
    sym["group"]            = group
    sym["std_in"]           = std_in
    sym["restd_min"]        = std_in
    sym["constexpr_in_std"] = std_in
    return sym


def cmd_scaffold(args):
    os.makedirs(args.data_dir, exist_ok=True)
    out_path = os.path.join(args.data_dir,
                            os.path.splitext(args.header)[0] + ".json")
    if os.path.exists(out_path):
        raise SystemExit(f"ERROR: {out_path} already exists. Refusing to "
                         f"overwrite.")
    data = {
        "header":       args.header,
        "subtitle":     args.subtitle,
        "legend_blurb": args.legend_blurb,
        "symbols":      [_make_symbol(n, g, args.std_in)
                         for n, g in args.symbol],
    }
    with open(out_path, "w") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print(f"Scaffolded {out_path} with {len(args.symbol)} symbol(s).")
    print("Hand-edit: fill in notes, set restd_min / failure_reason, and "
          "raise constexpr_in_restd where it actually shifts.")


# ----------------------------------------------------------------------------
# CLI
# ----------------------------------------------------------------------------

def main():
    p = argparse.ArgumentParser(description="restd coverage tooling.")
    sub = p.add_subparsers(dest="cmd", required=True)

    pb = sub.add_parser("build", help="Regenerate the restd coverage workbook.")
    pb.add_argument("--data-dir", default=DEFAULT_DATA_DIR)
    pb.add_argument("--output",   default=DEFAULT_OUTPUT)
    pb.add_argument("--keep-config", action="store_true",
                    help="Use the _config.json already in --data-dir instead "
                         "of rewriting restd's.")
    pb.set_defaults(func=cmd_build)

    pc = sub.add_parser("config", help="Write restd's _config.json.")
    pc.add_argument("--data-dir", default=DEFAULT_DATA_DIR)
    pc.set_defaults(func=cmd_config)

    ps = sub.add_parser("scaffold", help="Create a new <header>.json stub.")
    ps.add_argument("--data-dir", default=DEFAULT_DATA_DIR)
    ps.add_argument("--header", required=True,
                    help="Unit filename, e.g. 'span.hpp'. Used as the header "
                         "field and the JSON filename.")
    ps.add_argument("--subtitle",     default="")
    ps.add_argument("--legend-blurb", default="")
    ps.add_argument("--std-in", default="C++11",
                    help="Default std_in / restd_min / constexpr_in_std for "
                         "every scaffolded symbol.")
    ps.add_argument("--symbol", action="append", nargs=2,
                    metavar=("NAME", "GROUP"), default=[],
                    help="Add one symbol (name, group). Repeatable.")
    ps.set_defaults(func=cmd_scaffold)

    args = p.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
