/*******************************************************************************
* djinterp [re_std]                                                is_sorted.hpp
*
* is_sorted algorithm header:
*   Returns true if [_first, _last) is sorted in non-descending order
* per operator< (or _comp). Vacuously true for empty / one-element
* ranges. Built on is_sorted_until (the position of the first
* out-of-order element is _last iff the range is sorted).
*
*   PORTABILITY:
*   - std::is_sorted is C++11; re_std back-ports to C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/is_sorted.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_IS_SORTED_HPP
#define RE_STD_ALGORITHM_IS_SORTED_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./is_sorted_until.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   IS_SORTED (DEFAULT operator<)
// ===========================================================================

// is_sorted
//   function: returns true if every adjacent pair satisfies !(b < a).
template<typename ForwardIt>
RE_STD_CONSTEXPR_CPP14 bool
is_sorted(
    ForwardIt _first,
    ForwardIt _last
)
{
    return re_std::is_sorted_until(_first, _last) == _last;
}


// ===========================================================================
// II.  IS_SORTED (COMPARATOR)
// ===========================================================================

// is_sorted (comparator)
//   function: as above but adjacent-pair ordering is via _comp.
template<typename ForwardIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 bool
is_sorted(
    ForwardIt _first,
    ForwardIt _last,
    Compare   _comp
)
{
    return re_std::is_sorted_until(_first, _last, _comp) == _last;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_IS_SORTED_HPP
