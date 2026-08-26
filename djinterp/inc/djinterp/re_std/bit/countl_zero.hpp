/******************************************************************************
* djinterp [re_std]                                              countl_zero.hpp
*
* countl_zero header:
*   Number of consecutive 0 bits starting from the most significant.
* countl_zero(T(0)) is N, the full width.
*
*     countl_zero(uint8_t(0))    -> 8
*     countl_zero(uint8_t(1))    -> 7
*     countl_zero(uint8_t(0x80)) -> 0
*
*   Computed as N - bit_width, which sidesteps the trap in the obvious
* intrinsic implementation: __builtin_clz is UNDEFINED for a zero
* argument, and zero is the case callers most often pass. Deriving it
* from bit_width has no such edge -- bit_width(0) is 0, so the answer
* is N.
*
*   N is numeric_limits<T>::digits, so a narrow type gets its own width
* rather than the promoted one -- countl_zero(uint8_t(1)) is 7, not 31.
*
*   PORTABILITY:
*   C++20 in std, back-ported to C++11 and constexpr from C++11.
*
*
* path:      /inc/djinterp/re_std/bit/countl_zero.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_COUNTL_ZERO_
#define DJINTERP_RE_STD_BIT_COUNTL_ZERO_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"


NS_RESTD


// ===========================================================================
// I.   COUNTL_ZERO
// ===========================================================================

// countl_zero
//   function: leading zero bits. N for a zero operand.
template<typename _T>
D_CONSTEXPR typename internal::bit_enable<_T, int>::type
countl_zero(
    _T _v
) D_NOEXCEPT
{
    return internal::bit_digits<_T>::value - internal::bit_width_rec<_T>(_v);
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_COUNTL_ZERO_
