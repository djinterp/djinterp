/******************************************************************************
* djinterp [restd]                                            is_reference.hpp
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
* path:      /inc/djinterp/re_std/type_traits/is_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_REFERENCE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_REFERENCE_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


NS_RESTD


// =============================================================================
// I.   IS_REFERENCE
// =============================================================================

// is_reference
//   trait: false (primary template).
template<typename _Type>
struct is_reference : false_type
{};

// is_reference<_Type&>
//   trait: true for lvalue reference types.
template<typename _Type>
struct is_reference<_Type&> : true_type
{};

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    // is_reference<_Type&&>
    //   trait: true for rvalue reference types (C++11+).
    template<typename _Type>
    struct is_reference<_Type&&> : true_type
    {};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =============================================================================
// II.  IS_REFERENCE_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_reference_v
    //   variable: convenience for is_reference<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_reference_v = is_reference<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_REFERENCE_
