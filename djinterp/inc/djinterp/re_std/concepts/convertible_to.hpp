/******************************************************************************
* re_std [concepts]                                          convertible_to.hpp
*
*   _From converts to _To both implicitly and explicitly.
*
*   Both directions are required because they can disagree.  A type with an
* explicit conversion operator is static_cast-able but not implicitly
* convertible; a type with an implicit conversion whose result is ambiguous under
* static_cast is the reverse.  std requires both, so a template constrained on
* convertible_to can use either syntax without further checking.
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
* path:      /inc/djinterp/re_std/concepts/convertible_to.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_CONVERTIBLE_TO_
#define DJINTERP_RE_STD_CONCEPTS_CONVERTIBLE_TO_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"

NS_RESTD

// convertible_to
//   concept: _From is both implicitly and explicitly convertible to _To.
template<typename _From, typename _To>
concept convertible_to
    =  is_convertible<_From, _To>::value
    && requires { static_cast<_To>(declval<_From>()); };

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_CONVERTIBLE_TO_
