# re_std — Agent Implementation Guide

> **Version:** 0.1.0
> **Last updated:** 2026-04-14
> **Status:** Active development — `type_traits` and `any` modules complete

---

## What Is re_std?

re_std is a portable, header-only reimplementation of the C++ standard library
(`std`). It exists so that code can be written against a single interface —
`re_std::` — and compile correctly on any C++ compiler from C++98 through
C++26, degrading gracefully when language features are unavailable.

re_std is **not** a polyfill or a shim. It does not wrap `std`. Every symbol
is a self-contained implementation that uses zero standard library headers
for its core functionality. Where the standard library provides something
equivalent, re_std will match the interface and semantics exactly.

### Goals (in priority order)

1. **Portability.** Code using re_std compiles on C++98 through C++26 and on
   GCC, Clang, MSVC, and Intel compilers. No feature may cause a hard
   compilation failure on any supported tier — unsupported functionality
   is omitted or degraded, never errored.

2. **Backwards compatibility.** Features introduced in later standards are
   available on earlier standards whenever the language permits. For
   example, `re_std::any` provides SBO storage on C++98, even though
   `std::any` requires C++17.

3. **Constexpr maximization.** Anything that can be constexpr, is. If a
   function is constexpr on C++14+ but not C++11, it has both paths. The
   `D_CONSTEXPR` macro handles the qualification automatically.

