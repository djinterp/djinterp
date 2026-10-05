/*******************************************************************************
* djinterp [re_std]                                         remove_reference.hpp
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
*     RE_STD_LANG_HAS_RVALUE_REFERENCES.
*
*
* path:      /inc/re_std/type_traits/remove_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_REMOVE_REFERENCE_HPP
#define RE_STD_TYPE_TRAITS_REMOVE_REFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   REMOVE_REFERENCE
// =============================================================================

// remove_reference
//   trait: passthrough (primary template).
template<typename Type>
struct remove_reference
{
    typedef Type type;
};

// remove_reference<Type&>
//   trait: specialization stripping lvalue reference.
template<typename Type>
struct remove_reference<Type&>
{
    typedef Type type;
};

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    // remove_reference<Type&&>
    //   trait: specialization stripping rvalue reference.
    template<typename Type>
    struct remove_reference<Type&&>
    {
        typedef Type type;
    };

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES


// =============================================================================
// II.  REMOVE_REFERENCE_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // remove_reference_t
    //   alias: convenience alias for remove_reference<Type>::type.
    template<typename Type>
    using remove_reference_t = typename remove_reference<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_REMOVE_REFERENCE_HPP
