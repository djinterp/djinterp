/*******************************************************************************
* djinterp [re_std]                                        is_floating_point.hpp
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
* path:      /inc/re_std/type_traits/is_floating_point.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_FLOATING_POINT_HPP
#define RE_STD_TYPE_TRAITS_IS_FLOATING_POINT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_FLOATING_POINT
// =============================================================================

namespace internal
{

    // is_floating_point_base
    //   trait: false (primary template).
    template<typename Type>
    struct is_floating_point_base : false_type
    {};

    template<> struct is_floating_point_base<float>       : true_type {};
    template<> struct is_floating_point_base<double>      : true_type {};
    template<> struct is_floating_point_base<long double> : true_type {};

}  // internal

// is_floating_point
//   trait: true if Type is a standard floating-point type (cv-stripped).
template<typename Type>
struct is_floating_point
    : internal::is_floating_point_base<typename remove_cv<Type>::type>
{};


// =============================================================================
// II.  IS_FLOATING_POINT_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_floating_point_v
    //   variable: convenience for is_floating_point<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_floating_point_v = is_floating_point<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_FLOATING_POINT_HPP
