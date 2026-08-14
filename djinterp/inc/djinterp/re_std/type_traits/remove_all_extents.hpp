/******************************************************************************
* djinterp [restd]                                       remove_all_extents.hpp
*
* remove_all_extents trait header:
*   Recursively strips all array dimensions from a type. If _Type is an
* array of arrays, all extents are removed and the innermost element
* type is yielded. Non-array types are passthrough.
*
*     remove_all_extents<int[5]>::type            -> int
*     remove_all_extents<int[3][5]>::type         -> int
*     remove_all_extents<int[][5]>::type          -> int
*     remove_all_extents<int[2][3][4][5]>::type   -> int
*     remove_all_extents<int>::type               -> int      (passthrough)
*     remove_all_extents<int*[5]>::type           -> int*     (only arrays)
*
*
* path:      /inc/djinterp/re_std/type_traits/remove_all_extents.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_REMOVE_ALL_EXTENTS_
#define DJINTERP_RESTD_TYPE_TRAITS_REMOVE_ALL_EXTENTS_ 1

// std
#include <cstddef>
// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   REMOVE_ALL_EXTENTS
// =============================================================================

// remove_all_extents
//   trait: passthrough (primary template).
template<typename _Type>
struct remove_all_extents
{
    typedef _Type type;
};

// remove_all_extents<_Type[]>
//   trait: unbounded array; recurse on element type.
template<typename _Type>
struct remove_all_extents<_Type[]>
{
    typedef typename remove_all_extents<_Type>::type type;
};

// remove_all_extents<_Type[_N]>
//   trait: bounded array; recurse on element type.
template<typename    _Type,
         std::size_t _N>
struct remove_all_extents<_Type[_N]>
{
    typedef typename remove_all_extents<_Type>::type type;
};


// =============================================================================
// II.  REMOVE_ALL_EXTENTS_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // remove_all_extents_t
    //   alias: convenience alias for remove_all_extents<_Type>::type.
    template<typename _Type>
    using remove_all_extents_t =
        typename remove_all_extents<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_REMOVE_ALL_EXTENTS_
