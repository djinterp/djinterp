/******************************************************************************
* djinterp [re_std]                                              ratio_equal.hpp
*
* ratio_equal header:
*   ratio_equal<R1, R2> is true_type iff the two ratios denote the
* same rational.
*
*   This is a MEMBER COMPARISON, not a cross-multiplication, and that
* is only correct because ratio normalises at definition: num and den
* are always the reduced form with a positive denominator, so equal
* rationals are the same type. ratio_equal<ratio<1,2>, ratio<2,4>> is
* true because ratio<2,4>::num is 1 and ::den is 2 -- there is nothing
* left to reduce.
*
*   It costs no multiplication, so it cannot overflow.
*
*   PORTABILITY:
*   C++11 in std. The _v spelling is C++17 in std; re_std exposes it
* from C++14, where variable templates arrive.
*
*
* path:      /inc/djinterp/re_std/ratio/ratio_equal.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_RATIO_RATIO_EQUAL_
#define DJINTERP_RE_STD_RATIO_RATIO_EQUAL_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./ratio.hpp"
#include "../type_traits/integral_constant.hpp"
#include "../type_traits/true_type.hpp"
#include "../type_traits/false_type.hpp"


NS_RESTD


// ===========================================================================
// I.   RATIO_EQUAL
// ===========================================================================

// ratio_equal
//   trait: whether two ratios denote the same rational.
template<typename _R1,
         typename _R2>
struct ratio_equal
    : integral_constant<bool,
        ( _R1::num == _R2::num && _R1::den == _R2::den )>
{};


// ===========================================================================
// II.  RATIO_EQUAL_V (C++14+ variable)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _R1,
         typename _R2>
D_CONSTEXPR bool ratio_equal_v = ratio_equal<_R1, _R2>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RATIO_RATIO_EQUAL_
