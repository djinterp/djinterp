/******************************************************************************
* re_std [concepts]                                       unsigned_integral.hpp
*
*   _Type is an unsigned integral type.
*
*   Defined as integral-and-not-signed rather than integral-and-is_unsigned so
* that it is exactly the complement of signed_integral over the integral types.
* The two spellings differ for bool, where is_signed and is_unsigned can both be
* surprising.
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
* path:      /inc/djinterp/re_std/concepts/unsigned_integral.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_UNSIGNED_INTEGRAL_
#define RESTD_CONCEPTS_UNSIGNED_INTEGRAL_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/integral.hpp"
#include "../concepts/signed_integral.hpp"

NS_DJINTERP
NS_RESTD

// unsigned_integral
//   concept: _Type is integral and not signed.
template<typename _Type>
concept unsigned_integral = integral<_Type> && !signed_integral<_Type>;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_UNSIGNED_INTEGRAL_
