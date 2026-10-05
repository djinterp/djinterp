/*******************************************************************************
* djinterp [re_std]                                            type_identity.hpp
*
* type_identity trait header:
*   Wraps a type as a non-deduced context. Yields member typedef `type`
* as `Type` unchanged. Used to defeat template argument deduction for
* a parameter, forcing the caller to specify Type explicitly.
*
*   PORTABILITY:
*   The trait itself is implementable on C++98+. The `_t` alias requires
* alias templates (C++11+).
*
*
* path:      /inc/re_std/type_traits/type_identity.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_TYPE_IDENTITY_HPP
#define RE_STD_TYPE_TRAITS_TYPE_IDENTITY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   TYPE_IDENTITY
// =============================================================================

// type_identity
//   trait: passthrough; yields `type` as `Type` unchanged. Used as a
// non-deduced context to suppress argument deduction.
template<typename Type>
struct type_identity
{
    typedef Type type;
};


// =============================================================================
// II.  TYPE_IDENTITY_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // type_identity_t
    //   alias: convenience alias for type_identity<Type>::type.
    template<typename Type>
    using type_identity_t = typename type_identity<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_TYPE_IDENTITY_HPP
