/*******************************************************************************
* djinterp [re_std]                                              stable_sort.hpp
*
* stable_sort algorithm header:
*   Sorts the elements of [_first, _last) in non-descending order per
* operator< (or _comp). STABLE: elements that compare equivalent retain
* their original relative order. O(N log^2 N) worst case.
*
*   ALGORITHM:
*   Bottom-up merge sort with in-place no-buffer merge. The bottom level
* uses insertion sort on blocks of 16 (stable). Successive passes double
* the merged block size until the entire range is one sorted block.
*   The merge step is the classical "no-buffer" merge from Knuth, TAOCP
* 5.2.4 (also documented in [Stepanov & McJones, "Elements of
* Programming"] and implemented as libstdc++'s
* __merge_without_buffer):
*     1. If either half is empty, done.
*     2. If both halves have one element each, swap if needed.
*     3. Otherwise: cut the larger half at its midpoint, binary-search
*        the mirror position in the smaller half, rotate the four
*        sub-segments to bring the pivot value into place, recurse on
*        the two halves.
*   The binary searches use lower_bound semantics (for right-side
* elements probed into the left half via upper_bound, and left-side
* elements probed into the right half via lower_bound) to preserve
* stability: equal right-side elements end up AFTER equal left-side
* elements in the merged output.
*
*   COMPLEXITY:
*   The in-place merge is O((n1 + n2) * log(min(n1, n2))) — strictly
* worse than the O(n1 + n2) buffered merge that std::stable_sort uses
* when allocation succeeds. Net: O(N log^2 N) versus the buffered
* O(N log N). re_std accepts the trade for allocator independence.
*
*   PORTABILITY:
*   - std::stable_sort is C++98.
*   - NOT constexpr — std does not lift this through C++26 either.
*   - C++11+ uses move semantics in the insertion-sort base; C++98
*     uses copy.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*
*   INTERNAL HELPERS:
*   Inlines _stable_lower_bound_ and _stable_upper_bound_ here rather
*   than depending on the public binary-search algorithms (shipped in a
*   later batch). Once those land this file should call
*   re_std::lower_bound and re_std::upper_bound directly.
*
*
* path:      /inc/re_std/algorithm/stable_sort.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_STABLE_SORT_HPP
#define RE_STD_ALGORITHM_STABLE_SORT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./rotate.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../functional/less.hpp"
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "../utility/move.hpp"
#endif




namespace re_std
{
//   Opened here 2026-08-25. This file previously began its namespaced
// content without opening re_std, so everything above the first closing
// brace lived at GLOBAL SCOPE, and that brace closed a namespace never
// opened.

// ===========================================================================
// 0.   INTERNAL HELPERS
// ===========================================================================

// _stable_insertion_sort_
//   function: stable insertion sort on [_first, _last). Used at the bottom of
// merge sort. Distinct from sort.hpp's _sort_insertion_ only in that
// the duplication keeps the two algorithm files independent until the
// binary-search batch lands.
template<typename RandomIt,
         typename Compare>
void
_stable_insertion_sort_(
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
            // stability: stop at the first position where the predecessor
            // is NOT greater than _value (i.e., where _value is NOT less).
            // This leaves equal elements in their original order.
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


// _stable_lower_bound_
//   function: returns the first iterator in [_first, _last) at which *iter is
// NOT less than _value (per _comp). I.e., the leftmost insertion point
// that preserves _value's relative order against equivalent elements
// already at or after the returned position.
template<typename RandomIt,
         typename Type,
         typename Compare>
RandomIt
_stable_lower_bound_(
    RandomIt    _first,
    RandomIt    _last,
    const Type& _value,
    Compare     _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    _Diff _len = _last - _first;
    while (_len > 0)
    {
        _Diff     _half = _len / 2;
        RandomIt _mid  = _first + _half;
        if (_comp(*_mid, _value))
        {
            _first  = _mid;
            ++_first;
            _len   -= _half + 1;
        }
        else
        {
            _len = _half;
        }
    }
    return _first;
}


// _stable_upper_bound_
//   function: returns the first iterator in [_first, _last) at which *iter is
// strictly greater than _value (per _comp).
template<typename RandomIt,
         typename Type,
         typename Compare>
RandomIt
_stable_upper_bound_(
    RandomIt    _first,
    RandomIt    _last,
    const Type& _value,
    Compare     _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    _Diff _len = _last - _first;
    while (_len > 0)
    {
        _Diff     _half = _len / 2;
        RandomIt _mid  = _first + _half;
        if (_comp(_value, *_mid))
        {
            _len = _half;
        }
        else
        {
            _first  = _mid;
            ++_first;
            _len   -= _half + 1;
        }
    }
    return _first;
}


// _stable_in_place_merge_
//   function: merges sorted [_first, _middle) with sorted [_middle, _last) into
// sorted [_first, _last) using rotate as the only data-movement
// primitive. Recursive; stack depth O(log N). Stable.
//
// STABILITY: in the recursion, the cut on the left half is found by
// lower_bound on a RIGHT-side element (right element belongs AT the
// first position where it is not greater than equiv left element);
// the cut on the right half is found by upper_bound on a LEFT-side
// element (left element belongs AFTER any equal right elements... no:
// AT the first position where _value is strictly less than the right
// element, i.e., BEFORE any equal right elements). The asymmetric
// choice is what preserves left-before-right ordering of equals.
template<typename RandomIt,
         typename Compare>
void
_stable_in_place_merge_(
    RandomIt _first,
    RandomIt _middle,
    RandomIt _last,
    Compare  _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    // empty halves: nothing to do
    if ( (_first == _middle) ||
         (_middle == _last) )
    {
        return;
    }

    _Diff _len1 = _middle - _first;
    _Diff _len2 = _last   - _middle;

    // base case: 1 + 1 elements — at most one swap needed
    if ((_len1 + _len2) == 2)
    {
        if (_comp(*_middle, *_first))
        {
            iter_swap(_first, _middle);
        }
        return;
    }

    RandomIt _cut1;
    RandomIt _cut2;
    _Diff     _len11;
    _Diff     _len22;

    if (_len1 > _len2)
    {
        // split the larger (left) half at its midpoint; find the
        // mirror position in the right half where *_cut1 belongs
        _len11 = _len1 / 2;
        _cut1  = _first + _len11;
        // *_cut1 is a left-side element; lower_bound gives the first
        // position in the right half not less than *_cut1 — left-side
        // elements equal to *_cut1 stay BEFORE right-side equals
        _cut2  = _stable_lower_bound_(_middle, _last, *_cut1, _comp);
        _len22 = _cut2 - _middle;
    }
    else
    {
        // split the right half at its midpoint; mirror in the left
        _len22 = _len2 / 2;
        _cut2  = _middle + _len22;
        // *_cut2 is a right-side element; upper_bound on the left
        // gives the first position strictly greater than *_cut2 —
        // left-side elements equal to *_cut2 stay BEFORE right-side
        _cut1  = _stable_upper_bound_(_first, _middle, *_cut2, _comp);
        _len11 = _cut1 - _first;
    }

    // rotate the inner segment [_cut1, _cut2): the new boundary
    // between merged halves moves to _first + _len11 + _len22
    rotate(_cut1, _middle, _cut2);
    RandomIt _new_middle = _first + (_len11 + _len22);

    _stable_in_place_merge_(_first,       _cut1, _new_middle, _comp);
    _stable_in_place_merge_(_new_middle,  _cut2, _last,       _comp);
}


// _stable_sort_impl_
//   function: bottom-up driver: insertion-sort 16-element blocks, then merge
// blocks of doubling size until one block covers the whole range.
template<typename RandomIt,
         typename Compare>
void
_stable_sort_impl_(
    RandomIt _first,
    RandomIt _last,
    Compare  _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    // base-block size; same threshold rationale as sort.hpp
    enum { _STABLE_SORT_BLOCK_ = 16 };

    _Diff _n = _last - _first;
    if (_n < 2)
    {
        return;
    }

    // pass 0: insertion-sort each contiguous block
    for (RandomIt _it = _first; _it < _last; )
    {
        RandomIt _block_end = _it + _STABLE_SORT_BLOCK_;
        if (_block_end > _last)
        {
            _block_end = _last;
        }
        _stable_insertion_sort_(_it, _block_end, _comp);
        _it = _block_end;
    }

    // subsequent passes: merge adjacent blocks of size _block into
    // blocks of size 2 * _block
    for (_Diff _block = _STABLE_SORT_BLOCK_;
         _block < _n;
         _block *= 2)
    {
        for (RandomIt _it = _first; _it < _last; )
        {
            RandomIt _mid = _it + _block;
            if (_mid >= _last)
            {
                // tail block too short to need merging this pass
                break;
            }
            RandomIt _end = _mid + _block;
            if (_end > _last)
            {
                _end = _last;
            }
            _stable_in_place_merge_(_it, _mid, _end, _comp);
            _it = _end;
        }
    }
}


}  // re_std


// ===========================================================================
// I.   STABLE_SORT
// ===========================================================================

namespace re_std
{


// stable_sort (comparator)
//   function: stable-sorts [_first, _last) into non-descending order
// per _comp. Equal elements retain their original relative order.
template<typename RandomIt,
         typename Compare>
void
stable_sort(
    RandomIt _first,
    RandomIt _last,
    Compare  _comp
)
{
    _stable_sort_impl_(_first, _last, _comp);
}


// stable_sort (default operator<)
//   function: stable-sorts [_first, _last) per operator<.
template<typename RandomIt>
void
stable_sort(
    RandomIt _first,
    RandomIt _last
)
{
    typedef typename iterator_traits<RandomIt>::value_type _Value;
    stable_sort(_first, _last, re_std::less<_Value>());
}




}  // re_std   (added 2026-08-25 -- was never closed)

#endif  // RE_STD_ALGORITHM_STABLE_SORT_HPP
