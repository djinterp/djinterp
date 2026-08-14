/***********************************************************************
* restd                                                               data.hpp
*
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
* path:      /inc/djinterp/re_std/iterator/data.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_DATA_
#define RESTD_ITERATOR_DATA_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <initializer_list>


namespace restd
{

template<typename _C>
D_CONSTEXPR auto data(_C& _c) -> decltype(_c.data())
{
    return _c.data();
}

template<typename _C>
D_CONSTEXPR auto data(const _C& _c) -> decltype(_c.data())
{
    return _c.data();
}

template<typename _T, std::size_t _N>
D_CONSTEXPR _T* data(_T (&_arr)[_N]) D_NOEXCEPT
{
    return _arr;
}

template<typename _E>
D_CONSTEXPR const _E* data(std::initializer_list<_E> _il) D_NOEXCEPT
{
    return _il.begin();
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_DATA_
