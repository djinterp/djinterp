/******************************************************************************
* djinterp [re_std]                                                  nullopt.hpp
*
* nullopt_t / nullopt:
*   The disengaged-optional tag. `nullopt` is the value used to denote
* "no value present" when constructing or assigning to an optional<T>:
*     re_std::optional<int> opt = re_std::nullopt;
*     opt = re_std::nullopt;
*     if (opt == re_std::nullopt) { ... }
*
*   nullopt_t is intentionally non-default-constructible from {} alone --
* its constructor takes a private tag struct so that brace-initialization
* of an optional<T> from an empty brace-init-list does not accidentally
* construct a nullopt_t. This matches the standard's design.
*
*   STANDARD STATUS:
*   Introduced in C++17 alongside std::optional. re_std provides it on
* C++11+, since re_std's optional port targets C++11+ (lower tiers omit
* the entire optional module).
*
*   PORTABILITY:
*   Available on C++11 and later. The `inline constexpr` form for the
* `nullopt` instance uses the C++17 inline-variable feature when
* available; on C++11/14, `constexpr` at namespace scope provides
* internal linkage automatically, giving each translation unit its own
* copy of nullopt -- functionally indistinguishable since nullopt_t is
* stateless.
*
*
* path:      /inc/djinterp/re_std/optional/nullopt.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_OPTIONAL_NULLOPT_
#define DJINTERP_RE_STD_OPTIONAL_NULLOPT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_RESTD


    // nullopt_t
    //   struct: tag type denoting the disengaged state of an optional.
    //           Has an explicit constructor taking an internal tag
    //           struct, so that the only ways to obtain a nullopt_t
    //           value are (a) the global `nullopt`, or (b) explicit
    //           construction `nullopt_t{nullopt_t::construct_tag_{}}`.
    //           This prevents `optional<T> o = {}` from accidentally
    //           selecting an overload taking nullopt_t.
    struct nullopt_t
    {
        // construct_tag_
        //   struct: empty internal tag, used to gate nullopt_t's
        //           explicit constructor.
        struct construct_tag_
        {
            explicit D_CONSTEXPR construct_tag_() D_NOEXCEPT
            {}
        };

        explicit D_CONSTEXPR nullopt_t(construct_tag_) D_NOEXCEPT
        {}
    };


    // nullopt
    //   constant: the disengaged-optional value. Inline on C++17+ for
    //             single-instance semantics; pre-C++17, namespace-scope
    //             constexpr gives internal linkage and per-TU instances,
    //             which is harmless because nullopt_t carries no state.
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        inline
    #endif
    D_CONSTEXPR nullopt_t nullopt{nullopt_t::construct_tag_{}};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_OPTIONAL_NULLOPT_
