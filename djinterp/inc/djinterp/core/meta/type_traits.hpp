/******************************************************************************
* djinterp [meta]                                              type_traits.hpp
*
* djinterp type traits header:
*   This header provides type trait utilities and extensions to the standard
* <type_traits> library. It includes:
*   - portable definitions for standard type traits across C++ versions
*   - SFINAE / detection-idiom machinery and convenience macros
*   - tuple introspection utilities
*   - class definition rule traits (Rule of Zero/Three/Five)
*   - container/allocator traits
* STRUCTURE:
*   The header is laid out in dependency order:
*     0.  SFINAE / detection-idiom machinery (void_t, the idiom, the macros).
*         All trait-detection infrastructure lives here, in one place, at the
*         very top, because the rest of the file depends on it.
*     I.  Portable standard-library traits (logical metafunctions, callable
*         traits, C++20 / C++23 features, C++17 _v aliases).
*    II.  Portable macro shorthands for the logical metafunctions.
*   III.  Custom djinterp-specific traits built on top of the above.
* PORTABILITY:
*   This header uses env.h for C++ version detection and cpp_features.h for
* fine-grained feature detection. It provides fallback implementations for
* features that may be definable but not defined in older C++ versions:
*   - bool_constant    (C++17, definable in C++11)
*   - conjunction      (C++17, definable in C++11)
*   - disjunction      (C++17, definable in C++11)
*   - invoke_result    (C++17, replaces result_of in C++11)
*   - is_bounded_array (C++20, definable in C++11)
*   - is_invocable     (C++17, definable in C++11 with limitations)
*   - is_scoped_enum   (C++23, definable in C++11)
*   - negation         (C++17, definable in C++11)
*   - remove_cvref     (C++20, definable in C++11)
*   - type_identity    (C++20, definable in C++11)
*   - void_t           (C++17, definable in C++11)
*
*
* path:      /inc/djinterp/core/meta/type_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2024.03.09
******************************************************************************/

#ifndef DJINTERP_META_TYPE_TRAITS_
#define DJINTERP_META_TYPE_TRAITS_ 1

// std
#include <memory>
#include <tuple>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"


// =============================================================================
// 0.   SFINAE / DETECTION IDIOM MACHINERY
// =============================================================================
// This section is the single point of truth for trait detection in djinterp.
// It contains:
//
//   0.1  void_t          : the foundational SFINAE helper. Defined first
//                          because both the idiom (below) and the macros
//                          (further below) depend on it.
//   0.2  Detection idiom : nonesuch, detector, detected_or, detected_or_t,
//                          detected_t, is_detected, is_detected_convertible,
//                          is_detected_exact (+ _v variants).
//   0.3  Detection macros: D_VOID_T plus the D_TRAIT_* family. These macros
//                          are pure shorthand - they expand to code that
//                          uses the idiom (via void_t), they never
//                          reimplement the SFINAE pattern.
//
// Version detection (env.h):
//   - D_ENV_LANG_IS_CPP11_OR_HIGHER  : true if C++11 or later
//   - D_ENV_LANG_IS_CPP14_OR_HIGHER  : true if C++14 or later
//   - D_ENV_LANG_IS_CPP17_OR_HIGHER  : true if C++17 or later
//   - D_ENV_LANG_IS_CPP20_OR_HIGHER  : true if C++20 or later
//   - D_ENV_LANG_IS_CPP23_OR_HIGHER  : true if C++23 or later
//
// Feature detection (cpp_features.hpp):
//   - D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
//   - D_ENV_CPP_FEATURE_LANG_DECLTYPE
//   - D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
//   - D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

NS_DJINTERP


// -----------------------------------------------------------------------------
// 0.1  void_t (foundational SFINAE helper)
// -----------------------------------------------------------------------------
// Required by the detection idiom (0.2) and the detection macros (0.3). Pulled
// in first for that reason; the rest of the portable logical metafunctions
// live further down in section I.1.

// void_t is provided canonically by djinterp.hpp (namespace djinterp) and
// pulled in transitively via the core include below.  This header MUST NOT
// re-declare it: a second declaration in the same namespace (e.g.
// `using std::void_t;`) conflicts with djinterp::void_t under clang/MSVC
// ("target of using declaration conflicts with declaration already in
// scope").  The detection idiom (0.2) below uses the unqualified name, which
// resolves to djinterp::void_t.  D_VOID_T (0.3) remains available for the
// portable macro spelling.


// -----------------------------------------------------------------------------
// 0.2  Detection idiom (C++11+)
// -----------------------------------------------------------------------------
// djinterp-owned detection idiom support.
//   - detected_or, detected_or_t, detected_t
//   - is_detected, is_detected_convertible, is_detected_exact
//   - nonesuch
// This is a djinterp portability facility, not a standard-library import.
// It requires the core language machinery needed for generic detection idioms.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER           &&                                \
    D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES  &&                                \
    D_ENV_CPP_FEATURE_LANG_DECLTYPE         &&                                \
    D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

// nonesuch
//   type: placeholder type for detection idiom representing "no such type".
// Cannot be constructed, destructed, or copied.
struct nonesuch
{
    nonesuch()                      = delete;
    ~nonesuch()                     = delete;
    nonesuch(const nonesuch&)       = delete;
    void operator=(const nonesuch&) = delete;
};

NS_INTERNAL
    // detector
    //   trait: primary template for detection idiom (failure case).
    // Returns false_type and _Default when _Op<_Args...> is ill-formed.
    template<typename                       _Default,
          typename                       _AlwaysVoid,
          template<typename...> typename _Op,
          typename...                    _Args>
    struct detector
    {
        using value_t = std::false_type;
        using type    = _Default;
    };

    // detector
    //   trait: specialization for detection idiom (success case).
    // Returns true_type and _Op<_Args...> when well-formed.
    template<typename                       _Default,
          template<typename...> typename _Op,
             typename...                    _Args>
    struct detector<_Default, void_t<_Op<_Args...>>, _Op, _Args...>
    {
        using value_t = std::true_type;
        using type    = _Op<_Args...>;
    };
NS_END  // internal

// detected_or
//   trait: detector with custom default type.
// Provides value_t and type members.
template<typename                       _Default,
         template<typename...> typename _Op,
         typename...                    _Args>
struct detected_or : internal::detector<_Default, void, _Op, _Args...>
{};

// detected_or_t
//   alias template: yields _Op<_Args...> if well-formed, else _Default.
template<typename                       _Default,
         template<typename...> typename _Op,
         typename...                    _Args>
using detected_or_t = typename detected_or<_Default, _Op, _Args...>::type;

// detected_t
//   alias template: yields _Op<_Args...> if well-formed, else nonesuch.
template<template<typename...> typename _Op,
         typename...                    _Args>
using detected_t = typename internal::detector<nonesuch, void, _Op, _Args...>::type;

// is_detected
//   trait: detects if _Op<_Args...> is well-formed.
template<template<typename...> typename _Op,
         typename...                    _Args>
struct is_detected
    : internal::detector<nonesuch, void, _Op, _Args...>::value_t
{};

// is_detected_convertible
//   trait: checks if _Op<_Args...> is well-formed and convertible to _To.
template<typename                       _To,
         template<typename...> typename _Op,
         typename...                    _Args>
struct is_detected_convertible
    : std::is_convertible<detected_t<_Op, _Args...>, _To>
{};

// is_detected_exact
//   trait: checks if _Op<_Args...> is well-formed and exactly _Expected.
template<typename                       _Expected,
         template<typename...> typename _Op,
         typename...                    _Args>
struct is_detected_exact
    : std::is_same<_Expected, detected_t<_Op, _Args...>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_detected_v
    //   variable template: value of is_detected<_Op, _Args...>.
    template<template<typename...> typename _Op,
             typename...                    _Args>
    constexpr bool is_detected_v = is_detected<_Op, _Args...>::value;

    // is_detected_convertible_v
    //   variable template: value of is_detected_convertible<_To, _Op, _Args...>.
    template<typename                       _To,
             template<typename...> typename _Op,
             typename...                    _Args>
    constexpr bool is_detected_convertible_v =
        is_detected_convertible<_To, _Op, _Args...>::value;

    // is_detected_exact_v
    //   variable template: value of is_detected_exact<_Expected, _Op, _Args...>.
    template<typename                       _Expected,
             template<typename...> typename _Op,
             typename...                    _Args>
    constexpr bool is_detected_exact_v =
        is_detected_exact<_Expected, _Op, _Args...>::value;
