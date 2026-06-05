# restd module implementation workflow

This document describes the end-to-end process for shipping a restd module.
It supersedes step 7 of `RESTD_AGENT_README.md` ("Update documentation"),
which referenced now-deprecated tables. The workflow has three phases,
in strict order:

1. **Header files** — the actual implementation.
2. **Coverage JSON** — one file per std header, the workbook's data source.
3. **Workbook build** — regenerate `restd_coverage.xlsx` and update the
   roadmap.

Each phase is verified before moving to the next.

---

## Phase 1 — Header files

Implement the module per `RESTD_AGENT_README.md` § _How to Implement a
New Module_, steps 1–6 (granular per-symbol `.hpp`s, umbrella header,
tiered `#if` guards). Then **compile-test** before considering the phase
done:

- Build a small smoke fixture under `/home/claude/<module>/test_stubs/`
  that supplies any missing restd dependencies with `std::` aliases.
- Compile the umbrella under every supported standard tier with
  `g++ -Wall -Wextra` and confirm zero warnings.
- Run an end-to-end exercise (`./smoke`) that prints `ok` from every
  tier.

Common pitfalls:

- Trailing return types that reference `m_storage` parse before private
  members are visible. Use `std::declval<T&>()` etc. in the trailing
  return; bodies can use the real member.
- `bad_*_access` exception classes need three inheritance fallbacks
  (`std::bad_cast` → `std::exception` → standalone) gated on
  `D_ENV_CPP98_HAS_TYPEINFO` and `D_ENV_CPP98_HAS_EXCEPTION`. Pattern
  is in `any/bad_any_cast.hpp` and `expected/bad_expected_access.hpp`.
- C++11+ modules should gate every header on
  `#if D_ENV_LANG_IS_CPP11_OR_HIGHER` (whole-file include guards),
  not just the symbols that use C++11 features. Otherwise tag types
  and exception bases compile on C++98 but fail later because of
  ref-qualifiers / `constexpr` ctors.

Output: `docs/restd/inc/djinterp/restd/<module>/*.hpp` + the umbrella.

---

## Phase 2 — Coverage JSON

Each std header has exactly one JSON file:
`docs/restd/data/<header>.json`. These files are the data source; the
workbook builder **discovers them by globbing** `docs/restd/data/*.json`
— any file whose name starts with `_` (such as `_config.json` and
`_roadmap.json`) is skipped. One header JSON → one data sheet. There is
no manifest or module list to maintain: drop a JSON in the directory and
it becomes a sheet. The schema below is what `build_coverage_workbook.py`
consumes (see *Tooling architecture* at the end of this doc).

### Schema

```json
{
  "header":       "<expected>",
  "subtitle":     "Long per-header description shown in the title bar.",
  "legend_blurb": "Short blurb shown in the Legend sheet's listing.",
  "symbols": [
    {
      "name":               "expected<T,E>",
      "group":              "Wrapper class",
      "std_in":             "C++23",
      "restd_min":          "C++11",
      "constexpr_in_std":   "C++23",
      "constexpr_in_restd": "C++20",
      "intrinsic_required": false,
      "intrinsic_names":    "",
      "detection_macro":    "",
      "t_alias_in":         null,
      "v_var_in":           null,
      "deprecated_in":      null,
      "notes":              "Implementation notes, especially where restd differs from std.",
      "depends_on":         [],
      "failure_reason":     null,
      "reexport":           false
    },
    ...
  ]
}
```

### Field semantics

