/******************************************************************************
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
* path:      /inc/djinterp/re_std/bit/bit_floor.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_BIT_FLOOR_
#define DJINTERP_RE_STD_BIT_BIT_FLOOR_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"


NS_RESTD


// ===========================================================================
// I.   BIT_FLOOR
// ===========================================================================

// bit_floor
//   function: greatest power of two <= _v; 0 when _v is 0. Cannot
// overflow -- the result never exceeds the operand.
template<typename _T>
D_CONSTEXPR typename internal::bit_enable<_T>::type
bit_floor(
    _T _v
) D_NOEXCEPT
{
    return (_v == 0)
        ? static_cast<_T>(0)
        : static_cast<_T>( static_cast<_T>(1)
              << (internal::bit_width_rec<_T>(_v) - 1) );
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_BIT_FLOOR_
