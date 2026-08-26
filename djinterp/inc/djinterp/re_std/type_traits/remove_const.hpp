/******************************************************************************
* djinterp [re_std]                                           remove_const.hpp
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
* path:      /inc/djinterp/re_std/type_traits/remove_const.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_REMOVE_CONST_
#define DJINTERP_RE_STD_TYPE_TRAITS_REMOVE_CONST_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   REMOVE_CONST
// =============================================================================

// remove_const
//   trait: passthrough (primary template).
template<typename _Type>
struct remove_const
{
    typedef _Type type;
};

// remove_const<const _Type>
//   trait: specialization stripping top-level const.
template<typename _Type>
struct remove_const<const _Type>
{
    typedef _Type type;
};


// =============================================================================
// II.  REMOVE_CONST_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // remove_const_t
    //   alias: convenience alias for remove_const<_Type>::type.
    template<typename _Type>
    using remove_const_t = typename remove_const<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_REMOVE_CONST_
