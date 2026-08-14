/******************************************************************************
* djinterp [restd]                                                    launder.hpp
*
* std::launder back-port:
*   Per [ptr.launder], launder(p) is the standard's way of obtaining
* a pointer to the most recently constructed object at the storage
* location p points to, after that storage has been re-used through
* placement-new. Without launder, the compiler is permitted to
* assume p still refers to the *original* object — leading to
* miscompiles in code that legitimately re-uses storage.
*
*   STRATEGY:
*     C++17+: using-declaration from std::launder (note: many std
*             implementations themselves dispatch to __builtin_launder).
*     C++11 - C++14: back-port via __builtin_launder when available
*                    (GCC 7+, Clang 3.6+). Fall back to identity
*                    function when intrinsic absent. Documented in
*                    the detection-macro contract below.
*     C++98 - C++03: same fallback strategy as C++11; no constexpr.
*
*   FALLBACK SAFETY:
*   The identity-function fallback is correct for the COMMON case:
* you placement-new a new object into existing storage of the SAME
* dynamic type, then access through the original pointer. It's
* incorrect for the more aggressive case of replacing an object with
* a different type, and only matters under optimisation when the
* compiler tracks object lifetimes (LTO + restrict analysis). Use
* the intrinsic path when possible; fallback is best-effort.
*
*   CONSTEXPR:
*   std::launder is constexpr from C++17. The intrinsic path is
* constexpr-compatible; the identity-fallback path is also
* compile-time-evaluable.
*
*   DETECTION MACRO:
*   D_RESTD_HAS_LAUNDER_INTRINSIC
*     - 1 if a compiler builtin is available (the safe path is taken).
*     - 0 if only the identity fallback is available (best-effort).
*   Override by predefining before #include.
*
*
* path:      /inc/djinterp/re_std/new/launder.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_LAUNDER_
#define DJINTERP_RESTD_LAUNDER_ 1

#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   DETECTION
// ===========================================================================

#ifndef D_RESTD_HAS_LAUNDER_INTRINSIC
    // GCC 7+ ships __builtin_launder; Clang since 3.6.
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_launder)
            #define D_RESTD_HAS_LAUNDER_INTRINSIC 1
        #else
            #define D_RESTD_HAS_LAUNDER_INTRINSIC 0
        #endif
    #elif defined(__GNUC__) && (__GNUC__ >= 7)
        #define D_RESTD_HAS_LAUNDER_INTRINSIC 1
    #else
        #define D_RESTD_HAS_LAUNDER_INTRINSIC 0
    #endif
#endif


#ifndef D_CONSTEXPR_CPP17
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_CONSTEXPR_CPP17   constexpr
    #else
        #define D_CONSTEXPR_CPP17
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   LAUNDER
// ===========================================================================

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#include <new>

// C++17+: defer to std::launder. The std implementation itself almost
// always dispatches to the compiler builtin, so we get the strong
// guarantee.
using std::launder;

#else

// Pre-C++17 back-port.
//   When the builtin is available: forward to it (strong guarantee).
//   Otherwise: identity function (best-effort, documented in the
// module-level subtitle above).
template<typename _Type>
D_CONSTEXPR_CPP17 _Type*
launder(
    _Type* _p
) D_NOEXCEPT
{
#if D_RESTD_HAS_LAUNDER_INTRINSIC
    return __builtin_launder(_p);
#else
    return _p;
#endif
}

#endif


NS_END  // restd


#endif  // DJINTERP_RESTD_LAUNDER_
