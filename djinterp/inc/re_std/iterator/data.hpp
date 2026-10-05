/*******************************************************************************
* djinterp [re_std]                                                     data.hpp
*
* data function header:
* data(c) returns a pointer to the contiguous storage backing the
* container. For containers with member data(), forwards. For raw
* arrays, returns &arr[0]. For initializer_list, returns il.begin().
*
* added in std C++17. Requires the container to be contiguously
* stored — the standard does not enforce this at the type-system
* level, but calling data() on a non-contiguous container yields
* a pointer that does not generalise to the next element via ++.
*
*
* path:      /inc/re_std/iterator/data.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_DATA_HPP
#define RE_STD_ITERATOR_DATA_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>
    #include <initializer_list>


namespace re_std
{

template<typename C>
RE_STD_CONSTEXPR auto data(C& _c) -> decltype(_c.data())
{
    return _c.data();
}

template<typename C>
RE_STD_CONSTEXPR auto data(const C& _c) -> decltype(_c.data())
{
    return _c.data();
}

template<typename T, std::size_t N>
RE_STD_CONSTEXPR T* data(T (&_arr)[N]) RE_STD_NOEXCEPT
{
    return _arr;
}

template<typename E>
RE_STD_CONSTEXPR const E* data(std::initializer_list<E> _il) RE_STD_NOEXCEPT
{
    return _il.begin();
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_DATA_HPP
