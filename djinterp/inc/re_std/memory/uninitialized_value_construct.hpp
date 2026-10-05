/*******************************************************************************
* djinterp [re_std]                            uninitialized_value_construct.hpp
*
* uninitialized_value_construct function header:
* value-initialise each element in [_first, _last) — i.e. construct
* each as if by:  ::new (p) _T();      (with parens)
*
* value-initialisation:
*   - for class types with a user-provided default ctor: runs it
*   - for class types without one: zero-initialises non-static data
*     members and bases, then runs the implicit default ctor
*   - for trivial types: zero-initialises (this is the key difference
*     from default-init)
*
* see uninitialized_default_construct.hpp for the contrast.
*
*
* path:      /inc/re_std/memory/uninitialized_value_construct.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNINITIALIZED_VALUE_CONSTRUCT_HPP
#define RE_STD_MEMORY_UNINITIALIZED_VALUE_CONSTRUCT_HPP 1

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

template<typename ForwardIt>
void uninitialized_value_construct
(
    ForwardIt   _first,
    ForwardIt   _last
)
{
    typedef typename internal::iter_value<ForwardIt>::type _T;

    ForwardIt _current = _first;

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            for (; _current != _last; ++_current)
            {
                // Parens after _T: value-initialisation.
                ::new (static_cast<void*>(re_std::addressof(*_current))) _T();
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
            ::new (static_cast<void*>(re_std::addressof(*_current))) _T();
        }
    #endif
}


}  // re_std
#endif  // RE_STD_HAS_HEADER_NEW

#endif  // floor, for now


#endif  // RE_STD_MEMORY_UNINITIALIZED_VALUE_CONSTRUCT_HPP
