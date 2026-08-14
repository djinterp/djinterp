/***********************************************************************
* restd                                                             rbegin.hpp
*
* rbegin(c) returns an iterator to the last element, traversing in
* reverse. For containers with member rbegin(), forwards. For raw
* arrays and initializer_list, wraps end()/il.end() in a
* reverse_iterator.
*
* added in std C++14.
*
*
* path:      /inc/djinterp/re_std/iterator/rbegin.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_RBEGIN_
#define RESTD_ITERATOR_RBEGIN_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <initializer_list>

    #include "restd/iterator/reverse_iterator.hpp"


namespace restd
{

template<typename _C>
D_CONSTEXPR auto rbegin(_C& _c) -> decltype(_c.rbegin())
{
    return _c.rbegin();
}

template<typename _C>
D_CONSTEXPR auto rbegin(const _C& _c) -> decltype(_c.rbegin())
{
    return _c.rbegin();
}

template<typename _T, std::size_t _N>
D_CONSTEXPR reverse_iterator<_T*> rbegin(_T (&_arr)[_N])
{
    return reverse_iterator<_T*>(_arr + _N);
}

template<typename _E>
D_CONSTEXPR reverse_iterator<const _E*> rbegin(std::initializer_list<_E> _il)
{
    return reverse_iterator<const _E*>(_il.end());
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_RBEGIN_
