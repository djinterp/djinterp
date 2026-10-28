# re_std batch — 2026-08-25

Seven headers. Five new modules, one repaired, two verified.

## Deliverables

| Path | What |
|---|---|
| `re_std_modules_2026-08-25.zip` | The headers. Unpack over `/inc/djinterp/`. |
| `data/*.json` | Phase-2 coverage files for the six new headers. |
| `data/_roadmap.json` | Updated roadmap (125 entries). |
| `restd_coverage.xlsx` | Rebuilt workbook, 42 sheets. |
| `tests/` | Smoke fixtures + `run.sh`. **Not deliverables** — see below. |

## Status

| Header | Files | Symbols | Note |
|---|---|---|---|
| `<cstddef>` | 8 | 11 | `byte` back-ported C++17 → C++11 |
| `<cstdint>` | 2 | 32 typedefs / 10 rows | all re-exports, optional families gated |
| `<climits>` | 2 | 6 rows | macro-only; fills the C++11 `LLONG_*` set |
| `<cfloat>` | 2 | 11 rows | macro-only; fills the C11-via-C++17 set |
| `<cstdlib>` | 9 | ~40 / 13 rows | all re-exports, 2 symbols gated |
| `<chrono>` | 27 | 28 of 38 groups | core only; calendar + zones deferred |
| `<ratio>` | 13 | — | **repaired**, see below |
| `<numbers>`, `<bitset>` | — | — | verified, unchanged |

Verification: 250/250 standalone header compilations, 7 fixtures × 5 tiers
(C++11/14/17/20/23), `-Wall -Wextra -pedantic`, plus all 15 forced
clock-source configurations.

## Three things to look at

**`<ratio>` was broken.** All 12 files had the core include mangled to
`// djinterp', '#include "../../core/djinterp.hpp"', '` — one line, entirely a
comment, so none of them included `djinterp.hpp`. Looks like a Python list
str()-ed into a replacement. Nothing compiled. Repaired and verified. Worth
grepping the rest of the corpus for that signature; it's confined to `ratio/`
in this zip, but whatever produced it isn't.

**`re_std::byte{0x2A}` does not back-port.** Direct-list-initialising a scoped
enum from an integer is P0138R2, a C++17 *language* change. The type reaches
C++11 fine; only that spelling doesn't. C++11/14 callers write
`static_cast<byte>(0x2A)`, constexpr at every tier. No `to_byte()` factory was
added to hide it — std has no such function, and inventing surface std lacks
would break portability in the other direction. Recorded as a language ceiling
in `cstddef.json`.

**`<chrono>` days/months/years are fixed-length durations.** `t + years(1)`
advances by the mean Gregorian year (31556952s), *not* to the same date next
year. The shipped `d` and `y` literals are the duration forms; when the
calendar lands, `15d` and `2020y` become ambiguous with `chrono::day` /
`chrono::year` and the literals will need splitting by context. Recorded in the
new roadmap entry (priority 39) rather than worked around.

## The tests directory is a fixture, not a deliverable

`tests/core/djinterp.hpp` is a **stand-in** for the real core header, which the
corpus ships without. It supplies exactly the macro vocabulary the headers
reference, derived from the tables in `RE_STD_AGENT_README.md`. Delete it and
build against the real `core/djinterp.hpp` before trusting any result.

```sh
./run.sh test/t_chrono.cpp      # compiles + runs at all five tiers
```

`run.sh` deletes its binary before each compile — an earlier version of this
batch reported `ok` from a stale binary after a failed build.

## Known-imperfect, deliberately

- Every new module floors at C++11, so all rows render as uncovered against the
  workbook's C++98 floor. Each carries a `failure_reason` saying so. This is the
  known tier-floor item, not new debt.
- `duration_cast` overflow of the common type is unchecked, matching std.
- `chrono::abs(duration::min())` is undefined, matching std.
- `re_std::abs` on a floating-point argument is not portable until `<cmath>`
  lands — a using-declaration captures whatever `std::abs` overloads are visible
  when the header is parsed.

## Post-delivery audit (added after the batch shipped)

Verified the "Known false-greens" list in `re_std_modules.md` against the
filesystem rather than the data. **All ten are still false**, and they are not
only a cataloguing problem:

- 8 `<type_traits>` files do not exist. `optional_base.hpp` uses one of them
  (`is_trivially_copyable`), so **`<optional>` does not compile**, and
  `<ranges>` fails through it.
- `compare/compare` includes 2 files that do not exist, so **the `<compare>`
  umbrella does not compile**.

Then compiled all 27 module umbrellas at the placement each one's own includes
imply. **21 pass, 6 fail.** Beyond the four above:

