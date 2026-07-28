/******************************************************************************
* djinterp [restd]                                          is_null_pointer.hpp
*
* is_null_pointer trait header:
*   Yields true_type if _Type (after cv-stripping) is std::nullptr_t,
* false_type otherwise. The C++14 trait spelling.
*
*     is_null_pointer<std::nullptr_t>::value         -> true
*     is_null_pointer<const std::nullptr_t>::value   -> true
*     is_null_pointer<int*>::value                   -> false
*     is_null_pointer<void*>::value                  -> false
*     is_null_pointer<int>::value                    -> false
*
*   PORTABILITY:
*   nullptr_t is a C++11 type. On C++98/03 the type does not exist; this
* trait is omitted entirely. Consumer code must gate on
* D_ENV_LANG_IS_CPP11_OR_HIGHER.
*
*
* path:      /inc/djinterp/restd/type_traits/is_null_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_NULL_POINTER_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_NULL_POINTER_ 1

// djinterp
#include "../../core/djinterp.hpp"


// gate: nullptr_t is C++11
#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// std
#include <cstddef>
// djinterp
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


NS_RESTD


// =============================================================================
// I.   IS_NULL_POINTER
// =============================================================================

NS_INTERNAL

    // is_null_pointer_base
    //   helper: matches std::nullptr_t exactly (post cv-stripping).
    template<typename _Type>
    struct is_null_pointer_base : false_type
    {};

    template<>
    struct is_null_pointer_base<std::nullptr_t> : true_type
    {};

NS_END  // internal

// is_null_pointer
//   trait: true if _Type (cv-stripped) is std::nullptr_t.
template<typename _Type>
struct is_null_pointer
    : internal::is_null_pointer_base<typename remove_cv<_Type>::type>
{};


// =============================================================================
// II.  IS_NULL_POINTER_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_null_pointer_v
    //   variable: convenience for is_null_pointer<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_null_pointer_v = is_null_pointer<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_NULL_POINTER_
