/******************************************************************************
* djinterp [re_std]                                        remove_volatile.hpp
*
* remove_volatile trait header:
*   Strips top-level volatile-qualifier from a type. Yields member
* typedef `type` as the unqualified form.
*
*     remove_volatile<volatile int>::type        -> int
*     remove_volatile<int>::type                 -> int          (passthrough)
*     remove_volatile<volatile int*>::type       -> volatile int*
*                                                 (top-level only)
*     remove_volatile<int* volatile>::type       -> int*
*                                                 (top-level only)
*     remove_volatile<const volatile int>::type  -> const int    (const kept)
*
*
* path:      /inc/djinterp/re_std/type_traits/remove_volatile.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_REMOVE_VOLATILE_
#define DJINTERP_RE_STD_TYPE_TRAITS_REMOVE_VOLATILE_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   REMOVE_VOLATILE
// =============================================================================

// remove_volatile
//   trait: passthrough (primary template).
template<typename _Type>
struct remove_volatile
{
    typedef _Type type;
};

// remove_volatile<volatile _Type>
//   trait: specialization stripping top-level volatile.
template<typename _Type>
struct remove_volatile<volatile _Type>
{
    typedef _Type type;
};


// =============================================================================
// II.  REMOVE_VOLATILE_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // remove_volatile_t
    //   alias: convenience alias for remove_volatile<_Type>::type.
    template<typename _Type>
    using remove_volatile_t = typename remove_volatile<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_REMOVE_VOLATILE_
