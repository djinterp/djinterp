#!/usr/bin/env python3
"""
generator_for_each_full_v4.py

Generate djinterp-style "for-each" macro families and tuple-grouped variants.

Families (internals):
  - D_INTERNAL_FOR_EACH_N
  - D_INTERNAL_FOR_EACH_SEPARATOR_N
  - D_INTERNAL_FOR_EACH_PAIR_N / D_INTERNAL_FOR_EACH_PAIR_SEPARATOR_N   (even N only; odd => error token)
  - D_INTERNAL_FOR_EACH_TRIPLE_N / D_INTERNAL_FOR_EACH_TRIPLE_SEPARATOR_N (N divisible by 3; otherwise error token)
  - D_INTERNAL_FOR_EACH_DATA_SEPARATOR_N  (fn(data, arg_i) joined by sep)
  - Tuple families (arity k = 1..16 by default):
      D_INTERNAL_FOR_EACH_%_TUPLE_SEPARATOR_#   (default naming; % = arity, # = arg-count)

Wrappers (optional; require D_CONCAT + D_VARG_COUNT from your macro library):
  - D_FOR_EACH
  - D_FOR_EACH_SEPARATOR
  - D_FOR_EACH_COMMA / _SEMICOLON
  - D_FOR_EACH_PAIR / _PAIR_SEPARATOR
  - D_FOR_EACH_TRIPLE / _TRIPLE_SEPARATOR
  - D_FOR_EACH_DATA_SEPARATOR
  - D_FOR_EACH_<k>_TUPLE_SEPARATOR (for selected tuple arities)

Targets / multi-file:
  - --max N emits a single output (name may include '#', which will be substituted with N)
  - --intervals "(start,end,step)" ... emits one output per interval (substitute '#' with either interval-max or each)
  - --tuple-per-file emits one file per (tuple-arity, target), substituting '%' in the filename.

MSVC safety:
  - --msvc caps each family so its macro signature stays within 127 arguments.
    (e.g. *_SEPARATOR families cap N <= 125 because they take (_fn,_sep, ...).)
  - In --msvc mode, wrappers are disabled by default (classic D_VARG_COUNT approaches
    cannot dispatch past ~62 args under the 127-arg limit). Use --wrappers to force them.

Interval semantics:
  - This script treats (start,end,step) as inclusive start & inclusive end.
    (Your PowerShell front-end can adjust for exclusive endpoints before passing them.)

New in v4:
  - File/group selection (--only, --exclude)
  - Alias support (--alias)
  - Overwrite control (--overwrite, --no-overwrite)
  - Timestamps/durations (--timestamps, --durations, --total-time)
  - Verbose/quiet modes (--verbose, --quiet)
  - Custom destinations (--dest)
  - Create if/if-not exists (--if-exists, --if-not-exists)
"""

from __future__ import annotations
import argparse
import os
import re
import sys
import time
from dataclasses import dataclass, field
from enum import Enum, auto
from pathlib import Path
from typing import List, Tuple, Dict, Optional, Set, Callable, Any


# ============================================================================
# Constants & Regex
# ============================================================================

_INTERVAL_RE = re.compile(r"^\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*\)$")
_TUPLE_RANGE_RE = re.compile(r"^\s*(\d+)\s*-\s*(\d+)\s*$")

# Family identifiers
class Family(Enum):
    FOR_EACH = auto()
    FOR_EACH_SEPARATOR = auto()
    FOR_EACH_PAIR = auto()
    FOR_EACH_PAIR_SEPARATOR = auto()
    FOR_EACH_TRIPLE = auto()
    FOR_EACH_TRIPLE_SEPARATOR = auto()
    FOR_EACH_DATA_SEPARATOR = auto()
    FOR_EACH_HARDCODED_SEP = auto()  # Families with separator baked into the macro body (MSVC-safe)
    TUPLE = auto()  # Generic tuple family
    WRAPPERS = auto()


FAMILY_NAMES = {
    "for_each": Family.FOR_EACH,
    "for-each": Family.FOR_EACH,
    "foreach": Family.FOR_EACH,
    "separator": Family.FOR_EACH_SEPARATOR,
    "for_each_separator": Family.FOR_EACH_SEPARATOR,
    "pair": Family.FOR_EACH_PAIR,
    "for_each_pair": Family.FOR_EACH_PAIR,
    "pair_separator": Family.FOR_EACH_PAIR_SEPARATOR,
    "for_each_pair_separator": Family.FOR_EACH_PAIR_SEPARATOR,
    "triple": Family.FOR_EACH_TRIPLE,
    "for_each_triple": Family.FOR_EACH_TRIPLE,
    "triple_separator": Family.FOR_EACH_TRIPLE_SEPARATOR,
    "for_each_triple_separator": Family.FOR_EACH_TRIPLE_SEPARATOR,
    "data_separator": Family.FOR_EACH_DATA_SEPARATOR,
    "for_each_data_separator": Family.FOR_EACH_DATA_SEPARATOR,
    "hardcoded_sep": Family.FOR_EACH_HARDCODED_SEP,
    "hardcoded": Family.FOR_EACH_HARDCODED_SEP,
    "tuple": Family.TUPLE,
    "tuples": Family.TUPLE,
    "wrappers": Family.WRAPPERS,
    "wrapper": Family.WRAPPERS,
}


# ============================================================================
# Data Classes
# ============================================================================

@dataclass(frozen=True)
class Interval:
    start: int
    end: int
    step: int


@dataclass
class CascadeTarget:
    """A target file in cascade mode with its range and optional include."""
    path: str
    range_start: int  # First N to emit (inclusive)
    range_end: int    # Last N to emit (inclusive)
    include_file: Optional[str] = None  # File to #include at top (if any)


@dataclass
class GeneratorConfig:
    """Configuration for the generator."""
    # Core settings
    max_n: int = 64
    msvc: bool = False
    include_wrappers: bool = True
    tuples_only: bool = False
    pure_tuples: bool = False  # Only emit tuple macros, no base FOR_EACH_SEPARATOR
    include_zero_tuple: bool = False
    
    # Tuple settings
    tuple_arities: List[int] = field(default_factory=list)
    tuple_naming: str = "D_INTERNAL_FOR_EACH_%_TUPLE_SEPARATOR_#"
    tuple_per_file: bool = False
    cascade: bool = False  # Cascade mode: each file includes previous and emits only its range
    
    # File selection
    only_families: Set[Family] = field(default_factory=set)
    exclude_families: Set[Family] = field(default_factory=set)
    only_tuples: Set[int] = field(default_factory=set)  # Specific tuple arities to include
    exclude_tuples: Set[int] = field(default_factory=set)  # Specific tuple arities to exclude
    
    # Aliases
    aliases: Dict[str, str] = field(default_factory=dict)  # tuple_N -> alias, or family -> alias
    
    # Output control
    overwrite: Optional[bool] = None  # None = prompt, True = always, False = never
    if_exists: bool = False  # Only write if file exists
    if_not_exists: bool = False  # Only write if file doesn't exist
    
    # Logging
    verbose: bool = False
    quiet: bool = False
    show_timestamps: bool = False
    show_durations: bool = False
    show_total_time: bool = False
    
    # Custom destinations
    destinations: Dict[str, str] = field(default_factory=dict)  # tuple_N -> path, or family -> path

    # Hardcoded-separator families: list of (NAME, separator_literal)
    # e.g. [("COMMA", ","), ("SEMICOLON", ";")] generates
    #   D_INTERNAL_FOR_EACH_COMMA_N and D_INTERNAL_FOR_EACH_SEMICOLON_N
    # with the separator baked into the macro body rather than passed as an argument.
    # This avoids the MSVC C4002 arity bug that occurs when a bare comma is
    # forwarded as a _sep parameter.
    hardcoded_seps: List[tuple] = field(default_factory=list)  # List[Tuple[str, str]]


