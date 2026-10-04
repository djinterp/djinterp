/******************************************************************************
* djinterp [re_std]                                                   ignore.hpp
*
* ignore object header:
*   A sink object whose assignment operator accepts and discards any
* value. Used with tie() to skip elements when destructuring a tuple.
*
*     int a, c;
*     tie(a, ignore, c) = some_3_tuple;
*     // middle element discarded
*
*   IMPLEMENTATION:
*   The standard does not name the type explicitly (it is exposition-
* only as `unspecified`). re_std uses `internal::ignore_t` and exposes
* a `const` instance named `ignore` at namespace scope.
*
*   PORTABILITY:
*   Requires C++11+ (declared inline since C++17, but the const-instance
* form below is fine across all tiers >= C++11).
*
*
* path:      /inc/djinterp/re_std/tuple/ignore.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_TUPLE_IGNORE_
#define DJINTERP_RE_STD_TUPLE_IGNORE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_RESTD


// =============================================================================
// I.   IGNORE
// =============================================================================

NS_INTERNAL

    // ignore_t
    //   class: discard sink. Accepts any value via operator= and does
    // nothing with it. Constexpr-friendly on C++14+.
    struct ignore_t
    {
        template<typename _T>
        D_CONSTEXPR const ignore_t&
        operator=(
            const _T&
        ) const D_NOEXCEPT
        {
            return *this;
        }
    };

NS_END  // internal


// ignore
//   variable: a const ignore_t instance for use with tie(). Discards
// any value assigned to it.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    inline D_CONSTEXPR internal::ignore_t ignore = {};
#else
    static const internal::ignore_t ignore = internal::ignore_t();
#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_TUPLE_IGNORE_
