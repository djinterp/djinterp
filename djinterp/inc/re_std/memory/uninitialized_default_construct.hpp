/*******************************************************************************
* djinterp [re_std]                          uninitialized_default_construct.hpp
*
* uninitialized_default_construct function header:
* default-initialise each element in [_first, _last) — i.e. construct
* each as if by:  ::new (p) _T;        (no parens)
*
* default-initialisation:
*   - for class types with a user-provided default ctor: runs that ctor
*   - for trivial types: leaves the storage in INDETERMINATE state
*     (uninitialised bytes; reading them is UB)
*   - for arrays: each element is default-initialised in turn
*
* contrast with value-init (uninitialized_value_construct):
*   - for trivial types: zero-initialises
*   - for class types with a user-provided default ctor: same as default
*
* the trivial-type difference is the entire reason both functions exist.
* container implementations use default_construct when they're about to
* overwrite the storage anyway (avoiding the wasted zero-init), and
* value_construct when the user expects defined contents.
*
* added in std C++17; re_std back-ports unconditionally.
*
*
* path:      /inc/re_std/memory/uninitialized_default_construct.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_HPP
#define RE_STD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_HPP 1

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
void uninitialized_default_construct
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
                // Note the absence of parens after _T: this is
                // default-initialisation, not value-initialisation.
                ::new (static_cast<void*>(re_std::addressof(*_current))) _T;
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
            ::new (static_cast<void*>(re_std::addressof(*_current))) _T;
        }
    #endif
}


}  // re_std
#endif  // RE_STD_HAS_HEADER_NEW

#endif  // floor, for now


#endif  // RE_STD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_HPP
