/******************************************************************************
* djinterp [test]                                              test_traits.hpp
*
*   The trait-TESTING toolkit: the shorthands and idioms a unit test needs in
* order to interrogate a SFINAE / detection trait, as opposed to the machinery
* used to WRITE one.  trait_detect.hpp emits traits; type_traits.hpp section
* 0.2 supplies the detection idiom they are built from; this header is the
* third corner - the vocabulary for asking whether the trait a suite is
* testing actually behaves like a trait.
*
*   WHY IT HAS TO BE ITS OWN THING:
*   A trait test cannot be written the way an ordinary test is written.  The
* negative case - "this expression is NOT valid for _Type" - cannot be spelled
* by writing the expression, because an ill-formed expression is a hard error,
* not a `false`.  Every honest negative therefore has to be routed through a
* SFINAE context first.  That routing is mechanical, it is easy to get subtly
* wrong, and it is currently re-derived by hand in every suite that touches a
* trait.  This header names it once.
*
*   WHAT A TRAIT TEST ACTUALLY ASKS:
*     1. is the probed expression well-formed for _Type?             (I, II)
*     2. and what does it yield - which type exactly, in which value
*        category, throwing or not, constant or not?                 (I, II)
*     3. does that answer hold across a SET of types?                (III)
*     4. is the trait ITSELF a well-formed bool trait - `::value`,
*        `value_type`, `::type`, and a `_v` companion that agrees?   (IV)
*     5. does it answer identically for `_Type`, `const _Type&`,
*        `_Type&&`, ...?                                             (V)
*     6. does it survive the types nobody thought to try it on?      (VI)
*     7. (C++20) does its concept face agree with it, and does the
*        refinement ladder genuinely subsume?                        (VIII)
*
*   USES, DOES NOT RE-SPELL:
*   The detection idiom already exists - is_detected, is_detected_exact,
* is_detected_convertible, detected_t, detected_or_t and nonesuch, all in
* type_traits.hpp section 0.2, all reachable unqualified from djinterp::test.
* Nothing here renames them; a suite asking "is this expression valid?" says
* is_detected, and a suite asking "does it yield exactly X?" says
* is_detected_exact.  What is added is only what those cannot express: the
* value-category / noexcept / constant-expression readings of a probe, the
* NON-short-circuiting quantifiers a test wants (conjunction stops at the
* first false and would therefore hide a later cell that does not compile),
* trait-shape conformance, the cv-ref agreement matrix, and the fixture zoo.
*
*   HARD FAILURE vs REPORTED FAILURE:
*   Almost everything here reduces to a `constexpr bool`, and that is a
* deliberate choice: a suite hands the bool to its own check macro, and a
* broken trait then shows up as a red line in the console and the PDF.
* D_TEST_STATIC (VII) is the opposite choice - it fails the BUILD.  Reach for
* it only where a regression should stop the line, because a dead build
* produces no report at all.
*
*   PORTABILITY:
*   C++17.  That floor is set by the detection idiom this header READS, not by
* anything written here: type_traits.hpp section 0.2 spells its template-
* template parameters `template<typename...> typename _Op`, which is C++17
* (P0522), notwithstanding the C++11 claim in its own comment block.  Beyond
* the floor: `_v` companions need variable templates (C++14+), is_valid's
* inline spelling needs generic lambdas (C++14+), and section VIII needs
* concepts (C++20).
*
*
* TABLE OF CONTENTS
* =================
* I.    PROBE DECLARATORS
* II.   READING A PROBE
* III.  TYPE-SET QUANTIFIERS
* IV.   TRAIT SHAPE
* V.    CV-REF AGREEMENT
* VI.   FIXTURES  (the type zoo)
* VII.  BUILD-TIME PINS
* VIII. CONCEPT LAYER  (C++20)
*
*
* path:      /inc/djinterp/test/test_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.12
******************************************************************************/

#ifndef DJINTERP_TEST_TRAITS_
#define DJINTERP_TEST_TRAITS_ 1

#ifndef __cplusplus
    #error "test_traits.hpp requires C++ compilation"
#endif  // __cplusplus

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/meta/trait_detect.hpp"   // D_VOID_T + the D_TYPE_TRAIT_* family
#include "../core/meta/type_traits.hpp"    // the detection idiom this header reads
#include "./test_common.hpp"


#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
    #error "test_traits.hpp requires C++17 or higher"
#endif  // !D_ENV_LANG_IS_CPP17_OR_HIGHER


#ifndef D_KEYWORD_FIXTURES
    #define D_KEYWORD_FIXTURES  fixtures
#endif  // D_KEYWORD_FIXTURES

#ifndef NS_FIXTURES
    #define NS_FIXTURES         D_NAMESPACE(D_KEYWORD_FIXTURES)
#endif  // NS_FIXTURES


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   PROBE DECLARATORS                                    ///
///////////////////////////////////////////////////////////////////////////////
//
//   A PROBE is an alias template whose instantiation is well-formed exactly
// when the thing under test is.  It is the unit of a trait test: the probe
// carries the question, and the queries in section II read the answer off it
// without ever making the compiler commit to the expression.
//
//   Four flavours, because there are four separate facts a probe can carry
// and no single spelling carries more than one of them:
//
//     D_TEST_TYPE_PROBE      is this TYPE expression well-formed?
//     D_TEST_EXPR_PROBE      is this EXPRESSION well-formed - and, since the
//                            probe is its decltype, what type and value
//                            category does it yield?
//     D_TEST_NOEXCEPT_PROBE  ...and is it non-throwing?
//     D_TEST_CONSTEXPR_PROBE ...and is it a CONSTANT expression?
//
//   Every declarator names its type parameter `_Type` (and `_Other`, for the
// binary forms), so the expression body is written against those names.  They
// are alias TEMPLATES, so they must be declared at namespace or class scope -
// never inside a test function.  For the one-off, in-function case there is
// is_valid (section II), which needs no declaration at all.
//
//   These macros sit at file scope in the preprocessor's eyes and are
// namespace-agnostic; the probe they emit lands in whatever namespace the
// macro is invoked in (normally the suite's own djinterp::testing).

// D_TEST_TYPE_PROBE
//   macro: names a one-parameter TYPE probe over `_Type`.  Instantiation is
// well-formed iff the type expression is.  This is the probe for nested
// typedefs, alias-template applications, and anything else that is a type
// rather than an expression.
//
// Usage:
//   D_TEST_TYPE_PROBE(p_value_type, typename _Type::value_type)
//   ...
//   is_detected<p_value_type, my_type>::value
#define D_TEST_TYPE_PROBE(PROBE_NAME, ...)                                    \
    template<typename _Type>                                                  \
    using PROBE_NAME = __VA_ARGS__;

