/***********************************************************************
* re_std                                                  construct_at.hpp
*
* placement-new wrapper, normalised to look like a function call:
*   re_std::construct_at(_p, _args...) is equivalent to
*   ::new (static_cast<void*>(_p)) _T(re_std::forward<_Args>(_args)...).
* The C++20 std introduces this so that constexpr-allocator code can
* construct objects at known addresses without writing the placement-new
* expression directly (which is not constexpr until C++20).
*
* portability:
*   re_std back-ports the function to C++11+. The constexpr qualification
* is honest: it is applied only on C++20+, where the compiler is
* required to permit placement new in constant expressions. On C++11
* through C++17 the function is plain, matching what the language
* permits.
*
* C++11+ floor:
*   Requires variadic templates and perfect forwarding. On C++98/03 the
* header is empty. Code that needs construct_at on C++98 must do the
* placement-new directly.
*
*
* path:      /inc/djinterp/re_std/memory/construct_at.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.01
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_CONSTRUCT_AT_
#define DJINTERP_RE_STD_MEMORY_CONSTRUCT_AT_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #if !D_ENV_CPP98_HAS_NEW
        // Without <new>, placement-new is not declared. Skip the entire
        // body rather than emitting a hard error.
    #else

        #include <new>
        #include "re_std/utility/forward.hpp"


namespace re_std
{

// =============================================================================
// construct_at
// =============================================================================

// construct_at
//   function: in-place construct a _T at _p, forwarding _args.
//   Returns _p. constexpr only on C++20+ (placement-new in constexpr
//   contexts is a C++20 feature).
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename _T, typename... _Args>
    constexpr _T* construct_at(_T* _p, _Args&&... _args)
    {
        return ::new (static_cast<void*>(_p))
            _T(re_std::forward<_Args>(_args)...);
    }

#else

    template<typename _T, typename... _Args>
    _T* construct_at(_T* _p, _Args&&... _args)
    {
        return ::new (static_cast<void*>(_p))
            _T(re_std::forward<_Args>(_args)...);
    }

#endif


}  // namespace re_std

    #endif  // D_ENV_CPP98_HAS_NEW

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_CONSTRUCT_AT_
