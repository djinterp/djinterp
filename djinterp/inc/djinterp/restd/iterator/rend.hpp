/***********************************************************************
* restd                                                               rend.hpp
*
* rend(c) — reverse-iteration end. Pairs with rbegin(c).
*
*
* path:      /inc/restd/iterator/rend.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_REND_
#define RESTD_ITERATOR_REND_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <initializer_list>

    #include "restd/iterator/reverse_iterator.hpp"


namespace restd
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


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_REND_
