/*******************************************************************************
* djinterp [re_std]                                                   extent.hpp
*
* extent trait header:
*   Yields the size of the Index-th dimension of Type as a
* `std::size_t` value. For non-array types, or when Index is out of
* range, or when querying an unbounded dimension, yields 0.
*
*     extent<int[5]>::value             -> 5
*     extent<int[5], 0>::value          -> 5         (default index)
*     extent<int[3][5], 0>::value       -> 3
*     extent<int[3][5], 1>::value       -> 5
*     extent<int[3][5], 2>::value       -> 0         (out of range)
*     extent<int[]>::value              -> 0         (unbounded)
*     extent<int[][5], 0>::value        -> 0         (unbounded outer)
*     extent<int[][5], 1>::value        -> 5         (bounded inner)
*     extent<int>::value                -> 0         (not array)
*
*
* path:      /inc/re_std/type_traits/extent.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_EXTENT_HPP
#define RE_STD_TYPE_TRAITS_EXTENT_HPP 1

// std
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"


namespace re_std
{


// =============================================================================
// I.   EXTENT
// =============================================================================

// extent
//   trait: 0 for non-array (primary template).
template<typename    Type,
         unsigned    Index = 0>
struct extent : integral_constant<std::size_t, 0>
{};

// extent<Type[], 0>
//   trait: unbounded outer dimension at index 0 -> 0.
template<typename Type>
struct extent<Type[], 0>
    : integral_constant<std::size_t, 0>
{};

// extent<Type[], Index>
//   trait: unbounded outer dimension at deeper index -> recurse.
template<typename Type,
         unsigned Index>
struct extent<Type[], Index>
    : integral_constant<std::size_t, extent<Type, Index - 1>::value>
{};

// extent<Type[N], 0>
//   trait: bounded outer dimension at index 0 -> N.
template<typename    Type,
         std::size_t N>
struct extent<Type[N], 0>
    : integral_constant<std::size_t, N>
{};

// extent<Type[N], Index>
//   trait: bounded outer dimension at deeper index -> recurse.
template<typename    Type,
         std::size_t N,
         unsigned    Index>
struct extent<Type[N], Index>
    : integral_constant<std::size_t, extent<Type, Index - 1>::value>
{};


// =============================================================================
// II.  EXTENT_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // extent_v
    //   variable: convenience for extent<Type, Index>::value.
    template<typename Type,
             unsigned Index = 0>
    RE_STD_CONSTEXPR std::size_t extent_v = extent<Type, Index>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_EXTENT_HPP
