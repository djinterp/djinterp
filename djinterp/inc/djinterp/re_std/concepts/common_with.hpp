/******************************************************************************
* re_std [concepts]                                             common_with.hpp
*
*   _TypeA and _TypeB share a common type they both convert to.
*
*   Stronger than common_reference_with, and the last two clauses are the
* reason: they force the common TYPE and the common REFERENCE to agree.  Without
* them a pair of types could have a common_type that is unrelated to their
* common_reference, and generic code that mixes value and reference contexts
* would silently pick different types in different places.
*
*   C++20 ONLY - AND THAT IS NOT A GAP.
*   `concept` is a core language keyword with no builtin behind it, so unlike
* re_std's intrinsic-backed traits there is nothing to detect and nothing to
* back-port.  Below C++20 this header is EMPTY rather than degraded: a concept
* that does not exist cannot give a wrong answer, and naming one is an
* immediate, localised compile error.  Test D_ENV_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/djinterp/re_std/concepts/common_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_COMMON_WITH_
#define DJINTERP_RE_STD_CONCEPTS_COMMON_WITH_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../concepts/same_as.hpp"
#include "../concepts/common_reference_with.hpp"

NS_RESTD

// common_with
//   concept: both types convert to a shared common type, consistently with
// their common reference type.
template<typename _TypeA, typename _TypeB>
concept common_with
    =  same_as<typename common_type<_TypeA, _TypeB>::type,
               typename common_type<_TypeB, _TypeA>::type>
    && requires {
           static_cast<typename common_type<_TypeA, _TypeB>::type>(
               declval<_TypeA>());
           static_cast<typename common_type<_TypeA, _TypeB>::type>(
               declval<_TypeB>());
       }
    && common_reference_with<
           typename add_lvalue_reference<const _TypeA>::type,
           typename add_lvalue_reference<const _TypeB>::type>
    && common_reference_with<
           typename add_lvalue_reference<
               typename common_type<_TypeA, _TypeB>::type>::type,
           typename common_reference<
               typename add_lvalue_reference<const _TypeA>::type,
               typename add_lvalue_reference<const _TypeB>::type>::type>;

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_COMMON_WITH_