#endif

#endif  // detection idiom support

NS_END  // djinterp


// -----------------------------------------------------------------------------
// 0.3  Detection macros
// -----------------------------------------------------------------------------
// Convenience shorthands for the detection idiom. Each macro below expands
// to a trait that uses `void_t<DETECTION_EXPR>` (i.e., the detection idiom
// pattern) - none of them reimplement SFINAE machinery from scratch.
//
// Macros sit at file scope (intentionally - the C++ preprocessor has no
// concept of namespaces), but the traits they emit are intended to be
// instantiated inside whatever namespace the macro is invoked in (typically
// the djinterp namespace below).
//
// Family overview:
//   - D_VOID_T                     : portable spelling of void_t.
//   - D_TRAIT_VALUE_BOOL           : emits the `_v` variable-template alias.
//   - D_TRAIT_TYPE_ALIAS           : emits the `_t` type-alias companion.
//   - D_TRAIT_IS_DETECTED_AS       : parameterized core; emits a SFINAE trait
//                                    whose success-case base class is given
//                                    explicitly. Used by the others below.
//   - D_TRAIT_IS_DETECTED          : one-line shorthand for the bread-and-
//                                    butter SFINAE detection pattern; true if
//                                    each DETECTION_EXPR is well-formed, else
//                                    false. Variadic - pass any number of
//                                    void_t expressions to AND together.
//                                    (Renamed from D_TYPE_TRAIT_DETECTED.)
//   - D_TRAIT_IS_DETECTED_FROM     : like D_TRAIT_IS_DETECTED, but inherits
//                                    from BASE_TRAIT<_Type> on success rather
//                                    than std::true_type. (Renamed from
//                                    D_TYPE_TRAIT_NEW.)
//   - D_TRAIT_DETECT_METHOD,
//     D_TRAIT_DETECT_METHOD_ARGS,
//     D_TRAIT_DETECT_BINARY_OP,
//     D_TRAIT_DETECT_UNARY_OP      : decltype-expression generators meant to
//                                    be passed as DETECTION_EXPR.
//   - D_TRAIT_HAS_METHOD,
//     D_TRAIT_HAS_METHOD_ARGS,
//     D_TRAIT_HAS_METHOD_TYPED,
//     D_TRAIT_HAS_METHOD_CONVERTIBLE,
//     D_TRAIT_HAS_BINARY_OP,
//     D_TRAIT_HAS_UNARY_OP,
//     D_TRAIT_HAS_TYPE,
//     D_TRAIT_HAS_STATIC_MEMBER    : member-detection trait shortcuts built
//                                    from the above.
//   - D_TRAIT_IS_SPECIALIZATION_OF_AS,
//     D_TRAIT_IS_SPECIALIZATION_OF : partial-specialization-based "is this a
//                                    foo<...>?" traits - distinct family from
//                                    the SFINAE detection idiom above.

// D_VOID_T
//   macro: portable void_t for use in SFINAE contexts. Resolves to
// std::void_t in C++17+, djinterp::void_t otherwise.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_VOID_T  std::void_t
#elif D_ENV_LANG_IS_CPP11_OR_HIGHER
    #define D_VOID_T  djinterp::void_t
#endif

// D_TRAIT_VALUE_BOOL
//   macro: emits an `inline constexpr bool` variable-template alias
// `TRAIT_NAME_v` for the unary trait `TRAIT_NAME`.
#define D_TRAIT_VALUE_BOOL(TRAIT_NAME)                                        \
template<typename _Type>                                                      \
inline constexpr bool TRAIT_NAME##_v = TRAIT_NAME<_Type>::value;

// D_TRAIT_IS_DETECTED_AS
//   macro: parameterized core of the detection-trait family. Emits the
// primary template (std::false_type) plus the well-formed partial
// specialization, whose base class is `INHERIT_EXPR` (an arbitrary type
// expression that may mention `_Type`). Does NOT emit the `_v` alias -
// callers (or the shorthand macros below) add it via D_TRAIT_VALUE_BOOL.
#define D_TRAIT_IS_DETECTED_AS(TRAIT_NAME, DETECTION_EXPR, INHERIT_EXPR)      \
    template<typename _Type,                                                  \
            typename = void>                                                  \
    struct TRAIT_NAME : std::false_type                                       \
    {};                                                                       \
                                                                              \
    template<typename _Type>                                                  \
    struct TRAIT_NAME<_Type, D_VOID_T<DETECTION_EXPR>> : INHERIT_EXPR         \
    {};

// D_TRAIT_IS_DETECTED
//   macro: one-line shorthand for the bread-and-butter SFINAE trait pattern.
// Emits `TRAIT_NAME<_Type>` (inheriting std::true_type when every detection
// expression in `...` is well-formed for _Type, std::false_type otherwise)
// plus the `TRAIT_NAME_v` variable template. Variadic: pass one expression
// for a simple check, or several to require all of them to be well-formed
// (the AND-shape of the SFINAE `void_t<...>` idiom). Renamed from
// D_TYPE_TRAIT_DETECTED - the new name mirrors the standard
// `is_detected<_Op, _Args...>` from the detection-idiom proposal and makes
// the semantics obvious at the call site.
#define D_TRAIT_IS_DETECTED(TRAIT_NAME, ...)                                  \
    template<typename _Type,                                                  \
             typename = void>                                                 \
    struct TRAIT_NAME : std::false_type                                       \
    {};                                                                       \
                                                                              \
    template<typename _Type>                                                  \
    struct TRAIT_NAME<_Type, D_VOID_T<__VA_ARGS__>> : std::true_type          \
    {};                                                                       \
                                                                              \
    D_TRAIT_VALUE_BOOL(TRAIT_NAME)

// D_TRAIT_IS_DETECTED_FROM
//   macro: like D_TRAIT_IS_DETECTED, but inherits from `BASE_TRAIT<_Type>`
// on success instead of std::true_type. Useful when the trait needs to
// delegate further checks to another unary trait. Renamed from
// D_TYPE_TRAIT_NEW.
#define D_TRAIT_IS_DETECTED_FROM(TRAIT_NAME, DETECTION_EXPR, BASE_TRAIT)      \
D_TRAIT_IS_DETECTED_AS(TRAIT_NAME, DETECTION_EXPR, BASE_TRAIT<_Type>)         \
D_TRAIT_VALUE_BOOL(TRAIT_NAME)

// D_TRAIT_DETECT_METHOD
//   macro: yields a decltype-expression that detects a call to
// `_Type::METHOD_NAME(_Type::value_type{})`. Intended to be passed as the
// DETECTION_EXPR argument to one of the D_TRAIT_IS_DETECTED* macros.
#define D_TRAIT_DETECT_METHOD(METHOD_NAME)                                    \
decltype(std::declval<_Type&>().METHOD_NAME(                                  \
    std::declval<typename _Type::value_type>()))

// D_TRAIT_DETECT_METHOD_ARGS
//   macro: yields a decltype-expression that detects a call to
// `_Type::METHOD_NAME(...)` with arguments of the variadic types supplied.
// Intended to be passed as the DETECTION_EXPR argument to one of the
// D_TRAIT_IS_DETECTED* macros.
#define D_TRAIT_DETECT_METHOD_ARGS(METHOD_NAME, ...)                          \
decltype(std::declval<_Type&>().METHOD_NAME(std::declval<__VA_ARGS__>()))

// D_TRAIT_HAS_METHOD
//   macro: trait that detects whether `_Type` has a method `METHOD_NAME`
// callable with a single argument of type `_Type::value_type`. Sugar for
// `D_TRAIT_IS_DETECTED(..., D_TRAIT_DETECT_METHOD(METHOD_NAME))`.
#define D_TRAIT_HAS_METHOD(TRAIT_NAME, METHOD_NAME)                           \
D_TRAIT_IS_DETECTED(TRAIT_NAME, D_TRAIT_DETECT_METHOD(METHOD_NAME))

// D_TRAIT_HAS_METHOD_ARGS
//   macro: trait that detects whether `_Type` has a method `METHOD_NAME`
// callable with arguments of the variadic types supplied.
#define D_TRAIT_HAS_METHOD_ARGS(TRAIT_NAME, METHOD_NAME, ...)                 \
    D_TRAIT_IS_DETECTED(TRAIT_NAME,                                           \
        D_TRAIT_DETECT_METHOD_ARGS(METHOD_NAME, __VA_ARGS__))

