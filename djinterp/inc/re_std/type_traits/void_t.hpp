/******************************************************************************
* djinterp [re_std]                                                 void_t.hpp
*
* void_t alias header:
*   Maps any well-formed type sequence to `void`. The cornerstone of
* the SFINAE detection idiom: a substitution failure in any of the
* template arguments disables the specialization.
*
*   USAGE:
*     template<typename, typename = void>
*     struct has_type_member : false_type {};
*
*     template<typename _T>
*     struct has_type_member<_T, void_t<typename _T::type>> : true_type {};
*
*   PORTABILITY:
*   Requires alias templates and variadic templates (C++11+). Not
* available on C++98/03.
*
*
* path:      /inc/djinterp/re_std/type_traits/void_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_VOID_T_
#define DJINTERP_RE_STD_TYPE_TRAITS_VOID_T_ 1

// djinterp
#include "../../core/djinterp.hpp"

// gate: requires alias templates + variadic templates
#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES &&                               \
      D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES )


NS_RESTD


// =============================================================================
// I.   VOID_T
// =============================================================================

NS_INTERNAL

    // make_void
    //   trait: maps any well-formed type sequence to void. Indirection
    // is required pre-CWG1558 (resolved in C++14) to make void_t
    // properly trigger SFINAE.
    template<typename...>
    struct make_void
    {
        typedef void type;
    };

NS_END  // internal

// void_t
//   alias: maps any well-formed type sequence to void. Used to trigger
// SFINAE on the well-formedness of an arbitrary expression or type.
template<typename... _Types>
using void_t = typename internal::make_void<_Types...>::type;


NS_END  // re_std


#endif  // alias templates && variadic templates


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_VOID_T_
