/*******************************************************************************
* djinterp [re_std]                                 is_default_constructible.hpp
*
* is_default_constructible trait header:
*   Equivalent to is_constructible<Type>. Yields true_type if Type is
* default-constructible (i.e. `Type t;` is well-formed), false_type
* otherwise.
*
*     is_default_constructible<int>::value     -> true
*     struct A { A() = delete; };
*     is_default_constructible<A>::value       -> false
*
*
* path:      /inc/re_std/type_traits/is_default_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_DEFAULT_CONSTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_DEFAULT_CONSTRUCTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: depends on is_constructible
#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// re_std
#include "./integral_constant.hpp"
#include "./is_constructible.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_DEFAULT_CONSTRUCTIBLE
// =============================================================================

template<typename Type>
struct is_default_constructible
    : integral_constant<bool, is_constructible<Type>::value>
{};


// =============================================================================
// II.  IS_DEFAULT_CONSTRUCTIBLE_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename Type>
    RE_STD_CONSTEXPR bool is_default_constructible_v =
        is_default_constructible<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TYPE_TRAITS_IS_DEFAULT_CONSTRUCTIBLE_HPP
