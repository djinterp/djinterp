# djinterp — Configuration Subframework

> **Location:** this file belongs at `config/README.md`.
> **Audience:** humans authoring/maintaining config, and agents editing it.
> **One-liner:** every piece of build-time configuration lives in a `cfg_*.h`
> header, exposes `D_CFG_*` toggles, and resolves itself **in that file** so the
> modules it targets stay logic-free.

---

## 0. Quick reference (read this first)

| Thing | Rule |
|---|---|
| **File name** | `cfg_<name>.h` — every config header is prefixed `cfg_` (excepting dconfig.h). |
| **File path** | Mirrors the target module's path under `config/` (e.g. module `c/memory/pool.h` → `config/c/memory/cfg_pool.h`). Infrastructure files that target no single module live at the config root. |
| **Public symbols** | `D_CFG_*` — user-settable knobs, always integer `0`/`1` (or a small enum). |
| **Derived symbols** | `D_INTERNAL_*` — computed *from* `D_CFG_*`; read by modules, never set by users. |
| **Where logic lives** | In the `cfg_*.h` file. The target module reads resolved symbols only — no `#ifndef D_CFG_… #define` blocks in module headers. |
| **Load order** | Every `cfg_*.h` includes `cfg_common.h` first; then its dependencies' configs; then its own cascade. |
| **Resolution** | User override → preset/aggregate → derived/propagated → env-detected → hard default. Each layer `#ifndef`-guarded. |
| **Testing** | `D_CFG_TESTING` (canonical) follows `D_TESTING`; when on, `cfg_testing.h` is auto-applied. |
| **Fixed-width integers** | Only through re_std: `cstdint/dstdint.h` (C), `cstdint/cstdint.hpp` (C++); its switches are in section 12. |

**File set at a glance**

| File | Role | Prefixed? | Targets a module? |
|---|---|---|---|
| `cfg_common.h` | Root: helpers, user pickup, testing flag, preset hook. Included first by *every* config file. | yes | no (infrastructure) |
| `dconfig.h` | Umbrella: `#include`s every `cfg_*.h`. Opt-in, for global resolution / PCH. | yes | no |
| `cfg_testing.h` | Testing/debug preset. Auto-applied when `D_CFG_TESTING == 1`. | yes | no |
| `cfg_custom.h` | Your overrides. Auto-detected via `__has_include`. **Not shipped.** | yes | no |
| `cfg_<module>.h` | Per-module config. | yes | **yes** — path mirrors the module |

---

## 1. What this is and why it exists

The configuration subframework is a set of **pure-preprocessor** headers (no C/C++
code, no build step) that gate framework functionality behind `D_CFG_*` toggles.
It is built on four principles:

1. **Localized.** All resolution logic for a knob — its default, any propagation,
   validation, and the derived values modules consume — lives in one `cfg_*.h`
   file. A module never re-derives config; it reads the answer.
2. **Single source of truth.** Each knob has exactly one canonical default, in
   one file. Everyone else *reads* it.
3. **Demand-loaded.** A module pulls its own `cfg_<module>.h`; if you never
   include the module, its config never loads. You pay only for what you use.
   (Preprocessing even thousands of guarded knobs is a few milliseconds, and
   re-inclusion is free thanks to include guards — so this is about clarity, not
   raw speed.)
4. **Overridable anywhere.** The `#ifndef`-guarded cascade means any value you
   `#define` before the framework loads — via `-D`, a prefix header, or
   `cfg_custom.h` — wins.

---

## 2. Naming & layout

### 2.1 File naming

- **Prefix every config header with `cfg_`.** No exceptions, other than `dconfig.h`.
- **Per-module configs mirror the module's path** under `config/`. If the module
  is `c/memory/pool.h`, its config is `config/c/memory/cfg_pool.h`. This makes the
  target obvious from the path and keeps config discoverable next to nothing but
  other config.
- **Infrastructure configs** (`cfg_common.h`, `dconfig.h`, `cfg_testing.h`,
  `cfg_custom.h`) target no single module and live at the config root.

### 2.2 Directory layout

