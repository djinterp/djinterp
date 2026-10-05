/*******************************************************************************
* djinterp [re_std]                                        span_range_traits.hpp
*
* forwarding header:
*   span's opt-in to the ranges customisation points --
* enable_borrowed_range<span<T, E>> and enable_view<span<T, E>>, both true
* -- is defined once, in re_std/span/span_range_opt_in.hpp, whose banner
* explains why it is a header of its own. This file held a second copy,
* so a translation unit that included both redefined both
* specialisations; it now forwards to the single definition, and its
* path keeps working.
*
*
* path:      /inc/re_std/span/span_range_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_SPAN_SPAN_RANGE_TRAITS_HPP
#define RE_STD_SPAN_SPAN_RANGE_TRAITS_HPP 1

// re_std
#include "./span_range_opt_in.hpp"  // span's opt-in

#endif  // RE_STD_SPAN_SPAN_RANGE_TRAITS_HPP
