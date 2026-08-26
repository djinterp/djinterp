/******************************************************************************
* djinterp [re_std]                                            ratio_greater.hpp
*
* ratio_greater header:
*   R1 > R2, i.e. R2 < R1 with the operands swapped.
*
*   Defined in terms of ratio_less rather than independently, so
* all four orderings share one overflow-free comparison and cannot
* drift apart.
*
*   PORTABILITY:
*   C++11 in std; the _v spelling is C++17 in std and C++14 here.
*
*
* path:      /inc/djinterp/re_std/ratio/ratio_greater.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_RATIO_RATIO_GREATER_
#define DJINTERP_RE_STD_RATIO_RATIO_GREATER_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./ratio.hpp"
#include "./ratio_less.hpp"
#include "../type_traits/integral_constant.hpp"


NS_RESTD


// ===========================================================================
// I.   RATIO_GREATER
// ===========================================================================

// ratio_greater
//   trait: R1 > R2, i.e. R2 < R1 with the operands swapped.
template<typename _R1,
         typename _R2>
struct ratio_greater
    : integral_constant<bool, ratio_less<_R2, _R1>::value>
{};


// ===========================================================================
// II.  RATIO_GREATER_V (C++14+ variable)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _R1,
         typename _R2>
D_CONSTEXPR bool ratio_greater_v = ratio_greater<_R1, _R2>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RATIO_RATIO_GREATER_
