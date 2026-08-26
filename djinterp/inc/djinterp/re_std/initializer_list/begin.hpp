/***********************************************************************
* re_std                                                       begin.hpp
*
* non-member begin for initializer_list:
*   returns a pointer to the first element of an initializer_list. std
*   qualifies the non-member begin constexpr only from C++14; re_std
*   qualifies it constexpr from C++11, because
*   initializer_list::begin() is itself constexpr in C++11 — a
*   one-tier constexpr back-port. noexcept on every tier.
*
*
* path:      /inc/djinterp/re_std/initializer_list/begin.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef DJINTERP_RE_STD_INITIALIZER_LIST_BEGIN_
#define DJINTERP_RE_STD_INITIALIZER_LIST_BEGIN_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "initializer_list.hpp"

NS_RESTD

    // begin
    //   function: pointer to the first element of an initializer_list.
    template<typename _Type>
    D_CONSTEXPR const _Type*
    begin(
        initializer_list<_Type> _il
    ) D_NOEXCEPT
    {
        return _il.begin();
    }

NS_END  // re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_INITIALIZER_LIST_BEGIN_
