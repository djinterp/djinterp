/*******************************************************************************
* djinterp [re_std]                                                enable_if.hpp
*
* enable_if trait header:
*   Defines member typedef `type` as `Type` when `Condition` is true.
* When `Condition` is false, the primary template has no `type` member,
* causing substitution failure (SFINAE).
*
*   USAGE:
*   In C++11+, typically used as a default template argument:
*     template<typename _T,
*              typename enable_if<is_integral<_T>::value, int>::type = 0>
*     void foo(_T _v);
*
*   In C++98/03, used as a return type or extra parameter:
*     template<typename _T>
*     typename enable_if<is_integral<_T>::value, void>::type
*     foo(_T _v);
*
*
* path:      /inc/re_std/type_traits/enable_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_ENABLE_IF_HPP
#define RE_STD_TYPE_TRAITS_ENABLE_IF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   ENABLE_IF
// =============================================================================

// enable_if
//   trait: SFINAE primitive. Has member typedef `type` only when
// `Condition` is true.
template<bool     Condition,
         typename Type = void>
struct enable_if
{};

// enable_if<true, Type>
//   trait: specialization for the true case; provides member typedef
// `type` as `Type`.
template<typename Type>
struct enable_if<true, Type>
{
    typedef Type type;
};


// =============================================================================
// II.  ENABLE_IF_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // enable_if_t
    //   alias: convenience alias for enable_if<Condition, Type>::type.
    template<bool     Condition,
             typename Type = void>
    using enable_if_t = typename enable_if<Condition, Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_ENABLE_IF_HPP
