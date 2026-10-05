/*******************************************************************************
* djinterp [re_std]                                                  is_heap.hpp
*
* is_heap algorithm header:
*   Returns true if [_first, _last) is a max-heap under operator< (or
* _comp). Built on is_heap_until: the range is a heap iff the
* until-iterator equals _last.
*
*   PORTABILITY:
*   - std::is_heap is C++11; re_std back-ports to C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/is_heap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_IS_HEAP_HPP
#define RE_STD_ALGORITHM_IS_HEAP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./is_heap_until.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   IS_HEAP (DEFAULT operator<)
// ===========================================================================

// is_heap
//   function: returns true if [_first, _last) is a max-heap.
template<typename RandomIt>
RE_STD_CONSTEXPR_CPP14 bool
is_heap(
    RandomIt _first,
    RandomIt _last
)
{
    return re_std::is_heap_until(_first, _last) == _last;
}


// ===========================================================================
// II.  IS_HEAP (COMPARATOR)
// ===========================================================================

// is_heap (comparator)
//   function: as above but using _comp for parent-child comparisons.
template<typename RandomIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 bool
is_heap(
    RandomIt _first,
    RandomIt _last,
    Compare  _comp
)
{
    return re_std::is_heap_until(_first, _last, _comp) == _last;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_IS_HEAP_HPP
