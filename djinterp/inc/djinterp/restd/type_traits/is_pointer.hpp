/******************************************************************************
* djinterp [restd]                                              is_pointer.hpp
*
* is_pointer trait header:
*   Detects whether a type, ignoring cv-qualifiers, is a pointer type.
* This includes both object pointers and function pointers, but NOT
* references, pointer-to-member, or std::nullptr_t.
*
*     is_pointer<int*>::value         -> true
*     is_pointer<const int*>::value   -> true   (pointee cv does not matter)
*     is_pointer<int* const>::value   -> true   (top-level cv stripped)
*     is_pointer<void(*)(int)>::value -> true   (function pointer)
*     is_pointer<int>::value          -> false
*     is_pointer<int&>::value         -> false  (reference is not pointer)
*     is_pointer<int Foo::*>::value   -> false  (pointer-to-member is
*                                                separate)
*
*
* path:      /inc/djinterp/restd/type_traits/is_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_POINTER_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_POINTER_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


NS_RESTD


// =============================================================================
// I.   IS_POINTER
// =============================================================================

NS_INTERNAL

    // is_pointer_base
    //   trait: false (primary template).
    template<typename _Type>
    struct is_pointer_base : false_type
    {};

    // is_pointer_base<_Type*>
    //   trait: true for pointer types.
    template<typename _Type>
    struct is_pointer_base<_Type*> : true_type
    {};

NS_END  // internal

// is_pointer
//   trait: true if _Type (cv-stripped) is a pointer.
template<typename _Type>
struct is_pointer
    : internal::is_pointer_base<typename remove_cv<_Type>::type>
{};


// =============================================================================
// II.  IS_POINTER_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_pointer_v
    //   variable: convenience for is_pointer<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_pointer_v = is_pointer<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_POINTER_
