/******************************************************************************
* djinterp [restd]                                               enable_if.hpp
*
* enable_if trait header:
*   Defines member typedef `type` as `_Type` when `_Condition` is true.
* When `_Condition` is false, the primary template has no `type` member,
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
* path:      /inc/djinterp/re_std/type_traits/enable_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_ENABLE_IF_
#define DJINTERP_RESTD_TYPE_TRAITS_ENABLE_IF_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   ENABLE_IF
// =============================================================================

// enable_if
//   trait: SFINAE primitive. Has member typedef `type` only when
// `_Condition` is true.
template<bool     _Condition,
         typename _Type = void>
struct enable_if
{};

// enable_if<true, _Type>
//   trait: specialization for the true case; provides member typedef
// `type` as `_Type`.
template<typename _Type>
struct enable_if<true, _Type>
{
    typedef _Type type;
};


// =============================================================================
// II.  ENABLE_IF_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // enable_if_t
    //   alias: convenience alias for enable_if<_Condition, _Type>::type.
    template<bool     _Condition,
             typename _Type = void>
    using enable_if_t = typename enable_if<_Condition, _Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_ENABLE_IF_
