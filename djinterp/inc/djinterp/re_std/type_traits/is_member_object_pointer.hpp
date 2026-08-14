/******************************************************************************
* djinterp [restd]                                is_member_object_pointer.hpp
*
* is_member_object_pointer trait header:
*   Yields true_type if _Type is a pointer to a non-function member
* (i.e. a data member); false_type otherwise. Equivalent to
* is_member_pointer && !is_member_function_pointer.
*
*     struct S { int m; void f(); };
*     is_member_object_pointer<int   S::*>::value          -> true
*     is_member_object_pointer<void (S::*)()>::value       -> false (function)
*     is_member_object_pointer<int*>::value                -> false
*
*
* path:      /inc/djinterp/re_std/type_traits/is_member_object_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./is_member_pointer.hpp"
#include "./is_member_function_pointer.hpp"


NS_RESTD


// =============================================================================
// I.   IS_MEMBER_OBJECT_POINTER
// =============================================================================

// is_member_object_pointer
//   trait: composite (member pointer that is NOT a function pointer).
template<typename _Type>
struct is_member_object_pointer
    : integral_constant<bool,
          ( is_member_pointer<_Type>::value &&
            !is_member_function_pointer<_Type>::value )>
{};


// =============================================================================
// II.  IS_MEMBER_OBJECT_POINTER_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_member_object_pointer_v
    //   variable: convenience for is_member_object_pointer<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_member_object_pointer_v =
        is_member_object_pointer<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_
