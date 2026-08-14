/******************************************************************************
* re_std [concepts]                                      constructible_from.hpp
*
*   _Type is constructible from _Args and destructible.
*
*   The destructible conjunct matters more than it looks: constructing an
* object you cannot safely destroy is not usable in any container or algorithm,
* so std folds the requirement in here rather than repeating it at every use.
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
* path:      /inc/djinterp/re_std/concepts/constructible_from.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_CONSTRUCTIBLE_FROM_
#define RESTD_CONCEPTS_CONSTRUCTIBLE_FROM_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/destructible.hpp"

NS_DJINTERP
NS_RESTD

// constructible_from
//   concept: _Type is destructible and constructible from _Args...
template<typename _Type, typename... _Args>
concept constructible_from
    = destructible<_Type> && is_constructible<_Type, _Args...>::value;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_CONSTRUCTIBLE_FROM_
