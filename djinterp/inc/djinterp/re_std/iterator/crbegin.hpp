/***********************************************************************
* restd                                                            crbegin.hpp
*
* crbegin(c) — explicit const reverse iteration. Forces the const
* overload of rbegin() and so always yields a const_reverse_iterator
* (or reverse_iterator<const T*> for arrays).
*
*
* path:      /inc/restd/iterator/crbegin.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_CRBEGIN_
#define RESTD_ITERATOR_CRBEGIN_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/iterator/rbegin.hpp"


namespace restd
{

template<typename _C>
D_CONSTEXPR auto crbegin(const _C& _c) -> decltype(restd::rbegin(_c))
{
    return restd::rbegin(_c);
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_CRBEGIN_
