/*******************************************************************************
* djinterp [re_std]                                                    crend.hpp
*
* crend function header:
* crend(c) — explicit const reverse iteration end. Pairs with crbegin.
*
*
* path:      /inc/re_std/iterator/crend.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_CREND_HPP
#define RE_STD_ITERATOR_CREND_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/rend.hpp"


namespace re_std
{

template<typename C>
RE_STD_CONSTEXPR auto crend(const C& _c) -> decltype(re_std::rend(_c))
{
    return re_std::rend(_c);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_CREND_HPP
