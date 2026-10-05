/*******************************************************************************
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
* path:      /inc/re_std/tuple/ignore.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_IGNORE_HPP
#define RE_STD_TUPLE_IGNORE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


namespace re_std
{


// =============================================================================
// I.   IGNORE
// =============================================================================

namespace internal
{

    // ignore_t
    //   class: discard sink. Accepts any value via operator= and does
    // nothing with it. Constexpr-friendly on C++14+.
    struct ignore_t
    {
        template<typename T>
        RE_STD_CONSTEXPR const ignore_t&
        operator=(
            const T&
        ) const RE_STD_NOEXCEPT
        {
            return *this;
        }
    };

}  // internal


// ignore
//   variable: a const ignore_t instance for use with tie(). Discards
// any value assigned to it.
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
    inline RE_STD_CONSTEXPR internal::ignore_t ignore = {};
#else
    static const internal::ignore_t ignore = internal::ignore_t();
#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_TUPLE_IGNORE_HPP
