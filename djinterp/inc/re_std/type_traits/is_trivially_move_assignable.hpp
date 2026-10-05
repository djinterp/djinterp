/*******************************************************************************
* djinterp [re_std]                             is_trivially_move_assignable.hpp
*
* is_trivially_move_assignable trait header:
*   Equivalent to is_trivially_assignable<Type&, Type&&>.
*
*
* path:      /inc/re_std/type_traits/is_trivially_move_assignable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_TRIVIALLY_MOVE_ASSIGNABLE_HPP
#define RE_STD_TYPE_TRAITS_IS_TRIVIALLY_MOVE_ASSIGNABLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./integral_constant.hpp"
#include "./is_trivially_assignable.hpp"
#include "./add_lvalue_reference.hpp"
#include "./add_rvalue_reference.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_TRIVIALLY_MOVE_ASSIGNABLE
// =============================================================================

template<typename Type>
struct is_trivially_move_assignable
    : integral_constant<bool,
          is_trivially_assignable<
              typename add_lvalue_reference<Type>::type,
              typename add_rvalue_reference<Type>::type
          >::value>
{};


// =============================================================================
// II.  IS_TRIVIALLY_MOVE_ASSIGNABLE_V
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename Type>
    RE_STD_CONSTEXPR bool is_trivially_move_assignable_v =
        is_trivially_move_assignable<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_TYPE_TRAITS_IS_TRIVIALLY_MOVE_ASSIGNABLE_HPP
