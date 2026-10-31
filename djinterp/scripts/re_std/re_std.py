#!/usr/bin/env python3
"""
re_std specialization of the generic coverage tooling.

The generic scripts (scripts/workbook/build_coverage_workbook.py and
scaffold_unit.py) are project-agnostic and speak of a "primary" baseline and
a "secondary" back-port tier. re_std is the canonical two-tier consumer --
primary = "std", secondary = "re_std" -- and keeps two workbooks, one per
language tree, laid out as the code is:

    docs/re_std/cpp/data/            C++ data  -> re_std_cpp_coverage.xlsx
    docs/re_std/c/data/              C data    -> re_std_c_coverage.xlsx

This wrapper pins re_std's configuration for each (RE_STD_CONFIG,
RE_STD_C_CONFIG) so day-to-day commands stay short:

    # regenerate a workbook (C++ by default)
    python3 scripts/re_std/re_std.py build
    python3 scripts/re_std/re_std.py build --lang c

    # print the configuration a build uses
    python3 scripts/re_std/re_std.py show-config [--lang c]

    # (re)write <data dir>/_config.json, so a direct run of the generic
    # builder looks the same as a run through this wrapper
    python3 scripts/re_std/re_std.py config [--lang c]

    # scaffold a new <header>.json unit
    python3 scripts/re_std/re_std.py scaffold --header span.hpp \\
        --symbol "span<T,Extent>" "view class" \\
        --symbol "dynamic_extent"  "constant"

--data-dir and --output override the defaults on any subcommand.

The configuration is passed to build_coverage_workbook.build() in memory
(its third layer), so a build never rewrites the _config.json beside the
data. This file holds NO rendering logic; that all lives in
build_coverage_workbook.py.
"""

import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "workbook"))

import build_coverage_workbook as bcw  # noqa: E402


# ----------------------------------------------------------------------------
# re_std presets
# ----------------------------------------------------------------------------

LANGS = ("cpp", "c")


def default_data_dir(lang):
    return os.path.join("docs", "re_std", lang, "data")


def default_output(lang):
    return os.path.join("docs", "re_std", lang, f"re_std_{lang}_coverage.xlsx")


# Only the keys that differ from build_coverage_workbook.DEFAULT_CONFIG are
# listed; the builder deep-merges the rest.
RE_STD_CONFIG = {
    "project_name": "re_std",
    "workbook_title": "re_std coverage matrix",
    "workbook_subtitle": "Per-std-header coverage of re_std across C++ versions. Green = re_std covers it, yellow = only std has it (a coverage gap), red = no coverage. A \"cx\" in a cell marks versions where the symbol is usable in a constant expression.",
    "unit_label": "Std Header",
    "unit_label_plural": "std headers",
    "tiers": {
        "primary": {
            "label": "std"
        },
        "secondary": {
            "label": "re_std"
        }
    },
    "support_coloring": "coverage",
    "category_case": "lower",
    "columns": {
        "constexpr_matrix": False,
        "support_shows_constexpr": True
    }
}

RE_STD_C_CONFIG = {
    "project_name": "re_std",
    "workbook_title": "re_std C coverage matrix",
    "workbook_subtitle": "Per-std-header coverage of re_std across C versions. Green = re_std covers it, yellow = only std has it (a coverage gap), red = no coverage. A \"cx\" in a cell marks versions where the macro is an integer constant expression usable in #if.",
    "unit_label": "Std Header",
    "unit_label_plural": "std headers",
    "tiers": {
        "primary": {
            "label": "std"
        },
        "secondary": {
            "label": "re_std"
        }
    },
    "support_coloring": "coverage",
    "category_case": "lower",
    "language_label": "C",
    "versions": [
        "C90",
        "C95",
        "C99",
        "C11",
        "C17",
        "C23",
        "C2y"
    ],
    "columns": {
        "constexpr_matrix": False,
        "support_shows_constexpr": True,
        "t_alias": False,
        "v_var": False
    }
}

CONFIGS = {"cpp": RE_STD_CONFIG, "c": RE_STD_C_CONFIG}

