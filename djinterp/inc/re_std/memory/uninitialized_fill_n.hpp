/*******************************************************************************
* djinterp [re_std]                                     uninitialized_fill_n.hpp
*
* uninitialized_fill_n function header:
* fill the uninitialised range [_first, _first + _n) by copy-constructing
* each element from _value.
*
* return value:
*   the past-the-end iterator (_first advanced _n positions). Std
*   added the return value in C++11; the C++98 form returned void.
*   re_std matches the C++11 signature on every tier — code that wants
*   to discard the return value can do so explicitly.
*
*
* path:      /inc/re_std/memory/uninitialized_fill_n.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNINITIALIZED_FILL_N_HPP
#define RE_STD_MEMORY_UNINITIALIZED_FILL_N_HPP 1

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

template<typename ForwardIt, typename Size, typename T>
ForwardIt uninitialized_fill_n
(
    ForwardIt   _first,
    Size        _n,
    const T&    _value
)
{
    typedef typename internal::iter_value<ForwardIt>::type _U;

    ForwardIt _current = _first;

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            for (; _n > 0; ++_current, --_n)
            {
                ::new (static_cast<void*>(re_std::addressof(*_current)))
                    _U(_value);
            }
            return _current;
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
        for (; _n > 0; ++_current, --_n)
        {
            ::new (static_cast<void*>(re_std::addressof(*_current)))
                _U(_value);
        }
        return _current;
    #endif
}


}  // re_std
#endif  // RE_STD_HAS_HEADER_NEW

#endif  // floor, for now


#endif  // RE_STD_MEMORY_UNINITIALIZED_FILL_N_HPP
