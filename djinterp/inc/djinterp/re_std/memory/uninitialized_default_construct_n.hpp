/***********************************************************************
* restd                                uninitialized_default_construct_n.hpp
*
* sized variant of uninitialized_default_construct — default-initialise
* the first _n elements at _first.
*
* return value: past-the-end iterator.
*
* see uninitialized_default_construct.hpp for the semantic difference
* between default-init and value-init.
*
*
* path:      /inc/djinterp/re_std/memory/uninitialized_default_construct_n.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_N_
#define RESTD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_N_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_NEW

    #include <new>

    #include "restd/memory/addressof.hpp"
    #include "restd/memory/destroy_at.hpp"
    #include "restd/memory/internal/iter_value.hpp"


namespace restd
{

template<typename _ForwardIt, typename _Size>
_ForwardIt uninitialized_default_construct_n
(
    _ForwardIt   _first,
    _Size        _n
)
{
    typedef typename internal::iter_value<_ForwardIt>::type _T;

    _ForwardIt _current = _first;

    #if D_ENV_CPP98_HAS_EXCEPTION
        try
        {
            for (; _n > 0; ++_current, --_n)
            {
                ::new (static_cast<void*>(restd::addressof(*_current))) _T;
            }
            return _current;
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
        for (; _n > 0; ++_current, --_n)
        {
            ::new (static_cast<void*>(restd::addressof(*_current))) _T;
        }
        return _current;
    #endif
}


}  // namespace restd

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // RESTD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_N_
