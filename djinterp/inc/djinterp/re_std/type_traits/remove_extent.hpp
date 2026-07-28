/******************************************************************************
* djinterp [restd]                                            remove_extent.hpp
*
* remove_extent trait header:
*   Strips one level of array dimensioning from a type. If _Type is an
* array (bounded or unbounded), yields the element type; otherwise
* yields _Type unchanged.
*
*     remove_extent<int[5]>::type      -> int
*     remove_extent<int[]>::type       -> int
*     remove_extent<int[3][5]>::type   -> int[5]      (only one level)
*     remove_extent<int>::type         -> int         (passthrough)
*     remove_extent<int*>::type        -> int*        (pointers untouched)
*
*
* path:      /inc/djinterp/restd/type_traits/remove_extent.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_REMOVE_EXTENT_
#define DJINTERP_RESTD_TYPE_TRAITS_REMOVE_EXTENT_ 1

// std
#include <cstddef>
// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   REMOVE_EXTENT
// =============================================================================

// remove_extent
//   trait: passthrough (primary template).
template<typename _Type>
struct remove_extent
{
    typedef _Type type;
};

// remove_extent<_Type[]>
//   trait: unbounded array specialization.
template<typename _Type>
struct remove_extent<_Type[]>
{
    typedef _Type type;
};

// remove_extent<_Type[_N]>
//   trait: bounded array specialization.
template<typename    _Type,
         std::size_t _N>
struct remove_extent<_Type[_N]>
{
    typedef _Type type;
};


// =============================================================================
// II.  REMOVE_EXTENT_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // remove_extent_t
    //   alias: convenience alias for remove_extent<_Type>::type.
    template<typename _Type>
    using remove_extent_t = typename remove_extent<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_REMOVE_EXTENT_
