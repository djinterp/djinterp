/******************************************************************************
* djinterp [re_std]                                           range_size_t.hpp
*
* range_size_t alias template header:
*   Yields the size type of a range — the unsigned integer type
* returned by re_std::size on an lvalue of the range type.
*
*   PORTABILITY:
*   Requires alias templates AND decltype. Available C++11+ only,
* and only when re_std::size is reachable for _Range (the range must
* be a sized_range — either expose .size() or have known compile-time
* extent).
*
*
* path:      /inc/djinterp/re_std/ranges/range_size_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_RANGE_SIZE_T_
#define DJINTERP_RE_STD_RANGES_RANGE_SIZE_T_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../utility/declval.hpp"
#include "../iterator/size.hpp"


NS_RESTD


// ===========================================================================
// I.   RANGE_SIZE_T
// ===========================================================================

// range_size_t
//   alias: the size type of _Range, deduced as the return type of
// re_std::size on an lvalue of _Range.
// note: only valid when re_std::size(declval<_Range&>()) is a
// well-formed expression. For ranges that don't expose .size(),
// instantiating this alias is an error.
template<typename _Range>
using range_size_t = decltype(re_std::size(declval<_Range&>()));


NS_END  // re_std


#endif  // alias templates + C++11


#endif  // DJINTERP_RE_STD_RANGES_RANGE_SIZE_T_
