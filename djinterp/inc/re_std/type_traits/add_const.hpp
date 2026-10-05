/*******************************************************************************
* djinterp [re_std]                                                add_const.hpp
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
* path:      /inc/re_std/type_traits/add_const.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_ADD_CONST_HPP
#define RE_STD_TYPE_TRAITS_ADD_CONST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   ADD_CONST
// =============================================================================

// add_const
//   trait: yields Type with a top-level const added. Per [meta.trans.cv],
// if Type is a reference, function, or already const, the trait is a
// no-op. The compiler enforces these rules naturally; no specializations
// are required.
template<typename Type>
struct add_const
{
    typedef const Type type;
};


// =============================================================================
// II.  ADD_CONST_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // add_const_t
    //   alias: convenience alias for add_const<Type>::type.
    template<typename Type>
    using add_const_t = typename add_const<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_ADD_CONST_HPP
