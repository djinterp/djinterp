/*******************************************************************************
* djinterp [re_std]                                        range_reference_t.hpp
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
* path:      /inc/re_std/ranges/range_reference_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_RANGE_REFERENCE_T_HPP
#define RE_STD_RANGES_RANGE_REFERENCE_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../iterator/iterator_traits.hpp"
#include "./iterator_t.hpp"


namespace re_std
{


// ===========================================================================
// I.   RANGE_REFERENCE_T
// ===========================================================================

// range_reference_t
//   alias: the reference type of Range — the type yielded by
// dereferencing an iterator. Equivalent to
// iterator_traits<iterator_t<Range>>::reference.
// note: in C++20 std this is iter_reference_t<iterator_t<R>>, which
// is defined as decltype(*declval<I&>()). The iterator_traits route
// is equivalent for every iterator whose traits primary is
// detection-based (re_std's, and std's C++17+).
template<typename Range>
using range_reference_t =
    typename iterator_traits<iterator_t<Range> >::reference;


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_RANGE_REFERENCE_T_HPP
