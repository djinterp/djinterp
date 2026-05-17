# restd — Compatibility Reference

> **Last updated:** 2026-05-07
> **Covers:** `type_traits.hpp`, `any.hpp`, `any_cast.hpp`, `any_swap.hpp`,
> `bad_any_cast.hpp`, `make_any.hpp`, `utility/swap.hpp`,
> `utility/move.hpp`, `utility/forward.hpp`, `utility/declval.hpp`,
> `utility/pair.hpp`, `utility/make_pair.hpp`,
> `functional/` (33 symbols — see §7)

---

## How to Read This Document

Each table below lists every public symbol exposed by a restd module. Columns
describe behavior at each C++ standard tier. The **Compiler Intrinsic** column
marks traits that require a compiler builtin — all other traits are pure
template metaprogramming and work on any conforming compiler.

### Legend

| Symbol | Meaning |
|--------|---------|
| ✓ | Fully functional |
| constexpr | Available and constexpr-qualified |
| ✗ | Not available / omitted at this tier |
| fallback | Available with reduced functionality (see notes) |
| intrinsic | Requires compiler builtin (see Compiler Intrinsic column) |
| gated | Conditionally compiled; availability depends on env detection |

---

## 1  `type_traits.hpp` — Trait Availability

### 1.1  Foundation Types

| Symbol | Kind | C++98/03 | C++11 | C++14+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------------------|
| `integral_constant<T,V>` | trait | ✓ (static const) | ✓ (static constexpr) | ✓ | No |
| `true_type` | typedef | ✓ | ✓ | ✓ | No |
| `false_type` | typedef | ✓ | ✓ | ✓ | No |

### 1.2  SFINAE Support

| Symbol | Kind | C++98/03 | C++11 | C++14+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------------------|
| `enable_if<B,T>` | trait | ✓ | ✓ | ✓ | No |
| `enable_if_t<B,T>` | alias | ✗ | ✓ | ✓ | No |

### 1.3  Type Modification

| Symbol | Kind | C++98/03 | C++11 | C++14+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------------------|
| `remove_const<T>` | trait | ✓ | ✓ | ✓ | No |
| `remove_const_t<T>` | alias | ✗ | ✓ | ✓ | No |
| `remove_volatile<T>` | trait | ✓ | ✓ | ✓ | No |
| `remove_volatile_t<T>` | alias | ✗ | ✓ | ✓ | No |
| `remove_cv<T>` | trait | ✓ | ✓ | ✓ | No |
| `remove_cv_t<T>` | alias | ✗ | ✓ | ✓ | No |
| `remove_pointer<T>` | trait | ✓ | ✓ | ✓ | No |
| `remove_pointer_t<T>` | alias | ✗ | ✓ | ✓ | No |

### 1.4  Type Comparison

| Symbol | Kind | C++98/03 | C++11 | C++14+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------------------|
| `is_same<A,B>` | trait | ✓ | ✓ | ✓ | No |
| `is_same_v<A,B>` | variable | ✗ | ✗ | ✓ (C++14) | No |
| `is_const<T>` | trait | ✓ | ✓ | ✓ | No |
| `is_const_v<T>` | variable | ✗ | ✗ | ✓ (C++14) | No |

### 1.5  Primary Type Categories

| Symbol | Kind | C++98/03 | C++11 | C++14+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------------------|
| `is_integral<T>` | trait | ✓ | ✓ (+char16/32_t) | ✓ | No |
| `is_integral_v<T>` | variable | ✗ | ✗ | ✓ (C++14) | No |
| `is_floating_point<T>` | trait | ✓ | ✓ | ✓ | No |
| `is_floating_point_v<T>` | variable | ✗ | ✗ | ✓ (C++14) | No |
| `is_pointer<T>` | trait | ✓ | ✓ | ✓ | No |
| `is_pointer_v<T>` | variable | ✗ | ✗ | ✓ (C++14) | No |
| `is_function<T>` | trait | ✓ (arity ≤10) | ✓ (all arities) | ✓ | No |
| `is_function_v<T>` | variable | ✗ | ✗ | ✓ (C++14) | No |
| `is_enum<T>` | trait | intrinsic | intrinsic | intrinsic | **Yes** — `__is_enum` |
| `is_enum_v<T>` | variable | ✗ | ✗ | intrinsic (C++14) | **Yes** |

