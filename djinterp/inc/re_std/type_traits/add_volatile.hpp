/*******************************************************************************
* djinterp [re_std]                                             add_volatile.hpp
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
* path:      /inc/re_std/type_traits/add_volatile.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_ADD_VOLATILE_HPP
#define RE_STD_TYPE_TRAITS_ADD_VOLATILE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   ADD_VOLATILE
// =============================================================================

// add_volatile
//   trait: yields Type with a top-level volatile added. Per
// [meta.trans.cv], if Type is a reference, function, or already
// volatile, the trait is a no-op. The compiler enforces these rules
// naturally; no specializations are required.
template<typename Type>
struct add_volatile
{
    typedef volatile Type type;
};


// =============================================================================
// II.  ADD_VOLATILE_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // add_volatile_t
    //   alias: convenience alias for add_volatile<Type>::type.
    template<typename Type>
    using add_volatile_t = typename add_volatile<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_ADD_VOLATILE_HPP
