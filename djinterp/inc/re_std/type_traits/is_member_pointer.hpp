/******************************************************************************
* djinterp [re_std]                                      is_member_pointer.hpp
*
* is_member_pointer trait header:
*   Yields true_type if _Type (after cv-stripping) is a pointer-to-member
* (object or function); false_type otherwise. Composite of
* is_member_object_pointer and is_member_function_pointer; this primary
* form does not distinguish between the two.
*
*     struct S { int m; void f(); };
*     is_member_pointer<int S::*>::value          -> true
*     is_member_pointer<void (S::*)()>::value     -> true
*     is_member_pointer<int*>::value              -> false
*     is_member_pointer<int>::value               -> false
*
*
* path:      /inc/djinterp/re_std/type_traits/is_member_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_MEMBER_POINTER_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_MEMBER_POINTER_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


NS_RESTD


// =============================================================================
// I.   IS_MEMBER_POINTER
// =============================================================================

NS_INTERNAL

    // is_member_pointer_base
    //   helper: detects T C::* form.
    template<typename _Type>
    struct is_member_pointer_base : false_type
    {};

    template<typename _Type,
             typename _Class>
    struct is_member_pointer_base<_Type _Class::*> : true_type
    {};

NS_END  // internal

// is_member_pointer
//   trait: true if _Type is any pointer-to-member (cv-stripped).
template<typename _Type>
struct is_member_pointer
    : internal::is_member_pointer_base<typename remove_cv<_Type>::type>
{};


// =============================================================================
// II.  IS_MEMBER_POINTER_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_member_pointer_v
    //   variable: convenience for is_member_pointer<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_member_pointer_v = is_member_pointer<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_MEMBER_POINTER_