### 1.6  Sign Detection

| Symbol | Kind | C++98/03 | C++11 | C++14+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------------------|
| `is_signed<T>` | trait | ✓ | ✓ | ✓ | No |
| `is_signed_v<T>` | variable | ✗ | ✗ | ✓ (C++14) | No |
| `is_unsigned<T>` | trait | ✓ | ✓ | ✓ | No |
| `is_unsigned_v<T>` | variable | ✗ | ✗ | ✓ (C++14) | No |

### 1.7  Enum Support

| Symbol | Kind | C++98/03 | C++11 | C++14+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------------------|
| `underlying_type<T>` | trait | intrinsic | intrinsic | intrinsic | **Yes** — `__underlying_type` |
| `underlying_type_t<T>` | alias | ✗ | intrinsic | intrinsic | **Yes** |

---

## 2  `type_traits.hpp` — Compiler Intrinsic Availability

The following table shows the minimum compiler version at which each
intrinsic becomes available. These are available in **all language modes**,
including C++98/03.

| Intrinsic | GCC | Clang | MSVC | Intel | Purpose |
|-----------|-----|-------|------|-------|---------|
| `__is_enum(T)` | 4.3+ | All | 2012+ (v11) | 13.0+ | `is_enum` implementation |
| `__underlying_type(T)` | 4.7+ | All | 2012+ (v11) | 13.0+ | `underlying_type` implementation |

### Fallback Behavior When Intrinsics Are Absent

| Trait | Fallback | Effect on `any` |
|-------|----------|-----------------|
| `is_enum<T>` | Always `false_type` | Enum values route to heap storage (functional but no SBO) |
| `underlying_type<T>` | Not defined | Never instantiated (gated behind `is_enum::value`) |

### Detection Macros

| Macro | Value | Meaning |
|-------|-------|---------|
| `D_RESTD_HAS_IS_ENUM` | 0 or 1 | Whether `is_enum` uses a real intrinsic vs. false fallback |
| `D_RESTD_HAS_UNDERLYING_TYPE` | 0 or 1 | Whether `underlying_type` is defined at all |

---

## 3  `any.hpp` — Feature Availability

### 3.1  Storage Paths

| Feature | C++98/03 | C++11 | C++14+ | C++20+ |
|---------|----------|-------|--------|--------|
| SBO: bool | ✓ | ✓ | constexpr | constexpr |
| SBO: signed integrals | ✓ | ✓ | constexpr | constexpr |
| SBO: unsigned integrals | ✓ | ✓ | constexpr | constexpr |
| SBO: floating point | ✓ | ✓ | constexpr | constexpr |
| SBO: non-const pointer | ✓ | ✓ | constexpr | constexpr |
| SBO: const pointer | ✓ | ✓ | constexpr | constexpr |
| SBO: enum | fallback¹ | ✓² | constexpr² | constexpr² |
| Heap: class types | gated³ | gated³ | gated³ | gated³ |

¹ Requires `D_RESTD_HAS_IS_ENUM`; otherwise enum → heap.
² Requires `D_RESTD_HAS_UNDERLYING_TYPE` for lossless round-trip.
³ Requires `D_ENV_CPP98_HAS_NEW`.

### 3.2  Class Members

