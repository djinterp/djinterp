/******************************************************************************
* djinterp [restd]                                     range_difference_t.hpp
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
* path:      /inc/djinterp/restd/ranges/range_difference_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_RANGE_DIFFERENCE_T_
#define DJINTERP_RESTD_RANGES_RANGE_DIFFERENCE_T_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../iterator/iterator_traits.hpp"
#include "./iterator_t.hpp"


NS_RESTD


// ===========================================================================
// I.   RANGE_DIFFERENCE_T
// ===========================================================================

// range_difference_t
//   alias: the signed integer difference type of _Range.
// Equivalent to iterator_traits<iterator_t<_Range>>::difference_type.
// note: in C++20 std this is iter_difference_t<iterator_t<R>>; the
// route through iterator_traits is equivalent because restd's
// iterator_traits primary mirrors the std primary (cf.
// SYMBOLS_ITERATOR notes on iterator_traits).
template<typename _Range>
using range_difference_t =
    typename iterator_traits<iterator_t<_Range> >::difference_type;


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_RANGE_DIFFERENCE_T_
