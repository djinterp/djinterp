/***********************************************************************
* restd                                                             cbegin.hpp
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
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_CBEGIN_
#define RESTD_ITERATOR_CBEGIN_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/iterator/begin.hpp"


namespace restd
{

template<typename _C>
D_CONSTEXPR auto cbegin(const _C& _c) -> decltype(restd::begin(_c))
{
    return restd::begin(_c);
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_CBEGIN_
