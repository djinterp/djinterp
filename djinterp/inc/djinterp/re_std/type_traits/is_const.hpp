/******************************************************************************
* djinterp [restd]                                                is_const.hpp
*
* is_const trait header:
*   Detects whether a type has a top-level const qualifier. Note that
* references are never const-qualified at the top level (the referent
* may be).
*
*     is_const<const int>::value       -> true
*     is_const<int>::value             -> false
*     is_const<int* const>::value      -> true   (pointer is const)
*     is_const<const int*>::value      -> false  (pointee is const, not
*                                                pointer)
*     is_const<const int&>::value      -> false  (references are never
*                                                const at top level)
*
*
* path:      /inc/djinterp/restd/type_traits/is_const.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_CONST_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_CONST_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


NS_RESTD


// =============================================================================
// I.   IS_CONST
// =============================================================================

// is_const
//   trait: false (primary template).
template<typename _Type>
struct is_const : false_type
{};

// is_const<const _Type>
//   trait: true for top-level const-qualified types.
template<typename _Type>
struct is_const<const _Type> : true_type
{};


// =============================================================================
// II.  IS_CONST_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_const_v
    //   variable: convenience for is_const<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_const_v = is_const<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_CONST_
