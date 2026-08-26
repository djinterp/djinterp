/******************************************************************************
* re_std [concepts]                                                 same_as.hpp
*
*   T and U are the same type, cv-qualification included.
*
*   Written as a two-way conjunction of an internal helper rather than a bare
* is_same_v.  That is not redundancy: subsumption compares NORMALISED constraint
* expressions, and the symmetric form is what lets `same_as<T, U>` and
* `same_as<U, T>` subsume each other, so an overload constrained on one is not
* ambiguous against an overload constrained on the other.  A single-atom
* definition would break every overload set that relies on it.
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
* path:      /inc/djinterp/re_std/concepts/same_as.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_SAME_AS_
#define DJINTERP_RE_STD_CONCEPTS_SAME_AS_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"

NS_RESTD

NS_INTERNAL

    template<typename _TypeA, typename _TypeB>
    concept same_as_impl = is_same<_TypeA, _TypeB>::value;

NS_END  // internal

// same_as
//   concept: _TypeA and _TypeB name the same type.
template<typename _TypeA, typename _TypeB>
concept same_as
    =  internal::same_as_impl<_TypeA, _TypeB>
    && internal::same_as_impl<_TypeB, _TypeA>;

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_SAME_AS_
