/******************************************************************************
* re_std [concepts]                                                 regular.hpp
*
*   _Type is semiregular and equality-comparable.
*
*   Stepanov's regular type: copyable, default-constructible, and comparable
* for equality.  This is the concept most generic containers actually want.
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
* path:      /inc/djinterp/re_std/concepts/regular.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_REGULAR_
#define RESTD_CONCEPTS_REGULAR_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/semiregular.hpp"
#include "../concepts/equality_comparable.hpp"

NS_DJINTERP
NS_RESTD

// regular
//   concept: _Type is semiregular and equality_comparable.
template<typename _Type>
concept regular = semiregular<_Type> && equality_comparable<_Type>;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_REGULAR_
