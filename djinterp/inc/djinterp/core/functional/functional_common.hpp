/******************************************************************************
* djinterp [functional]                                  functional_common.hpp
*
* Shared callable vocabulary for the functional module (C++).
*   The combinator modules (filter, pipeline, fn_builder, and the accumulator /
* comparator / extractor / transducer families that include this aggregator)
* constrain and introspect on their callable arguments through three traits:
*
*    is_callable<F, Args...>       - can a const-lvalue F be called on Args?
*    callable_result_t<F, Args...> - the type that call yields
*    is_predicate<P, Arg>          - can P be called on Arg, result -> bool?
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.06
*                                                          revised: 2026.07.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.   CALL TRAITS                        (reuse function_traits call detection)
     1.  is_callable<F, Args...>        (can F be called on Args?)
     2.  callable_result_t<F, Args...>  (the result of that call)
II.  PREDICATE TRAIT                     
     1.  is_predicate<P, Arg>           (callable on Arg, result -> bool)
III. CONVENIENCE ALIASES                (C++14 variable templates)
     1.  is_callable_v<F, Args...>      
     2.  is_predicate_v<P, Arg>         
IV.  CONCEPT FACES                      (C++20)
     1.  Callable<F, Args...>           (face of is_callable)
     2.  Predicate<P, Arg>              (face of is_predicate)
*/

#ifndef DJINTERP_FUNCTIONAL_COMMON_
#define DJINTERP_FUNCTIONAL_COMMON_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./function_traits.hpp"   // is_invocable_with / call_result_t /
                                   // is_invocable_r_with (call detection)


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///             I.    CALL TRAITS                                           ///
///////////////////////////////////////////////////////////////////////////////

// is_callable
//   trait: true when a const-lvalue _Fn can be called on _Args. The
// functional-module-facing name for function_traits.hpp's is_invocable_with;
// it succeeds on generic lambdas and other templated operator() callables.
template<typename    _Fn,
         typename... _Args>
struct is_callable
    : is_invocable_with<_Fn, _Args...>
{};

// callable_result_t
//   alias: the type produced by calling a const-lvalue _Fn on _Args, or
// internal::call_nonesuch (from function_traits.hpp) when that call is
// ill-formed. Callers gate on is_callable before relying on the result.
template<typename    _Fn,
         typename... _Args>
using callable_result_t = call_result_t<_Fn, _Args...>;


///////////////////////////////////////////////////////////////////////////////
///             II.   PREDICATE TRAIT                                       ///
///////////////////////////////////////////////////////////////////////////////

// is_predicate
//   trait: true when a const-lvalue _Pred can be called on _Arg and the result
// is convertible to bool - the predicate shape accepted across the functional
// combinators (filter, take_while, partition, ...). Defined as the bool case
// of function_traits.hpp's is_invocable_r_with.
template<typename _Pred,
         typename _Arg>
struct is_predicate
    : is_invocable_r_with<bool, _Pred, _Arg>
{};


///////////////////////////////////////////////////////////////////////////////
///             III.  CONVENIENCE ALIASES                                   ///
///////////////////////////////////////////////////////////////////////////////
//   Variable-template shorthands; gated so the header stays clean under
// -std=c++11. Pre-C++14 callers use the ::value forms above.

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_callable_v
//   constant: shorthand for is_callable<_Fn, _Args...>::value.
template<typename _Fn,
         typename... _Args>
static D_CONSTEXPR bool is_callable_v = is_callable<_Fn, _Args...>::value;

// is_predicate_v
//   constant: shorthand for is_predicate<_Pred, _Arg>::value.
template<typename _Pred,
         typename _Arg>
static D_CONSTEXPR bool is_predicate_v = is_predicate<_Pred, _Arg>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///             IV.   CONCEPT FACES                                         ///
///////////////////////////////////////////////////////////////////////////////
//   The C++20 faces of the vocabulary above, for code that prefers concept
// syntax to the SFINAE traits:
//
//       Callable<F, Args...>   <-  is_callable<F, Args...>
//       Predicate<P, Arg>      <-  is_predicate<P, Arg>
//
// Absent under earlier standards, where callers use the ::value forms.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// Callable
//   concept: satisfied when a const-lvalue _Fn can be called on _Args. The
// concept face of is_callable; succeeds on generic lambdas and other templated
// operator() callables.
template<typename    _Fn,
         typename... _Args>
concept Callable = is_callable<_Fn, _Args...>::value;

// Predicate
//   concept: satisfied when _Pred is a predicate over _Arg - callable on _Arg
// with a result convertible to bool. The concept face of is_predicate.
template<typename _Pred,
         typename _Arg>
concept Predicate = is_predicate<_Pred, _Arg>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_COMMON_