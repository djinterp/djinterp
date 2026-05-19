"""
One-shot converter: dumps the in-Python SYMBOLS_* tables in
coverage_data.py to one JSON file per header under docs/restd/data/.

Run once; thereafter docs/restd/data/*.json is the single source of
truth and coverage_data.py can be retired.

Each JSON file is a single object:
{
  "header":       "<ranges>",
  "subtitle":     "...long subtitle for the title bar...",
  "legend_blurb": "...short blurb for the Legend sheet listing...",
  "symbols":      [ {...}, {...}, ... ]
}

Each symbol object carries every field of coverage_data.Symbol with
sensible JSON encoding: None -> null, tuple -> array, "" -> "".
"""

import json
import os

import coverage_data as cd

# Resolve docs/restd/data/ relative to this script's location.
_HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.normpath(os.path.join(_HERE, "..", ".."))
DATA_DIR = os.path.join(PROJECT_ROOT, "docs", "restd", "data")
os.makedirs(DATA_DIR, exist_ok=True)


def symbol_to_dict(sym):
    return {
        "name":               sym.name,
        "group":              sym.group,
        "std_in":             sym.std_in,
        "restd_min":          sym.restd_min,
        "constexpr_in_std":   sym.constexpr_in_std,
        "constexpr_in_restd": sym.constexpr_in_restd,
        "intrinsic_required": bool(sym.intrinsic_required),
        "intrinsic_names":    sym.intrinsic_names,
        "detection_macro":    sym.detection_macro,
        "t_alias_in":         sym.t_alias_in,
        "v_var_in":           sym.v_var_in,
        "deprecated_in":      sym.deprecated_in,
        "notes":              sym.notes,
        "depends_on":         list(sym.depends_on),
        "failure_reason":     sym.failure_reason,
    }


# Build a (header -> legend_blurb) map from LEGEND_SHEETS so we can
# co-locate the blurb in each JSON file.
LEGEND_BLURB = {name: blurb for name, blurb in cd.LEGEND_SHEETS}


def header_filename(header_name):
    # "<ranges>" -> "ranges.json"
    return header_name.strip("<>").replace("/", "_") + ".json"


for header_name, syms in cd.HEADERS.items():
    obj = {
        "header":       header_name,
        "subtitle":     cd.HEADER_SUBTITLES.get(header_name, ""),
        "legend_blurb": LEGEND_BLURB.get(header_name, ""),
        "symbols":      [symbol_to_dict(s) for s in syms],
    }
    path = os.path.join(DATA_DIR, header_filename(header_name))
    with open(path, "w") as f:
        json.dump(obj, f, indent=2, ensure_ascii=False)
    print(f"Wrote {path} ({len(syms)} symbols)")

print()
print(f"Done. {len(cd.HEADERS)} files written to {DATA_DIR}/")
