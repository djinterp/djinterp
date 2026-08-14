/***********************************************************************
* restd                                                uninitialized_move.hpp
*
* move elements from [_first, _last) into uninitialised storage at
* _d_first, constructing each destination via move-construction:
*   ::new (p) _T(restd::move(*src))
*
* exception safety:
*   strong w.r.t. the destination range — any throw destroys all
*   destination elements already constructed. NOTE: per the standard,
*   any source elements that were moved-from BEFORE the throw remain
*   moved-from. Recovery is the caller's responsibility.
*
* added in std C++17; restd back-ports to C++11+ (move semantics
* required).
*
*
* path:      /inc/djinterp/re_std/memory/uninitialized_move.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_UNINITIALIZED_MOVE_
#define RESTD_MEMORY_UNINITIALIZED_MOVE_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER && D_ENV_CPP98_HAS_NEW

    #include <new>

    #include "restd/memory/addressof.hpp"
    #include "restd/memory/destroy_at.hpp"
    #include "restd/memory/internal/iter_value.hpp"
    #include "restd/utility/move.hpp"


namespace restd
{

template<typename _InputIt, typename _ForwardIt>
_ForwardIt uninitialized_move
(
    _InputIt    _first,
    _InputIt    _last,
    _ForwardIt  _d_first
)
{
    typedef typename internal::iter_value<_ForwardIt>::type _T;

    _ForwardIt _current = _d_first;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            for (; _first != _last; ++_first, (void)++_current)
            {
                ::new (static_cast<void*>(restd::addressof(*_current)))
                    _T(restd::move(*_first));
            }
            return _current;
        }
        catch (...)
        {
            for (; _d_first != _current; ++_d_first)
            {
                restd::destroy_at(restd::addressof(*_d_first));
            }
            throw;
        }
    #else
        for (; _first != _last; ++_first, (void)++_current)
        {
            ::new (static_cast<void*>(restd::addressof(*_current)))
                _T(restd::move(*_first));
        }
        return _current;
    #endif
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER && D_ENV_CPP98_HAS_NEW

#endif  // RESTD_MEMORY_UNINITIALIZED_MOVE_
