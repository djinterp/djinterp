/******************************************************************************
* djinterp [restd]                                               add_const.hpp
*
* add_const trait header:
*   Adds a top-level const-qualifier to a type. Yields member typedef
* `type` as the const-qualified form.
*
*     add_const<int>::type            -> const int
*     add_const<const int>::type      -> const int       (idempotent)
*     add_const<int&>::type           -> int&            (refs ignore cv)
*     add_const<int*>::type           -> int* const
*
*   PORTABILITY:
*   Reference types are unchanged: cv-qualifiers attached to a reference
* type are silently ignored per the C++ standard's reference rules.
*
*
* path:      /inc/djinterp/re_std/type_traits/add_const.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_ADD_CONST_
#define DJINTERP_RESTD_TYPE_TRAITS_ADD_CONST_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   ADD_CONST
// =============================================================================

// add_const
//   trait: yields _Type with a top-level const added. Per [meta.trans.cv],
// if _Type is a reference, function, or already const, the trait is a
// no-op. The compiler enforces these rules naturally; no specializations
// are required.
template<typename _Type>
struct add_const
{
    typedef const _Type type;
};


// =============================================================================
// II.  ADD_CONST_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // add_const_t
    //   alias: convenience alias for add_const<_Type>::type.
    template<typename _Type>
    using add_const_t = typename add_const<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_ADD_CONST_