```
config/
├── cfg_common.h            # root — included first by every cfg_*.h
├── cfg_custom.h            # YOUR overrides (auto-detected; git-ignore if desired)
├── cfg_qualifiers.h        # storage / linkage qualifiers
├── cfg_re_std.h            # djinterp's settings for re_std
├── cfg_testing.h           # testing/debug preset (auto-applied when testing)
├── dconfig.h               # umbrella — includes every cfg_*.h (opt-in)
├── env/
│   └── cfg_env.h           # targets env/env.h
├── c/
│   └── memory/
│       └── cfg_pool.h      # targets c/memory/pool.h
└── cpp/                    # the C++ modules' configs, likewise
```

### 2.3 Symbol naming

- **`D_CFG_<AREA>_<KNOB>`** — a public, user-settable knob. Always resolves to
  the integer `0` or `1` (or a small documented enum). Example:
  `D_CFG_DEFINE_INLINE`, `D_CFG_TESTING_STRIP_CONSTEXPR`.
- **`D_INTERNAL_*`** — a value *derived* from `D_CFG_*` inside a `cfg_*.h`,
  consumed by the target module. Not user-settable. Example:
  `D_INTERNAL_CFG_INLINE`, `D_INTERNAL_QUAL_TESTING`.
- Keep the area segment consistent with the module (`D_CFG_ENV_*`,
  `D_CFG_QUAL_*`, …) so the namespace mirrors the tree.

---

## 3. Shared vocabulary (`cfg_common.h`)

`cfg_common.h` is the root every config file includes first. It provides:

- **`D_CFG_ON` / `D_CFG_OFF`** — canonical `1` / `0`.
- **`D_CFG_NORM(x)`** → `(x + 0)` — normalizes a possibly-empty / possibly-
  undefined flag to an int. (No parentheses around `x`, so an empty expansion
  becomes unary `+ 0` == `0`.)
- **`D_CFG_IS_ON(x)` / `D_CFG_IS_OFF(x)`** — `#if`-safe predicates:
  `#if D_CFG_IS_ON(D_CFG_FOO)`.
- **`D_CFG_TESTING`** — the single canonical "testing/debug mode" flag,
  defaulting from `D_TESTING`.
- **User-override pickup** — auto-`#include`s `cfg_custom.h` if present.
- **Testing-preset hook** — `#include`s `cfg_testing.h` when `D_CFG_TESTING`.

**Empty-flag convention:** flags are always `0` or `1`. Undefined, `=0`, and true-
empty (`-DFLAG=`) all read as *off*; only `=1` (or GCC's bare `-DFLAG`, which is
`=1`) reads as *on*. **Always test flags with `D_CFG_IS_ON` / `D_CFG_NORM`,
never a bare `#if D_CFG_FOO == 1`,** so an empty define can't cause a syntax
error.

---

## 4. The resolution cascade (the core idiom)

Every knob resolves top-down. Each layer only applies if nothing higher set the
value, which is what the `#ifndef` guards enforce:

1. **Explicit user override** — the user `#define`s it (via `-D`, a prefix
   header, or `cfg_custom.h`).
2. **Preset / aggregate** — a group switch fills many knobs at once
   (`cfg_testing.h`, or a `D_CFG_<AREA>_ALL` toggle).
3. **Derived / propagated** — the framework's job: parent-on ⇒ child-on, or one
   module expressing a need to a dependency.
4. **Environment-detected** — a default taken from `env.h` detection.
5. **Hard default** — the final fallback.

You do not *deduce* "skip this module's defaults." If a module isn't included,
its `cfg_*.h` never loads and its defaults never run — the include graph is the
mechanism.

---

## 5. The localization rule (important)

> **All `D_CFG_*` logic lives in `cfg_*.h`. A target module contains no config
> resolution — it reads the already-computed `D_CFG_*` / `D_INTERNAL_*` symbols.**

Concretely: defaults, propagation, aggregate fallbacks, validation, and the
`D_INTERNAL_*` "effective value" computation all belong in the `cfg_<module>.h`.
The module header just consumes the result.

**Do** (in `cfg_qualifiers.h`):

```c
// compute the effective gate here, once
#if (D_CFG_TESTING == 1)
    #define D_INTERNAL_CFG_INLINE  /* … from testing knobs … */
#else
    #define D_INTERNAL_CFG_INLINE  /* … from normal knobs … */
#endif
```

**Do** (in the module, e.g. `djinterp.h`):

```c
#include "../config/cfg_qualifiers.h"   // pulls cfg_common.h transitively
// … just READ the resolved symbol …
#if !defined(D_INLINE) && (D_INTERNAL_CFG_INLINE == 1)
    #define D_INLINE  /* … */
#endif
```

