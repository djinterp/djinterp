/******************************************************************************
* djinterp [restd]                                      is_rvalue_reference.hpp
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
*     D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES).
*
*
* path:      /inc/djinterp/re_std/type_traits/is_rvalue_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_RVALUE_REFERENCE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_RVALUE_REFERENCE_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


NS_RESTD


// =============================================================================
// I.   IS_RVALUE_REFERENCE
// =============================================================================

// is_rvalue_reference
//   trait: false (primary template).
template<typename _Type>
struct is_rvalue_reference : false_type
{};

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    // is_rvalue_reference<_Type&&>
    //   trait: true for rvalue reference types (C++11+).
    template<typename _Type>
    struct is_rvalue_reference<_Type&&> : true_type
    {};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =============================================================================
// II.  IS_RVALUE_REFERENCE_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_rvalue_reference_v
    //   variable: convenience for is_rvalue_reference<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_rvalue_reference_v =
        is_rvalue_reference<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_RVALUE_REFERENCE_
