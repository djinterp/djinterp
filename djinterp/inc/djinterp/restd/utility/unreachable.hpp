/***********************************************************************
* restd                                                   unreachable.hpp
*
* unreachable-code marker:
*   Marks a code path as logically unreachable. Behaviour if reached
* is undefined; the compiler is free to optimise on that assumption,
* typically eliminating the surrounding code or using the marker as
* a hint for path analysis.
*
*   Wraps __builtin_unreachable on GCC / Clang / Intel and __assume(0)
* on MSVC. On unknown compilers, the function enters an infinite loop
* to satisfy [[noreturn]] without invoking UB-on-fall-through.
*
*   STANDARD STATUS:
*   Introduced in C++23. restd back-ports to C++11+ via the existing
* compiler intrinsics, which all major compilers have shipped for years.
*
*
* path:      /inc/restd/utility/unreachable.hpp
* link(s):   TBA
* author(s): restd team                                  date: 2026.05.02
***********************************************************************/

#ifndef RESTD_UTILITY_UNREACHABLE_
#define RESTD_UTILITY_UNREACHABLE_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_RESTD

// =============================================================================
// UNREACHABLE
// =============================================================================

// unreachable
//   function: marks a code path as unreachable. [[noreturn]] hints
//   the compiler that control does not return; the body actually
//   invokes the platform's "unreachable" intrinsic to produce the
//   same optimisation hint at the IR level. On unknown compilers,
//   falls back to an infinite loop.
[[noreturn]] inline void unreachable() noexcept
{
    #if defined(D_ENV_COMPILER_GCC) \
        || defined(D_ENV_COMPILER_CLANG) \
        || defined(D_ENV_COMPILER_INTEL)
        __builtin_unreachable();
    #elif defined(D_ENV_COMPILER_MSVC)
        __assume(0);
    #else
        // Fallback: infinite loop. Satisfies [[noreturn]] without
        // hitting fall-off-end UB.
        for ( ; ; ) {}
    #endif
}

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_UTILITY_UNREACHABLE_
