/******************************************************************************
* djinterp [re_std]                                          ratio_not_equal.hpp
*
* ratio_not_equal header:
*   The negation of ratio_equal.
*
*   PORTABILITY:
*   C++11 in std; the _v spelling is C++17 in std and C++14 here.
*
*
* path:      /inc/djinterp/re_std/ratio/ratio_not_equal.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_RATIO_RATIO_NOT_EQUAL_
#define DJINTERP_RE_STD_RATIO_RATIO_NOT_EQUAL_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./ratio.hpp"
#include "./ratio_equal.hpp"
#include "../type_traits/integral_constant.hpp"


NS_RESTD


// ===========================================================================
// I.   RATIO_NOT_EQUAL
// ===========================================================================

// ratio_not_equal
//   trait: the complement of ratio_equal.
template<typename _R1,
         typename _R2>
struct ratio_not_equal
    : integral_constant<bool, !ratio_equal<_R1, _R2>::value>
{};


// ===========================================================================
// II.  RATIO_NOT_EQUAL_V (C++14+ variable)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _R1,
         typename _R2>
D_CONSTEXPR bool ratio_not_equal_v = ratio_not_equal<_R1, _R2>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RATIO_RATIO_NOT_EQUAL_
