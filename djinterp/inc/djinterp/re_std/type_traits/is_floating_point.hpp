/******************************************************************************
* djinterp [restd]                                       is_floating_point.hpp
*
* is_floating_point trait header:
*   Detects whether a type, ignoring cv-qualifiers, is one of the
* standard floating-point types: float, double, long double.
*
*     is_floating_point<float>::value        -> true
*     is_floating_point<double>::value       -> true
*     is_floating_point<long double>::value  -> true
*     is_floating_point<const double>::value -> true   (cv stripped)
*     is_floating_point<int>::value          -> false
*
*   Note: extended floating-point types from C++23 (std::float16_t et
* al.) are not specialized here. They can be added behind a tier guard
* when the host compiler supports them.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_floating_point.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_FLOATING_POINT_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_FLOATING_POINT_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


NS_RESTD


// =============================================================================
// I.   IS_FLOATING_POINT
// =============================================================================

NS_INTERNAL

    // is_floating_point_base
    //   trait: false (primary template).
    template<typename _Type>
    struct is_floating_point_base : false_type
    {};

    template<> struct is_floating_point_base<float>       : true_type {};
    template<> struct is_floating_point_base<double>      : true_type {};
    template<> struct is_floating_point_base<long double> : true_type {};

NS_END  // internal

// is_floating_point
//   trait: true if _Type is a standard floating-point type (cv-stripped).
template<typename _Type>
struct is_floating_point
    : internal::is_floating_point_base<typename remove_cv<_Type>::type>
{};


// =============================================================================
// II.  IS_FLOATING_POINT_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_floating_point_v
    //   variable: convenience for is_floating_point<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_floating_point_v = is_floating_point<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_FLOATING_POINT_
