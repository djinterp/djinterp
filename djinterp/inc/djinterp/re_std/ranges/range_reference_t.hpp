/******************************************************************************
* djinterp [restd]                                      range_reference_t.hpp
*
* range_reference_t alias template header:
*   Yields the reference type of a range — the type of *it for an
* iterator of that range. Equivalent to
* iterator_traits<iterator_t<R>>::reference.
*
*   PORTABILITY:
*   Requires alias templates. Available C++11+ only.
*
*
* path:      /inc/djinterp/re_std/ranges/range_reference_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_RANGE_REFERENCE_T_
#define DJINTERP_RESTD_RANGES_RANGE_REFERENCE_T_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../iterator/iterator_traits.hpp"
#include "./iterator_t.hpp"


NS_RESTD


// ===========================================================================
// I.   RANGE_REFERENCE_T
// ===========================================================================

// range_reference_t
//   alias: the reference type of _Range — the type yielded by
// dereferencing an iterator. Equivalent to
// iterator_traits<iterator_t<_Range>>::reference.
// note: in C++20 std this is iter_reference_t<iterator_t<R>>, which
// is defined as decltype(*declval<I&>()). The iterator_traits route
// is equivalent for every iterator whose traits primary is
// detection-based (restd's, and std's C++17+).
template<typename _Range>
using range_reference_t =
    typename iterator_traits<iterator_t<_Range> >::reference;


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_RANGE_REFERENCE_T_
