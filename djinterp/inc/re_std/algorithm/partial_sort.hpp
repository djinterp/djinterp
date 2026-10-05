/*******************************************************************************
* djinterp [re_std]                                             partial_sort.hpp
*
* partial_sort algorithm header:
*   Rearranges [_first, _last) so that the smallest (_middle - _first)
* elements end up at the front of the range in sorted order; the
* remaining [_middle, _last) is left in unspecified order. Per
* operator< or _comp.
*
*   ALGORITHM:
*   Heap-based, O(N log k) where k = _middle - _first:
*     1. Make a max-heap of [_first, _middle).
*     2. For each it in [_middle, _last): if *it < heap_top, swap and
*        sift-down.
*     3. sort_heap on [_first, _middle): O(k log k).
*   The heap's max stays at *_first throughout; the invariant after the
*   scan is "the heap contains the k smallest elements seen, with the
*   k-th smallest at the root".
*
*   PORTABILITY:
*   - std::partial_sort is C++98.
*   - NOT constexpr — std does not lift this through C++26.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*
*   INTERNAL HELPERS:
*   Inlines _partial_sift_down_ here rather than calling the public
*   heap primitives (which ship in a later batch). Duplicates the same
*   helper that sort.hpp uses. When make_heap/sort_heap land this file
*   should refactor.
*
*
* path:      /inc/re_std/algorithm/partial_sort.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_PARTIAL_SORT_HPP
#define RE_STD_ALGORITHM_PARTIAL_SORT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./iter_swap.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../functional/less.hpp"


namespace re_std
{


// ===========================================================================
// 0.   INTERNAL
// ===========================================================================

// _partial_sift_down_
//   function: max-heap sift-down. Same shape as sort.hpp's _sort_sift_down_;
// duplicated to keep the two files independent until heap primitives
// ship publicly.
template<typename RandomIt,
         typename Distance,
         typename Compare>
void
_partial_sift_down_(
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
// I.   PARTIAL_SORT
// ===========================================================================

// partial_sort (comparator)
//   function: arranges the smallest k = _middle - _first elements of
// [_first, _last) at the front in sorted order; [_middle, _last)
// retains the rest in unspecified order. Per _comp.
template<typename RandomIt,
         typename Compare>
void
partial_sort(
    RandomIt _first,
    RandomIt _middle,
    RandomIt _last,
    Compare  _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    _Diff _k = _middle - _first;
    if (_k < 2)
    {
        // k == 0: trivial
        // k == 1: scan for the minimum and place at *_first
        if ( (_k == 1) &&
             (_first != _last) )
        {
            RandomIt _min = _first;
            for (RandomIt _it = _first + 1; _it != _last; ++_it)
            {
                if (_comp(*_it, *_min))
                {
                    _min = _it;
                }
            }
            iter_swap(_first, _min);
        }
        return;
    }

    // 1. build a max-heap of [_first, _middle)
    for (_Diff _i = _k / 2 - 1; _i >= 0; --_i)
    {
        _partial_sift_down_(_first, _i, _k, _comp);
    }

    // 2. for each element of [_middle, _last): if it is smaller than
    //    the heap's max, evict the max and sift the new value down
    for (RandomIt _it = _middle; _it != _last; ++_it)
    {
        if (_comp(*_it, *_first))
        {
            iter_swap(_first, _it);
            _partial_sift_down_(_first, static_cast<_Diff>(0), _k, _comp);
        }
    }

    // 3. sort_heap on [_first, _middle): repeatedly extract the max
    //    to the end of the shrinking heap
    for (_Diff _i = _k - 1; _i > 0; --_i)
    {
        iter_swap(_first, _first + _i);
        _partial_sift_down_(_first, static_cast<_Diff>(0), _i, _comp);
    }
}


// partial_sort (default operator<)
//   function: as above with re_std::less<value_type>().
template<typename RandomIt>
void
partial_sort(
    RandomIt _first,
    RandomIt _middle,
    RandomIt _last
)
{
    typedef typename iterator_traits<RandomIt>::value_type _Value;
    partial_sort(_first, _middle, _last, re_std::less<_Value>());
}


}  // re_std


#endif  // RE_STD_ALGORITHM_PARTIAL_SORT_HPP
