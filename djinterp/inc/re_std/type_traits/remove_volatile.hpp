/*******************************************************************************
* djinterp [re_std]                                          remove_volatile.hpp
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
* path:      /inc/re_std/type_traits/remove_volatile.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_REMOVE_VOLATILE_HPP
#define RE_STD_TYPE_TRAITS_REMOVE_VOLATILE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   REMOVE_VOLATILE
// =============================================================================

// remove_volatile
//   trait: passthrough (primary template).
template<typename Type>
struct remove_volatile
{
    typedef Type type;
};

// remove_volatile<volatile Type>
//   trait: specialization stripping top-level volatile.
template<typename Type>
struct remove_volatile<volatile Type>
{
    typedef Type type;
};


// =============================================================================
// II.  REMOVE_VOLATILE_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // remove_volatile_t
    //   alias: convenience alias for remove_volatile<Type>::type.
    template<typename Type>
    using remove_volatile_t = typename remove_volatile<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_REMOVE_VOLATILE_HPP
