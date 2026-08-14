/***********************************************************************
* restd                                                          end.hpp
*
* non-member end for initializer_list:
*   returns a pointer one past the last element of an initializer_list.
*   Like begin, std makes the non-member end constexpr only from C++14;
*   restd qualifies it constexpr from C++11 (initializer_list::end() is
*   constexpr in C++11) — a one-tier constexpr back-port. noexcept on
*   every tier.
*
*
* path:      /inc/djinterp/re_std/initializer_list/end.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_INITIALIZER_LIST_END_
#define RESTD_INITIALIZER_LIST_END_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// restd
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

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_INITIALIZER_LIST_END_