// D_TEST_TYPE_PROBE_2
//   macro: the binary form of D_TEST_TYPE_PROBE, over `_Type` and `_Other`.
#define D_TEST_TYPE_PROBE_2(PROBE_NAME, ...)                                  \
    template<typename _Type,                                                  \
             typename _Other>                                                 \
    using PROBE_NAME = __VA_ARGS__;

// D_TEST_EXPR_PROBE
//   macro: names a one-parameter EXPRESSION probe over `_Type`.  The probe is
// the `decltype` of the expression, so a successful detection hands back not
// just "well-formed" but the exact result type - which is what lets
// is_detected_exact, is_detected_convertible and the yields_* family in
// section II say something sharper than yes/no.
//
//   Write the expression with std::declval so no object is required and the
// value category under test is stated explicitly: `declval<_Type&>()` for a
// mutable lvalue, `declval<const _Type&>()` for an immutable one,
// `declval<_Type>()` for an rvalue.
//
// Usage:
//   D_TEST_EXPR_PROBE(p_size, std::declval<const _Type&>().size())
//   ...
//   is_detected_convertible<std::size_t, p_size, my_type>::value
#define D_TEST_EXPR_PROBE(PROBE_NAME, ...)                                    \
    template<typename _Type>                                                  \
    using PROBE_NAME = decltype(__VA_ARGS__);

// D_TEST_EXPR_PROBE_2
//   macro: the binary form of D_TEST_EXPR_PROBE, over `_Type` and `_Other` -
// the shape every cross-type question wants (comparability, assignability,
// constructibility from, conversion to).
//
// Usage:
//   D_TEST_EXPR_PROBE_2(p_eq, std::declval<const _Type&>() ==
//                             std::declval<const _Other&>())
//   ...
//   is_detected<p_eq, my_type, other_type>::value
#define D_TEST_EXPR_PROBE_2(PROBE_NAME, ...)                                  \
    template<typename _Type,                                                  \
             typename _Other>                                                 \
    using PROBE_NAME = decltype(__VA_ARGS__);

// D_TEST_NOEXCEPT_PROBE
//   macro: names a probe that is well-formed iff the expression is, and whose
// `::value` reports whether that expression is non-throwing.  Two facts, one
// probe, and they must travel together: `noexcept(EXPR)` on an ill-formed
// EXPR is a hard error, so "is it noexcept" is only askable of an expression
// already known to be valid.  Wrapping the noexcept operator in the alias
// makes ill-formedness a substitution failure instead.
//
//   Read it with is_nothrow_probe (section II), which folds the two facts
// into the single bool a test wants: valid AND non-throwing.  To tell an
// ill-formed expression apart from a merely throwing one, pair it with a
// D_TEST_EXPR_PROBE over the same expression.
//
// Usage:
//   D_TEST_NOEXCEPT_PROBE(p_swap_nothrow,
//                         std::declval<_Type&>().swap(std::declval<_Type&>()))
//   ...
//   is_nothrow_probe<p_swap_nothrow, my_type>::value
#define D_TEST_NOEXCEPT_PROBE(PROBE_NAME, ...)                                \
    template<typename _Type>                                                  \
    using PROBE_NAME =                                                        \
        std::integral_constant<bool, noexcept(__VA_ARGS__)>;

// D_TEST_CONSTEXPR_PROBE
//   macro: names a probe that is well-formed iff the expression is a CONSTANT
// expression.  The expression is evaluated, discarded, and replaced by a
// literal `0` in a template-argument position - so it must survive the
// compiler's constant evaluator to get there, whatever its own type.  A
// non-constant expression fails in the immediate context of the substitution
// and is therefore detected as false rather than diagnosed.
//
//   Read it with is_detected, exactly like a type probe: it either forms or
// it does not.
//
//   Caveat: the probe conflates "ill-formed" with "well-formed but not
// constant".  Pair it with a D_TEST_EXPR_PROBE over the same expression when
// the difference matters - EXPR detected + CONSTEXPR not detected is
// precisely "compiles, but is not constexpr".
//
// Usage:
//   D_TEST_CONSTEXPR_PROBE(p_make_ce, make_test_kind(1, "k", 0, true))
//   ...
//   is_detected<p_make_ce, my_type>::value
#define D_TEST_CONSTEXPR_PROBE(PROBE_NAME, ...)                               \
    template<typename _Type>                                                  \
    using PROBE_NAME =                                                        \
        std::integral_constant<int, ((void)(__VA_ARGS__), 0)>;


///////////////////////////////////////////////////////////////////////////////
///                II.  READING A PROBE                                      ///
///////////////////////////////////////////////////////////////////////////////
//
//   The plain readings are already spelled, in type_traits.hpp section 0.2,
// and are used from here unchanged:
//
//     is_detected<P, T...>                is the probe well-formed?
//     is_detected_exact<X, P, T...>       ...and is its result exactly X?
//     is_detected_convertible<X, P, T...> ...and is its result convertible to X?
//     detected_t<P, T...>                 the result type, or `nonesuch`
//     detected_or_t<D, P, T...>           the result type, or D
//
//   What follows is only what those cannot say.

// is_nothrow_probe
//   trait: reads a D_TEST_NOEXCEPT_PROBE.  True iff the probed expression is
// BOTH well-formed for `_Types...` AND non-throwing.  An ill-formed
// expression reads as false rather than as a diagnostic, which is the whole
// point: it makes the negative case testable.
template<template<typename...> typename _Probe,
         typename...                    _Types>
struct is_nothrow_probe
    : detected_or_t<std::false_type, _Probe, _Types...>
{};

// yields_lvalue
//   trait: true iff the EXPRESSION probe is well-formed and its result is an
// lvalue - that is, `decltype(EXPR)` is `X&`.  The question every accessor
// test asks and no plain detection trait answers: `front()` returning a
// reference and `front()` returning a copy are both "detected".
template<template<typename...> typename _Probe,
         typename...                    _Types>
struct yields_lvalue
    : std::is_lvalue_reference<detected_t<_Probe, _Types...>>
{};

// yields_xvalue
//   trait: true iff the probe is well-formed and its result is an xvalue -
// `decltype(EXPR)` is `X&&`.
template<template<typename...> typename _Probe,
         typename...                    _Types>
