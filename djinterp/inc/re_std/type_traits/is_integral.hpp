/*******************************************************************************
* djinterp [re_std]                                              is_integral.hpp
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
* path:      /inc/re_std/type_traits/is_integral.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_INTEGRAL_HPP
#define RE_STD_TYPE_TRAITS_IS_INTEGRAL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


namespace re_std
{

// =============================================================================
// I.   IS_INTEGRAL
// =============================================================================

namespace internal
{
    // is_integral_base
    //   trait: false (primary template).
    template<typename Type>
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

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    template<> struct is_integral_base<long long>          : true_type {};
    template<> struct is_integral_base<unsigned long long> : true_type {};
    template<> struct is_integral_base<char16_t>           : true_type {};
    template<> struct is_integral_base<char32_t>           : true_type {};
#endif

    // ---- C++20+ additions --------------------------------------------------

#if RE_STD_LANG_IS_CPP20_OR_HIGHER
    template<> struct is_integral_base<char8_t>            : true_type {};
#endif

}  // internal

// is_integral
//   trait: true if Type is a standard integral type (cv-stripped).
template<typename Type>
struct is_integral
    : internal::is_integral_base<typename remove_cv<Type>::type>
{};


// =============================================================================
// II.  IS_INTEGRAL_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
    // is_integral_v
    //   variable: convenience for is_integral<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_integral_v = is_integral<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_INTEGRAL_HPP
