/*******************************************************************************
* djinterp [re_std]                                              is_compound.hpp
*
* is_compound trait header:
*   Yields true_type if Type is a compound type, false_type otherwise.
* A compound type is any type that is NOT fundamental: arrays, functions,
* pointers, references, classes, unions, enumerations, and pointers-to-
* members. Equivalent to !is_fundamental.
*
*     is_compound<int>::value         -> false  (fundamental)
*     is_compound<int*>::value        -> true   (pointer)
*     is_compound<int[5]>::value      -> true   (array)
*     is_compound<int&>::value        -> true   (reference)
*     is_compound<void()>::value      -> true   (function)
*     is_compound<S>::value           -> true   (class)
*     is_compound<E>::value           -> true   (enum, when intrinsic available)
*     is_compound<void>::value        -> false  (fundamental)
*
*
* path:      /inc/re_std/type_traits/is_compound.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_COMPOUND_HPP
#define RE_STD_TYPE_TRAITS_IS_COMPOUND_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./is_fundamental.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_COMPOUND
// =============================================================================

// is_compound
//   trait: !is_fundamental.
template<typename Type>
struct is_compound
    : integral_constant<bool, !is_fundamental<Type>::value>
{};


// =============================================================================
// II.  IS_COMPOUND_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_compound_v
    //   variable: convenience for is_compound<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_compound_v = is_compound<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_COMPOUND_HPP
