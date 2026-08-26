/***********************************************************************
* re_std                                 uninitialized_default_construct.hpp
*
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
* path:      /inc/djinterp/re_std/memory/uninitialized_default_construct.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_
#define DJINTERP_RE_STD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_NEW

    #include <new>

    #include "re_std/memory/addressof.hpp"
    #include "re_std/memory/destroy_at.hpp"
    #include "re_std/memory/iter_value.hpp"


namespace re_std
{

template<typename _ForwardIt>
void uninitialized_default_construct
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


}  // namespace re_std

#endif  // D_ENV_CPP98_HAS_NEW

#endif  // DJINTERP_RE_STD_MEMORY_UNINITIALIZED_DEFAULT_CONSTRUCT_
