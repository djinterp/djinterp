/***********************************************************************
* restd                                              uninitialized_fill_n.hpp
*
* fill the uninitialised range [_first, _first + _n) by copy-constructing
* each element from _value.
*
* return value:
*   the past-the-end iterator (_first advanced _n positions). Std
*   added the return value in C++11; the C++98 form returned void.
*   restd matches the C++11 signature on every tier — code that wants
*   to discard the return value can do so explicitly.
*
*
* path:      /inc/djinterp/re_std/memory/uninitialized_fill_n.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_UNINITIALIZED_FILL_N_
#define RESTD_MEMORY_UNINITIALIZED_FILL_N_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_NEW

    #include <new>

    #include "restd/memory/addressof.hpp"
    #include "restd/memory/destroy_at.hpp"
    #include "restd/memory/internal/iter_value.hpp"


namespace restd
{

template<typename _ForwardIt, typename _Size, typename _T>
_ForwardIt uninitialized_fill_n
(
    _ForwardIt   _first,
    _Size        _n,
    const _T&    _value
)
{
    typedef typename internal::iter_value<_ForwardIt>::type _U;

    _ForwardIt _current = _first;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            for (; _n > 0; ++_current, --_n)
            {
                ::new (static_cast<void*>(restd::addressof(*_current)))
                    _U(_value);
            }
            return _current;
        }
        catch (...)
        {
            for (; _first != _current; ++_first)
            {
                restd::destroy_at(restd::addressof(*_first));
            }
            throw;
        }
    #else
        for (; _n > 0; ++_current, --_n)
        {
            ::new (static_cast<void*>(restd::addressof(*_current)))
                _U(_value);
        }
        return _current;
    #endif
}


}  // namespace restd

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // RESTD_MEMORY_UNINITIALIZED_FILL_N_
