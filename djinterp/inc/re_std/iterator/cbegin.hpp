/*******************************************************************************
* djinterp [re_std]                                                   cbegin.hpp
*
* cbegin(c) — explicit const-iteration access. Conceptually:
*
*   cbegin(c)   ===   begin(static_cast<const C&>(c))
*
* this means cbegin(c) returns the const_iterator (or const T* for
* arrays) regardless of whether c itself is const.
*
* added in std C++14.
*
*
* path:      /inc/re_std/iterator/cbegin.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_CBEGIN_HPP
#define RE_STD_ITERATOR_CBEGIN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/begin.hpp"


namespace re_std
{

template<typename C>
RE_STD_CONSTEXPR auto cbegin(const C& _c) -> decltype(re_std::begin(_c))
{
    return re_std::begin(_c);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_CBEGIN_HPP
