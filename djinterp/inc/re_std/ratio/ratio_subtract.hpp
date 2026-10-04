/******************************************************************************
* djinterp [re_std]                                           ratio_subtract.hpp
*
* ratio_subtract header:
*   ratio_subtract<R1, R2> is the reduced difference R1 - R2.
*
*   Implemented as R1 + (-R2), so it inherits ratio_add's
* denominator-gcd reduction rather than repeating it.
*
*   Negating R2::num is safe because ratio rejects the most-negative
* intmax_t at definition, so no reduced numerator can be un-negatable.
* That assert is what this file quietly depends on.
*
*   PORTABILITY:
*   C++11, matching std.
*
*
* path:      /inc/djinterp/re_std/ratio/ratio_subtract.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_RATIO_RATIO_SUBTRACT_
#define DJINTERP_RE_STD_RATIO_RATIO_SUBTRACT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./ratio.hpp"
#include "./ratio_add.hpp"


NS_RESTD


// ===========================================================================
// I.   RATIO_SUBTRACT
// ===========================================================================

// ratio_subtract
//   alias: the reduced difference of two ratios.
template<typename _R1,
         typename _R2>
struct ratio_subtract
    : ratio_add< _R1, ratio<-_R2::num, _R2::den> >::type
{};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RATIO_RATIO_SUBTRACT_
