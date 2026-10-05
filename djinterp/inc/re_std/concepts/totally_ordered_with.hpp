/*******************************************************************************
* djinterp [re_std]                                     totally_ordered_with.hpp
*
* totally_ordered_with concept header:
*   TypeA and TypeB are mutually totally ordered.
*
*   Same C++20-versus-C++23 caveat as equality_comparable_with: P2404R3
* retargeted the common-type conjunct, and re_std ships the C++20 form.
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
* path:      /inc/re_std/concepts/totally_ordered_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_TOTALLY_ORDERED_WITH_HPP
#define RE_STD_CONCEPTS_TOTALLY_ORDERED_WITH_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "totally_ordered.hpp"
#include "equality_comparable_with.hpp"

namespace re_std
{

// totally_ordered_with
//   concept: both types are totally_ordered and mutually ordered.
template<typename TypeA, typename TypeB>
concept totally_ordered_with
    =  totally_ordered<TypeA>
    && totally_ordered<TypeB>
    && equality_comparable_with<TypeA, TypeB>
    && totally_ordered<
           typename common_reference<
               const typename remove_reference<TypeA>::type&,
               const typename remove_reference<TypeB>::type&>::type>
    && internal::partially_ordered_with<TypeA, TypeB>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_TOTALLY_ORDERED_WITH_HPP
