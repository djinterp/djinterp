/***********************************************************************
* re_std                                                             crend.hpp
*
* crend(c) — explicit const reverse iteration end. Pairs with crbegin.
*
*
* path:      /inc/djinterp/re_std/iterator/crend.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_CREND_
#define DJINTERP_RE_STD_ITERATOR_CREND_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/rend.hpp"


namespace re_std
{

template<typename _C>
D_CONSTEXPR auto crend(const _C& _c) -> decltype(re_std::rend(_c))
{
    return re_std::rend(_c);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_CREND_
