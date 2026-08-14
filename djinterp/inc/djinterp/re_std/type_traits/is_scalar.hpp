/******************************************************************************
* djinterp [restd]                                                is_scalar.hpp
*
* is_scalar trait header:
*   Yields true_type if _Type is a scalar type (per [basic.types]):
*     - arithmetic (integral or floating-point)
*     - enumeration
*     - pointer
*     - pointer-to-member
*     - std::nullptr_t (C++11+)
*
*     is_scalar<int>::value             -> true   (arithmetic)
*     is_scalar<float>::value           -> true   (arithmetic)
*     is_scalar<int*>::value            -> true   (pointer)
*     is_scalar<int (S::*)>::value      -> true   (member pointer)
*     is_scalar<std::nullptr_t>::value  -> true   (nullptr_t, C++11+)
*     is_scalar<int[5]>::value          -> false  (array)
*     is_scalar<void()>::value          -> false  (function)
*     is_scalar<S>::value               -> false  (class)
*     is_scalar<int&>::value            -> false  (reference)
*     is_scalar<void>::value            -> false
*
*
* path:      /inc/djinterp/re_std/type_traits/is_scalar.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_SCALAR_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_SCALAR_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./is_arithmetic.hpp"
#include "./is_enum.hpp"
#include "./is_pointer.hpp"
#include "./is_member_pointer.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include "./is_null_pointer.hpp"
#endif


NS_RESTD


// =============================================================================
// I.   IS_SCALAR
// =============================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // is_scalar (C++11+)
    //   trait: arithmetic OR enum OR pointer OR member-pointer OR nullptr_t.
    template<typename _Type>
    struct is_scalar
        : integral_constant<bool,
              ( is_arithmetic<_Type>::value      ||
                is_enum<_Type>::value            ||
                is_pointer<_Type>::value         ||
                is_member_pointer<_Type>::value  ||
                is_null_pointer<_Type>::value )>
    {};

#else

    // is_scalar (C++98/03)
    //   trait: arithmetic OR enum OR pointer OR member-pointer.
    template<typename _Type>
    struct is_scalar
        : integral_constant<bool,
              ( is_arithmetic<_Type>::value      ||
                is_enum<_Type>::value            ||
                is_pointer<_Type>::value         ||
                is_member_pointer<_Type>::value )>
    {};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


// =============================================================================
// II.  IS_SCALAR_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_scalar_v
    //   variable: convenience for is_scalar<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_scalar_v = is_scalar<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_SCALAR_
