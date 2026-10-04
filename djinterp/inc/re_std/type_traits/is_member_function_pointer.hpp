/******************************************************************************
* djinterp [re_std]                             is_member_function_pointer.hpp
*
* is_member_function_pointer trait header:
*   Yields true_type if _Type (after cv-stripping) is a pointer to a
* member function; false_type otherwise. Distinguished from
* is_member_object_pointer by checking whether the pointee type is a
* function type.
*
*     struct S { void f(); int g(int); };
*     is_member_function_pointer<void (S::*)()>::value     -> true
*     is_member_function_pointer<int  (S::*)(int)>::value  -> true
*     is_member_function_pointer<int   S::*>::value        -> false (object)
*     is_member_function_pointer<int*>::value              -> false
*
*
* path:      /inc/djinterp/re_std/type_traits/is_member_function_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_function.hpp"
#include "./integral_constant.hpp"
#include "./remove_cv.hpp"


NS_RESTD


// =============================================================================
// I.   IS_MEMBER_FUNCTION_POINTER
// =============================================================================

NS_INTERNAL

    // is_mem_fn_ptr_base
    //   helper: detects T C::* and forwards is_function on T.
    template<typename _Type>
    struct is_mem_fn_ptr_base : false_type
    {};

    template<typename _Type,
             typename _Class>
    struct is_mem_fn_ptr_base<_Type _Class::*>
        : integral_constant<bool, is_function<_Type>::value>
    {};

NS_END  // internal

// is_member_function_pointer
//   trait: true if _Type is a pointer-to-member-function (cv-stripped).
template<typename _Type>
struct is_member_function_pointer
    : internal::is_mem_fn_ptr_base<typename remove_cv<_Type>::type>
{};


// =============================================================================
// II.  IS_MEMBER_FUNCTION_POINTER_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_member_function_pointer_v
    //   variable: convenience for is_member_function_pointer<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_member_function_pointer_v =
        is_member_function_pointer<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_
