/***********************************************************************
* re_std                                                            cbegin.hpp
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
* path:      /inc/djinterp/re_std/iterator/cbegin.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_CBEGIN_
#define DJINTERP_RE_STD_ITERATOR_CBEGIN_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/begin.hpp"


namespace re_std
{

template<typename _C>
D_CONSTEXPR auto cbegin(const _C& _c) -> decltype(re_std::begin(_c))
{
    return re_std::begin(_c);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_CBEGIN_
