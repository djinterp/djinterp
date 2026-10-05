/*******************************************************************************
* djinterp [re_std]                                      ratio_greater_equal.hpp
*
* ratio_greater_equal header:
*   R1 >= R2, i.e. not R1 < R2.
*
*   Defined in terms of ratio_less rather than independently, so
* all four orderings share one overflow-free comparison and cannot
* drift apart.
*
*   PORTABILITY:
*   C++11 in std; the _v spelling is C++17 in std and C++14 here.
*
*
* path:      /inc/re_std/ratio/ratio_greater_equal.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RATIO_RATIO_GREATER_EQUAL_HPP
#define RE_STD_RATIO_RATIO_GREATER_EQUAL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./ratio.hpp"
#include "./ratio_less.hpp"
#include "../type_traits/integral_constant.hpp"


namespace re_std
{


// ===========================================================================
// I.   RATIO_GREATER_EQUAL
// ===========================================================================

// ratio_greater_equal
//   trait: R1 >= R2, i.e. not R1 < R2.
template<typename R1,
         typename R2>
struct ratio_greater_equal
    : integral_constant<bool, !ratio_less<R1, R2>::value>
{};


// ===========================================================================
// II.  RATIO_GREATER_EQUAL_V (C++14+ variable)
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename R1,
         typename R2>
RE_STD_CONSTEXPR bool ratio_greater_equal_v = ratio_greater_equal<R1, R2>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RATIO_RATIO_GREATER_EQUAL_HPP
