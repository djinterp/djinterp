/*******************************************************************************
* djinterp [re_std]                                       remove_all_extents.hpp
*
* remove_all_extents trait header:
*   Recursively strips all array dimensions from a type. If Type is an
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
* path:      /inc/re_std/type_traits/remove_all_extents.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_REMOVE_ALL_EXTENTS_HPP
#define RE_STD_TYPE_TRAITS_REMOVE_ALL_EXTENTS_HPP 1

// std
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   REMOVE_ALL_EXTENTS
// =============================================================================

// remove_all_extents
//   trait: passthrough (primary template).
template<typename Type>
struct remove_all_extents
{
    typedef Type type;
};

// remove_all_extents<Type[]>
//   trait: unbounded array; recurse on element type.
template<typename Type>
struct remove_all_extents<Type[]>
{
    typedef typename remove_all_extents<Type>::type type;
};

// remove_all_extents<Type[N]>
//   trait: bounded array; recurse on element type.
template<typename    Type,
         std::size_t N>
struct remove_all_extents<Type[N]>
{
    typedef typename remove_all_extents<Type>::type type;
};


// =============================================================================
// II.  REMOVE_ALL_EXTENTS_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // remove_all_extents_t
    //   alias: convenience alias for remove_all_extents<Type>::type.
    template<typename Type>
    using remove_all_extents_t =
        typename remove_all_extents<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_REMOVE_ALL_EXTENTS_HPP
