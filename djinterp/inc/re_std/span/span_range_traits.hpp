/******************************************************************************
* re_std [span]                                           span_range_traits.hpp
*
*   ranges::enable_borrowed_range and ranges::enable_view opt-ins for span.
*
*   BOTH ARE OPT-INS THAT CANNOT BE DEDUCED, which is why they exist as
* variable templates for a type to specialise rather than as traits the
* library computes.
*
*   enable_borrowed_range<span> is true because a span does not own its
* elements: its iterators stay valid after the span itself dies, so a function
* may safely return an iterator into a span passed by value.  Getting this
* wrong in either direction is a real bug - false would reject correct code
* with a borrowed_range constraint, and true on an owning type would hand back
* dangling iterators.
*
*   enable_view<span> is true because span is cheap to copy and its copy
* semantics are those of a view, not a container.  Note that span with a
* STATIC extent other than dynamic_extent is still a view: the size being a
* compile-time constant does not make it own anything.
*
*   STD IS C++20; re_std IS C++11 - the specialisations themselves need only
* variable templates, and re_std's <ranges> surface is trait-shaped from C++11.
*
* path:      /inc/djinterp/re_std/span/span_range_traits.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_SPAN_RANGE_TRAITS_
#define DJINTERP_RE_STD_SPAN_RANGE_TRAITS_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../ranges/enable_borrowed_range.hpp"
#include "../ranges/enable_view.hpp"
#include "./span.hpp"

NS_RESTD
D_NAMESPACE(ranges)

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // enable_borrowed_range<span<T, N>>
    //   constant: span never owns, so its iterators outlive it.
    template<typename _Type, size_t _Extent>
    D_CONSTEXPR bool enable_borrowed_range<span<_Type, _Extent> > = true;

    // enable_view<span<T, N>>
    //   constant: span has view copy semantics at every extent.
    template<typename _Type, size_t _Extent>
    D_CONSTEXPR bool enable_view<span<_Type, _Extent> > = true;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

NS_END  // ranges
NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_SPAN_RANGE_TRAITS_
