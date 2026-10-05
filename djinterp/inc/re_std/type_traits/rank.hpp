/*******************************************************************************
* djinterp [re_std]                                                     rank.hpp
*
* rank trait header:
*   Yields the number of array dimensions of Type as a `std::size_t`
* value. For non-array types, yields 0.
*
*     rank<int>::value             -> 0
*     rank<int[]>::value           -> 1
*     rank<int[5]>::value          -> 1
*     rank<int[3][5]>::value       -> 2
*     rank<int[1][2][3][4]>::value -> 4
*
*
* path:      /inc/re_std/type_traits/rank.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_RANK_HPP
#define RE_STD_TYPE_TRAITS_RANK_HPP 1

// std
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"


namespace re_std
{


// =============================================================================
// I.   RANK
// =============================================================================

// rank
//   trait: 0 for non-array (primary template).
template<typename Type>
struct rank : integral_constant<std::size_t, 0>
{};

// rank<Type[]>
//   trait: unbounded array; recurse on element type and add 1.
template<typename Type>
struct rank<Type[]>
    : integral_constant<std::size_t, rank<Type>::value + 1>
{};

// rank<Type[N]>
//   trait: bounded array; recurse on element type and add 1.
template<typename    Type,
         std::size_t N>
struct rank<Type[N]>
    : integral_constant<std::size_t, rank<Type>::value + 1>
{};


// =============================================================================
// II.  RANK_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // rank_v
    //   variable: convenience for rank<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR std::size_t rank_v = rank<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_RANK_HPP
