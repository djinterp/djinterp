/******************************************************************************
* re_std [concepts]                                                integral.hpp
*
*   _Type is an integral type.
*
*   Note this DOES admit bool and the character types, unlike the integer-type
* restriction used by <numeric>'s saturation arithmetic.  The two notions are
* genuinely different and the difference is easy to trip over: integral<char> is
* true, but add_sat('a', 'b') does not compile.
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
* path:      /inc/djinterp/re_std/concepts/integral.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_INTEGRAL_
#define DJINTERP_RE_STD_CONCEPTS_INTEGRAL_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"

NS_RESTD

// integral
//   concept: _Type is an integral type (bool and the character types included).
template<typename _Type>
concept integral = is_integral<_Type>::value;

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_INTEGRAL_
