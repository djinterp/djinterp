/*******************************************************************************
* djinterp [core]                                          functional_common.hpp
*
* Shared callable vocabulary for the functional module (C++).
*   The combinator modules (filter, pipeline, fn_builder, and the accumulator /
* comparator / extractor / transducer families that include this aggregator)
* constrain and introspect on their callable arguments through three traits:
*
*    is_callable<F, Args...>       - can a const-lvalue F be called on Args?
*    callable_result_t<F, Args...> - the type that call yields
*    is_predicate<P, Args...>      - can P be called on Args, result -> bool?
*
* with is_predicate's arity forms (is_nullary_predicate, is_unary_predicate,
* is_binary_predicate).  This is the module's ONE predicate vocabulary:
* predicate.hpp, curry.hpp, consumer.hpp and comparator.hpp each used to define
* is_predicate or its arity forms, with four parameter lists and four rules, so
* no translation unit could include two of them; all four now use these.
*
*   These are EXPRESSION-probing traits: they succeed on generic lambdas and
* other templated operator() callables, exactly the shapes the functional
* combinators take.  They are thin reuses of the call-detection primitives in
* function_traits.hpp (is_invocable_with / call_result_t / is_invocable_r_with)
* - this header is the functional-facing name layer over that detection, and it
* re-exports function_traits.hpp so a single include carries both the declared-
* shape introspection and this can-I-call-it vocabulary.
*
*   Each trait carries its full triple, per the module convention: the trait
* itself, a _v variable-template shorthand (C++14+), and a C++20 concept face
* (Callable / Predicate) for code that prefers concept syntax.  The concept
* faces were previously carried by a companion header, functional_concepts.hpp,
* which this module ABSORBS: every other functional module (functor, monoid,
* semigroup, profunctor, traversable, ...) keeps its trait, its _v, and its
* concept together, and the split forced consumers to include two headers to
* obtain one vocabulary.  Only the GENERIC, cross-cutting concepts belong here;
* the domain faces (Composable, Monad, ViewType, Comparator, ...) stay with the
* combinators that define them, so this header is not an aggregate of every
* functional concept, which would redeclare them.  Concept names follow the
* project convention: the PascalCase parallel of the trait, with a leading is_ /
* has_ dropped.
*
* NOTE (reconstruction):
*   The trait half of this file was reconstructed from the interface its
* consumers reference and from the reuse relationship documented in
* 'function_traits.hpp'; reconcile it with the in-tree original before
* committing.
*
* USAGE:
*   auto pred = [](const int& x){ return x > 0; };
*   is_callable<decltype(pred), const int&>::value;        // true
*   callable_result_t<decltype(pred), const int&>;         // bool
*   is_predicate<decltype(pred), const int&>::value;       // true
*
*   // the C++20 concept faces of the same vocabulary:
*   template<Predicate<const int&> P>
*   void keep_if(std::vector<int>&, P);
*
*   template<typename F, typename T>
*       requires Callable<F, const T&>
*   auto apply_to(const T&, F);
*
*
* path:      /inc/djinterp/core/functional/functional_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.06
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CALL TRAITS                        (reuse function_traits call detection)
      -------------------------------------------------------------------------
      1.    is_callable<F, Args...>        (can F be called on Args?)
      2.    callable_result_t<F, Args...>  (the result of that call; absent
                                           when it is ill-formed)

II.   PREDICATE TRAITS
      ----------------
      1.    is_predicate<P, Args...>       (callable on Args, result -> bool)
      2.    is_nullary_predicate<P>        (a predicate of no arguments)
      3.    is_unary_predicate<P, Arg>     (a predicate of one argument)
      4.    is_binary_predicate<P, A, B>   (a predicate of two arguments)
      5.    is_predicate_contextual<P, Args...>
                                           (result -> bool contextually, as
                                            an if condition converts it)

III.  CONVENIENCE ALIASES                (C++14 variable templates)
      -------------------------------------------------------------
      1.    is_callable<F, Args...>::value
      2.    is_predicate<P, Args...>::value
      3.    is_nullary_predicate / is_unary_predicate /
            is_binary_predicate ::value
      4.    is_predicate_contextual<P, Args...>::value

IV.   CONCEPT FACES                      (C++20)
      ------------------------------------------
      1.    Callable<F, Args...>           (face of is_callable)
      2.    Predicate<P, Args...>          (face of is_predicate)
      3.    PredicateContextual<P, Args...>
                                           (face of is_predicate_contextual)
*/

