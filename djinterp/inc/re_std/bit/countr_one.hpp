/******************************************************************************
* djinterp [re_std]                                               countr_one.hpp
*
* countr_one header:
*   Number of consecutive 1 bits starting from the least significant.
*
*   countr_zero of the complement, with the same narrow-type cast as
* countl_one -- see that header.
*
*   PORTABILITY:
*   C++20 in std, back-ported to C++11 and constexpr from C++11.
*
*
* path:      /inc/djinterp/re_std/bit/countr_one.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_COUNTR_ONE_
#define DJINTERP_RE_STD_BIT_COUNTR_ONE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"
#include "./countr_zero.hpp"


NS_RESTD


// ===========================================================================
// I.   COUNTR_ONE
// ===========================================================================

// countr_one
//   function: trailing one bits.
template<typename _T>
D_CONSTEXPR typename internal::bit_enable<_T, int>::type
countr_one(
    _T _v
) D_NOEXCEPT
{
    return re_std::countr_zero(static_cast<_T>(~_v));
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_COUNTR_ONE_
