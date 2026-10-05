/*******************************************************************************
* djinterp [re_std]                                        is_member_pointer.hpp
*
* is_member_pointer trait header:
*   Yields true_type if Type (after cv-stripping) is a pointer-to-member
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
* path:      /inc/re_std/type_traits/is_member_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_MEMBER_POINTER_HPP
#define RE_STD_TYPE_TRAITS_IS_MEMBER_POINTER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_MEMBER_POINTER
// =============================================================================

namespace internal
{

    // is_member_pointer_base
    //   trait: detects T C::* form.
    template<typename Type>
    struct is_member_pointer_base : false_type
    {};

    template<typename Type,
             typename Class>
    struct is_member_pointer_base<Type Class::*> : true_type
    {};

}  // internal

// is_member_pointer
//   trait: true if Type is any pointer-to-member (cv-stripped).
template<typename Type>
struct is_member_pointer
    : internal::is_member_pointer_base<typename remove_cv<Type>::type>
{};


// =============================================================================
// II.  IS_MEMBER_POINTER_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_member_pointer_v
    //   variable: convenience for is_member_pointer<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_member_pointer_v = is_member_pointer<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_MEMBER_POINTER_HPP
