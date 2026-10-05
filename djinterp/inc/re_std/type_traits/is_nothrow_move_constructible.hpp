/*******************************************************************************
* djinterp [re_std]                            is_nothrow_move_constructible.hpp
*
* is_nothrow_move_constructible trait header:
*   Equivalent to is_nothrow_constructible<Type, Type&&>.
*
*
* path:      /inc/re_std/type_traits/is_nothrow_move_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_NOTHROW_MOVE_CONSTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_NOTHROW_MOVE_CONSTRUCTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// re_std
#include "./integral_constant.hpp"
#include "./is_nothrow_constructible.hpp"
#include "./add_rvalue_reference.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_NOTHROW_MOVE_CONSTRUCTIBLE
// =============================================================================

template<typename Type>
struct is_nothrow_move_constructible
    : integral_constant<bool,
          is_nothrow_constructible<
              Type,
              typename add_rvalue_reference<Type>::type
          >::value>
{};


// =============================================================================
// II.  IS_NOTHROW_MOVE_CONSTRUCTIBLE_V
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename Type>
    RE_STD_CONSTEXPR bool is_nothrow_move_constructible_v =
        is_nothrow_move_constructible<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TYPE_TRAITS_IS_NOTHROW_MOVE_CONSTRUCTIBLE_HPP
