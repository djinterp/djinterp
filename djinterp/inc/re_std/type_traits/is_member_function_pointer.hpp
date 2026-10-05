/*******************************************************************************
* djinterp [re_std]                               is_member_function_pointer.hpp
*
* is_member_function_pointer trait header:
*   Yields true_type if Type (after cv-stripping) is a pointer to a
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
* path:      /inc/re_std/type_traits/is_member_function_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_HPP
#define RE_STD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_function.hpp"
#include "./integral_constant.hpp"
#include "./remove_cv.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_MEMBER_FUNCTION_POINTER
// =============================================================================

namespace internal
{

    // is_mem_fn_ptr_base
    //   trait: detects T C::* and forwards is_function on T.
    template<typename Type>
    struct is_mem_fn_ptr_base : false_type
    {};

    template<typename Type,
             typename Class>
    struct is_mem_fn_ptr_base<Type Class::*>
        : integral_constant<bool, is_function<Type>::value>
    {};

}  // internal

// is_member_function_pointer
//   trait: true if Type is a pointer-to-member-function (cv-stripped).
template<typename Type>
struct is_member_function_pointer
    : internal::is_mem_fn_ptr_base<typename remove_cv<Type>::type>
{};


// =============================================================================
// II.  IS_MEMBER_FUNCTION_POINTER_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_member_function_pointer_v
    //   variable: convenience for is_member_function_pointer<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_member_function_pointer_v =
        is_member_function_pointer<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_HPP
