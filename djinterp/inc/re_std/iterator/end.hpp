/*******************************************************************************
* djinterp [re_std]                                                      end.hpp
*
* end function header:
* free-function end(container) — see begin.hpp for design notes.
*
*
* path:      /inc/re_std/iterator/end.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ITERATOR_END_HPP
#define RE_STD_ITERATOR_END_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>                      // size_t
    // re_std
    #include "../initializer_list/end.hpp"  // end(initializer_list<E>)


namespace re_std
{

template<typename C>
RE_STD_CONSTEXPR auto end(C& _c) -> decltype(_c.end())
{
    return _c.end();
}

template<typename C>
RE_STD_CONSTEXPR auto end(const C& _c) -> decltype(_c.end())
{
    return _c.end();
}

template<typename T, std::size_t N>
RE_STD_CONSTEXPR T* end(T (&_arr)[N]) RE_STD_NOEXCEPT
{
    return _arr + N;
}

// initializer_list: defined once, in initializer_list/end.hpp, where std
// declares it (<initializer_list>), and included above.


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_END_HPP