- `<expected>` — `unexpected.hpp:170` uses `std::swap` with no `<utility>`.
- `<string_view>` and `<tuple>` — mis-ordered namespace macros.

**Four files have `NS_` opens and `NS_END` closes that are count-balanced but
wrongly sequenced**, so a plain tally reports them clean (my first check did
exactly that and missed them). A running-depth check finds them:
`algorithm/sort.hpp:337`, `algorithm/stable_sort.hpp:354`,
`string_view/char_traits.hpp:392`, `tuple/tuple.hpp:118`.

`char_traits.hpp` shows the full damage: `NS_INTERNAL` opens with no enclosing
`NS_RESTD`, a stray `NS_END` emits `}` at global scope, and the later
`NS_RESTD` is never closed — so `char_traits<char>` sits at global scope and
everything included afterwards nests into `re_std::re_std`.

The two `algorithm` files are the ones to worry about: `<algorithm>` compiles
anyway, because a sibling leaves a namespace open where they are included. They
are correct only by include order — the same failure mode as the `<ratio>`
mangling and the clock-header missing includes found earlier in this batch.

`tests/audit.sh` re-runs the umbrella sweep; the order-aware namespace check is
in `re_std_modules.md`.

**None of these are mine** — they predate this batch, and I have not modified
any of the affected modules. Fixing them is a separate decision.

---

# Repair batch — same day

Requested: the 8 missing type traits, the comparables, `<expected>`'s missing
`<utility>`, and the namespace errors. All done. **Umbrellas now compile: 25 of
27, up from 21.**

| Item | Result |
|---|---|
| 8 intrinsic type traits | written, `is_scoped_enum` needs **no intrinsic** — exact on both paths |
| **21 orphaned traits** | files existed, umbrella never included them — now wired in |
| `three_way_comparable` / `_with` | written; C++20-only |
| `<expected>` missing `<utility>` | fixed |
| 4 namespace-order defects | fixed (`sort`, `stable_sort`, `tuple`, `char_traits`) |
| `<optional>` missing `in_place.hpp` include | fixed (found en route) |

## Three things worth your attention

**The false-green list was incomplete.** Beyond the documented 8, **21 further
`<type_traits>` headers existed on disk but were never included by
`type_traits.hpp`** — the module's documented entry point. All 21 compile
standalone, so they were working code withheld from every caller. This is what
made `optional.hpp` fail on `is_nothrow_swappable` and left comments in
`utility/pair.hpp` and `utility/swap.hpp` saying the trait "re_std does not yet
[have]" — it did, in the same directory. Now wired in under a new section XV.

**`is_scoped_enum` needed no intrinsic.** Scopedness is observable through the
type system (`is_enum` + `!is_convertible<T, underlying_type_t<T>>`), so unlike
the other seven its fallback is *exact*, not degraded. I verified both paths
separately with every intrinsic forced off.

**The comparables exposed a real inconsistency in `<compare>.`** My first version
failed on `int`. Cause: `re_std::partial_ordering` is re_std's **own class**
(back-ported to C++11), but `operator<=>` is a *language* feature that can only
ever yield `std::strong_ordering`. Folding a std category against an re_std one
gives `void`, so the concept matched nothing. The concepts therefore take
**`std::`** category arguments — which matches `compare_three_way_result`, whose
own docs already say it yields `std::strong_ordering`. Worth a decision: re_std
owns four category types the language will never produce.

Also corrected in `compare.json`: both concepts were catalogued at a **C++11**
floor. That was wrong twice — the files didn't exist, and the tier is
unreachable in principle. Now C++20 with a `failure_reason`.

## Still broken (2 of 27) — beyond what you asked for

- **`<optional>`** — `internal::throw_bad_optional_access()` is *called in three
  places and defined nowhere in the corpus*. A genuinely missing function, not a
  missing include.
- **`<ranges>`** — `view_interface.hpp:69`, a constexpr/const issue. Not
  investigated.

I stopped here rather than expanding scope again. Both are small and I can take
them next.

---

# Close-out: optional + ranges

**All 27 umbrellas compile at all five tiers — 135/135.**

## `<optional>` — a function called three times and defined nowhere

`internal::throw_bad_optional_access()` had no definition in the corpus.
Written into `bad_optional_access.hpp`, next to the exception it raises.

Two decisions in it worth knowing:

- **A function, not a bare `throw` at each call site.** `value()` is constexpr
  and its body is a conditional expression — `engaged ? m_value :
  (throw_bad_optional_access(), m_value)` — and a throw-expression is not a
  valid operand there before C++14. The call keeps the expression legal at
  every tier and puts the exceptions-disabled handling in one place.
