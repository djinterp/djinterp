/******************************************************************************
* re_std [concepts]                                          swappable_with.hpp
*
*   _TypeA and _TypeB are mutually swappable.
*
*   All four combinations are required, including each type with itself.
* That is not over-testing: heterogeneous swap is only coherent if both operands
* are individually swappable and the cross-swaps agree, and the
* common_reference_with conjunct is what ties the two types together as denoting
* the same underlying object domain.
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
* path:      /inc/djinterp/re_std/concepts/swappable_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_SWAPPABLE_WITH_
#define RESTD_CONCEPTS_SWAPPABLE_WITH_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/common_reference_with.hpp"
#include "../concepts/ranges_swap.hpp"

NS_DJINTERP
NS_RESTD

// swappable_with
//   concept: _TypeA and _TypeB can be swapped with themselves and each other.
template<typename _TypeA, typename _TypeB>
concept swappable_with
    =  common_reference_with<_TypeA, _TypeB>
    && requires(_TypeA&& a, _TypeB&& b)
       {
           ranges::swap(static_cast<_TypeA&&>(a), static_cast<_TypeA&&>(a));
           ranges::swap(static_cast<_TypeB&&>(b), static_cast<_TypeB&&>(b));
           ranges::swap(static_cast<_TypeA&&>(a), static_cast<_TypeB&&>(b));
           ranges::swap(static_cast<_TypeB&&>(b), static_cast<_TypeA&&>(a));
       };

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_SWAPPABLE_WITH_
