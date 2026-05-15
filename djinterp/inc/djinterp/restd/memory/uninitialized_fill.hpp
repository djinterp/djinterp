/***********************************************************************
* restd                                                uninitialized_fill.hpp
*
* fill the uninitialised range [_first, _last) by copy-constructing
* each element from _value.
*
* exception safety: strong. Any thrown ctor leads to all already-
* constructed elements being destroyed.
*
*
* path:      /inc/restd/memory/uninitialized_fill.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_UNINITIALIZED_FILL_
#define RESTD_MEMORY_UNINITIALIZED_FILL_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_NEW

    #include <new>

    #include "restd/memory/addressof.hpp"
    #include "restd/memory/destroy_at.hpp"
    #include "restd/memory/internal/iter_value.hpp"


namespace restd
{

template<typename _ForwardIt, typename _T>
void uninitialized_fill
(
    _ForwardIt   _first,
    _ForwardIt   _last,
    const _T&    _value
)
{
    typedef typename internal::iter_value<_ForwardIt>::type _U;

    _ForwardIt _current = _first;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            for (; _current != _last; ++_current)
            {
                ::new (static_cast<void*>(restd::addressof(*_current)))
                    _U(_value);
            }
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
        for (; _current != _last; ++_current)
        {
            ::new (static_cast<void*>(restd::addressof(*_current)))
                _U(_value);
        }
    #endif
}


}  // namespace restd

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // RESTD_MEMORY_UNINITIALIZED_FILL_
