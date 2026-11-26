/*******************************************************************************
* djinterp [re_std]                         is_nothrow_default_constructible.hpp
*
* is_nothrow_default_constructible trait header:
*   Equivalent to is_nothrow_constructible<Type>. Yields true_type if
* default construction of Type is well-formed and noexcept.
*
*
* path:      /inc/re_std/cpp/type_traits/is_nothrow_default_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  IS_NOTHROW_DEFAULT_CONSTRUCTIBLE
    --------------------------------
2.  IS_NOTHROW_DEFAULT_CONSTRUCTIBLE_V
    ----------------------------------
*/

#ifndef RE_STD_TYPE_TRAITS_IS_NOTHROW_DEFAULT_CONSTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_NOTHROW_DEFAULT_CONSTRUCTIBLE_HPP 1

// re_std
#include "../../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// re_std
#include "./integral_constant.hpp"
#include "./is_nothrow_constructible.hpp"


namespace re_std
{


//==============================================================================
// 1.  IS_NOTHROW_DEFAULT_CONSTRUCTIBLE
//==============================================================================

template<typename Type>
struct is_nothrow_default_constructible
    : integral_constant<bool, is_nothrow_constructible<Type>::value>
{};


//==============================================================================
// 2.  IS_NOTHROW_DEFAULT_CONSTRUCTIBLE_V
//==============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename Type>
    RE_STD_CONSTEXPR bool is_nothrow_default_constructible_v =
        is_nothrow_default_constructible<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TYPE_TRAITS_IS_NOTHROW_DEFAULT_CONSTRUCTIBLE_HPP
