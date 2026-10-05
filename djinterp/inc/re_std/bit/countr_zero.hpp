/*******************************************************************************
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
* path:      /inc/re_std/bit/countr_zero.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BIT_COUNTR_ZERO_HPP
#define RE_STD_BIT_COUNTR_ZERO_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./bit_internal.hpp"


namespace re_std
{


// ===========================================================================
// I.   COUNTR_ZERO
// ===========================================================================

// countr_zero
//   function: trailing zero bits. N for a zero operand, which is checked
// here because the helper cannot terminate on it.
template<typename T>
RE_STD_CONSTEXPR typename internal::bit_enable<T, int>::type
countr_zero(
    T _v
) RE_STD_NOEXCEPT
{
    return (_v == 0)
        ? internal::bit_digits<T>::value
        : internal::bit_ctz_rec<T>(_v, 0);
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BIT_COUNTR_ZERO_HPP
