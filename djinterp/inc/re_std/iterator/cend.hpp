/*******************************************************************************
* djinterp [re_std]                                                     cend.hpp
*
* cend function header:
* cend(c) — explicit const-iteration end. Pairs with cbegin(c).
*
*
* path:      /inc/re_std/iterator/cend.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_CEND_HPP
#define RE_STD_ITERATOR_CEND_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/end.hpp"


namespace re_std
{

template<typename C>
RE_STD_CONSTEXPR auto cend(const C& _c) -> decltype(re_std::end(_c))
{
    return re_std::end(_c);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_CEND_HPP
