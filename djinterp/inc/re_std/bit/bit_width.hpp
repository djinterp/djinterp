/*******************************************************************************
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
* path:      /inc/re_std/bit/bit_width.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BIT_BIT_WIDTH_HPP
#define RE_STD_BIT_BIT_WIDTH_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./bit_internal.hpp"


namespace re_std
{


// ===========================================================================
// I.   BIT_WIDTH
// ===========================================================================

// bit_width
//   function: bits needed to represent _v; 0 when _v is 0.
template<typename T>
RE_STD_CONSTEXPR typename internal::bit_enable<T, int>::type
bit_width(
    T _v
) RE_STD_NOEXCEPT
{
    return internal::bit_width_rec<T>(_v);
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BIT_BIT_WIDTH_HPP
