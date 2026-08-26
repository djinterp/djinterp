/******************************************************************************
* djinterp [re_std]                                                 bit_ceil.hpp
*
* bit_ceil header:
*   The smallest power of two not less than the value. bit_ceil(0) and
* bit_ceil(1) are both 1.
*
*     bit_ceil(0u) -> 1    bit_ceil(5u) -> 8    bit_ceil(8u) -> 8
*
*   THIS ONE CAN OVERFLOW, AND THE STANDARD SAYS SO:
*   When the answer is not representable in _T the behaviour is
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
* path:      /inc/djinterp/re_std/bit/bit_ceil.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_BIT_CEIL_
#define DJINTERP_RE_STD_BIT_BIT_CEIL_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"


NS_RESTD


// ===========================================================================
// I.   BIT_CEIL
// ===========================================================================

// bit_ceil
//   function: least power of two >= _v; 1 for _v of 0 or 1. Undefined
// when the result is not representable in _T -- see the header note.
template<typename _T>
D_CONSTEXPR typename internal::bit_enable<_T>::type
bit_ceil(
    _T _v
) D_NOEXCEPT
{
    return (_v <= 1)
        ? static_cast<_T>(1)
        : static_cast<_T>( static_cast<_T>(1)
              << internal::bit_width_rec<_T>(static_cast<_T>(_v - 1)) );
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_BIT_CEIL_
