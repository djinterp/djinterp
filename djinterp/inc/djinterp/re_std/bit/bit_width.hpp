/******************************************************************************
* djinterp [re_std]                                                bit_width.hpp
*
* bit_width header:
*   The number of bits needed to represent the value: 0 for zero,
* otherwise one more than the index of the highest set bit.
*
*     bit_width(0u) -> 0    bit_width(1u) -> 1    bit_width(255u) -> 8
*
*   This is the primitive the rest of the module leans on -- countl_zero
* is N minus this, bit_floor is one shift from it, and bit_ceil is one
* shift from it applied to v-1.
*
*   Constrained to the unsigned integer types; see bit_internal.hpp for
* why that is spelled out rather than derived from is_unsigned.
*
*   PORTABILITY:
*   std added it in C++20 as constexpr; re_std back-ports to C++11 and
* is constexpr from C++11.
*
*
* path:      /inc/djinterp/re_std/bit/bit_width.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_BIT_WIDTH_
#define DJINTERP_RE_STD_BIT_BIT_WIDTH_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"


NS_RESTD


// ===========================================================================
// I.   BIT_WIDTH
// ===========================================================================

// bit_width
//   function: bits needed to represent _v; 0 when _v is 0.
template<typename _T>
D_CONSTEXPR typename internal::bit_enable<_T, int>::type
bit_width(
    _T _v
) D_NOEXCEPT
{
    return internal::bit_width_rec<_T>(_v);
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_BIT_WIDTH_
