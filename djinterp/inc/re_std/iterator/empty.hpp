/*******************************************************************************
* djinterp [re_std]                                                    empty.hpp
*
* empty function header:
* empty(c) returns true iff the container is empty. For containers
* with a member empty(), forwards. For raw arrays, always false (a
* zero-extent array is ill-formed in standard C++). For
* initializer_list, uses .size() == 0.
*
* added in std C++17.
*
*
* path:      /inc/re_std/iterator/empty.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_EMPTY_HPP
#define RE_STD_ITERATOR_EMPTY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>
    #include <initializer_list>


namespace re_std
{

template<typename C>
RE_STD_CONSTEXPR auto empty(const C& _c) -> decltype(_c.empty())
{
    return _c.empty();
}

// Raw arrays are never empty (zero-extent is ill-formed).
template<typename T, std::size_t N>
RE_STD_CONSTEXPR bool empty(const T (&)[N]) RE_STD_NOEXCEPT
{
    return false;
}

template<typename E>
RE_STD_CONSTEXPR bool empty(std::initializer_list<E> _il) RE_STD_NOEXCEPT
{
    return _il.size() == 0;
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_EMPTY_HPP
