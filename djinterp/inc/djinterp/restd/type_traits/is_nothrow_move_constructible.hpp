/******************************************************************************
* djinterp [restd]                              is_nothrow_move_constructible.hpp
*
* is_nothrow_move_constructible trait header:
*   Equivalent to is_nothrow_constructible<_Type, _Type&&>.
*
*
* path:      /inc/djinterp/restd/type_traits/is_nothrow_move_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_MOVE_CONSTRUCTIBLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_MOVE_CONSTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// djinterp
#include "./integral_constant.hpp"
#include "./is_nothrow_constructible.hpp"
#include "./add_rvalue_reference.hpp"


NS_RESTD


// =============================================================================
// I.   IS_NOTHROW_MOVE_CONSTRUCTIBLE
// =============================================================================

template<typename _Type>
struct is_nothrow_move_constructible
    : integral_constant<bool,
          is_nothrow_constructible<
              _Type,
              typename add_rvalue_reference<_Type>::type
          >::value>
{};


// =============================================================================
// II.  IS_NOTHROW_MOVE_CONSTRUCTIBLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool is_nothrow_move_constructible_v =
        is_nothrow_move_constructible<_Type>::value;

#endif


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_MOVE_CONSTRUCTIBLE_