| Member | Kind | C++98/03 | C++11 | C++14+ | Notes |
|--------|------|----------|-------|--------|-------|
| `any()` | ctor | ✓ | ✓ | constexpr | Default (empty) |
| `any(T)` | ctor | ✓ | ✓ | constexpr (SBO) | SBO or heap depending on T |
| `any(const any&)` | ctor | ✓ | ✓ | ✓ | Deep-copies heap values |
| `any(any&&)` | ctor | ✗ | ✓ | ✓ | Gated on rvalue references |
| `operator=(const any&)` | assign | ✓ | ✓ | ✓ | |
| `operator=(any&&)` | assign | ✗ | ✓ | ✓ | Gated on rvalue references |
| `~any()` | dtor | ✓ | ✓ | ✓ | Calls reset() |
| `has_value()` | observer | ✓ | ✓ | constexpr | |
| `operator bool()` | observer | fallback⁴ | ✓ (explicit) | constexpr | |
| `category()` | observer | ✓ | ✓ | constexpr | |
| `type()` | observer | ✓ | ✓ | constexpr | Returns `any_type_id` |
| `holds<T>()` | observer | ✓ | ✓ | constexpr | |
| `is_sbo()` | observer | ✓ | ✓ | constexpr | |
| `get<T>()` | retrieval | ✓ | ✓ | constexpr (SBO) | |
| `get_ref<T>()` | retrieval | ✓ | ✓ | ✓ | Heap types only |
| `reset()` | modifier | ✓ | ✓ | ✓ | |
| `swap(any&)` | modifier | ✓ | ✓ | ✓ | |
| `emplace<T>(args...)` | modifier | ✗ | ✓ | ✓ | Requires variadic templates |

⁴ C++98/03 uses safe-bool idiom instead of explicit operator bool.

### 3.3  Type Identity

| Symbol | Kind | C++98/03 | C++11 | C++14+ |
|--------|------|----------|-------|--------|
| `any_type_id` | typedef | ✓ | ✓ | ✓ |
| `any_type_id_of<T>` | trait | ✓ | ✓ | constexpr |
| `any_type_id_of_v<T>` | variable | ✗ | ✗ | ✓ (C++14) |
| `DAnyCategory` | enum | ✓ (struct wrapper) | ✓ (enum class) | ✓ |

---

## 4  Free Function Modules

### 4.1  `any_cast.hpp`

| Overload | C++98/03 | C++11 | C++14+ | Notes |
|----------|----------|-------|--------|-------|
| `any_cast<T>(any*)` → `T*` | ✓ | ✓ | ✓ | nullptr on mismatch |
| `any_cast<T>(const any*)` → `const T*` | ✓ | ✓ | ✓ | nullptr on mismatch |
| `any_cast<T>(const any&)` → `T` | ✓ | ✓ | ✓ | Throws or UB (see below) |
| `any_cast<T>(any&)` → `T&` | ✓ | ✓ | ✓ | Throws or UB |
| `any_cast<T>(any&&)` → `T` | ✗ | ✓ | ✓ | Requires rvalue references |

**Exception behavior of reference overloads:**

| Condition | Behavior |
|-----------|----------|
| `D_ENV_CPP98_HAS_TYPEINFO` = 1 | Throws `bad_any_cast` on mismatch |
| `D_ENV_CPP98_HAS_EXCEPTION` = 1 (no typeinfo) | Throws `bad_any_cast` on mismatch |
| Neither available | Unchecked — undefined behavior on mismatch |

### 4.2  `any_swap.hpp`

| Symbol | C++98/03 | C++11 | C++14+ |
|--------|----------|-------|--------|
| `swap(any&, any&)` | ✓ | ✓ | constexpr inline |

### 4.3  `bad_any_cast.hpp`

| Condition | Base Class | `what()` | Virtual? |
|-----------|-----------|----------|----------|
| `D_ENV_CPP98_HAS_TYPEINFO` = 1 | `std::bad_cast` → `std::exception` | `"bad any_cast"` | Yes (override) |
| `D_ENV_CPP98_HAS_EXCEPTION` = 1 | `std::exception` | `"bad any_cast"` | Yes (override) |
| Neither | Standalone (no base) | `"bad any_cast"` | No |

### 4.4  `make_any.hpp`

| Overload | C++98/03 | C++11 | C++14+ | Notes |
|----------|----------|-------|--------|-------|
| `make_any<T>(args...)` | ✗ | ✓ | ✓ | Requires variadic templates |
| `make_any<T>(init_list, args...)` | ✗ | ✓ | ✓ | Requires variadic templates + initializer_list |

---

## 5  `utility/*` — Feature Availability

### 5.1  `swap.hpp`

| Overload | C++98/03 | C++11 | C++14+ | C++20+ | Notes |
|----------|----------|-------|--------|--------|-------|
| `swap(T&, T&)` | ✓ (copy) | ✓ (move, noexcept) | constexpr | constexpr | Implementation switches to move-based on C++11+ |
| `swap(T(&)[N], T(&)[N])` | ✓ | ✓ (noexcept) | constexpr | constexpr | Element-wise via the scalar overload |

