/******************************************************************************
* djinterp [restd]                                            add_volatile.hpp
*
* add_volatile trait header:
*   Adds a top-level volatile-qualifier to a type. Yields member typedef
* `type` as the volatile-qualified form.
*
*     add_volatile<int>::type             -> volatile int
*     add_volatile<volatile int>::type    -> volatile int   (idempotent)
*     add_volatile<int&>::type            -> int&           (refs ignore cv)
*     add_volatile<int*>::type            -> int* volatile
*
*
* path:      /inc/djinterp/re_std/type_traits/add_volatile.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_ADD_VOLATILE_
#define DJINTERP_RESTD_TYPE_TRAITS_ADD_VOLATILE_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   ADD_VOLATILE
// =============================================================================

// add_volatile
//   trait: yields _Type with a top-level volatile added. Per
// [meta.trans.cv], if _Type is a reference, function, or already
// volatile, the trait is a no-op. The compiler enforces these rules
// naturally; no specializations are required.
template<typename _Type>
struct add_volatile
{
    typedef volatile _Type type;
};


// =============================================================================
// II.  ADD_VOLATILE_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // add_volatile_t
    //   alias: convenience alias for add_volatile<_Type>::type.
    template<typename _Type>
    using add_volatile_t = typename add_volatile<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_ADD_VOLATILE_
