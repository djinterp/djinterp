/*******************************************************************************
* djinterp [re_std]                                        partial_sort_copy.hpp
*
* partial_sort_copy algorithm header:
*   Copies the smallest min(N, M) elements of [_first, _last) into
* [_d_first, _d_last) in sorted order, where N = _last - _first and
* M = _d_last - _d_first. Returns the iterator one past the last
* element written.
*
*   ALGORITHM:
*   Heap-based, mirror of partial_sort:
*     1. Copy the first min(N, M) input elements directly into the
*        output range.
*     2. Make a max-heap of what was copied.
*     3. For each remaining input element: if smaller than the heap's
*        max, write it into *_d_first and sift down.
*     4. sort_heap on the populated output range.
*   Complexity O(N log M) where M = min(input_size, output_capacity).
*
*   PORTABILITY:
*   - std::partial_sort_copy is C++98.
*   - constexpr in std from C++26; re_std does NOT lift (mirrors std).
*   - Input may be a forward iterator; output must be random access.
*   - Two overloads: default operator< and custom comparator.
*
*   INTERNAL HELPERS:
*   Duplicates _partial_sort_copy_sift_down_ here. Same cleanup note as
*   partial_sort.hpp: refactor to call public heap primitives when
*   those land.
*
*
* path:      /inc/re_std/algorithm/partial_sort_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_PARTIAL_SORT_COPY_HPP
#define RE_STD_ALGORITHM_PARTIAL_SORT_COPY_HPP 1

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

// _partial_sort_copy_sift_down_
//   function: max-heap sift-down. Same shape as the sort.hpp / partial_sort.hpp
// helpers; duplicated to keep this file independent.
template<typename RandomIt,
         typename Distance,
         typename Compare>
void
_partial_sort_copy_sift_down_(
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
// I.   PARTIAL_SORT_COPY
// ===========================================================================

// partial_sort_copy (comparator)
//   function: copies the smallest min(N, M) input elements into the
// output in sorted order. Returns one past the last element written.
template<typename InputIt,
         typename RandomIt,
         typename Compare>
RandomIt
partial_sort_copy(
    InputIt  _first,
    InputIt  _last,
    RandomIt _d_first,
    RandomIt _d_last,
    Compare  _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    if (_d_first == _d_last)
    {
        // empty output; consume nothing
        return _d_first;
    }
    if (_first == _last)
    {
        return _d_first;
    }

    // 1. copy up to M elements into the output
    RandomIt _out = _d_first;
    while ( (_first != _last) &&
            (_out   != _d_last) )
    {
        *_out = *_first;
        ++_out;
        ++_first;
    }
    _Diff _populated = _out - _d_first;

    // 2. heapify what we copied
    for (_Diff _i = _populated / 2 - 1; _i >= 0; --_i)
    {
        _partial_sort_copy_sift_down_(_d_first, _i, _populated, _comp);
    }

    // 3. stream the rest of the input
    for (; _first != _last; ++_first)
    {
        if (_comp(*_first, *_d_first))
        {
            *_d_first = *_first;
            _partial_sort_copy_sift_down_(_d_first, static_cast<_Diff>(0),
                                          _populated, _comp);
        }
    }

    // 4. sort_heap on the populated prefix
    for (_Diff _i = _populated - 1; _i > 0; --_i)
    {
        iter_swap(_d_first, _d_first + _i);
        _partial_sort_copy_sift_down_(_d_first, static_cast<_Diff>(0),
                                      _i, _comp);
    }

    return _d_first + _populated;
}


// partial_sort_copy (default operator<)
//   function: as above with re_std::less<value_type>().
template<typename InputIt,
         typename RandomIt>
RandomIt
partial_sort_copy(
    InputIt  _first,
    InputIt  _last,
    RandomIt _d_first,
    RandomIt _d_last
)
{
    typedef typename iterator_traits<RandomIt>::value_type _Value;
    return partial_sort_copy(_first, _last, _d_first, _d_last,
                             re_std::less<_Value>());
}


}  // re_std


#endif  // RE_STD_ALGORITHM_PARTIAL_SORT_COPY_HPP
