/******************************************************************************
* djinterp [restd]                                  hardware_interference_size.hpp
*
* hardware_destructive_interference_size /
* hardware_constructive_interference_size header:
*
*   Per [hardware.interference]:
*     - hardware_destructive_interference_size — the minimum offset
*       between two objects to avoid false sharing. Equal to the
*       cache line size on most architectures.
*     - hardware_constructive_interference_size — the maximum offset
*       between two objects to promote true sharing. Typically equal
*       to or smaller than the destructive size.
*
*   STRATEGY:
*     C++17+ in std: defer to std::hardware_*_interference_size.
*       NOTE: GCC's libstdc++ removed these by default to avoid an
*       ABI hazard (the value affects the layout of types that use
*       it as alignment). If the constants are unavailable from std,
*       we fall through to the portable defaults below.
*     C++11 - C++14 back-port: portable constexpr constants.
*     C++98 - C++03 back-port: `static const` constants (no constexpr).
*
*   PORTABLE DEFAULTS:
*     x86 / x86_64 / generic:        64 / 64
*     Apple Silicon ARM64 (perf):    128 / 64
*     PowerPC (older):               128 / 128 (not auto-detected)
*
*   COMPILER OVERRIDES:
*     If __GCC_DESTRUCTIVE_SIZE / __GCC_CONSTRUCTIVE_SIZE are
*   predefined (GCC 12+), use them. Otherwise apply heuristic.
*
*   USER OVERRIDES:
*     Predefine D_RESTD_HARDWARE_DESTRUCTIVE_SIZE and/or
*   D_RESTD_HARDWARE_CONSTRUCTIVE_SIZE before #include to force.
*
*   CONSTEXPR:
*   Constexpr from C++11 (these are integral constant expressions).
* std waited for C++17. RESTD AHEAD OF STD.
*
*
* path:      /inc/djinterp/re_std/new/hardware_interference_size.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_HARDWARE_INTERFERENCE_SIZE_
#define DJINTERP_RESTD_HARDWARE_INTERFERENCE_SIZE_ 1

#include <cstddef>
#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   PLATFORM HEURISTIC
// ===========================================================================

#ifndef D_RESTD_HARDWARE_DESTRUCTIVE_SIZE
    #if defined(__GCC_DESTRUCTIVE_SIZE)
        #define D_RESTD_HARDWARE_DESTRUCTIVE_SIZE   __GCC_DESTRUCTIVE_SIZE
    #elif defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
        // Apple Silicon performance cores use 128-byte cache lines.
        #define D_RESTD_HARDWARE_DESTRUCTIVE_SIZE   128
    #else
        // x86, x86_64, generic ARM, others.
        #define D_RESTD_HARDWARE_DESTRUCTIVE_SIZE   64
    #endif
#endif

#ifndef D_RESTD_HARDWARE_CONSTRUCTIVE_SIZE
    #if defined(__GCC_CONSTRUCTIVE_SIZE)
        #define D_RESTD_HARDWARE_CONSTRUCTIVE_SIZE  __GCC_CONSTRUCTIVE_SIZE
    #else
        #define D_RESTD_HARDWARE_CONSTRUCTIVE_SIZE  64
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   HARDWARE_DESTRUCTIVE_INTERFERENCE_SIZE
// ===========================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

D_CONSTEXPR std::size_t hardware_destructive_interference_size =
    D_RESTD_HARDWARE_DESTRUCTIVE_SIZE;

D_CONSTEXPR std::size_t hardware_constructive_interference_size =
    D_RESTD_HARDWARE_CONSTRUCTIVE_SIZE;

#else

// C++98 — `static const` integral has the same compile-time-constant
// behaviour as C++11+ `constexpr` for integral types.
static const std::size_t hardware_destructive_interference_size =
    D_RESTD_HARDWARE_DESTRUCTIVE_SIZE;

static const std::size_t hardware_constructive_interference_size =
    D_RESTD_HARDWARE_CONSTRUCTIVE_SIZE;

#endif


NS_END  // restd


#endif  // DJINTERP_RESTD_HARDWARE_INTERFERENCE_SIZE_
