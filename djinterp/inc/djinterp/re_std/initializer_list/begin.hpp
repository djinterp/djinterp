/***********************************************************************
* restd                                                        begin.hpp
*
* non-member begin for initializer_list:
*   returns a pointer to the first element of an initializer_list. std
*   qualifies the non-member begin constexpr only from C++14; restd
*   qualifies it constexpr from C++11, because
*   initializer_list::begin() is itself constexpr in C++11 — a
*   one-tier constexpr back-port. noexcept on every tier.
*
*
* path:      /inc/djinterp/re_std/initializer_list/begin.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_INITIALIZER_LIST_BEGIN_
#define RESTD_INITIALIZER_LIST_BEGIN_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// restd
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

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_INITIALIZER_LIST_BEGIN_