| Field | Meaning |
|---|---|
| `std_in` | Earliest C++ tier in std (e.g. `"C++23"`). Use `null` only for never-in-std symbols. |
| `restd_min` | Earliest tier restd ships at. `null` means not implemented. **`< std_in` is the back-port win** — those tiers are *covered by restd before std has them*, painted **green** (restd coverage) on the data sheet. Tiers where only std has the symbol (restd hasn't reached them, or `restd_min` is `null`) are **yellow** — a coverage gap. |
| `constexpr_in_std` / `constexpr_in_restd` | Same axis, for compile-time eval. `null` if N/A (e.g. tag types). In restd's layout this is **merged into the support cell**: a covered cell shows `cx` at the tiers where the symbol is constexpr (there is no separate constexpr column block). |
| `intrinsic_required` | `true` if the symbol cannot be implemented without a compiler builtin (e.g. `__is_class`). |
| `reexport` | `true` if the symbol is an identity-preserving re-export of a runtime/ABI/compiler/language-provided std entity that cannot be portably reimplemented — restd contributes only the in-namespace alias (`using std::X;` or a typedef), so `restd::X` **is** `std::X`. Drives the **Re-exports** sheet. Distinct from `intrinsic_required` (restd *owns* the symbol, its body just uses a builtin) and from a back-port (`restd_min < std_in`, where restd *does* provide its own implementation). See the README directive "Marking re-exports". |
| `intrinsic_names` / `detection_macro` | Surface only when `intrinsic_required` is true. |
| `t_alias_in` / `v_var_in` | When the `<symbol>_t` alias / `<symbol>_v` variable was added to std (typically C++14/17). |
| `deprecated_in` | When std deprecated the symbol (e.g. legacy `!=` operators since C++20). |
| `notes` | Implementation reality — algorithm choices, fallback behaviour, limitations. |
| `depends_on` | List of unmet restd dependencies (e.g. `"<compare>"`, `"restd::invoke"`). Empty when fully shipped. |
| `failure_reason` | Explanation when `restd_min` cannot reach C++98. Required when `restd_min != "C++98"` — drives the Coverage Failures sheet. |

### Validation

```bash
python3 -c "import json; json.load(open('docs/restd/data/<header>.json'))"
```

If JSON parses, move to phase 3.

---

## Phase 3 — Workbook build + roadmap update

### 3a. Roadmap entry

Edit `docs/restd/data/_roadmap.json`:

1. **If the module had a planned entry** (priority > 0), remove it.
2. **Add a priority-0 SHIPPED entry** right after the most recent
   priority-0 entry, e.g.:

```json
{
  "priority": 0,
  "header": "<expected>",
  "summary": "SHIPPED YYYY-MM-DD. ...one-paragraph summary including any deferred language-feature-ceiling symbols and design limitations...",
  "blockers": "—",
  "est_min_cpp": "C++11 / C++17 / C++20"
}
```

3. **If the module's local conventions land in the meta entry** (e.g.
   another `D_CONSTEXPR_CPP14` user), add the header to the meta
   entry's list (priority 9999).

### 3b. Regenerate

```bash
python3 scripts/restd/restd.py build
```

`restd.py` is restd's thin specialization: it injects restd's config
(see *Tooling architecture* below) and calls the generic
`build_coverage_workbook.py`. The builder reads every `*.json` in
`docs/restd/data/` (skipping anything starting with `_`) — one header
JSON per data sheet — and produces sheets in this order:

```
Legend → To Do → <header sheets, alphabetical> → Pending Dependencies → Coverage Failures → Re-exports
```

…writing to `docs/restd/restd_coverage.xlsx`. The first two and last
sheets are **synthesized** with no backing data file: Legend, To Do
(which also reads `_roadmap.json`), Pending Dependencies, Coverage
Failures, and Re-exports aggregate across all header JSONs and are
toggled by the `sheets` booleans in the config. The Re-exports sheet is
emitted only when at least one symbol is flagged `reexport: true`.

### 3c. Verify

Open the xlsx and spot-check:

- The new module's data sheet has the right colour pattern: **green
  where restd covers the symbol**, **yellow where only std has it** (a
  coverage gap — restd hasn't reached that tier, or the symbol is
  unimplemented), **red where neither covers it**.
