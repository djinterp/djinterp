/******************************************************************************
* re_std [concepts]                                    totally_ordered_with.hpp
*
*   _TypeA and _TypeB are mutually totally ordered.
*
*   Same C++20-versus-C++23 caveat as equality_comparable_with: P2404R3
* retargeted the common-type conjunct, and re_std ships the C++20 form.
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
* path:      /inc/djinterp/re_std/concepts/totally_ordered_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_TOTALLY_ORDERED_WITH_
#define RESTD_CONCEPTS_TOTALLY_ORDERED_WITH_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/totally_ordered.hpp"
#include "../concepts/equality_comparable_with.hpp"

NS_DJINTERP
NS_RESTD

// totally_ordered_with
//   concept: both types are totally_ordered and mutually ordered.
template<typename _TypeA, typename _TypeB>
concept totally_ordered_with
    =  totally_ordered<_TypeA>
    && totally_ordered<_TypeB>
    && equality_comparable_with<_TypeA, _TypeB>
    && totally_ordered<
           typename common_reference<
               const typename remove_reference<_TypeA>::type&,
               const typename remove_reference<_TypeB>::type&>::type>
    && internal::partially_ordered_with<_TypeA, _TypeB>;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_TOTALLY_ORDERED_WITH_