# Symbol stub for `scaffold`. These are the names the existing C++ corpus was
# first written with (std_in / restd_min); the builder accepts them as
# aliases of the canonical primary_in / secondary_min. Convert a scaffolded
# file to the canonical names when it is filled in, as MODULE_WORKFLOW.md
# describes.
RE_STD_STUB_FIELDS = {
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

def write_config(lang, data_dir):
    os.makedirs(data_dir, exist_ok=True)
    path = os.path.join(data_dir, "_config.json")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(CONFIGS[lang], f, indent=2, ensure_ascii=False)
        f.write("\n")
    return path


def cmd_config(args):
    print(f"Wrote {write_config(args.lang, args.data_dir)}")


def cmd_show_config(args):
    print(json.dumps(CONFIGS[args.lang], indent=2, ensure_ascii=False))


# ----------------------------------------------------------------------------
# build
# ----------------------------------------------------------------------------

def cmd_build(args):
    wb = bcw.build(args.data_dir, config=CONFIGS[args.lang])
    out = os.path.abspath(args.output)
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    wb.save(out)
    print(f"Wrote {args.output}")
    print(f"  data:    {args.data_dir}")
    print(f"  sheets ({len(wb.sheetnames)}): {wb.sheetnames}")


# ----------------------------------------------------------------------------
# scaffold
# ----------------------------------------------------------------------------

def _make_symbol(name, group, std_in):
    sym = dict(RE_STD_STUB_FIELDS)
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
    std_in = args.std_in or CONFIGS[args.lang].get(
        "versions", bcw.DEFAULT_CONFIG["versions"])[0]
    data = {
        "header":       args.header,
        "subtitle":     args.subtitle,
        "legend_blurb": args.legend_blurb,
        "symbols":      [_make_symbol(n, g, std_in) for n, g in args.symbol],
    }
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print(f"Scaffolded {out_path} with {len(args.symbol)} symbol(s).")
    print("Hand-edit: fill in notes, set restd_min / failure_reason, and "
          "raise constexpr_in_restd where it actually shifts.")


# ----------------------------------------------------------------------------
# CLI
# ----------------------------------------------------------------------------

def main():
    p = argparse.ArgumentParser(description="re_std coverage tooling.")
    sub = p.add_subparsers(dest="cmd", required=True)

    def common(sp, output=False):
        sp.add_argument("--lang", choices=LANGS, default="cpp",
                        help="Which workbook: cpp (default) or c.")
        sp.add_argument("--data-dir", default=None)
        if output:
            sp.add_argument("--output", default=None)

    pb = sub.add_parser("build", help="Regenerate a coverage workbook.")
    common(pb, output=True)
    pb.set_defaults(func=cmd_build)

    pc = sub.add_parser("config", help="Write <data dir>/_config.json.")
    common(pc)
    pc.set_defaults(func=cmd_config)

    pv = sub.add_parser("show-config", help="Print the configuration.")
    common(pv)
    pv.set_defaults(func=cmd_show_config)

    ps = sub.add_parser("scaffold", help="Create a new <header>.json stub.")
    common(ps)
    ps.add_argument("--header", required=True,
                    help="Unit filename, e.g. 'span.hpp' or 'stdbit.h'. Used "
                         "as the header field and the JSON filename.")
    ps.add_argument("--subtitle",     default="")
    ps.add_argument("--legend-blurb", default="")
    ps.add_argument("--std-in", default=None,
                    help="Default std_in / restd_min / constexpr_in_std for "
                         "every scaffolded symbol; the first version of the "
                         "language's axis if omitted.")
    ps.add_argument("--symbol", action="append", nargs=2,
                    metavar=("NAME", "GROUP"), default=[],
                    help="Add one symbol (name, group). Repeatable.")
    ps.set_defaults(func=cmd_scaffold)

    args = p.parse_args()
    if args.data_dir is None:
        args.data_dir = default_data_dir(args.lang)
    if getattr(args, "output", "unset") is None:
        args.output = default_output(args.lang)
    args.func(args)


if __name__ == "__main__":
    main()
