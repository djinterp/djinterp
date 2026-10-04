/***********************************************************************
* re_std                                                              size.hpp
*
* size(c) returns c.size() for containers, or the extent N for raw
* arrays of size N. The array overload returns std::size_t (the
* signed C++20 ssize variant is a separate symbol, ssize.hpp, not
* yet implemented).
*
* added in std C++17.
*
*
* path:      /inc/djinterp/re_std/iterator/size.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_SIZE_
#define DJINTERP_RE_STD_ITERATOR_SIZE_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>


namespace re_std
{

template<typename _C>
D_CONSTEXPR auto size(const _C& _c) -> decltype(_c.size())
{
    return _c.size();
}

template<typename _T, std::size_t _N>
D_CONSTEXPR std::size_t size(const _T (&)[_N]) D_NOEXCEPT
{
    return _N;
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_SIZE_
