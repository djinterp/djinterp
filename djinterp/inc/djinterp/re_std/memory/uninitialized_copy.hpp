/***********************************************************************
* restd                                                uninitialized_copy.hpp
*
* copy elements from [first, last) into raw uninitialized storage at
* d_first, constructing each destination element via copy-construction.
*
* exception safety:
*   strong guarantee. If any element's copy-construction throws, all
*   previously-constructed destination elements are destroyed before
*   the exception propagates.
*
* preconditions:
*   - [d_first, d_first + (last - first)) refers to UNINITIALISED memory
*     (not constructed objects). Calling this on already-constructed
*     storage leaks those objects.
*   - destination memory has appropriate alignment for the value type.
*
* return value:
*   iterator to the past-the-end position in the destination range.
*
*
* path:      /inc/restd/memory/uninitialized_copy.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_UNINITIALIZED_COPY_
#define RESTD_MEMORY_UNINITIALIZED_COPY_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_NEW

    #include <new>

    #include "restd/memory/addressof.hpp"
    #include "restd/memory/destroy_at.hpp"
    #include "restd/iterator/iterator_traits.hpp"


namespace restd
{

template<typename _InputIt, typename _ForwardIt>
_ForwardIt uninitialized_copy
(
    _InputIt    _first,
    _InputIt    _last,
    _ForwardIt  _d_first
)
{
    typedef typename iterator_traits<_ForwardIt>::value_type _T;

    _ForwardIt _current = _d_first;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            for (; _first != _last; ++_first, (void)++_current)
            {
                ::new (static_cast<void*>(restd::addressof(*_current)))
                    _T(*_first);
            }
            return _current;
        }
        catch (...)
        {
            // Roll back any constructions that already succeeded.
            for (; _d_first != _current; ++_d_first)
            {
                restd::destroy_at(restd::addressof(*_d_first));
            }
            throw;
        }
    #else
        // No exception support: any throw from _T's ctor terminates
        // the program. The loop body is the same.
        for (; _first != _last; ++_first, (void)++_current)
        {
            ::new (static_cast<void*>(restd::addressof(*_current)))
                _T(*_first);
        }
        return _current;
    #endif
}


}  // namespace restd

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // RESTD_MEMORY_UNINITIALIZED_COPY_