- **Not marked `D_CONSTEXPR`.** It never succeeds, so it can never appear in a
  successful constant evaluation; marking it constexpr would imply otherwise.
  The enclosing `value()` stays constexpr regardless.
- With exceptions disabled it calls `std::abort()`. There is no value to return
  and no way to report; handing back a reference to disengaged storage would be
  strictly worse than a clean abort.

## `<ranges>` — 28 C++11 conformance defects, all one root cause

`ranges.json` claims a **C++11 floor for 59 symbols**. The module did not
compile at C++11 *at all*.

Every defect is hard-won rule 2 — on C++11 `constexpr` on a member function
implies `const` — showing up as three different diagnostics:

| symptom | count | why it looks different |
|---|---|---|
| `cannot be overloaded` | 22 | a non-const/const pair collapses to two identical signatures |
| `increment of read-only location` | 5 | a **mutator with no const twin** — no collision to diagnose, it just silently became const |
| `constexpr constructor does not have empty body` | 1 | C++11 requires an empty constexpr ctor body |

The middle row is the one to remember: it produces no overload clash, so a
collision-driven check misses it entirely. My first pass did exactly that and
had to be extended.

Fixes were **error-driven, not heuristic** — `tests/fix_rule2.py` and
`fix_rule2b.py` parse the compiler's own diagnostics and re-qualify only the
functions it names, so free functions and already-const members are never
touched. Files: `view_interface`, `single_view`, `owning_view`, `reverse_view`,
`take_while_view`, `join_view`, `as_rvalue_view`, `as_const_view`,
`repeat_view`, `movable_box`.

**One self-inflicted error, caught and fixed:** a blind string replace turned
`D_CONSTEXPR_INLINE` into `D_CONSTEXPR_CPP14_INLINE`, a macro that does not
exist — briefly breaking tiers that had been passing. The per-tier matrix caught
it; a C++20-only check would not have.

---

# Design decision recorded + warning cleanup

## The `<compare>` category split is now documented, not just decided

Wrote the rule into `compare/compare`'s header comment, where someone about to
write an `operator<=>` will actually hit it:

> **Writing an `operator<=>` by hand** on a class that must work at C++11 →
> return an **re_std** category, that is what they are for.
> **Constraining on, or inspecting, the result of the builtin `<=>`** → use
> **std's**, because the subject is `decltype(a <=> b)` and the language decides
> that.

The comment also records the failure mode, which is the dangerous part:
`common_comparison_category` ranks re_std's categories only, so folding a std
category against an re_std one falls through to `void` and a `same_as` check
just fails. **A constraint written with the wrong set does not misbehave loudly
— it matches nothing.** That is exactly how the first `three_way_comparable`
came to reject `int`. Both rejected alternatives are recorded with their
reasons.

## Zero warnings across 135 umbrella × tier builds

Was 13. Four fixes:

| Warning | Cause | Fix |
|---|---|---|
| inline variables are C++17 | **my test stub** defined `D_CONSTEXPR_INLINE` as `constexpr inline`, but the corpus applies it to *variable* definitions | stub routes through `D_INLINE_VAR` — tells us the real core header must gate the inline part |
| literal suffix not preceded by `_` | re_std *is* the implementation, so reserved suffixes are correct | scoped `push`/`ignored`/`pop`, as std's own headers do |
| `-Winvalid-constexpr` on `bad_optional_access()` | ctor was `D_CONSTEXPR` but its `std::exception` base ctor is not — ill-formed NDR | constexpr only on the standalone (no-base) path |
| floating-point suffix `d` shadowed | **my mistake** — see below | removed |

**I had invented API std does not have.** I gave `operator""d` and `operator""y`
`long double` overloads to match the other six suffixes. The standard declares
**only** the `unsigned long long` form for those two — `d` collides with the
decimal-float suffixes some compilers offer. Removed. This is the same principle
I applied when declining to add a `to_byte()` factory for `<cstddef>`; I did not
apply it consistently the first time, and the compiler caught it.

## Two findings left on the table

**106 headers fail to compile standalone** (1282/1388 across all modules at
C++11 and C++20) — they work only via umbrella include order. Concentrated in
`memory` (23), `ranges` (9), `type_traits` (6), `functional` (5). This is the
same order-dependence class as the `<ratio>` mangling and the clock includes;
the umbrellas are all green, so it is latent rather than breaking.

**`internal::throw_bad_variant_access()` is called and never defined** — the
exact bug I just fixed in `<optional>`, still live in
`variant/variant_visit.hpp`. `<variant>`'s umbrella compiles only because that
header is orphaned from it. Same one-function fix.

