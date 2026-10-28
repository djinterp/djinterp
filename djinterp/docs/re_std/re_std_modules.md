# re_std modules

37 modules catalogued, 816 symbols.
26 of 125 roadmap headers are shipped.

**Coverage counts below are "symbols with an implementation", not the workbook
percentage.** The workbook floors at C++98 while `core/djinterp.hpp` hard-errors
below C++11, so a fully-implemented C++11 module still renders as 0%. That is
the open tier-floor question, not a gap. See `MODULE_WORKFLOW.md`.

| header | implemented | notes |
|---|---|---|
| `<algorithm>` | 89/93 | Per-symbol coverage of the C++ <algorithm> header. Phase 10 (2026-05-13) shipped 89 algorithms: … |
| `<any>` | 5/5 | Per-symbol coverage of the C++ <any> header (C++17).… |
| `<array>` | 14/14 | std::array<T,N> — C++11's fixed-size aggregate sequence container. re_std back-ports the class i… |
| `<bit>` | 14/14 | Per-symbol coverage of the C++ <bit> header. SHIPPED 2026-08-25. All 14 catalogued symbols acros… |
| `<bitset>` | 14/17 | Per-symbol coverage of the C++ <bitset> header. SHIPPED 2026-08-25. 14 of 17 catalogued symbols … |
| `<cfloat>` **(new)** | 11/11 | Per-symbol coverage of the C++ <cfloat> header. SHIPPED 2026-08-25. Macro-only, like <climits>, … |
| `<chrono>` **(new)** | 27/37 | Per-symbol coverage of the C++ <chrono> header. CORE SHIPPED 2026-08-25: 28 of 38 catalogued sym… |
| `<climits>` **(new)** | 6/6 | Per-symbol coverage of the C++ <climits> header. SHIPPED 2026-08-25. A macro-only module: every … |
| `<compare>` | 17/17 | Per-symbol coverage of the C++ <compare> header (C++20). Phases C1-C5 (2026-05-17) shipped all 1… |
| `<cstddef>` **(new)** | 11/11 | Per-symbol coverage of the C++ <cstddef> header. SHIPPED 2026-08-25. All 11 catalogued symbols a… |
| `<cstdint>` **(new)** | 10/10 | Per-symbol coverage of the C++ <cstdint> header. SHIPPED 2026-08-25; REACHES C++98 since the face's… |
| `<cstdlib>` **(new)** | 13/13 | Per-symbol coverage of the C++ <cstdlib> header. SHIPPED 2026-08-25. All 40-odd symbols across 8… |
| `<exception>` | 13/13 | std::<exception> — the foundational exception-handling header. re_std's role mirrors <new>: the … |
| `<expected>` | 14/14 | std::expected<T,E> — C++23's discriminated-union 'value or error' wrapper. re_std back-ports to … |
| `<functional>` | 36/44 | Per-symbol coverage of the C++ <functional> header. 34 of 44 symbols shipped: the 33-symbol firs… |
| `<initializer_list>` | 3/3 | std::<initializer_list> — the C++11 header holding the compiler-magic initializer_list<E> type p… |
| `<iterator>` | 36/44 | Per-symbol coverage of the C++ <iterator> header. Tags, iterator_traits, stepping (advance/dista… |
| `<limits>` | 3/3 | Arithmetic-type properties: numeric_limits<T> for every fundamental type, plus the rounding and … |
| `<memory>` | 47/51 | Per-symbol coverage of the C++ <memory> header. Smart pointers (unique_ptr, shared_ptr, weak_ptr… |
| `<new>` | 11/11 | std::<new> — the foundational header for dynamic memory and exception types. re_std's role is st… |
| `<numbers>` | 13/13 | Per-symbol coverage of the C++ <numbers> header. SHIPPED 2026-08-25. All 13 constants, each as a… |
| `<numeric>` | 15/19 | Per-symbol coverage of the C++ <numeric> header. Serial folds (accumulate, inner_product), seque… |
| `<optional>` | 6/16 | Per-symbol coverage of the C++ <optional> header (C++17).… |
| `<ranges>` | 65/78 | Per-symbol coverage of the C++ <ranges> header (C++20+). Phases R1-R29 (2026-05-13) shipped 65 o… |
| `<ratio>` | 12/12 | Per-symbol coverage of the C++ <ratio> header (C++11+). SHIPPED 2026-08-25. All 12 catalogued sy… |
| `<source_location>` | 2/2 | std::source_location — C++20's value type capturing a source position (file, function, line, col… |
| `<span>` | 7/7 | std::span<ElementType, Extent> — C++20's non-owning view over a contiguous sequence, back-ported… |
| `<stdexcept>` | 9/9 | std::<stdexcept> — the standard exception-class hierarchy: the two bases logic_error and runtime… |
| `<string_view>` | 13/16 | std::basic_string_view<CharT,Traits> — C++17's non-owning read-only character view. re_std back-… |
| `<system_error>` | 13/13 | std::<system_error> — the C++11 diagnostic-code facility: the error_category abstract base, the … |
| `<tuple>` | 21/22 | Per-symbol coverage of the C++ <tuple> header (C++11).… |
| `<type_traits>` | 113/113 | Per-symbol coverage of the C++ <type_traits> header.… |
| `<typeindex>` | 3/3 | std::type_index — C++11's copyable, assignable wrapper around a std::type_info reference, so a t… |
| `<typeinfo>` | 3/3 | std::<typeinfo> — the RTTI support header (type_info, bad_cast, bad_typeid). re_std's role is pu… |
| `<utility>` | 42/43 | Per-symbol coverage of the C++ <utility> header. Foundational symbols (swap, move, forward, decl… |
| `<variant>` | 13/15 | std::variant<Ts...> — C++17's type-safe discriminated union. re_std back-ports to C++11 (runtime… |
| `<version>` | 1/1 | std::<version> — the C++20 header that aggregates every __cpp_lib_* library feature-test macro a… |

---

## Shipped this cycle

| module | what landed |
|---|---|
| `<cstddef>` | 11 symbols. Two halves: the implementation-fixed typedefs are identity-preserving re-exports; `byte` and its thirteen operations are re_std's own, back-ported C++17 → C++11. `byte{0x2A}` does **not** back-port (P0138R2 is a language change) — cast instead. |
| `<cstdint>` | 32 typedefs, all re-exports. Exact-width and pointer families gated per pair on their own limit macro, the standard's own test for existence. |
| `<climits>` | Macro-only. Fills the C++11 `LLONG_*` set from `__LONG_LONG_MAX__` where the platform header predates it. |
| `<cfloat>` | Macro-only. Fills the C11-via-C++17 `*_DECIMAL_DIG` / `*_TRUE_MIN` / `*_HAS_SUBNORM` families from compiler builtins; absent (not invented) on MSVC. |
| `<cstdlib>` | ~40 symbols, all re-exports — each is a contract with the C runtime. `aligned_alloc` and the `quick_exit` pair are omitted where unavailable rather than redirected. |
| `<chrono>` | Core: 28 of 38 groups. `duration`, `time_point`, three clocks, both casts, `floor`/`ceil`/`round`/`abs`, `is_clock`, literals. Calendar and time zones deferred to priority 39. |
| `<ratio>` | **Repaired.** All 12 files had the core include mangled into a comment; nothing compiled. |

## Known false-greens — re-verified 2026-08-25, all still false

Ten symbols are catalogued with a `secondary_min` but have **no implementation**.
Verified against the filesystem, not the data:

- `<type_traits>` — `is_trivial`, `is_trivially_copyable`, `is_standard_layout`,
  `is_abstract`, `is_aggregate`, `has_virtual_destructor`,
  `has_unique_object_representations`, `is_scoped_enum`. **No file exists for any
  of the eight.**
- `<compare>` — `three_way_comparable`, `three_way_comparable_with`. The
  `compare/compare` umbrella includes two files that do not exist, so
  **`#include "re_std/cpp/compare/compare"` fails to compile.**

These are not merely a cataloguing problem. `optional/optional_base.hpp` uses
`re_std::is_trivially_copyable`, so **`<optional>` does not compile**, and
`<ranges>` fails through it. `bit/bit_cast.hpp` calls `__is_trivially_copyable`
directly to work around the same gap — delete that local detection when the
trait ships.

## Umbrella compile audit — 2026-08-25

21 of 27 module umbrellas compile. Six do not:

| module | cause |
|---|---|
| `<compare>` | umbrella includes two non-existent files (false-green above) |
| `<optional>` | uses the missing `is_trivially_copyable` |
| `<ranges>` | fails through `<optional>` — not an independent defect |
| `<expected>` | `unexpected.hpp:170` uses `std::swap` without including `<utility>` |
| `<string_view>` | mis-ordered namespace macros, below |
| `<tuple>` | mis-ordered namespace macros, below |

### Mis-ordered namespace macros — four files

`NS_` opens and `NS_END` closes are **count-balanced but wrongly sequenced**, so
a plain open/close tally reports them clean. A running-depth check does not:

| file | defect |
|---|---|
| `algorithm/sort.hpp` | stray `NS_END` at line 337 |
| `algorithm/stable_sort.hpp` | stray `NS_END` at line 354 |
| `string_view/char_traits.hpp` | stray `NS_END` at line 392 |
| `tuple/tuple.hpp` | stray `NS_END` at line 118 |

`char_traits.hpp` is the clearest case and shows the full consequence:
`NS_INTERNAL` opens at line 65 with **no enclosing `NS_RESTD`**, so
`char_traits_base` lands in `::internal`; the stray `NS_END` at 392 emits a `}`
at global scope; and the `NS_RESTD` at line 431 is **never closed**, leaving
`re_std` open so every later include nests into `re_std::re_std`. The
`char_traits<char>` and `<wchar_t>` specialisations therefore sit at global
scope, which is exactly what the compiler reports.

The two `algorithm` files are the dangerous ones: `<algorithm>`'s umbrella
compiles anyway, because a sibling header happens to leave a namespace open at
the point they are included. They are correct only by include order.

**Audit command** (order-aware, catches what a tally misses):

```sh
python3 - <<'PY'
import glob, re, os
OPEN  = re.compile(r'^\s*(NS_RESTD|NS_INTERNAL|NS_RANGES|NS_CONCEPTS|NS_DJINTERP|D_NAMESPACE)\b')
CLOSE = re.compile(r'^\s*NS_END\b')
for f in sorted(glob.glob('re_std/**/*', recursive=True)):
    if not os.path.isfile(f): continue
    depth = 0
    for n, line in enumerate(open(f, errors='ignore'), 1):
        if OPEN.match(line):  depth += 1
        elif CLOSE.match(line):
            depth -= 1
            if depth < 0: print(f'{f}:{n} stray NS_END'); break
    else:
        if depth: print(f'{f} ends with depth {depth:+d}')
PY
```

## Caveat on this audit

All compilation was done against a **stand-in** `core/djinterp.hpp`, since the
corpus ships without the real one. The stub supplies the macro vocabulary the
headers reference, derived from `RE_STD_AGENT_README.md`. The four namespace
defects above are confirmed by static analysis and do not depend on the stub;
the `<expected>` and `<optional>` failures are ordinary missing-declaration
errors that do not either. Re-run against the real core header before acting.

