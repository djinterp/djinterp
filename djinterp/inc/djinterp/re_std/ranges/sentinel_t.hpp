/******************************************************************************
* djinterp [restd]                                              sentinel_t.hpp
*
* sentinel_t alias template header:
*   Yields the sentinel type of a range: the return type of end()
* on an lvalue of the range type. For ranges where end() returns the
* iterator type (common case), sentinel_t and iterator_t coincide.
*
*   PORTABILITY:
*   Requires alias templates AND decltype. Available C++11+ only.
*
*
* path:      /inc/djinterp/re_std/ranges/sentinel_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_SENTINEL_T_
#define DJINTERP_RESTD_RANGES_SENTINEL_T_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../utility/declval.hpp"
#include "../iterator/end.hpp"


NS_RESTD


// ===========================================================================
// I.   SENTINEL_T
// ===========================================================================

// sentinel_t
//   alias: the sentinel type of _Range, deduced as the return type
// of restd::end on an lvalue of _Range.
// note: for legacy ranges (where end() returns the iterator type)
// sentinel_t<R> is the same as iterator_t<R>.
template<typename _Range>
using sentinel_t = decltype(restd::end(declval<_Range&>()));


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_SENTINEL_T_
