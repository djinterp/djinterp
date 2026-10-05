/*******************************************************************************
* djinterp [re_std]                                            remove_extent.hpp
*
* remove_extent trait header:
*   Strips one level of array dimensioning from a type. If Type is an
* array (bounded or unbounded), yields the element type; otherwise
* yields Type unchanged.
*
*     remove_extent<int[5]>::type      -> int
*     remove_extent<int[]>::type       -> int
*     remove_extent<int[3][5]>::type   -> int[5]      (only one level)
*     remove_extent<int>::type         -> int         (passthrough)
*     remove_extent<int*>::type        -> int*        (pointers untouched)
*
*
* path:      /inc/re_std/type_traits/remove_extent.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_REMOVE_EXTENT_HPP
#define RE_STD_TYPE_TRAITS_REMOVE_EXTENT_HPP 1

// std
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   REMOVE_EXTENT
// =============================================================================

// remove_extent
//   trait: passthrough (primary template).
template<typename Type>
struct remove_extent
{
    typedef Type type;
};

// remove_extent<Type[]>
//   trait: unbounded array specialization.
template<typename Type>
struct remove_extent<Type[]>
{
    typedef Type type;
};

// remove_extent<Type[N]>
//   trait: bounded array specialization.
template<typename    Type,
         std::size_t N>
struct remove_extent<Type[N]>
{
    typedef Type type;
};


// =============================================================================
// II.  REMOVE_EXTENT_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // remove_extent_t
    //   alias: convenience alias for remove_extent<Type>::type.
    template<typename Type>
    using remove_extent_t = typename remove_extent<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_REMOVE_EXTENT_HPP
