/******************************************************************************
* djinterp [restd]                                                is_object.hpp
*
* is_object trait header:
*   Yields true_type if _Type is an object type, per [basic.types].
* An object type is any type that is NOT a function, NOT a reference,
* and NOT void. Equivalent to:
*   !(is_function || is_reference || is_void)
*
*     is_object<int>::value          -> true   (scalar is object)
*     is_object<int[5]>::value       -> true   (array is object)
*     is_object<S>::value            -> true   (class is object)
*     is_object<int*>::value         -> true   (pointer is object)
*     is_object<int&>::value         -> false  (reference)
*     is_object<void()>::value       -> false  (function)
*     is_object<void>::value         -> false  (void)
*
*
* path:      /inc/djinterp/re_std/type_traits/is_object.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_OBJECT_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_OBJECT_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./is_function.hpp"
#include "./is_reference.hpp"
#include "./is_void.hpp"


NS_RESTD


// =============================================================================
// I.   IS_OBJECT
// =============================================================================

// is_object
//   trait: NOT function, NOT reference, NOT void.
template<typename _Type>
struct is_object
    : integral_constant<bool,
          ( !is_function<_Type>::value  &&
            !is_reference<_Type>::value &&
            !is_void<_Type>::value )>
{};


// =============================================================================
// II.  IS_OBJECT_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_object_v
    //   variable: convenience for is_object<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_object_v = is_object<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_OBJECT_
