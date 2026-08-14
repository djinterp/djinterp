/******************************************************************************
* re_std [concepts]                                    equivalence_relation.hpp
*
*   _Rel is a relation that is (semantically) an equivalence.
*
*   Syntactically identical to relation; reflexivity, symmetry and
* transitivity are semantic requirements no compiler can verify.  Named
* separately so callers can state which they mean.
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
* path:      /inc/djinterp/re_std/concepts/equivalence_relation.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_EQUIVALENCE_RELATION_
#define RESTD_CONCEPTS_EQUIVALENCE_RELATION_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/relation.hpp"

NS_DJINTERP
NS_RESTD

// equivalence_relation
//   concept: relation, semantically required to be an equivalence relation.
template<typename _Rel, typename _TypeA, typename _TypeB>
concept equivalence_relation = relation<_Rel, _TypeA, _TypeB>;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_EQUIVALENCE_RELATION_
