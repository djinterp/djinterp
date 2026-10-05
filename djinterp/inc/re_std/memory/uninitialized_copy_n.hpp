/*******************************************************************************
* djinterp [re_std]                                     uninitialized_copy_n.hpp
*
* like uninitialized_copy, but takes a count instead of a sentinel:
*   copies _n elements from _first into uninitialised storage at
*   _d_first, constructing each via copy.
*
* return value:
*   pair<InputIt, ForwardIt> — the input iterator advanced _n
*   positions, and the destination past-the-end iterator. The pair
*   form was added with std::make_pair-style return; re_std uses
*   re_std::pair.
*
* added in std C++11; re_std back-ports to C++98+ where pair is
* available.
*
*
* path:      /inc/re_std/memory/uninitialized_copy_n.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNINITIALIZED_COPY_N_HPP
#define RE_STD_MEMORY_UNINITIALIZED_COPY_N_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std

#if RE_STD_HAS_HEADER_NEW

    // std
    #include <cstddef>
    #include <new>

    #include "re_std/memory/addressof.hpp"
    #include "re_std/memory/destroy_at.hpp"
    #include "re_std/memory/iter_value.hpp"
    #include "re_std/utility/pair.hpp"


namespace re_std
{

template<typename InputIt, typename Size, typename ForwardIt>
pair<InputIt, ForwardIt> uninitialized_copy_n
(
    InputIt    _first,
    Size       _n,
    ForwardIt  _d_first
)
{
    typedef typename internal::iter_value<ForwardIt>::type _T;

    ForwardIt _current = _d_first;

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            for (; _n > 0; ++_first, (void)++_current, --_n)
            {
                ::new (static_cast<void*>(re_std::addressof(*_current)))
                    _T(*_first);
            }
            return pair<InputIt, ForwardIt>(_first, _current);
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
        for (; _n > 0; ++_first, (void)++_current, --_n)
        {
            ::new (static_cast<void*>(re_std::addressof(*_current)))
                _T(*_first);
        }
        return pair<InputIt, ForwardIt>(_first, _current);
    #endif
}


}  // re_std
#endif  // RE_STD_HAS_HEADER_NEW

#endif  // floor, for now


#endif  // RE_STD_MEMORY_UNINITIALIZED_COPY_N_HPP