struct yields_xvalue
    : std::is_rvalue_reference<detected_t<_Probe, _Types...>>
{};

// yields_prvalue
//   trait: true iff the probe is well-formed and its result is a prvalue -
// `decltype(EXPR)` is a non-reference.  The detection guard is load-bearing:
// on failure detected_t yields `nonesuch`, which is not a reference either,
// so a naive `!is_reference<detected_t<...>>` would report every ill-formed
// expression as a prvalue.
template<template<typename...> typename _Probe,
         typename...                    _Types>
struct yields_prvalue
    : bool_constant<
        ( is_detected<_Probe, _Types...>::value &&
          (!std::is_reference<detected_t<_Probe, _Types...>>::value) )>
{};


NS_INTERNAL

    // valid_call
    //   trait: the SFINAE dispatch behind is_valid.  The `int` overload
    // survives substitution only when `_Probe` is callable on `_Args...`; the
    // ellipsis overload is the fallback and is always a worse match for the
    // literal 0 the caller passes.
    //
    //   The result is fed through `void(...)` rather than a bare comma so a
    // probed type that overloads `operator,` (see fixtures::evil) cannot
    // hijack the dispatch.
    template<typename    _Probe,
             typename... _Args>
    D_CONSTEXPR auto
    valid_call(
        int _tag
    ) -> decltype(void(std::declval<_Probe>()(std::declval<_Args>()...)), true)
    {
        return ( (void)_tag, true );
    }

    // valid_call
    //   trait: the fallback overload - reached only when the call above fails
    // to substitute.
    template<typename    _Probe,
             typename... _Args>
    D_CONSTEXPR bool
    valid_call(
        ...
    )
    {
        return false;
    }

NS_END  // internal


// is_valid
//   function: true iff `_probe(std::declval<_Args>()...)` is a well-formed
// call.  This is the INLINE spelling of a probe - hand it a generic lambda
// whose trailing return type names the expression under test and it answers
// on the spot, with no alias template declared and no name added to the
// suite's namespace.  Use it for the one-off question; use a declarator from
// section I for a probe several tests share.
//
//   The lambda is never invoked - only its declaration is ever considered -
// so an empty body is correct.  Spell the trailing return type
// `decltype(void( EXPR ))`: `void(...)` makes the return type unconditionally
// void (so the empty body is unambiguously well-formed) and denies an
// overloaded `operator,` any say in the matter.
//
//   Requires generic lambdas (C++14+) to be useful; the function itself
// imposes nothing.
//
// Usage:
//   const bool ok = is_valid<my_type&>(
//       [](auto&& _x) -> decltype(void(_x.size())) {});
//
//   const bool no = is_valid<my_type&, int>(
//       [](auto&& _x, auto&& _i) -> decltype(void(_x.at(_i))) {});
template<typename... _Args,
         typename    _Probe>
D_NODISCARD D_CONSTEXPR bool
is_valid(
    _Probe&& _probe
)
{
    return ( (void)_probe,
             internal::valid_call<_Probe&&, _Args...>(0) );
}


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_nothrow_probe_v
    //   value: convenience alias for is_nothrow_probe<...>::value.
    template<template<typename...> typename _Probe,
             typename...                    _Types>
    constexpr bool is_nothrow_probe_v =
        is_nothrow_probe<_Probe, _Types...>::value;

    // yields_lvalue_v
    //   value: convenience alias for yields_lvalue<...>::value.
    template<template<typename...> typename _Probe,
             typename...                    _Types>
    constexpr bool yields_lvalue_v =
        yields_lvalue<_Probe, _Types...>::value;

    // yields_xvalue_v
    //   value: convenience alias for yields_xvalue<...>::value.
    template<template<typename...> typename _Probe,
             typename...                    _Types>
    constexpr bool yields_xvalue_v =
        yields_xvalue<_Probe, _Types...>::value;

    // yields_prvalue_v
    //   value: convenience alias for yields_prvalue<...>::value.
    template<template<typename...> typename _Probe,
             typename...                    _Types>
    constexpr bool yields_prvalue_v =
        yields_prvalue<_Probe, _Types...>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///                III. TYPE-SET QUANTIFIERS                                 ///
///////////////////////////////////////////////////////////////////////////////
//
//   "The trait is true for these and false for those" is the shape of nearly
// every trait test, and conjunction / disjunction are the wrong tools for it.
// They SHORT-CIRCUIT: conjunction stops at the first false, disjunction at
// the first true, and the instantiations past that point never happen.  In
// production that is a feature.  In a test it is a hole - the cell that would
// have failed to compile is precisely the one you needed to reach.
//
//   count_holds below evaluates every cell, always, and the three quantifiers
// derive from it.  A trait that hard-errors on the fourth of five types
// therefore breaks the build here instead of hiding behind the third.

NS_INTERNAL

    // count_true
    //   trait: the number of `true` values in a bool pack.  Recursive rather
    // than folded so the C++11-era spelling still reads the same; the pack is
    // fully expanded by the caller, so every trait instantiation in it has
    // already happened by the time this is entered.
    template<bool... _Values>
    struct count_true;

    // count_true (empty case)
    //   trait: base case - the empty pack holds nothing.
    template<>
    struct count_true<>
    {
        static D_CONSTEXPR std::size_t value = 0;
    };

    // count_true (recursive case)
    //   trait: adds the head to the count of the tail.
    template<bool    _Value,
             bool... _Rest>
    struct count_true<_Value, _Rest...>
    {
        static D_CONSTEXPR std::size_t value =
            ( (_Value ? static_cast<std::size_t>(1)
                      : static_cast<std::size_t>(0)) +
              count_true<_Rest...>::value );
    };

NS_END  // internal


// count_holds
//   trait: how many of `_Types...` satisfy `_Trait`.  Every `_Trait<_Type>`
// in the pack is instantiated - there is no short-circuit - so this doubles
// as the blunt instrument that proves a trait is SFINAE-friendly across a
// type set: if any cell hard-errors instead of answering, the build stops
// here.  (That is the only way to detect a non-SFINAE-friendly trait from
// inside the same translation unit; a hard error cannot be caught, only
// provoked.)
template<template<typename> typename _Trait,
         typename...                 _Types>
struct count_holds
    : std::integral_constant<
          std::size_t,
          internal::count_true<_Trait<_Types>::value...>::value>
{};

// holds_for_all
//   trait: true iff `_Trait` is satisfied by every one of `_Types...`.  The
// positive battery.
template<template<typename> typename _Trait,
         typename...                 _Types>
