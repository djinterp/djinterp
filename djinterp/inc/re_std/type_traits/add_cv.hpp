/*******************************************************************************
* djinterp [re_std]                                                   add_cv.hpp
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
* path:      /inc/re_std/type_traits/add_cv.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_ADD_CV_HPP
#define RE_STD_TYPE_TRAITS_ADD_CV_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./add_const.hpp"
#include "./add_volatile.hpp"


namespace re_std
{


// =============================================================================
// I.   ADD_CV
// =============================================================================

// add_cv
//   trait: yields Type with both top-level const and volatile added.
template<typename Type>
struct add_cv
{
    typedef typename add_const<
                typename add_volatile<Type>::type
            >::type type;
};


// =============================================================================
// II.  ADD_CV_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // add_cv_t
    //   alias: convenience alias for add_cv<Type>::type.
    template<typename Type>
    using add_cv_t = typename add_cv<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_ADD_CV_HPP
