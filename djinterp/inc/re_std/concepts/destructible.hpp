/*******************************************************************************
* djinterp [re_std]                                             destructible.hpp
*
* destructible concept header:
*   Type can be destroyed without throwing.
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
* immediate, localised compile error.  Test RE_STD_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/re_std/concepts/destructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_DESTRUCTIBLE_HPP
#define RE_STD_CONCEPTS_DESTRUCTIBLE_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"

namespace re_std
{

// destructible
//   concept: Type has a non-throwing destructor.
template<typename Type>
concept destructible = is_nothrow_destructible<Type>::value;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_DESTRUCTIBLE_HPP