4. **Granularity.** Every public symbol lives in its own header file
   ("module"). Users include only what they need. Umbrella headers
   (matching `std`'s header names) are provided for convenience but are
   never required.

---

## Architecture

### Namespace Layout

re_std mirrors `std`'s namespace structure exactly:

```
re_std::                          ← flat (like std::)
re_std::chrono::                  ← mirrors std::chrono::
re_std::filesystem::              ← mirrors std::filesystem::
re_std::ranges::                  ← mirrors std::ranges::
re_std::views::                   ← mirrors std::views::
re_std::execution::               ← mirrors std::execution::
re_std::pmr::                     ← mirrors std::pmr::
re_std::literals::                ← mirrors std::literals::
re_std::numbers::                 ← mirrors std::numbers::
re_std::this_thread::             ← mirrors std::this_thread::
re_std::internal::                ← implementation details (no std equivalent)
```

**Everything that lives in `std::` lives in `re_std::` directly.** There
is no `re_std::any::` sub-namespace. `any`, `any_cast`, `bad_any_cast`,
and `make_any` all live directly in `re_std::`.

### File Layout

```
re_std/
├── config.h                         ← configuration, read by both languages
├── config.hpp                       ← C++ configuration (includes config.h)
├── c/                               ← the C tree: .h and .c files only
│   └── cstdint/
│       └── dstdint.h                ←   <stdint.h> + <inttypes.h> formats
└── cpp/                             ← the C++ tree: everything below

re_std/cpp/
├── any                              ← umbrella header (#include "any/...")
├── type_traits                      ← umbrella header
├── utility                          ← umbrella header
├── ...
│
├── any/                             ← granular modules
│   ├── any.hpp                      ←   class any
│   ├── any_cast.hpp                 ←   any_cast overloads
│   ├── bad_any_cast.hpp             ←   bad_any_cast exception
│   ├── make_any.hpp                 ←   make_any factory
│   └── any_swap.hpp                 ←   swap(any&, any&) specialization
│
├── type_traits/                     ← granular modules
│   ├── type_traits.hpp              ←   umbrella; ~110 granular headers beside it
│   └── ...
│
└── internal/                        ← shared implementation details
    └── ...
```

**Umbrella naming is not uniform, and that is fine.** Three forms are in use:
extensionless (`ratio/ratio`, `span/span`, `bit/bit`), `.hpp`
(`type_traits/type_traits.hpp`, `memory/memory.hpp`), and `_umbrella`
(`array/array_umbrella`, `new/new_umbrella`). Match whatever the module you are
touching already does. Note that the extensionless form is invisible to an
`*.hpp` glob — a rename pass once skipped all 17 of them for exactly that
reason, so walk by content, not by extension.

**Granularity has one documented exception.** `numbers/numbers.hpp` holds all
thirteen constants rather than thirteen two-line files. Granularity exists to
narrow compile-time dependencies, and a table of literals has nothing to
narrow. If you break the rule, say why in the umbrella.

**The granularity rule:** every public symbol gets its own `.hpp` file.
This includes free functions, classes, type traits, constants, and
specializations. A user who only needs `re_std::is_integral` should be
able to include a single small header, not all of `<type_traits>`.

**Umbrella headers** have no extension (matching `#include <any>` style)
and simply `#include` all the granular modules for that standard header.

### Dependency Rules

1. **No circular includes.** The dependency graph must be a DAG.
2. **No `<type_traits>` or other standard trait headers.** Use
   `re_std/cpp/type_traits/type_traits.hpp` instead.
3. **Standard headers for fundamental types only:** `<cstddef>` (for
   `size_t`, `nullptr_t`) and
   `<new>` / `<initializer_list>` when gated on availability macros.
   Fixed-width integers come from re_std itself, at every level:
   `re_std/cpp/cstdint/cstdint.hpp`, never `<cstdint>` (C++11) or
   `<stdint.h>` -- only `re_std/c/cstdint/dstdint.h` includes those.
4. **Granular modules may include other granular modules.** For example,
   `any_cast.hpp` includes `any.hpp` and `bad_any_cast.hpp`.
5. **Always include the core header:** every re_std module includes
   `djinterp.hpp` (or the re_std core equivalent when extracted) for
   macro infrastructure.

---

## Environment Detection

re_std uses the djinterp env detection system. Key macros you will use:

### Language Standard Tiers

| Macro | True when |
|-------|-----------|
| `D_ENV_LANG_IS_CPP11_OR_HIGHER` | C++11 or later |
| `D_ENV_LANG_IS_CPP14_OR_HIGHER` | C++14 or later |
| `D_ENV_LANG_IS_CPP17_OR_HIGHER` | C++17 or later |
| `D_ENV_LANG_IS_CPP20_OR_HIGHER` | C++20 or later |
| `D_ENV_LANG_IS_CPP23_OR_HIGHER` | C++23 or later |

### Feature-Level Detection

| Macro | Feature |
|-------|---------|
| `D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES` | Move semantics |
| `D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES` | `typename...` packs |
| `D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES` | `using X = ...` aliases |
| `D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES` | `template<> T v = ...` |
| `D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR` | `if constexpr (...)` |
| `D_ENV_CPP_FEATURE_LANG_CONSTEXPR` | `constexpr` keyword at all |
| `D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS` | `(args + ...)` |
| `D_ENV_CPP_FEATURE_LANG_CONCEPTS` | `concept` / `requires` |

### Header Availability

| Macro | Header |
|-------|--------|
| `D_ENV_CPP98_HAS_NEW` | `<new>` |
| `D_ENV_CPP98_HAS_UTILITY` | `<utility>` |
| `D_ENV_CPP98_HAS_TYPEINFO` | `<typeinfo>` |
| `D_ENV_CPP98_HAS_EXCEPTION` | `<exception>` |
| `D_ENV_CPP98_HAS_STDEXCEPT` | `<stdexcept>` |

### Compiler Detection

| Macro | Compiler |
|-------|----------|
| `D_ENV_COMPILER_GCC` | GCC |
| `D_ENV_COMPILER_CLANG` | Clang/LLVM |
| `D_ENV_COMPILER_MSVC` | Microsoft Visual C++ |
| `D_ENV_COMPILER_INTEL` | Intel C++ |
| `D_ENV_COMPILER_VERSION_AT_LEAST(maj,min,patch)` | Version check |

### Portable Qualifier Macros

| Macro | Expands to | Fallback |
|-------|-----------|----------|
| `D_CONSTEXPR` | `constexpr` | empty (C++98/03) |
| `D_STATIC` | `static` | stripped under `D_TESTING` |
| `D_INLINE` | forced inline | `inline` on unknown compilers |
| `D_CONSTEXPR_INLINE` | `constexpr inline` | `inline` on C++98/03 |
| `D_STATIC_CONSTEXPR` | `static constexpr` | `static` on C++98/03 |

---

## Coding Conventions

### Style Guide Summary

re_std follows the djinterp C++ style guide. Key points for
implementation:

**Naming:**
- Types, traits, functions: `snake_case` (e.g., `is_integral`, `any_cast`)
- Internal helpers: suffix `_helper` or `_base`, in `internal::` namespace
- Type aliases: suffix `_t` (e.g., `enable_if_t`)
- Value aliases: suffix `_v` (e.g., `is_same_v`)
- Detection macros: `D_RE_STD_HAS_*`
- Template parameters: `_PascalCase` with leading underscore (e.g., `_Type`, `_Value`)
- Private members: `m_` prefix

**Formatting:**
- Spaces, not tabs
- `{` on its own line (Allman style)
- Multi-condition booleans: each condition on its own line, enclosed in outer parens
- Template parameters: one per line, aligned on `_` character
- Always `typename`, never `class`, for type parameters
- Void functions: explicit `return;` at end
- Comments: `//` style, lowercase first letter

**Comments (brief format):**
```cpp
// symbol_name
//   category: brief description.
```

Where category is one of: `trait`, `type`, `typedef`, `function`,
`class`, `struct`, `enum`, `constant`, `variable`, `alias`.

### Trait Implementation Pattern

Every trait follows this structure:

```cpp
// =============================================================================
// SECTION HEADER
// =============================================================================

NS_INTERNAL

    // foo_base
    //   trait: internal helper for foo (primary template).
    template<typename _Type>
    struct foo_base : false_type
    {};

    // foo_base<specialized>
    //   trait: specialization for [description].
    template<>
    struct foo_base<int> : true_type
    {};

NS_END  // internal

// foo
//   trait: true if _Type is [description].
template<typename _Type>
struct foo
    : internal::foo_base<typename remove_cv<_Type>::type>
{};

// foo_v (C++14+)
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool foo_v = foo<_Type>::value;
#endif
```

**Key points:**
- Primary classification logic goes in `internal::foo_base`.
- The public `foo` strips cv-qualifiers (when appropriate for the trait)
  and delegates.
- `_v` aliases are gated on `D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES`.
- `_t` aliases are gated on `D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES`.

### Tiered Implementation Pattern

When functionality varies by standard tier, use this structure:

```cpp
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // best path: uses C++20 features
    // ...
#elif D_ENV_LANG_IS_CPP14_OR_HIGHER
    // good path: constexpr-capable
    // ...
#elif D_ENV_LANG_IS_CPP11_OR_HIGHER
    // adequate path: uses C++11 features
    // ...
#else
    // C++98/03 fallback
    // ...
#endif
```

**Rules for tiering:**
- The most capable tier goes first (top).
- Each tier must be self-contained — no fallthrough.
- If a function signature changes between tiers (e.g., gains `noexcept`),
  all variants must have the same name and be in the same namespace.
- Never `#error` on a missing feature. Degrade or omit.

### Compiler Intrinsic Pattern

For traits that need compiler magic:

```cpp
// D_RE_STD_HAS_FOO
//   constant: 1 if __foo compiler intrinsic is available.
#ifndef D_RE_STD_HAS_FOO
    #if defined(__foo)
        #define D_RE_STD_HAS_FOO  1
    #elif ( defined(D_ENV_COMPILER_GCC) &&                    \
            D_ENV_COMPILER_VERSION_AT_LEAST(x, y, z) )
        #define D_RE_STD_HAS_FOO  1
    #elif defined(D_ENV_COMPILER_CLANG)
        #define D_RE_STD_HAS_FOO  1
    #elif ( defined(D_ENV_COMPILER_MSVC) &&                   \
            D_ENV_COMPILER_VERSION_AT_LEAST(x, y, z) )
        #define D_RE_STD_HAS_FOO  1
    #else
        #define D_RE_STD_HAS_FOO  0
    #endif
#endif

#if D_RE_STD_HAS_FOO
    // real implementation using __foo
#else
    // safe fallback (false_type, omission, or degraded behavior)
#endif
```

**Rules:**
- Detection macro is `D_RE_STD_HAS_*`, always `#ifndef`-guarded so
  users can override.
- Test for the builtin name directly first (`defined(__foo)`), then
  fall back to compiler+version checks.
- The fallback must never cause a compilation error.

---

### Marking re-exports

Some symbols **cannot be reimplemented as portable re_std code** — they are
provided by the language runtime, the C++ ABI, or the compiler itself, and
re_std surfaces them in-namespace via an identity-preserving re-export
(`using std::X;` or a typedef alias) so that `re_std::X` **is** `std::X`.
Examples: `operator new` / placement `new`, `type_info`, `bad_cast` /
`bad_typeid`, the `std::exception` and `std::logic_error` / `runtime_error`
hierarchies, and the `exception_ptr` / nested-exception families. Re-export
is the *correct* design here, not a shortcut: a distinct `re_std::bad_cast`
would not be caught by a language-emitted `throw`, and `typeid` could not
bind to it.

When a symbol is such a re-export, set **`"reexport": true`** in its
coverage JSON (Phase 2). This is what feeds the workbook's **Re-exports**
sheet.

Keep three cases distinct:

- **`reexport: true`** — re_std contributes only the in-namespace alias; the
  entity is runtime/ABI/compiler/language-provided. `re_std::X` is `std::X`
  (type identity preserved).
- **`intrinsic_required: true`** — re_std *owns* the symbol
  (`re_std::is_class` is its own trait), but its implementation needs a
  compiler builtin. Set `reexport: false`.
- **A back-port** (`secondary_min < std_in`) — re_std provides its own
  implementation on tiers where std lacks the symbol (e.g. `get_terminate`,
  `bad_array_new_length`, `uncaught_exceptions`). Set `reexport: false`,
  even if the symbol is a thin `using`-declaration on its *upper* tiers;
  the back-port is a genuine re_std implementation.

A `reexport` symbol's `secondary_min` equals its `primary_in` (re_std adds nothing
but the namespace alias), and its `notes` should say what it re-exports and
why it cannot be reimplemented.

