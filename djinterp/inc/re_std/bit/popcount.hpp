/*******************************************************************************
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
* path:      /inc/re_std/bit/popcount.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BIT_POPCOUNT_HPP
#define RE_STD_BIT_POPCOUNT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./bit_internal.hpp"


namespace re_std
{


// ===========================================================================
// I.   POPCOUNT
// ===========================================================================

// popcount
//   function: population count.
template<typename T>
RE_STD_CONSTEXPR typename internal::bit_enable<T, int>::type
popcount(
    T _v
) RE_STD_NOEXCEPT
{
    return internal::bit_popcount_rec<T>(_v, 0);
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BIT_POPCOUNT_HPP
