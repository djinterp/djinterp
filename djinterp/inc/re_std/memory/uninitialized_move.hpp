/***********************************************************************
* re_std                                               uninitialized_move.hpp
*
* move elements from [_first, _last) into uninitialised storage at
* _d_first, constructing each destination via move-construction:
*   ::new (p) _T(re_std::move(*src))
*
* exception safety:
*   strong w.r.t. the destination range — any throw destroys all
*   destination elements already constructed. NOTE: per the standard,
*   any source elements that were moved-from BEFORE the throw remain
*   moved-from. Recovery is the caller's responsibility.
*
* added in std C++17; re_std back-ports to C++11+ (move semantics
* required).
*
*
* path:      /inc/djinterp/re_std/memory/uninitialized_move.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_UNINITIALIZED_MOVE_
#define DJINTERP_RE_STD_MEMORY_UNINITIALIZED_MOVE_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER && D_ENV_CPP98_HAS_NEW

    #include <new>

    #include "re_std/memory/addressof.hpp"
    #include "re_std/memory/destroy_at.hpp"
    #include "re_std/memory/iter_value.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
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
                ::new (static_cast<void*>(re_std::addressof(*_current)))
                    _T(re_std::move(*_first));
            }
            return _current;
        }
        catch (...)
        {
            for (; _d_first != _current; ++_d_first)
            {
                re_std::destroy_at(re_std::addressof(*_d_first));
            }
            throw;
        }
    #else
        for (; _first != _last; ++_first, (void)++_current)
        {
            ::new (static_cast<void*>(re_std::addressof(*_current)))
                _T(re_std::move(*_first));
        }
        return _current;
    #endif
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER && D_ENV_CPP98_HAS_NEW

#endif  // DJINTERP_RE_STD_MEMORY_UNINITIALIZED_MOVE_
