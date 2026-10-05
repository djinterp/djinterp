/*******************************************************************************
* djinterp [re_std]                                     is_nothrow_swappable.hpp
*
* is_nothrow_swappable trait:
*   true_type if objects of type Type can be swapped with each other AND
* the swap operation is noexcept; false_type otherwise. Equivalent to
* is_nothrow_swappable_with<Type&, Type&> for referenceable types, false
* for non-referenceable types (handled implicitly by add_lvalue_reference
* + SFINAE inside is_swappable_with).
*
*   PORTABILITY:
*   Available on C++11 and later. C++98/03 omits the trait.
*
*   DEPENDENCIES:
*   is_nothrow_swappable_with, add_lvalue_reference.
*
*
* path:      /inc/re_std/type_traits/is_nothrow_swappable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_NOTHROW_SWAPPABLE_HPP
#define RE_STD_TYPE_TRAITS_IS_NOTHROW_SWAPPABLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./is_nothrow_swappable_with.hpp"
#include "./add_lvalue_reference.hpp"


namespace re_std
{


    // is_nothrow_swappable
    //   trait: true_type if Type is swappable with itself and the swap
    //          operation is noexcept, false_type otherwise.
    template<typename Type>
    struct is_nothrow_swappable
        : is_nothrow_swappable_with<
              typename add_lvalue_reference<Type>::type,
              typename add_lvalue_reference<Type>::type >
    {};


    // is_nothrow_swappable_v (C++14+)
    #if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
        template<typename Type>
        RE_STD_CONSTEXPR bool is_nothrow_swappable_v
            = is_nothrow_swappable<Type>::value;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_IS_NOTHROW_SWAPPABLE_HPP
