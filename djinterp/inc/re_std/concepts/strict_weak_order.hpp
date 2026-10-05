/*******************************************************************************
* djinterp [re_std]                                        strict_weak_order.hpp
*
* strict_weak_order concept header:
*   Rel is a relation that is (semantically) a strict weak ordering.
*
*   Syntactically identical to relation.  This is the one whose semantic
* requirement is most often violated in practice - a comparator that is not
* irreflexive makes sort undefined behaviour - and no amount of concept checking
* will catch it.
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
* path:      /inc/re_std/concepts/strict_weak_order.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_STRICT_WEAK_ORDER_HPP
#define RE_STD_CONCEPTS_STRICT_WEAK_ORDER_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "relation.hpp"

namespace re_std
{

// strict_weak_order
//   concept: relation, semantically required to be a strict weak ordering.
template<typename Rel, typename TypeA, typename TypeB>
concept strict_weak_order = relation<Rel, TypeA, TypeB>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_STRICT_WEAK_ORDER_HPP
