/******************************************************************************
* djinterp [re_std]                                         is_fundamental.hpp
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
* path:      /inc/djinterp/re_std/type_traits/is_fundamental.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_FUNDAMENTAL_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_FUNDAMENTAL_ 1

// 
#include <cstddef>
// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./is_arithmetic.hpp"
#include "./is_void.hpp"
#include "./is_same.hpp"
#include "./remove_cv.hpp"


NS_RESTD


// =============================================================================
// I.   IS_FUNDAMENTAL
// =============================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // is_fundamental
    //   trait: true if _Type is arithmetic, void, or std::nullptr_t.
    template<typename _Type>
    struct is_fundamental
        : integral_constant<bool,
            ( is_arithmetic<_Type>::value ||
              is_void<_Type>::value       ||
              is_same<typename remove_cv<_Type>::type,
                      std::nullptr_t>::value )>
    {};

#else  // C++98/03 - no std::nullptr_t

    // is_fundamental
    //   trait: true if _Type is arithmetic or void (C++98/03 has no
    // nullptr_t).
    template<typename _Type>
    struct is_fundamental
        : integral_constant<bool,
            ( is_arithmetic<_Type>::value ||
              is_void<_Type>::value )>
    {};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


// =============================================================================
// II.  IS_FUNDAMENTAL_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_fundamental_v
    //   variable: convenience for is_fundamental<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_fundamental_v = is_fundamental<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_FUNDAMENTAL_