@dataclass
class GenerationResult:
    """Result of generating a single file."""
    path: str
    success: bool
    duration_ms: float = 0.0
    skipped: bool = False
    skip_reason: str = ""
    error: Optional[str] = None


class Logger:
    """Simple logger with verbosity control."""
    
    def __init__(self, verbose: bool = False, quiet: bool = False, 
                 timestamps: bool = False, durations: bool = False):
        self.verbose = verbose
        self.quiet = quiet
        self.timestamps = timestamps
        self.durations = durations
    
    def _timestamp(self) -> str:
        if self.timestamps:
            return f"[{time.strftime('%H:%M:%S')}] "
        return ""
    
    def info(self, msg: str, duration_ms: float = 0.0) -> None:
        if not self.quiet:
            dur = f" ({duration_ms:.1f}ms)" if self.durations and duration_ms > 0 else ""
            print(f"{self._timestamp()}{msg}{dur}")
    
    def debug(self, msg: str) -> None:
        if self.verbose and not self.quiet:
            print(f"{self._timestamp()}[DEBUG] {msg}")
    
    def warn(self, msg: str) -> None:
        if not self.quiet:
            print(f"{self._timestamp()}[WARN] {msg}", file=sys.stderr)
    
    def error(self, msg: str) -> None:
        print(f"{self._timestamp()}[ERROR] {msg}", file=sys.stderr)


# ============================================================================
# Utility Functions
# ============================================================================

def mkdir_p(path: str) -> None:
    os.makedirs(path, exist_ok=True)


def parse_interval(s: str) -> Interval:
    m = _INTERVAL_RE.match(s.strip())
    if not m:
        raise ValueError(f"Invalid interval '{s}'. Expected format: (start,end,step)")
    start = int(m.group(1).strip())
    end = int(m.group(2).strip())
    step = int(m.group(3).strip())
    if step == 0:
        raise ValueError("Interval step cannot be 0")
    return Interval(start, end, step)


def interval_values(iv: Interval) -> List[int]:
    vals: List[int] = []
    if iv.step > 0:
        x = iv.start
        while x <= iv.end:
            vals.append(x)
            x += iv.step
    else:
        x = iv.start
        while x >= iv.end:
            vals.append(x)
            x += iv.step
    return vals


def compute_targets(
    name_pattern: str,
    outdir: str,
    intervals: List[str],
    substitute: str,
    fallback_max: int | None,
) -> List[Tuple[str, int]]:
    targets: List[Tuple[str, int]] = []
    if intervals:
        ivs = [parse_interval(s) for s in intervals]
        for iv in ivs:
            vals = interval_values(iv)
            if not vals:
                continue
            if substitute == "max":
                n = max(vals)
                fname = name_pattern.replace("#", str(n)) if "#" in name_pattern else name_pattern
                targets.append((os.path.join(outdir, fname), n))
            elif substitute == "each":
                for n in vals:
                    fname = name_pattern.replace("#", str(n)) if "#" in name_pattern else name_pattern
                    targets.append((os.path.join(outdir, fname), n))
            else:
                raise ValueError(f"Unknown substitute mode: {substitute}")
    else:
        if fallback_max is None:
            raise ValueError("Either --max or --intervals must be provided")
        n = int(fallback_max)
        fname = name_pattern.replace("#", str(n)) if "#" in name_pattern else name_pattern
        targets.append((os.path.join(outdir, fname), n))

    # Deduplicate
    uniq: List[Tuple[str, int]] = []
    seen = set()
    for path, n in targets:
        key = (os.path.abspath(path), int(n))
        if key in seen:
            continue
        seen.add(key)
        uniq.append((path, int(n)))
    return uniq


def apply_tuple_to_path(path: str, tuple_arity: int) -> str:
    if "%" in path:
        return path.replace("%", str(tuple_arity))
    base, ext = os.path.splitext(path)
    return f"{base}_tuple{tuple_arity}{ext}"


def validate_tuple_naming(naming: str, tuple_arities: List[int]) -> None:
    if "#" not in naming:
        raise ValueError("--tuple-naming-convention must contain '#' (argument-count placeholder)")
    if len(tuple_arities) > 1 and "%" not in naming:
        raise ValueError("--tuple-naming-convention must contain '%' when generating multiple tuple arities")


def _parse_tuple_range(tok: str) -> Tuple[int, int]:
    m = _TUPLE_RANGE_RE.match(tok)
    if not m:
        raise ValueError(f"Invalid tuple range '{tok}'. Expected like 4-16.")
    return int(m.group(1)), int(m.group(2))


def parse_family(name: str) -> Optional[Family]:
    """Parse a family name string to Family enum."""
    normalized = name.lower().strip().replace("-", "_")
    return FAMILY_NAMES.get(normalized)


def parse_alias(alias_str: str) -> Tuple[str, str]:
    """Parse an alias string like 'tuple1=for_each' or 'pair=my_pair'."""
    if "=" not in alias_str:
        raise ValueError(f"Invalid alias format '{alias_str}'. Expected 'key=value'")
    key, value = alias_str.split("=", 1)
    return key.strip(), value.strip()


def parse_dest(dest_str: str) -> Tuple[str, str]:
    """Parse a destination string like 'tuple1=/path/to/dir' or 'pair=/other/dir'."""
    if "=" not in dest_str:
        raise ValueError(f"Invalid destination format '{dest_str}'. Expected 'key=path'")
    key, path = dest_str.split("=", 1)
    return key.strip(), path.strip()


# ============================================================================
# MSVC Caps
# ============================================================================

def msvc_cap(max_n: int, fixed_args: int) -> int:
    cap = 127 - fixed_args
    return max(0, min(int(max_n), cap))


