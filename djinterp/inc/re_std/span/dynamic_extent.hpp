/*******************************************************************************
* djinterp [re_std]                                           dynamic_extent.hpp
*
* the dynamic_extent constant:
*   Sentinel size used as the default second template argument of
* re_std::span, selecting the run-time-sized (rather than fixed-extent)
* specialization. Equal to (size_t)-1, matching std::dynamic_extent.
*
*
* path:      /inc/re_std/span/dynamic_extent.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SPAN_DYNAMIC_EXTENT_HPP
#define RE_STD_SPAN_DYNAMIC_EXTENT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
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
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
    inline constexpr std::size_t dynamic_extent = static_cast<std::size_t>(-1);
#else
    RE_STD_CONSTEXPR std::size_t dynamic_extent = static_cast<std::size_t>(-1);
#endif

}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SPAN_DYNAMIC_EXTENT_HPP
