/*******************************************************************************
* djinterp [re_std]                                             construct_at.hpp
*
* placement-new wrapper, normalised to look like a function call:
*   re_std::construct_at(_p, _args...) is equivalent to
*   ::new (static_cast<void*>(_p)) T(re_std::forward<Args>(_args)...).
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
* path:      /inc/re_std/memory/construct_at.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.01
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_CONSTRUCT_AT_HPP
#define RE_STD_MEMORY_CONSTRUCT_AT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #if !RE_STD_HAS_HEADER_NEW
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
//   function: in-place construct a T at _p, forwarding _args.
//   Returns _p. constexpr only on C++20+ (placement-new in constexpr
//   contexts is a C++20 feature).
#if RE_STD_LANG_IS_CPP20_OR_HIGHER

    template<typename T, typename... Args>
    constexpr T* construct_at(T* _p, Args&&... _args)
    {
        return ::new (static_cast<void*>(_p))
            T(re_std::forward<Args>(_args)...);
    }

#else

    template<typename T, typename... Args>
    T* construct_at(T* _p, Args&&... _args)
    {
        return ::new (static_cast<void*>(_p))
            T(re_std::forward<Args>(_args)...);
    }

#endif


}  // re_std
    #endif  // RE_STD_HAS_HEADER_NEW

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_CONSTRUCT_AT_HPP
