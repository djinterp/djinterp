/***********************************************************************
* re_std                                               uninitialized_fill.hpp
*
* fill the uninitialised range [_first, _last) by copy-constructing
* each element from _value.
*
* exception safety: strong. Any thrown ctor leads to all already-
* constructed elements being destroyed.
*
*
* path:      /inc/djinterp/re_std/memory/uninitialized_fill.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_UNINITIALIZED_FILL_
#define DJINTERP_RE_STD_MEMORY_UNINITIALIZED_FILL_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_NEW

    #include <new>

    #include "re_std/memory/addressof.hpp"
    #include "re_std/memory/destroy_at.hpp"
    #include "re_std/memory/iter_value.hpp"


namespace re_std
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
                ::new (static_cast<void*>(re_std::addressof(*_current)))
                    _U(_value);
            }
        }
        catch (...)
        {
            for (; _first != _current; ++_first)
            {
                re_std::destroy_at(re_std::addressof(*_first));
            }
            throw;
        }
    #else
        for (; _current != _last; ++_current)
        {
            ::new (static_cast<void*>(re_std::addressof(*_current)))
                _U(_value);
        }
    #endif
}


}  // namespace re_std

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // DJINTERP_RE_STD_MEMORY_UNINITIALIZED_FILL_