struct holds_for_all
    : bool_constant<
        ( count_holds<_Trait, _Types...>::value == sizeof...(_Types) )>
{};

// holds_for_any
//   trait: true iff `_Trait` is satisfied by at least one of `_Types...`.
template<template<typename> typename _Trait,
         typename...                 _Types>
struct holds_for_any
    : bool_constant<( count_holds<_Trait, _Types...>::value > 0 )>
{};

// holds_for_none
//   trait: true iff `_Trait` is satisfied by none of `_Types...`.  The
// negative battery - and, paired with the D_TEST_HOSTILE_* lists in section
// VI, the statement "this trait rejects everything it should reject and
// survives everything it cannot classify".
template<template<typename> typename _Trait,
         typename...                 _Types>
struct holds_for_none
    : bool_constant<( count_holds<_Trait, _Types...>::value == 0 )>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // count_holds_v
    //   value: convenience alias for count_holds<...>::value.
    template<template<typename> typename _Trait,
             typename...                 _Types>
    constexpr std::size_t count_holds_v =
        count_holds<_Trait, _Types...>::value;

    // holds_for_all_v
    //   value: convenience alias for holds_for_all<...>::value.
    template<template<typename> typename _Trait,
             typename...                 _Types>
    constexpr bool holds_for_all_v =
        holds_for_all<_Trait, _Types...>::value;

    // holds_for_any_v
    //   value: convenience alias for holds_for_any<...>::value.
    template<template<typename> typename _Trait,
             typename...                 _Types>
    constexpr bool holds_for_any_v =
        holds_for_any<_Trait, _Types...>::value;

    // holds_for_none_v
    //   value: convenience alias for holds_for_none<...>::value.
    template<template<typename> typename _Trait,
             typename...                 _Types>
    constexpr bool holds_for_none_v =
        holds_for_none<_Trait, _Types...>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///                IV.  TRAIT SHAPE                                          ///
///////////////////////////////////////////////////////////////////////////////
//
//   A trait is not just a `::value`.  The standard's UnaryTypeTrait contract -
// which every trait emitted by trait_detect.hpp inherits by construction, and
// which every hand-written one is supposed to honour - says the trait is
// publicly and unambiguously derived from an `integral_constant`, and
// therefore carries `value_type`, `type`, a conversion to its value type and
// a call operator that yields it.  Code downstream relies on all of that:
// `std::conditional_t<is_foo<T>{}, ...>` needs the conversion,
// `conjunction<is_foo<T>, is_bar<T>>` needs `::value` to be a bool, and a
// concept face needs `::value` to be a constant expression.
//
//   A trait that answers correctly but is shaped wrong will pass every test a
// suite thinks to write and then break at the first use site that leans on
// the contract.  is_bool_trait is that missing test.

// is_bool_trait
//   trait: true iff `_Trait` - an INSTANTIATED trait, e.g. `is_foo<int>`, not
// the template - has the shape the standard requires of a bool trait:
//     - a nested `value_type` that is exactly bool,
//     - a `::value` usable as a constant expression,
//     - a nested `type` that is `bool_constant<value>`,
//     - public, unambiguous derivation from `bool_constant<value>`,
//     - a conversion to bool and a call operator, both inherited from that
//       base and both probed here in case an override has shadowed them.
//
//   Every clause is probed, so an instantiation missing any of them reads as
// false instead of diagnosing.  `is_bool_trait<nonesuch>` is false, as is
// `is_bool_trait<int>`.
template<typename _Trait,
         typename = void>
struct is_bool_trait : std::false_type
{};

// is_bool_trait (well-formed case)
//   trait: the specialization reached once every member the contract names is
// present; it then checks that they say the right things.
template<typename _Trait>
struct is_bool_trait<_Trait,
    D_VOID_T<typename _Trait::value_type,
             typename _Trait::type,
             std::integral_constant<bool, _Trait::value>,
             decltype(static_cast<bool>(std::declval<const _Trait&>())),
             decltype(std::declval<const _Trait&>()())>>
    : bool_constant<
        ( std::is_same<typename _Trait::value_type, bool>::value      &&
          std::is_same<typename _Trait::type,
                       bool_constant<_Trait::value>>::value           &&
          std::is_base_of<bool_constant<_Trait::value>, _Trait>::value )>
{};

NS_INTERNAL

    // well_formed_binder
    //   trait: binds `_Trait` so that "is `_Trait<_Type>` a well-formed bool
    // trait?" becomes the one-parameter trait the quantifiers in section III
    // take.  Declared after is_bool_trait, which it names.
    template<template<typename> typename _Trait>
    struct well_formed_binder
    {
        // check
        //   trait: is_bool_trait applied to one instantiation of _Trait.
        template<typename _Type>
        using check = is_bool_trait<_Trait<_Type>>;
    };

NS_END  // internal


// trait_is_well_formed
//   trait: true iff `_Trait<_Type>` is a well-formed bool trait for every one
// of `_Types...`.  The shape check, run over a battery - and the natural
// partner of the D_TEST_HOSTILE_* lists, since a trait's shape is exactly the
// thing most likely to quietly degrade on the types its author never tried.
template<template<typename> typename _Trait,
         typename...                 _Types>
struct trait_is_well_formed
    : holds_for_all<internal::well_formed_binder<_Trait>::template check,
                    _Types...>
{};


