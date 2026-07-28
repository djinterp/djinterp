/***********************************************************************
* restd                                                                end.hpp
*
* free-function end(container) — see begin.hpp for design notes.
*
*
* path:      /inc/restd/iterator/end.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_END_
#define RESTD_ITERATOR_END_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <initializer_list>


namespace restd
{

template<typename _C>
D_CONSTEXPR auto end(_C& _c) -> decltype(_c.end())
{
    return _c.end();
}

template<typename _C>
D_CONSTEXPR auto end(const _C& _c) -> decltype(_c.end())
{
    return _c.end();
}

template<typename _T, std::size_t _N>
D_CONSTEXPR _T* end(_T (&_arr)[_N]) D_NOEXCEPT
{
    return _arr + _N;
}

template<typename _E>
D_CONSTEXPR const _E* end(std::initializer_list<_E> _il) D_NOEXCEPT
{
    return _il.end();
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_END_