---

## How to Implement a New Module

### Step-by-step

**1. Identify the std symbol(s).**

Determine exactly which symbols from the standard header you're
implementing. Example: `<optional>` has `optional<T>`, `nullopt_t`,
`nullopt`, `bad_optional_access`, `make_optional`, and a `swap`
specialization.

**2. Create one file per symbol.**

```
re_std/cpp/optional/
├── optional.hpp              ← class optional<T>
├── nullopt.hpp               ← nullopt_t + nullopt constant
├── bad_optional_access.hpp   ← exception class
├── make_optional.hpp         ← factory function
└── optional_swap.hpp         ← swap(optional&, optional&)
```

**3. Determine minimum viable standard tier.**

For each symbol, identify the lowest C++ standard at which a
meaningful implementation is possible. Document this in the file
header. Common blockers:

| Feature needed | Minimum |
|----------------|---------|
| Variadic templates | C++11 |
| Rvalue references / move semantics | C++11 |
| `constexpr` (basic) | C++11 |
| `constexpr` (relaxed — locals, loops) | C++14 |
| `if constexpr` | C++17 |
| Concepts / requires | C++20 |
| `constexpr` dynamic allocation | C++20 |
| Deducing `this` | C++23 |

**4. Write the file header.**

Every file starts with the standard djinterp block comment:

