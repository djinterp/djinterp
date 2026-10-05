/*******************************************************************************
* djinterp [re_std]                                          ratio_not_equal.hpp
*
* ratio_not_equal header:
*   The negation of ratio_equal.
*
*   PORTABILITY:
*   C++11 in std; the _v spelling is C++17 in std and C++14 here.
*
*
* path:      /inc/re_std/ratio/ratio_not_equal.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RATIO_RATIO_NOT_EQUAL_HPP
#define RE_STD_RATIO_RATIO_NOT_EQUAL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./ratio.hpp"
#include "./ratio_equal.hpp"
#include "../type_traits/integral_constant.hpp"


namespace re_std
{


// ===========================================================================
// I.   RATIO_NOT_EQUAL
// ===========================================================================

// ratio_not_equal
//   trait: the complement of ratio_equal.
template<typename R1,
         typename R2>
struct ratio_not_equal
    : integral_constant<bool, !ratio_equal<R1, R2>::value>
{};


// ===========================================================================
// II.  RATIO_NOT_EQUAL_V (C++14+ variable)
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename R1,
         typename R2>
RE_STD_CONSTEXPR bool ratio_not_equal_v = ratio_not_equal<R1, R2>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RATIO_RATIO_NOT_EQUAL_HPP
