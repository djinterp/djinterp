/******************************************************************************
* djinterp [re_std]                                              countr_zero.hpp
*
* countr_zero header:
*   Number of consecutive 0 bits starting from the least significant.
* countr_zero(T(0)) is N.
*
*     countr_zero(uint8_t(1))    -> 0
*     countr_zero(uint8_t(8))    -> 3
*     countr_zero(uint8_t(0))    -> 8
*
*   The zero case is tested before the walk rather than inside it: the
* recursive helper terminates on finding a set bit, so a zero operand
* would never terminate. Same hazard as __builtin_ctz, which is also
* undefined at zero, handled here rather than inherited.
*
*   PORTABILITY:
*   C++20 in std, back-ported to C++11 and constexpr from C++11.
*
*
* path:      /inc/djinterp/re_std/bit/countr_zero.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_COUNTR_ZERO_
#define DJINTERP_RE_STD_BIT_COUNTR_ZERO_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"


NS_RESTD


// ===========================================================================
// I.   COUNTR_ZERO
// ===========================================================================

// countr_zero
//   function: trailing zero bits. N for a zero operand, which is checked
// here because the helper cannot terminate on it.
template<typename _T>
D_CONSTEXPR typename internal::bit_enable<_T, int>::type
countr_zero(
    _T _v
) D_NOEXCEPT
{
    return (_v == 0)
        ? internal::bit_digits<_T>::value
        : internal::bit_ctz_rec<_T>(_v, 0);
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_COUNTR_ZERO_