**Don't** (config logic leaking into the module):

```c
// ❌ in djinterp.h — belongs in cfg_qualifiers.h
#ifndef D_CFG_DEFINE_INLINE
#  define D_CFG_DEFINE_INLINE 1
#endif
#if D_CFG_TESTING …            // ❌ re-deriving what cfg_qualifiers.h already resolved
```

Why: one place to change behavior, no drift between a knob's default in two
files, and modules that read as intent rather than plumbing.

---

## 6. Authoring a new `cfg_<module>.h`

### 6.1 Skeleton

```c
/*****************************************************************************
* djinterp [core]                                                cfg_<name>.h
*
*   <One or two sentences: what this configures.>
*
*   targets:  <module header(s) and the symbols they consume, e.g.
*             djinterp.h -> D_STATIC / D_INLINE / D_STATIC_INLINE>
*   requires: cfg_common.h (helpers, D_CFG_TESTING); <dependency cfgs>
*
* path:      /config/<mirrored/module/path>/cfg_<name>.h
* author(s): <name>
*****************************************************************************/

#ifndef DJINTERP_CONFIG_<PATH>_H
#define DJINTERP_CONFIG_<PATH>_H 1

// (0) root first: helpers, user overrides, testing flag/preset.
#include "<relative>/cfg_common.h"
// (0b) dependencies' configs, if this module's config depends on them.
// #include "../<dep>/cfg_<dep>.h"

// (1) user overrides are honored automatically (every #define below is guarded).

// (2) propagation — parent-on => child-on (guarded so user values win).
// #if D_CFG_IS_ON(D_CFG_<AREA>_PARENT)
// #   ifndef D_CFG_<AREA>_CHILD
// #     define D_CFG_<AREA>_CHILD 1
// #   endif
// #endif

// (3) defaults — the SINGLE canonical home for each knob.

// D_CFG_<AREA>_<KNOB>
//   brief: <what it gates, and what 0/1 mean>.
#ifndef D_CFG_<AREA>_<KNOB>
#   if defined(D_CFG_<AREA>_ALL)          // optional aggregate fallback
#       define D_CFG_<AREA>_<KNOB> D_CFG_<AREA>_ALL
#   else
#       define D_CFG_<AREA>_<KNOB> 0
#   endif
#endif

// (4) validation — cheap; catches typos before they mis-gate anything.
#if !D_CFG_IS_ON(D_CFG_<AREA>_<KNOB>) && !D_CFG_IS_OFF(D_CFG_<AREA>_<KNOB>)
#   error "D_CFG_<AREA>_<KNOB> must be 0 or 1"
#endif

// (5) derived / effective values consumed by the module (D_INTERNAL_*).
// D_INTERNAL_<AREA>_<THING>
//   brief: <what the module reads; how it is computed>.
#if (D_CFG_TESTING == 1)
#   define D_INTERNAL_<AREA>_<THING> /* … */
#else
#   define D_INTERNAL_<AREA>_<THING> /* … */
#endif

#endif  // DJINTERP_CONFIG_<PATH>_H
```

### 6.2 Ordering rules

- `cfg_common.h` **first**, always.
- A config that influences a **dependency**: set the dependency's override
  *then* `#include` the dependency's `cfg_*.h`, so its cascade honors your value.
- Within a knob: **propagation before defaults** (propagation uses `#ifndef` so
  a user value still wins; the default then sees the value set and skips).
- **Cross-cutting** deductions (module A flipping something in an unrelated
  module B) can't be ordered by demand-loading — put those in `dconfig.h`.

### 6.3 Aggregates (`D_CFG_<AREA>_ALL`)

Don't force N child defines from an aggregate (you can't `#define` from a macro
anyway). Instead make each child's **default fall back to the aggregate**, as in
the skeleton's knob block: individual override > aggregate > hard default,
composes cleanly, no central block.

---

## 7. Wiring a module to its config

1. In the module header, `#include` its `cfg_<module>.h` (which pulls
   `cfg_common.h` transitively). Do this near the module's other includes.
2. Read only resolved symbols (`D_CFG_*`, `D_INTERNAL_*`). Add no config logic.
3. For global up-front resolution or a precompiled header, include `dconfig.h`
   once, early — otherwise rely on per-module demand-loading.

