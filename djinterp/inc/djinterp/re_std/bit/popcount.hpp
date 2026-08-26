/******************************************************************************
* djinterp [re_std]                                                 popcount.hpp
*
* popcount header:
*   Number of 1 bits in the value.
*
*     popcount(uint8_t(0))    -> 0
*     popcount(uint8_t(0xFF)) -> 8
*
*   PORTABILITY:
*   C++20 in std, back-ported to C++11 and constexpr from C++11.
*
*
* path:      /inc/djinterp/re_std/bit/popcount.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_POPCOUNT_
#define DJINTERP_RE_STD_BIT_POPCOUNT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"


NS_RESTD


// ===========================================================================
// I.   POPCOUNT
// ===========================================================================

// popcount
//   function: population count.
template<typename _T>
D_CONSTEXPR typename internal::bit_enable<_T, int>::type
popcount(
    _T _v
) D_NOEXCEPT
{
    return internal::bit_popcount_rec<_T>(_v, 0);
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_POPCOUNT_
