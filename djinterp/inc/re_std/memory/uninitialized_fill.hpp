/*******************************************************************************
* djinterp [re_std]                                       uninitialized_fill.hpp
*
* uninitialized_fill function header:
* fill the uninitialised range [_first, _last) by copy-constructing
* each element from _value.
*
* exception safety: strong. Any thrown ctor leads to all already-
* constructed elements being destroyed.
*
*
* path:      /inc/re_std/memory/uninitialized_fill.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNINITIALIZED_FILL_HPP
#define RE_STD_MEMORY_UNINITIALIZED_FILL_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std

#if RE_STD_HAS_HEADER_NEW

    // std
    #include <new>

    #include "re_std/memory/addressof.hpp"
    #include "re_std/memory/destroy_at.hpp"
    #include "re_std/memory/iter_value.hpp"


namespace re_std
{

template<typename ForwardIt, typename T>
void uninitialized_fill
(
    ForwardIt   _first,
    ForwardIt   _last,
    const T&    _value
)
{
    typedef typename internal::iter_value<ForwardIt>::type _U;

    ForwardIt _current = _first;

    #if RE_STD_HAS_EXCEPTIONS
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


}  // re_std
#endif  // RE_STD_HAS_HEADER_NEW

#endif  // floor, for now


#endif  // RE_STD_MEMORY_UNINITIALIZED_FILL_HPP