Example (qualifiers): `djinterp.h` includes `cfg_qualifiers.h`, then defines
`D_STATIC` / `D_INLINE` / `D_STATIC_INLINE` gated on `D_INTERNAL_CFG_*`;
`djinterp.hpp` inherits `cfg_qualifiers.h` through `djinterp.h` and defines only the
constexpr family.

---

## 8. Testing / debug configuration (`cfg_testing.h`)

- `cfg_common.h` normalizes `D_TESTING` into **`D_CFG_TESTING`** (the single flag
  every config reads) and, when it's `1`, auto-applies `cfg_testing.h`.
- `cfg_testing.h` is the **one visible home** for "what differs in a test/debug
  build." Its knobs are `#ifndef`-guarded, so explicit user overrides still win:
  **user overrides > testing preset > module defaults.**
- Trigger it by building with `-DD_TESTING=1`. To force it without `D_TESTING`,
  set `-DD_CFG_TESTING=1`; to couple it to unoptimized builds instead, define
  `D_CFG_TESTING` from `env.h`'s `D_ENV_BUILD_DEBUG` in `cfg_custom.h`. Suppress it
  entirely with `-DD_CFG_NO_TESTING_PRESET`.
- Add a subframework's testing overrides as a labelled, guarded block in
  `cfg_testing.h`; keep the per-module *defaults* in the module's `cfg_*.h`.

---

## 9. Documentation & commenting standards

**Every config file has a header comment** that states, at minimum:

- **what it configures** (one or two sentences),
- **`targets:`** — the module header(s) it serves and the symbols they consume,
- **`requires:`** — `cfg_common.h` plus any dependency configs,
- path and author.

**Every symbol is commented** in the two-line house format:

```c
// D_CFG_<AREA>_<KNOB>
//   brief: <what it gates; what 0 and 1 mean; any default rationale>.
```

A family that one sentence describes shares one brief. Its head names every
member, in one of the forms the tree uses, and the members are defined right
below it:

| Head | For | Example |
|---|---|---|
| `A / B` | two or three names | `// D_CFG_IS_ON(x) / D_CFG_IS_OFF(x)` |
| `A / _B / _C` | siblings; a name that starts with `_` is the first name's prefix plus it | `// D_INTERNAL_MEM_POISON / _POISON_ALLOC / _POISON_FREE` |
| `A .. B` | a numbered run | `// D_CFG_DMACRO_INCLUDE_TUPLE_1 .. D_CFG_DMACRO_INCLUDE_TUPLE_16` |
| `PREFIX_*`, `PREFIX_<X>` | a whole family, by its pattern | `// D_CFG_DMACRO_VARIANT_*`, `// D_INTERNAL_TABLE_DEFAULT_<CATEGORY>` |

- Use `//` comments and an 80-column limit (matches the rest of the tree).
- Give each file a short table-of-contents block if it defines more than a
  handful of knobs.
- `D_INTERNAL_*` values get a `brief:` too, noting how they're computed and who
  reads them.

---

## 10. Checklists

### Create a new `cfg_<module>.h`
- [ ] Named `cfg_<module>.h`, placed at the config path mirroring the module.
- [ ] Header comment with `targets:` and `requires:`.
- [ ] Include guard `DJINTERP_CONFIG_<PATH>_H`, named after the file's path
  under `inc/djinterp/config/` (`c/memory/cfg_pool.h` has
  `DJINTERP_CONFIG_C_MEMORY_CFG_POOL_H`); `#include "…/cfg_common.h"` first.
- [ ] Dependency configs included (overrides set *before* including them).
- [ ] Every knob: single canonical default, `#ifndef`-guarded, commented.
- [ ] Aggregates done as per-child fallbacks, not central force-blocks.
- [ ] Validation `#error` for each boolean knob.
- [ ] `D_INTERNAL_*` effective values computed here; commented.
- [ ] Added to `dconfig.h`'s include list (dependency order).

### Add a knob to an existing config
- [ ] Lives in the right `cfg_*.h` (never in a module header).
- [ ] `D_CFG_<AREA>_<KNOB>`, integer `0`/`1`, `#ifndef`-guarded default.
- [ ] Commented (`brief:`), validated.
- [ ] If it changes for tests, add the guarded override to `cfg_testing.h`.