def msvc_cap_multiple(max_n: int, fixed_args: int, arity: int) -> int:
    cap = msvc_cap(max_n, fixed_args=fixed_args)
    if arity <= 0:
        return cap
    return (cap // arity) * arity


# ============================================================================
# Emit Helpers
# ============================================================================

def params_list(prefix: str, n: int) -> List[str]:
    return [f"{prefix}{i}" for i in range(1, n + 1)]


def emit_define(name: str, params: str, body: str) -> str:
    return f"#define {name}({params}) {body}\n"


def emit_for_each(max_n: int, config: GeneratorConfig) -> str:
    out: List[str] = []
    out.append("/* --- D_INTERNAL_FOR_EACH_N --- */\n\n")
    out.append(emit_define("D_INTERNAL_FOR_EACH_0", "_fn, ...", "/* nothing */"))
    for n in range(1, max_n + 1):
        args = ["_fn"] + [f"_{i}" for i in range(1, n + 1)]
        body = " ".join([f"_fn(_{i})" for i in range(1, n + 1)])
        out.append(emit_define(f"D_INTERNAL_FOR_EACH_{n}", ", ".join(args), body))
    out.append("\n")
    return "".join(out)


def emit_for_each_separator(max_n: int, config: GeneratorConfig) -> str:
    out: List[str] = []
    out.append("/* --- D_INTERNAL_FOR_EACH_SEPARATOR_N --- */\n\n")
    out.append(emit_define("D_INTERNAL_FOR_EACH_SEPARATOR_0", "_fn, _sep, ...", "/* nothing */"))
    for n in range(1, max_n + 1):
        args = ["_fn", "_sep"] + [f"_{i}" for i in range(1, n + 1)]
        parts = [f"_fn(_{i})" for i in range(1, n + 1)]
        body = " _sep ".join(parts)
        out.append(emit_define(f"D_INTERNAL_FOR_EACH_SEPARATOR_{n}", ", ".join(args), body))
    out.append("\n")
    return "".join(out)


def emit_for_each_pair(max_n: int, config: GeneratorConfig) -> str:
    out: List[str] = []
    out.append("/* --- D_INTERNAL_FOR_EACH_PAIR_N (even only) --- */\n\n")
    out.append(emit_define("D_INTERNAL_FOR_EACH_PAIR_0", "_fn, ...", "/* nothing */"))
    out.append("/* odd counts expand to an undefined token to force a diagnostic */\n")
    for n in range(1, max_n + 1):
        if n % 2 != 0:
            out.append(emit_define(f"D_INTERNAL_FOR_EACH_PAIR_{n}", "_fn, ...", "D_INTERNAL_FOR_EACH_PAIR__ODD_ARG_COUNT_ERROR"))
            continue
        args = ["_fn"] + [f"_{i}" for i in range(1, n + 1)]
        groups = [f"_fn(_{i}, _{i+1})" for i in range(1, n + 1, 2)]
        body = " ".join(groups)
        out.append(emit_define(f"D_INTERNAL_FOR_EACH_PAIR_{n}", ", ".join(args), body))
    out.append("\n")
    return "".join(out)


def emit_for_each_pair_separator(max_n: int, config: GeneratorConfig) -> str:
    out: List[str] = []
    out.append("/* --- D_INTERNAL_FOR_EACH_PAIR_SEPARATOR_N (even only) --- */\n\n")
    out.append(emit_define("D_INTERNAL_FOR_EACH_PAIR_SEPARATOR_0", "_fn, _sep, ...", "/* nothing */"))
    out.append("/* odd counts expand to an undefined token to force a diagnostic */\n")
    for n in range(1, max_n + 1):
        if n % 2 != 0:
            out.append(emit_define(f"D_INTERNAL_FOR_EACH_PAIR_SEPARATOR_{n}", "_fn, _sep, ...", "D_INTERNAL_FOR_EACH_PAIR__ODD_ARG_COUNT_ERROR"))
            continue
        args = ["_fn", "_sep"] + [f"_{i}" for i in range(1, n + 1)]
        groups = [f"_fn(_{i}, _{i+1})" for i in range(1, n + 1, 2)]
        body = " _sep ".join(groups)
        out.append(emit_define(f"D_INTERNAL_FOR_EACH_PAIR_SEPARATOR_{n}", ", ".join(args), body))
    out.append("\n")
    return "".join(out)


def emit_for_each_triple(max_n: int, config: GeneratorConfig) -> str:
    out: List[str] = []
    out.append("/* --- D_INTERNAL_FOR_EACH_TRIPLE_N (divisible by 3 only) --- */\n\n")
    out.append(emit_define("D_INTERNAL_FOR_EACH_TRIPLE_0", "_fn, ...", "/* nothing */"))
    out.append("/* non-multiples of 3 expand to an undefined token to force a diagnostic */\n")
    for n in range(1, max_n + 1):
        if n % 3 != 0:
            out.append(emit_define(f"D_INTERNAL_FOR_EACH_TRIPLE_{n}", "_fn, ...", "D_INTERNAL_FOR_EACH_TRIPLE__BAD_ARG_COUNT_ERROR"))
            continue
        args = ["_fn"] + [f"_{i}" for i in range(1, n + 1)]
        groups = [f"_fn(_{i}, _{i+1}, _{i+2})" for i in range(1, n + 1, 3)]
        body = " ".join(groups)
        out.append(emit_define(f"D_INTERNAL_FOR_EACH_TRIPLE_{n}", ", ".join(args), body))
    out.append("\n")
    return "".join(out)


def emit_for_each_triple_separator(max_n: int, config: GeneratorConfig) -> str:
    out: List[str] = []
    out.append("/* --- D_INTERNAL_FOR_EACH_TRIPLE_SEPARATOR_N (divisible by 3 only) --- */\n\n")
    out.append(emit_define("D_INTERNAL_FOR_EACH_TRIPLE_SEPARATOR_0", "_fn, _sep, ...", "/* nothing */"))
    out.append("/* non-multiples of 3 expand to an undefined token to force a diagnostic */\n")
    for n in range(1, max_n + 1):
        if n % 3 != 0:
            out.append(emit_define(f"D_INTERNAL_FOR_EACH_TRIPLE_SEPARATOR_{n}", "_fn, _sep, ...", "D_INTERNAL_FOR_EACH_TRIPLE__BAD_ARG_COUNT_ERROR"))
            continue
        args = ["_fn", "_sep"] + [f"_{i}" for i in range(1, n + 1)]
        groups = [f"_fn(_{i}, _{i+1}, _{i+2})" for i in range(1, n + 1, 3)]
        body = " _sep ".join(groups)
        out.append(emit_define(f"D_INTERNAL_FOR_EACH_TRIPLE_SEPARATOR_{n}", ", ".join(args), body))
    out.append("\n")
    return "".join(out)


def _tuple_macro_name(naming: str, arity: int, n: int | str) -> str:
    s = naming.replace("%", str(arity))
    return s.replace("#", str(n))


def emit_for_each_k_tuple_separator(max_n: int, arity: int, naming: str, include_zero: bool, config: GeneratorConfig, start_n: int = 0) -> str:
    """
    Emit tuple separator macros.
    
    Args:
        max_n: Maximum N to emit (inclusive)
        arity: Tuple arity (1, 2, 3, etc.)
        naming: Naming convention with % for arity and # for count
        include_zero: Whether to include the _0 macro
        config: Generator configuration
        start_n: Starting N (for cascade mode). Macros are emitted for start_n < n <= max_n
                 (or from arity if start_n < arity)
    """
    if arity < 1:
        raise ValueError("Tuple arity must be >= 1")

    out: List[str] = []
    out.append(f"/* --- {_tuple_macro_name(naming, arity, 'N')} (counts divisible by {arity}) --- */\n")
    out.append("/* Only valid counts are emitted. */\n\n")

    # Only emit _0 if include_zero and we're starting from 0
    if include_zero and start_n == 0:
        name0 = _tuple_macro_name(naming, arity, 0)
        out.append(emit_define(name0, "_fn, _sep, ...", "/* nothing */"))

    # Calculate actual start: must be a multiple of arity and > start_n
    actual_start = arity
    if start_n > 0:
        # Find first multiple of arity that is > start_n
        actual_start = ((start_n // arity) + 1) * arity

    for n in range(actual_start, max_n + 1, arity):
        name = _tuple_macro_name(naming, arity, n)
        args = ["_fn", "_sep"] + [f"_{i}" for i in range(1, n + 1)]
        groups: List[str] = []
        for i in range(1, n + 1, arity):
            params = ", ".join([f"_{j}" for j in range(i, i + arity)])
            groups.append(f"_fn({params})")
        body = " _sep ".join(groups)
        out.append(emit_define(name, ", ".join(args), body))

    out.append("\n")
    return "".join(out)


def emit_for_each_data_separator(max_n: int, config: GeneratorConfig) -> str:
    out: List[str] = []
    out.append("/* --- D_INTERNAL_FOR_EACH_DATA_SEPARATOR_N --- */\n\n")
    out.append(emit_define("D_INTERNAL_FOR_EACH_DATA_SEPARATOR_0", "_fn, _sep, _data, ...", "/* nothing */"))
    for n in range(1, max_n + 1):
        args = ["_fn", "_sep", "_data"] + [f"_{i}" for i in range(1, n + 1)]
        parts = [f"_fn(_data, _{i})" for i in range(1, n + 1)]
        body = " _sep ".join(parts)
        out.append(emit_define(f"D_INTERNAL_FOR_EACH_DATA_SEPARATOR_{n}", ", ".join(args), body))
    out.append("\n")
    return "".join(out)


def emit_for_each_hardcoded_sep(max_n: int, name: str, sep_literal: str, config: GeneratorConfig) -> str:
    """
    Emit a FOR_EACH family whose separator is baked into the macro body rather
    than passed as a parameter.

    This is the MSVC-safe alternative to D_INTERNAL_FOR_EACH_SEPARATOR_N for
    separators like ',' that are argument delimiters in the C preprocessor.
    When a bare comma is forwarded as a macro argument, MSVC pre-expands it and
    miscounts arity (C4002).  By hardcoding the separator in the body, no comma
    ever travels as a macro parameter value.

    Args:
        max_n:       Maximum N to emit (inclusive).
        name:        Short uppercase name for the family, e.g. "COMMA" or "SEMICOLON".
                     Produces macros named D_INTERNAL_FOR_EACH_<NAME>_N.
        sep_literal: The literal separator token, e.g. "," or ";".
        config:      Generator configuration (used for MSVC caps etc.).

    Example output for name="COMMA", sep_literal=",", max_n=3:
        #define D_INTERNAL_FOR_EACH_COMMA_0(_fn, ...) /* nothing */
        #define D_INTERNAL_FOR_EACH_COMMA_1(_fn, _1) _fn(_1)
        #define D_INTERNAL_FOR_EACH_COMMA_2(_fn, _1, _2) _fn(_1), _fn(_2)
        #define D_INTERNAL_FOR_EACH_COMMA_3(_fn, _1, _2, _3) _fn(_1), _fn(_2), _fn(_3)
    """
    prefix = f"D_INTERNAL_FOR_EACH_{name.upper()}"
    out: List[str] = []
    out.append(f"/* --- {prefix}_N (separator '{sep_literal}' hardcoded in body; MSVC-safe) --- */\n\n")
    out.append(emit_define(f"{prefix}_0", "_fn, ...", "/* nothing */"))
    for n in range(1, max_n + 1):
        args = ["_fn"] + [f"_{i}" for i in range(1, n + 1)]
        parts = [f"_fn(_{i})" for i in range(1, n + 1)]
        body = f"{sep_literal} ".join(parts)
        # For single-element case or when sep is not a space, don't add trailing space
        # Re-join cleanly: fn(_1), fn(_2), fn(_3)
        body = (f"{sep_literal} ").join(parts)
        out.append(emit_define(f"{prefix}_{n}", ", ".join(args), body))
    out.append("\n")
    return "".join(out)


def emit_for_each_k_tuple_hardcoded_sep(
    max_n: int,
    arity: int,
    naming: str,
    sep_name: str,
    sep_literal: str,
    include_zero: bool,
    config: GeneratorConfig,
    start_n: int = 0,
) -> str:
    """
    Emit a tuple-family whose separator is baked into the macro body rather
    than passed as a _sep parameter.

    This is the tuple equivalent of emit_for_each_hardcoded_sep().  It avoids
    the MSVC C4002 arity bug for tuple iteration with comma separators.

    Args:
        max_n:        Maximum total argument count to emit (inclusive).
        arity:        Tuple arity (1, 2, 3, ...).
        naming:       Naming convention with % for arity and # for count.
                      e.g. "D_INTERNAL_FOR_EACH_%_TUPLE_COMMA_#"
        sep_name:     Short uppercase name, e.g. "COMMA".
        sep_literal:  The literal separator token, e.g. ",".
        include_zero: Whether to include the _0 macro.
        config:       Generator configuration.
        start_n:      Starting N for cascade mode.

    Example output for arity=2, sep_name="COMMA", sep_literal=",", max_n=4:
        #define D_INTERNAL_FOR_EACH_2_TUPLE_COMMA_0(_fn, ...) /* nothing */
        #define D_INTERNAL_FOR_EACH_2_TUPLE_COMMA_2(_fn, _1, _2) _fn(_1, _2)
        #define D_INTERNAL_FOR_EACH_2_TUPLE_COMMA_4(_fn, _1, _2, _3, _4) _fn(_1, _2), _fn(_3, _4)
    """
    if arity < 1:
        raise ValueError("Tuple arity must be >= 1")

    out: List[str] = []
    label = _tuple_macro_name(naming, arity, "N")
    out.append(f"/* --- {label} (separator '{sep_literal}' hardcoded in body; MSVC-safe) --- */\n")
    out.append(f"/* Only counts divisible by {arity} are emitted. */\n\n")

    # _0 macro
    if include_zero and start_n == 0:
        name0 = _tuple_macro_name(naming, arity, 0)
        out.append(emit_define(name0, "_fn, ...", "/* nothing */"))

    # Calculate actual start: must be a multiple of arity and > start_n
    actual_start = arity
    if start_n > 0:
        actual_start = ((start_n // arity) + 1) * arity

    for n in range(actual_start, max_n + 1, arity):
        name = _tuple_macro_name(naming, arity, n)
        # No _sep parameter — separator is hardcoded in the body
        args = ["_fn"] + [f"_{i}" for i in range(1, n + 1)]
        groups: List[str] = []
        for i in range(1, n + 1, arity):
            params = ", ".join([f"_{j}" for j in range(i, i + arity)])
            groups.append(f"_fn({params})")
        body = (f"{sep_literal} ").join(groups)
        out.append(emit_define(name, ", ".join(args), body))

    out.append("\n")
    return "".join(out)


def emit_convenience_macros(tuple_arities: List[int], tuple_naming: str, config: GeneratorConfig) -> str:
    lines: List[str] = []
    lines.append("/* --- Convenience wrappers (require D_CONCAT + D_VARG_COUNT) --- */\n\n")
    lines.append("#define D_SEPARATOR_COMMA     ,\n")
    lines.append("#define D_SEPARATOR_SEMICOLON ;\n")
    lines.append("#define D_SEPARATOR_SPACE     \n\n")

    lines.append("#define D_FOR_EACH(_fn, ...) \\\n")
    lines.append("    D_CONCAT(D_INTERNAL_FOR_EACH_, D_VARG_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)\n\n")

    lines.append("#define D_FOR_EACH_SEPARATOR(_fn, _sep, ...) \\\n")
    lines.append("    D_CONCAT(D_INTERNAL_FOR_EACH_SEPARATOR_, D_VARG_COUNT(__VA_ARGS__))(_fn, _sep, __VA_ARGS__)\n\n")

    # D_FOR_EACH_COMMA: if a hardcoded COMMA family was generated, route through it
    # (avoids MSVC C4002 caused by forwarding a bare comma as a macro argument).
    # Otherwise fall back to the generic separator wrapper.
    hardcoded_names = {name.upper() for name, _ in config.hardcoded_seps}
    if "COMMA" in hardcoded_names:
        lines.append(
            "/* NOTE: routes through D_INTERNAL_FOR_EACH_COMMA_N (comma hardcoded in body)\n"
            " *       rather than D_FOR_EACH_SEPARATOR to avoid MSVC C4002. */\n"
        )
        lines.append("#define D_FOR_EACH_COMMA(_fn, ...) \\\n")
        lines.append("    D_CONCAT(D_INTERNAL_FOR_EACH_COMMA_, D_VARG_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)\n\n")
    else:
        lines.append("#define D_FOR_EACH_COMMA(_fn, ...) \\\n")
        lines.append("    D_FOR_EACH_SEPARATOR(_fn, D_SEPARATOR_COMMA, __VA_ARGS__)\n\n")

    # D_FOR_EACH_SEMICOLON: same pattern — use hardcoded family if present
    if "SEMICOLON" in hardcoded_names:
        lines.append(
            "/* NOTE: routes through D_INTERNAL_FOR_EACH_SEMICOLON_N (semicolon hardcoded in body). */\n"
        )
        lines.append("#define D_FOR_EACH_SEMICOLON(_fn, ...) \\\n")
        lines.append("    D_CONCAT(D_INTERNAL_FOR_EACH_SEMICOLON_, D_VARG_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)\n\n")
    else:
        lines.append("#define D_FOR_EACH_SEMICOLON(_fn, ...) \\\n")
        lines.append("    D_FOR_EACH_SEPARATOR(_fn, D_SEPARATOR_SEMICOLON, __VA_ARGS__)\n\n")

    lines.append("#define D_FOR_EACH_PAIR(_fn, ...) \\\n")
    lines.append("    D_CONCAT(D_INTERNAL_FOR_EACH_PAIR_, D_VARG_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)\n\n")

    lines.append("#define D_FOR_EACH_PAIR_SEPARATOR(_fn, _sep, ...) \\\n")
    lines.append("    D_CONCAT(D_INTERNAL_FOR_EACH_PAIR_SEPARATOR_, D_VARG_COUNT(__VA_ARGS__))(_fn, _sep, __VA_ARGS__)\n\n")

    lines.append("#define D_FOR_EACH_TRIPLE(_fn, ...) \\\n")
    lines.append("    D_CONCAT(D_INTERNAL_FOR_EACH_TRIPLE_, D_VARG_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)\n\n")

    lines.append("#define D_FOR_EACH_TRIPLE_SEPARATOR(_fn, _sep, ...) \\\n")
    lines.append("    D_CONCAT(D_INTERNAL_FOR_EACH_TRIPLE_SEPARATOR_, D_VARG_COUNT(__VA_ARGS__))(_fn, _sep, __VA_ARGS__)\n\n")

    lines.append("#define D_FOR_EACH_DATA_SEPARATOR(_fn, _sep, _data, ...) \\\n")
    lines.append("    D_CONCAT(D_INTERNAL_FOR_EACH_DATA_SEPARATOR_, D_VARG_COUNT(__VA_ARGS__))(_fn, _sep, _data, __VA_ARGS__)\n\n")

    for k in tuple_arities:
        lines.append(f"#define D_FOR_EACH_{k}_TUPLE_SEPARATOR(_fn, _sep, ...) \\\n")
        lines.append(f"    D_CONCAT({_tuple_macro_name(tuple_naming, k, '')}, D_VARG_COUNT(__VA_ARGS__))(_fn, _sep, __VA_ARGS__)\n\n")

    return "".join(lines)


# ============================================================================
# Header Building
# ============================================================================

def should_include_family(family: Family, config: GeneratorConfig) -> bool:
    """Check if a family should be included based on config."""
    if config.only_families and family not in config.only_families:
        return False
    if family in config.exclude_families:
        return False
    return True


def should_include_tuple(arity: int, config: GeneratorConfig) -> bool:
    """Check if a tuple arity should be included based on config."""
    if config.only_tuples and arity not in config.only_tuples:
        return False
    if arity in config.exclude_tuples:
        return False
    return True


def build_header(
    max_n: int,
    config: GeneratorConfig,
    tuple_arities: Optional[List[int]] = None,
    start_n: int = 0,
    include_file: Optional[str] = None,
) -> str:
    """
    Build header content.
    
    Args:
        max_n: Maximum N to emit
        config: Generator configuration
        tuple_arities: Override tuple arities (defaults to config.tuple_arities)
        start_n: Starting N for cascade mode (emit macros for start_n < n <= max_n)
        include_file: File to #include at top (for cascade mode)
    """
    if tuple_arities is None:
        tuple_arities = config.tuple_arities
    
    parts: List[str] = ["/* Auto-generated. Do not edit by hand. */\n\n"]
    
    # Add #include directive for cascade mode
    if include_file:
        parts.append(f'#include "{include_file}"\n\n')

    if config.msvc:
        max_for_each = msvc_cap(max_n, fixed_args=1)
        max_for_each_sep = msvc_cap(max_n, fixed_args=2)
        max_pair = msvc_cap_multiple(max_n, fixed_args=1, arity=2)
        max_pair_sep = msvc_cap_multiple(max_n, fixed_args=2, arity=2)
        max_triple = msvc_cap_multiple(max_n, fixed_args=1, arity=3)
        max_triple_sep = msvc_cap_multiple(max_n, fixed_args=2, arity=3)
        max_data_sep = msvc_cap(max_n, fixed_args=3)
    else:
        max_for_each = max_for_each_sep = max_data_sep = max_n
        max_pair = max_pair_sep = max_n
        max_triple = max_triple_sep = max_n

    # pure_tuples mode: emit ONLY tuple families
    if config.pure_tuples:
        if should_include_family(Family.TUPLE, config):
            for k in tuple_arities:
                if should_include_tuple(k, config):
                    naming = config.tuple_naming
                    alias_key = f"tuple{k}"
                    if alias_key in config.aliases:
                        naming = naming.replace(f"_{k}_TUPLE_", f"_{config.aliases[alias_key]}_")
                    if config.hardcoded_seps:
                        # Emit hardcoded-sep versions (MSVC-safe comma baked into body)
                        for hname, hsep in config.hardcoded_seps:
                            parts.append(emit_for_each_k_tuple_hardcoded_sep(
                                max_for_each, k, naming, hname, hsep,
                                config.include_zero_tuple, config, start_n,
                            ))
                    else:
                        parts.append(emit_for_each_k_tuple_separator(max_for_each_sep, k, naming, config.include_zero_tuple, config, start_n))
        return "".join(parts)

    if config.tuples_only:
        # Only emit FOR_EACH_SEPARATOR (needed as base) and tuple families
        if should_include_family(Family.FOR_EACH_SEPARATOR, config):
            parts.append(emit_for_each_separator(max_for_each_sep, config))
    else:
        if should_include_family(Family.FOR_EACH, config):
            parts.append(emit_for_each(max_for_each, config))
        if should_include_family(Family.FOR_EACH_SEPARATOR, config):
            parts.append(emit_for_each_separator(max_for_each_sep, config))
        if should_include_family(Family.FOR_EACH_PAIR, config):
            parts.append(emit_for_each_pair(max_pair, config))
        if should_include_family(Family.FOR_EACH_PAIR_SEPARATOR, config):
            parts.append(emit_for_each_pair_separator(max_pair_sep, config))
        if should_include_family(Family.FOR_EACH_TRIPLE, config):
            parts.append(emit_for_each_triple(max_triple, config))
        if should_include_family(Family.FOR_EACH_TRIPLE_SEPARATOR, config):
            parts.append(emit_for_each_triple_separator(max_triple_sep, config))

    # Tuple families
    if should_include_family(Family.TUPLE, config):
        for k in tuple_arities:
            if should_include_tuple(k, config):
                # Apply alias if present
                naming = config.tuple_naming
                alias_key = f"tuple{k}"
                if alias_key in config.aliases:
                    # Replace the tuple naming pattern with alias
                    naming = naming.replace(f"_{k}_TUPLE_", f"_{config.aliases[alias_key]}_")
                parts.append(emit_for_each_k_tuple_separator(max_for_each_sep, k, naming, config.include_zero_tuple, config, start_n))

    if not config.tuples_only:
        if should_include_family(Family.FOR_EACH_DATA_SEPARATOR, config):
            parts.append(emit_for_each_data_separator(max_data_sep, config))

        # Hardcoded-separator families (e.g. D_INTERNAL_FOR_EACH_COMMA_N).
        # These are always emitted when configured regardless of --only/--exclude
        # for the base families, because they are independent named families.
        if should_include_family(Family.FOR_EACH_HARDCODED_SEP, config):
            for hname, hsep in config.hardcoded_seps:
                parts.append(emit_for_each_hardcoded_sep(max_for_each_sep, hname, hsep, config))

    if config.include_wrappers and not config.tuples_only:
        if should_include_family(Family.WRAPPERS, config):
            parts.append(emit_convenience_macros(tuple_arities, config.tuple_naming, config))

    return "".join(parts)


# ============================================================================
# File Writing with Controls
# ============================================================================

def should_write_file(path: str, config: GeneratorConfig, logger: Logger) -> Tuple[bool, str]:
    """
    Determine if we should write to the file.
    Returns (should_write, reason_if_not).
    """
    exists = os.path.exists(path)
    
    if config.if_exists and not exists:
        return False, "file does not exist (--if-exists specified)"
    
    if config.if_not_exists and exists:
        return False, "file already exists (--if-not-exists specified)"
    
    if exists:
        if config.overwrite is False:
            return False, "file exists and --no-overwrite specified"
        elif config.overwrite is None:
            # Interactive prompt (not implemented in batch mode, default to overwrite)
            logger.debug(f"File exists, overwriting: {path}")
    
    return True, ""


def write_file(path: str, content: str, config: GeneratorConfig, logger: Logger) -> GenerationResult:
    """Write content to file with all controls applied."""
    start_time = time.perf_counter()
    
    should_write, reason = should_write_file(path, config, logger)
    if not should_write:
        return GenerationResult(
            path=path,
            success=True,
            skipped=True,
            skip_reason=reason,
            duration_ms=(time.perf_counter() - start_time) * 1000
        )
    
    try:
        mkdir_p(os.path.dirname(path) or ".")
        with open(path, "w", newline="\n") as f:
            f.write(content)
        duration_ms = (time.perf_counter() - start_time) * 1000
        return GenerationResult(path=path, success=True, duration_ms=duration_ms)
    except Exception as e:
        duration_ms = (time.perf_counter() - start_time) * 1000
        return GenerationResult(path=path, success=False, error=str(e), duration_ms=duration_ms)


# ============================================================================
# Argument Parsing
# ============================================================================

def resolve_tuple_arities(args: argparse.Namespace) -> List[int]:
    if args.no_tuples:
        return []

    chosen: List[int] = []

    # explicit --k-tuple flags
    if getattr(args, "tuple_flags", None):
        chosen.extend(args.tuple_flags)

    # ranges
    for tok in args.tuple_range:
        a, b = _parse_tuple_range(tok)
        lo, hi = (a, b) if a <= b else (b, a)
        chosen.extend(list(range(lo, hi + 1)))

    # explicit list
    if args.tuple_arities is not None and len(args.tuple_arities) > 0:
        chosen.extend([int(x) for x in args.tuple_arities])

    # fallback min/max
    if not chosen:
        chosen.extend(list(range(args.tuple_min, args.tuple_max + 1)))

    # normalize unique & bounds
    out = sorted(set(int(x) for x in chosen))
    out = [k for k in out if 1 <= k <= 16]
    return out


def parse_args(argv: List[str]) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Generate C preprocessor for-each macro families",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate all families up to 64 args
  %(prog)s --name for_each_#.h --max 64

  # Generate only tuple files, one per arity
  %(prog)s --name "for_each_%%_tuple_#.h" --max 64 --tuple-per-file --tuples-only

  # Generate specific tuples with aliases
  %(prog)s --name for_each.h --max 64 --tuple-arities 2 3 \\
           --alias tuple2=pair --alias tuple3=triple

  # Generate with verbose output and timing
  %(prog)s --name for_each_#.h --max 64 --verbose --timestamps --durations --total-time

  # Only generate if files don't exist
  %(prog)s --name for_each_#.h --max 64 --if-not-exists

  # Custom destinations per tuple
  %(prog)s --name "for_each_%%_tuple_#.h" --max 64 --tuple-per-file \\
           --dest tuple1=/path/to/singles --dest tuple2=/path/to/pairs
"""
    )

    # Core output options
    p.add_argument("--name", required=True, 
                   help="Output file name or pattern (# = max N, %% = tuple arity)")
    p.add_argument("--outdir", default=".", help="Output directory (default: current)")
    p.add_argument("--max", type=int, default=None, 
                   help="Max N to generate (required if --intervals not provided)")
    p.add_argument("--intervals", nargs="*", default=[], 
                   help="Intervals like (1,64,1) (inclusive)")
    p.add_argument("--substitute", choices=["max", "each"], default="max",
                   help="How to substitute # when using --intervals")

    # Content control
    p.add_argument("--tuples-only", action="store_true",
                   help="Emit only D_INTERNAL_FOR_EACH_SEPARATOR_* and tuple families")
    p.add_argument("--pure-tuples", action="store_true",
                   help="Emit ONLY tuple families (no base FOR_EACH_SEPARATOR)")
    p.add_argument("--no-wrappers", action="store_true",
                   help="Do not emit D_FOR_EACH* wrapper macros")
    p.add_argument("--wrappers", action="store_true",
                   help="Force wrappers on (overrides --msvc default)")

    # MSVC
    p.add_argument("--msvc", action="store_true",
                   help="MSVC-safe mode: cap to 127-argument limit; defaults --max 127 and disables wrappers")

    # Tuple selection
    p.add_argument("--tuple-min", type=int, default=4,
                   help="Minimum tuple arity (fallback default: 4)")
    p.add_argument("--tuple-max", type=int, default=16,
                   help="Maximum tuple arity (fallback default: 16)")
    p.add_argument("--tuple-arities", nargs="*", default=None,
                   help="Explicit tuple arities (overrides min/max)")
    p.add_argument("--tuple-range", "-tuple-range", action="append", default=[],
                   help="Inclusive tuple arity range like 1-16 (repeatable)")
    p.add_argument("--no-tuples", action="store_true",
                   help="Do not emit tuple families")
    p.add_argument("--tuple-per-file", action="store_true",
                   help="Emit one file per (tuple arity, target). Use %% in --name.")
    p.add_argument("--cascade", action="store_true",
                   help="Cascade mode: each interval file includes the previous one and only emits its range")
    p.add_argument("--tuple-naming-convention", 
                   default="D_INTERNAL_FOR_EACH_%_TUPLE_SEPARATOR_#",
                   help="Tuple macro naming. Use %% for arity and # for arg count.")
    p.add_argument("--include-zero-arg", action="store_true",
                   help="Also emit the *_0 macro for tuple families")

    # Individual tuple flags
    for k in range(1, 17):
        p.add_argument(f"--{k}-tuple", f"-{k}-tuple", dest="tuple_flags",
                       action="append_const", const=k, help=f"Include tuple arity {k}")

    # === NEW OPTIONS ===

    # File/group selection
    p.add_argument("--only", nargs="*", default=[],
                   help="Only include specified families/tuples (e.g., 'for_each', 'tuple', 'tuple2', 'pair')")
    p.add_argument("--exclude", nargs="*", default=[],
                   help="Exclude specified families/tuples")

    # Aliases
    p.add_argument("--alias", action="append", default=[],
                   help="Alias for renaming: 'tuple2=pair' or 'for_each=FE'")

    # Overwrite control
    overwrite_group = p.add_mutually_exclusive_group()
    overwrite_group.add_argument("--overwrite", action="store_true",
                                  help="Always overwrite existing files")
    overwrite_group.add_argument("--no-overwrite", action="store_true",
                                  help="Never overwrite existing files")

    # Create conditions
    create_group = p.add_mutually_exclusive_group()
    create_group.add_argument("--if-exists", action="store_true",
                               help="Only write if file already exists")
    create_group.add_argument("--if-not-exists", action="store_true",
                               help="Only write if file does not exist")

    # Logging/output
    verbosity_group = p.add_mutually_exclusive_group()
    verbosity_group.add_argument("--verbose", "-v", action="store_true",
                                  help="Verbose output")
    verbosity_group.add_argument("--quiet", "-q", action="store_true",
                                  help="Suppress all non-error output")

    p.add_argument("--timestamps", action="store_true",
                   help="Show timestamps in output")
    p.add_argument("--durations", action="store_true",
                   help="Show duration for each file generation")
    p.add_argument("--total-time", action="store_true",
                   help="Show total generation time at end")

    # Custom destinations
    p.add_argument("--dest", action="append", default=[],
                   help="Custom destination: 'tuple2=/path/to/dir' or 'pair=/other/dir'")

    # Hardcoded-separator families (MSVC-safe)
    p.add_argument(
        "--hardcoded-sep", action="append", default=[], metavar="NAME=LITERAL",
        help=(
            "Generate a D_INTERNAL_FOR_EACH_<NAME>_N family with LITERAL baked into "
            "the macro body rather than passed as a _sep argument.  Repeatable.  "
            "e.g. --hardcoded-sep COMMA=, --hardcoded-sep SEMICOLON=;  "
            "Avoids the MSVC C4002 arity bug that occurs when a bare comma is "
            "forwarded through D_FOR_EACH_SEPARATOR."
        ),
    )
    p.add_argument(
        "--msvc-comma", action="store_true",
        help=(
            "Shorthand for --hardcoded-sep COMMA=,  "
            "Generates D_INTERNAL_FOR_EACH_COMMA_N and rewires D_FOR_EACH_COMMA "
            "to use it instead of the generic separator path."
        ),
    )

    return p.parse_args(argv)


def build_config(args: argparse.Namespace) -> GeneratorConfig:
    """Build GeneratorConfig from parsed arguments."""
    config = GeneratorConfig()
    
    # Core settings
    if args.msvc and args.max is None and not args.intervals:
        config.max_n = 127
    elif args.max is not None:
        config.max_n = args.max
    
    config.msvc = args.msvc
    config.tuples_only = args.tuples_only
    config.pure_tuples = args.pure_tuples
    config.include_zero_tuple = args.include_zero_arg
    
    # Wrappers
    config.include_wrappers = not args.no_wrappers
    if args.msvc and not args.wrappers:
        config.include_wrappers = False
    if args.wrappers:
        config.include_wrappers = True
    
    # Tuple settings
    config.tuple_arities = resolve_tuple_arities(args)
    config.tuple_naming = args.tuple_naming_convention
    config.tuple_per_file = args.tuple_per_file
    config.cascade = args.cascade
    
    # Validate tuple naming
    if config.tuple_arities:
        validate_tuple_naming(config.tuple_naming, config.tuple_arities)
    
    # File/group selection
    for item in args.only:
        # Check if it's a tuple number like "tuple2" or just "2"
        if item.startswith("tuple") and item[5:].isdigit():
            config.only_tuples.add(int(item[5:]))
        elif item.isdigit():
            config.only_tuples.add(int(item))
        else:
            family = parse_family(item)
            if family:
                config.only_families.add(family)
    
    for item in args.exclude:
        if item.startswith("tuple") and item[5:].isdigit():
            config.exclude_tuples.add(int(item[5:]))
        elif item.isdigit():
            config.exclude_tuples.add(int(item))
        else:
            family = parse_family(item)
            if family:
                config.exclude_families.add(family)
    
    # Aliases
    for alias_str in args.alias:
        key, value = parse_alias(alias_str)
        config.aliases[key] = value
    
    # Overwrite control
    if args.overwrite:
        config.overwrite = True
    elif args.no_overwrite:
        config.overwrite = False
    else:
        config.overwrite = None
    
    config.if_exists = args.if_exists
    config.if_not_exists = args.if_not_exists
    
    # Logging
    config.verbose = args.verbose
    config.quiet = args.quiet
    config.show_timestamps = args.timestamps
    config.show_durations = args.durations
    config.show_total_time = args.total_time
    
    # Custom destinations
    for dest_str in args.dest:
        key, path = parse_dest(dest_str)
        config.destinations[key] = path

    # Hardcoded-separator families
    hardcoded_seps: List[tuple] = []
    if args.msvc_comma:
        hardcoded_seps.append(("COMMA", ","))
    for spec in args.hardcoded_sep:
        if "=" not in spec:
            raise ValueError(
                f"Invalid --hardcoded-sep value '{spec}'. Expected NAME=LITERAL, e.g. COMMA=,"
            )
        hname, hliteral = spec.split("=", 1)
        hname = hname.strip().upper()
        if not hname:
            raise ValueError(f"--hardcoded-sep name cannot be empty in '{spec}'")
        # Deduplicate: if --msvc-comma already added COMMA, skip a redundant explicit one
        if not any(n == hname for n, _ in hardcoded_seps):
            hardcoded_seps.append((hname, hliteral))
    config.hardcoded_seps = hardcoded_seps
    
    return config


# ============================================================================
# Main Entry Point
# ============================================================================

def main(argv: List[str]) -> int:
    args = parse_args(argv)
    config = build_config(args)
    
    logger = Logger(
        verbose=config.verbose,
        quiet=config.quiet,
        timestamps=config.show_timestamps,
        durations=config.show_durations
    )
    
    total_start = time.perf_counter()
    
    mkdir_p(args.outdir)

    targets = compute_targets(
        name_pattern=args.name,
        outdir=args.outdir,
        intervals=args.intervals,
        substitute=args.substitute,
        fallback_max=args.max if args.max else (127 if args.msvc else None),
    )

    results: List[GenerationResult] = []

    if config.tuple_per_file:
        # For cascade mode, we need to process targets in order and track previous files
        # Group by tuple arity for cascade
        if config.cascade:
            for k in config.tuple_arities:
                if not should_include_tuple(k, config):
                    continue
                
                prev_file: Optional[str] = None
                prev_max: int = 0
                
                for base_path, max_n in targets:
                    # Determine output path
                    dest_key = f"tuple{k}"
                    if dest_key in config.destinations:
                        out_dir = config.destinations[dest_key]
                        out_path = os.path.join(out_dir, os.path.basename(apply_tuple_to_path(base_path, k)))
                    else:
                        out_path = apply_tuple_to_path(base_path, k)
                    
                    # For cascade: include previous file and only emit range (prev_max, max_n]
                    include_file = os.path.basename(prev_file) if prev_file else None
                    start_n = prev_max
                    
                    logger.debug(f"Generating tuple {k} -> {out_path} (range {start_n+1}..{max_n}, include={include_file})")
                    
                    content = build_header(
                        max_n=max_n,
                        config=config,
                        tuple_arities=[k],
                        start_n=start_n,
                        include_file=include_file,
                    )
                    
                    result = write_file(out_path, content, config, logger)
                    results.append(result)
                    
                    if result.skipped:
                        logger.info(f"Skipped {out_path}: {result.skip_reason}")
                    elif result.success:
                        range_desc = f"{start_n+1}..{max_n}" if start_n > 0 else f"0..{max_n}"
                        logger.info(f"Wrote {out_path} (tuple={k}, {range_desc})", result.duration_ms)
                    else:
                        logger.error(f"Failed to write {out_path}: {result.error}")
                    
                    # Update for next iteration
                    prev_file = out_path
                    prev_max = max_n
        else:
            # Non-cascade mode: each file is independent
            for base_path, max_n in targets:
                for k in config.tuple_arities:
                    if not should_include_tuple(k, config):
                        continue
                    
                    # Determine output path (with custom destinations)
                    dest_key = f"tuple{k}"
                    if dest_key in config.destinations:
                        out_dir = config.destinations[dest_key]
                        out_path = os.path.join(out_dir, os.path.basename(apply_tuple_to_path(base_path, k)))
                    else:
                        out_path = apply_tuple_to_path(base_path, k)
                    
                    logger.debug(f"Generating tuple {k} -> {out_path}")
                    
                    content = build_header(
                        max_n=max_n,
                        config=config,
                        tuple_arities=[k],
                    )
                    
                    result = write_file(out_path, content, config, logger)
                    results.append(result)
                    
                    if result.skipped:
                        logger.info(f"Skipped {out_path}: {result.skip_reason}")
                    elif result.success:
                        logger.info(f"Wrote {out_path} (tuple={k}, 0..{max_n})", result.duration_ms)
                    else:
                        logger.error(f"Failed to write {out_path}: {result.error}")
    else:
        for path, max_n in targets:
            # Check for custom destination for the whole file
            # (This is less common but supported)
            dest_key = "all"
            if dest_key in config.destinations:
                out_dir = config.destinations[dest_key]
                path = os.path.join(out_dir, os.path.basename(path))
            
            logger.debug(f"Generating {path} with max_n={max_n}")
            
            content = build_header(
                max_n=max_n,
                config=config,
            )
            
            result = write_file(path, content, config, logger)
            results.append(result)
            
            if result.skipped:
                logger.info(f"Skipped {path}: {result.skip_reason}")
            elif result.success:
                logger.info(f"Wrote {path} (0..{max_n})", result.duration_ms)
            else:
                logger.error(f"Failed to write {path}: {result.error}")

    # Summary
    total_duration = (time.perf_counter() - total_start) * 1000
    
    if config.show_total_time and not config.quiet:
        succeeded = sum(1 for r in results if r.success and not r.skipped)
        skipped = sum(1 for r in results if r.skipped)
        failed = sum(1 for r in results if not r.success)
        
        print(f"\n{logger._timestamp()}Summary: {succeeded} written, {skipped} skipped, {failed} failed")
        print(f"{logger._timestamp()}Total time: {total_duration:.1f}ms")

    # Return error code if any failures
    return 1 if any(not r.success for r in results) else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))