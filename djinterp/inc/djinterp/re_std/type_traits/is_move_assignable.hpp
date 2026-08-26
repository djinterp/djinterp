/******************************************************************************
* djinterp [re_std]                                      is_move_assignable.hpp
*
* is_move_assignable trait header:
*   Equivalent to is_assignable<_Type&, _Type&&>.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_move_assignable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_MOVE_ASSIGNABLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_MOVE_ASSIGNABLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./integral_constant.hpp"
#include "./is_assignable.hpp"
#include "./add_lvalue_reference.hpp"
#include "./add_rvalue_reference.hpp"


NS_RESTD


// =============================================================================
// I.   IS_MOVE_ASSIGNABLE
// =============================================================================

template<typename _Type>
struct is_move_assignable
    : integral_constant<bool,
          is_assignable<
              typename add_lvalue_reference<_Type>::type,
              typename add_rvalue_reference<_Type>::type
          >::value>
{};


// =============================================================================
// II.  IS_MOVE_ASSIGNABLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool is_move_assignable_v =
        is_move_assignable<_Type>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_MOVE_ASSIGNABLE_
