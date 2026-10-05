/*******************************************************************************
* djinterp [re_std]                                          signed_integral.hpp
*
* signed_integral concept header:
*   Type is a signed integral type.
*
*   is_signed alone is not enough: it is also true for the floating-point
* types, so the integral conjunct is doing real work.
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
* path:      /inc/re_std/concepts/signed_integral.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_SIGNED_INTEGRAL_HPP
#define RE_STD_CONCEPTS_SIGNED_INTEGRAL_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "integral.hpp"

namespace re_std
{

// signed_integral
//   concept: Type is integral and signed.
template<typename Type>
concept signed_integral = integral<Type> && is_signed<Type>::value;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_SIGNED_INTEGRAL_HPP
