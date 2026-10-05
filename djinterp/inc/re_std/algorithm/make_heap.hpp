/*******************************************************************************
* djinterp [re_std]                                                make_heap.hpp
*
* make_heap algorithm header:
*   Rearranges [_first, _last) into a max-heap per operator< (or _comp).
* Uses the bottom-up sift-down construction (Floyd's algorithm), which
* is O(N) — strictly faster than N successive push_heap calls (O(N log
* N)).
*
*   PORTABILITY:
*   - std::make_heap is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/make_heap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_MAKE_HEAP_HPP
#define RE_STD_ALGORITHM_MAKE_HEAP_HPP 1

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

// _make_heap_sift_down_
//   function: sifts the element at index _start downward through the prefix
// [_first, _first + _length) until the heap property holds at and
// below _start.
template<typename RandomIt,
         typename Distance,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 void
_make_heap_sift_down_(
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
// I.   MAKE_HEAP
// ===========================================================================

// make_heap (comparator)
//   function: rearranges [_first, _last) into a max-heap per _comp.
// Builds bottom-up by sifting every non-leaf node down in reverse
// index order — O(N) total work.
template<typename RandomIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 void
make_heap(
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

    // last non-leaf is at index (length / 2) - 1
    for (_Diff _i = _length / 2 - 1; _i >= 0; --_i)
    {
        _make_heap_sift_down_<RandomIt, _Diff, Compare>(
            _first, _i, _length, _comp);
    }
}


// make_heap (default operator<)
//   function: as above with re_std::less<value_type>().
template<typename RandomIt>
RE_STD_CONSTEXPR_CPP14 void
make_heap(
    RandomIt _first,
    RandomIt _last
)
{
    typedef typename iterator_traits<RandomIt>::value_type _Value;
    make_heap(_first, _last, re_std::less<_Value>());
}


}  // re_std


#endif  // RE_STD_ALGORITHM_MAKE_HEAP_HPP
