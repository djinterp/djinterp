/******************************************************************************
* djinterp [re_std]                                                is_same.hpp
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
* path:      /inc/djinterp/re_std/type_traits/is_same.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_SAME_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_SAME_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


NS_RESTD


// =============================================================================
// I.   IS_SAME
// =============================================================================

// is_same
//   trait: false (primary template).
template<typename _A,
         typename _B>
struct is_same : false_type
{};

// is_same<_A, _A>
//   trait: true when both type parameters are the same type.
template<typename _A>
struct is_same<_A, _A> : true_type
{};


// =============================================================================
// II.  IS_SAME_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_same_v
    //   variable: convenience for is_same<_A, _B>::value.
    template<typename _A,
             typename _B>
    D_CONSTEXPR bool is_same_v = is_same<_A, _B>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_SAME_