---

# Standalone header repair — 3470/3470

Every header in the corpus now compiles **on its own** at C++11/14/17/20/23.
Umbrellas remain 135/135 with **zero warnings**; namespace-order defects and
includes-inside-a-namespace are both at zero.

## What the original 106 actually were

**46 were my test fixture, not the corpus.** The stub was missing `D_DELETE_FN`,
`D_ENV_HAS_LONG_LONG`, the long-long diagnostic pair, and the inline-variables
feature macro. Worth stating plainly: nearly half the "corpus bugs" I reported
were mine. The stub is now checked against the corpus's full macro vocabulary.

The remaining 60 were real, in five classes:

| Class | Examples |
|---|---|
| **Wrong include paths** (13 files) | `../functional/compare_three_way.hpp` → `../compare/`; `range_access.hpp` → `ranges_access.hpp`; `iterator_tags.hpp` and `range_traits.hpp` don't exist — content lives in per-symbol headers |
| **Missing includes** (30+ files) | `get`, `apply`, `sentinel_t`, `make_index_sequence`, `range_reference_t`, `remove_cv`, `<type_traits>`, `<initializer_list>` |
| **`#include <new>` inside `namespace re_std`** | `new/launder.hpp`, `new/destroying_delete.hpp` — declares `operator new` in a namespace, which is ill-formed |
| **Wrong construct** | `span_range_traits.hpp` specialised `enable_borrowed_range`/`enable_view` as *variable templates*; they are **struct traits**. Also opened `ranges` when they live in `re_std::` |
| **Typos / tier bugs** | `restd::` for `re_std::` (7×) in `ranges/iter_move.hpp`; `equal_to<>` needs a C++14 default arg, so the three searchers failed at C++11 |

Also fixed: **`internal::throw_bad_variant_access()`**, called and defined
nowhere — the same bug as `<optional>`'s, now written into
`bad_variant_access.hpp`.

## A cycle I created, and what it revealed

Wiring the 21 orphaned type_traits into the umbrella exposed a latent
**three-way include cycle**: `invoke_result.hpp` (type_traits) needs
`is_reference_wrapper` (functional), which pulled the whole type_traits
umbrella back, and `reference_wrapper.hpp` → `invoke.hpp` → `is_reference_wrapper`
closed a second loop. It had never surfaced because `invoke_result.hpp` was
orphaned and therefore never compiled.

Fixed by narrowing `is_reference_wrapper.hpp` to the traits it actually uses and
**forward-declaring** `reference_wrapper` instead of including it — the same
technique as `chrono/duration_fwd.hpp`. `invoke_result.hpp` now declares its
`functional/` dependency explicitly.

**I broke the tree doing this** — one intermediate state went from 47 failures to
295. The per-header sweep caught it immediately; a build that only checks
umbrellas would have shipped it.

## `tests/standalone_sweep.sh`

Added, and it belongs in CI next to `audit.sh`. Every bug in this session — the
`<ratio>` mangling, the clock includes, the namespace defects, and all 60 of
these — is caught by one of those two scripts, both of which run in seconds.

---

# Correction: the C++98 floor claim was false

Every `failure_reason` in this batch carried the line:

> "core/djinterp.hpp hard-errors below C++11, so C++98 is not a reachable tier"

**It is not true.** `djinterp.hpp` contains nothing that hard-errors at C++98.
Compiled at `-std=c++98 -pedantic`, exactly two constructs are post-C++98 and
both are warnings at most: an anonymous variadic macro in `D_NOEXCEPT_IF(...)`,
and `_Pragma` in the long-long diagnostic pair.

`RE_STD_AGENT_README` says the opposite of the claim: *"compiles on C++98
through C++26"*, with a macro table that degrades every C++11 feature
(`D_CONSTEXPR` → empty, `D_CONSTEXPR_INLINE` → `inline`) and a review checklist
of explicit C++98 idioms. Decisively, its known-issues table logs
*"any.hpp | Entire file gated on C++11; needs C++98 path | **Planned work**"* —
blanket C++11 gating is tracked as a **defect**, not an accepted floor.

**Provenance.** I checked rather than assumed: the sentence is verbatim
pre-existing in `bit.json` (14×), `numbers.json` (13×) and `memory.json` (1×). I
did not invent it — but I adopted it for 49 more fields without verifying a
load-bearing claim about a file nobody in this working set has, and used it to
justify gating 38 of 50 new files. The tell was in my own output: I never gated
`<climits>` or `<cfloat>`, and both are C++98-clean and working.

## What was corrected