**`noexcept` qualification:** conditional on C++11+, matching the
standard's `noexcept(is_nothrow_move_constructible<T>::value &&
is_nothrow_move_assignable<T>::value)` form. Because
`is_nothrow_move_constructible` requires variadic templates, the
conditional clause is gated on
`D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES`. On the rare
rvalue-references-without-variadic-templates compiler, swap is
unqualified rather than falsely `noexcept`. The array overload is
unqualified pending `is_nothrow_swappable`.

### 5.2  `move.hpp`

| Symbol | C++98/03 | C++11 | C++14+ | Notes |
|--------|----------|-------|--------|-------|
| `move(T&&)` | ✗ | ✓ (constexpr, noexcept) | ✓ (constexpr, noexcept) | Requires rvalue references |

Header is empty when `D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES` is 0.

### 5.3  `forward.hpp`

| Overload | C++98/03 | C++11 | C++14+ | Notes |
|----------|----------|-------|--------|-------|
| `forward<T>(remove_reference<T>::type&)` | ✗ | ✓ (constexpr, noexcept) | ✓ (constexpr, noexcept) | Lvalue overload |
| `forward<T>(remove_reference<T>::type&&)` | ✗ | ✓ (constexpr, noexcept) | ✓ (constexpr, noexcept) | Rvalue overload, includes `static_assert` against forwarding rvalue as lvalue |

Header is empty when `D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES` is 0.

### 5.4  `declval.hpp`

| Symbol | C++98/03 | C++11 | C++14+ | Notes |
|--------|----------|-------|--------|-------|
| `declval<T>()` | ✗ | ✓ (declared only) | ✓ (declared only) | Unevaluated contexts only; not defined |

Return type is `add_rvalue_reference<T>::type`, so `declval<void>()`
is well-formed and yields a prvalue of type `void`, matching the
standard library.

### 5.5  `pair.hpp`

| Member | Kind | C++98/03 | C++11 | C++14+ | Notes |
|--------|------|----------|-------|--------|-------|
| `pair()` | ctor | ✓ | constexpr | constexpr | Value-initializes both members |
| `pair(const T1&, const T2&)` | ctor | ✓ | constexpr | constexpr | |
| `pair(const pair<U1,U2>&)` | ctor | ✓ | constexpr | constexpr | Templated converting copy |
| `pair(U1&&, U2&&)` | ctor | ✗ | constexpr | constexpr | Perfect-forwarding |
| `pair(pair&&)` | ctor | ✗ | constexpr | constexpr | Move; conditional `noexcept` (variadic-gated) |
| `pair(pair<U1,U2>&&)` | ctor | ✗ | constexpr | constexpr | Templated converting move; conditional `noexcept` |
| `operator=(const pair&)` | assign | ✓ | ✓ | ✓ | Member-wise copy |
| `operator=(const pair<U1,U2>&)` | assign | ✓ | ✓ | ✓ | Templated converting copy assign |
| `operator=(pair&&)` | assign | ✗ | ✓ | ✓ | Member-wise move |
| `operator=(pair<U1,U2>&&)` | assign | ✗ | ✓ | ✓ | Templated converting move assign |
| `swap(pair&)` | member | ✓ | ✓ | ✓ | Member-wise via `restd::swap` |
| `first` / `second` | data | ✓ | ✓ | ✓ | Public, by std convention |

**Comparison operators** (non-member, all `constexpr` on C++11+):

| Operator | C++98/03 | C++11+ | Notes |
|----------|----------|--------|-------|
| `operator==` | ✓ | constexpr | |
| `operator!=` | ✓ | constexpr | Defined as `!(==)` |
| `operator<` | ✓ | constexpr | Lexicographic; uses `<` only |
| `operator<=` | ✓ | constexpr | Defined as `!(rhs < lhs)` |
| `operator>` | ✓ | constexpr | Defined as `rhs < lhs` |
| `operator>=` | ✓ | constexpr | Defined as `!(lhs < rhs)` |

**Non-member swap:** `swap(pair&, pair&)` — available on all tiers,
delegates to the member swap.

**Move-op `noexcept`:** conditional, matching the standard.
- Move ctor: `noexcept(is_nothrow_move_constructible<T1> &&
  is_nothrow_move_constructible<T2>)`. Gated on
  `D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES`; falls back to
  unqualified on rvalue-refs-without-variadic compilers.
- Move assignment: `noexcept(is_nothrow_move_assignable<T1> &&
  is_nothrow_move_assignable<T2>)` (always available on C++11+).
- Member `swap` and non-member `swap`: unqualified pending
  `is_nothrow_swappable`.

**Not provided:** `tuple_size` / `tuple_element` specializations
(awaiting `<tuple>`); `piecewise_construct` (awaiting `<tuple>`);
C++20 `<=>` (awaiting `<compare>`).

### 5.6  `make_pair.hpp`

| Overload | C++98/03 | C++11 | C++14+ | Notes |
|----------|----------|-------|--------|-------|
| `make_pair(T1, T2)` | ✓ | ✗ | ✗ | Pass-by-value (C++98/03 path) |
| `make_pair(T1&&, T2&&)` | ✗ | ✓ (constexpr) | ✓ (constexpr) | Forwarding (C++11+ path) |

**Deviation from `std::make_pair`:** `reference_wrapper` unwrapping
is not performed (no `reference_wrapper` in restd yet). Type decay
(reference removal, cv-stripping, array-to-pointer, function-to-pointer)
is now correct via `restd::decay` on the C++11+ path.

---

## 6  Dependency Map

Shows which env detection macros gate which features.

| Macro | Source Header | Gates |
|-------|-------------|-------|
| `D_ENV_LANG_IS_CPP11_OR_HIGHER` | `env.h` | `<type_traits>` alternative, enum class, using-alias, noexcept |
| `D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES` | `env_cpp_features.h` | Move ctor/assign, `any_cast(any&&)`, `move`, `forward`, `declval`, pair move ops, forwarding `make_pair` |
| `D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES` | `env_cpp_features.h` | `is_function` variadic path, `make_any`, `emplace`, `is_nothrow_move_constructible`, conditional-`noexcept` clauses on `swap` and `pair`'s move ctor |
| `D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES` | `env_cpp_features.h` | All `_v` convenience variables |
| `D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES` | `env_cpp_features.h` | All `_t` convenience aliases |
| `D_ENV_CPP98_HAS_NEW` | `env_cpp98.h` | Heap storage path |
| `D_ENV_CPP98_HAS_UTILITY` | `env_cpp98.h` | `<utility>` include |
| `D_ENV_CPP98_HAS_TYPEINFO` | `env_cpp98.h` | `bad_any_cast` base class, checked `any_cast` |
| `D_ENV_CPP98_HAS_EXCEPTION` | `env_cpp98.h` | `bad_any_cast` base class (fallback) |
| `D_RESTD_HAS_IS_ENUM` | `type_traits.hpp` | Enum SBO path |
| `D_RESTD_HAS_UNDERLYING_TYPE` | `type_traits.hpp` | `underlying_type` availability |

---

## 7  `functional/` — Feature Availability

The first batch ships 33 of the 44 surface symbols. The 11 deferred
symbols are listed in §7.6.

### 7.1  Function Objects

All 19 share the same shape: per-`_Type` primary template at the
indicated tier, transparent-void specialisation at C++14+. Legacy
`first/second_argument_type` and `result_type` typedefs are present on
C++98 through C++17 and gated out from C++20 (matching std's removal).

| Symbol | Kind | C++98/03 | C++11 | C++14+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------------------|
| `plus<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `plus<>`) | No |
| `minus<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `minus<>`) | No |
| `multiplies<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `multiplies<>`) | No |
| `divides<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `divides<>`) | No |
| `modulus<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `modulus<>`) | No |
| `negate<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `negate<>`) | No |
| `equal_to<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `equal_to<>`) | No |
| `not_equal_to<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `not_equal_to<>`) | No |
| `greater<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `greater<>`) | No |
| `less<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `less<>`) | No |
| `greater_equal<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `greater_equal<>`) | No |
| `less_equal<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `less_equal<>`) | No |
| `logical_and<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `logical_and<>`) | No |
| `logical_or<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `logical_or<>`) | No |
| `logical_not<T>` | class | ✓ | ✓ constexpr | ✓ constexpr (+ `logical_not<>`) | No |
| `bit_and<T>` | class | ✗ | ✓ constexpr | ✓ constexpr (+ `bit_and<>`) | No |
| `bit_or<T>` | class | ✗ | ✓ constexpr | ✓ constexpr (+ `bit_or<>`) | No |
| `bit_xor<T>` | class | ✗ | ✓ constexpr | ✓ constexpr (+ `bit_xor<>`) | No |
| `bit_not<T>` | class | ✗ | ✓ constexpr (back-port) | ✓ constexpr (+ `bit_not<>`) | No |

restd is `constexpr` from C++11 on every primary template; std didn't
make these constexpr until C++14. The transparent-void specialisations
require `decltype` and perfect forwarding and so are C++14+ regardless.

### 7.2  Reference Wrappers

| Symbol | Kind | C++98/03 | C++11 | C++14+ | C++17+ | C++20+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------|--------|--------------------|
| `reference_wrapper<T>` | class | ✗ | ✓ constexpr (back-port) | ✓ constexpr | ✓ constexpr (+ deduction guide) | ✓ constexpr | No |
| `ref(t)` | factory | ✗ | ✓ constexpr | ✓ constexpr | ✓ constexpr | ✓ constexpr | No |
| `cref(t)` | factory | ✗ | ✓ constexpr | ✓ constexpr | ✓ constexpr | ✓ constexpr | No |
| `unwrap_reference<T>` | trait | ✗ | ✓ (back-port) | ✓ | ✓ | ✓ | No |
| `unwrap_reference_t<T>` | alias | ✗ | ✓ | ✓ | ✓ | ✓ | No |
| `unwrap_ref_decay<T>` | trait | ✗ | ✓ (back-port) | ✓ | ✓ | ✓ | No |
| `unwrap_ref_decay_t<T>` | alias | ✗ | ✓ | ✓ | ✓ | ✓ | No |
| `is_reference_wrapper<T>` | trait (restd extension) | ✗ | ✓ | ✓ | ✓ | ✓ | No |
| `is_reference_wrapper_v<T>` | variable | ✗ | ✗ | ✓ (C++14) | ✓ | ✓ | No |

`reference_wrapper`'s rvalue ctor is explicitly deleted on every
supporting tier. `is_reference_wrapper` is restd's exposed name for the
trait the standard uses internally; user code may use it for SFINAE.

### 7.3  Invocation

| Symbol | Kind | C++98/03 | C++11 | C++14+ | C++17+ | C++20+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------|--------|--------------------|
| `invoke(f, args...)` | function | ✗ | ✓ constexpr (back-port) | ✓ constexpr | ✓ constexpr | ✓ constexpr | No |
| `invoke_r<R>(f, args...)` | function (non-void R) | ✗ | ✓ constexpr (back-port) | ✓ constexpr | ✓ constexpr | ✓ constexpr | No |
| `invoke_r<void>(f, args...)` | function (void R) | ✗ | ✓ (back-port, *not* constexpr) | ✓ constexpr | ✓ constexpr | ✓ constexpr | No |
| `mem_fn(pm)` | factory | ✗ | ✓ constexpr | ✓ constexpr | ✓ constexpr | ✓ constexpr | No |
| `not_fn(f)` | factory | ✗ | ✓ constexpr (back-port) | ✓ constexpr | ✓ constexpr | ✓ constexpr | No |
| `identity` | class | ✗ | ✓ constexpr (back-port) | ✓ constexpr | ✓ constexpr | ✓ constexpr | No |

restd makes all of these constexpr from C++11, even though the standard
did not arrive at the same point until C++20 (P1065). The exception is
`invoke_r<void>`: C++11 forbids constexpr void functions, so on C++11
that overload is *not* constexpr; from C++14 it is.

### 7.4  Bind-Support Traits, Bad-Function-Call

| Symbol | Kind | C++98/03 | C++11 | C++14+ | C++17+ | Compiler Intrinsic |
|--------|------|----------|-------|--------|--------|--------------------|
| `is_bind_expression<T>` | trait | ✗ | ✓ (primary only) | ✓ | ✓ | No |
| `is_bind_expression_v<T>` | variable | ✗ | ✗ | ✓ (C++14) | ✓ | No |
| `is_placeholder<T>` | trait | ✗ | ✓ (primary only) | ✓ | ✓ | No |
| `is_placeholder_v<T>` | variable | ✗ | ✗ | ✓ (C++14) | ✓ | No |
| `bad_function_call` | class | ✓ (standalone fallback) | ✓ (`std::exception` derived if `D_ENV_CPP98_HAS_EXCEPTION`) | ✓ | ✓ | No |

`is_bind_expression` and `is_placeholder` ship as primary templates
only — the `bind` family that would specialise them is deferred. User
code can still specialise these traits for its own custom binders /
placeholders today.

### 7.5  Hash

The primary template is empty (no `operator()`); using
`hash<UnsupportedKey>` is ill-formed at instantiation, matching std's
"disabled specialisation" idiom.

| Specialisation | C++98/03 | C++11 | C++14+ | C++17+ | C++20+ | constexpr in restd | Notes |
|----------------|----------|-------|--------|--------|--------|--------------------|-------|
| `hash<bool>` | ✓ | ✓ | ✓ | ✓ | ✓ | C++11 | identity cast |
| `hash<char>` | ✓ | ✓ | ✓ | ✓ | ✓ | C++11 | identity cast |
| `hash<signed char>` | ✓ | ✓ | ✓ | ✓ | ✓ | C++11 | |
| `hash<unsigned char>` | ✓ | ✓ | ✓ | ✓ | ✓ | C++11 | |
| `hash<wchar_t>` | ✓ | ✓ | ✓ | ✓ | ✓ | C++11 | |
| `hash<short>`, `hash<int>`, `hash<long>` (and unsigned forms) | ✓ | ✓ | ✓ | ✓ | ✓ | C++11 | |
| `hash<long long>`, `hash<unsigned long long>` | ✗ | ✓ | ✓ | ✓ | ✓ | C++11 | |
| `hash<char16_t>`, `hash<char32_t>` | ✗ | ✓ | ✓ | ✓ | ✓ | C++11 | |
| `hash<char8_t>` | ✗ | ✗ | ✗ | ✗ | ✓ | C++11 (where available) | |
| `hash<float>`, `hash<double>`, `hash<long double>` | ✓ | ✓ | ✓ | ✓ | ✓ | not constexpr | bytewise via `memcpy`; -0.0 normalises to 0 |
| `hash<T*>` | ✓ | ✓ | ✓ | ✓ | ✓ | not constexpr | `reinterpret_cast` is non-constexpr |
| `hash<nullptr_t>` | ✗ | ✓ | ✓ | ✓ | ✓ | C++11 | always returns 0 |

Container/string-aware specialisations (`hash<basic_string>`,
`hash<unique_ptr>`, `hash<optional>`, etc.) ship alongside their
respective container modules.

### 7.6  Deferred Symbols (NOT YET IMPLEMENTED)

| Symbol | std introduced | Blocked on |
|--------|----------------|-----------|
| `function<Sig>` | C++11 | type-erasure infrastructure |
| `move_only_function` | C++23 | `function` infrastructure + signature-qualifier parsing |
| `copyable_function` | C++26 | `function` infrastructure |
| `function_ref` | C++26 | type-erasure infrastructure (non-owning view) |
| `bind` | C++11 | `restd::tuple` |
| `bind_front` | C++20 | `restd::tuple` |
| `bind_back` | C++23 | `restd::tuple` |
| `placeholders::_1`...`_N` | C++11 | ships with `bind` |
| `default_searcher` | C++17 | `restd::algorithm` (`search` family) |
| `boyer_moore_searcher` | C++17 | `restd::algorithm` |
| `boyer_moore_horspool_searcher` | C++17 | `restd::algorithm` |

These appear on the workbook's *Coverage Failures* and *Pending
Dependencies* sheets. The roadmap entry for `<functional>` covers the
follow-on milestone.
