/*******************************************************************************
* djinterp [re_std]                                             remove_const.hpp
*
* remove_const trait header:
*   Strips top-level const-qualifier from a type. Yields member typedef
* `type` as the unqualified form.
*
*     remove_const<const int>::type           -> int
*     remove_const<int>::type                 -> int          (passthrough)
*     remove_const<const int*>::type          -> const int*   (top-level only)
*     remove_const<int* const>::type          -> int*         (top-level only)
*     remove_const<const volatile int>::type  -> volatile int (volatile kept)
*
*
* path:      /inc/re_std/type_traits/remove_const.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_REMOVE_CONST_HPP
#define RE_STD_TYPE_TRAITS_REMOVE_CONST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   REMOVE_CONST
// =============================================================================

// remove_const
//   trait: passthrough (primary template).
template<typename Type>
struct remove_const
{
    typedef Type type;
};

// remove_const<const Type>
//   trait: specialization stripping top-level const.
template<typename Type>
struct remove_const<const Type>
{
    typedef Type type;
};


// =============================================================================
// II.  REMOVE_CONST_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // remove_const_t
    //   alias: convenience alias for remove_const<Type>::type.
    template<typename Type>
    using remove_const_t = typename remove_const<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_REMOVE_CONST_HPP
