/*******************************************************************************
* djinterp [re_std]                                                  launder.hpp
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
*   RE_STD_HAS_LAUNDER_INTRINSIC
*     - 1 if a compiler builtin is available (the safe path is taken).
*     - 0 if only the identity fallback is available (best-effort).
*   Override by predefining before #include.
*
*
* path:      /inc/re_std/new/launder.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NEW_LAUNDER_HPP
#define RE_STD_NEW_LAUNDER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

// std
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
    #include <new>  // std::launder
#endif


// ===========================================================================
// 0.   DETECTION
// ===========================================================================

#ifndef RE_STD_HAS_LAUNDER_INTRINSIC
    // GCC 7+ ships __builtin_launder; Clang since 3.6.
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_launder)
            #define RE_STD_HAS_LAUNDER_INTRINSIC 1
        #else
            #define RE_STD_HAS_LAUNDER_INTRINSIC 0
        #endif
    #elif defined(__GNUC__) && (__GNUC__ >= 7)
        #define RE_STD_HAS_LAUNDER_INTRINSIC 1
    #else
        #define RE_STD_HAS_LAUNDER_INTRINSIC 0
    #endif
#endif


namespace re_std
{


// ===========================================================================
// I.   LAUNDER
// ===========================================================================

#if RE_STD_LANG_IS_CPP17_OR_HIGHER

// C++17+: defer to std::launder. The std implementation itself almost
// always dispatches to the compiler builtin, so we get the strong
// guarantee.
using std::launder;

#else

// Pre-C++17 back-port.
//   When the builtin is available: forward to it (strong guarantee).
//   Otherwise: identity function (best-effort, documented in the
// module-level subtitle above).
template<typename Type>
RE_STD_CONSTEXPR_CPP17 Type*
launder(
    Type* _p
) RE_STD_NOEXCEPT
{
#if RE_STD_HAS_LAUNDER_INTRINSIC
    return __builtin_launder(_p);
#else
    return _p;
#endif
}

#endif


}  // re_std


#endif  // RE_STD_NEW_LAUNDER_HPP
