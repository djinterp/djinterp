/******************************************************************************
* djinterp [restd]                                         partial_sort_copy.hpp
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
*   - constexpr in std from C++26; restd does NOT lift (mirrors std).
*   - Input may be a forward iterator; output must be random access.
*   - Two overloads: default operator< and custom comparator.
*
*   INTERNAL HELPERS:
*   Duplicates _partial_sort_copy_sift_down_ here. Same cleanup note as
*   partial_sort.hpp: refactor to call public heap primitives when
*   those land.
*
*
* path:      /inc/djinterp/restd/algorithm/partial_sort_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_PARTIAL_SORT_COPY_
#define DJINTERP_RESTD_ALGORITHM_PARTIAL_SORT_COPY_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./iter_swap.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../functional/less.hpp"


NS_RESTD


// ===========================================================================
// 0.   INTERNAL
// ===========================================================================

// _partial_sort_copy_sift_down_
//   max-heap sift-down. Same shape as the sort.hpp / partial_sort.hpp
// helpers; duplicated to keep this file independent.
template<typename _RandomIt,
         typename _Distance,
         typename _Compare>
void
_partial_sort_copy_sift_down_(
    _RandomIt _first,
    _Distance _start,
    _Distance _length,
    _Compare  _comp
)
{
    _Distance _parent = _start;
    while (true)
    {
        _Distance _child = static_cast<_Distance>(2 * _parent + 1);
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
template<typename _InputIt,
         typename _RandomIt,
         typename _Compare>
_RandomIt
partial_sort_copy(
    _InputIt  _first,
    _InputIt  _last,
    _RandomIt _d_first,
    _RandomIt _d_last,
    _Compare  _comp
)
{
    typedef typename iterator_traits<_RandomIt>::difference_type _Diff;

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
    _RandomIt _out = _d_first;
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
//   function: as above with restd::less<value_type>().
template<typename _InputIt,
         typename _RandomIt>
_RandomIt
partial_sort_copy(
    _InputIt  _first,
    _InputIt  _last,
    _RandomIt _d_first,
    _RandomIt _d_last
)
{
    typedef typename iterator_traits<_RandomIt>::value_type _Value;
    return partial_sort_copy(_first, _last, _d_first, _d_last,
                             restd::less<_Value>());
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_PARTIAL_SORT_COPY_
