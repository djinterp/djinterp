/*******************************************************************************
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
* path:      /inc/re_std/bit/countr_one.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BIT_COUNTR_ONE_HPP
#define RE_STD_BIT_COUNTR_ONE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./bit_internal.hpp"
#include "./countr_zero.hpp"


namespace re_std
{


// ===========================================================================
// I.   COUNTR_ONE
// ===========================================================================

// countr_one
//   function: trailing one bits.
template<typename T>
RE_STD_CONSTEXPR typename internal::bit_enable<T, int>::type
countr_one(
    T _v
) RE_STD_NOEXCEPT
{
    return re_std::countr_zero(static_cast<T>(~_v));
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BIT_COUNTR_ONE_HPP
