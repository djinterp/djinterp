/***********************************************************************
* re_std                                 uninitialized_value_construct_n.hpp
*
* sized variant of uninitialized_value_construct.
*
* return value: past-the-end iterator.
*
*
* path:      /inc/djinterp/re_std/memory/uninitialized_value_construct_n.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_UNINITIALIZED_VALUE_CONSTRUCT_N_
#define DJINTERP_RE_STD_MEMORY_UNINITIALIZED_VALUE_CONSTRUCT_N_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_NEW

    #include <new>

    #include "re_std/memory/addressof.hpp"
    #include "re_std/memory/destroy_at.hpp"
    #include "re_std/memory/iter_value.hpp"


namespace re_std
{

template<typename _ForwardIt, typename _Size>
_ForwardIt uninitialized_value_construct_n
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
                ::new (static_cast<void*>(re_std::addressof(*_current))) _T();
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
            ::new (static_cast<void*>(re_std::addressof(*_current))) _T();
        }
        return _current;
    #endif
}


}  // namespace re_std

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // DJINTERP_RE_STD_MEMORY_UNINITIALIZED_VALUE_CONSTRUCT_N_
