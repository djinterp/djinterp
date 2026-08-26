/******************************************************************************
* re_std [ranges]                                    range_common_reference.hpp
*
*   range_common_reference_t<R> - the common reference type of a range's
* reference and rvalue-reference types.
*
*   WHAT IT IS FOR.
*   An algorithm that may hold an element either as a reference into the range
* or as a moved-from rvalue needs one type that both convert to.  For an
* ordinary container that is just `T&`; for a proxy range - vector<bool>, or a
* zip_view yielding a tuple of references - the two differ, and this alias is
* what names the type generic code must use.
*
*   It is a one-line composition over pieces that already exist, which is why
* it was the only <ranges> gap not waiting on an adaptor.
*
*   STD IS C++20; re_std IS C++11 - it needs only alias templates and
* common_reference, both of which re_std already has.
*
* path:      /inc/djinterp/re_std/ranges/range_common_reference.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_RANGE_COMMON_REFERENCE_
#define DJINTERP_RE_STD_RANGES_RANGE_COMMON_REFERENCE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./range_traits.hpp"

NS_RESTD
D_NAMESPACE(ranges)

// range_common_reference_t
//   alias: the common reference of a range's reference and rvalue-reference.
template<typename _Range>
using range_common_reference_t = typename common_reference<
    range_reference_t<_Range>,
    range_rvalue_reference_t<_Range> >::type;

NS_END  // ranges
NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_RANGES_RANGE_COMMON_REFERENCE_
