/*******************************************************************************
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
* path:      /inc/re_std/ratio/ratio_divide.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RATIO_RATIO_DIVIDE_HPP
#define RE_STD_RATIO_RATIO_DIVIDE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./ratio.hpp"
#include "./ratio_multiply.hpp"


namespace re_std
{


// ===========================================================================
// I.   RATIO_DIVIDE
// ===========================================================================

// ratio_divide
//   alias: the reduced quotient of two ratios. A divisor with a zero
// numerator is rejected by ratio's denominator assert.
template<typename R1,
         typename R2>
struct ratio_divide
    : ratio_multiply< R1, ratio<R2::den, R2::num> >::type
{};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RATIO_RATIO_DIVIDE_HPP
