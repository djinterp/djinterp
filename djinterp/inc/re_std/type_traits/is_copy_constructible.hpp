/*******************************************************************************
* djinterp [re_std]                                    is_copy_constructible.hpp
*
* is_copy_constructible trait header:
*   Equivalent to is_constructible<Type, Type const&>. Yields true_type
* if Type can be constructed from a const lvalue reference to itself.
*
*     is_copy_constructible<int>::value           -> true
*     struct A { A(const A&) = delete; };
*     is_copy_constructible<A>::value             -> false
*
*
* path:      /inc/re_std/type_traits/is_copy_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_COPY_CONSTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_COPY_CONSTRUCTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// re_std
#include "./integral_constant.hpp"
#include "./is_constructible.hpp"
#include "./add_const.hpp"
#include "./add_lvalue_reference.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_COPY_CONSTRUCTIBLE
// =============================================================================

template<typename Type>
struct is_copy_constructible
    : integral_constant<bool,
          is_constructible<
              Type,
              typename add_lvalue_reference<
                  typename add_const<Type>::type
              >::type
          >::value>
{};


// =============================================================================
// II.  IS_COPY_CONSTRUCTIBLE_V
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename Type>
    RE_STD_CONSTEXPR bool is_copy_constructible_v =
        is_copy_constructible<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TYPE_TRAITS_IS_COPY_CONSTRUCTIBLE_HPP
