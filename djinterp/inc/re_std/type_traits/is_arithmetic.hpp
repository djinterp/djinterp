/*******************************************************************************
* djinterp [re_std]                                            is_arithmetic.hpp
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
* path:      /inc/re_std/type_traits/is_arithmetic.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_ARITHMETIC_HPP
#define RE_STD_TYPE_TRAITS_IS_ARITHMETIC_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./is_integral.hpp"
#include "./is_floating_point.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_ARITHMETIC
// =============================================================================

// is_arithmetic
//   trait: true if Type is integral OR floating-point.
template<typename Type>
struct is_arithmetic
    : integral_constant<bool,
        ( is_integral<Type>::value ||
          is_floating_point<Type>::value )>
{};


// =============================================================================
// II.  IS_ARITHMETIC_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_arithmetic_v
    //   variable: convenience for is_arithmetic<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_arithmetic_v = is_arithmetic<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_ARITHMETIC_HPP