// D_TEST_TRAIT_V_AGREES
//   macro: constant expression, true iff the `_v` companion of TRAIT agrees
// with `TRAIT<...>::value` for the same arguments.  The two are emitted from
// different macros (D_TYPE_TRAIT_TRUE and D_TYPE_TRAIT_VALUE_BOOL) and are
// hand-written just as often; nothing but a test keeps them in step, and a
// stale `_v` pointing at a renamed or re-based trait is a silent, total
// inversion of the answer at every call site that prefers the shorthand.
//
//   TRAIT is pasted, so pass the trait's NAME, not an instantiation - a
// qualified name is fine (only its last token is pasted onto `_v`).  On a
// standard without variable templates there is no `_v` to disagree with, so
// the check is vacuously true.
//
// Usage:
//   D_TEST_TRAIT_V_AGREES(is_test_kind_set, kind_set_type)
//   D_TEST_TRAIT_V_AGREES(::djinterp::test::is_test_kind_set, int)
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    #define D_TEST_TRAIT_V_AGREES(TRAIT, ...)                                 \
        ( (TRAIT##_v<__VA_ARGS__>) == (TRAIT<__VA_ARGS__>::value) )
#else
    #define D_TEST_TRAIT_V_AGREES(TRAIT, ...)   (true)
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_bool_trait_v
    //   value: convenience alias for is_bool_trait<_Trait>::value.
    template<typename _Trait>
    constexpr bool is_bool_trait_v = is_bool_trait<_Trait>::value;

    // trait_is_well_formed_v
    //   value: convenience alias for trait_is_well_formed<...>::value.
    template<template<typename> typename _Trait,
             typename...                 _Types>
    constexpr bool trait_is_well_formed_v =
        trait_is_well_formed<_Trait, _Types...>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///                V.   CV-REF AGREEMENT                                     ///
///////////////////////////////////////////////////////////////////////////////
//
//   Almost every trait in the framework is written to be cv-ref agnostic -
// D_TYPE_TRAIT_HAS_TYPE strips through `clean_t`, and traits that dispatch on
// a class template do the same by hand (is_test_kind_set is
// `internal::..._instantiation<clean_t<_Type>>`).  A trait that FORGETS the
// strip still passes every test written against the bare type and then
// answers false the first time a caller hands it a `const T&`.
//
//   The obvious test - probe all eight forms and AND them together - loses
// the one fact worth reporting: WHICH form broke.  cvref_report keeps the
// cells, so a failing suite says "const volatile _Type" instead of "cvref:
// FAIL".

NS_INTERNAL

    // cv-ref cells
    //   the eight forms a cv-ref-agnostic trait must answer identically for.
    // Named once, here, so the report builder and the agreement trait cannot
    // drift apart.  Each is total: add_const / add_lvalue_reference and their
    // siblings are no-ops on the types (void, functions, references) that
    // cannot take the qualifier, so the cells are well-formed for every
    // `_Type` a trait might be handed.

    // cell_const
    //   type: `const _Type`.
    template<typename _Type>
    using cell_const = typename std::add_const<_Type>::type;

    // cell_volatile
    //   type: `volatile _Type`.
    template<typename _Type>
    using cell_volatile = typename std::add_volatile<_Type>::type;

    // cell_cv
    //   type: `const volatile _Type`.
    template<typename _Type>
    using cell_cv = typename std::add_cv<_Type>::type;

    // cell_lvalue_ref
    //   type: `_Type&`.
    template<typename _Type>
    using cell_lvalue_ref = typename std::add_lvalue_reference<_Type>::type;

    // cell_const_lvalue_ref
    //   type: `const _Type&` - the form a by-const-reference parameter hands
    // a trait, and the one most often forgotten.
    template<typename _Type>
    using cell_const_lvalue_ref =
        typename std::add_lvalue_reference<cell_const<_Type>>::type;

    // cell_rvalue_ref
    //   type: `_Type&&`.
    template<typename _Type>
    using cell_rvalue_ref = typename std::add_rvalue_reference<_Type>::type;

    // cell_const_rvalue_ref
    //   type: `const _Type&&`.
    template<typename _Type>
    using cell_const_rvalue_ref =
        typename std::add_rvalue_reference<cell_const<_Type>>::type;

NS_END  // internal


// cvref_report
//   struct: the eight-cell outcome of asking one trait about one type under
// every cv-ref qualification.  An aggregate of bools, so it is a literal type
// and the whole matrix is computed during constant evaluation; the accessors
// are what a suite actually reports.
struct cvref_report
{
    bool bare;               // _Type
    bool with_const;         // const _Type
    bool with_volatile;      // volatile _Type
    bool with_cv;            // const volatile _Type
    bool lvalue_ref;         // _Type&
    bool const_lvalue_ref;   // const _Type&
    bool rvalue_ref;         // _Type&&
    bool const_rvalue_ref;   // const _Type&&


    // first_disagreement
    //   the name of the first cell whose answer differs from the bare type's,
    // or nullptr when every cell agrees.  This is what turns a failed cv-ref
    // check from "something is wrong" into "`const volatile _Type` is wrong",
    // which is the difference between a report line and a debugging session.
    D_CONSTEXPR const char*
    first_disagreement() const D_NOEXCEPT
    {
        return (with_const       != bare) ? "const _Type"
             : (with_volatile    != bare) ? "volatile _Type"
             : (with_cv          != bare) ? "const volatile _Type"
             : (lvalue_ref       != bare) ? "_Type&"
             : (const_lvalue_ref != bare) ? "const _Type&"
             : (rvalue_ref       != bare) ? "_Type&&"
             : (const_rvalue_ref != bare) ? "const _Type&&"
             :                              nullptr;
    }

    // agrees
    //   true iff every cell answers the same as the bare type.  The question
    // a cv-ref test is really asking; note that it is satisfied by a trait
    // that is uniformly FALSE just as well as by one that is uniformly true -
    // pair it with `bare` (or with all() / none()) to pin the polarity.
    D_CONSTEXPR bool
    agrees() const D_NOEXCEPT
    {
        return (first_disagreement() == nullptr);
    }

    // all
    //   true iff the trait holds for all eight forms.
    D_CONSTEXPR bool
    all() const D_NOEXCEPT
    {
        return ( bare             &&
                 with_const       &&
                 with_volatile    &&
                 with_cv          &&
                 lvalue_ref       &&
                 const_lvalue_ref &&
                 rvalue_ref       &&
                 const_rvalue_ref );
    }

    // none
    //   true iff the trait holds for none of the eight forms.
    D_CONSTEXPR bool
    none() const D_NOEXCEPT
    {
        return ( (!bare)             &&
                 (!with_const)       &&
                 (!with_volatile)    &&
                 (!with_cv)          &&
                 (!lvalue_ref)       &&
                 (!const_lvalue_ref) &&
                 (!rvalue_ref)       &&
                 (!const_rvalue_ref) );
    }
};

// trait_across_cvref
//   function: instantiates `_Trait` over all eight cv-ref forms of `_Type`
// and hands back the filled report.  Constexpr, so a suite can hold the
// result in a `constexpr cvref_report` and still print the failing cell's
// name at runtime.
//
// Usage:
//   D_CONSTEXPR cvref_report r =
//       trait_across_cvref<is_test_kind_set, kind_set_type>();
//   ok &= D_TK_CHECK(r.all(), "is_test_kind_set: cv-ref agnostic");
template<template<typename> typename _Trait,
         typename                    _Type>
D_NODISCARD D_CONSTEXPR cvref_report
trait_across_cvref()
{
    return cvref_report{
        _Trait<_Type>::value,
        _Trait<internal::cell_const<_Type>>::value,
        _Trait<internal::cell_volatile<_Type>>::value,
        _Trait<internal::cell_cv<_Type>>::value,
        _Trait<internal::cell_lvalue_ref<_Type>>::value,
        _Trait<internal::cell_const_lvalue_ref<_Type>>::value,
        _Trait<internal::cell_rvalue_ref<_Type>>::value,
        _Trait<internal::cell_const_rvalue_ref<_Type>>::value
    };
}

// trait_ignores_cvref
//   trait: true iff `_Trait` answers identically for all eight cv-ref forms
// of `_Type` - the report's agrees(), lifted to a trait so it can be pinned
// with D_TEST_STATIC or folded into a larger constant expression.
template<template<typename> typename _Trait,
         typename                    _Type>
struct trait_ignores_cvref
    : bool_constant<trait_across_cvref<_Trait, _Type>().agrees()>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // trait_ignores_cvref_v
    //   value: convenience alias for trait_ignores_cvref<...>::value.
    template<template<typename> typename _Trait,
             typename                    _Type>
    constexpr bool trait_ignores_cvref_v =
        trait_ignores_cvref<_Trait, _Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///                VI.  FIXTURES  (the type zoo)                             ///
///////////////////////////////////////////////////////////////////////////////
//
//   A trait test is only as good as the types it is tried on, and the types a
// trait's author thinks of are, by construction, the types the trait already
// handles.  The fixtures below are the ones nobody thinks of - each is here
// because it is a known way for a detection trait to be WRONG rather than
// merely false:
//
//     private_members     access failure must be a substitution failure
//     ambiguous_members   ambiguous lookup must be a substitution failure
//     incomplete          a probe must not require completeness
//     greedy              converts to anything: catches a trait that tests
//                         convertibility where it meant to test an expression
//     evil                overloads `operator&` and `operator,`: catches a
//                         probe that spells `&x` or leans on a bare comma
//     throwing            the negative for every noexcept probe
//     nonliteral          the negative for every constexpr probe
//     the non-class list  void, functions, arrays, references, enums, member
//                         pointers - the shapes that turn a careless
//                         `_Type::value_type` into a hard error
//
//   None of these is exotic.  Every one of them is a type a generic container
// or a generic algorithm will eventually be instantiated on.
//
//   The fixtures declare, and mostly do not define, their members: a trait
// only ever names them in unevaluated operands, which does not odr-use them,
// so there is nothing to link.

NS_FIXTURES

// ---------------------------------------------------------------------------
//  class-type fixtures
// ---------------------------------------------------------------------------

// empty
//   struct: no members at all.  Every member probe must report false for it,
// and none may diagnose.
struct empty
{
};

// incomplete
//   struct: declared and never defined.  A probe must tolerate it - report
// false - rather than instantiate anything that requires a complete type.
struct incomplete;

// abstract
//   struct: carries a pure virtual, so it is never an object type.  Anything
// that tries to materialize a value of it (rather than a reference) breaks.
struct abstract
{
    virtual ~abstract() = default;

    virtual void run() = 0;
};

// final_type
//   struct: cannot be derived from.  The fixture for any trait that probes by
// inheriting from its subject.
struct final_type final
{
};

// private_members
//   class: carries a `value_type` and a `size()` that are both PRIVATE.  A
// correct probe reports false; a probe that ignores access control fails to
// compile.  Access checking is part of substitution, so the failure lands in
// the immediate context - this fixture is what proves a probe relies on that
// rather than on luck.
class private_members
{
private:
    using value_type = int;

    int size() const;
};

// value_type_a
//   struct: a base carrying `value_type`.  Half of ambiguous_members.
struct value_type_a
{
    using value_type = int;
};

// value_type_b
//   struct: a second base carrying `value_type`.  The other half.
struct value_type_b
{
    using value_type = char;
};

// ambiguous_members
//   struct: inherits `value_type` from two bases, so the name is ambiguous.
// Lookup fails in the immediate context - another substitution failure a
// probe has to survive rather than diagnose.
struct ambiguous_members : value_type_a,
                           value_type_b
{
};

// greedy
//   struct: converts to ANYTHING, via a template conversion operator.  It
// satisfies every convertibility-shaped probe and no expression-shaped one,
// which makes it the fixture that separates the two: a trait that claims
// `greedy` models a contract is testing convertibility where it meant to test
// an expression.
struct greedy
{
    template<typename _Type>
    operator _Type() const;
};

// evil
//   struct: overloads unary `operator&` and `operator,` - the two operators a
// carelessly written probe routes through by accident.  A probe that says
// `&_x` where it meant `std::addressof(_x)`, or that separates sub-expressions
// with a bare comma, silently changes meaning here and nowhere else.
struct evil
{
    void* operator&() const;

    template<typename _Type>
    evil operator,(_Type&& _rhs) const;
};

// throwing
//   struct: default-constructs, copies, assigns and destroys, and none of it
// is noexcept.  The negative fixture for every noexcept probe.
struct throwing
{
    throwing();
    throwing(const throwing& _other);
    throwing(throwing&& _other);

    throwing& operator=(const throwing& _other);
    throwing& operator=(throwing&& _other);

    ~throwing();
};

// nothrowing
//   struct: the noexcept mirror of `throwing` - same surface, every operation
// non-throwing.  The positive fixture for every noexcept probe.
struct nothrowing
{
    nothrowing() D_NOEXCEPT;
    nothrowing(const nothrowing& _other) D_NOEXCEPT;
    nothrowing(nothrowing&& _other) D_NOEXCEPT;

    nothrowing& operator=(const nothrowing& _other) D_NOEXCEPT;
    nothrowing& operator=(nothrowing&& _other) D_NOEXCEPT;

    ~nothrowing() D_NOEXCEPT;
};

// literal
//   struct: a literal type with a constexpr constructor.  The positive
// fixture for D_TEST_CONSTEXPR_PROBE.
struct literal
{
    D_CONSTEXPR literal(
        int _value
    ) D_NOEXCEPT
        : value(_value)
    {}

    int value;
};

// nonliteral
//   struct: not a literal type - its constructor and destructor are not
// constexpr - so no expression that builds one is ever a constant expression.
// The negative fixture for D_TEST_CONSTEXPR_PROBE.
struct nonliteral
{
    nonliteral();

    ~nonliteral();
};

// plain_enum
//   enum: an unscoped enumeration - converts to its underlying type, so it
// slips through integral-shaped probes that were meant to reject it.
enum plain_enum
{
    plain_enum_zero = 0
};

// scoped_enum
//   enum: a scoped enumeration - no implicit conversion, so it slips through
// nothing.
enum class scoped_enum
{
    zero = 0
};


// ---------------------------------------------------------------------------
//  non-class fixtures
// ---------------------------------------------------------------------------
//   Named as aliases so they can be spelled in a pack without a suite having
// to remember which of `int[]`, `int(&)(int)` and `int literal::*` needs
// parentheses.

// void_type
//   type: `void` - has no members, no size, and no value.
using void_type = void;

// const_void_type
//   type: `const void` - the qualified form, which a cv-ref matrix will
// produce whether or not a suite asked for it.
using const_void_type = const void;

// function_type
//   type: a function type.  Not an object type: it cannot be cv-qualified, it
// cannot be a member, and `declval<_Type>()` on it is a reference.
using function_type = int(int);

// function_ptr_type
//   type: a pointer to function.
using function_ptr_type = int (*)(int);

// function_ref_type
//   type: a reference to function.
using function_ref_type = int (&)(int);

// array_type
//   type: a bounded array.
using array_type = int[3];

// unbounded_array_type
//   type: an unbounded array - an incomplete object type.
using unbounded_array_type = int[];

// lvalue_ref_type
//   type: an lvalue reference.  References have no members of their own; a
// trait that forgets to strip one answers about the reference, not the type.
using lvalue_ref_type = int&;

// rvalue_ref_type
//   type: an rvalue reference.
using rvalue_ref_type = int&&;

// member_object_ptr_type
//   type: a pointer to data member.
using member_object_ptr_type = int literal::*;

// member_fn_ptr_type
//   type: a pointer to member function.
using member_fn_ptr_type = void (throwing::*)();

// nullptr_type
//   type: std::nullptr_t - converts to every pointer and models nothing.
using nullptr_type = std::nullptr_t;

NS_END  // fixtures


// D_TEST_HOSTILE_CLASS_TYPES
//   macro: the comma-separated list of every CLASS-type fixture, ready to
// drop straight into a pack position.  Spelled as a macro rather than a type
// list because that is what the quantifiers in section III take, and because
// a pack cannot be handed around any other way at this floor.
//
// Usage:
//   holds_for_none<is_test_kind_set, D_TEST_HOSTILE_CLASS_TYPES>::value
#define D_TEST_HOSTILE_CLASS_TYPES                                            \
    ::djinterp::test::fixtures::empty,                                        \
    ::djinterp::test::fixtures::incomplete,                                   \
    ::djinterp::test::fixtures::abstract,                                     \
    ::djinterp::test::fixtures::final_type,                                   \
    ::djinterp::test::fixtures::private_members,                              \
    ::djinterp::test::fixtures::ambiguous_members,                            \
    ::djinterp::test::fixtures::greedy,                                       \
    ::djinterp::test::fixtures::evil,                                         \
    ::djinterp::test::fixtures::throwing,                                     \
    ::djinterp::test::fixtures::nothrowing,                                   \
    ::djinterp::test::fixtures::literal,                                      \
    ::djinterp::test::fixtures::nonliteral,                                   \
    ::djinterp::test::fixtures::plain_enum,                                   \
    ::djinterp::test::fixtures::scoped_enum

// D_TEST_HOSTILE_NONCLASS_TYPES
//   macro: the comma-separated list of every NON-class fixture.  Kept apart
// from the class list because a trait may legitimately be documented for
// object types only; such a trait must still SURVIVE this list (answer false),
// which is what a run through holds_for_none proves.
#define D_TEST_HOSTILE_NONCLASS_TYPES                                         \
    ::djinterp::test::fixtures::void_type,                                    \
    ::djinterp::test::fixtures::const_void_type,                              \
    ::djinterp::test::fixtures::function_type,                                \
    ::djinterp::test::fixtures::function_ptr_type,                            \
    ::djinterp::test::fixtures::function_ref_type,                            \
    ::djinterp::test::fixtures::array_type,                                   \
    ::djinterp::test::fixtures::unbounded_array_type,                         \
    ::djinterp::test::fixtures::lvalue_ref_type,                              \
    ::djinterp::test::fixtures::rvalue_ref_type,                              \
    ::djinterp::test::fixtures::member_object_ptr_type,                       \
    ::djinterp::test::fixtures::member_fn_ptr_type,                           \
    ::djinterp::test::fixtures::nullptr_type

// D_TEST_HOSTILE_TYPES
//   macro: both lists - the full zoo.  `holds_for_none<is_foo,
// D_TEST_HOSTILE_TYPES>::value` is the one-line statement that `is_foo`
// rejects everything it does not recognize AND compiles for everything it
// cannot classify; the second half of that is enforced by the build, since
// count_holds instantiates every cell.
#define D_TEST_HOSTILE_TYPES                                                  \
    D_TEST_HOSTILE_CLASS_TYPES,                                               \
    D_TEST_HOSTILE_NONCLASS_TYPES

// D_TEST_HOSTILE_CLASS_TYPES_COMPLETE
//   macro: the class list MINUS the incomplete fixture.
//
//   Some traits legitimately require a complete type, and cannot be run
// against `incomplete` at all - not "report false", but "fail the build".  The
// std library mandates completeness for most of the <type_traits> property
// traits (is_trivially_destructible and friends), so any probe that routes
// through one inherits the requirement: a trait built on is_literal_type, or
// on std::is_trivially_default_constructible, hard-errors on an incomplete
// type no matter how carefully its own detection expression is written.
//
//   Where that requirement is INTENDED, run the battery over these lists and
// state the requirement as a finding.  Where it is not, the trait has a
// SFINAE-friendliness bug and D_TEST_HOSTILE_CLASS_TYPES is the list that
// finds it - by failing to compile.
#define D_TEST_HOSTILE_CLASS_TYPES_COMPLETE                                   \
    ::djinterp::test::fixtures::empty,                                        \
    ::djinterp::test::fixtures::abstract,                                     \
    ::djinterp::test::fixtures::final_type,                                   \
    ::djinterp::test::fixtures::private_members,                              \
    ::djinterp::test::fixtures::ambiguous_members,                            \
    ::djinterp::test::fixtures::greedy,                                       \
    ::djinterp::test::fixtures::evil,                                         \
    ::djinterp::test::fixtures::throwing,                                     \
    ::djinterp::test::fixtures::nothrowing,                                   \
    ::djinterp::test::fixtures::literal,                                      \
    ::djinterp::test::fixtures::nonliteral,                                   \
    ::djinterp::test::fixtures::plain_enum,                                   \
    ::djinterp::test::fixtures::scoped_enum

// D_TEST_HOSTILE_TYPES_COMPLETE
//   macro: the full zoo minus the incomplete fixture - every adversarial shape
// a completeness-requiring trait can still be held to.
#define D_TEST_HOSTILE_TYPES_COMPLETE                                         \
    D_TEST_HOSTILE_CLASS_TYPES_COMPLETE,                                      \
    D_TEST_HOSTILE_NONCLASS_TYPES


///////////////////////////////////////////////////////////////////////////////
///                VII. BUILD-TIME PINS                                      ///
///////////////////////////////////////////////////////////////////////////////

// D_TEST_STATIC
//   macro: pins a compile-time fact, failing the BUILD rather than the report.
// The stringized condition becomes the diagnostic, so the failure names
// itself.
//
//   Use it sparingly and deliberately.  The framework's default is the
// opposite: hand the constant expression to the suite's own check macro, let
// the run record it, and get a red line in the console and the PDF.  A
// D_TEST_STATIC that fires produces no report at all - which is the right
// trade only for invariants whose regression should stop the line, e.g. that
// a probe stayed SFINAE-friendly, or that a trait is still a bool trait at
// all (everything downstream of that is meaningless if it is not).
//
// Usage:
//   D_TEST_STATIC(is_bool_trait<is_test_kind_set<int>>::value);
#define D_TEST_STATIC(...)                                                    \
    static_assert((__VA_ARGS__), "djinterp test: " #__VA_ARGS__)


///////////////////////////////////////////////////////////////////////////////
///                VIII. CONCEPT LAYER  (C++20)                              ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

//   The framework layers a concept face over almost every trait and states,
// in header after header, that "the two agree by construction".  They agree
// by construction only for as long as the concept keeps forwarding to the
// trait; the day someone tightens one and not the other, the trait-constrained
// overload and the concept-constrained overload begin to disagree about the
// same type, and nothing in the build says so.  The two checks below are the
// ones that say so.

// D_TEST_TRAIT_CONCEPT_AGREE
//   macro: constant expression, true iff TRAIT<...>::value and CONCEPT<...>
// give the same answer for the same arguments.  Both are pasted through
// unchanged, so either may be qualified.
//
// Usage:
//   D_TEST_TRAIT_CONCEPT_AGREE(is_test_evaluable, test_evaluable, basic_test)
#define D_TEST_TRAIT_CONCEPT_AGREE(TRAIT, CONCEPT, ...)                       \
    ( (TRAIT<__VA_ARGS__>::value) == (CONCEPT<__VA_ARGS__>) )

// D_TEST_DECLARE_SUBSUMES
//   macro: emits `TRAIT_NAME<_Type>` - a trait true iff `_Type` satisfies BOTH
// concepts and CONCEPT_MORE genuinely SUBSUMES CONCEPT_LESS, i.e. the more
// constrained overload wins.
//
//   Subsumption is not implication, and a refinement ladder that only implies
// is a latent ambiguity: `is_buildable_test_container` implying
// `is_test_object_container` says the set of buildable containers is a subset,
// which is all a trait can say.  Subsumption says the OVERLOAD RESOLVER knows
// it - and it only knows it if the stronger concept is written in terms of the
// weaker one rather than restating its requirements.  A ladder built by
// copy-pasting requirements satisfies every implication test and then makes
// every pair of overloads constrained on it ambiguous.
//
//   The mechanism: two overloads of a ranked constexpr function, one
// constrained on each concept.  If MORE subsumes LESS, the MORE overload is
// more constrained and wins outright (rank 2).  If neither subsumes, the call
// is AMBIGUOUS - which would be a hard error at any ordinary call site, so the
// call is made inside a detection probe, where ambiguity is a substitution
// failure and reads back as false.
//
//   Opens an `internal` namespace for its helpers, so it must be invoked at
// namespace scope inside djinterp (as trait_detect.hpp's
// D_TYPE_TRAIT_MEMBER_TYPE_OR must).  Names are derived from TRAIT_NAME, so
// each invocation needs a distinct one.
//
// Usage:
//   D_TEST_DECLARE_SUBSUMES(buildable_subsumes_object,
//                           is_buildable_test_container_c,
//                           is_test_object_container_c)
//   ...
//   buildable_subsumes_object<my_container>::value
#define D_TEST_DECLARE_SUBSUMES(TRAIT_NAME, CONCEPT_MORE, CONCEPT_LESS)       \
    NS_INTERNAL                                                               \
                                                                              \
        /* TRAIT_NAME##_rank                                              */  \
        /*   function: the less-constrained overload (rank 1).            */  \
        template<typename _Type>                                              \
            requires CONCEPT_LESS<_Type>                                      \
        D_CONSTEXPR int                                                       \
        TRAIT_NAME##_rank()                                                   \
        {                                                                     \
            return 1;                                                         \
        }                                                                     \
                                                                              \
        /* TRAIT_NAME##_rank                                              */  \
        /*   function: the more-constrained overload (rank 2).  Chosen    */  \
        /* over the above only if CONCEPT_MORE subsumes CONCEPT_LESS.     */  \
        template<typename _Type>                                              \
            requires CONCEPT_MORE<_Type>                                      \
        D_CONSTEXPR int                                                       \
        TRAIT_NAME##_rank()                                                   \
        {                                                                     \
            return 2;                                                         \
        }                                                                     \
                                                                              \
        /* TRAIT_NAME##_probe                                             */  \
        /*   trait: the ranked call, in a detection context - an          */  \
        /* ambiguous overload set is a substitution failure here, not a   */  \
        /* diagnostic.                                                    */  \
        template<typename _Type>                                              \
        using TRAIT_NAME##_probe =                                            \
            std::integral_constant<int, TRAIT_NAME##_rank<_Type>()>;          \
                                                                              \
    NS_END  /* internal */                                                    \
                                                                              \
    /* TRAIT_NAME                                                         */  \
    /*   trait: true iff _Type satisfies both concepts and the more       */  \
    /* constrained one wins the overload.                                 */  \
    template<typename _Type>                                                  \
    struct TRAIT_NAME                                                         \
        : ::djinterp::bool_constant<                                          \
            ( CONCEPT_LESS<_Type> &&                                          \
              CONCEPT_MORE<_Type> &&                                          \
              (::djinterp::detected_or_t<                                     \
                   std::integral_constant<int, 0>,                            \
                   internal::TRAIT_NAME##_probe,                              \
                   _Type>::value == 2) )>                                     \
    {};

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TRAITS_
