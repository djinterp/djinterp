/******************************************************************************
* djinterp [re_std]                                             is_compound.hpp
*
* is_compound trait header:
*   Yields true_type if _Type is a compound type, false_type otherwise.
* A compound type is any type that is NOT fundamental: arrays, functions,
* pointers, references, classes, unions, enumerations, and pointers-to-
* members. Equivalent to !is_fundamental.
*
*     is_compound<int>::value         -> false  (fundamental)
*     is_compound<int*>::value        -> true   (pointer)
*     is_compound<int[5]>::value      -> true   (array)
*     is_compound<int&>::value        -> true   (reference)
*     is_compound<void()>::value      -> true   (function)
*     is_compound<S>::value           -> true   (class)
*     is_compound<E>::value           -> true   (enum, when intrinsic available)
*     is_compound<void>::value        -> false  (fundamental)
*
*
* path:      /inc/djinterp/re_std/type_traits/is_compound.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_COMPOUND_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_COMPOUND_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./is_fundamental.hpp"


NS_RESTD


// =============================================================================
// I.   IS_COMPOUND
// =============================================================================

// is_compound
//   trait: !is_fundamental.
template<typename _Type>
struct is_compound
    : integral_constant<bool, !is_fundamental<_Type>::value>
{};


// =============================================================================
// II.  IS_COMPOUND_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_compound_v
    //   variable: convenience for is_compound<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_compound_v = is_compound<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_COMPOUND_
