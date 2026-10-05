/*******************************************************************************
* djinterp [re_std]                                       uninitialized_move.hpp
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
* path:      /inc/re_std/memory/uninitialized_move.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNINITIALIZED_MOVE_HPP
#define RE_STD_MEMORY_UNINITIALIZED_MOVE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER && RE_STD_HAS_HEADER_NEW

    // std
    #include <new>

    #include "re_std/memory/addressof.hpp"
    #include "re_std/memory/destroy_at.hpp"
    #include "re_std/memory/iter_value.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

template<typename InputIt, typename ForwardIt>
ForwardIt uninitialized_move
(
    InputIt    _first,
    InputIt    _last,
    ForwardIt  _d_first
)
{
    typedef typename internal::iter_value<ForwardIt>::type _T;

    ForwardIt _current = _d_first;

    #if RE_STD_HAS_EXCEPTIONS
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


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER && RE_STD_HAS_HEADER_NEW

#endif  // RE_STD_MEMORY_UNINITIALIZED_MOVE_HPP
