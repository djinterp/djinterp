/******************************************************************************
* djinterp [restd]                                                  add_cv.hpp
*
* add_cv trait header:
*   Adds top-level const and volatile qualifiers to a type. Composition
* of add_const over add_volatile (order is irrelevant).
*
*     add_cv<int>::type            -> const volatile int
*     add_cv<const int>::type      -> const volatile int    (idempotent on c)
*     add_cv<int&>::type           -> int&                  (refs ignore cv)
*
*
* path:      /inc/djinterp/re_std/type_traits/add_cv.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_ADD_CV_
#define DJINTERP_RESTD_TYPE_TRAITS_ADD_CV_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./add_const.hpp"
#include "./add_volatile.hpp"


NS_RESTD


// =============================================================================
// I.   ADD_CV
// =============================================================================

// add_cv
//   trait: yields _Type with both top-level const and volatile added.
template<typename _Type>
struct add_cv
{
    typedef typename add_const<
                typename add_volatile<_Type>::type
            >::type type;
};


// =============================================================================
// II.  ADD_CV_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // add_cv_t
    //   alias: convenience alias for add_cv<_Type>::type.
    template<typename _Type>
    using add_cv_t = typename add_cv<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_ADD_CV_
