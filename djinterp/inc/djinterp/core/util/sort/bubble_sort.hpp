/******************************************************************************
* djinterp [utility]                                           bubble_sort.hpp
*
* bubble sort algorithm implementation.
*   Provides bubble_sort() entry-point functions and compile-time traits.
* bubble sort: in-place, iterative, stable, comparison-based O(n) best case,
* O(n^2) average and worst case algorithm. The algorithm progressively walks
* adjacent element pairs and swaps out-of-order values; each pass bubbles the
* next largest/smallest element (depending on order) toward its final position,
* and the sorted portion of the data grows one element at a time until the
* sorted subsection constitutes the entire collection.
*
*   complexity:
*     best:     O(n)        (already sorted — single pass, adaptive)
*     average:  O(n^2)
*     worst:    O(n^2)
*     space:    O(1)        (in-place)
*     stable:   yes
*
*
* path:      /inc/djinterp/core/util/sort/bubble_sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_BUBBLE_
#define DJINTERP_UTILITY_SORT_BUBBLE_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"


NS_DJINTERP

// implementation
NS_INTERNAL

    // bubble_sort_apply
    //   function: performs an optimised bubble sort on [_first, _last).
    //
    //   The outer loop shrinks the unsorted region from the right.  On
    // each pass the inner loop walks from _first up to the current
    // unsorted boundary, bubbling the largest unsorted element into its
    // final position.  The position of the last swap is recorded so that
    // elements known to be in order are never re-examined.
    //
    //   If a complete pass produces no swaps the range is sorted and the
    // function returns immediately, giving O(n) best-case performance on
    // already-sorted or nearly-sorted input.
    template<typename _RandomIterator,
             typename _Comparator>
    void bubble_sort_apply(_RandomIterator _first,
                           _RandomIterator _last,
                           _Comparator     _comparator)
    {
        // nothing to sort for empty or single-element ranges
        if (_first == _last)
        {
            return;
        }

        _RandomIterator unsorted_end;
        _RandomIterator new_end;
        _RandomIterator current;
        _RandomIterator next;
        bool            swapped;

        unsorted_end = _last;

        // outer: repeat until no swaps occur or range is exhausted
        for (;;)
        {
            swapped = false;
            new_end = _first;
            current = _first;
            next    = _first;
            ++next;

            // inner: walk the unsorted region, bubbling out-of-order
            // pairs
            for (; next != unsorted_end; ++current, ++next)
            {
                // swap if the pair is in the wrong order according
                // to the comparator
                if (_comparator(*next, *current))
                {
                    std::iter_swap(current, next);
                    swapped = true;
                    new_end = next;
                }
            }

            // early exit: no swaps means the range is sorted
            if (!swapped)
            {
                break;
            }

            // shrink: everything after the last swap is already in
            // its final position
            unsorted_end = new_end;

            // single element left means we are done
            if (_first == unsorted_end)
            {
                break;
            }
        }

        return;
    }

NS_END  // internal


// bubble_sort(first, last, comp)

// bubble_sort
//   function: sorts the range [_first, _last) using bubble sort with
// the comparator _comparator.
template<typename _RandomIterator,
         typename _Comparator>
void bubble_sort(_RandomIterator _first,
                 _RandomIterator _last,
                 _Comparator     _comparator)
{
    internal::bubble_sort_apply(_first,
                                _last,
                                _comparator);

    return;
}

// bubble_sort(first, last)      (C++11+)
//    Uses operator< via less<value_type>.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// bubble_sort
//   function: sorts the range [_first, _last) using bubble sort with
// the default ascending comparator.
template<typename _RandomIterator>
void bubble_sort(_RandomIterator _first,
                 _RandomIterator _last)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    internal::bubble_sort_apply(_first,
                                _last,
                                less<value_type>());

    return;
}

#endif  // C++11

// bubble_sort_ordered
//   function: sorts the range [_first, _last) using bubble sort,
// adapting _comparator to the requested _order.
//     Wraps the comparator to honour `_ascending` at runtime.
template<typename _RandomIterator,
         typename _Comparator>
void bubble_sort_ordered(_RandomIterator _first,
                         _RandomIterator _last,
                         _Comparator     _comparator,
                         bool            _ascending)
{
    internal::order_comparator<_Comparator> wrapped(_comparator, _ascending);

    internal::bubble_sort_apply(_first,
                                _last,
                                wrapped);

    return;
}

NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_BUBBLE_
