/*******************************************************************************
* djinterp [re_std]                                       move_constructible.hpp
*
* move_constructible concept header:
*   Type is constructible from, and convertible from, an rvalue of itself.
*
*   Both halves are required because they can disagree: a type with an
* explicit move constructor is constructible_from<Type, Type> but not
* convertible_to<Type, Type>, and generic code that returns by value needs the
* conversion, not just the construction.
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
* path:      /inc/re_std/concepts/move_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_MOVE_CONSTRUCTIBLE_HPP
#define RE_STD_CONCEPTS_MOVE_CONSTRUCTIBLE_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "constructible_from.hpp"
#include "convertible_to.hpp"

namespace re_std
{

// move_constructible
//   concept: Type can be constructed and converted from an rvalue Type.
template<typename Type>
concept move_constructible
    = constructible_from<Type, Type> && convertible_to<Type, Type>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_MOVE_CONSTRUCTIBLE_HPP
