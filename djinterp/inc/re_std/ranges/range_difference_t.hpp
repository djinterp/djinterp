/*******************************************************************************
* djinterp [re_std]                                       range_difference_t.hpp
*
* range_difference_t alias template header:
*   Yields the difference type of a range — the signed integer type
* used to express distances between iterators of that range.
* Equivalent to iterator_traits<iterator_t<R>>::difference_type.
*
*   PORTABILITY:
*   Requires alias templates. Available C++11+ only.
*
*
* path:      /inc/re_std/ranges/range_difference_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_RANGE_DIFFERENCE_T_HPP
#define RE_STD_RANGES_RANGE_DIFFERENCE_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../iterator/iterator_traits.hpp"
#include "./iterator_t.hpp"


namespace re_std
{


// ===========================================================================
// I.   RANGE_DIFFERENCE_T
// ===========================================================================

// range_difference_t
//   alias: the signed integer difference type of Range.
// Equivalent to iterator_traits<iterator_t<Range>>::difference_type.
// note: in C++20 std this is iter_difference_t<iterator_t<R>>; the
// route through iterator_traits is equivalent because re_std's
// iterator_traits primary mirrors the std primary (cf.
// SYMBOLS_ITERATOR notes on iterator_traits).
template<typename Range>
using range_difference_t =
    typename iterator_traits<iterator_t<Range> >::difference_type;


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_RANGE_DIFFERENCE_T_HPP
