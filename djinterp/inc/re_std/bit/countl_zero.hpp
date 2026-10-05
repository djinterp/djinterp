/*******************************************************************************
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
* path:      /inc/re_std/bit/countl_zero.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BIT_COUNTL_ZERO_HPP
#define RE_STD_BIT_COUNTL_ZERO_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./bit_internal.hpp"


namespace re_std
{


// ===========================================================================
// I.   COUNTL_ZERO
// ===========================================================================

// countl_zero
//   function: leading zero bits. N for a zero operand.
template<typename T>
RE_STD_CONSTEXPR typename internal::bit_enable<T, int>::type
countl_zero(
    T _v
) RE_STD_NOEXCEPT
{
    return internal::bit_digits<T>::value - internal::bit_width_rec<T>(_v);
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BIT_COUNTL_ZERO_HPP
