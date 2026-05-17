# restd — Agent Implementation Guide

> **Version:** 0.1.0
> **Last updated:** 2026-04-14
> **Status:** Active development — `type_traits` and `any` modules complete

---

## What Is restd?

restd is a portable, header-only reimplementation of the C++ standard library
(`std`). It exists so that code can be written against a single interface —
`restd::` — and compile correctly on any C++ compiler from C++98 through
C++26, degrading gracefully when language features are unavailable.

restd is **not** a polyfill or a shim. It does not wrap `std`. Every symbol
is a self-contained implementation that uses zero standard library headers
for its core functionality. Where the standard library provides something
equivalent, restd will match the interface and semantics exactly.

### Goals (in priority order)

1. **Portability.** Code using restd compiles on C++98 through C++26 and on
   GCC, Clang, MSVC, and Intel compilers. No feature may cause a hard
   compilation failure on any supported tier — unsupported functionality
   is omitted or degraded, never errored.

2. **Backwards compatibility.** Features introduced in later standards are
   available on earlier standards whenever the language permits. For
   example, `restd::any` provides SBO storage on C++98, even though
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

restd mirrors `std`'s namespace structure exactly:

```
restd::                          ← flat (like std::)
restd::chrono::                  ← mirrors std::chrono::
restd::filesystem::              ← mirrors std::filesystem::
restd::ranges::                  ← mirrors std::ranges::
restd::views::                   ← mirrors std::views::
restd::execution::               ← mirrors std::execution::
restd::pmr::                     ← mirrors std::pmr::
restd::literals::                ← mirrors std::literals::
restd::numbers::                 ← mirrors std::numbers::
restd::this_thread::             ← mirrors std::this_thread::
restd::internal::                ← implementation details (no std equivalent)
```

**Everything that lives in `std::` lives in `restd::` directly.** There
is no `restd::any::` sub-namespace. `any`, `any_cast`, `bad_any_cast`,
and `make_any` all live directly in `restd::`.

### File Layout

```
restd/
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
│   └── type_traits.hpp              ←   all traits (will be split later)
│
└── internal/                        ← shared implementation details
    └── ...
```

**The granularity rule:** every public symbol gets its own `.hpp` file.
This includes free functions, classes, type traits, constants, and
specializations. A user who only needs `restd::is_integral` should be
able to include a single small header, not all of `<type_traits>`.

**Umbrella headers** have no extension (matching `#include <any>` style)
and simply `#include` all the granular modules for that standard header.

### Dependency Rules

1. **No circular includes.** The dependency graph must be a DAG.
2. **No `<type_traits>` or other standard trait headers.** Use
   `restd/type_traits/type_traits.hpp` instead.
3. **Standard headers for fundamental types only:** `<cstddef>` (for
   `size_t`, `nullptr_t`), `<cstdint>` (for fixed-width types), and
   `<new>` / `<initializer_list>` when gated on availability macros.
4. **Granular modules may include other granular modules.** For example,
   `any_cast.hpp` includes `any.hpp` and `bad_any_cast.hpp`.
5. **Always include the core header:** every restd module includes
   `djinterp.hpp` (or the restd core equivalent when extracted) for
   macro infrastructure.

---

## Environment Detection

restd uses the djinterp env detection system. Key macros you will use:

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

restd follows the djinterp C++ style guide. Key points for
implementation:

**Naming:**
- Types, traits, functions: `snake_case` (e.g., `is_integral`, `any_cast`)
- Internal helpers: suffix `_helper` or `_base`, in `internal::` namespace
- Type aliases: suffix `_t` (e.g., `enable_if_t`)
- Value aliases: suffix `_v` (e.g., `is_same_v`)
- Detection macros: `D_RESTD_HAS_*`
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
// D_RESTD_HAS_FOO
//   constant: 1 if __foo compiler intrinsic is available.
#ifndef D_RESTD_HAS_FOO
    #if defined(__foo)
        #define D_RESTD_HAS_FOO  1
    #elif ( defined(D_ENV_COMPILER_GCC) &&                    \
            D_ENV_COMPILER_VERSION_AT_LEAST(x, y, z) )
        #define D_RESTD_HAS_FOO  1
    #elif defined(D_ENV_COMPILER_CLANG)
        #define D_RESTD_HAS_FOO  1
    #elif ( defined(D_ENV_COMPILER_MSVC) &&                   \
            D_ENV_COMPILER_VERSION_AT_LEAST(x, y, z) )
        #define D_RESTD_HAS_FOO  1
    #else
        #define D_RESTD_HAS_FOO  0
    #endif
#endif

#if D_RESTD_HAS_FOO
    // real implementation using __foo
#else
    // safe fallback (false_type, omission, or degraded behavior)
#endif
```

**Rules:**
- Detection macro is `D_RESTD_HAS_*`, always `#ifndef`-guarded so
  users can override.
- Test for the builtin name directly first (`defined(__foo)`), then
  fall back to compiler+version checks.
- The fallback must never cause a compilation error.

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
restd/optional/
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
* restd                                                  symbol_name.hpp
*
* brief description header:
*   Extended description of what this module provides, any tiered
* behavior, and portability notes.
*
*
* path:      /inc/restd/header_group/symbol_name.hpp
* link(s):   TBA
* author(s): Your Name                                  date: YYYY.MM.DD
***********************************************************************/

#ifndef RESTD_HEADER_GROUP_SYMBOL_NAME_
#define RESTD_HEADER_GROUP_SYMBOL_NAME_ 1
```

**5. Implement with tiered `#if` guards.**

Start from the highest tier and work down. Ensure each tier compiles
independently.

**6. Write the umbrella header.**

```
restd/optional   ← no extension
```

This file includes all granular modules for that standard header.

**7. Update documentation.**

Add entries to:
- `restd_compatibility.md` — the per-symbol version/compiler table
- `restd_modules.md` — the detailed module documentation
- This file's "Current Status" section below

### Checklist for Every New Symbol

- [ ] Lives in `restd::` (or appropriate sub-namespace like `restd::chrono::`)
- [ ] Has its own `.hpp` file in the correct subdirectory
- [ ] Include guard follows `RESTD_GROUP_SYMBOLNAME_` pattern
- [ ] Includes the restd core header (for macros)
- [ ] Uses `restd::` type traits, not `std::` type traits
- [ ] Uses `D_CONSTEXPR`, `D_STATIC`, `D_INLINE` macros (not raw keywords)
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
headers. restd has zero external dependencies beyond a C++98 compiler
and the restd core header.

### Why not wrap `std` when available?

Because behavioral differences between vendor implementations would
leak through. `std::is_trivially_copyable` has had bugs on specific
MSVC versions. `std::any`'s SBO threshold varies between libstdc++,
libc++, and MSVC's STL. restd provides one implementation with one
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

### Can I mix `restd::` and `std::` types?

Yes, but they are distinct types. `restd::any` is not `std::any`.
You cannot pass a `restd::any` to a function expecting `std::any`.
Within a project, pick one and use it consistently.
