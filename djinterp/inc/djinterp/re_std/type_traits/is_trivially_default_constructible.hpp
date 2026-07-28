/******************************************************************************
* djinterp [restd]                        is_trivially_default_constructible.hpp
*
* is_trivially_default_constructible trait header:
*   Equivalent to is_trivially_constructible<_Type>. Yields true_type
* if _Type is trivially default-constructible.
*
*
* path:      /inc/djinterp/restd/type_traits/is_trivially_default_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// djinterp
#include "./integral_constant.hpp"
#include "./is_trivially_constructible.hpp"


NS_RESTD


// =============================================================================
// I.   IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE
// =============================================================================

template<typename _Type>
struct is_trivially_default_constructible
    : integral_constant<bool,
          is_trivially_constructible<_Type>::value>
{};


// =============================================================================
// II.  IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool is_trivially_default_constructible_v =
        is_trivially_default_constructible<_Type>::value;

#endif


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE_
