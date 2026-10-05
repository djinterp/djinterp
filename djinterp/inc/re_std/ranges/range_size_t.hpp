/*******************************************************************************
* djinterp [re_std]                                             range_size_t.hpp
*
* range_size_t alias template header:
*   Yields the size type of a range — the unsigned integer type
* returned by re_std::size on an lvalue of the range type.
*
*   PORTABILITY:
*   Requires alias templates AND decltype. Available C++11+ only,
* and only when re_std::size is reachable for Range (the range must
* be a sized_range — either expose .size() or have known compile-time
* extent).
*
*
* path:      /inc/re_std/ranges/range_size_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_RANGE_SIZE_T_HPP
#define RE_STD_RANGES_RANGE_SIZE_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../utility/declval.hpp"
#include "../iterator/size.hpp"


namespace re_std
{


// ===========================================================================
// I.   RANGE_SIZE_T
// ===========================================================================

// range_size_t
//   alias: the size type of Range, deduced as the return type of
// re_std::size on an lvalue of Range.
// note: only valid when re_std::size(declval<Range&>()) is a
// well-formed expression. For ranges that don't expose .size(),
// instantiating this alias is an error.
template<typename Range>
using range_size_t = decltype(re_std::size(declval<Range&>()));


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_RANGE_SIZE_T_HPP
