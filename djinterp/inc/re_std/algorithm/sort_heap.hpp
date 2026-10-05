/*******************************************************************************
* djinterp [re_std]                                                sort_heap.hpp
*
* sort_heap algorithm header:
*   Converts the max-heap [_first, _last) into a non-descending sorted
* range. Repeatedly pops the heap's max to the end of the shrinking
* heap; O(N log N).
*
*   PORTABILITY:
*   - std::sort_heap is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*   - Inlines a private _sift_down_ rather than depending on pop_heap
*     to avoid the per-pop range-length recheck and to keep
*     translation-unit dependencies orthogonal. The two implementations
*     are observably equivalent.
*
*
* path:      /inc/re_std/algorithm/sort_heap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_SORT_HEAP_HPP
#define RE_STD_ALGORITHM_SORT_HEAP_HPP 1

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
// 1.   INTERNAL: SIFT-DOWN
// ===========================================================================

// _sort_heap_sift_down_
//   function: max-heap sift-down. Identical in shape to the helpers in the
// other heap files; duplicated to keep this file standalone.
template<typename RandomIt,
         typename Distance,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 void
_sort_heap_sift_down_(
    RandomIt _first,
    Distance _start,
    Distance _length,
    Compare  _comp
)
{
    Distance _parent = _start;
    while (true)
    {
        Distance _child = static_cast<Distance>(2 * _parent + 1);
        if (_child >= _length)
        {
            break;
        }
        if ( ((_child + 1) < _length) &&
             _comp(*(_first + _child), *(_first + _child + 1)) )
        {
            ++_child;
        }
        if (!_comp(*(_first + _parent), *(_first + _child)))
        {
            break;
        }
        iter_swap(_first + _parent, _first + _child);
        _parent = _child;
    }
}


// ===========================================================================
// I.   SORT_HEAP
// ===========================================================================

// sort_heap (comparator)
//   function: converts the max-heap [_first, _last) into a sorted
// range. Each iteration: swap root with last-active, decrement the
// active size, sift the new root down. After N - 1 iterations the
// range is non-descending under _comp.
template<typename RandomIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 void
sort_heap(
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

    for (_Diff _i = _length - 1; _i > 0; --_i)
    {
        iter_swap(_first, _first + _i);
        _sort_heap_sift_down_<RandomIt, _Diff, Compare>(
            _first, static_cast<_Diff>(0), _i, _comp);
    }
}


// sort_heap (default operator<)
//   function: as above with re_std::less<value_type>().
template<typename RandomIt>
RE_STD_CONSTEXPR_CPP14 void
sort_heap(
    RandomIt _first,
    RandomIt _last
)
{
    typedef typename iterator_traits<RandomIt>::value_type _Value;
    sort_heap(_first, _last, re_std::less<_Value>());
}


}  // re_std


#endif  // RE_STD_ALGORITHM_SORT_HEAP_HPP
