/******************************************************************************
* djinterp [restd]                                                    rank.hpp
*
* rank trait header:
*   Yields the number of array dimensions of _Type as a `std::size_t`
* value. For non-array types, yields 0.
*
*     rank<int>::value             -> 0
*     rank<int[]>::value           -> 1
*     rank<int[5]>::value          -> 1
*     rank<int[3][5]>::value       -> 2
*     rank<int[1][2][3][4]>::value -> 4
*
*
* path:      /inc/djinterp/restd/type_traits/rank.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_RANK_
#define DJINTERP_RESTD_TYPE_TRAITS_RANK_ 1

// std
#include <cstddef>
// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"


NS_RESTD


// =============================================================================
// I.   RANK
// =============================================================================

// rank
//   trait: 0 for non-array (primary template).
template<typename _Type>
struct rank : integral_constant<std::size_t, 0>
{};

// rank<_Type[]>
//   trait: unbounded array; recurse on element type and add 1.
template<typename _Type>
struct rank<_Type[]>
    : integral_constant<std::size_t, rank<_Type>::value + 1>
{};

// rank<_Type[_N]>
//   trait: bounded array; recurse on element type and add 1.
template<typename    _Type,
         std::size_t _N>
struct rank<_Type[_N]>
    : integral_constant<std::size_t, rank<_Type>::value + 1>
{};


// =============================================================================
// II.  RANK_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // rank_v
    //   variable: convenience for rank<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR std::size_t rank_v = rank<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_RANK_
