/******************************************************************************
* djinterp [re_std]                                             ratio_divide.hpp
*
* ratio_divide header:
*   ratio_divide<R1, R2> is the reduced quotient R1 / R2.
*
*   Implemented as R1 multiplied by R2 inverted, which inherits
* ratio_multiply's cross-reduction and therefore its overflow
* behaviour -- there is no second algorithm to get wrong.
*
*   Inverting R2 puts its numerator in a denominator position, so a
* zero-numerator divisor becomes a zero denominator and is caught by
* ratio's own static_assert. The diagnostic names ratio rather than
* ratio_divide, which is worth knowing when reading the error.
*
*   PORTABILITY:
*   C++11, matching std.
*
*
* path:      /inc/djinterp/re_std/ratio/ratio_divide.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_RATIO_RATIO_DIVIDE_
#define DJINTERP_RE_STD_RATIO_RATIO_DIVIDE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./ratio.hpp"
#include "./ratio_multiply.hpp"


NS_RESTD


// ===========================================================================
// I.   RATIO_DIVIDE
// ===========================================================================

// ratio_divide
//   alias: the reduced quotient of two ratios. A divisor with a zero
// numerator is rejected by ratio's denominator assert.
template<typename _R1,
         typename _R2>
struct ratio_divide
    : ratio_multiply< _R1, ratio<_R2::den, _R2::num> >::type
{};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RATIO_RATIO_DIVIDE_
