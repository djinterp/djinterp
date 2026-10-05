/*******************************************************************************
* djinterp [re_std]                                              add_pointer.hpp
*
* add_pointer trait header:
*   Yields the pointer-to form of Type, stripping any top-level
* reference first (since `T&*` is ill-formed). Per [meta.trans.ptr].
*
*     add_pointer<int>::type           -> int*
*     add_pointer<int&>::type          -> int*           (ref stripped)
*     add_pointer<int&&>::type         -> int*           (ref stripped, C++11+)
*     add_pointer<int*>::type          -> int**
*     add_pointer<const int>::type     -> const int*
*     add_pointer<void()>::type        -> void(*)()      (function pointer)
*     add_pointer<void>::type          -> void*
*
*   PORTABILITY:
*   The reference-stripping step uses remove_reference; for non-
* referenceable types (the abstract case where forming a pointer fails,
* e.g. function types with cv/ref qualifiers), behavior matches the
* compiler's natural rules.
*
*
* path:      /inc/re_std/type_traits/add_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_ADD_POINTER_HPP
#define RE_STD_TYPE_TRAITS_ADD_POINTER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./remove_reference.hpp"


namespace re_std
{


// =============================================================================
// I.   ADD_POINTER
// =============================================================================

// add_pointer
//   trait: yields a pointer to the unreferenced form of Type.
template<typename Type>
struct add_pointer
{
    typedef typename remove_reference<Type>::type* type;
};


// =============================================================================
// II.  ADD_POINTER_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // add_pointer_t
    //   alias: convenience alias for add_pointer<Type>::type.
    template<typename Type>
    using add_pointer_t = typename add_pointer<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_ADD_POINTER_HPP
