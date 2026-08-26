/******************************************************************************
* djinterp [re_std]                                           has_single_bit.hpp
*
* has_single_bit header:
*   True iff exactly one bit is set -- i.e. the value is an integral
* power of two. False for zero.
*
*   Uses the v & (v-1) identity rather than popcount(v) == 1: clearing
* the lowest set bit yields zero exactly when there was only one. It is
* a single operation instead of a width-long walk, and it is what makes
* this usable in a constant expression at C++11 without recursion.
*
*   PORTABILITY:
*   C++20 in std, back-ported to C++11 and constexpr from C++11.
*
*
* path:      /inc/djinterp/re_std/bit/has_single_bit.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_HAS_SINGLE_BIT_
#define DJINTERP_RE_STD_BIT_HAS_SINGLE_BIT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"


NS_RESTD


// ===========================================================================
// I.   HAS_SINGLE_BIT
// ===========================================================================

// has_single_bit
//   function: whether _v is an integral power of two. False for zero.
template<typename _T>
D_CONSTEXPR typename internal::bit_enable<_T, bool>::type
has_single_bit(
    _T _v
) D_NOEXCEPT
{
    // clearing the lowest set bit empties the value only when there was
    // exactly one set bit to clear
    return (_v != 0) && ( (_v & static_cast<_T>(_v - 1)) == 0 );
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_HAS_SINGLE_BIT_
