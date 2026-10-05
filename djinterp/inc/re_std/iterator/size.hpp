/*******************************************************************************
* djinterp [re_std]                                                     size.hpp
*
* size function header:
* size(c) returns c.size() for containers, or the extent N for raw
* arrays of size N. The array overload returns std::size_t (the
* signed C++20 ssize variant is a separate symbol, ssize.hpp, not
* yet implemented).
*
* added in std C++17.
*
*
* path:      /inc/re_std/iterator/size.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_SIZE_HPP
#define RE_STD_ITERATOR_SIZE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>


namespace re_std
{

template<typename C>
RE_STD_CONSTEXPR auto size(const C& _c) -> decltype(_c.size())
{
    return _c.size();
}

template<typename T, std::size_t N>
RE_STD_CONSTEXPR std::size_t size(const T (&)[N]) RE_STD_NOEXCEPT
{
    return N;
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_SIZE_HPP
