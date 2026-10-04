/******************************************************************************
* djinterp [re_std]                                                 in_place.hpp
*
* in_place_t / in_place:
*   Disambiguating tag type and instance used by optional, variant, any,
* expected, and other type-erased / sum-type containers to select the
* "construct the held object in-place from these arguments" overload.
*
*   Without this tag, a constructor like `optional(Args&&... args)` would
* be a forwarding-reference catch-all that conflicts with the conversion
* constructor `optional(U&& u)`. The standard's solution is to require
* in-place constructors to take an `in_place_t` first parameter:
*     optional<MyT> opt(in_place, ctor_arg_a, ctor_arg_b);
*
*   STANDARD STATUS:
*   Introduced in C++17 in <utility>. Provided here on C++11+ because
* optional and similar containers want it before the rest of <utility>
* is ported. Once the full <utility> umbrella exists, this file should
* be referenced from there; no migration of users is needed because the
* file path under /re_std/utility/ is the same it would have if shipped
* as part of the <utility> port from the start.
*
*   PORTABILITY:
*   Available on C++11 and later. C++98/03 omits the trait (the consumers
* it exists for, like optional with in-place construction, are themselves
* C++11+ only).
*
*
* path:      /inc/djinterp/re_std/optional/in_place.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_UTILITY_IN_PLACE_
#define DJINTERP_RE_STD_UTILITY_IN_PLACE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_RESTD


    // in_place_t
    //   struct: tag type for in-place construction. Has an explicit
    //           default constructor so that callers must write
    //           `in_place_t{}` or use the `in_place` instance, never
    //           accidentally trigger it via brace-initialization.
    struct in_place_t
    {
        explicit D_CONSTEXPR in_place_t() D_NOEXCEPT
        {}
    };


    // in_place
    //   constant: a default-constructed in_place_t. Use this where
    //             possible instead of `in_place_t{}` for readability.
    //
    //             Marked `inline` on C++17+ so all translation units
    //             share a single instance. On C++11/14 the variable
    //             has implicit internal linkage (per the const/constexpr
    //             namespace-scope rule), giving each TU its own copy --
    //             functionally indistinguishable since in_place_t is
    //             stateless.
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        inline
    #endif
    D_CONSTEXPR in_place_t in_place{};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_UTILITY_IN_PLACE_