```cpp
/***********************************************************************
* re_std                                                  symbol_name.hpp
*
* brief description header:
*   Extended description of what this module provides, any tiered
* behavior, and portability notes.
*
*
* path:      /inc/re_std/cpp/header_group/symbol_name.hpp
* link(s):   TBA
* author(s): Your Name                                  date: YYYY.MM.DD
***********************************************************************/

#ifndef DJINTERP_RESTD_HEADER_GROUP_SYMBOL_NAME_
#define DJINTERP_RESTD_HEADER_GROUP_SYMBOL_NAME_ 1
```

**5. Implement with tiered `#if` guards.**

Start from the highest tier and work down. Ensure each tier compiles
independently.

**6. Write the umbrella header.**

```
re_std/optional   ← no extension
```

This file includes all granular modules for that standard header.

**7. Update documentation.**

Add entries to:
- `re_std_compatibility.md` — the per-symbol version/compiler table
- `re_std_modules.md` — the detailed module documentation
- This file's "Current Status" section below

> The coverage JSON + workbook regeneration (the data behind
> `re_std_cpp_coverage.xlsx`) is a separate, canonical workflow — see
> `MODULE_WORKFLOW.md`, phases 2–3. The workbook is rebuilt with
> `python3 scripts/re_std/re_std.py build`; re_std's config lives in
> `re_std.py` (`RE_STD_CONFIG`), and data sheets are discovered by
> globbing `docs/re_std/cpp/data/*.json`.

