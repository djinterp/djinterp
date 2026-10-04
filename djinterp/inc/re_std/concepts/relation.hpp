/******************************************************************************
* re_std [concepts]                                                relation.hpp
*
*   _Rel is a binary predicate over _TypeA and _TypeB in all four orders.
*
*   All four operand pairings are required so that an algorithm may call the
* relation either way round without further checking.
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
* path:      /inc/djinterp/re_std/concepts/relation.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_RELATION_
#define DJINTERP_RE_STD_CONCEPTS_RELATION_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/predicate.hpp"

NS_RESTD

// relation
//   concept: _Rel is a predicate for every combination of _TypeA and _TypeB.
template<typename _Rel, typename _TypeA, typename _TypeB>
concept relation
    =  predicate<_Rel, _TypeA, _TypeA>
    && predicate<_Rel, _TypeB, _TypeB>
    && predicate<_Rel, _TypeA, _TypeB>
    && predicate<_Rel, _TypeB, _TypeA>;

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_RELATION_
