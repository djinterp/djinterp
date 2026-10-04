/******************************************************************************
* re_std [concepts]                                         signed_integral.hpp
*
*   _Type is a signed integral type.
*
*   is_signed alone is not enough: it is also true for the floating-point
* types, so the integral conjunct is doing real work.
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
* path:      /inc/djinterp/re_std/concepts/signed_integral.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_SIGNED_INTEGRAL_
#define DJINTERP_RE_STD_CONCEPTS_SIGNED_INTEGRAL_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/integral.hpp"

NS_RESTD

// signed_integral
//   concept: _Type is integral and signed.
template<typename _Type>
concept signed_integral = integral<_Type> && is_signed<_Type>::value;

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_SIGNED_INTEGRAL_
