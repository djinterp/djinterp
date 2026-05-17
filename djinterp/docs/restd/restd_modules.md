# restd — Module Documentation

> **Last updated:** 2026-05-07

---

## Table of Contents

1. [type_traits.hpp](#1-type_traitshpp)
2. [any.hpp](#2-anyhpp)
3. [any_cast.hpp](#3-any_casthpp)
4. [any_swap.hpp](#4-any_swaphpp)
5. [bad_any_cast.hpp](#5-bad_any_casthpp)
6. [make_any.hpp](#6-make_anyhpp)
7. [utility/swap.hpp](#7-utilityswaphpp)
8. [utility/move.hpp](#8-utilitymovehpp)
9. [utility/forward.hpp](#9-utilityforwardhpp)
10. [utility/declval.hpp](#10-utilitydeclvalhpp)
11. [utility/pair.hpp](#11-utilitypairhpp)
12. [utility/make_pair.hpp](#12-utilitymake_pairhpp)
13. [functional/ — module overview](#13-functional--module-overview)
    - [13.1 Function objects (arithmetic, comparison, logical, bitwise)](#131-function-objects-arithmetic-comparison-logical-bitwise)
    - [13.2 `identity`](#132-identity)
    - [13.3 Reference wrappers (`reference_wrapper`, `ref`, `cref`, `unwrap_reference`, `unwrap_ref_decay`)](#133-reference-wrappers-reference_wrapper-ref-cref-unwrap_reference-unwrap_ref_decay)
    - [13.4 Invocation (`invoke`, `invoke_r`, `mem_fn`, `not_fn`)](#134-invocation-invoke-invoke_r-mem_fn-not_fn)
    - [13.5 Bind-support traits (`is_bind_expression`, `is_placeholder`)](#135-bind-support-traits-is_bind_expression-is_placeholder)
    - [13.6 `bad_function_call`](#136-bad_function_call)
    - [13.7 `hash`](#137-hash)
    - [13.8 Deferred symbols](#138-deferred-symbols)

---

## 1  `type_traits.hpp`

**Path:** `/inc/restd/type_traits/type_traits.hpp`
**Namespace:** `restd`
**Min standard:** C++98
**Dependencies:** `djinterp.hpp` (or standalone restd core when extracted)

### Purpose

Provides the minimal set of compile-time type introspection traits needed
by the `any` module, without any dependency on `<type_traits>`. Every trait
is a self-contained implementation that works on C++98/03 compilers. This
header is intended to be the foundation that all other restd modules build
upon when they need type classification.

### Design Decisions

**No `<type_traits>` dependency, ever.** Even on C++11+ where `<type_traits>`
is available, restd provides its own implementations. This ensures that
restd's behavior is identical across all standard tiers and that no subtle
differences in vendor implementations leak through.

**Compiler intrinsics are gated, not assumed.** The two traits that require
builtins (`is_enum`, `underlying_type`) are wrapped behind detection macros
(`D_RESTD_HAS_IS_ENUM`, `D_RESTD_HAS_UNDERLYING_TYPE`) that test for the
builtin at preprocessing time. Dependent code must check these macros before
relying on the traits. When intrinsics are absent, `is_enum` degrades to
`false_type` (never reports true) and `underlying_type` is not defined at all.

**CV-stripping is explicit.** Traits like `is_integral` strip cv-qualifiers
via `remove_cv` before testing, matching the standard's behavior. This means
`is_integral<const int>::value` is `true`.

**ODR safety for C++03.** `integral_constant::value` has an out-of-class
definition to prevent linker errors when `::value` is ODR-used (e.g. passed
by reference or address-of). This is harmless on C++17+ where inline variables
make it redundant.

### Symbols

---

#### `integral_constant<_Type, _Value>`

**Category:** trait
**Standard:** C++98+

Wraps a compile-time constant. Base class for all boolean traits. Provides:

- `static const _Type value` — the wrapped constant.
- `typedef _Type value_type` — the constant's type.
- `typedef integral_constant<_Type, _Value> type` — identity typedef.

---

#### `true_type` / `false_type`

**Category:** typedef
**Standard:** C++98+

Convenience typedefs for `integral_constant<bool, true>` and
`integral_constant<bool, false>`. Used as base classes for boolean
traits.

---

#### `enable_if<_Condition, _Type>`

**Category:** trait (SFINAE)
**Standard:** C++98+

Defines member `typedef type` as `_Type` when `_Condition` is `true`.
When `_Condition` is `false`, the primary template has no `type` member,
causing substitution failure.

**`enable_if_t<_Condition, _Type>`** — alias template (C++11+).

**Usage patterns:**

In C++11+, `enable_if` is typically used as a default template argument:
```cpp
template<typename _T,
         typename restd::enable_if<restd::is_integral<_T>::value,
                                   int>::type = 0>
void foo(_T _v);
```

In C++98/03, it must be used as a return type or extra parameter:
```cpp
template<typename _T>
typename restd::enable_if<restd::is_integral<_T>::value, void>::type
foo(_T _v);
```

---

#### `remove_const<_Type>` / `remove_volatile<_Type>` / `remove_cv<_Type>`

**Category:** trait
**Standard:** C++98+

Removes top-level cv-qualifiers. `remove_cv` composes both.

`remove_const<const int>::type` → `int`
`remove_cv<const volatile int>::type` → `int`

Aliases: `remove_const_t`, `remove_volatile_t`, `remove_cv_t` (C++11+).

---

#### `remove_pointer<_Type>`

**Category:** trait
**Standard:** C++98+

Removes one level of pointer indirection, including through cv-qualified
pointer forms (`T* const`, `T* volatile`, `T* const volatile`).

`remove_pointer<int*>::type` → `int`
`remove_pointer<int* const>::type` → `int`
`remove_pointer<int>::type` → `int` (passthrough)

Alias: `remove_pointer_t` (C++11+).

---

#### `is_same<_A, _B>`

**Category:** trait
**Standard:** C++98+

`true_type` if `_A` and `_B` are the identical type (including
cv-qualification), `false_type` otherwise.

`is_same<int, int>::value` → `true`
`is_same<int, const int>::value` → `false`

Variable: `is_same_v` (C++14+).

---

#### `is_const<_Type>`

**Category:** trait
**Standard:** C++98+

`true_type` if `_Type` is const-qualified, `false_type` otherwise.

`is_const<const int>::value` → `true`
`is_const<int>::value` → `false`

Variable: `is_const_v` (C++14+).

---

#### `is_integral<_Type>`

**Category:** trait
**Standard:** C++98+

`true_type` if `_Type` (ignoring cv) is one of: `bool`, `char`,
`signed char`, `unsigned char`, `wchar_t`, `short`, `unsigned short`,
`int`, `unsigned int`, `long`, `unsigned long`, `long long`,
`unsigned long long`. On C++11+, also includes `char16_t` and `char32_t`.
On C++20+, also includes `char8_t`.

Implemented via explicit specializations of an internal
`is_integral_base` template (no compiler magic).

Variable: `is_integral_v` (C++14+).

---

#### `is_floating_point<_Type>`

**Category:** trait
**Standard:** C++98+

`true_type` if `_Type` (ignoring cv) is `float`, `double`, or
`long double`.

Variable: `is_floating_point_v` (C++14+).

---

#### `is_pointer<_Type>`

**Category:** trait
**Standard:** C++98+

`true_type` if `_Type` (ignoring cv) is a pointer type. This includes
both object pointers and function pointers.

`is_pointer<int*>::value` → `true`
`is_pointer<void(*)(int)>::value` → `true`
`is_pointer<int>::value` → `false`

Variable: `is_pointer_v` (C++14+).

---

#### `is_function<_Type>`

**Category:** trait
**Standard:** C++98+

`true_type` if `_Type` is a function type (not a function pointer, not
a functor, not a lambda). Used by `any` to exclude function pointers
from the pointer SBO category.

**C++11+ path:** Two variadic-template partial specializations cover all
arities — `R(Args...)` and `R(Args..., ...)`.

**C++98/03 path:** Explicit specializations for arities 0 through 10,
each with and without C-style variadic parameters. Functions with more
than 10 parameters are not detected. This is sufficient for the `any`
module's pointer-filtering logic.

`is_function<void(int)>::value` → `true`
`is_function<void(*)(int)>::value` → `false` (that's a pointer)

Variable: `is_function_v` (C++14+).

---

#### `is_enum<_Type>`

**Category:** trait
**Standard:** C++98+ (with intrinsic)
**Requires:** `D_RESTD_HAS_IS_ENUM` = 1

`true_type` if `_Type` is an enumeration type. Implemented via the
`__is_enum` compiler builtin.

**Fallback (no intrinsic):** Always `false_type`. This causes enum values
to route through the heap storage path in `any` rather than the SBO.

Variable: `is_enum_v` (C++14+, with intrinsic).

---

#### `is_signed<_Type>` / `is_unsigned<_Type>`

**Category:** trait
**Standard:** C++98+

`is_signed` is `true_type` if `_Type` (ignoring cv) is a signed
arithmetic type (signed integral or floating-point). `is_unsigned` is
`true_type` if `_Type` is an unsigned integral type.

Sign detection for integral types uses the
`static_cast<T>(-1) < static_cast<T>(0)` technique, which is a constant
expression and works at compile time on all conforming compilers.

Floating-point types are always signed, never unsigned.

Variables: `is_signed_v`, `is_unsigned_v` (C++14+).

---

#### `underlying_type<_Type>`

**Category:** trait
**Standard:** C++98+ (with intrinsic)
**Requires:** `D_RESTD_HAS_UNDERLYING_TYPE` = 1

Yields `typedef type` as the underlying integral type of enumeration
`_Type`. Implemented via the `__underlying_type` compiler builtin.

**When absent:** Not defined at all (not a fallback to `void` or similar).
Code that uses `underlying_type` must be gated on
`D_RESTD_HAS_UNDERLYING_TYPE`. This is safe because the only consumer
(enum SBO in `any`) is itself gated on `is_enum::value`, which is false
when intrinsics are absent.

Alias: `underlying_type_t` (C++11+, with intrinsic).

---

## 2  `any.hpp`

**Path:** `/inc/restd/any/any.hpp`
**Namespace:** `restd`
**Min standard:** C++98 (planned; currently C++11)
**Dependencies:** `type_traits.hpp`, `djinterp.hpp`

### Purpose

Type-erased value container. Portable alternative to `std::any` (C++17)
with constexpr support for small trivial types via SBO (small buffer
optimization). Works on environments where `<any>` is unavailable.

### Architecture

The class uses a two-path storage strategy:

**SBO (Small Buffer Optimization):** A union of fundamental categories
stores any type whose value fits one of: bool, signed integral, unsigned
integral, floating point, non-const pointer, const pointer, or enum
(when intrinsics available). SBO construction and retrieval are constexpr
on C++14+.

**Heap:** Types that don't fit the SBO (classes, containers, large
aggregates) are stored in a heap-allocated control block with type-erased
copy/move/destroy via a function-pointer operations table. Requires
`D_ENV_CPP98_HAS_NEW`. Never constexpr.

**Type identity** is RTTI-free: each type gets a unique identity derived
from the address of a static function template instantiation
(`any_type_tag_fn<T>`). This enables constexpr type checking with zero
virtual dispatch overhead.

### Key Types

#### `any_type_id`

Typedef for `void(*)()`. The opaque identifier for a stored type.

#### `any_type_id_of<_Type>`

Trait yielding the `any_type_id` for `_Type` as `::value`.

#### `DAnyCategory`

Scoped enum (C++11+) or struct-wrapped enum (C++98/03) identifying which
union member is active: `cat_empty`, `cat_bool`, `cat_signed`,
`cat_unsigned`, `cat_floating`, `cat_pointer`, `cat_cpointer`, `cat_heap`.

#### `any`

The main class. See the compatibility table for per-member availability
across standard tiers.

### Internal Types (namespace `restd::internal`)

- **`any_category_of<T>`** — maps a type to its `DAnyCategory`. Primary
  template yields `cat_heap`; partial specializations handle SBO types.
- **`is_sbo_type<T>`** — true if `T` uses the SBO path.
- **`any_sbo`** — union with per-member constexpr constructors.
- **`any_heap_ops`** — function-pointer operations table (destroy, clone).
- **`heap_destroy<T>`** / **`heap_clone<T>`** — typed implementations.
- **`any_heap_ops_for<T>()`** — returns the ops table for `T` via local
  static (thread-safe in C++11).

### Usage Examples

```cpp
// SBO path — constexpr on C++14+
constexpr restd::any a(42);           // cat_signed
constexpr int v = a.get<int>();        // 42

// Heap path — runtime
restd::any b(std::string("hello"));    // cat_heap
std::string& s = b.get<std::string>(); // "hello"

// Type checking — no RTTI
if (a.holds<int>())   { /* ... */ }    // true
if (a.holds<float>()) { /* ... */ }    // false
```

---

## 3  `any_cast.hpp`

**Path:** `/inc/restd/any/any_cast.hpp`
**Namespace:** `restd`
**Min standard:** C++98
**Dependencies:** `any.hpp`, `bad_any_cast.hpp`

### Purpose

Provides type-safe access to the value stored in a `restd::any`. Five
overloads mirror the C++17 `std::any_cast` interface.

### Overloads

#### Pointer overloads (always available, unchecked)

```cpp
template<typename _Type> _Type*       any_cast(any* _a)       noexcept;
template<typename _Type> const _Type* any_cast(const any* _a) noexcept;
```

Returns a pointer to the stored value if it matches `_Type`, or `nullptr`
otherwise. The caller must check the return value. Never throws.

#### Reference overloads (checked — when exceptions available)

Available when `D_ENV_CPP98_HAS_TYPEINFO` or `D_ENV_CPP98_HAS_EXCEPTION`
is true.

```cpp
template<typename _Type> _Type  any_cast(const any& _a);  // copy
template<typename _Type> _Type& any_cast(any& _a);        // reference
template<typename _Type> _Type  any_cast(any&& _a);       // move (C++11+)
```

Throws `bad_any_cast` when the stored type does not match `_Type`.

#### Reference overloads (unchecked — when exceptions unavailable)

When neither `D_ENV_CPP98_HAS_TYPEINFO` nor `D_ENV_CPP98_HAS_EXCEPTION`
is available, the reference overloads are provided without type checking.
Accessing a mismatched type is **undefined behavior**.

### Known Issue in Current Code

Line 95 of `any_cast.hpp` contains a typo in the preprocessor guard:
`D_ENV_CPP98_HAS_TypeYPEINFO` should be `D_ENV_CPP98_HAS_TYPEINFO`.
This causes the checked overloads to never compile when only `<typeinfo>`
is available (the exception-only path still works).

---

## 4  `any_swap.hpp`

**Path:** `/inc/restd/any/any_swap.hpp`
**Namespace:** `restd`
**Min standard:** C++98
**Dependencies:** `any.hpp`

### Purpose

Non-member ADL swap overload for `restd::any`. Delegates to the
`any::swap` member function. This is the any-specific swap; the general
`restd::swap` (when implemented) will be in a separate `utility/swap.hpp`.

### Signature

```cpp
D_CONSTEXPR_INLINE void swap(any& _lhs, any& _rhs) noexcept;
```

### Behavior

Calls `_lhs.swap(_rhs)`. The member swap currently uses copy-and-swap
via a temporary. This is correct but not optimal for heap-stored values
(it clones). A future optimization could pointer-swap the heap members
directly.

---

## 5  `bad_any_cast.hpp`

**Path:** `/inc/restd/any/bad_any_cast.hpp`
**Namespace:** `restd`
**Min standard:** C++98
**Dependencies:** conditionally `<typeinfo>` or `<exception>`

### Purpose

Exception type thrown by the checked `any_cast` reference overloads when
the requested type does not match the stored type.

### Tiered Implementation

The class adapts its inheritance hierarchy based on available headers:

**Tier 1 — `D_ENV_CPP98_HAS_TYPEINFO` is true:**
Inherits `std::bad_cast` (which itself inherits `std::exception`).
`what()` is virtual, marked `noexcept override`.

**Tier 2 — `D_ENV_CPP98_HAS_EXCEPTION` is true (no typeinfo):**
Inherits `std::exception` directly. `what()` is virtual, marked
`noexcept override`.

**Tier 3 — neither available:**
Standalone class with no base. `what()` is non-virtual. Can be thrown and
caught by type, but does not participate in `catch(std::exception&)`
handlers.

### `what()` Return Value

All tiers return `"bad any_cast"` (matches std convention).

---

## 6  `make_any.hpp`

**Path:** `/inc/restd/any/make_any.hpp`
**Namespace:** `restd`
**Min standard:** C++11 (requires variadic templates)
**Dependencies:** `any.hpp`, `<initializer_list>`

### Purpose

Factory functions for constructing `restd::any` objects with emplaced
values. Mirrors the C++17 `std::make_any` interface.

### Overloads

```cpp
template<typename _Type, typename... _Args>
any make_any(_Args&&... _args);

template<typename _Type, typename _U, typename... _Args>
any make_any(std::initializer_list<_U> _il, _Args&&... _args);
```

Both create an empty `any`, then call `emplace<_Type>(...)` on it.

### Known Issue in Current Code

The initializer_list overload parameter is written as
`_Type::initializer_list<_U>`, which is syntactically invalid — `_Type`
is the target value type, not `std` (or `restd`). This should be
`std::initializer_list<_U>` (or the restd equivalent). The forwarding
overload is correct.

### Why Not In C++98?

Both overloads require variadic templates (`typename... _Args`) and
perfect forwarding (`_Args&&...`). There is no meaningful C++98
equivalent — the user can construct an `any` directly via its value
constructors instead.

---

## 7  `utility/swap.hpp`

**Path:** `/inc/restd/utility/swap.hpp`
**Namespace:** `restd`
**Min standard:** C++98
**Dependencies:** `djinterp.hpp`, `<cstddef>`; conditionally
`restd/utility/move.hpp`,
`restd/type_traits/is_nothrow_move_constructible.hpp`, and
`restd/type_traits/is_nothrow_move_assignable.hpp`

### Purpose

Generic `swap` algorithm for arbitrary types. Provides both the
two-reference scalar overload and the two-array overload. This is the
generic fallback; type-specific overloads (e.g. `swap(any&, any&)` in
`any/any_swap.hpp`) take precedence via the standard two-step swap
idiom (unqualified call after a `using restd::swap;` directive).

### Tiered Implementation

Three independent tiers, each self-contained:

**C++14+:** `D_CONSTEXPR` (yields `constexpr`) + conditional
`noexcept`. Move-based body. The constexpr-relaxation rules of C++14
permit the multi-statement body.

**C++11:** Conditional `noexcept`. Move-based body. Not `constexpr`
because move construction followed by two move assignments is a
multi-statement function body, which is not permitted in C++11
`constexpr` functions.

**C++98/03:** No qualifiers. Copy-based body (`tmp = lhs; lhs = rhs;
rhs = tmp;`).

### `noexcept`

Matches the standard:

```cpp
noexcept(is_nothrow_move_constructible<T>::value &&
        is_nothrow_move_assignable<T>::value)
```

Because `is_nothrow_move_constructible` requires variadic templates,
the conditional clause is gated on
`D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES`. On the rare
rvalue-references-without-variadic-templates compiler, swap is
unqualified rather than falsely `noexcept`. The array overload is
unqualified pending `is_nothrow_swappable` (C++17 trait, not yet in
restd).

### Array Overload

```cpp
template<typename T, std::size_t N>
void swap(T (&lhs)[N], T (&rhs)[N]);
```

Implemented as an element-wise loop calling the scalar overload.
Provided on all standard tiers (the standard introduced it in C++11,
but the implementation is C++98-compatible).

---

## 8  `utility/move.hpp`

**Path:** `/inc/restd/utility/move.hpp`
**Namespace:** `restd`
**Min standard:** C++11 (requires rvalue references)
**Dependencies:** `djinterp.hpp`, `restd/type_traits/remove_reference.hpp`

### Purpose

Provides `restd::move`, the canonical rvalue cast. Produces an rvalue
reference to its argument so that move-aware constructors and
assignment operators can take the rvalue path.

### Implementation

Single-statement function body — `static_cast` to the argument's type
with references stripped, then `&&` re-applied. Single-statement bodies
are `constexpr`-eligible on C++11.

```cpp
template<typename T>
constexpr typename remove_reference<T>::type&&
move(T&& v) noexcept
{
    return static_cast<typename remove_reference<T>::type&&>(v);
}
```

### Empty on C++98/03

Without rvalue references there is no meaningful implementation. The
entire body of the header is gated on
`D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES`. Code that uses
`restd::move` must itself be gated on the same macro.

---

## 9  `utility/forward.hpp`

**Path:** `/inc/restd/utility/forward.hpp`
**Namespace:** `restd`
**Min standard:** C++11 (requires rvalue references)
**Dependencies:** `djinterp.hpp`,
`restd/type_traits/remove_reference.hpp`,
`restd/type_traits/is_lvalue_reference.hpp`

### Purpose

Provides `restd::forward`, the canonical mechanism for perfect
forwarding inside templates. Used to pass an argument to another
function while preserving its value category and cv-qualification.

### Two Overloads

```cpp
template<typename T>
constexpr T&& forward(typename remove_reference<T>::type&  v) noexcept;

template<typename T>
constexpr T&& forward(typename remove_reference<T>::type&& v) noexcept;
```

The lvalue overload is used when an lvalue is being forwarded; the
rvalue overload is used for rvalues. Both rely on reference collapsing
(`T&& &` collapses to `T&`, `T&& &&` collapses to `T&&`) to deliver
the right value category.

### Safety `static_assert`

The rvalue overload contains:

```cpp
static_assert(!is_lvalue_reference<T>::value,
              "restd::forward: cannot forward an rvalue as an lvalue");
```

This catches the misuse `forward<T&>(rvalue_expression)`, which would
silently bind an rvalue to an lvalue reference if not detected.

### Empty on C++98/03

Same gating as `move.hpp`.

---

## 10  `utility/declval.hpp`

**Path:** `/inc/restd/utility/declval.hpp`
**Namespace:** `restd`
**Min standard:** C++11 (requires rvalue references)
**Dependencies:** `djinterp.hpp`,
`restd/type_traits/add_rvalue_reference.hpp`

### Purpose

Provides `restd::declval<T>()`, used in unevaluated contexts
(`decltype`, `sizeof`, `noexcept`, `requires`) to obtain a value of
type `T` without needing a default constructor.

### Implementation

```cpp
template<typename T>
typename add_rvalue_reference<T>::type
declval() noexcept;   // declared, intentionally never defined
```

The function is **declared but never defined**. Calling it at runtime
is a linker error — that is the safety mechanism that keeps it from
being misused outside unevaluated contexts.

### Return Type via `add_rvalue_reference`

| Argument `T`     | Return type |
|------------------|-------------|
| `int`            | `int&&`     |
| `int&`           | `int&`      |
| `int&&`          | `int&&`     |
| `void`           | `void`      |
| `const void`     | `const void`|

The cv-`void` cases are handled by `add_rvalue_reference`'s explicit
specializations (forming `void&&` would otherwise be ill-formed).
This matches the standard library exactly: `declval<void>()` is
well-formed and yields a prvalue of type `void`.

---

## 11  `utility/pair.hpp`

**Path:** `/inc/restd/utility/pair.hpp`
**Namespace:** `restd`
**Min standard:** C++98
**Dependencies:** `djinterp.hpp`, `restd/utility/swap.hpp`;
conditionally `restd/utility/move.hpp`,
`restd/utility/forward.hpp`,
`restd/type_traits/is_nothrow_move_constructible.hpp`, and
`restd/type_traits/is_nothrow_move_assignable.hpp`

### Purpose

Heterogeneous two-value aggregate. Mirrors `std::pair` with
restd's standard portability tiering.

### Members

| Member | Description |
|--------|-------------|
| `first_type` | Typedef alias for `T1` |
| `second_type` | Typedef alias for `T2` |
| `first` | Public `T1` data member |
| `second` | Public `T2` data member |

### Constructors

**Available on all tiers:**
- `pair()` — value-initializes both members
- `pair(const T1&, const T2&)` — by-value copy
- `pair(const pair<U1, U2>&)` — converting copy

**C++11+ only (require rvalue references):**
- `pair(U1&&, U2&&)` — perfect-forwarding constructor
- `pair(pair&&)` — move constructor
- `pair(pair<U1, U2>&&)` — converting move constructor

All constructors use `D_CONSTEXPR` (yields `constexpr` on C++11+).
Their bodies are empty — all initialization happens in the member
init list — which is C++11 `constexpr`-compliant.

### Assignment

The four assignment overloads (copy, converting copy, move, converting
move) are not `constexpr`-qualified. C++11 forbids `constexpr` non-static
non-`const` member functions, and even on C++14+ `std::pair`'s `operator=`
did not become `constexpr` until C++20.

### `noexcept`

Move ops carry conditional `noexcept`:

- **Move ctor:** `noexcept(is_nothrow_move_constructible<T1>::value &&
  is_nothrow_move_constructible<T2>::value)`. Because the trait
  needs variadic templates, the clause is gated on
  `D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES`. On the rare
  rvalue-refs-without-variadic-templates compiler the move ctor is
  unqualified.
- **Move assignment:** `noexcept(is_nothrow_move_assignable<T1>::value
  && is_nothrow_move_assignable<T2>::value)` — always available on
  C++11+.
- **Member `swap` and non-member `swap`:** unqualified pending
  `is_nothrow_swappable` (C++17 trait, not yet in restd).

### Comparison Operators

Six non-member operators (`==`, `!=`, `<`, `<=`, `>`, `>=`), all
`D_CONSTEXPR`, all single-`return` bodies. Lexicographic ordering is
defined entirely in terms of `operator<`, so element types only need
to supply `==` and `<` for the full set to work.

### `swap`

Two swap entry points:

- **Member:** `void pair::swap(pair&)` — member-wise via
  `restd::swap` on each field.
- **Non-member:** `restd::swap(pair&, pair&)` — delegates to the
  member.

The non-member overload makes `restd::pair` work correctly with the
two-step swap idiom and ADL.

### Not Provided (Yet)

| Symbol | Reason |
|--------|--------|
| `tuple_size<pair>` / `tuple_element<N, pair>` | Awaits `<tuple>` |
| `piecewise_construct_t` / `piecewise_construct` | Depends on `tuple` |
| `operator<=>` | Awaits `<compare>` (C++20) |
| Structured binding support | Provided automatically by aggregate-style data members; works with the C++17 "via tuple_size" path only after `<tuple>` lands |

---

## 12  `utility/make_pair.hpp`

**Path:** `/inc/restd/utility/make_pair.hpp`
**Namespace:** `restd`
**Min standard:** C++98
**Dependencies:** `djinterp.hpp`, `restd/utility/pair.hpp`;
conditionally `restd/utility/forward.hpp` and
`restd/type_traits/decay.hpp`

### Purpose

Factory function that constructs a `pair` while deducing its element
types from the arguments.

### Two Tiers

**C++11+:** Perfect-forwarding overload using `decay` for the result
type, matching `std::make_pair`'s type-decay behavior.

```cpp
template<typename T1, typename T2>
constexpr pair<typename decay<T1>::type,
               typename decay<T2>::type>
make_pair(T1&& x, T2&& y);
```

**C++98/03:** Pass-by-value overload.

```cpp
template<typename T1, typename T2>
pair<T1, T2> make_pair(T1 x, T2 y);
```

The C++98 form drops references and cv-qualifiers naturally through
parameter passing. The C++11+ form preserves move-only semantics via
forwarding while applying the same decay rules as `std::make_pair`
(reference removal, cv-stripping, array-to-pointer, function-to-pointer).

### Deviation from `std::make_pair`

**No `reference_wrapper` unwrapping.** `std::make_pair` has a special
case where `reference_wrapper<T>` deduces to `T&`. restd does not have
`reference_wrapper` yet, so this case is not handled. Will be
addressed when `reference_wrapper` lands.


---

## 13  `functional/` — Module Overview

**Path:** `/inc/restd/functional/`
**Umbrella header:** `restd/functional/functional.hpp`
**Namespace:** `restd`
**Min standard:** C++98 (function objects), C++11 (everything else)
**Dependencies:** `djinterp.hpp`, `restd/type_traits/type_traits.hpp`,
`restd/utility/forward.hpp`

### Purpose

Provides standard `<functional>` facilities — function objects, reference
wrappers, generalised invocation, hashing primary templates — without any
dependency on `<functional>` itself. The first batch ships everything that
can be implemented without `restd::tuple` (which `bind` needs) or
`restd::algorithm` (which the searchers need); those symbols are listed
in §13.8.

### Design Decisions

**Granular files, single-symbol-per-header.** Every public symbol gets its
own `.hpp`. `restd/functional/functional.hpp` is the umbrella. Including
the umbrella is the easy default; including just `restd/functional/invoke.hpp`
is the right call for headers that only need invoke and want to keep the
TU dependency graph tight.

**Constexpr maximisation.** The standard delivered most `<functional>`
constexpr in waves (function objects in C++14; `invoke` / `mem_fn` /
`not_fn` in C++20 via P1065). restd lifts `D_CONSTEXPR` onto every body
that meets C++11's single-return requirement, so on a C++11 compiler
restd's `invoke` is `constexpr` even though `std::invoke` of the same era
is not.

**No dependency on the standard library's `<functional>`.** The whole
module is implemented in terms of djinterp + restd's own type_traits and
utility headers. The only `<…>` dependencies are `<cstddef>` and
`<cstring>` for `hash` (`size_t` and `memcpy`-based fp hashing) and
`<exception>` for `bad_function_call` (gated on
`D_ENV_CPP98_HAS_EXCEPTION`).

**Reference_wrapper / invoke include cycle.** `invoke` dispatches on
`reference_wrapper<U>` and `reference_wrapper::operator()` forwards to
`invoke`. The cycle is broken by having `invoke.hpp` forward-declare
`reference_wrapper` and `is_reference_wrapper`, and by having
`reference_wrapper.hpp` include `invoke.hpp` *at the bottom of the file*
— after its own class definition is complete. That way, no matter which
header user code includes first, both types are fully defined before any
template body is instantiated.

---

### 13.1  Function Objects (Arithmetic, Comparison, Logical, Bitwise)

**Files:**
- Arithmetic: `plus.hpp`, `minus.hpp`, `multiplies.hpp`, `divides.hpp`,
  `modulus.hpp`, `negate.hpp`
- Comparison: `equal_to.hpp`, `not_equal_to.hpp`, `greater.hpp`, `less.hpp`,
  `greater_equal.hpp`, `less_equal.hpp`
- Logical: `logical_and.hpp`, `logical_or.hpp`, `logical_not.hpp`
- Bitwise: `bit_and.hpp`, `bit_or.hpp`, `bit_xor.hpp`, `bit_not.hpp`

All 19 follow the same shape:

```cpp
template<typename _Type
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
                     = void
#endif
        >
struct OP
{
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    typedef _Type first_argument_type;   // omitted on unary
    typedef _Type second_argument_type;  // omitted on unary
    typedef _Type/_bool result_type;
#endif

    D_CONSTEXPR _Type/bool
    operator()(const _Type& _x, const _Type& _y) const
    { return _x OP _y; }
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<>
struct OP<void>
{
    typedef int is_transparent;

    template<typename _T, typename _U>
    D_CONSTEXPR auto
    operator()(_T&& _x, _U&& _y) const
        -> decltype(restd::forward<_T>(_x) OP restd::forward<_U>(_y))
    { return restd::forward<_T>(_x) OP restd::forward<_U>(_y); }
};
#endif
```

**Tier behaviour:**
- C++98: per-`_Type` primary only. No transparent specialisation; no
  default `void` argument. The legacy `first_argument_type` /
  `second_argument_type` / `argument_type` / `result_type` typedefs are
  present.
- C++11: same surface, plus `D_CONSTEXPR` on `operator()`.
- C++14: default `_Type = void` and the transparent-void specialisation
  with `is_transparent` typedef.
- C++17: legacy typedefs deprecated by std but still present in restd
  (matches std).
- C++20: legacy typedefs gated out (matches std's removal).

**Notes:**
- `bit_and` / `bit_or` / `bit_xor` are C++11 in std; restd matches.
- `bit_not` is C++14 in std; restd back-ports it to C++11 since the
  primary needs nothing C++14-specific.
- Comparison and logical operations have `result_type = bool`; arithmetic
  and bitwise have `result_type = _Type`.

---

### 13.2  `identity`

**File:** `restd/functional/identity.hpp`
**Standard:** C++20+ (back-ported to C++11)

Perfect-forwarding passthrough callable. Returns its argument unchanged.
Used by ranges-style code as the default projection.

```cpp
struct identity
{
    typedef int is_transparent;
    template<typename _Type>
    D_CONSTEXPR _Type&& operator()(_Type&& _v) const noexcept
    { return restd::forward<_Type>(_v); }
};
```

The `is_transparent` typedef lets `identity` participate in
heterogeneous-lookup overloads of associative containers.

---

### 13.3  Reference Wrappers

**Files:** `reference_wrapper.hpp`, `ref.hpp`, `cref.hpp`,
`unwrap_reference.hpp`, `unwrap_ref_decay.hpp`
**Standard:** C++11 for `reference_wrapper`/`ref`/`cref`, C++20 for
`unwrap_reference` and `unwrap_ref_decay` (restd back-ports the latter
two to C++11)

**`reference_wrapper<T>`** stores a `T*` and exposes the wrapped
reference via implicit conversion, `get()`, and a variadic `operator()`
that forwards through `restd::invoke`. The rvalue ctor is explicitly
deleted to forbid binding to temporaries.

**`ref(t)`** / **`cref(t)`** are the canonical factories. Each provides
three overloads: lvalue (returns wrapper), rvalue (deleted), and
reference_wrapper passthrough so `ref(ref(x))` is just `ref(x)`.

**`unwrap_reference<T>`** yields `T` for ordinary types and `U&` for
`reference_wrapper<U>`. **`unwrap_ref_decay<T>`** is
`unwrap_reference<decay_t<T>>` — the canonical "what does `make_pair` /
`make_tuple` / `bind_front` deduce for arg `T`?" computation.

The `is_reference_wrapper<T>` trait (defined in `reference_wrapper.hpp`)
is what `invoke` consults internally for its bullet-2 / bullet-5
dispatch.

---

### 13.4  Invocation

**Files:** `invoke.hpp`, `invoke_r.hpp`, `mem_fn.hpp`, `not_fn.hpp`
**Standard:** `invoke` is C++17, `invoke_r` is C++23, `mem_fn` is C++11,
`not_fn` is C++17 — all back-ported / forward-ported as needed.

**`invoke(f, args...)`** implements the standard's INVOKE pseudo-op
across all seven cases (PMF or PMD × {base/derived, reference_wrapper,
pointer/smart-pointer first arg}, plus a generic-callable bullet).
Each case is a separate SFINAE-discriminated overload of an
`internal::INVOKE` helper; the public `invoke` forwards to the helper.

```cpp
template<typename _F, typename... _Args>
D_CONSTEXPR auto
invoke(_F&& _f, _Args&&... _args)
    -> decltype(internal::INVOKE(restd::forward<_F>(_f),
                                 restd::forward<_Args>(_args)...));
```

Constexpr from C++11 in restd; the standard did not adopt constexpr-
INVOKE until C++20 (P1065).

**`invoke_r<R>(f, args...)`** is `invoke` with an explicit return type.
Two overloads: non-void (lets implicit conversion to `R` happen at the
return) and void (discards the result). The void overload is not
constexpr on C++11 because C++11 forbids constexpr void functions; from
C++14 it is `D_CONSTEXPR`-qualified.

**`mem_fn(pm)`** wraps a pointer-to-member into a uniform callable. The
returned object holds the member pointer and forwards its `operator()`
to `restd::invoke`, so PMF/PMD dispatch and reference_wrapper handling
come for free.

**`not_fn(f)`** returns a callable that negates the result of invoking
`f`. Replaces the deprecated C++98 `not1` / `not2` adaptors. Stores `f`
by decayed value; provides both lvalue and const-lvalue `operator()`.

---

### 13.5  Bind-Support Traits

**Files:** `is_bind_expression.hpp`, `is_placeholder.hpp`
**Standard:** C++11+

These are the user customisation points the standard exposes for
recognising bind-expression and placeholder types. Both ship as primary
templates only (`false_type` and `integral_constant<int, 0>`
respectively) until `bind` itself ships, at which point its result types
will specialise these traits. User code that defines its own binders or
placeholders specialises them today — that part of the surface works
even without `bind`.

The C++17 `_v` variable templates ship gated on
`D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES`.

---

### 13.6  `bad_function_call`

**File:** `restd/functional/bad_function_call.hpp`
**Standard:** C++11 (the type), but the body is C++98-compatible

The exception type that a future `restd::function::operator()` will
throw when invoked while empty. Inheritance is tiered, mirroring
`bad_any_cast` and `bad_optional_access`:

- When `D_ENV_CPP98_HAS_EXCEPTION`: derives from `std::exception` so it
  participates in `catch (std::exception&)`.
- Otherwise: standalone class with an internal `what()`.

The exception type lands now even though `function` itself is deferred,
so user code that catches it can be written today.

---

### 13.7  `hash`

**File:** `restd/functional/hash.hpp`
**Standard:** C++11+

Customisation point for hashing values. The primary template is empty
(no `operator()`), so `hash<UnsupportedType>{}(x)` is ill-formed at
instantiation — exactly matching std's "disabled specialisation" idiom.

**Specialisations shipped this milestone:**
- All integral types (`bool`, `char`, signed / unsigned variants,
  `wchar_t`, `short`, `int`, `long`, `long long` and unsigned forms,
  `char8_t`/`char16_t`/`char32_t` gated on language tier).
- All floating-point types (`float`, `double`, `long double`).
- Pointer types (`T*` for any `T`).
- `nullptr_t` (gated on C++11+).

The integer specialisations use identity-cast to `size_t` (matching
libstdc++/libc++'s choice for short keys); they are constexpr from
C++11. The pointer specialisation uses `reinterpret_cast<size_t>` and
is therefore not constexpr on any tier. The floating-point
specialisations bytewise-hash through `memcpy`, normalising signed-zero
to a zero hash so equal-comparing values hash equally; not constexpr
because `memcpy` is not constexpr until C++20.

**Specialisations deferred** to ship with their respective container
modules: `hash<basic_string<…>>`, `hash<string_view>`, `hash<bitset<N>>`,
`hash<vector<bool>>`, `hash<unique_ptr<…>>`, `hash<shared_ptr<…>>`,
`hash<optional<T>>`, `hash<variant<…>>`, `hash<thread::id>`,
`hash<error_code>`, `hash<error_condition>`, `hash<type_index>`,
`hash<coroutine_handle<…>>`.

---

### 13.8  Deferred Symbols

The following `<functional>` symbols are **not yet shipped** in this
milestone. They appear on the workbook's *Coverage Failures* sheet as
`NOT IMPLEMENTED` with their dependency.

| Symbol | Blocked on |
|--------|-----------|
| `function`, `move_only_function`, `copyable_function`, `function_ref` | type-erasure infrastructure (heap allocation gated on `D_ENV_CPP98_HAS_NEW`, signature-pack parsing) |
| `bind`, `bind_front`, `bind_back` | `restd::tuple` (used as the bound-args storage) |
| `placeholders::_1` ... `placeholders::_N` | ships with `bind` |
| `default_searcher`, `boyer_moore_searcher`, `boyer_moore_horspool_searcher` | `restd::algorithm` (`search` family) |

The roadmap entry for `<functional>` (priority 6 in
`coverage_data.py::ROADMAP_ENTRIES`) covers the next batch.
