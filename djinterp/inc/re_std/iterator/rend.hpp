/*******************************************************************************
* djinterp [re_std]                                                     rend.hpp
*
* rend function header:
* rend(c) — reverse-iteration end. Pairs with rbegin(c).
*
*
* path:      /inc/re_std/iterator/rend.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_REND_HPP
#define RE_STD_ITERATOR_REND_HPP 1

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
RE_STD_CONSTEXPR auto rend(C& _c) -> decltype(_c.rend())
{
    return _c.rend();
}

template<typename C>
RE_STD_CONSTEXPR auto rend(const C& _c) -> decltype(_c.rend())
{
    return _c.rend();
}

template<typename T, std::size_t N>
RE_STD_CONSTEXPR reverse_iterator<T*> rend(T (&_arr)[N])
{
    return reverse_iterator<T*>(_arr);
}

template<typename E>
RE_STD_CONSTEXPR reverse_iterator<const E*> rend(std::initializer_list<E> _il)
{
    return reverse_iterator<const E*>(_il.begin());
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_REND_HPP
