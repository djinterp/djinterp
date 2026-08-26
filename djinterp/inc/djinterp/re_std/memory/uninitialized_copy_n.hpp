/***********************************************************************
* re_std                                             uninitialized_copy_n.hpp
*
* like uninitialized_copy, but takes a count instead of a sentinel:
*   copies _n elements from _first into uninitialised storage at
*   _d_first, constructing each via copy.
*
* return value:
*   pair<_InputIt, _ForwardIt> — the input iterator advanced _n
*   positions, and the destination past-the-end iterator. The pair
*   form was added with std::make_pair-style return; re_std uses
*   re_std::pair.
*
* added in std C++11; re_std back-ports to C++98+ where pair is
* available.
*
*
* path:      /inc/djinterp/re_std/memory/uninitialized_copy_n.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_UNINITIALIZED_COPY_N_
#define DJINTERP_RE_STD_MEMORY_UNINITIALIZED_COPY_N_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_NEW

    #include <cstddef>
    #include <new>

    #include "re_std/memory/addressof.hpp"
    #include "re_std/memory/destroy_at.hpp"
    #include "re_std/memory/iter_value.hpp"
    #include "re_std/utility/pair.hpp"


namespace re_std
{

template<typename _InputIt, typename _Size, typename _ForwardIt>
pair<_InputIt, _ForwardIt> uninitialized_copy_n
(
    _InputIt    _first,
    _Size       _n,
    _ForwardIt  _d_first
)
{
    typedef typename internal::iter_value<_ForwardIt>::type _T;

    _ForwardIt _current = _d_first;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            for (; _n > 0; ++_first, (void)++_current, --_n)
            {
                ::new (static_cast<void*>(re_std::addressof(*_current)))
                    _T(*_first);
            }
            return pair<_InputIt, _ForwardIt>(_first, _current);
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
        return pair<_InputIt, _ForwardIt>(_first, _current);
    #endif
}


}  // namespace re_std

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // DJINTERP_RE_STD_MEMORY_UNINITIALIZED_COPY_N_
