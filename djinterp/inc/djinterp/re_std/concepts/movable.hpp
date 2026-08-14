/******************************************************************************
* re_std [concepts]                                                 movable.hpp
*
*   _Type is an object type that can be moved and swapped.
*
*   is_object excludes references, functions and void, which is what makes
* this a concept about VALUES rather than about arbitrary types.
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
* path:      /inc/djinterp/re_std/concepts/movable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_MOVABLE_
#define RESTD_CONCEPTS_MOVABLE_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/move_constructible.hpp"
#include "../concepts/assignable_from.hpp"
#include "../concepts/swappable.hpp"

NS_DJINTERP
NS_RESTD

// movable
//   concept: _Type is an object type, move-constructible, move-assignable
// and swappable.
template<typename _Type>
concept movable
    =  is_object<_Type>::value
    && move_constructible<_Type>
    && assignable_from<_Type&, _Type>
    && swappable<_Type>;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_MOVABLE_
