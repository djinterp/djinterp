/*******************************************************************************
* djinterp [re_std]                                    is_move_constructible.hpp
*
* is_move_constructible trait header:
*   Equivalent to is_constructible<Type, Type&&>. Yields true_type if
* Type can be constructed from an rvalue reference to itself.
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
* path:      /inc/re_std/type_traits/is_move_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// re_std
#include "./integral_constant.hpp"
#include "./is_constructible.hpp"
#include "./add_rvalue_reference.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_MOVE_CONSTRUCTIBLE
// =============================================================================

template<typename Type>
struct is_move_constructible
    : integral_constant<bool,
          is_constructible<
              Type,
              typename add_rvalue_reference<Type>::type
          >::value>
{};


// =============================================================================
// II.  IS_MOVE_CONSTRUCTIBLE_V
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename Type>
    RE_STD_CONSTEXPR bool is_move_constructible_v =
        is_move_constructible<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TYPE_TRAITS_IS_MOVE_CONSTRUCTIBLE_HPP
