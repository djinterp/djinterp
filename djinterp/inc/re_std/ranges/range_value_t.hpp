/*******************************************************************************
* djinterp [re_std]                                            range_value_t.hpp
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
* path:      /inc/re_std/ranges/range_value_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_RANGE_VALUE_T_HPP
#define RE_STD_RANGES_RANGE_VALUE_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../iterator/iterator_traits.hpp"
#include "./iterator_t.hpp"


namespace re_std
{


// ===========================================================================
// I.   RANGE_VALUE_T
// ===========================================================================

// range_value_t
//   alias: the value type of Range. Equivalent to
// iterator_traits<iterator_t<Range>>::value_type.
// note: in C++20 std this is iter_value_t<iterator_t<R>>. re_std
// routes through iterator_traits directly because iter_value_t is
// not yet shipped in re_std's <iterator> surface; the result type
// is the same.
template<typename Range>
using range_value_t =
    typename iterator_traits<iterator_t<Range> >::value_type;


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_RANGE_VALUE_T_HPP
