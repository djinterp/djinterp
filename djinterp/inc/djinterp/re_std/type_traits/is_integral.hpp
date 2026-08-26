/******************************************************************************
* djinterp [re_std]                                            is_integral.hpp
*
* is_integral trait header:
*   Detects whether a type, ignoring cv-qualifiers, is one of the
* standard integral types.
*   STANDARD INTEGRAL TYPES BY TIER:
*   - C++98+:  bool, char, signed char, unsigned char, wchar_t, short,
*              unsigned short, int, unsigned int, long, unsigned long.
*   - C++11+:  adds long long, unsigned long long, char16_t, char32_t.
*   - C++20+:  adds char8_t.
*   Implemented via explicit specializations of an internal
* is_integral_base template - no compiler magic required.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_integral.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_INTEGRAL_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_INTEGRAL_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


NS_RESTD

// =============================================================================
// I.   IS_INTEGRAL
// =============================================================================

NS_INTERNAL
    // is_integral_base
    //   trait: false (primary template).
    template<typename _Type>
    struct is_integral_base : false_type
    {};

    // ---- C++98+ standard integral types ------------------------------------

    template<> struct is_integral_base<bool>               : true_type {};
    template<> struct is_integral_base<char>               : true_type {};
    template<> struct is_integral_base<signed char>        : true_type {};
    template<> struct is_integral_base<unsigned char>      : true_type {};
    template<> struct is_integral_base<wchar_t>            : true_type {};
    template<> struct is_integral_base<short>              : true_type {};
    template<> struct is_integral_base<unsigned short>     : true_type {};
    template<> struct is_integral_base<int>                : true_type {};
    template<> struct is_integral_base<unsigned int>       : true_type {};
    template<> struct is_integral_base<long>               : true_type {};
    template<> struct is_integral_base<unsigned long>      : true_type {};

    // ---- C++11+ additions --------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<> struct is_integral_base<long long>          : true_type {};
    template<> struct is_integral_base<unsigned long long> : true_type {};
    template<> struct is_integral_base<char16_t>           : true_type {};
    template<> struct is_integral_base<char32_t>           : true_type {};
#endif

    // ---- C++20+ additions --------------------------------------------------

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    template<> struct is_integral_base<char8_t>            : true_type {};
#endif

NS_END  // internal

// is_integral
//   trait: true if _Type is a standard integral type (cv-stripped).
template<typename _Type>
struct is_integral
    : internal::is_integral_base<typename remove_cv<_Type>::type>
{};


// =============================================================================
// II.  IS_INTEGRAL_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_integral_v
    //   variable: convenience for is_integral<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_integral_v = is_integral<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_INTEGRAL_