### Checklist for Every New Symbol

- [ ] Lives in `re_std::` (or appropriate sub-namespace like `re_std::chrono::`)
- [ ] Has its own `.hpp` file in the correct subdirectory
- [ ] Include guard follows `RE_STD_GROUP_SYMBOLNAME_` pattern
- [ ] Includes the re_std core header (for macros)
- [ ] Uses `re_std::` type traits, not `std::` type traits
- [ ] Uses `D_CONSTEXPR`, `D_STATIC`, `D_INLINE` macros (not raw keywords)
- [ ] `reexport: true` set if the symbol is a runtime/ABI/compiler/language re-export (see "Marking re-exports")
- [ ] Uses `typedef` (not `using`) when C++98 support is needed
- [ ] Uses `static const` (not `static constexpr`) for C++98 trait values
- [ ] Move semantics gated on `D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES`
- [ ] Variadic templates gated on `D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES`
- [ ] `_t` aliases gated on `D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES`
- [ ] `_v` variables gated on `D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES`
- [ ] `noexcept` gated or uses a `D_NOEXCEPT` macro
- [ ] `enum class` replaced with struct-wrapper for C++98/03
- [ ] `nullptr` avoided or replaced with `0` / `NULL` for C++98/03
- [ ] `explicit operator bool()` uses safe-bool idiom for C++98/03
- [ ] No `#error` on missing features — always degrade or omit
- [ ] Umbrella header updated to include the new file
- [ ] Compatibility table entry added
- [ ] Module documentation entry added

---

## Current Status

### Implemented

| Module | Symbols | Min Standard |
|--------|---------|-------------|
| `type_traits` | `integral_constant`, `true_type`, `false_type`, `enable_if`, `remove_const`, `remove_volatile`, `remove_cv`, `remove_pointer`, `is_same`, `is_const`, `is_integral`, `is_floating_point`, `is_pointer`, `is_function`, `is_enum`, `is_signed`, `is_unsigned`, `underlying_type` | C++98 |
| `any` | `any`, `any_type_id`, `any_type_id_of`, `DAnyCategory` | C++11 (C++98 planned) |
| `any` | `any_cast` (5 overloads) | C++11 (C++98 planned) |
| `any` | `swap(any&, any&)` | C++11 (C++98 planned) |
| `any` | `bad_any_cast` | C++98 |
| `any` | `make_any` (2 overloads) | C++11 |
| `functional` | `plus`, `minus`, `multiplies`, `divides`, `modulus`, `negate` (arithmetic), `equal_to`, `not_equal_to`, `greater`, `less`, `greater_equal`, `less_equal` (compare), `logical_and`, `logical_or`, `logical_not` | C++98 |
| `functional` | `bit_and`, `bit_or`, `bit_xor`, `bit_not` | C++11 (`bit_not` back-ported from C++14) |
| `functional` | `reference_wrapper`, `ref`, `cref`, `unwrap_reference`, `unwrap_ref_decay` | C++11 (back-ported from C++20 for the latter two) |
| `functional` | `invoke`, `invoke_r`, `mem_fn`, `not_fn`, `identity` | C++11 (back-ported from C++17/20/23) |
| `functional` | `bad_function_call`, `is_bind_expression`, `is_placeholder`, `hash` (primary + scalar specs) | C++98 / C++11 |

### Not Yet Implemented (partial list of high-value targets)

