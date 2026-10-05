/*******************************************************************************
* djinterp [re_std]                                                    begin.hpp
*
* non-member begin for initializer_list:
*   returns a pointer to the first element of an initializer_list. std
*   qualifies the non-member begin constexpr only from C++14; re_std
*   qualifies it constexpr from C++11, because
*   initializer_list::begin() is itself constexpr in C++11 — a
*   one-tier constexpr back-port. noexcept on every tier.
*
*
* path:      /inc/re_std/initializer_list/begin.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_INITIALIZER_LIST_BEGIN_HPP
#define RE_STD_INITIALIZER_LIST_BEGIN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "initializer_list.hpp"

namespace re_std
{

    // begin
    //   function: pointer to the first element of an initializer_list.
    template<typename Type>
    RE_STD_CONSTEXPR const Type*
    begin(
        initializer_list<Type> _il
    ) RE_STD_NOEXCEPT
    {
        return _il.begin();
    }

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_INITIALIZER_LIST_BEGIN_HPP
