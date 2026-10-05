/*******************************************************************************
* djinterp [re_std]                                                bit_floor.hpp
*
* bit_floor header:
*   The largest power of two not greater than the value; 0 for a zero
* operand.
*
*     bit_floor(0u) -> 0    bit_floor(5u) -> 4    bit_floor(8u) -> 8
*
*   Unlike bit_ceil this can never overflow: the answer is always <= the
* input, so it is representable whenever the input is.
*
*   PORTABILITY:
*   C++20 in std, back-ported to C++11 and constexpr from C++11.
*
*
* path:      /inc/re_std/bit/bit_floor.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BIT_BIT_FLOOR_HPP
#define RE_STD_BIT_BIT_FLOOR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./bit_internal.hpp"


namespace re_std
{


// ===========================================================================
// I.   BIT_FLOOR
// ===========================================================================

// bit_floor
//   function: greatest power of two <= _v; 0 when _v is 0. Cannot
// overflow -- the result never exceeds the operand.
template<typename T>
RE_STD_CONSTEXPR typename internal::bit_enable<T>::type
bit_floor(
    T _v
) RE_STD_NOEXCEPT
{
    return (_v == 0)
        ? static_cast<T>(0)
        : static_cast<T>( static_cast<T>(1)
              << (internal::bit_width_rec<T>(_v) - 1) );
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BIT_BIT_FLOOR_HPP
