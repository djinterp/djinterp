/*******************************************************************************
* djinterp [re_std]                                                   void_t.hpp
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
* path:      /inc/re_std/type_traits/void_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_VOID_T_HPP
#define RE_STD_TYPE_TRAITS_VOID_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

// gate: requires alias templates + variadic templates
#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES &&                               \
      RE_STD_LANG_HAS_VARIADIC_TEMPLATES )


namespace re_std
{


// =============================================================================
// I.   VOID_T
// =============================================================================

namespace internal
{

    // make_void
    //   trait: maps any well-formed type sequence to void. Indirection
    // is required pre-CWG1558 (resolved in C++14) to make void_t
    // properly trigger SFINAE.
    template<typename...>
    struct make_void
    {
        typedef void type;
    };

}  // internal

// void_t
//   alias: maps any well-formed type sequence to void. Used to trigger
// SFINAE on the well-formedness of an arbitrary expression or type.
template<typename... Types>
using void_t = typename internal::make_void<Types...>::type;


}  // re_std


#endif  // alias templates && variadic templates


#endif  // RE_STD_TYPE_TRAITS_VOID_T_HPP
