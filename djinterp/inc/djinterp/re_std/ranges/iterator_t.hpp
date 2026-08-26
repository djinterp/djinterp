/******************************************************************************
* djinterp [re_std]                                             iterator_t.hpp
*
* iterator_t alias template header:
*   Yields the iterator type of a range: the return type of begin()
* on an lvalue of the range type, after reference-stripping.
*
*   PORTABILITY:
*   Requires alias templates AND decltype. Available C++11+ only. The
* alias is unavailable on C++98/03; in that case use the range type's
* own ::iterator member typedef directly, or include
* <iterator/iterator_traits.hpp> and write iterator_traits<R::iterator>.
*
*
* path:      /inc/djinterp/re_std/ranges/iterator_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_ITERATOR_T_
#define DJINTERP_RE_STD_RANGES_ITERATOR_T_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../utility/declval.hpp"
#include "../iterator/begin.hpp"


NS_RESTD


// ===========================================================================
// I.   ITERATOR_T
// ===========================================================================

// iterator_t
//   alias: the iterator type of _Range, deduced as the return type
// of re_std::begin on an lvalue of _Range. Matches C++20
// std::ranges::iterator_t.
// note: takes the begin of an lvalue (declval<_Range&>()) rather
// than an rvalue, exactly as the standard prescribes.
template<typename _Range>
using iterator_t = decltype(re_std::begin(declval<_Range&>()));


NS_END  // re_std


#endif  // alias templates + C++11


#endif  // DJINTERP_RE_STD_RANGES_ITERATOR_T_
