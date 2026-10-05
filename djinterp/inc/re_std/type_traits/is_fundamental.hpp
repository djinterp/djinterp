/*******************************************************************************
* djinterp [re_std]                                           is_fundamental.hpp
*
* is_fundamental trait header:
*   Composite trait. Detects whether a type, ignoring cv-qualifiers, is
* a fundamental type. Fundamental types are: arithmetic types (integral
* + floating-point), void, and (on C++11+) std::nullptr_t.
*
*     is_fundamental<int>::value          -> true
*     is_fundamental<double>::value       -> true
*     is_fundamental<void>::value         -> true
*     is_fundamental<bool>::value         -> true
*     is_fundamental<int*>::value         -> false
*     is_fundamental<int&>::value         -> false
*     is_fundamental<std::nullptr_t>::value -> true   (C++11+)
*
*   PORTABILITY:
*   On C++98/03, std::nullptr_t does not exist; the trait reports based
* on arithmetic + void only. On C++11+, nullptr_t is also recognized.
*
*
* path:      /inc/re_std/type_traits/is_fundamental.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_FUNDAMENTAL_HPP
#define RE_STD_TYPE_TRAITS_IS_FUNDAMENTAL_HPP 1

// std
//
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./is_arithmetic.hpp"
#include "./is_void.hpp"
#include "./is_same.hpp"
#include "./remove_cv.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_FUNDAMENTAL
// =============================================================================

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // is_fundamental
    //   trait: true if Type is arithmetic, void, or std::nullptr_t.
    template<typename Type>
    struct is_fundamental
        : integral_constant<bool,
            ( is_arithmetic<Type>::value ||
              is_void<Type>::value       ||
              is_same<typename remove_cv<Type>::type,
                      std::nullptr_t>::value )>
    {};

#else  // C++98/03 - no std::nullptr_t

    // is_fundamental
    //   trait: true if Type is arithmetic or void (C++98/03 has no
    // nullptr_t).
    template<typename Type>
    struct is_fundamental
        : integral_constant<bool,
            ( is_arithmetic<Type>::value ||
              is_void<Type>::value )>
    {};

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


// =============================================================================
// II.  IS_FUNDAMENTAL_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_fundamental_v
    //   variable: convenience for is_fundamental<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_fundamental_v = is_fundamental<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_FUNDAMENTAL_HPP
