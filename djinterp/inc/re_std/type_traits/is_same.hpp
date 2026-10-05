/*******************************************************************************
* djinterp [re_std]                                                  is_same.hpp
*
* is_same trait header:
*   Compares two types for exact identity, including cv-qualification
* and reference category.
*
*     is_same<int, int>::value           -> true
*     is_same<int, const int>::value     -> false   (cv differs)
*     is_same<int, int&>::value          -> false   (reference differs)
*     is_same<int, signed int>::value    -> true    (same canonical type)
*     is_same<char, signed char>::value  -> false   (`char` is its own type)
*
*   Note: is_same is symmetric; is_same<A,B>::value == is_same<B,A>::value.
*
*
* path:      /inc/re_std/type_traits/is_same.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_SAME_HPP
#define RE_STD_TYPE_TRAITS_IS_SAME_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_SAME
// =============================================================================

// is_same
//   trait: false (primary template).
template<typename A,
         typename B>
struct is_same : false_type
{};

// is_same<A, A>
//   trait: true when both type parameters are the same type.
template<typename A>
struct is_same<A, A> : true_type
{};


// =============================================================================
// II.  IS_SAME_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_same_v
    //   variable: convenience for is_same<A, B>::value.
    template<typename A,
             typename B>
    RE_STD_CONSTEXPR bool is_same_v = is_same<A, B>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_SAME_HPP