// D_TRAIT_HAS_METHOD_TYPED
//   macro: trait that detects whether `_Type` has a method `METHOD_NAME`
// callable with the given argument types AND whose return type is exactly
// `RETURN_TYPE`. Built on D_TRAIT_IS_DETECTED_AS by inheriting from
// `std::is_same<call-result, RETURN_TYPE>` in the success case.
#define D_TRAIT_HAS_METHOD_TYPED(TRAIT_NAME, METHOD_NAME, RETURN_TYPE, ...)   \
    D_TRAIT_IS_DETECTED_AS(TRAIT_NAME,                                        \
        D_TRAIT_DETECT_METHOD_ARGS(METHOD_NAME, __VA_ARGS__),                 \
        std::is_same<D_TRAIT_DETECT_METHOD_ARGS(METHOD_NAME, __VA_ARGS__),    \
                        RETURN_TYPE>)                                         \
    D_TRAIT_VALUE_BOOL(TRAIT_NAME)

// D_TRAIT_HAS_METHOD_CONVERTIBLE
//   macro: like D_TRAIT_HAS_METHOD_TYPED, but checks that the call's return
// type is *convertible to* RETURN_TYPE rather than exactly RETURN_TYPE. Useful
// for traits that should accept covariant or implicitly-convertible returns
// (e.g. a `size()` that returns `unsigned` where you want it `size_t`).
#define D_TRAIT_HAS_METHOD_CONVERTIBLE(TRAIT_NAME,                            \
                                       METHOD_NAME,                           \
                                       RETURN_TYPE,                           \
                                       ...)                                   \
    D_TRAIT_IS_DETECTED_AS(TRAIT_NAME,                                        \
        D_TRAIT_DETECT_METHOD_ARGS(METHOD_NAME, __VA_ARGS__),                 \
        std::is_convertible<                                                  \
            D_TRAIT_DETECT_METHOD_ARGS(METHOD_NAME, __VA_ARGS__),             \
            RETURN_TYPE>)                                                     \
    D_TRAIT_VALUE_BOOL(TRAIT_NAME)

// D_TRAIT_HAS_TYPE
//   macro: trait that detects whether `_Type` has a nested type alias named
// `TYPE_NAME` (i.e., `typename _Type::TYPE_NAME` is well-formed). Sugar for
// the most common single-line SFINAE pattern.
#define D_TRAIT_HAS_TYPE(TRAIT_NAME, TYPE_NAME)                               \
    D_TRAIT_IS_DETECTED(TRAIT_NAME, typename _Type::TYPE_NAME)

// D_TRAIT_HAS_STATIC_MEMBER
//   macro: trait that detects whether `_Type` has an accessible static data
// member or constexpr named `MEMBER_NAME` (i.e., `decltype(_Type::MEMBER_NAME)`
// is well-formed). Note: this only validates that the name exists at class
// scope; it does NOT constrain the member's type. For type-constrained checks,
// combine with the multi-expression form of D_TRAIT_IS_DETECTED.
#define D_TRAIT_HAS_STATIC_MEMBER(TRAIT_NAME,                                 \
                                  MEMBER_NAME)                                \
    D_TRAIT_IS_DETECTED(TRAIT_NAME, decltype(_Type::MEMBER_NAME))

// D_TRAIT_DETECT_BINARY_OP
//   macro: yields a decltype-expression that detects a binary operator
// invocation `_Type{} OP _Type{}`. Intended to be passed as the DETECTION_EXPR
// argument to one of the D_TRAIT_IS_DETECTED* macros. Operands are `const
// _Type&` so the trait accepts immutable operands and won't be defeated by
// `const`-qualified types.
#define D_TRAIT_DETECT_BINARY_OP(OP)                                          \
    decltype(std::declval<const _Type&>() OP std::declval<const _Type&>())

// D_TRAIT_HAS_BINARY_OP
//   macro: trait that detects whether `_Type` supports binary operator `OP`
// between two instances of itself. Sugar for
// `D_TRAIT_IS_DETECTED(..., D_TRAIT_DETECT_BINARY_OP(OP))`.
#define D_TRAIT_HAS_BINARY_OP(TRAIT_NAME,                                     \
                              OP)                                             \
    D_TRAIT_IS_DETECTED(TRAIT_NAME,                                           \
                        D_TRAIT_DETECT_BINARY_OP(OP))

// D_TRAIT_DETECT_UNARY_OP
//   macro: yields a decltype-expression that detects a unary operator
// invocation `OP _Type{}`. Intended to be passed as the DETECTION_EXPR
// argument to one of the D_TRAIT_IS_DETECTED* macros.
#define D_TRAIT_DETECT_UNARY_OP(OP)                                           \
    decltype(OP std::declval<_Type&>())

// D_TRAIT_HAS_UNARY_OP
//   macro: trait that detects whether `_Type` supports unary operator `OP`
// (prefix form: `-x`, `*x`, `!x`, `++x`, etc.).
#define D_TRAIT_HAS_UNARY_OP(TRAIT_NAME,                                      \
                             OP)                                              \
    D_TRAIT_IS_DETECTED(TRAIT_NAME,                                           \
                        D_TRAIT_DETECT_UNARY_OP(OP))

// D_TRAIT_TYPE_ALIAS
//   macro: emits a `TRAIT_NAME_t` alias template for the unary trait
// `TRAIT_NAME`, defined as `typename TRAIT_NAME<_Type>::type`. The `::type`
// counterpart to D_TRAIT_VALUE_BOOL - together they cover the standard
// `_t` / `_v` companion pair for any unary trait exposing both a `::type`
// member and a `::value` member.
#define D_TRAIT_TYPE_ALIAS(TRAIT_NAME)                                        \
    template<typename _Type>                                                  \
    using TRAIT_NAME##_t = typename TRAIT_NAME<_Type>::type;

// D_TRAIT_IS_SPECIALIZATION_OF_AS
//   macro: parameterized core for the "is_X<Y>" partial-specialization
// pattern. Emits the primary template (inheriting std::false_type) and a
// partial specialization on `TEMPLATE_NAME<_Types...>` whose base class is
// `INHERIT_EXPR` (which may reference the deduced pack `_Types...`). Distinct
// from the D_TRAIT_IS_DETECTED family because it dispatches via partial
// specialization on a known class template rather than via SFINAE on an
// expression - useful for detecting "is this a foo<...>?" without an
// expression to substitute.
//
//   Limitation: TEMPLATE_NAME must be a class template whose parameters are
// all `typename`-kind (e.g. std::tuple, std::vector with default allocator
// elided). Templates with non-type or template-template parameters (e.g.
// std::array<T, N>) cannot be matched by this macro.
#define D_TRAIT_IS_SPECIALIZATION_OF_AS(TRAIT_NAME,                           \
                                        TEMPLATE_NAME,                        \
                                        INHERIT_EXPR)                         \
    template<typename _Type>                                                  \
    struct TRAIT_NAME : std::false_type                                       \
    {};                                                                       \
                                                                              \
    template<typename... _Types>                                              \
    struct TRAIT_NAME<TEMPLATE_NAME<_Types...>> : INHERIT_EXPR                \
    {};

// D_TRAIT_IS_SPECIALIZATION_OF
//   macro: one-line shorthand for the bread-and-butter "is this a
// TEMPLATE_NAME<...>?" partial-specialization trait. Inherits from
// std::true_type in the matching specialization and emits a `_v` alias.
// Sugar over D_TRAIT_IS_SPECIALIZATION_OF_AS the same way D_TRAIT_IS_DETECTED
// is sugar over D_TRAIT_IS_DETECTED_AS.
#define D_TRAIT_IS_SPECIALIZATION_OF(TRAIT_NAME,                              \
                                     TEMPLATE_NAME)                           \
    D_TRAIT_IS_SPECIALIZATION_OF_AS(TRAIT_NAME,                               \
                                    TEMPLATE_NAME,                            \
                                    std::true_type)                           \
    D_TRAIT_VALUE_BOOL(TRAIT_NAME)

// -----------------------------------------------------------------------------
// HAS_METHOD_OF_TYPE family (enable_if expressions, not trait generators)
// -----------------------------------------------------------------------------
// These are a different shape from the D_TRAIT_* family - they expand to
// `std::enable_if_t<...>` (or a bool value) rather than to a struct
// definition. They are kept here for now to consolidate all SFINAE machinery
// in one section; consider folding them into the D_TRAIT_* family in a
// follow-up pass.

