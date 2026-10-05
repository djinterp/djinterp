/*******************************************************************************
* djinterp [re_std]                                               sentinel_t.hpp
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
* path:      /inc/re_std/ranges/sentinel_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_SENTINEL_T_HPP
#define RE_STD_RANGES_SENTINEL_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../utility/declval.hpp"
#include "../iterator/end.hpp"


namespace re_std
{


// ===========================================================================
// I.   SENTINEL_T
// ===========================================================================

// sentinel_t
//   alias: the sentinel type of Range, deduced as the return type
// of re_std::end on an lvalue of Range.
// note: for legacy ranges (where end() returns the iterator type)
// sentinel_t<R> is the same as iterator_t<R>.
template<typename Range>
using sentinel_t = decltype(re_std::end(declval<Range&>()));


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_SENTINEL_T_HPP
