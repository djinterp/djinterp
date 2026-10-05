/*******************************************************************************
* djinterp [re_std]                                      is_rvalue_reference.hpp
*
* is_rvalue_reference trait header:
*   Detects whether a type is an rvalue reference (T&&). Lvalue
* references (T&) are NOT rvalue references; see is_lvalue_reference.
*
*     is_rvalue_reference<int&&>::value       -> true   (C++11+)
*     is_rvalue_reference<int&>::value        -> false
*     is_rvalue_reference<int>::value         -> false
*
*   PORTABILITY:
*   - C++98/03: rvalue references do not exist. The trait is provided
*     and always reports false_type.
*   - C++11+:   real specialization on T&& (gated on
*     RE_STD_LANG_HAS_RVALUE_REFERENCES).
*
*
* path:      /inc/re_std/type_traits/is_rvalue_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_RVALUE_REFERENCE_HPP
#define RE_STD_TYPE_TRAITS_IS_RVALUE_REFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_RVALUE_REFERENCE
// =============================================================================

// is_rvalue_reference
//   trait: false (primary template).
template<typename Type>
struct is_rvalue_reference : false_type
{};

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    // is_rvalue_reference<Type&&>
    //   trait: true for rvalue reference types (C++11+).
    template<typename Type>
    struct is_rvalue_reference<Type&&> : true_type
    {};

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES


// =============================================================================
// II.  IS_RVALUE_REFERENCE_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_rvalue_reference_v
    //   variable: convenience for is_rvalue_reference<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_rvalue_reference_v =
        is_rvalue_reference<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_RVALUE_REFERENCE_HPP
