/*******************************************************************************
* djinterp [re_std]                                              nth_element.hpp
*
* nth_element algorithm header:
*   Partially-sorts [_first, _last) such that the element at _nth is
* the one that would be there in a fully-sorted range, every element
* in [_first, _nth) is not greater than *_nth, and every element in
* [_nth, _last) is not less. The internal order within each side is
* unspecified.
*
*   ALGORITHM:
*   Introselect, per Musser's introspective approach:
*     1. Quickselect via median-of-3 + Lomuto partition; descend only
*        into the side containing _nth.
*     2. When recursion depth exceeds 2 * floor(log2(N)), fall back to
*        partial_sort-style heap selection on the current subrange,
*        which finishes in O(N log N).
*     3. Below the small-range threshold, insertion-sort the remaining
*        slice. The desired element falls into place as a side effect.
*
*   COMPLEXITY:
*   Average O(N), worst-case O(N log N) (the fallback bound). Matches
*   libstdc++/libc++ and std's complexity guarantee.
*
*   PORTABILITY:
*   - std::nth_element is C++98.
*   - NOT constexpr — std does not lift this.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*
*   INTERNAL HELPERS:
*   Duplicates the median-of-3, Lomuto partition, sift-down, and
*   insertion-sort helpers from sort.hpp. Cleanup: when heap primitives
*   and sort's helpers are factored out (later batch), this file should
*   share them.
*
*
* path:      /inc/re_std/algorithm/nth_element.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_NTH_ELEMENT_HPP
#define RE_STD_ALGORITHM_NTH_ELEMENT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./iter_swap.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../functional/less.hpp"
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "../utility/move.hpp"
#endif


namespace re_std
{


// ===========================================================================
// 0.   INTERNAL HELPERS
// ===========================================================================

template<typename Distance>
inline Distance
_nth_log2_floor_(
    Distance _n
)
{
    Distance _r = 0;
    while (_n >= 2)
    {
        ++_r;
        _n /= 2;
    }
    return _r;
}


template<typename RandomIt,
         typename Compare>
void
_nth_median_of_3_(
    RandomIt _a,
    RandomIt _b,
    RandomIt _c,
    Compare  _comp
)
{
    if (_comp(*_b, *_a))
    {
        iter_swap(_a, _b);
    }
    if (_comp(*_c, *_a))
    {
        iter_swap(_a, _c);
    }
    if (_comp(*_c, *_b))
    {
        iter_swap(_b, _c);
    }
}


// Lomuto partition with median-of-3. Mirrors sort.hpp's variant.
template<typename RandomIt,
         typename Compare>
RandomIt
_nth_partition_(
    RandomIt _first,
    RandomIt _last,
    Compare  _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    _Diff     _len = _last - _first;
    RandomIt _mid = _first + (_len / 2);
    RandomIt _hi  = _last - 1;

    _nth_median_of_3_(_first, _mid, _hi, _comp);
    iter_swap(_mid, _hi);

    RandomIt _i = _first;
    for (RandomIt _j = _first; _j != _hi; ++_j)
    {
        if (_comp(*_j, *_hi))
        {
            iter_swap(_i, _j);
            ++_i;
        }
    }
    iter_swap(_i, _hi);
    return _i;
}


template<typename RandomIt,
         typename Compare>
void
_nth_insertion_(
    RandomIt _first,
    RandomIt _last,
    Compare  _comp
)
{
    typedef typename iterator_traits<RandomIt>::value_type _Value;

    if (_first == _last)
    {
        return;
    }

    RandomIt _i = _first;
    ++_i;
    for (; _i != _last; ++_i)
    {
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
        _Value _value = re_std::move(*_i);
#else
        _Value _value = *_i;
#endif

        RandomIt _j = _i;
        while (_j != _first)
        {
            RandomIt _prev = _j;
            --_prev;
            if (!_comp(_value, *_prev))
            {
                break;
            }
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
            *_j = re_std::move(*_prev);
#else
            *_j = *_prev;
#endif
            _j = _prev;
        }

#if RE_STD_LANG_HAS_RVALUE_REFERENCES
        *_j = re_std::move(_value);
#else
        *_j = _value;
#endif
    }
}


// heap-based selection fallback. Builds a max-heap of [_first, _nth+1)
// and streams the rest, replacing the heap's max whenever a smaller
// element appears. After the scan, *_nth holds the (nth - first + 1)-
// smallest element. Total O((last - first) * log(nth - first + 1)),
// strictly within O(N log N).
template<typename RandomIt,
         typename Distance,
         typename Compare>
void
_nth_sift_down_(
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


template<typename RandomIt,
         typename Compare>
void
_nth_heap_select_(
    RandomIt _first,
    RandomIt _nth,
    RandomIt _last,
    Compare  _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    _Diff _heap_size = (_nth - _first) + 1;
    if (_heap_size <= 0)
    {
        return;
    }

    // heap covers [_first, _first + _heap_size); build max-heap
    for (_Diff _i = _heap_size / 2 - 1; _i >= 0; --_i)
    {
        _nth_sift_down_(_first, _i, _heap_size, _comp);
    }

    // stream the rest: anything smaller than the heap's max evicts it
    for (RandomIt _it = _first + _heap_size; _it != _last; ++_it)
    {
        if (_comp(*_it, *_first))
        {
            iter_swap(_first, _it);
            _nth_sift_down_(_first, static_cast<_Diff>(0), _heap_size, _comp);
        }
    }

    // the heap now contains the (_heap_size) smallest elements. The
    // root is the largest of them — exactly the element that belongs
    // at *_nth.
    iter_swap(_first, _nth);
}


// ===========================================================================
// 1.   INTROSELECT DRIVER
// ===========================================================================

template<typename RandomIt,
         typename Compare,
         typename Distance>
void
_nth_introselect_(
    RandomIt _first,
    RandomIt _nth,
    RandomIt _last,
    Distance _depth_limit,
    Compare  _comp
)
{
    enum { _NTH_SMALL_THRESHOLD_ = 16 };

    while ((_last - _first) > _NTH_SMALL_THRESHOLD_)
    {
        if (_depth_limit == 0)
        {
            _nth_heap_select_(_first, _nth, _last, _comp);
            return;
        }
        --_depth_limit;

        RandomIt _cut = _nth_partition_(_first, _last, _comp);

        if (_cut == _nth)
        {
            return;
        }
        if (_cut < _nth)
        {
            _first = _cut + 1;
        }
        else
        {
            _last = _cut;
        }
    }

    _nth_insertion_(_first, _last, _comp);
}


// ===========================================================================
// I.   NTH_ELEMENT
// ===========================================================================

// nth_element (comparator)
//   function: positions *_nth as if [_first, _last) had been sorted,
// with [_first, _nth) all <= *_nth and (_nth, _last) all >= *_nth per
// _comp.
template<typename RandomIt,
         typename Compare>
void
nth_element(
    RandomIt _first,
    RandomIt _nth,
    RandomIt _last,
    Compare  _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    if ( (_first == _last) ||
         (_nth   == _last) )
    {
        return;
    }

    _Diff _len = _last - _first;
    _Diff _depth_limit = static_cast<_Diff>(2) * _nth_log2_floor_(_len);

    _nth_introselect_(_first, _nth, _last, _depth_limit, _comp);
}


// nth_element (default operator<)
//   function: as above with re_std::less<value_type>().
template<typename RandomIt>
void
nth_element(
    RandomIt _first,
    RandomIt _nth,
    RandomIt _last
)
{
    typedef typename iterator_traits<RandomIt>::value_type _Value;
    nth_element(_first, _nth, _last, re_std::less<_Value>());
}


}  // re_std


#endif  // RE_STD_ALGORITHM_NTH_ELEMENT_HPP