- Covered cells show `cx` at the tiers where the symbol is constexpr.
  (The support and constexpr axes are merged into a single column block
  in restd's layout — there is no separate constexpr matrix.)
- The bottom of the sheet has a **Coverage Failures (this sheet only)**
  section if any symbol has a non-trivial `failure_reason`.
- The To Do sheet shows the module with a green completion fill if
  every symbol shipped at the lowest viable tier, or yellow/red
  otherwise.
- The Coverage Failures sheet picked up every symbol whose `restd_min`
  isn't `"C++98"` — each with a clear language-feature-ceiling reason.
- If the module re-exports any runtime/ABI/compiler/language-provided
  std entity (`reexport: true`), those symbols appear on the **Re-exports**
  sheet.

---

## Tooling architecture

The workbook is built by two files in `scripts/restd/`:

- **`build_coverage_workbook.py`** — the generic, project-agnostic
  engine. It knows nothing restd-specific: it speaks of a "primary"
  baseline tier and an optional "secondary" back-port tier and is driven
  entirely by a config object. Its canonical symbol fields are
  `primary_in` / `secondary_min` / `constexpr_in_primary` /
  `constexpr_in_secondary`, but it transparently accepts the restd field
  names (`std_in` / `restd_min` / `constexpr_in_std` /
  `constexpr_in_restd`) as aliases, so the data files load unchanged.
- **`restd.py`** — restd's thin specialization. It holds restd's config
  as the `RESTD_CONFIG` dict (primary = `std`, secondary = `restd`, the
  library-centric "coverage" colouring, lowercase categories, and the
  merged support/constexpr column block) and exposes `build`,
  `scaffold`, and `show-config` subcommands. `python3
  scripts/restd/restd.py build` is the equivalent of the old
  `build_workbook.py` run.

**Config is layered**, deep-merged in this order:

1. `DEFAULT_CONFIG` in `build_coverage_workbook.py` — generic baseline
   defaults.
2. An optional `_config.json` in the data dir — read if present.
3. An optional in-memory `config=` argument to `build()`.

restd uses layer 3: `restd.py` calls
`build_coverage_workbook.build(data_dir, config=RESTD_CONFIG)`. **There
is no `_config.json` on disk** — restd's configuration is baked into
`restd.py`, the single source of truth (dump it with `python3
scripts/restd/restd.py show-config`). Running the generic
`build_coverage_workbook.py` directly, without that config, produces a
neutral baseline look; restd's appearance only comes through `restd.py`.

**Data discovery is by globbing, not a list:** every `*.json` in
`docs/restd/data/` whose name does not start with `_` becomes one data
sheet (alphabetical). `_config.json` and `_roadmap.json` are skipped by
that leading-underscore rule.

**Synthesized sheets** have no backing JSON — Legend, To Do (also reads
`_roadmap.json`), Pending Dependencies, Coverage Failures, and Re-exports
— aggregating across every header JSON, toggled by the `sheets` booleans
in the config. Re-exports appears only when some symbol is flagged
`reexport: true`.

### Changing the workbook's appearance

restd's look — colours, the coverage-vs-baseline colouring mode,
category casing, and which column blocks appear — lives in
`RESTD_CONFIG` in `scripts/restd/restd.py`. Engine-wide visual
primitives that aren't exposed through config (the `FILLS` / `FONTS` /
fixed dimensions) live in the `STYLE` section near the top of
`build_coverage_workbook.py`. To re-theme: edit `RESTD_CONFIG` for
restd-specific choices, or the `STYLE` block for engine-wide primitives,
then re-run `python3 scripts/restd/restd.py build`.

---

## Common deliverables checklist (per module)

- [ ] Granular per-symbol `.hpp` files under
      `docs/restd/inc/djinterp/restd/<module>/`
- [ ] Umbrella header at `docs/restd/inc/djinterp/restd/<module>`
- [ ] Compile-tested under every supported `-std=` tier with
      `-Wall -Wextra`
- [ ] Smoke test prints `ok` at every tier
- [ ] `docs/restd/data/<header>.json` exists with the full symbol
      roster
- [ ] `failure_reason` populated for every symbol whose `restd_min`
      isn't `"C++98"`
- [ ] `depends_on` accurate (empty if no external blockers)
- [ ] `reexport: true` set on any runtime/ABI/compiler/language re-export
- [ ] `_roadmap.json` priority-0 SHIPPED entry added; any prior
      planned entry removed
- [ ] `restd.py build` re-run; xlsx regenerated and visually checked
