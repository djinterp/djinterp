/******************************************************************************
* djinterp [restd]                                     is_move_constructible.hpp
*
* is_move_constructible trait header:
*   Equivalent to is_constructible<_Type, _Type&&>. Yields true_type if
* _Type can be constructed from an rvalue reference to itself.
*
*     is_move_constructible<int>::value           -> true
*     struct A { A(A&&) = default; };
*     is_move_constructible<A>::value             -> true
*
*   PORTABILITY:
*   Requires rvalue references (C++11+). Already required by the
* underlying is_constructible's variadic-template gate.
*
*
* path:      /inc/djinterp/restd/type_traits/is_move_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_MOVE_CONSTRUCTIBLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_MOVE_CONSTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// djinterp
#include "./integral_constant.hpp"
#include "./is_constructible.hpp"
#include "./add_rvalue_reference.hpp"


NS_RESTD


// =============================================================================
// I.   IS_MOVE_CONSTRUCTIBLE
// =============================================================================

template<typename _Type>
struct is_move_constructible
    : integral_constant<bool,
          is_constructible<
              _Type,
              typename add_rvalue_reference<_Type>::type
          >::value>
{};


// =============================================================================
// II.  IS_MOVE_CONSTRUCTIBLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool is_move_constructible_v =
        is_move_constructible<_Type>::value;

#endif


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_MOVE_CONSTRUCTIBLE_
