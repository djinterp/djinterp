/******************************************************************************
* djinterp [re_std]                                               is_array.hpp
*
* is_array trait header:
*   Detects whether a type is a C-style array, bounded or unbounded.
*
*     is_array<int[5]>::value   -> true   (bounded)
*     is_array<int[]>::value    -> true   (unbounded)
*     is_array<int[3][4]>::value -> true  (multidimensional)
*     is_array<int>::value      -> false
*     is_array<int*>::value     -> false  (pointer is not array)
*     is_array<std::array<int, 5>>::value -> false  (std::array is a class)
*
*   Note: cv-qualifiers and references are not stripped here. is_array
* on a const-qualified array type is still true because const-qualifying
* an array type yields an array of const elements.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_ARRAY_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_ARRAY_ 1

// 
#include <cstddef>
// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


NS_RESTD


// =============================================================================
// I.   IS_ARRAY
// =============================================================================

// is_array
//   trait: false (primary template).
template<typename _Type>
struct is_array : false_type
{};

// is_array<_Type[]>
//   trait: true for unbounded arrays.
template<typename _Type>
struct is_array<_Type[]> : true_type
{};

// is_array<_Type[_N]>
//   trait: true for bounded arrays.
template<typename _Type,
         std::size_t _N>
struct is_array<_Type[_N]> : true_type
{};


// =============================================================================
// II.  IS_ARRAY_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_array_v
    //   variable: convenience for is_array<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_array_v = is_array<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_ARRAY_
