/******************************************************************************
* djinterp [re_std]                                              remove_cv.hpp
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
* path:      /inc/djinterp/re_std/type_traits/remove_cv.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_REMOVE_CV_
#define DJINTERP_RE_STD_TYPE_TRAITS_REMOVE_CV_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./remove_const.hpp"
#include "./remove_volatile.hpp"


NS_RESTD


// =============================================================================
// I.   REMOVE_CV
// =============================================================================

// remove_cv
//   trait: strips top-level const and volatile via composition of
// remove_const and remove_volatile.
template<typename _Type>
struct remove_cv
{
    typedef typename remove_volatile<
                typename remove_const<_Type>::type
            >::type type;
};


// =============================================================================
// II.  REMOVE_CV_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // remove_cv_t
    //   alias: convenience alias for remove_cv<_Type>::type.
    template<typename _Type>
    using remove_cv_t = typename remove_cv<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_REMOVE_CV_
