/*******************************************************************************
* djinterp [re_std]                                       uninitialized_copy.hpp
*
* uninitialized_copy function header:
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
* path:      /inc/re_std/memory/uninitialized_copy.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNINITIALIZED_COPY_HPP
#define RE_STD_MEMORY_UNINITIALIZED_COPY_HPP 1

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
    #include "re_std/iterator/iterator_traits.hpp"


namespace re_std
{

template<typename InputIt, typename ForwardIt>
ForwardIt uninitialized_copy
(
    InputIt    _first,
    InputIt    _last,
    ForwardIt  _d_first
)
{
    typedef typename iterator_traits<ForwardIt>::value_type _T;

    ForwardIt _current = _d_first;

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            for (; _first != _last; ++_first, (void)++_current)
            {
                ::new (static_cast<void*>(re_std::addressof(*_current)))
                    _T(*_first);
            }
            return _current;
        }
        catch (...)
        {
            // Roll back any constructions that already succeeded.
            for (; _d_first != _current; ++_d_first)
            {
                re_std::destroy_at(re_std::addressof(*_d_first));
            }
            throw;
        }
    #else
        // No exception support: any throw from _T's ctor terminates
        // the program. The loop body is the same.
        for (; _first != _last; ++_first, (void)++_current)
        {
            ::new (static_cast<void*>(re_std::addressof(*_current)))
                _T(*_first);
        }
        return _current;
    #endif
}


}  // re_std
#endif  // RE_STD_HAS_HEADER_NEW

#endif  // floor, for now


#endif  // RE_STD_MEMORY_UNINITIALIZED_COPY_HPP
