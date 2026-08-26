/***********************************************************************
* re_std                                                              cend.hpp
*
* cend(c) — explicit const-iteration end. Pairs with cbegin(c).
*
*
* path:      /inc/djinterp/re_std/iterator/cend.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_CEND_
#define DJINTERP_RE_STD_ITERATOR_CEND_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/end.hpp"


namespace re_std
{

template<typename _C>
D_CONSTEXPR auto cend(const _C& _c) -> decltype(re_std::end(_c))
{
    return re_std::end(_c);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_CEND_
