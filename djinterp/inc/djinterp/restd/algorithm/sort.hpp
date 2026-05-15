/******************************************************************************
* djinterp [restd]                                                      sort.hpp
*
* sort algorithm header:
*   Sorts the elements of [_first, _last) in non-descending order per
* operator< (or _comp). Unstable (equal elements may be reordered).
* Average and worst-case complexity O(N log N).
*
*   ALGORITHM:
*   Introsort, per Musser, "Introspective Sorting and Selection
* Algorithms", Software: Practice and Experience 27(8), 1997.
*   Three components:
*     1. Quicksort, with median-of-3 pivot and Lomuto partition.
*     2. When recursion depth exceeds 2 * floor(log2(N)), abandons the
*        current subrange to heapsort. Guarantees O(N log N) worst-case
*        without sacrificing the quicksort fast path on typical input.
*     3. When the active subrange falls below a small-size threshold
*        (16), defers to insertion sort. Reduces constant factor for
*        the dense bottom of the recursion tree.
*   Tail-recursion is eliminated on the larger partition half; the
* explicit recursive call is on the smaller side, bounding stack depth
* at O(log N).
*
*   PORTABILITY:
*   - std::sort is C++98.
*   - NOT constexpr — std does not lift this through C++26 either.
*   - C++11+ uses move semantics for the insertion-sort element shifts;
*     C++98 uses copy (gated on D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES).
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*
*   INTERNAL HELPERS:
*   The heapsort fallback inlines _sort_sift_down_ and _sort_heap_sort_
*   here rather than depending on the public make_heap/sort_heap (which
*   ship in a later batch). Once those land this file should call
*   restd::sort_heap(first, last, comp) and remove the duplicates.
*
*
* path:      /inc/djinterp/restd/algorithm/sort.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_SORT_
#define DJINTERP_RESTD_ALGORITHM_SORT_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./iter_swap.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../functional/less.hpp"
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "../utility/move.hpp"
#endif


NS_RESTD


// ===========================================================================
// 0.   INTERNAL HELPERS
// ===========================================================================

// _sort_log2_floor_
//   integer floor(log2(_n)) for _n >= 1. Returns 0 for _n == 1.
template<typename _Distance>
inline _Distance
_sort_log2_floor_(
    _Distance _n
)
{
    _Distance _r = 0;
    while (_n >= 2)
    {
        ++_r;
        _n /= 2;
    }
    return _r;
}


// _sort_insertion_
//   insertion sort on [_first, _last). Used as the bottom-of-recursion
// fallback in introsort. Stable on its own (the stability is incidental
// and not contractually exposed by sort).
template<typename _RandomIt,
         typename _Compare>
void
_sort_insertion_(
    _RandomIt _first,
    _RandomIt _last,
    _Compare  _comp
)
{
    typedef typename iterator_traits<_RandomIt>::value_type _Value;

    if (_first == _last)
    {
        return;
    }

    _RandomIt _i = _first;
    ++_i;
    for (; _i != _last; ++_i)
    {
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
        _Value _value = restd::move(*_i);
#else
        _Value _value = *_i;
#endif

        _RandomIt _j = _i;
        while (_j != _first)
        {
            _RandomIt _prev = _j;
            --_prev;
            if (!_comp(_value, *_prev))
            {
                break;
            }
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
            *_j = restd::move(*_prev);
#else
            *_j = *_prev;
#endif
            _j = _prev;
        }

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
        *_j = restd::move(_value);
#else
        *_j = _value;
#endif
    }
}


// _sort_sift_down_
//   max-heap sift-down on [_first, _first + _length) per _comp. Sifts
// the element at index _start downward to restore the heap property.
// Children of index i are at 2i + 1 and 2i + 2.
template<typename _RandomIt,
         typename _Distance,
         typename _Compare>
void
_sort_sift_down_(
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
        // pick the larger child
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


// _sort_heap_sort_
//   heapsort on [_first, _last). Used as the recursion-depth-exceeded
// fallback in introsort. O(N log N) worst case.
template<typename _RandomIt,
         typename _Compare>
void
_sort_heap_sort_(
    _RandomIt _first,
    _RandomIt _last,
    _Compare  _comp
)
{
    typedef typename iterator_traits<_RandomIt>::difference_type _Diff;

    _Diff _length = _last - _first;
    if (_length < 2)
    {
        return;
    }

    // build the max-heap bottom-up
    for (_Diff _i = _length / 2 - 1; _i >= 0; --_i)
    {
        _sort_sift_down_(_first, _i, _length, _comp);
    }

    // sort: repeatedly move max to the end and shrink the heap
    for (_Diff _i = _length - 1; _i > 0; --_i)
    {
        iter_swap(_first, _first + _i);
        _sort_sift_down_(_first, static_cast<_Diff>(0), _i, _comp);
    }
}


// _sort_median_of_3_
//   sorts {*_a, *_b, *_c} via two-element swaps so that *_a <= *_b <=
// *_c per _comp. The median ends up at *_b.
template<typename _RandomIt,
         typename _Compare>
void
_sort_median_of_3_(
    _RandomIt _a,
    _RandomIt _b,
    _RandomIt _c,
    _Compare  _comp
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


// _sort_partition_
//   Lomuto partition with median-of-3 pivot selection. Picks the
// median of {*_first, *(_first + N/2), *(_last - 1)} as the pivot,
// moves it to the end of the range, partitions in-place, and returns
// the final iterator position of the pivot. After return, every
// element in [_first, ret) compares less than *ret, and every element
// in [ret + 1, _last) is not less.
template<typename _RandomIt,
         typename _Compare>
_RandomIt
_sort_partition_(
    _RandomIt _first,
    _RandomIt _last,
    _Compare  _comp
)
{
    typedef typename iterator_traits<_RandomIt>::difference_type _Diff;

    _Diff     _len = _last - _first;
    _RandomIt _mid = _first + (_len / 2);
    _RandomIt _hi  = _last - 1;

    // sort {first, mid, hi} so that the median lives at *_mid
    _sort_median_of_3_(_first, _mid, _hi, _comp);

    // move the pivot to *_hi so Lomuto can use it as a fixed reference
    iter_swap(_mid, _hi);

    // Lomuto: i tracks the boundary "[_first, i) contains all less-than-pivot
    // elements seen so far". For each j in [_first, _hi), if *_j < pivot,
    // place it at *_i and advance i.
    _RandomIt _i = _first;
    for (_RandomIt _j = _first; _j != _hi; ++_j)
    {
        if (_comp(*_j, *_hi))
        {
            iter_swap(_i, _j);
            ++_i;
        }
    }

    // place pivot at *_i (its final sorted position)
    iter_swap(_i, _hi);

    return _i;
}


// _sort_introsort_loop_
//   the main introsort driver. Quicksorts down to the small-range
// threshold, dropping to heapsort if recursion depth is exhausted.
// Tail-recurses on the larger side to keep stack depth O(log N).
template<typename _RandomIt,
         typename _Compare,
         typename _Distance>
void
_sort_introsort_loop_(
    _RandomIt _first,
    _RandomIt _last,
    _Distance _depth_limit,
    _Compare  _comp
)
{
    // small-range threshold below which insertion sort wins on
    // constant factor. 16 matches libstdc++/libc++.
    enum { _SORT_SMALL_THRESHOLD_ = 16 };

    while ((_last - _first) > _SORT_SMALL_THRESHOLD_)
    {
        if (_depth_limit == 0)
        {
            // recursion limit hit; finish this range with heapsort
            _sort_heap_sort_(_first, _last, _comp);
            return;
        }
        --_depth_limit;

        _RandomIt _cut = _sort_partition_(_first, _last, _comp);

        // recurse on the smaller half, iterate on the larger half
        if ((_cut - _first) < (_last - _cut))
        {
            _sort_introsort_loop_(_first, _cut, _depth_limit, _comp);
            _first = _cut + 1;  // skip the pivot (already final)
        }
        else
        {
            _sort_introsort_loop_(_cut + 1, _last, _depth_limit, _comp);
            _last = _cut;
        }
    }

    // tail: insertion sort cleans up the remaining short range(s)
    _sort_insertion_(_first, _last, _comp);
}


NS_END  // restd


// ===========================================================================
// I.   SORT
// ===========================================================================

NS_RESTD


// sort (comparator)
//   function: sorts [_first, _last) into non-descending order per
// _comp. Unstable.
template<typename _RandomIt,
         typename _Compare>
void
sort(
    _RandomIt _first,
    _RandomIt _last,
    _Compare  _comp
)
{
    typedef typename iterator_traits<_RandomIt>::difference_type _Diff;

    _Diff _len = _last - _first;
    if (_len < 2)
    {
        return;
    }

    // depth limit = 2 * floor(log2(N)); per Musser's recommendation
    _Diff _depth_limit = static_cast<_Diff>(2) * _sort_log2_floor_(_len);

    _sort_introsort_loop_(_first, _last, _depth_limit, _comp);
}


// sort (default operator<)
//   function: sorts [_first, _last) per operator<. Equivalent to
// calling the comparator overload with restd::less<value_type>().
template<typename _RandomIt>
void
sort(
    _RandomIt _first,
    _RandomIt _last
)
{
    typedef typename iterator_traits<_RandomIt>::value_type _Value;
    sort(_first, _last, restd::less<_Value>());
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_SORT_
