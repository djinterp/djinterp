/******************************************************************************
* djinterp [re_std]                                          is_arithmetic.hpp
*
* is_arithmetic trait header:
*   Composite trait. Detects whether a type, ignoring cv-qualifiers, is
* an arithmetic type - either an integral type or a floating-point type.
*
*     is_arithmetic<int>::value          -> true
*     is_arithmetic<double>::value       -> true
*     is_arithmetic<bool>::value         -> true   (bool is integral)
*     is_arithmetic<const float>::value  -> true
*     is_arithmetic<int*>::value         -> false
*     is_arithmetic<void>::value         -> false
*
*   Equivalent to is_integral<T>::value || is_floating_point<T>::value.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_arithmetic.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_ARITHMETIC_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_ARITHMETIC_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./is_integral.hpp"
#include "./is_floating_point.hpp"


NS_RESTD


// =============================================================================
// I.   IS_ARITHMETIC
// =============================================================================

// is_arithmetic
//   trait: true if _Type is integral OR floating-point.
template<typename _Type>
struct is_arithmetic
    : integral_constant<bool,
        ( is_integral<_Type>::value ||
          is_floating_point<_Type>::value )>
{};


// =============================================================================
// II.  IS_ARITHMETIC_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_arithmetic_v
    //   variable: convenience for is_arithmetic<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_arithmetic_v = is_arithmetic<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_ARITHMETIC_
