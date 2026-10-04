/******************************************************************************
* djinterp [re_std]                                        span_range_opt_in.hpp
*
* span range opt-in header:
*   The two ranges customisation points that span must specialise to
* participate in <ranges>:
*
*     enable_borrowed_range<span<T, E>>  -> true
*     enable_view<span<T, E>>            -> true
*
*   WHY THIS IS A SEPARATE HEADER:
*   span.hpp must not include <ranges> -- span is a C++11-tier vocabulary
* type and <ranges> sits far above it, so a dependency in that direction
* would drag the whole ranges tower into every span translation unit.
* Putting the opt-in in its own header inverts the dependency: whoever
* wants span to behave as a range includes this, and nobody else pays.
* This is the same split std achieves with a forward-declared variable
* template, which needs C++14 variable templates re_std cannot assume.
*
*   WHAT EACH ONE MEANS:
*   borrowed -- a span does not own its elements, so iterators taken from
* an EXPIRING span stay valid; that is exactly the borrowed_range
* contract, and it is what lets ranges algorithms return iterators into
* a span rvalue instead of dangling.
*   view -- a span is O(1) to copy and destroy, so it satisfies the view
* semantic requirement and can be piped through view adaptors by value.
*
*   PORTABILITY:
*   std added both in C++20; re_std back-ports them to the C++11 tier the
* ranges traits themselves ship on.
*
*
* path:      /inc/djinterp/re_std/span/span_range_opt_in.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_SPAN_SPAN_RANGE_OPT_IN_
#define DJINTERP_RE_STD_SPAN_SPAN_RANGE_OPT_IN_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./span.hpp"
#include "./dynamic_extent.hpp"
#include "../ranges/enable_borrowed_range.hpp"
#include "../ranges/enable_view.hpp"
#include "../type_traits/true_type.hpp"


NS_RESTD


// ===========================================================================
// I.   ENABLE_BORROWED_RANGE<span>
// ===========================================================================

// enable_borrowed_range<span<_Type, _Extent>>
//   trait: span refers to storage it does not own, so an iterator
// obtained from a span rvalue outlives that rvalue. Both the fixed- and
// dynamic-extent forms are covered by the one partial specialisation,
// since _Extent is a parameter here.
template<typename    _Type,
         std::size_t _Extent>
struct enable_borrowed_range<span<_Type, _Extent> >
    : true_type
{};


// ===========================================================================
// II.  ENABLE_VIEW<span>
// ===========================================================================

// enable_view<span<_Type, _Extent>>
//   trait: span is a pointer plus (at most) a size, so copy and destroy
// are both O(1) -- the semantic requirement view imposes and which no
// syntactic check can verify.
template<typename    _Type,
         std::size_t _Extent>
struct enable_view<span<_Type, _Extent> >
    : true_type
{};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_SPAN_SPAN_RANGE_OPT_IN_
