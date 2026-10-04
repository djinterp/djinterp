/******************************************************************************
* djinterp [re_std]                                          type_identity.hpp
*
* type_identity trait header:
*   Wraps a type as a non-deduced context. Yields member typedef `type`
* as `_Type` unchanged. Used to defeat template argument deduction for
* a parameter, forcing the caller to specify _Type explicitly.
*
*   PORTABILITY:
*   The trait itself is implementable on C++98+. The `_t` alias requires
* alias templates (C++11+).
*
*
* path:      /inc/djinterp/re_std/type_traits/type_identity.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_TYPE_IDENTITY_
#define DJINTERP_RE_STD_TYPE_TRAITS_TYPE_IDENTITY_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   TYPE_IDENTITY
// =============================================================================

// type_identity
//   trait: passthrough; yields `type` as `_Type` unchanged. Used as a
// non-deduced context to suppress argument deduction.
template<typename _Type>
struct type_identity
{
    typedef _Type type;
};


// =============================================================================
// II.  TYPE_IDENTITY_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // type_identity_t
    //   alias: convenience alias for type_identity<_Type>::type.
    template<typename _Type>
    using type_identity_t = typename type_identity<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_TYPE_IDENTITY_
