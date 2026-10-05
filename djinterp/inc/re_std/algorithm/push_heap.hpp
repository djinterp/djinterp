/*******************************************************************************
* djinterp [re_std]                                                push_heap.hpp
*
* push_heap algorithm header:
*   Treats [_first, _last - 1) as a max-heap and inserts *(_last - 1)
* into it by sifting that element upward toward the root until the heap
* property is restored.
*
*   PORTABILITY:
*   - std::push_heap is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*   - Implementation uses iter_swap-based sift-up; see header comment in
*     pop_heap.hpp for the swap-vs-hole-walking trade-off.
*
*
* path:      /inc/re_std/algorithm/push_heap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_PUSH_HEAP_HPP
#define RE_STD_ALGORITHM_PUSH_HEAP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./iter_swap.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../functional/less.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// 1.   INTERNAL: SIFT-UP
// ===========================================================================

// _push_heap_sift_up_
//   function: sifts the element at index _start in [_first, _first + _length)
// upward, swapping with its parent while it compares greater than that
// parent under _comp. The parent of index i is (i - 1) / 2.
template<typename RandomIt,
         typename Distance,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 void
_push_heap_sift_up_(
    RandomIt _first,
    Distance _start,
    Compare  _comp
)
{
    Distance _hole = _start;
    while (_hole > 0)
    {
        Distance _parent = static_cast<Distance>((_hole - 1) / 2);
        if (!_comp(*(_first + _parent), *(_first + _hole)))
        {
            break;
        }
        iter_swap(_first + _parent, _first + _hole);
        _hole = _parent;
    }
}


// ===========================================================================
// I.   PUSH_HEAP
// ===========================================================================

// push_heap (comparator)
//   function: inserts *(_last - 1) into the max-heap that [_first,
// _last - 1) is assumed to be. After return, [_first, _last) is a
// valid heap. No-op if the input range has fewer than two elements.
template<typename RandomIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 void
push_heap(
    RandomIt _first,
    RandomIt _last,
    Compare  _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    _Diff _length = _last - _first;
    if (_length < 2)
    {
        return;
    }

    _push_heap_sift_up_<RandomIt, _Diff, Compare>(
        _first, _length - 1, _comp);
}


// push_heap (default operator<)
//   function: as above with re_std::less<value_type>().
template<typename RandomIt>
RE_STD_CONSTEXPR_CPP14 void
push_heap(
    RandomIt _first,
    RandomIt _last
)
{
    typedef typename iterator_traits<RandomIt>::value_type _Value;
    push_heap(_first, _last, re_std::less<_Value>());
}


}  // re_std


#endif  // RE_STD_ALGORITHM_PUSH_HEAP_HPP
