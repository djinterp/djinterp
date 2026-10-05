/*******************************************************************************
* djinterp [re_std]                                           remove_pointer.hpp
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
* path:      /inc/re_std/type_traits/remove_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_REMOVE_POINTER_HPP
#define RE_STD_TYPE_TRAITS_REMOVE_POINTER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   REMOVE_POINTER
// =============================================================================

// remove_pointer
//   trait: passthrough (primary template).
template<typename Type>
struct remove_pointer
{
    typedef Type type;
};

// remove_pointer<Type*>
//   trait: specialization stripping unqualified pointer.
template<typename Type>
struct remove_pointer<Type*>
{
    typedef Type type;
};

// remove_pointer<Type* const>
//   trait: specialization stripping const-qualified pointer.
template<typename Type>
struct remove_pointer<Type* const>
{
    typedef Type type;
};

// remove_pointer<Type* volatile>
//   trait: specialization stripping volatile-qualified pointer.
template<typename Type>
struct remove_pointer<Type* volatile>
{
    typedef Type type;
};

// remove_pointer<Type* const volatile>
//   trait: specialization stripping cv-qualified pointer.
template<typename Type>
struct remove_pointer<Type* const volatile>
{
    typedef Type type;
};


// =============================================================================
// II.  REMOVE_POINTER_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // remove_pointer_t
    //   alias: convenience alias for remove_pointer<Type>::type.
    template<typename Type>
    using remove_pointer_t = typename remove_pointer<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_REMOVE_POINTER_HPP
