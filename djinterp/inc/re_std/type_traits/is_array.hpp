/*******************************************************************************
* djinterp [re_std]                                                 is_array.hpp
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
* path:      /inc/re_std/type_traits/is_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_ARRAY_HPP
#define RE_STD_TYPE_TRAITS_IS_ARRAY_HPP 1

// std
//
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_ARRAY
// =============================================================================

// is_array
//   trait: false (primary template).
template<typename Type>
struct is_array : false_type
{};

// is_array<Type[]>
//   trait: true for unbounded arrays.
template<typename Type>
struct is_array<Type[]> : true_type
{};

// is_array<Type[N]>
//   trait: true for bounded arrays.
template<typename Type,
         std::size_t N>
struct is_array<Type[N]> : true_type
{};


// =============================================================================
// II.  IS_ARRAY_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_array_v
    //   variable: convenience for is_array<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_array_v = is_array<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_ARRAY_HPP
