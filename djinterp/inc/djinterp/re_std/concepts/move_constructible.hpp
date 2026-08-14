/******************************************************************************
* re_std [concepts]                                      move_constructible.hpp
*
*   _Type is constructible from, and convertible from, an rvalue of itself.
*
*   Both halves are required because they can disagree: a type with an
* explicit move constructor is constructible_from<_Type, _Type> but not
* convertible_to<_Type, _Type>, and generic code that returns by value needs the
* conversion, not just the construction.
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
* path:      /inc/djinterp/re_std/concepts/move_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_MOVE_CONSTRUCTIBLE_
#define RESTD_CONCEPTS_MOVE_CONSTRUCTIBLE_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/constructible_from.hpp"
#include "../concepts/convertible_to.hpp"

NS_DJINTERP
NS_RESTD

// move_constructible
//   concept: _Type can be constructed and converted from an rvalue _Type.
template<typename _Type>
concept move_constructible
    = constructible_from<_Type, _Type> && convertible_to<_Type, _Type>;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_MOVE_CONSTRUCTIBLE_