// HAS_METHOD_OF_TYPE
//   macro: SFINAE-guard expression for methods without arguments.
// Yields void if `_Type::method_name()` returns exactly `return_type`,
// otherwise substitution fails.
#define HAS_METHOD_OF_TYPE(method_name, return_type)                          \
    std::enable_if_t<std::is_same_v<                                          \
        decltype(std::declval<_Type>().method_name()),                        \
        return_type>                                                          \
    >

// HAS_METHOD_OF_TYPE_ARGS
//   macro: SFINAE-guard expression for methods with arguments.
#define HAS_METHOD_OF_TYPE_ARGS(method_name, return_type, ...)                \
    std::enable_if_t<std::is_same_v<                                          \
        decltype(std::declval<_Type>().method_name(__VA_ARGS__)),             \
        return_type>>

// HAS_METHOD_OF_TYPE_ARGS_V
//   macro: bool-valued version for methods with arguments.
#define HAS_METHOD_OF_TYPE_ARGS_V(T, method_name, return_type, ...)           \
    std::is_same_v<                                                           \
        decltype(std::declval<T>().method_name(__VA_ARGS__)),                 \
        return_type                                                           \
    >

// HAS_METHOD_OF_TYPE_V
//   macro: bool-valued version for methods without arguments.
#define HAS_METHOD_OF_TYPE_V(T, method_name, return_type)                     \
std::is_same_v<decltype(std::declval<T>().method_name()), return_type>

// =============================================================================
// I.   PORTABLE STANDARD-LIBRARY TRAITS
// =============================================================================
// Portable implementations of standard type traits across supported C++
// versions. Anything that requires only `void_t` (and not the full SFINAE
// detection machinery in section 0) lives here.

NS_DJINTERP

// -----------------------------------------------------------------------------
// I.1  Logical metafunctions (bool_constant, conjunction, disjunction,
//      negation)
// -----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

using std::bool_constant;

using std::conjunction;
using std::conjunction_v;
using std::disjunction;
using std::disjunction_v;
using std::negation;
using std::negation_v;

#elif D_ENV_LANG_IS_CPP11_OR_HIGHER

// -------------------------------------------------------------------------
// bool_constant
// -------------------------------------------------------------------------

// bool_constant
//   alias template: integral_constant<bool, B> helper for boolean traits.
template<bool _Value>
using bool_constant = std::integral_constant<bool, _Value>;

// -------------------------------------------------------------------------
// conjunction
// -------------------------------------------------------------------------

// conjunction
//   trait: logical AND of type traits. Inherits from first false trait or
// last trait if all are true.
template<typename...>
struct conjunction : std::true_type
{};

template<typename _B1>
struct conjunction<_B1> : _B1
{};

template<typename _B1,
         typename... _Bn>
struct conjunction<_B1, _Bn...>
    : std::conditional<bool(_B1::value), conjunction<_Bn...>, _B1>::type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // conjunction_v
    //   variable template: value of conjunction<_Bn...>.
    template<typename... _Bn>
    constexpr bool conjunction_v = conjunction<_Bn...>::value;
#endif

// -------------------------------------------------------------------------
// disjunction
// -------------------------------------------------------------------------

// disjunction
//   trait: logical OR of type traits. Inherits from first true trait or
// last trait if all are false.
template<typename...>
struct disjunction : std::false_type
{};

template<typename _B1>
struct disjunction<_B1> : _B1
{};

template<typename _B1,
         typename... _Bn>
struct disjunction<_B1, _Bn...>
    : std::conditional<bool(_B1::value), _B1, disjunction<_Bn...>>::type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // disjunction_v
    //   variable template: value of disjunction<_Bn...>.
    template<typename... _Bn>
    constexpr bool disjunction_v = disjunction<_Bn...>::value;
#endif

// -------------------------------------------------------------------------
// negation
// -------------------------------------------------------------------------

// negation
//   trait: logical NOT of a type trait.
template<typename _B>
struct negation : bool_constant<!bool(_B::value)>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // negation_v
    //   variable template: value of negation<_B>.
    template<typename _B>
    constexpr bool negation_v = negation<_B>::value;
#endif

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER / D_ENV_LANG_IS_CPP11_OR_HIGHER


// -----------------------------------------------------------------------------
// I.2  Callable traits (C++17, with C++11 fallbacks)
// -----------------------------------------------------------------------------
//   - invoke_result, invoke_result_t
//   - is_invocable, is_invocable_r
//   - is_nothrow_invocable, is_nothrow_invocable_r

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using std::invoke_result;
    using std::invoke_result_t;

    using std::is_invocable;
    using std::is_invocable_r;
    using std::is_invocable_r_v;
    using std::is_invocable_v;
    using std::is_nothrow_invocable;
    using std::is_nothrow_invocable_r;
    using std::is_nothrow_invocable_r_v;
    using std::is_nothrow_invocable_v;
#elif D_ENV_LANG_IS_CPP11_OR_HIGHER

// -------------------------------------------------------------------------
// invoke_result
// -------------------------------------------------------------------------

// invoke_result
//   trait: deduces the return type of an INVOKE expression at compile time.
// Uses result_of as fallback in C++11/14.
template<typename _Fn,
         typename... _Args>
struct invoke_result : std::result_of<_Fn(_Args...)>
{};

// invoke_result_t
//   alias template: shorthand for invoke_result<...>::type.
template<typename _Fn,
         typename... _Args>
using invoke_result_t = typename invoke_result<_Fn, _Args...>::type;

// -------------------------------------------------------------------------
// is_invocable family
// -------------------------------------------------------------------------

NS_INTERNAL
    // is_invocable_helper
    //   helper: SFINAE-based detection of invocability.
    template<typename _Fn,
             typename... _Args>
    struct is_invocable_helper
    {
    private:
        template<typename _F,
                 typename... _As>
        static auto test(int) -> decltype(
            std::declval<_F>()(std::declval<_As>()...),
            std::true_type{}
        );

        template<typename,
                 typename...>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Fn, _Args...>(0));
    };

    // is_invocable_r_helper
    //   helper: SFINAE-based detection of invocability with return type check.
    template<typename _Ret,
             typename _Fn,
             typename... _Args>
    struct is_invocable_r_helper
    {
    private:
        template<typename _R,
                 typename _F,
                 typename... _As>
        static auto test(int) -> typename std::enable_if<
            std::is_convertible<
                decltype(std::declval<_F>()(std::declval<_As>()...)),
                _R
            >::value,
            std::true_type
        >::type;

        template<typename,
                 typename,
                 typename...>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Ret, _Fn, _Args...>(0));
    };
NS_END  // internal

// is_invocable
//   trait: determines if _Fn can be invoked with _Args...
template<typename _Fn,
         typename... _Args>
struct is_invocable : internal::is_invocable_helper<_Fn, _Args...>::type
{};

// is_invocable_r
//   trait: determines if _Fn can be invoked with _Args... and return
// type is convertible to _Ret.
template<typename _Ret,
         typename _Fn,
         typename... _Args>
struct is_invocable_r : internal::is_invocable_r_helper<_Ret, _Fn, _Args...>::type
{};

// is_nothrow_invocable
//   trait: determines if _Fn can be invoked with _Args... without throwing.
template<typename _Fn,
         typename... _Args>
struct is_nothrow_invocable : bool_constant<
    is_invocable<_Fn, _Args...>::value &&
    noexcept(std::declval<_Fn>()(std::declval<_Args>()...))
>
{};

// is_nothrow_invocable_r
//   trait: determines if _Fn can be invoked with _Args... without throwing
// and return type is convertible to _Ret.
template<typename _Ret,
         typename _Fn,
         typename... _Args>
struct is_nothrow_invocable_r : bool_constant<
    is_invocable_r<_Ret, _Fn, _Args...>::value &&
    noexcept(std::declval<_Fn>()(std::declval<_Args>()...))
