/******************************************************************************
* djinterp [re_std]                             is_nothrow_copy_constructible.hpp
*
* is_nothrow_copy_constructible trait header:
*   Equivalent to is_nothrow_constructible<_Type, _Type const&>.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_nothrow_copy_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_COPY_CONSTRUCTIBLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_COPY_CONSTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// djinterp
#include "./integral_constant.hpp"
#include "./is_nothrow_constructible.hpp"
#include "./add_const.hpp"
#include "./add_lvalue_reference.hpp"


NS_RESTD


// =============================================================================
// I.   IS_NOTHROW_COPY_CONSTRUCTIBLE
// =============================================================================

template<typename _Type>
struct is_nothrow_copy_constructible
    : integral_constant<bool,
          is_nothrow_constructible<
              _Type,
              typename add_lvalue_reference<
                  typename add_const<_Type>::type
              >::type
          >::value>
{};


// =============================================================================
// II.  IS_NOTHROW_COPY_CONSTRUCTIBLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool is_nothrow_copy_constructible_v =
        is_nothrow_copy_constructible<_Type>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_COPY_CONSTRUCTIBLE_
