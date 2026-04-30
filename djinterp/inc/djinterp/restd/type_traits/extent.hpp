/******************************************************************************
* djinterp [restd]                                                  extent.hpp
*
* extent trait header:
*   Yields the size of the _Index-th dimension of _Type as a
* `std::size_t` value. For non-array types, or when _Index is out of
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
* path:      /inc/djinterp/restd/type_traits/extent.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_EXTENT_
#define DJINTERP_RESTD_TYPE_TRAITS_EXTENT_ 1

// std
#include <cstddef>
// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"


NS_RESTD


// =============================================================================
// I.   EXTENT
// =============================================================================

// extent
//   trait: 0 for non-array (primary template).
template<typename    _Type,
         unsigned    _Index = 0>
struct extent : integral_constant<std::size_t, 0>
{};

// extent<_Type[], 0>
//   trait: unbounded outer dimension at index 0 -> 0.
template<typename _Type>
struct extent<_Type[], 0>
    : integral_constant<std::size_t, 0>
{};

// extent<_Type[], _Index>
//   trait: unbounded outer dimension at deeper index -> recurse.
template<typename _Type,
         unsigned _Index>
struct extent<_Type[], _Index>
    : integral_constant<std::size_t, extent<_Type, _Index - 1>::value>
{};

// extent<_Type[_N], 0>
//   trait: bounded outer dimension at index 0 -> _N.
template<typename    _Type,
         std::size_t _N>
struct extent<_Type[_N], 0>
    : integral_constant<std::size_t, _N>
{};

// extent<_Type[_N], _Index>
//   trait: bounded outer dimension at deeper index -> recurse.
template<typename    _Type,
         std::size_t _N,
         unsigned    _Index>
struct extent<_Type[_N], _Index>
    : integral_constant<std::size_t, extent<_Type, _Index - 1>::value>
{};


// =============================================================================
// II.  EXTENT_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // extent_v
    //   variable: convenience for extent<_Type, _Index>::value.
    template<typename _Type,
             unsigned _Index = 0>
    D_CONSTEXPR std::size_t extent_v = extent<_Type, _Index>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_EXTENT_
