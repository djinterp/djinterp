/*******************************************************************************
* djinterp [re_std]                                                remove_cv.hpp
*
* remove_cv trait header:
*   Strips both top-level const and volatile qualifiers from a type.
* Composes remove_const and remove_volatile.
*
*     remove_cv<const volatile int>::type  -> int
*     remove_cv<const int>::type           -> int
*     remove_cv<volatile int>::type        -> int
*     remove_cv<int>::type                 -> int  (passthrough)
*     remove_cv<const int*>::type          -> const int*  (top-level only)
*
*
* path:      /inc/re_std/type_traits/remove_cv.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_REMOVE_CV_HPP
#define RE_STD_TYPE_TRAITS_REMOVE_CV_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./remove_const.hpp"
#include "./remove_volatile.hpp"


namespace re_std
{


// =============================================================================
// I.   REMOVE_CV
// =============================================================================

// remove_cv
//   trait: strips top-level const and volatile via composition of
// remove_const and remove_volatile.
template<typename Type>
struct remove_cv
{
    typedef typename remove_volatile<
                typename remove_const<Type>::type
            >::type type;
};


// =============================================================================
// II.  REMOVE_CV_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // remove_cv_t
    //   alias: convenience alias for remove_cv<Type>::type.
    template<typename Type>
    using remove_cv_t = typename remove_cv<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_REMOVE_CV_HPP
