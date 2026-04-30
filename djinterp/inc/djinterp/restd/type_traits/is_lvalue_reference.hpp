/******************************************************************************
* djinterp [restd]                                      is_lvalue_reference.hpp
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
* path:      /inc/djinterp/restd/type_traits/is_lvalue_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_LVALUE_REFERENCE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_LVALUE_REFERENCE_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


NS_RESTD


// =============================================================================
// I.   IS_LVALUE_REFERENCE
// =============================================================================

// is_lvalue_reference
//   trait: false (primary template).
template<typename _Type>
struct is_lvalue_reference : false_type
{};

// is_lvalue_reference<_Type&>
//   trait: true for lvalue reference types.
template<typename _Type>
struct is_lvalue_reference<_Type&> : true_type
{};


// =============================================================================
// II.  IS_LVALUE_REFERENCE_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_lvalue_reference_v
    //   variable: convenience for is_lvalue_reference<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_lvalue_reference_v =
        is_lvalue_reference<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_LVALUE_REFERENCE_
