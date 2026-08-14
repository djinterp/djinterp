/******************************************************************************
* re_std [concepts]                                            destructible.hpp
*
*   _Type can be destroyed without throwing.
*
*   Defined on is_NOTHROW_destructible, not is_destructible.  std made that
* choice deliberately: a throwing destructor makes almost every generic algorithm
* unable to give any exception guarantee at all, so the whole library simply
* declines to work with such types rather than degrading silently.
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
* path:      /inc/djinterp/re_std/concepts/destructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_DESTRUCTIBLE_
#define RESTD_CONCEPTS_DESTRUCTIBLE_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"

NS_DJINTERP
NS_RESTD

// destructible
//   concept: _Type has a non-throwing destructor.
template<typename _Type>
concept destructible = is_nothrow_destructible<_Type>::value;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_DESTRUCTIBLE_
