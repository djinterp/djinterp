/***********************************************************************
* re_std                                                           crbegin.hpp
*
* crbegin(c) — explicit const reverse iteration. Forces the const
* overload of rbegin() and so always yields a const_reverse_iterator
* (or reverse_iterator<const T*> for arrays).
*
*
* path:      /inc/djinterp/re_std/iterator/crbegin.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_CRBEGIN_
#define DJINTERP_RE_STD_ITERATOR_CRBEGIN_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/rbegin.hpp"


namespace re_std
{

template<typename _C>
D_CONSTEXPR auto crbegin(const _C& _c) -> decltype(re_std::rbegin(_c))
{
    return re_std::rbegin(_c);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_CRBEGIN_
