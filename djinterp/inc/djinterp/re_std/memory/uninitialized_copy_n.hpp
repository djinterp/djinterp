/***********************************************************************
* restd                                              uninitialized_copy_n.hpp
*
* like uninitialized_copy, but takes a count instead of a sentinel:
*   copies _n elements from _first into uninitialised storage at
*   _d_first, constructing each via copy.
*
* return value:
*   pair<_InputIt, _ForwardIt> — the input iterator advanced _n
*   positions, and the destination past-the-end iterator. The pair
*   form was added with std::make_pair-style return; restd uses
*   restd::pair.
*
* added in std C++11; restd back-ports to C++98+ where pair is
* available.
*
*
* path:      /inc/restd/memory/uninitialized_copy_n.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_UNINITIALIZED_COPY_N_
#define RESTD_MEMORY_UNINITIALIZED_COPY_N_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_NEW

    #include <cstddef>
    #include <new>

    #include "restd/memory/addressof.hpp"
    #include "restd/memory/destroy_at.hpp"
    #include "restd/memory/internal/iter_value.hpp"
    #include "restd/utility/pair.hpp"


namespace restd
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
                ::new (static_cast<void*>(restd::addressof(*_current)))
                    _T(*_first);
            }
            return pair<_InputIt, _ForwardIt>(_first, _current);
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
        for (; _n > 0; ++_first, (void)++_current, --_n)
        {
            ::new (static_cast<void*>(restd::addressof(*_current)))
                _T(*_first);
        }
        return pair<_InputIt, _ForwardIt>(_first, _current);
    #endif
}


}  // namespace restd

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // RESTD_MEMORY_UNINITIALIZED_COPY_N_
