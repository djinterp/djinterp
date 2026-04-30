/******************************************************************************
* djinterp [restd]                                          remove_pointer.hpp
*
* remove_pointer trait header:
*   Removes one level of pointer indirection, including through cv-
* qualified pointer forms (T* const, T* volatile, T* const volatile).
*
*     remove_pointer<int*>::type                 -> int
*     remove_pointer<int* const>::type           -> int
*     remove_pointer<int* volatile>::type        -> int
*     remove_pointer<int* const volatile>::type  -> int
*     remove_pointer<int>::type                  -> int  (passthrough)
*     remove_pointer<int**>::type                -> int* (one level only)
*     remove_pointer<const int*>::type           -> const int  (cv on pointee
*                                                              is preserved)
*
*
* path:      /inc/djinterp/restd/type_traits/remove_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_REMOVE_POINTER_
#define DJINTERP_RESTD_TYPE_TRAITS_REMOVE_POINTER_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   REMOVE_POINTER
// =============================================================================

// remove_pointer
//   trait: passthrough (primary template).
template<typename _Type>
struct remove_pointer
{
    typedef _Type type;
};

// remove_pointer<_Type*>
//   trait: specialization stripping unqualified pointer.
template<typename _Type>
struct remove_pointer<_Type*>
{
    typedef _Type type;
};

// remove_pointer<_Type* const>
//   trait: specialization stripping const-qualified pointer.
template<typename _Type>
struct remove_pointer<_Type* const>
{
    typedef _Type type;
};

// remove_pointer<_Type* volatile>
//   trait: specialization stripping volatile-qualified pointer.
template<typename _Type>
struct remove_pointer<_Type* volatile>
{
    typedef _Type type;
};

// remove_pointer<_Type* const volatile>
//   trait: specialization stripping cv-qualified pointer.
template<typename _Type>
struct remove_pointer<_Type* const volatile>
{
    typedef _Type type;
};


// =============================================================================
// II.  REMOVE_POINTER_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // remove_pointer_t
    //   alias: convenience alias for remove_pointer<_Type>::type.
    template<typename _Type>
    using remove_pointer_t = typename remove_pointer<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_REMOVE_POINTER_
