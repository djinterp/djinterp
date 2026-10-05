/*******************************************************************************
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
* path:      /inc/re_std/ratio/ratio_subtract.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RATIO_RATIO_SUBTRACT_HPP
#define RE_STD_RATIO_RATIO_SUBTRACT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./ratio.hpp"
#include "./ratio_add.hpp"


namespace re_std
{


// ===========================================================================
// I.   RATIO_SUBTRACT
// ===========================================================================

// ratio_subtract
//   alias: the reduced difference of two ratios.
template<typename R1,
         typename R2>
struct ratio_subtract
    : ratio_add< R1, ratio<-R2::num, R2::den> >::type
{};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RATIO_RATIO_SUBTRACT_HPP
