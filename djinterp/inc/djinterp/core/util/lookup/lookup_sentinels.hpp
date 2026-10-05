/*******************************************************************************
* djinterp [core]                                           lookup_sentinels.hpp
*
*   The two lookup sentinels, factored into their own dependency-free header
* so that both lookup.hpp (the search families) and bsearch.hpp (the engine
* the sorted family delegates to) can share them without a cyclic include.
* `bsearch.hpp` reports misses with these values; `lookup.hpp` re-exports them
* as part of its public surface (sections I and onward).
* Nothing else belongs here.
*
*
* path:      /inc/djinterp/core/util/lookup/lookup_sentinels.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.03
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UTIL_LOOKUP_LOOKUP_SENTINELS_HPP
#define DJINTERP_UTIL_LOOKUP_LOOKUP_SENTINELS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
// djinterp
#include "../../../djinterp.hpp"


NS_DJINTERP


// lookup_not_found
//   type: sentinel returned by find_*<...>::type (and bsearch_by) when no
// entry matches the search.  Inspectable: callers can detect a miss via the
// ::found bool or by checking the returned type against this tag.
struct lookup_not_found
{};

// lookup_npos
//   value: "no position" sentinel -- the maximum value of
// std::size_t. Mirrors the std::string::npos convention: an
// unsigned-max marker returned by lookups to mean "not found".
// Kept unsigned (not ssize_t) so comparisons against .size() and
// other size_t indices stay sign-clean.
//
//   Resolution order:
//     1. C++17+   -- inline variable: a single definition shared
//        across all translation units.
//     2. C++11/14 -- static constexpr: internal linkage, so each TU
//        gets its own copy. Harmless for an integral by-value
//        constant; the only observable difference is the variable's
//        address, which a sentinel never depends on.
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    inline constexpr std::size_t lookup_npos = static_cast<std::size_t>(-1);
#else
    static constexpr std::size_t lookup_npos = static_cast<std::size_t>(-1);
#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_LOOKUP_LOOKUP_SENTINELS_HPP
