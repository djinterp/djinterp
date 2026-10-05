/*******************************************************************************
* djinterp [re_std]                                                swappable.hpp
*
* swappable concept header:
*   Type is swappable with itself via re_std::ranges::swap.
*
*   Expressed through the ranges::swap CPO rather than an unqualified swap
* call, so a type that only provides a hidden-friend swap is still recognised.
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
* path:      /inc/re_std/concepts/swappable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_SWAPPABLE_HPP
#define RE_STD_CONCEPTS_SWAPPABLE_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "ranges_swap.hpp"

namespace re_std
{

// swappable
//   concept: two Type lvalues can be swapped.
template<typename Type>
concept swappable
    = requires(Type& a, Type& b) { ranges::swap(a, b); };

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_SWAPPABLE_HPP