>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_invocable_r_v
    //   variable template: value of is_invocable_r<_Ret, _Fn, _Args...>.
    template<typename _Ret,
             typename _Fn,
             typename... _Args>
    constexpr bool is_invocable_r_v = is_invocable_r<_Ret, _Fn, _Args...>::value;

    // is_invocable_v
    //   variable template: value of is_invocable<_Fn, _Args...>.
    template<typename _Fn,
             typename... _Args>
    constexpr bool is_invocable_v = is_invocable<_Fn, _Args...>::value;

    // is_nothrow_invocable_r_v
    //   variable template: value of is_nothrow_invocable_r<_Ret, _Fn, _Args...>.
    template<typename _Ret,
             typename _Fn,
             typename... _Args>
    constexpr bool is_nothrow_invocable_r_v =
        is_nothrow_invocable_r<_Ret, _Fn, _Args...>::value;

    // is_nothrow_invocable_v
    //   variable template: value of is_nothrow_invocable<_Fn, _Args...>.
    template<typename _Fn,
             typename... _Args>
    constexpr bool is_nothrow_invocable_v =
        is_nothrow_invocable<_Fn, _Args...>::value;
#endif

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER / D_ENV_LANG_IS_CPP11_OR_HIGHER


// -----------------------------------------------------------------------------
// I.3  C++20 features (with C++11 fallbacks)
// -----------------------------------------------------------------------------
//   - is_bounded_array, is_unbounded_array
//   - remove_cvref, remove_cvref_t
//   - type_identity, type_identity_t

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    using std::is_bounded_array;
    using std::is_bounded_array_v;
    using std::is_unbounded_array;
    using std::is_unbounded_array_v;

    using std::remove_cvref;
    using std::remove_cvref_t;

    using std::type_identity;
    using std::type_identity_t;

#elif D_ENV_LANG_IS_CPP11_OR_HIGHER

// -------------------------------------------------------------------------
// is_bounded_array
// -------------------------------------------------------------------------

// is_bounded_array
//   trait: checks if a type is an array type with known bound.
template<typename _Type>
struct is_bounded_array : std::false_type
{};

template<typename    _Type,
            std::size_t _N>
struct is_bounded_array<_Type[_N]> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_bounded_array_v
    //   variable template: value of is_bounded_array<_Type>.
    template<typename _Type>
    constexpr bool is_bounded_array_v = is_bounded_array<_Type>::value;
#endif

// -------------------------------------------------------------------------
// is_unbounded_array
// -------------------------------------------------------------------------

// is_unbounded_array
//   trait: checks if a type is an array type with unknown bound.
template<typename _Type>
struct is_unbounded_array : std::false_type
{};

template<typename _Type>
struct is_unbounded_array<_Type[]> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_unbounded_array_v
    //   variable template: value of is_unbounded_array<_Type>.
    template<typename _Type>
    constexpr bool is_unbounded_array_v = is_unbounded_array<_Type>::value;
#endif

// -------------------------------------------------------------------------
// remove_cvref
// -------------------------------------------------------------------------

// remove_cvref
//   trait: removes const, volatile, and reference qualifiers from a type.
template<typename _Type>
struct remove_cvref
{
    using type = typename std::remove_cv<typename std::remove_reference<_Type>::type>::type;
};

// remove_cvref_t
//   alias template: shorthand for remove_cvref<_Type>::type.
template<typename _Type>
using remove_cvref_t = typename remove_cvref<_Type>::type;

// -------------------------------------------------------------------------
// type_identity
// -------------------------------------------------------------------------

// type_identity
//   trait: provides a member typedef `type` that names _Type unchanged.
// Useful to establish non-deduced contexts in template argument deduction.
template<typename _Type>
struct type_identity
{
    using type = _Type;
};

// type_identity_t
//   alias template: shorthand for type_identity<_Type>::type.
template<typename _Type>
using type_identity_t = typename type_identity<_Type>::type;

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER / D_ENV_LANG_IS_CPP11_OR_HIGHER


// -----------------------------------------------------------------------------
// I.4  C++23 features (with C++11 fallbacks)
// -----------------------------------------------------------------------------
//   - is_scoped_enum

#if D_ENV_LANG_IS_CPP23_OR_HIGHER
    using std::is_scoped_enum;
    using std::is_scoped_enum_v;

#elif D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_INTERNAL
    // is_scoped_enum_helper
    //   helper: a scoped enum is_enum but NOT implicitly convertible to int.
    template<typename _Type,
                bool = std::is_enum<_Type>::value>
    struct is_scoped_enum_helper : std::false_type
    {};

    template<typename _Type>
    struct is_scoped_enum_helper<_Type, true>
        : bool_constant<!std::is_convertible<_Type, int>::value>
    {};
NS_END  // internal

// is_scoped_enum
//   trait: checks if a type is a scoped enumeration (enum class/struct).
template<typename _Type>
struct is_scoped_enum : internal::is_scoped_enum_helper<_Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_scoped_enum_v
    //   variable template: value of is_scoped_enum<_Type>.
    template<typename _Type>
    constexpr bool is_scoped_enum_v = is_scoped_enum<_Type>::value;
#endif

#endif  // D_ENV_LANG_IS_CPP23_OR_HIGHER / D_ENV_LANG_IS_CPP11_OR_HIGHER


// -----------------------------------------------------------------------------
// I.5  C++17 _v aliases (provided for C++14 when C++17 isn't available)
// -----------------------------------------------------------------------------
// Variable template aliases for standard type traits.

#if ( D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES &&                            \
      !D_ENV_LANG_IS_CPP17_OR_HIGHER )

// Primary type categories (alphabetized)
template<typename _Type>
constexpr bool is_array_v = std::is_array<_Type>::value;

template<typename _Type>
constexpr bool is_class_v = std::is_class<_Type>::value;

template<typename _Type>
constexpr bool is_enum_v = std::is_enum<_Type>::value;

template<typename _Type>
constexpr bool is_floating_point_v = std::is_floating_point<_Type>::value;

template<typename _Type>
constexpr bool is_function_v = std::is_function<_Type>::value;

template<typename _Type>
constexpr bool is_integral_v = std::is_integral<_Type>::value;

template<typename _Type>
constexpr bool is_lvalue_reference_v = std::is_lvalue_reference<_Type>::value;

template<typename _Type>
constexpr bool is_member_function_pointer_v = std::is_member_function_pointer<_Type>::value;

template<typename _Type>
constexpr bool is_member_object_pointer_v = std::is_member_object_pointer<_Type>::value;

template<typename _Type>
constexpr bool is_null_pointer_v = std::is_null_pointer<_Type>::value;

template<typename _Type>
constexpr bool is_pointer_v = std::is_pointer<_Type>::value;

template<typename _Type>
constexpr bool is_rvalue_reference_v = std::is_rvalue_reference<_Type>::value;

template<typename _Type>
constexpr bool is_union_v = std::is_union<_Type>::value;

template<typename _Type>
constexpr bool is_void_v = std::is_void<_Type>::value;

// Composite type categories (alphabetized)
template<typename _Type>
constexpr bool is_arithmetic_v = std::is_arithmetic<_Type>::value;

template<typename _Type>
constexpr bool is_compound_v = std::is_compound<_Type>::value;

template<typename _Type>
constexpr bool is_fundamental_v = std::is_fundamental<_Type>::value;

template<typename _Type>
constexpr bool is_member_pointer_v = std::is_member_pointer<_Type>::value;

template<typename _Type>
constexpr bool is_object_v = std::is_object<_Type>::value;

template<typename _Type>
constexpr bool is_reference_v = std::is_reference<_Type>::value;

template<typename _Type>
constexpr bool is_scalar_v = std::is_scalar<_Type>::value;

// Type properties (alphabetized)
template<typename _Type>
constexpr bool is_abstract_v = std::is_abstract<_Type>::value;

template<typename _Type>
constexpr bool is_const_v = std::is_const<_Type>::value;

template<typename _Type>
constexpr bool is_empty_v = std::is_empty<_Type>::value;

template<typename _Type>
constexpr bool is_final_v = std::is_final<_Type>::value;

template<typename _Type>
constexpr bool is_polymorphic_v = std::is_polymorphic<_Type>::value;

template<typename _Type>
constexpr bool is_signed_v = std::is_signed<_Type>::value;

template<typename _Type>
constexpr bool is_standard_layout_v = std::is_standard_layout<_Type>::value;

template<typename _Type>
constexpr bool is_trivial_v = std::is_trivial<_Type>::value;

template<typename _Type>
constexpr bool is_trivially_copyable_v = std::is_trivially_copyable<_Type>::value;

template<typename _Type>
constexpr bool is_unsigned_v = std::is_unsigned<_Type>::value;

template<typename _Type>
constexpr bool is_volatile_v = std::is_volatile<_Type>::value;

