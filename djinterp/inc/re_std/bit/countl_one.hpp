/******************************************************************************
* djinterp [re_std]                                               countl_one.hpp
*
* countl_one header:
*   Number of consecutive 1 bits starting from the most significant.
*
*   Implemented as countl_zero of the complement. The cast back to _T
* after ~ is load-bearing for narrow types: ~uint8_t(0) promotes to
* int, giving -1 rather than 255, and counting leading zeros of that
* would answer 0 for the wrong reason. Casting back re-truncates to the
* operand width first.
*
*   PORTABILITY:
*   C++20 in std, back-ported to C++11 and constexpr from C++11.
*
*
* path:      /inc/djinterp/re_std/bit/countl_one.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_COUNTL_ONE_
#define DJINTERP_RE_STD_BIT_COUNTL_ONE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"
#include "./countl_zero.hpp"


NS_RESTD


// ===========================================================================
// I.   COUNTL_ONE
// ===========================================================================

// countl_one
//   function: leading one bits.
template<typename _T>
D_CONSTEXPR typename internal::bit_enable<_T, int>::type
countl_one(
    _T _v
) D_NOEXCEPT
{
    // the cast re-truncates the promoted complement to _T's width
    return re_std::countl_zero(static_cast<_T>(~_v));
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_COUNTL_ONE_
