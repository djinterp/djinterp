/*******************************************************************************
* djinterp [re_std]                                 is_member_object_pointer.hpp
*
* is_member_object_pointer trait header:
*   Yields true_type if Type is a pointer to a non-function member
* (i.e. a data member); false_type otherwise. Equivalent to
* is_member_pointer && !is_member_function_pointer.
*
*     struct S { int m; void f(); };
*     is_member_object_pointer<int   S::*>::value          -> true
*     is_member_object_pointer<void (S::*)()>::value       -> false (function)
*     is_member_object_pointer<int*>::value                -> false
*
*
* path:      /inc/re_std/type_traits/is_member_object_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_HPP
#define RE_STD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./is_member_pointer.hpp"
#include "./is_member_function_pointer.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_MEMBER_OBJECT_POINTER
// =============================================================================

// is_member_object_pointer
//   trait: composite (member pointer that is NOT a function pointer).
template<typename Type>
struct is_member_object_pointer
    : integral_constant<bool,
          ( is_member_pointer<Type>::value &&
            !is_member_function_pointer<Type>::value )>
{};


// =============================================================================
// II.  IS_MEMBER_OBJECT_POINTER_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_member_object_pointer_v
    //   variable: convenience for is_member_object_pointer<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_member_object_pointer_v =
        is_member_object_pointer<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_HPP
