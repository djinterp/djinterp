/*******************************************************************************
* djinterp [re_std]                                             is_reference.hpp
*
* is_reference trait header:
*   Detects whether a type is any reference type - either an lvalue
* reference (T&) or an rvalue reference (T&&).
*
*     is_reference<int&>::value   -> true
*     is_reference<int&&>::value  -> true   (C++11+)
*     is_reference<int>::value    -> false
*     is_reference<int*>::value   -> false
*
*   Equivalent to is_lvalue_reference<T>::value || is_rvalue_reference<T>::value.
*
*
* path:      /inc/re_std/type_traits/is_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_REFERENCE_HPP
#define RE_STD_TYPE_TRAITS_IS_REFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_REFERENCE
// =============================================================================

// is_reference
//   trait: false (primary template).
template<typename Type>
struct is_reference : false_type
{};

// is_reference<Type&>
//   trait: true for lvalue reference types.
template<typename Type>
struct is_reference<Type&> : true_type
{};

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    // is_reference<Type&&>
    //   trait: true for rvalue reference types (C++11+).
    template<typename Type>
    struct is_reference<Type&&> : true_type
    {};

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES


// =============================================================================
// II.  IS_REFERENCE_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_reference_v
    //   variable: convenience for is_reference<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_reference_v = is_reference<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_REFERENCE_HPP
