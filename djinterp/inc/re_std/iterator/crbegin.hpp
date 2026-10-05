/*******************************************************************************
* djinterp [re_std]                                                  crbegin.hpp
*
* crbegin function header:
* crbegin(c) — explicit const reverse iteration. Forces the const
* overload of rbegin() and so always yields a const_reverse_iterator
* (or reverse_iterator<const T*> for arrays).
*
*
* path:      /inc/re_std/iterator/crbegin.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_CRBEGIN_HPP
#define RE_STD_ITERATOR_CRBEGIN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/rbegin.hpp"


namespace re_std
{

template<typename C>
RE_STD_CONSTEXPR auto crbegin(const C& _c) -> decltype(re_std::rbegin(_c))
{
    return re_std::rbegin(_c);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_CRBEGIN_HPP
