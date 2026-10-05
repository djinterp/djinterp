/*******************************************************************************
* djinterp [re_std]                                                      end.hpp
*
* non-member end for initializer_list:
*   returns a pointer one past the last element of an initializer_list.
*   Like begin, std makes the non-member end constexpr only from C++14;
*   re_std qualifies it constexpr from C++11 (initializer_list::end() is
*   constexpr in C++11) — a one-tier constexpr back-port. noexcept on
*   every tier.
*
*
* path:      /inc/re_std/initializer_list/end.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_INITIALIZER_LIST_END_HPP
#define RE_STD_INITIALIZER_LIST_END_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "initializer_list.hpp"

namespace re_std
{

    // end
    //   function: pointer one past the last element of an initializer_list.
    template<typename Type>
    RE_STD_CONSTEXPR const Type*
    end(
        initializer_list<Type> _il
    ) RE_STD_NOEXCEPT
    {
        return _il.end();
    }

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_INITIALIZER_LIST_END_HPP
