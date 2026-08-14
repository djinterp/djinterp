/******************************************************************************
* djinterp [restd]                                        remove_reference.hpp
*
* remove_reference trait header:
*   Removes one level of reference (lvalue or rvalue) from a type. CV-
* qualifiers on the referent are preserved.
*
*     remove_reference<int&>::type        -> int
*     remove_reference<int&&>::type       -> int          (C++11+ only)
*     remove_reference<const int&>::type  -> const int    (cv preserved)
*     remove_reference<int>::type         -> int          (passthrough)
*
*   PORTABILITY:
*   - C++98/03: only the lvalue reference specialization. References to
*     rvalues did not exist in the language.
*   - C++11+:   adds the rvalue reference specialization, gated on
*     D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES.
*
*
* path:      /inc/djinterp/re_std/type_traits/remove_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_REMOVE_REFERENCE_
#define DJINTERP_RESTD_TYPE_TRAITS_REMOVE_REFERENCE_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   REMOVE_REFERENCE
// =============================================================================

// remove_reference
//   trait: passthrough (primary template).
template<typename _Type>
struct remove_reference
{
    typedef _Type type;
};

// remove_reference<_Type&>
//   trait: specialization stripping lvalue reference.
template<typename _Type>
struct remove_reference<_Type&>
{
    typedef _Type type;
};

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    // remove_reference<_Type&&>
    //   trait: specialization stripping rvalue reference.
    template<typename _Type>
    struct remove_reference<_Type&&>
    {
        typedef _Type type;
    };

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =============================================================================
// II.  REMOVE_REFERENCE_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // remove_reference_t
    //   alias: convenience alias for remove_reference<_Type>::type.
    template<typename _Type>
    using remove_reference_t = typename remove_reference<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_REMOVE_REFERENCE_
