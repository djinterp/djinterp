/*******************************************************************************
* djinterp [re_std]                                           swappable_with.hpp
*
* swappable_with concept header:
*   TypeA and TypeB are mutually swappable.
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
* immediate, localised compile error.  Test RE_STD_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/re_std/concepts/swappable_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_SWAPPABLE_WITH_HPP
#define RE_STD_CONCEPTS_SWAPPABLE_WITH_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "common_reference_with.hpp"
#include "ranges_swap.hpp"

namespace re_std
{

// swappable_with
//   concept: TypeA and TypeB can be swapped with themselves and each other.
template<typename TypeA, typename TypeB>
concept swappable_with
    =  common_reference_with<TypeA, TypeB>
    && requires(TypeA&& a, TypeB&& b)
       {
           ranges::swap(static_cast<TypeA&&>(a), static_cast<TypeA&&>(a));
           ranges::swap(static_cast<TypeB&&>(b), static_cast<TypeB&&>(b));
           ranges::swap(static_cast<TypeA&&>(a), static_cast<TypeB&&>(b));
           ranges::swap(static_cast<TypeB&&>(b), static_cast<TypeA&&>(a));
       };

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_SWAPPABLE_WITH_HPP