| std Header | Key Symbols | Estimated Min |
|------------|------------|---------------|
| `<utility>` | `swap`, `move`, `forward`, `pair`, `declval` | C++98 (swap, pair), C++11 (move, forward, declval) |
| `<optional>` | `optional<T>`, `nullopt`, `bad_optional_access`, `make_optional` | C++11 |
| `<variant>` | `variant<Ts...>`, `visit`, `get`, `holds_alternative`, `bad_variant_access` | C++11 |
| `<tuple>` | `tuple<Ts...>`, `get`, `make_tuple`, `tie`, `tuple_size`, `tuple_element` | C++11 |
| `<functional>` | `function<Sig>`, `move_only_function`, `copyable_function`, `function_ref`, `bind`, `bind_front`, `bind_back`, `placeholders::_N`, `default_searcher`, `boyer_moore_searcher`, `boyer_moore_horspool_searcher` | C++11 (function family); `bind` family blocked on `<tuple>`; searchers blocked on `<algorithm>` |
| `<memory>` | `unique_ptr`, `shared_ptr`, `weak_ptr`, `make_unique`, `make_shared` | C++11 |
| `<string_view>` | `basic_string_view<CharT>` | C++11 |
| `<span>` | `span<T>` | C++11 |
| `<expected>` | `expected<T,E>`, `unexpected<E>` | C++11 |
| `<algorithm>` | `find`, `sort`, `transform`, `for_each`, ... | C++98 |
| `<array>` | `array<T,N>` | C++98 |
| `<type_traits>` | Remaining ~100 traits | C++98 (most) |

### Known Issues

| File | Issue | Severity |
|------|-------|----------|
| `any_cast.hpp:95` | Typo: `D_ENV_CPP98_HAS_TypeYPEINFO` should be `D_ENV_CPP98_HAS_TYPEINFO` | Bug — checked overloads don't compile on typeinfo-only systems |
| `make_any.hpp:57` | `_Type::initializer_list<_U>` should be `std::initializer_list<_U>` | Bug — initializer_list overload doesn't compile |
| `any.hpp` | Entire file gated on C++11; needs C++98 path | Planned work |
| `any.hpp` | `swap()` member uses copy-and-swap; suboptimal for heap | Optimization |
| `any.hpp` | `emplace<T>()` not yet implemented | Missing feature |
| `any.hpp` | `get_ref<T>()` not yet implemented | Missing feature |

---

## Frequently Asked Questions

### Why not just use Boost?

Boost's implementations are excellent but come with significant
transitive dependencies. `boost::any` alone pulls in multiple Boost
headers. re_std has zero external dependencies beyond a C++98 compiler
and the re_std core header.

### Why not wrap `std` when available?

Because behavioral differences between vendor implementations would
leak through. `std::is_trivially_copyable` has had bugs on specific
MSVC versions. `std::any`'s SBO threshold varies between libstdc++,
libc++, and MSVC's STL. re_std provides one implementation with one
behavior, tested everywhere.

### Why `typedef` instead of `using` in traits?

`using` aliases require C++11 (`__cpp_alias_templates`). Since the
core traits must work on C++98, they use `typedef`. The `_t`
convenience aliases (which do use `using`) are gated on the alias
templates feature macro.

### Why are `_v` variables gated on C++14, not C++11?

Variable templates (`template<typename T> constexpr bool v = ...`)
are a C++14 feature (`__cpp_variable_templates`). C++11 has no
equivalent — you must use `::value`.

### Can I mix `re_std::` and `std::` types?

Yes, but they are distinct types. `re_std::any` is not `std::any`.
You cannot pass a `re_std::any` to a function expecting `std::any`.
Within a project, pick one and use it consistently.

---

## Hard-won rules (read before writing a header)

Eight bugs have been found in headers the coverage workbook called *shipped*.
Every one parsed cleanly. Every one was found by the first code that actually
**instantiated** it. These are the specific traps.

### 1. The namespace is top-level `re_std`, via `NS_RESTD` alone

Do **not** wrap it in `NS_DJINTERP`. `NS_RESTD` expands to `namespace re_std {`
on its own; nesting it inside `NS_DJINTERP` produces `djinterp::re_std`, a
*different namespace* from the one every other header lives in. Ninety-five
files were written that way and none of them could see `re_std::invoke`.

