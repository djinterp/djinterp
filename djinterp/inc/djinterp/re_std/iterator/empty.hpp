/***********************************************************************
* re_std                                                             empty.hpp
*
* empty(c) returns true iff the container is empty. For containers
* with a member empty(), forwards. For raw arrays, always false (a
* zero-extent array is ill-formed in standard C++). For
* initializer_list, uses .size() == 0.
*
* added in std C++17.
*
*
* path:      /inc/djinterp/re_std/iterator/empty.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_EMPTY_
#define DJINTERP_RE_STD_ITERATOR_EMPTY_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <initializer_list>


namespace re_std
{

template<typename _C>
D_CONSTEXPR auto empty(const _C& _c) -> decltype(_c.empty())
{
    return _c.empty();
}

// Raw arrays are never empty (zero-extent is ill-formed).
template<typename _T, std::size_t _N>
D_CONSTEXPR bool empty(const _T (&)[_N]) D_NOEXCEPT
{
    return false;
}

template<typename _E>
D_CONSTEXPR bool empty(std::initializer_list<_E> _il) D_NOEXCEPT
{
    return _il.size() == 0;
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_EMPTY_
