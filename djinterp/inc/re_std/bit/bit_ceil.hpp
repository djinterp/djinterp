/*******************************************************************************
* djinterp [re_std]                                                 bit_ceil.hpp
*
* bit_ceil header:
*   The smallest power of two not less than the value. bit_ceil(0) and
* bit_ceil(1) are both 1.
*
*     bit_ceil(0u) -> 1    bit_ceil(5u) -> 8    bit_ceil(8u) -> 8
*
*   THIS ONE CAN OVERFLOW, AND THE STANDARD SAYS SO:
*   When the answer is not representable in T the behaviour is
* undefined -- and, per [bit.pow.two], the call is then not a constant
* expression, so a compile-time use is diagnosed while a run-time use
* is not. bit_ceil(uint8_t(200)) would need 256. No check is added here
* beyond what the standard mandates: a silent clamp would be worse than
* the specified UB, because it would return a wrong answer instead of
* failing.
*
*   The computation is 1 << bit_width(v - 1) rather than a doubling
* loop, so it is a single shift at every width.
*
*   PORTABILITY:
*   C++20 in std, back-ported to C++11 and constexpr from C++11.
*
*
* path:      /inc/re_std/bit/bit_ceil.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BIT_BIT_CEIL_HPP
#define RE_STD_BIT_BIT_CEIL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./bit_internal.hpp"


namespace re_std
{


// ===========================================================================
// I.   BIT_CEIL
// ===========================================================================

// bit_ceil
//   function: least power of two >= _v; 1 for _v of 0 or 1. Undefined
// when the result is not representable in T -- see the header note.
template<typename T>
RE_STD_CONSTEXPR typename internal::bit_enable<T>::type
bit_ceil(
    T _v
) RE_STD_NOEXCEPT
{
    return (_v <= 1)
        ? static_cast<T>(1)
        : static_cast<T>( static_cast<T>(1)
              << internal::bit_width_rec<T>(static_cast<T>(_v - 1)) );
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BIT_BIT_CEIL_HPP