// Supported operations - constructible (alphabetized)
template<typename _Type>
constexpr bool is_copy_constructible_v = std::is_copy_constructible<_Type>::value;

template<typename _Type>
constexpr bool is_default_constructible_v = std::is_default_constructible<_Type>::value;

template<typename _Type>
constexpr bool is_move_constructible_v = std::is_move_constructible<_Type>::value;

template<typename _Type>
constexpr bool is_trivially_copy_constructible_v =
    std::is_trivially_copy_constructible<_Type>::value;

template<typename _Type>
constexpr bool is_trivially_default_constructible_v =
    std::is_trivially_default_constructible<_Type>::value;

template<typename _Type>
constexpr bool is_trivially_move_constructible_v =
    std::is_trivially_move_constructible<_Type>::value;

template<typename _Type>
constexpr bool is_nothrow_copy_constructible_v =
    std::is_nothrow_copy_constructible<_Type>::value;

template<typename _Type>
constexpr bool is_nothrow_default_constructible_v =
    std::is_nothrow_default_constructible<_Type>::value;

template<typename _Type>
constexpr bool is_nothrow_move_constructible_v =
    std::is_nothrow_move_constructible<_Type>::value;

// Supported operations - assignable (alphabetized)
template<typename _Type>
constexpr bool is_copy_assignable_v = std::is_copy_assignable<_Type>::value;

template<typename _Type>
constexpr bool is_move_assignable_v = std::is_move_assignable<_Type>::value;

template<typename _Type>
constexpr bool is_nothrow_copy_assignable_v =
    std::is_nothrow_copy_assignable<_Type>::value;

template<typename _Type>
constexpr bool is_nothrow_move_assignable_v =
    std::is_nothrow_move_assignable<_Type>::value;

template<typename _Type>
constexpr bool is_trivially_copy_assignable_v =
    std::is_trivially_copy_assignable<_Type>::value;

template<typename _Type>
constexpr bool is_trivially_move_assignable_v =
    std::is_trivially_move_assignable<_Type>::value;

// Supported operations - destructible (alphabetized)
template<typename _Type>
constexpr bool is_destructible_v = std::is_destructible<_Type>::value;

template<typename _Type>
constexpr bool is_nothrow_destructible_v =
    std::is_nothrow_destructible<_Type>::value;

template<typename _Type>
constexpr bool is_trivially_destructible_v =
    std::is_trivially_destructible<_Type>::value;

// Type relationships (alphabetized)
template<typename _Base,
         typename _Derived>
constexpr bool is_base_of_v = std::is_base_of<_Base, _Derived>::value;

template<typename _From,
         typename _To>
constexpr bool is_convertible_v = std::is_convertible<_From, _To>::value;

template<typename _Type1,
         typename _Type2>
constexpr bool is_same_v = std::is_same<_Type1, _Type2>::value;

// Property queries (alphabetized)
template<typename _Type>
constexpr std::size_t alignment_of_v = std::alignment_of<_Type>::value;

template<typename _Type,
            unsigned _N = 0>
constexpr std::size_t extent_v = std::extent<_Type, _N>::value;

template<typename _Type>
constexpr std::size_t rank_v = std::rank<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES && 
    // !D_ENV_LANG_IS_CPP17_OR_HIGHER


// =============================================================================
// II.  PORTABLE LOGICAL METAFUNCTION MACROS
// =============================================================================
// Convenience macros for the logical metafunctions in section I.1. Resolve to
// std:: or djinterp:: based on C++ version. The SFINAE / detection macros
// live in section 0.3, not here.

// D_CONJUNCTION / D_DISJUNCTION / D_NEGATION
//   macros: portable logical metafunctions.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
#define D_CONJUNCTION std::conjunction
#define D_DISJUNCTION std::disjunction
#define D_NEGATION    std::negation
#elif D_ENV_LANG_IS_CPP11_OR_HIGHER
#define D_CONJUNCTION djinterp::conjunction
#define D_DISJUNCTION djinterp::disjunction
#define D_NEGATION    djinterp::negation
#endif


// =============================================================================
// III. CUSTOM TYPE TRAITS
// =============================================================================
// Custom type traits to supplement those found in std header <type_traits>.
// Note: is_tuple_homogeneous is in tuple_manip.hpp

// -------------------------------------------------------------------------
// first_arg
// -------------------------------------------------------------------------

// first_arg
//   trait: given a parameter pack, extracts the first parameter.
// Member alias `type` is the type of the first argument in the pack.
template<typename... _Types>
struct first_arg;

template<typename _Type>
struct first_arg<_Type>
{
    using type = _Type;
};

template<typename    _Type,
         typename... _Types>
struct first_arg<_Type, _Types...>
{
    using type = _Type;
};

// first_arg_t
//   type: convenience alias for `first_arg<_Types...>::type`.
template<typename... _Types>
using first_arg_t = typename first_arg<_Types...>::type;

// -------------------------------------------------------------------------
// is_tuple
// -------------------------------------------------------------------------

// is_tuple
//   trait: evaluates to `std::true_type` if `_Type` is a `std::tuple`,
// otherwise `std::false_type`.
D_TRAIT_IS_SPECIALIZATION_OF(is_tuple, std::tuple)

// -------------------------------------------------------------------------
// is_single_tuple_arg
// -------------------------------------------------------------------------

// is_single_tuple_arg
//   trait: evaluates to `std::true_type` if the parameter pack consists of
// exactly one argument that is itself a `std::tuple`.
//
//   The empty-pack case is handled by an explicit specialization below
// the primary template.  Without it, the primary's
//     std::conditional<(sizeof...(_Types) == 1),
//                      is_tuple<tuple_element<0, tuple<_Types...>>::type>,
//                      std::false_type>::type
// dispatch would be ill-formed for `_Types = <>`: std::conditional is
// not lazy, so both branches are instantiated as template arguments
// before the selection happens, and the `true` branch reduces to
// `tuple_element<0, std::tuple<>>`, which trips the standard library's
// "tuple index out of bounds" static_assert.  The specialization
// short-circuits the empty-pack case to std::false_type so the primary
// is never instantiated for it.
template<typename... _Types>
struct is_single_tuple_arg : std::conditional<
    (sizeof...(_Types) == 1),
    is_tuple<typename std::tuple_element<0, std::tuple<_Types...>>::type>,
    std::false_type
>::type
{};

// is_single_tuple_arg<>
//   specialization: empty pack -> false_type.  Required to avoid the
// ill-formed tuple_element<0, std::tuple<>> instantiation that the
// primary template's std::conditional would otherwise produce; see
// the primary's commentary above.
template<>
struct is_single_tuple_arg<> : std::false_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_single_tuple_arg_v
//   value: convenience alias for `is_single_tuple_arg<_Types...>::value`.
template<typename... _Types>
constexpr bool is_single_tuple_arg_v = is_single_tuple_arg<_Types...>::value;
#endif

// -------------------------------------------------------------------------
// to_tuple
// -------------------------------------------------------------------------

// to_tuple
//   trait: normalizes a parameter pack to a `std::tuple`.
// If the parameter pack consists of a single `std::tuple` type, `to_tuple`
// resolves to that tuple type directly. Otherwise, all arguments are
// wrapped in a new `std::tuple`.
template<typename... _Types>
struct to_tuple
{
    using type = typename std::conditional<
        is_single_tuple_arg<_Types...>::value,
        typename first_arg<_Types...>::type,
        std::tuple<_Types...>
    >::type;
};

// to_tuple (empty-pack specialization)
//   The primary template, when instantiated with an empty pack,
// evaluates `std::conditional` - which eagerly instantiates BOTH
// branches.  The "true" branch `first_arg<>::type` is undefined
// (first_arg has no empty-pack specialization), so the program
// becomes ill-formed whenever `to_tuple<>` is even named in a
// full-expression, not just when its `::type` is requested.
//
// Consumers that touch `to_tuple<>` include `tuple_apply_all` and
// `make_tuple_of` with a pack starting empty, so providing this
// specialization is the minimal fix that preserves the empty-
// sequence identity `to_tuple<>::type == std::tuple<>` without
// reworking the general template.
template<>
struct to_tuple<>
{
    using type = std::tuple<>;
};

template<typename _Type>
struct to_tuple<_Type>
{
    using type = typename std::conditional<
        is_tuple<_Type>::value,
        _Type,
        std::tuple<_Type>
    >::type;
};

