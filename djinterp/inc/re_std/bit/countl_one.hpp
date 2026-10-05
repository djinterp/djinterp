/*******************************************************************************
* djinterp [re_std]                                               countl_one.hpp
*
* countl_one header:
*   Number of consecutive 1 bits starting from the most significant.
*
*   Implemented as countl_zero of the complement. The cast back to T
* after ~ is load-bearing for narrow types: ~uint8_t(0) promotes to
* int, giving -1 rather than 255, and counting leading zeros of that
* would answer 0 for the wrong reason. Casting back re-truncates to the
* operand width first.
*
*   PORTABILITY:
*   C++20 in std, back-ported to C++11 and constexpr from C++11.
*
*
* path:      /inc/re_std/bit/countl_one.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BIT_COUNTL_ONE_HPP
#define RE_STD_BIT_COUNTL_ONE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./bit_internal.hpp"
#include "./countl_zero.hpp"


namespace re_std
{


// ===========================================================================
// I.   COUNTL_ONE
// ===========================================================================

// countl_one
//   function: leading one bits.
template<typename T>
RE_STD_CONSTEXPR typename internal::bit_enable<T, int>::type
countl_one(
    T _v
) RE_STD_NOEXCEPT
{
    // the cast re-truncates the promoted complement to T's width
    return re_std::countl_zero(static_cast<T>(~_v));
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BIT_COUNTL_ONE_HPP
