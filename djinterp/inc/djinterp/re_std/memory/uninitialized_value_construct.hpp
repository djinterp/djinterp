/***********************************************************************
* restd                                    uninitialized_value_construct.hpp
*
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
* path:      /inc/restd/memory/uninitialized_value_construct.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_UNINITIALIZED_VALUE_CONSTRUCT_
#define RESTD_MEMORY_UNINITIALIZED_VALUE_CONSTRUCT_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_NEW

    #include <new>

    #include "restd/memory/addressof.hpp"
    #include "restd/memory/destroy_at.hpp"
    #include "restd/memory/internal/iter_value.hpp"


namespace restd
{

template<typename _ForwardIt>
void uninitialized_value_construct
(
    _ForwardIt   _first,
    _ForwardIt   _last
)
{
    typedef typename internal::iter_value<_ForwardIt>::type _T;

    _ForwardIt _current = _first;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            for (; _current != _last; ++_current)
            {
                // Parens after _T: value-initialisation.
                ::new (static_cast<void*>(restd::addressof(*_current))) _T();
            }
        }
        catch (...)
        {
            for (; _first != _current; ++_first)
            {
                restd::destroy_at(restd::addressof(*_first));
            }
            throw;
        }
    #else
        for (; _current != _last; ++_current)
        {
            ::new (static_cast<void*>(restd::addressof(*_current))) _T();
        }
    #endif
}


}  // namespace restd

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // RESTD_MEMORY_UNINITIALIZED_VALUE_CONSTRUCT_
