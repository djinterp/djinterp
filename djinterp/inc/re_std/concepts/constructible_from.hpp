/*******************************************************************************
* djinterp [re_std]                                       constructible_from.hpp
*
* constructible_from concept header:
*   Type is constructible from Args and destructible.
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
* immediate, localised compile error.  Test RE_STD_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/re_std/concepts/constructible_from.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_CONSTRUCTIBLE_FROM_HPP
#define RE_STD_CONCEPTS_CONSTRUCTIBLE_FROM_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "destructible.hpp"

namespace re_std
{

// constructible_from
//   concept: Type is destructible and constructible from Args...
template<typename Type, typename... Args>
concept constructible_from
    = destructible<Type> && is_constructible<Type, Args...>::value;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_CONSTRUCTIBLE_FROM_HPP
