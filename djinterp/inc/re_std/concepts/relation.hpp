/*******************************************************************************
* djinterp [re_std]                                                 relation.hpp
*
* relation concept header:
*   Rel is a binary predicate over TypeA and TypeB in all four orders.
*
*   All four operand pairings are required so that an algorithm may call the
* relation either way round without further checking.
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
* path:      /inc/re_std/concepts/relation.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_RELATION_HPP
#define RE_STD_CONCEPTS_RELATION_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "predicate.hpp"

namespace re_std
{

// relation
//   concept: Rel is a predicate for every combination of TypeA and TypeB.
template<typename Rel, typename TypeA, typename TypeB>
concept relation
    =  predicate<Rel, TypeA, TypeA>
    && predicate<Rel, TypeB, TypeB>
    && predicate<Rel, TypeA, TypeB>
    && predicate<Rel, TypeB, TypeA>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_RELATION_HPP
