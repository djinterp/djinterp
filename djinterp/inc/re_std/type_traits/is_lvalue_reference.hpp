/*******************************************************************************
* djinterp [re_std]                                      is_lvalue_reference.hpp
*
* is_lvalue_reference trait header:
*   Detects whether a type is an lvalue reference (T&). Rvalue
* references (T&&) are NOT lvalue references; see is_rvalue_reference.
*
*     is_lvalue_reference<int&>::value        -> true
*     is_lvalue_reference<const int&>::value  -> true
*     is_lvalue_reference<int&&>::value       -> false  (rvalue reference)
*     is_lvalue_reference<int>::value         -> false
*     is_lvalue_reference<int*>::value        -> false
*
*
* path:      /inc/re_std/type_traits/is_lvalue_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_LVALUE_REFERENCE_HPP
#define RE_STD_TYPE_TRAITS_IS_LVALUE_REFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_LVALUE_REFERENCE
// =============================================================================

// is_lvalue_reference
//   trait: false (primary template).
template<typename Type>
struct is_lvalue_reference : false_type
{};

// is_lvalue_reference<Type&>
//   trait: true for lvalue reference types.
template<typename Type>
struct is_lvalue_reference<Type&> : true_type
{};


// =============================================================================
// II.  IS_LVALUE_REFERENCE_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_lvalue_reference_v
    //   variable: convenience for is_lvalue_reference<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_lvalue_reference_v =
        is_lvalue_reference<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_LVALUE_REFERENCE_HPP
