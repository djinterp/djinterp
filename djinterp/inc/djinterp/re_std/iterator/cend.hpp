/***********************************************************************
* restd                                                               cend.hpp
*
* cend(c) — explicit const-iteration end. Pairs with cbegin(c).
*
*
* path:      /inc/djinterp/re_std/iterator/cend.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_CEND_
#define RESTD_ITERATOR_CEND_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/iterator/end.hpp"


namespace restd
{

template<typename _C>
D_CONSTEXPR auto cend(const _C& _c) -> decltype(restd::end(_c))
{
    return restd::end(_c);
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_CEND_
