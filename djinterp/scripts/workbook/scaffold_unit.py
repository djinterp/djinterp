#!/usr/bin/env python3
"""
Scaffold a new <unit>.json file with one stub symbol per name supplied on
the command line. Run, then hand-edit the resulting file to fill in the
real per-symbol fields.

Usage:
    python3 scaffold_unit.py \\
        --data-dir docs/djinterp/functional/data \\
        --header   producer.hpp \\
        --primary-in C++11 \\
        --symbol "producers::iterate" "Producer factory" \\
        --symbol "producers::unfold"  "Producer factory" \\
        --symbol "producers::range"   "Producer factory"

The output is a parseable JSON file with primary_in/constexpr_in_primary
set to --primary-in for every symbol, and every other field at its default
(empty / null). Hand-edit to fill in real notes, raise constexpr_in_primary
where it actually shifts, and so on.
"""

import argparse
import json
import os


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument("--data-dir", required=True)
    p.add_argument("--header",   required=True,
                   help="Filename for the unit (e.g. 'producer.hpp'). "
                        "Used both as the 'header' field and as the JSON "
                        "filename (without .hpp it would not look like a "
                        "header — keep the extension).")
    p.add_argument("--subtitle",     default="",
                   help="Long description for the title bar.")
    p.add_argument("--legend-blurb", default="",
                   help="Short description for the Legend sheet.")
    p.add_argument("--primary-in",   default="C++11",
                   help="Default primary-tier minimum version applied to "
                        "every symbol.")
    p.add_argument("--symbol", action="append", nargs=2,
                   metavar=("NAME", "GROUP"), default=[],
                   help="Add one symbol with (name, group). May be passed "
                        "multiple times.")
    return p.parse_args()


STUB_FIELDS = {
    "name":                   None,
    "group":                  None,
    "primary_in":             None,
    "secondary_min":          None,
    "constexpr_in_primary":   None,
    "constexpr_in_secondary": None,
    "intrinsic_required":     False,
    "intrinsic_names":        "",
    "detection_macro":        "",
    "t_alias_in":             None,
    "v_var_in":               None,
    "deprecated_in":          None,
    "notes":                  "",
    "depends_on":             [],
    "failure_reason":         None,
}


def make_symbol(name, group, primary_in):
    sym = dict(STUB_FIELDS)
    sym["name"]                 = name
    sym["group"]                = group
    sym["primary_in"]           = primary_in
    sym["constexpr_in_primary"] = primary_in
    return sym


def main():
    args = parse_args()
    os.makedirs(args.data_dir, exist_ok=True)
    out_path = os.path.join(args.data_dir,
                            os.path.splitext(args.header)[0] + ".json")
    if os.path.exists(out_path):
        print(f"ERROR: {out_path} already exists. Refusing to overwrite.")
        raise SystemExit(1)

    data = {
        "header":       args.header,
        "subtitle":     args.subtitle,
        "legend_blurb": args.legend_blurb,
        "symbols":      [make_symbol(n, g, args.primary_in)
                         for n, g in args.symbol],
    }
    with open(out_path, "w") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    print(f"Scaffolded {out_path} with {len(args.symbol)} symbol(s).")
    print("Hand-edit to fill in notes, raise constexpr_in_primary where it "
          "shifts, and add failure_reason where applicable.")


if __name__ == "__main__":
    main()