### Wire / edit a module
- [ ] Module includes its `cfg_<module>.h`; no `D_CFG_` defaults in the module.
- [ ] Module reads only `D_CFG_*` / `D_INTERNAL_*`.
- [ ] Flags tested via `D_CFG_IS_ON` / `D_CFG_NORM`, not bare `== 1`.

---

## 11. Gotchas

- **You cannot `#define` from inside a macro expansion.** Aggregates and
  propagation are written as explicit directive blocks (or generated offline);
  the variadic macros in `dmacro.h` can't emit config `#define`s.
- **Keep include guards clean** — the guard macro must be the first thing in the
  file and `#endif` the last, or the compiler's multiple-include optimization
  won't skip re-reads. Don't over-fragment into hundreds of micro-files; group
  config per subframework.
- **Empty flags read as off.** Test with `D_CFG_IS_ON` / `D_CFG_NORM`; never
  `#if D_CFG_FOO == 1` on a flag that could arrive empty.
- **Cross-cutting deductions need `dconfig.h`.** Demand-loading resolves in
  include order; long-range "A flips B" rules only resolve deterministically
  when everything is loaded up front.
- **One canonical default per knob.** If a default appears in two files, that's
  a bug waiting to drift — delete one and read the other.

---

## 12. Fixed-width integers: re_std's switches

djinterp takes the fixed-width integer types, limits, constants and formats
from re_std: `cstdint/dstdint.h` in C, `cstdint/cstdint.hpp` in C++, never
`<stdint.h>`, `<cstdint>`, `<inttypes.h>` or `<cinttypes>` directly (the
style guides' "Include Directives"; `tools/check_stdint_includes.py`
enforces it). re_std reads only its own `config.h`. djinterp's config
carries its switches into re_std through `config/cfg_re_std.h`, which
`cfg_common.h` includes.

| Switch | Values | Meaning |
|---|---|---|
| `RE_STD_CFG_STDINT_BACKEND` | `RE_STD_CFG_STDINT_BACKEND_AUTO` (0, the default), `_PLATFORM` (1), `_OWN` (2) | Which definitions `dstdint.h` gives: the platform's headers, completed where they fall short, or the header's own, from the compiler's predefined types. AUTO takes the platform's where they can serve; under ISO strict C++98 that is the header's own. Any other value is an `#error`, and so is `_PLATFORM` where the platform's `<stdint.h>` cannot serve. It replaces `D_CFG_STDINT_BACKEND`, which no longer exists: a build that pinned that sets this. |
| `D_CFG_ENV_ISO_STRICT` | 0 (default), 1 | ISO strict C++98: no `long long`, no variadic macros. `config/cfg_re_std.h` turns re_std's `RE_STD_CFG_ISO_STRICT` on with it. |
| `RE_STD_HAS_HEADER_INTTYPES` | 0, 1 | The build's word on whether an `<inttypes.h>` exists. Only a build with a `<stdint.h>` of its own and no `<inttypes.h>` needs it, as 0: Clang's own `<inttypes.h>` wrapper makes the question unanswerable from inside a header. |
| `RE_STD_POINTER_BITS` | 16, 32, 64 | The pointer's width, where the compiler does not say. `D_ENV_ARCH_POINTER_BITS` (`env/env_arch.h`) is djinterp's; `dstdint.c` holds both to `sizeof(void*)`. |

Three things to know:

- The carry is `#ifndef`-guarded, so it holds only where djinterp's config
  is read before the first re_std header -- through the framework root or
  `cfg_common.h`. djinterp's own files see to it: their re_std includes sit
  in a `// re_std` group after `// djinterp`. Where a translation unit reads
  re_std's config first anyway and djinterp's later, and the two disagree,
  `config/cfg_re_std.h` stops the build with an `#error` rather than let
  re_std run on another configuration than djinterp's. A build that must
  not depend on include order at all sets each `RE_STD_CFG_*` switch beside
  its `D_CFG_*` one -- `RE_STD_CFG_ISO_STRICT=1` with
  `D_CFG_ENV_ISO_STRICT=1`, `RE_STD_CFG_TESTING=1` with `D_TESTING=1` -- as
  the CI ladder does.
- Clang 20 and older, freestanding, on the platform backend, give
  `UINT8_C` and `UINT16_C` the wrong type (WG14 DR 209, fixed in Clang 21);
  the own backend is the way out.
- MinGW with `__USE_MINGW_ANSI_STDIO=0` has no 8-bit scanf formats, by
  design: Microsoft's runtime would overrun the argument.