| | |
|---|---|
| `failure_reason` / `notes` fields | **85** corrected across 13 JSONs (49 mine, 36 pre-existing) |
| `<cstdlib>` | **ungated** — C++98 families now reach C++98; `atoll`, `strtof`, `strtold`, `strtoll`, `strtoull`, `_Exit`, `llabs`, `lldiv`, `lldiv_t` gated individually; `quick_exit` pair gate made tier-aware |
| `cstdlib.json` | 11 rows moved from a C++11 to a **C++98** floor |
| Roadmap | 2 new entries: **priority 41** (C++98 paths for this batch) and **priority 42** (`<chrono>` C++98 path) |

`re_std::atoi`, `malloc`, `free`, `abs`, `size_t`, `ptrdiff_t`, `offsetof` and
the `<climits>`/`<cfloat>` macros now all compile at `-std=c++98`.

## What genuinely stays gated

- **`nullptr_t`, `max_align_t`** — language constructs with no C++98 existence.
  Correctly gated.
- **`byte`** — needs the struct-wrapper idiom the checklist prescribes for
  `enum class`. A real back-port; logged at priority 41.
- **`<cstdint>`** — the header itself is C++11; needs a `<stdint.h>` fallback.
  Logged at priority 41.
- **`<chrono>`** — `std::chrono` is C++11, so a C++98 duration/time_point is
  exactly the back-port this library exists for, and the largest coverage win
  left in the module. Logged at priority 42 with the specific changes required.

No regression: umbrellas 135/135 with zero warnings, standalone 3470/3470,
all fixtures green.

**One open question this raises:** the "OPEN tier-floor item" may rest on the
same false premise. If C++98 is reachable, the workbook's C++98 floor is
correct and the *modules* are what need moving — the opposite of the conclusion
I offered earlier.

---

# The unblocked batch — and a 100-header finding

Three of the four items turned out **not to be missing code**. The files
already existed; their module umbrella just never included them.

| Item | What it actually was |
|---|---|
| `<optional>` comparisons, swap, hash, `<=>` | 8 headers present since the start, **never included** by `optional/optional` |
| `<utility>` `move_if_noexcept` | fully written; its stated blocker (`is_copy_constructible`) had long since landed |
| `<type_traits>` priorities 3 + 4 | already shipped — they were among the 21 orphans I wired in earlier today |
| `<tuple>` `operator<=>` | real bug, below |

**11 catalogued symbols went from "unimplemented" to shipped by adding include
lines.** The coverage data was *under*-reporting, which is the mirror of the
false-green problem and arguably worse: it hides finished work.

## 100 orphaned headers corpus-wide

A sweep for "exists under `re_std/<module>/` but absent from the umbrella"
found **100**. Some are legitimately internal (`optional_base`, `func_storage`,
`searcher_detail`, `literal_zero_helper`, `sp_control_block`). Most are not:

| module | orphans | includes |
|---|---|---|
| `iterator` | **33** | `advance`, `distance`, `next`, `prev`, `begin`/`end`, `reverse_iterator`, every iterator tag |
| `functional` | 13 | `bind`, `bind_front`, `bind_back`, all three searchers, `function_ref` |
| `ranges` | 9 | `zip_view`, `enumerate_view`, `common_view`, … |
| `exception` | 8 | `terminate`, `exception_ptr`, `nested_exception` |
| `memory`, `utility`, `any`, `tuple`, … | 4–5 each | `any_cast`, `make_any`, `bad_any_cast`, … |

This is now logged at **priority 43**, and I'd argue it is the single largest
source of catalogue error in the project. `<iterator>` at 33 means the module's
documented entry point surfaces almost nothing.

## The one real bug: the category split, again

`<tuple>`'s `operator<=>` failed with
`common_comparison_category_t<std::strong_ordering, std::strong_ordering>`
resolving to **`void`** — the exact hazard I documented this morning, now
confirmed as a live defect rather than a design note.

It affected **four modules**: `tuple`, `pair`, `array`, `variant`. Each folded
the results of the *builtin* `<=>` (which yields `std::` categories) through
*re_std's* fold, which ranks only re_std's categories. All four now use
`std::common_comparison_category_t`, per the rule in `compare/compare`'s header.
`pair`, `array` and `tuple` verified working at C++20/23.

That is two separate live bugs from this one split in a single day. It is worth
revisiting whether the four re_std category types should exist at all.

## Not done

**`<compare>` Phase C6** — the floating-point special case in
`strong_order`/`weak_order`. Untouched; it is genuine implementation work
(IEEE-754 `totalOrder` semantics), not a wiring fix, and I stopped rather than
start it with the session this deep.