#ifndef DJINTERP_FUNCTIONAL_FUNCTIONAL_COMMON_HPP
#define DJINTERP_FUNCTIONAL_FUNCTIONAL_COMMON_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>                // std::integral_constant, declval
// djinterp
#include "../../djinterp.hpp"         // framework root
#include "../meta/type_utility.hpp"   // void_t
#include "./function_traits.hpp"      // is_invocable_with, is_invocable_r_with


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///             I.    CALL TRAITS                                           ///
///////////////////////////////////////////////////////////////////////////////

// is_callable
//   trait: true when a const-lvalue Fn can be called on Args. The
// functional-module-facing name for function_traits.hpp's is_invocable_with;
// it succeeds on generic lambdas and other templated operator() callables.
template<typename    Fn,
         typename... Args>
struct is_callable : is_invocable_with<Fn, Args...>
{};

NS_INTERNAL

    // callable_result_helper
    //   trait: SFINAE result-type extractor (primary: no `type`,
    // so substitution into callable_result_t is a soft failure).
    template<typename    AlwaysVoid,
             typename    Function,
             typename... Args>
    struct callable_result_helper
    {};

    // callable_result_helper (well-formed specialization)
    //   trait: yields the result type of Function(Args...) when the
    // call expression is well-formed.
    template<typename    Function,
             typename... Args>
    struct callable_result_helper<
        void_t<decltype(std::declval<Function>()(
            std::declval<Args>()...))>,
        Function,
        Args...>
    {
        using type = decltype(std::declval<Function>()(
            std::declval<Args>()...));
    };

NS_END  // internal

// callable_result
//   trait: result type of invoking Function with Args... . Has a
// `::type` member only when the call expression is well-formed,
// making callable_result_t SFINAE-friendly as a default argument: an
// overload whose call would be ill-formed drops out, as with
// std::invoke_result. (call_result_t, in function_traits.hpp, is the form
// that never fails and yields internal::call_nonesuch instead.)
template<typename    Function,
         typename... Args>
struct callable_result
{
    using type =
        typename internal::callable_result_helper<void,
                                                  Function,
                                                  Args...>::type;
};

// callable_result_t
//   type: convenience alias for callable_result<...>::type.
template<typename    Function,
         typename... Args>
using callable_result_t = typename callable_result<Function, Args...>::type;


///////////////////////////////////////////////////////////////////////////////
///             II.   PREDICATE TRAITS                                      ///
///////////////////////////////////////////////////////////////////////////////

// is_predicate
//   trait: true when a const-lvalue Pred can be called on Args and the result
// is convertible to bool - the predicate shape accepted across the functional
// combinators (filter, take_while, partition, predicate_and, ...). Defined as
// the bool case of function_traits.hpp's is_invocable_r_with.
//   The conversion must be implicit, as std::predicate requires: a callable
// whose result converts to bool only explicitly (an optional, a unique_ptr) is
// not a predicate. Each argument is probed as std::declval gives it, so Arg is
// an rvalue and const Arg& a const lvalue; spell the reference a combinator
// passes. A pointer to member is not called, so it is not a predicate.
template<typename    Pred,
         typename... Args>
struct is_predicate
    : std::integral_constant<bool,
                             is_invocable_r_with<bool, Pred, Args...>::value>
{};

NS_INTERNAL

    // is_predicate_contextual_helper
    //   trait: primary template -- the call, or the bool cast of its result,
    // is ill-formed.
    template<typename    AlwaysVoid,
             typename    Pred,
             typename... Args>
    struct is_predicate_contextual_helper : std::false_type
    {};

    // is_predicate_contextual_helper (well-formed specialization)
    //   trait: a const-lvalue Pred can be called on Args, and static_cast<bool>
    // accepts the result.
    template<typename    Pred,
             typename... Args>
    struct is_predicate_contextual_helper<
        void_t<decltype(static_cast<bool>(
            std::declval<const Pred&>()(std::declval<Args>()...)))>,
        Pred,
        Args...> : std::true_type
    {};

NS_END  // internal

