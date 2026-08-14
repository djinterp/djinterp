/******************************************************************************
* djinterp [restd]                                          range_value_t.hpp
*
* range_value_t alias template header:
*   Yields the value type of a range — equivalent to the value_type
* of iterator_traits<iterator_t<R>>. Used wherever an algorithm needs
* the element type of a range without going through the iterator type
* explicitly.
*
*   PORTABILITY:
*   Requires alias templates. Available C++11+ only.
*
*
* path:      /inc/djinterp/re_std/ranges/range_value_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_RANGE_VALUE_T_
#define DJINTERP_RESTD_RANGES_RANGE_VALUE_T_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../iterator/iterator_traits.hpp"
#include "./iterator_t.hpp"


NS_RESTD


// ===========================================================================
// I.   RANGE_VALUE_T
// ===========================================================================

// range_value_t
//   alias: the value type of _Range. Equivalent to
// iterator_traits<iterator_t<_Range>>::value_type.
// note: in C++20 std this is iter_value_t<iterator_t<R>>. restd
// routes through iterator_traits directly because iter_value_t is
// not yet shipped in restd's <iterator> surface; the result type
// is the same.
template<typename _Range>
using range_value_t =
    typename iterator_traits<iterator_t<_Range> >::value_type;


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_RANGE_VALUE_T_
