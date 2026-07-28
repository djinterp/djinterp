/******************************************************************************
* djinterp [restd]                                                 is_void.hpp
*
* is_void trait header:
*   Detects whether a type, ignoring cv-qualifiers, is `void`.
*
*     is_void<void>::value                -> true
*     is_void<const void>::value          -> true
*     is_void<volatile void>::value       -> true
*     is_void<const volatile void>::value -> true
*     is_void<int>::value                 -> false
*     is_void<void*>::value               -> false  (pointer to void is not
*                                                   void)
*
*
* path:      /inc/djinterp/restd/type_traits/is_void.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_VOID_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_VOID_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


NS_RESTD


// =============================================================================
// I.   IS_VOID
// =============================================================================

NS_INTERNAL

    // is_void_base
    //   trait: false (primary template).
    template<typename _Type>
    struct is_void_base : false_type
    {};

    // is_void_base<void>
    //   trait: true for void.
    template<>
    struct is_void_base<void> : true_type
    {};

NS_END  // internal

// is_void
//   trait: true if _Type is `void`, ignoring cv-qualifiers.
template<typename _Type>
struct is_void
    : internal::is_void_base<typename remove_cv<_Type>::type>
{};


// =============================================================================
// II.  IS_VOID_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_void_v
    //   variable: convenience for is_void<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_void_v = is_void<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_VOID_
