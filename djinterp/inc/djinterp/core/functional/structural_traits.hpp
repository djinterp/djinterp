/*******************************************************************************
* djinterp [core]                                          structural_traits.hpp
*
* Structural detection traits for functional dataflow:
*   This header extends the functional-traits surface with structural checks
* used by the consolidated parse/scanner/pattern layers when they are wired
* onto the functional dataflow primitives (producer / consumer / view).
* Where the older code enforced these shapes informally - e.g. a comment in
* pattern_scanner.hpp asserting "patterns must additionally expose a
* find(input, pos, result) method" - the contract is now machine-checkable.
*
*   Everything here is purely structural and degrades to C++11.  It builds
* on function_traits.hpp (callable introspection) and the shared
* member-detection macros in core/meta/member_traits.hpp.
*
* TRAITS PROVIDED
*   has_find_method<T, In, Result>
*       Does T expose `bool find(const In&, std::size_t&, Result&)`?  This is
*       the "scan for the next occurrence" shape the pattern scanner pulls on
*       repeatedly; detecting it lets pattern_scanner static_assert the
*       requirement instead of failing deep inside a template instantiation.
*   has_match_result_type<T>
*       Does T expose a nested `match_result_type`?  (The pattern protocol's
*       extraction-face result.)
*   produces_optional_like<T>
*       Does T (a nullary callable) return something usable as an
*       unfold step - i.e. contextually convertible to bool and
*       dereferenceable?  Recognises the maybe<>/pointer "more values?"
*       protocol that drives a pull-based source.
*   is_nullary_callable<T>      / is_unary_callable<T, Arg>
*       Arity-plus-invocability checks expressed once, reused by the
*       source/sink role detectors.
*
*
* path:      /inc/djinterp/core/functional/structural_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.29
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_STRUCTURAL_TRAITS_HPP
#define DJINTERP_FUNCTIONAL_STRUCTURAL_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/type_utility.hpp"  // void_t, clean_t
#include "../meta/member_traits.hpp"


NS_DJINTERP


// ================================================================
//  has_match_result_type
// ================================================================
// The pattern protocol exposes its extraction-face output as a
// nested `match_result_type`.  Reuse the shared detector macro so
// this stays consistent with the other member-typedef detectors.

D_DEFINE_HAS_MEMBER_TYPE(match_result_type)


// ================================================================
//  has_find_method
// ================================================================

NS_INTERNAL

    // has_find_method_helper
    //   trait: primary template (failure case).
    template<typename Type,
             typename Input,
             typename Result,
             typename = void>
    struct has_find_method_helper : std::false_type
    {};

    // has_find_method_helper (success case)
    //   trait: succeeds when Type exposes a callable
    // `find(const Input&, std::size_t&, Result&)` whose return
    // is usable as a bool.  This is the repeated-search shape the
    // pattern scanner relies on: find returns false when no further
    // occurrence exists, and on success writes the start position
    // into the size_t and the captures into the result.
    template<typename Type,
             typename Input,
             typename Result>
    struct has_find_method_helper<
        Type,
        Input,
        Result,
        void_t<decltype(
            static_cast<bool>(
                std::declval<const Type&>().find(
                    std::declval<const Input&>(),
                    std::declval<std::size_t&>(),
                    std::declval<Result&>()
                )
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_find_method
//   trait: detects whether Type exposes a
// `find(const Input&, std::size_t&, Result&) -> bool` member,
// the searchable-pattern shape consumed by the pattern scanner.
template<typename Type,
         typename Input,
         typename Result>
struct has_find_method
    : internal::has_find_method_helper<clean_t<Type>, clean_t<Input>, clean_t<Result>>
{};

// has_find_method_v
//   value: convenience alias for has_find_method<...>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type,
             typename Input,
             typename Result>
    constexpr bool has_find_method_v =
        has_find_method<clean_t<Type>, clean_t<Input>, clean_t<Result>>::value;
#endif


// ================================================================
//  is_nullary_callable  /  is_unary_callable
// ================================================================

NS_INTERNAL

    // is_nullary_callable_helper
    //   trait: primary template (failure case).
    template<typename Type,
             typename = void>
    struct is_nullary_callable_helper : std::false_type
    {};

    // is_nullary_callable_helper (success case)
    //   trait: succeeds when Type is invocable with no arguments.
    template<typename Type>
    struct is_nullary_callable_helper<
        Type,
        void_t<decltype(std::declval<Type&>()())>
    > : std::true_type
    {};

    // is_unary_callable_helper
    //   trait: primary template (failure case).
    template<typename Type,
             typename Arg,
             typename = void>
    struct is_unary_callable_helper : std::false_type
    {};

    // is_unary_callable_helper (success case)
    //   trait: succeeds when Type is invocable with one Arg.
    template<typename Type,
             typename Arg>
    struct is_unary_callable_helper<
        Type,
        Arg,
        void_t<decltype(
            std::declval<Type&>()(std::declval<Arg>())
        )>
    > : std::true_type
    {};

NS_END  // internal

// is_nullary_callable
//   trait: detects whether Type can be invoked with no
// arguments.  The source role (a pull-based producer) is driven by
// a nullary "next" step, so this underpins source detection.
template<typename Type>
struct is_nullary_callable
    : internal::is_nullary_callable_helper<clean_t<Type>>
{};

// is_unary_callable
//   trait: detects whether Type can be invoked with a single
// argument of type Arg.  Transform and predicate steps are unary.
template<typename Type,
         typename Arg>
struct is_unary_callable
    : internal::is_unary_callable_helper<clean_t<Type>, clean_t<Arg>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_nullary_callable_v
    template<typename Type>
    constexpr bool is_nullary_callable_v =
        is_nullary_callable<clean_t<Type>>::value;

    // is_unary_callable_v
    template<typename Type,
             typename Arg>
    constexpr bool is_unary_callable_v =
        is_unary_callable<clean_t<Type>, clean_t<Arg>>::value;
#endif


// ================================================================
//  produces_optional_like
// ================================================================

NS_INTERNAL

    // produces_optional_like_helper
    //   trait: primary template (failure case).
    template<typename Type,
             typename = void>
    struct produces_optional_like_helper : std::false_type
    {};

    // produces_optional_like_helper (success case)
    //   trait: succeeds when Type is a nullary callable whose
    // return value is both contextually convertible to bool (is
    // there a value?) and dereferenceable (give me the value).
    // This is exactly the protocol an unfold source yields on each
    // pull: maybe<T>, T*, and std::optional<T> all satisfy it.
    template<typename Type>
    struct produces_optional_like_helper<
        Type,
        void_t<
            decltype(static_cast<bool>(std::declval<Type&>()())),
            decltype(*std::declval<Type&>()())
        >
    > : std::true_type
    {};

NS_END  // internal

// produces_optional_like
//   trait: detects whether Type is a nullary callable returning
// an optional-like value (bool-testable and dereferenceable),
// i.e. a well-formed pull-based unfold source step.
template<typename Type>
struct produces_optional_like
    : internal::produces_optional_like_helper<clean_t<Type>>
{};

// produces_optional_like_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool produces_optional_like_v =
        produces_optional_like<clean_t<Type>>::value;
#endif


// ================================================================
//  is_binary_callable
// ================================================================
//   Completes the arity trilogy (nullary / unary / binary).  A reducer
// step is binary - (acc, x) -> acc - so this is the structural floor the
// Reducer protocol concept builds on.

NS_INTERNAL

    // is_binary_callable_helper
    //   trait: primary template (failure case).
    template<typename Type,
             typename Arg0,
             typename Arg1,
             typename = void>
    struct is_binary_callable_helper : std::false_type
    {};

    // is_binary_callable_helper (success case)
    //   trait: succeeds when Type is invocable with (Arg0, Arg1).
    template<typename Type,
             typename Arg0,
             typename Arg1>
    struct is_binary_callable_helper<
        Type,
        Arg0,
        Arg1,
        void_t<decltype(
            std::declval<Type&>()(std::declval<Arg0>(),
                                   std::declval<Arg1>())
        )>
    > : std::true_type
    {};

NS_END  // internal

// is_binary_callable
//   trait: detects whether Type can be invoked with two arguments of
// types Arg0 and Arg1.  Reducer steps are binary.
template<typename Type,
         typename Arg0,
         typename Arg1>
struct is_binary_callable
    : internal::is_binary_callable_helper<clean_t<Type>,
                                          clean_t<Arg0>,
                                          clean_t<Arg1>>
{};

// is_binary_callable_v
//   value: convenience alias for is_binary_callable<...>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type,
             typename Arg0,
             typename Arg1>
    constexpr bool is_binary_callable_v =
        is_binary_callable<clean_t<Type>,
                           clean_t<Arg0>,
                           clean_t<Arg1>>::value;
#endif


// ================================================================
//  is_unfold_step
// ================================================================

NS_INTERNAL

    // is_unfold_step_helper
    //   trait: primary template (failure case).
    template<typename Step,
             typename State,
             typename = void>
    struct is_unfold_step_helper : std::false_type
    {};

    // is_unfold_step_helper (success case)
    //   trait: succeeds when invoking Step with a State yields an
    // optional-like value - contextually convertible to bool (is there a
    // next value?) and dereferenceable (the (value, next_state) pair).
    // This is the unary, state-threaded analog of produces_optional_like:
    // the pure unfold step `State -> maybe<(value, next_state)>`.
    template<typename Step,
             typename State>
    struct is_unfold_step_helper<
        Step,
        State,
        void_t<
            decltype(static_cast<bool>(
                std::declval<Step&>()(std::declval<State>()))),
            decltype(*std::declval<Step&>()(std::declval<State>()))
        >
    > : std::true_type
    {};

NS_END  // internal

// is_unfold_step
//   trait: detects whether Step is a pull-based unfold step over
// State, i.e. Step(state) returns an optional-like value (bool-testable
// and dereferenceable).  The compile-time form of a stateful source.
template<typename Step,
         typename State>
struct is_unfold_step
    : internal::is_unfold_step_helper<clean_t<Step>, clean_t<State>>
{};

// is_unfold_step_v
//   value: convenience alias for is_unfold_step<...>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Step,
             typename State>
    constexpr bool is_unfold_step_v =
        is_unfold_step<clean_t<Step>, clean_t<State>>::value;
#endif


// ================================================================
//  protocol concepts  (PascalCase)
// ================================================================
//   The dual-domain protocol faces.  PascalCase per the project's concept
// naming convention (cf. passthrough.hpp's Passthrough), each defined over
// the structural traits above so they degrade cleanly to C++11 (the traits
// remain; only the concept faces are guarded away).
//
//   The Carrier concept (over is_carrier) lives with the carriers in
// core/meta/carrier.hpp and is intentionally not redefined here.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // BinaryCallable
    //   concept: satisfied by a type invocable with two arguments.
    template<typename Type,
             typename Arg0,
             typename Arg1>
    concept BinaryCallable = is_binary_callable<Type, Arg0, Arg1>::value;

    // Reducer
    //   concept: a reduction step - a binary callable (acc, x) -> acc.
    // The pure `step` half of the step/driver split; one Reducer is run by
    // reduce_rt (runtime) or reduce_ct (compile time) unchanged.
    template<typename Fn,
             typename Acc,
             typename Elem>
    concept Reducer = BinaryCallable<Fn, Acc, Elem>;

    // Transducer
    //   concept: a reducer-to-reducer transformer - a unary callable that,
    // given a reducer Rf, yields a transformed reducer (map/filter/take/...).
    // Full reducer-ness of the result is checked where it is applied.
    template<typename Xf,
             typename Rf>
    concept Transducer = is_unary_callable<Xf, Rf>::value;

    // UnfoldStep
    //   concept: a pull-based source step `State -> maybe<(value, state)>`;
    // the compile-time form of a stateful producer.
    template<typename Step,
             typename State>
    concept UnfoldStep = is_unfold_step<Step, State>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_STRUCTURAL_TRAITS_HPP
