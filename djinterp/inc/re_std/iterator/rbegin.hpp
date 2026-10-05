/*******************************************************************************
* djinterp [re_std]                                                   rbegin.hpp
*
* rbegin function header:
* rbegin(c) returns an iterator to the last element, traversing in
* reverse. For containers with member rbegin(), forwards. For raw
* arrays and initializer_list, wraps end()/il.end() in a
* reverse_iterator.
*
* added in std C++14.
*
*
* path:      /inc/re_std/iterator/rbegin.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_RBEGIN_HPP
#define RE_STD_ITERATOR_RBEGIN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>
    #include <initializer_list>

    #include "re_std/iterator/reverse_iterator.hpp"


namespace re_std
{

template<typename C>
RE_STD_CONSTEXPR auto rbegin(C& _c) -> decltype(_c.rbegin())
{
    return _c.rbegin();
}

template<typename C>
RE_STD_CONSTEXPR auto rbegin(const C& _c) -> decltype(_c.rbegin())
{
    return _c.rbegin();
}

template<typename T, std::size_t N>
RE_STD_CONSTEXPR reverse_iterator<T*> rbegin(T (&_arr)[N])
{
    return reverse_iterator<T*>(_arr + N);
}

template<typename E>
RE_STD_CONSTEXPR reverse_iterator<const E*> rbegin(std::initializer_list<E> _il)
{
    return reverse_iterator<const E*>(_il.end());
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_RBEGIN_HPP
