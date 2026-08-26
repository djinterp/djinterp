/******************************************************************************
* re_std [concepts]                                             semiregular.hpp
*
*   _Type is copyable and default-initializable.
*
* The classic "behaves like a built-in type, except for comparison" bundle.
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
* path:      /inc/djinterp/re_std/concepts/semiregular.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_SEMIREGULAR_
#define DJINTERP_RE_STD_CONCEPTS_SEMIREGULAR_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/copyable.hpp"
#include "../concepts/default_initializable.hpp"

NS_RESTD

// semiregular
//   concept: _Type is copyable and default_initializable.
template<typename _Type>
concept semiregular = copyable<_Type> && default_initializable<_Type>;

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_SEMIREGULAR_
