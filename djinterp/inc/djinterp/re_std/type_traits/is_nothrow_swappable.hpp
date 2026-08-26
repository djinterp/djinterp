/******************************************************************************
* djinterp [re_std]                                    is_nothrow_swappable.hpp
*
* is_nothrow_swappable trait:
*   true_type if objects of type _Type can be swapped with each other AND
* the swap operation is noexcept; false_type otherwise. Equivalent to
* is_nothrow_swappable_with<_Type&, _Type&> for referenceable types, false
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
* path:      /inc/djinterp/re_std/type_traits/is_nothrow_swappable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_SWAPPABLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_SWAPPABLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./is_nothrow_swappable_with.hpp"
#include "./add_lvalue_reference.hpp"


NS_RESTD


    // is_nothrow_swappable
    //   trait: true_type if _Type is swappable with itself and the swap
    //          operation is noexcept, false_type otherwise.
    template<typename _Type>
    struct is_nothrow_swappable
        : is_nothrow_swappable_with<
              typename add_lvalue_reference<_Type>::type,
              typename add_lvalue_reference<_Type>::type >
    {};


    // is_nothrow_swappable_v (C++14+)
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        template<typename _Type>
        D_CONSTEXPR bool is_nothrow_swappable_v
            = is_nothrow_swappable<_Type>::value;
    #endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_SWAPPABLE_
