/******************************************************************************
* djinterp [restd]                                       is_copy_assignable.hpp
*
* is_copy_assignable trait header:
*   Equivalent to is_assignable<_Type&, _Type const&>. Yields true_type
* if _Type supports copy assignment.
*
*
* path:      /inc/djinterp/restd/type_traits/is_copy_assignable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_COPY_ASSIGNABLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_COPY_ASSIGNABLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./integral_constant.hpp"
#include "./is_assignable.hpp"
#include "./add_const.hpp"
#include "./add_lvalue_reference.hpp"


NS_RESTD


// =============================================================================
// I.   IS_COPY_ASSIGNABLE
// =============================================================================

template<typename _Type>
struct is_copy_assignable
    : integral_constant<bool,
          is_assignable<
              typename add_lvalue_reference<_Type>::type,
              typename add_lvalue_reference<
                  typename add_const<_Type>::type
              >::type
          >::value>
{};


// =============================================================================
// II.  IS_COPY_ASSIGNABLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool is_copy_assignable_v =
        is_copy_assignable<_Type>::value;

#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_COPY_ASSIGNABLE_