// to_tuple_t
//   type: convenience alias for `to_tuple<_Types...>::type`.
template<typename... _Types>
using to_tuple_t = typename to_tuple<_Types...>::type;

// -------------------------------------------------------------------------
// evaluate_types_for_trait
// -------------------------------------------------------------------------

NS_INTERNAL
    // evaluate_all_for_trait_helper
    //   helper: recursively applies a unary trait to all types in a tuple.
    template<typename                       _Tuple, 
             template<typename> typename    _UnaryTrait,
             template<typename...> typename _Evaluator = std::conjunction>
    struct evaluate_all_for_trait_helper;

    template<typename                       _Type,
             template<typename> typename    _UnaryTrait,
             template<typename...> typename _Evaluator>
    struct evaluate_all_for_trait_helper<std::tuple<_Type>, _UnaryTrait, _Evaluator> 
        : std::true_type
    {};

    template<typename                       _Type, 
             typename...                    _Types, 
             template<typename> typename    _UnaryTrait,
             template<typename...> typename _Evaluator>
    struct evaluate_all_for_trait_helper<std::tuple<_Type, _Types...>, _UnaryTrait, _Evaluator>
    {
        static constexpr bool value = 
            _Evaluator<_UnaryTrait<_Type>, _UnaryTrait<_Types>...>::value;
    };
NS_END  // internal

// evaluate_types_for_trait
//   trait: evaluates all types in a parameter pack or `std::tuple` for a 
// unary type trait using a specified evaluator (default: conjunction).
template<typename                       _Tuple,
         template<typename> typename    _UnaryTrait,
         template<typename...> typename _Evaluator = std::conjunction>
struct evaluate_types_for_trait
{
    static constexpr bool value = 
        internal::evaluate_all_for_trait_helper<
         typename to_tuple<_Tuple>::type, 
            _UnaryTrait, 
            _Evaluator
        >::value;
};

template<typename...                    _Types,
         template<typename> typename    _UnaryTrait,
         template<typename...> typename _Evaluator>
struct evaluate_types_for_trait<std::tuple<_Types...>, _UnaryTrait, _Evaluator>
{
    static constexpr bool value = 
        internal::evaluate_all_for_trait_helper<
         typename to_tuple<_Types...>::type, 
            _UnaryTrait, 
            _Evaluator
        >::value;
};

// evaluate_types_for_trait_v
//   variable template: value of evaluate_types_for_trait<...>.
template<typename                       _Tuple,
         template<typename> typename    _UnaryTrait,
         template<typename...> typename _Evaluator>
constexpr bool evaluate_types_for_trait_v = 
    evaluate_types_for_trait<typename to_tuple<_Tuple>::type, _UnaryTrait, std::conjunction>::value;

// -------------------------------------------------------------------------
// are_all_nonvoid
// -------------------------------------------------------------------------

// are_all_nonvoid
//   trait: evaluates whether all types in a parameter pack are non-void.
template<typename... _Types>
struct are_all_nonvoid 
    : std::negation<evaluate_types_for_trait<
        typename to_tuple<_Types...>::type, 
        std::is_void, 
        std::disjunction>>
{};

template<typename _Type>
struct are_all_nonvoid<_Type> 
    : evaluate_types_for_trait<
        typename to_tuple<_Type>::type, 
        std::is_void, 
        std::disjunction>
{}; 

// are_all_nonvoid_v
//   variable template: value of are_all_nonvoid<_Types...>.
template<typename... _Types>
inline constexpr bool are_all_nonvoid_v = are_all_nonvoid<_Types...>::value;

// -------------------------------------------------------------------------
// exclusive_disjunction
// -------------------------------------------------------------------------

// exclusive_disjunction
//   trait: forms the exclusive logical disjunction (XOR) of the type traits
// `_Bs...`, effectively performing a logical XOR on the sequence of traits.
template<typename... _Bs>
struct exclusive_disjunction : std::false_type 
{};

template<typename _B1>
struct exclusive_disjunction<_B1> : _B1
{};

template<typename _B1, 
         typename _B2>
struct exclusive_disjunction<_B1, _B2> 
    : std::integral_constant<bool, _B1::value != _B2::value>
{};

template<typename    _B1,
         typename    _B2, 
         typename... _Bs>
struct exclusive_disjunction<_B1, _B2, _Bs...> 
    : std::integral_constant<bool, 
        (_B1::value != _B2::value) && exclusive_disjunction<_Bs...>::value>
{};

// exclusive_disjunction_v
//   variable template: value of exclusive_disjunction<_Bs...>.
template<typename... _Bs>
inline constexpr bool exclusive_disjunction_v = exclusive_disjunction<_Bs...>::value;

// -------------------------------------------------------------------------
// follows_rule_of_five
// -------------------------------------------------------------------------

// follows_rule_of_five
//   trait: returns true if, and only if, class `_Type` follows the 
// Rule of Five for class definitions: that the class'
//   1. copy constructor
//   2. move constructor
//   3. copy assignment operator
//   4. move assignment operator
//   5. destructor
// are all defined.
D_TRAIT_IS_DETECTED(follows_rule_of_five,
    // copy constructor
    decltype(_Type(std::declval<const _Type&>())),
    // move constructor
    decltype(_Type(std::declval<_Type&&>())),
    // copy assignment with correct return type
    std::enable_if_t<std::is_same<
        decltype(std::declval<_Type&>() = std::declval<const _Type&>()),
        _Type&>::value>,
    // move assignment with correct return type
    std::enable_if_t<std::is_same<
        decltype(std::declval<_Type&>() = std::declval<_Type&&>()),
        _Type&>::value>)

// -------------------------------------------------------------------------
// follows_rule_of_three
// -------------------------------------------------------------------------

// follows_rule_of_three
//   trait: returns true if, and only if, class `_Type` follows the 
// Rule of Three for class definitions: that the class'
//   1. copy constructor
//   2. copy assignment operator
//   3. destructor
// are all defined.
D_TRAIT_IS_DETECTED(follows_rule_of_three,
    // copy constructor exists
    decltype(_Type(std::declval<const _Type&>())),
    // copy assignment exists and returns reference
    std::enable_if_t<std::is_same<
        decltype(std::declval<_Type&>() = std::declval<const _Type&>()),
        _Type&>::value>)

// -------------------------------------------------------------------------
// follows_rule_of_zero
// -------------------------------------------------------------------------

// follows_rule_of_zero
//   trait: returns true if, and only if, class `_Type` follows the 
// Rule of Zero for class definitions: that all special member functions 
//   1. copy constructor
//   2. move constructor
//   3. copy assignment operator
//   4. move assignment operator
//   5. destructor
// are trivially implemented by the compiler.
D_TRAIT_IS_DETECTED(follows_rule_of_zero,
    // trivial copy constructor
    std::enable_if_t<std::is_trivially_copy_constructible<_Type>::value>,
    // trivial move constructor  
    std::enable_if_t<std::is_trivially_move_constructible<_Type>::value>,
    // trivial copy assignment
    std::enable_if_t<std::is_trivially_copy_assignable<_Type>::value>,
    // trivial move assignment
    std::enable_if_t<std::is_trivially_move_assignable<_Type>::value>,
    // trivial destructor
    std::enable_if_t<std::is_trivially_destructible<_Type>::value>)

// -------------------------------------------------------------------------
// has_max_size
// -------------------------------------------------------------------------

// has_max_size
//   trait: determines if a type has both a `size_type` alias, and a
// `max_size` constexpr corresponding to that particular type.
D_TRAIT_IS_DETECTED(has_max_size,
    typename _Type::size_type,
    decltype(_Type::max_size),
    std::enable_if_t<std::is_same<
        decltype(_Type::max_size), 
        const typename _Type::size_type
    >::value>)

// -------------------------------------------------------------------------
// has_nested_template_type
// -------------------------------------------------------------------------

// has_nested_template_type
//   trait: determines if a type has a nested template alias named `type`.
D_TRAIT_IS_DETECTED(has_nested_template_type,
    typename _Type::template type<int>)

// -------------------------------------------------------------------------
// has_variadic_constructor
// -------------------------------------------------------------------------

// has_variadic_constructor
//   trait: determines if a type has a constructor that accepts variadic 
// arguments (specifically, can be constructed from itself).
D_TRAIT_IS_DETECTED(has_variadic_constructor,
    decltype(_Type(std::declval<_Type>())))

// -------------------------------------------------------------------------
// is_allocator
// -------------------------------------------------------------------------

