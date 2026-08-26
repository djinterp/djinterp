/***********************************************************************
* re_std                                                              rend.hpp
*
* rend(c) — reverse-iteration end. Pairs with rbegin(c).
*
*
* path:      /inc/djinterp/re_std/iterator/rend.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_REND_
#define DJINTERP_RE_STD_ITERATOR_REND_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <initializer_list>

    #include "re_std/iterator/reverse_iterator.hpp"


namespace re_std
{

template<typename _C>
D_CONSTEXPR auto rend(_C& _c) -> decltype(_c.rend())
{
    return _c.rend();
}

template<typename _C>
D_CONSTEXPR auto rend(const _C& _c) -> decltype(_c.rend())
{
    return _c.rend();
}

template<typename _T, std::size_t _N>
D_CONSTEXPR reverse_iterator<_T*> rend(_T (&_arr)[_N])
{
    return reverse_iterator<_T*>(_arr);
}

template<typename _E>
D_CONSTEXPR reverse_iterator<const _E*> rend(std::initializer_list<_E> _il)
{
    return reverse_iterator<const _E*>(_il.begin());
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_REND_
