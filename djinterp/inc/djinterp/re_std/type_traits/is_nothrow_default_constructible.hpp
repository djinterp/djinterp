/******************************************************************************
* djinterp [re_std]                         is_nothrow_default_constructible.hpp
*
* is_nothrow_default_constructible trait header:
*   Equivalent to is_nothrow_constructible<_Type>. Yields true_type if
* default construction of _Type is well-formed and noexcept.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_nothrow_default_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_DEFAULT_CONSTRUCTIBLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_DEFAULT_CONSTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// djinterp
#include "./integral_constant.hpp"
#include "./is_nothrow_constructible.hpp"


NS_RESTD


// =============================================================================
// I.   IS_NOTHROW_DEFAULT_CONSTRUCTIBLE
// =============================================================================

template<typename _Type>
struct is_nothrow_default_constructible
    : integral_constant<bool, is_nothrow_constructible<_Type>::value>
{};


// =============================================================================
// II.  IS_NOTHROW_DEFAULT_CONSTRUCTIBLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool is_nothrow_default_constructible_v =
        is_nothrow_default_constructible<_Type>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_DEFAULT_CONSTRUCTIBLE_