// is_allocator
//   trait: determines if a type satisfies the Allocator requirements by
// checking for value_type, allocate(), and deallocate() members.
D_TRAIT_IS_DETECTED(is_allocator,
    typename std::allocator_traits<_Type>::value_type,
    decltype(std::allocator_traits<_Type>::allocate(
        std::declval<_Type>(), std::size_t{})),
    decltype(std::allocator_traits<_Type>::deallocate(
        std::declval<_Type>(), nullptr, std::size_t{})))

// -------------------------------------------------------------------------
// is_bounded
// -------------------------------------------------------------------------

// is_bounded
//   trait: determines if a type satisfies a given trait and has a max_size
// member, indicating it has a bounded capacity.
template<typename                    _Type, 
         template<typename...> class _Trait>
struct is_bounded
    : D_CONJUNCTION<_Trait<_Type>, has_max_size<_Type>>
{};

// is_bounded_v
//   variable template: value of is_bounded<_Type, _Trait>.
template<typename _Type, 
         template<typename...> class _Trait>
inline constexpr bool is_bounded_v = is_bounded<_Type, _Trait>::value;

// -------------------------------------------------------------------------
// is_nonvoid
// -------------------------------------------------------------------------

// is_nonvoid
//   trait: evaluates to true_type if _Type is not void.
template<typename _Type>
struct is_nonvoid : D_NEGATION<std::is_void<_Type>>
{};

// is_nonvoid_v
//   variable template: value of is_nonvoid<_Type>.
template<typename _Type>
inline constexpr bool is_nonvoid_v = is_nonvoid<_Type>::value;

// -------------------------------------------------------------------------
// is_nonzero / is_zero
// -------------------------------------------------------------------------

// is_nonzero
//   trait: evaluates to true_type if N is not zero.
template<std::size_t N>
using is_nonzero = std::negation<std::is_same<
    std::integral_constant<std::size_t, N>, 
    std::integral_constant<std::size_t, 0>
>>;

// is_nonzero_v
//   variable template: value of is_nonzero<N>.
template<std::size_t N>
inline constexpr bool is_nonzero_v = is_nonzero<N>::value;

// is_zero
//   trait: evaluates to true_type if N is zero.
template<std::size_t N>
using is_zero = std::is_same<
    std::integral_constant<std::size_t, N>, 
    std::integral_constant<std::size_t, 0>
>;

// -------------------------------------------------------------------------
// is_single_arg
// -------------------------------------------------------------------------

// is_single_arg
//   trait: evaluates to true_type if the parameter pack contains exactly 
// one type. Provides member type alias `type` for the single type.
template<typename... _Types>
struct is_single_arg : std::false_type
{};

template<typename _Type>
struct is_single_arg<_Type> : std::true_type
{
    using type = _Type;
};

// is_single_arg_v
//   variable template: value of is_single_arg<_Types...>.
template<typename... _Types>
inline constexpr bool is_single_arg_v = is_single_arg<_Types...>::value;

// -------------------------------------------------------------------------
// is_single_type_arg
// -------------------------------------------------------------------------

// is_single_type_arg
//   trait: returns true if, and only if, a parameter pack consists of a 
// single argument of type `_Type`.
template<typename    _Type,
         typename... _Types>
struct is_single_type_arg : std::conjunction<
        is_single_arg<_Types...>, 
        std::is_same<typename is_single_arg<_Types...>::type, _Type>
    >
{};

// is_single_type_arg_v
//   variable template: value of is_single_type_arg<_Type, _Types...>.
template<typename... _Types>
inline constexpr bool is_single_type_arg_v = is_single_type_arg<_Types...>::value;

// -------------------------------------------------------------------------
// is_sized
// -------------------------------------------------------------------------

// is_sized
//   trait: evaluates whether the given type has:
//   - a `size_type` type alias
//   - a size() member function
//   - both `size_type` and `size()` return types convertible to `std::size_t`.
template<typename,
         typename = void>
struct is_sized : std::false_type
{};

template<typename _Type>
struct is_sized<_Type, void_t<
    typename _Type::size_type,
    decltype(std::declval<const _Type&>().size())
>> : std::conjunction<
    std::is_convertible<decltype(std::declval<const _Type&>().size()), std::size_t>, 
    std::is_convertible<typename _Type::size_type, std::size_t>
>
{};

// is_sized_v
//   variable template: value of is_sized<_Type>.
template<typename _Type>
inline constexpr bool is_sized_v = is_sized<_Type>::value;

// -------------------------------------------------------------------------
// is_template
// -------------------------------------------------------------------------

// is_template
//   trait: evaluates a type for being a template template (i.e., `_Type` 
// is itself a template with parameters). Returns true for empty template
// instantiations.
template<typename> 
struct is_template : std::false_type
{};

template<template<typename...> typename _Type>
struct is_template<_Type<>> : std::true_type 
{};

// is_template_v
//   variable template: value of is_template<_Type>.
template<typename _Type>
inline constexpr bool is_template_v = is_template<_Type>::value;

// -------------------------------------------------------------------------
// is_template_parameter_base_of
// -------------------------------------------------------------------------

// is_template_parameter_base_of
//   trait: evaluates whether type `_Type` contains a `value_type` member 
// type alias that is itself a base of `_Type`. Useful for dealing with 
// polymorphic and composite-patterned class templates.
D_TRAIT_IS_DETECTED(is_template_parameter_base_of,
    typename _Type::value_type, 
    std::enable_if_t<std::is_base_of<typename _Type::value_type, _Type>::value>)

// -------------------------------------------------------------------------
// is_template_with_args
// -------------------------------------------------------------------------

// is_template_with_args
//   trait: evaluates whether `_Type` is a template instantiation with one 
// or more template arguments.
template<typename> 
struct is_template_with_args : std::false_type
{};

template<template<typename...> typename _Type,
         typename...                     _Args>
struct is_template_with_args<_Type<_Args...>> : std::true_type
{};

// is_template_with_args_v
//   variable template: value of is_template_with_args<_Type>.
template<typename _Type>
inline constexpr bool is_template_with_args_v = is_template_with_args<_Type>::value;

// -------------------------------------------------------------------------
// is_valid_size_type
// -------------------------------------------------------------------------

// is_valid_size_type
//   trait: returns true if type is valid for use as a size type
// (unsigned arithmetic type).
template<typename _Type>
struct is_valid_size_type
    : D_CONJUNCTION<std::is_unsigned<_Type>, std::is_arithmetic<_Type>>
{};

// is_valid_size_type_v
//   variable template: value of is_valid_size_type<_Type>.
template<typename _Type>
inline constexpr bool is_valid_size_type_v = is_valid_size_type<_Type>::value;

// ===========================================================================
// X.   Indexed pack access  (pack_element)
// ===========================================================================
//   Direct positional access into a typename parameter pack, without
// routing through std::tuple.  This is the primitive that pack-walking
// utilities (binary search, sorted lookup, get-like access) should
// build on, so the "recurse over the index, peel one head per step"
// pattern lives in exactly one place.
//
//   Relationship to tuple_type_at (dtuple.hpp): tuple_type_at is the
// TUPLE-facing accessor - it normalizes its input via to_tuple_t (so
// it accepts either a pack or a single std::tuple) and additionally
// exposes a runtime `value(tuple)` getter.  pack_element is the
// leaner PACK-facing core: no tuple normalization, no runtime getter,
// just the type at an index.  tuple_type_at may be expressed in terms
// of pack_element; pack_element never depends on tuple_type_at.

// pack_element
//   trait: the _Index-th type in the pack (0-based).  Out-of-range
// _Index is a hard error (the recursion runs off the end of the pack
// and fails to match the base case) - callers that may pass an
// invalid index should guard it (see pack_element_or in the bsearch
// module, or precede the access with a bounds check).
template<std::size_t  _Index,
         typename     _Head,
         typename...  _Tail>
struct pack_element
{
    using type = typename pack_element<_Index - 1, _Tail...>::type;
};

// base case: index 0 -> the current head
template<typename    _Head,
         typename... _Tail>
struct pack_element<0, _Head, _Tail...>
{
    using type = _Head;
};

// pack_element_t
//   alias: shorthand for pack_element<...>::type.
template<std::size_t _Index,
         typename... _Pack>
using pack_element_t = typename pack_element<_Index, _Pack...>::type;


NS_END  // djinterp


#endif  // DJINTERP_META_TYPE_TRAITS_