/***********************************************************************
* restd                                                              crend.hpp
*
* crend(c) — explicit const reverse iteration end. Pairs with crbegin.
*
*
* path:      /inc/djinterp/re_std/iterator/crend.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_CREND_
#define RESTD_ITERATOR_CREND_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/iterator/rend.hpp"


namespace restd
{

template<typename _C>
D_CONSTEXPR auto crend(const _C& _c) -> decltype(restd::rend(_c))
{
    return restd::rend(_c);
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_CREND_
