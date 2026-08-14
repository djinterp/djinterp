/******************************************************************************
* djinterp [restd]                                             add_pointer.hpp
*
* add_pointer trait header:
*   Yields the pointer-to form of _Type, stripping any top-level
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
* path:      /inc/djinterp/re_std/type_traits/add_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_ADD_POINTER_
#define DJINTERP_RESTD_TYPE_TRAITS_ADD_POINTER_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./remove_reference.hpp"


NS_RESTD


// =============================================================================
// I.   ADD_POINTER
// =============================================================================

// add_pointer
//   trait: yields a pointer to the unreferenced form of _Type.
template<typename _Type>
struct add_pointer
{
    typedef typename remove_reference<_Type>::type* type;
};


// =============================================================================
// II.  ADD_POINTER_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // add_pointer_t
    //   alias: convenience alias for add_pointer<_Type>::type.
    template<typename _Type>
    using add_pointer_t = typename add_pointer<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_ADD_POINTER_
