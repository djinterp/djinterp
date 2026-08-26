/***********************************************************************
* re_std                                             dynamic_extent.hpp
*
* the dynamic_extent constant:
*   Sentinel size used as the default second template argument of
* re_std::span, selecting the run-time-sized (rather than fixed-extent)
* specialization. Equal to (size_t)-1, matching std::dynamic_extent.
*
*
* path:      /inc/djinterp/re_std/span/dynamic_extent.hpp
* link(s):   TBA
* author(s): re_std contributors                       date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_SPAN_DYNAMIC_EXTENT_
#define DJINTERP_RE_STD_SPAN_DYNAMIC_EXTENT_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>  // size_t

namespace re_std
{

    // dynamic_extent
    //   constant: size_t sentinel marking a span whose size is tracked at
    //   run time. Defined as (size_t)-1 to avoid a <limits> dependency;
    //   identical in value to std::numeric_limits<size_t>::max(). On
    //   C++17+ it is an inline variable (ODR-safe, external linkage,
    //   matching std); on C++11/14 it is a plain constexpr namespace-scope
    //   constant (internal linkage), which is sufficient since it is only
    //   ever consumed by value as a template argument or in comparisons.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    inline constexpr std::size_t dynamic_extent = static_cast<std::size_t>(-1);
#else
    D_CONSTEXPR std::size_t dynamic_extent = static_cast<std::size_t>(-1);
#endif

}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_SPAN_DYNAMIC_EXTENT_
