/******************************************************************************
* djinterp [functional]                                 functional_concepts.hpp
*
* C++20 concept faces for the functional module's callable vocabulary.
*   The companion to functional_traits.hpp: where that header exposes the
* SFINAE traits the combinators constrain on across every standard, this header
* exposes their C++20 concept parallels for code that prefers concept syntax:
*
*       Callable<F, Args...>   <-  is_callable<F, Args...>
*       Predicate<P, Arg>      <-  is_predicate<P, Arg>
*
*   These are the GENERIC, cross-cutting concepts only.  Domain-specific concept
* faces (Composable, Monad, ViewType, Comparator, ...) live with the combinators
* that define them; this header is not an aggregate of every functional concept,
* which would redeclare them.  Concept names follow the project convention: the
* PascalCase parallel of the trait, with a leading is_ / has_ dropped.
*
*   The whole header is gated to C++20; under earlier standards it is empty and
* callers use the is_callable / is_predicate traits from functional_traits.hpp.
*
* USAGE:
*   template<Predicate<const int&> P>
*   void keep_if(std::vector<int>&, P);
*
*   template<typename F, typename T>
*       requires Callable<F, const T&>
*   auto apply_to(const T&, F);
*
* 
* path:      /inc/djinterp/core/functional/functional_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.06
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CALLABLE CONCEPTS                   (C++20)
      1.  Callable<F, Args...>           (can F be called on Args?)
      2.  Predicate<P, Arg>             (callable on Arg, result -> bool)
*/


#ifndef DJINTERP_FUNCTIONAL_FUNCTIONAL_CONCEPTS_
#define DJINTERP_FUNCTIONAL_FUNCTIONAL_CONCEPTS_ 1

// djinterp
#include "../djinterp.hpp"
#include "./functional_traits.hpp"   // is_callable / is_predicate


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///             I.    CALLABLE CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////

// Callable
//   concept: satisfied when a const-lvalue _Fn can be called on _Args. The
// concept face of is_callable; succeeds on generic lambdas and other templated
// operator() callables.
template<typename _Fn,
         typename... _Args>
concept Callable = is_callable<_Fn, _Args...>::value;

// Predicate
//   concept: satisfied when _Pred is a predicate over _Arg - callable on _Arg
// with a result convertible to bool. The concept face of is_predicate.
template<typename _Pred,
         typename _Arg>
concept Predicate = is_predicate<_Pred, _Arg>::value;

NS_END  // djinterp

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_FUNCTIONAL_FUNCTIONAL_CONCEPTS_