The macro name `NS_RESTD` is frozen and keeps the old spelling. Everything
else — guards, `D_RE_STD_*` macros, `path:` fields — uses `RE_STD` / `re_std`.

### 2. In C++11, `constexpr` on a member function implies `const`

This has caused three separate bugs. A `constexpr` non-const accessor and its
`const` twin collapse into two functions differing only in return type; a
`constexpr` `&`-qualified `operator()` collides with the `const &` one. C++14
dropped the implicit const.

**Rule:** the non-const half of any accessor or call-operator pair gets
`D_CONSTEXPR_CPP14`, never `D_CONSTEXPR`.

### 3. `enable_if` must depend on the *member's* own template parameters

```cpp
// WRONG -- sizeof...(_Tail) is fixed once the class is instantiated, so
// enable_if<false, ...> is a hard error instead of removing the overload
template<typename _U1, typename _U2>
typename enable_if<sizeof...(_Tail) == 1, tuple&>::type operator=(...);

// RIGHT -- _N is a parameter of this member, so the condition is dependent
template<typename _U1, typename _U2, std::size_t _N = sizeof...(_Tail)>
typename enable_if<_N == 1, tuple&>::type operator=(...);
```

The wrong form made every `tuple<A,B>` ill-formed on instantiation, at every
tier.

### 4. Declaring a move operation deletes the copy constructor

`tuple` declared `operator=(tuple&&)` and nothing else. That *deletes* the
implicit copy constructor — no `re_std::tuple` was copyable. If you declare any
move operation, explicitly `= default` the copy operations you still want.

### 5. Both arms of a ternary are instantiated in a template

A self-referential ternary recurses forever at compile time regardless of which
branch the value selects. Drive compile-time recursion through a
**bool-dispatched helper** with two partial specialisations. See
`ratio/ratio_less.hpp`.

### 6. Narrow types promote to `int` before shifting

`x << r` on a `uint8_t` promotes, so bits shifted off the top survive in the
promoted value. Cast every sub-expression back to `_T`. Without it,
`rotl(uint8_t(0x80), 1)` gives `0x101` instead of `0x01`.

### 7. A variadic overload can hijack copy construction

`template<typename _G, typename... _B> ctor(_G&&, _B&&...)` out-matches the
implicit copy constructor for a non-const lvalue of the class's own type. Add a
leading tag parameter. See `functional/bind_front.hpp`.

### 8. Check an access specifier actually reopens

`shared_ptr`'s primary template opened `private:` for its construction section
and never returned to `public:`. Every constructor, the destructor, `get()`,
`reset()` and `use_count()` were inaccessible. The class parsed.

### 9. `void_t` is an alias template — it has no `::type`

`typename void_t<...>::type` is ill-formed. Write `void_t<...>`.

---

## Include and layout rules

- **Core header:** `#include "../../core/djinterp.hpp"` from a module
  directory. Three other spellings exist in the corpus and all three are wrong:
  `"djinterp.hpp"` needs an extra `-I`, and `"../djinterp.hpp"` and
  `"../../djinterp.hpp"` both point at paths that do not exist.
- **Sibling modules:** file-relative (`"../tuple/tuple.hpp"`), not root-relative.
- **Invert dependencies rather than adding heavy ones.** `span` must not
  include `<ranges>`; the ranges opt-in lives in its own
  `span/span_range_opt_in.hpp` so only callers that want it pay.

## The tier floor

`core/djinterp.hpp` **hard-errors below C++11**, and its own comment forbids
carrying C++98 fallbacks: *"an unreachable branch is a second implementation
nobody compiles."*

The workbook still floors at C++98 and 240 symbols still claim a C++98
minimum. Until `versions[0]` in `_config.json` changes, a new C++11-floor
symbol renders as *uncovered* and needs a `failure_reason` saying so. Do not
invent C++98 fallbacks to make the number look better.

## Every symbol needs a test that instantiates it

Not a syntax check — an instantiation. A `static_assert` on the deduced type
counts; `-fsyntax-only` on the header does not. All eight bugs above survived
because nothing instantiated the code.
