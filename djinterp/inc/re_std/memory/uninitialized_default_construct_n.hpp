/*******************************************************************************
* djinterp [re_std]                        uninitialized_default_construct_n.hpp
*
* uninitialized_default_construct_n function header:
* sized variant of uninitialized_default_construct — default-initialise
* the first _n elements at _first.
*
* return value: past-the-end iterator.
*
* see uninitialized_default_construct.hpp for the semantic difference
* between default-init and value-init.
*
*
* path:      /inc/re_std/memory/uninitialized_default_construct_n.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_N_HPP
#define RE_STD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_N_HPP 1

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

template<typename ForwardIt, typename Size>
ForwardIt uninitialized_default_construct_n
(
    ForwardIt   _first,
    Size        _n
)
{
    typedef typename internal::iter_value<ForwardIt>::type _T;

    ForwardIt _current = _first;

    #if RE_STD_HAS_EXCEPTIONS
        try
        {
            for (; _n > 0; ++_current, --_n)
            {
                ::new (static_cast<void*>(re_std::addressof(*_current))) _T;
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
            ::new (static_cast<void*>(re_std::addressof(*_current))) _T;
        }
        return _current;
    #endif
}


}  // re_std
#endif  // RE_STD_HAS_HEADER_NEW

#endif  // floor, for now


#endif  // RE_STD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_N_HPP