// is_predicate_contextual
//   trait: true when a const-lvalue Pred can be called on Args and the result
// converts to bool contextually -- by static_cast, as an if or while
// condition converts it -- so a callable returning an optional or a
// unique_ptr qualifies. The looser twin of is_predicate, which keeps
// std::predicate's implicit conversion; the owner's ruling of 2026.10.02
// keeps both. Arguments are probed as is_predicate probes them.
template<typename    Pred,
         typename... Args>
struct is_predicate_contextual
    : internal::is_predicate_contextual_helper<void, Pred, Args...>
{};

// is_nullary_predicate
//   trait: true when Pred is a predicate of no arguments.
template<typename Pred>
struct is_nullary_predicate : is_predicate<Pred>
{};

// is_unary_predicate
//   trait: true when Pred is a predicate of one argument, of type Arg.
template<typename Pred,
         typename Arg>
struct is_unary_predicate : is_predicate<Pred, Arg>
{};

// is_binary_predicate
//   trait: true when Pred is a predicate of two arguments, of types First and
// Second in that order. comparator.hpp's relations are binary predicates over
// (const T&, const T&).
template<typename Pred,
         typename First,
         typename Second>
struct is_binary_predicate : is_predicate<Pred, First, Second>
{};


///////////////////////////////////////////////////////////////////////////////
///             III.  CONVENIENCE ALIASES                                   ///
///////////////////////////////////////////////////////////////////////////////
//   Variable-template shorthands; gated so the header stays clean under
// -std=c++11. Pre-C++14 callers use the ::value forms above.

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_callable_v
//   constant: shorthand for is_callable<Fn, Args...>::value.
template<typename Fn,
         typename... Args>
static D_CONSTEXPR bool is_callable_v = is_callable<Fn, Args...>::value;

// is_predicate_v
//   constant: shorthand for is_predicate<Pred, Args...>::value.
template<typename    Pred,
         typename... Args>
static D_CONSTEXPR bool is_predicate_v = is_predicate<Pred, Args...>::value;

// is_predicate_contextual_v
//   constant: shorthand for is_predicate_contextual<Pred, Args...>::value.
template<typename    Pred,
         typename... Args>
static D_CONSTEXPR bool is_predicate_contextual_v =
    is_predicate_contextual<Pred, Args...>::value;

// is_nullary_predicate_v
//   constant: shorthand for is_nullary_predicate<Pred>::value.
template<typename Pred>
static D_CONSTEXPR bool is_nullary_predicate_v =
    is_nullary_predicate<Pred>::value;

// is_unary_predicate_v
//   constant: shorthand for is_unary_predicate<Pred, Arg>::value.
template<typename Pred,
         typename Arg>
static D_CONSTEXPR bool is_unary_predicate_v =
    is_unary_predicate<Pred, Arg>::value;

// is_binary_predicate_v
//   constant: shorthand for is_binary_predicate<Pred, First, Second>::value.
template<typename Pred,
         typename First,
         typename Second>
static D_CONSTEXPR bool is_binary_predicate_v =
    is_binary_predicate<Pred, First, Second>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///             IV.   CONCEPT FACES                                         ///
///////////////////////////////////////////////////////////////////////////////
//   The C++20 faces of the vocabulary above, for code that prefers concept
// syntax to the SFINAE traits:
//
//       Callable<F, Args...>   <-  is_callable<F, Args...>
//       Predicate<P, Args...>  <-  is_predicate<P, Args...>
//
// Absent under earlier standards, where callers use the ::value forms.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// Callable
//   concept: satisfied when a const-lvalue Fn can be called on Args. The
// concept face of is_callable; succeeds on generic lambdas and other templated
// operator() callables.
template<typename    Fn,
         typename... Args>
concept Callable = is_callable<Fn, Args...>::value;

// Predicate
//   concept: satisfied when Pred is a predicate over Args - callable on them
// with a result convertible to bool. The concept face of is_predicate.
template<typename    Pred,
         typename... Args>
concept Predicate = is_predicate<Pred, Args...>::value;

// PredicateContextual
//   concept: satisfied when Pred is a predicate over Args whose result
// converts to bool contextually. The concept face of is_predicate_contextual.
template<typename    Pred,
         typename... Args>
concept PredicateContextual = is_predicate_contextual<Pred, Args...>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_FUNCTIONAL_COMMON_HPP
