/***********************************************************************
* re_std                                                         end.hpp
*
* non-member end for initializer_list:
*   returns a pointer one past the last element of an initializer_list.
*   Like begin, std makes the non-member end constexpr only from C++14;
*   re_std qualifies it constexpr from C++11 (initializer_list::end() is
*   constexpr in C++11) — a one-tier constexpr back-port. noexcept on
*   every tier.
*
*
* path:      /inc/djinterp/re_std/initializer_list/end.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef DJINTERP_RE_STD_INITIALIZER_LIST_END_
#define DJINTERP_RE_STD_INITIALIZER_LIST_END_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "initializer_list.hpp"

NS_RESTD

    // end
    //   function: pointer one past the last element of an initializer_list.
    template<typename _Type>
    D_CONSTEXPR const _Type*
    end(
        initializer_list<_Type> _il
    ) D_NOEXCEPT
    {
        return _il.end();
    }

NS_END  // re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_INITIALIZER_LIST_END_
