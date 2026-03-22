/******************************************************************************
* djinterp [utility]                                        selection_sort.hpp
*
*   selection sort algorithm implementation.
* Provides selection_sort() entry-point functions and compile-time traits.
* selection sort: in-place, iterative, O^2 best, average, and worst case 
* algorithm. The algorithm progressively finds the next smallest/largest 
* element (depending on order) and swaps it with the first available unsorted
* position; the array in effect grows the sorted portion of the data one 
* element at a time, until the sorted subsection constitutes the entire 
* collection.
*
*   complexity:
*     best:     O(n^2)      (always performs n*(n-1)/2 comparisons)
*     average:  O(n^2)
*     worst:    O(n^2)
*     swaps:    O(n)        (at most n-1 swaps — minimal data movement)
*     space:    O(1)        (in-place)
*     stable:   no
*
* 
* path:      /inc/core/util/sort/selection_sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_SELECTION_
#define DJINTERP_UTILITY_SORT_SELECTION_ 1

#include "../../djinterp.hpp"
#include "./sort.hpp"


NS_SORT

///////////////////////////////////////////////////////////////////////////////
///                    II.  IMPLEMENTATION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // selection_sort_apply
    //   function: performs selection sort on [_first, _last).
    //
    //   The outer iterator _current walks from _first to one-before-_last,
    // representing the boundary between the sorted prefix and unsorted
    // suffix.  On each pass, the inner loop scans the unsorted suffix for
    // the minimum element (according to _comp), and that element is
    // swapped into the _current position.
    //
    //   Because the minimum is located before any swap occurs, selection
    // sort performs at most (n - 1) swaps total — far fewer than bubble
    // sort or insertion sort on average — making it well-suited to
    // situations where swap cost dominates comparison cost.
    template<typename _RandomIterator,
             typename _Comparator>
    void selection_sort_apply(_RandomIterator _first,
                              _RandomIterator _last,
                              _Comparator     _comparator)
    {
        // nothing to sort for empty or single-element ranges
        if (_first == _last)
        {
            return;
        }

        _RandomIterator current;
        _RandomIterator scan;
        _RandomIterator min_pos;

        // outer: advance the sorted/unsorted boundary one position at
        // a time
        for (current = _first; current != _last; ++current)
        {
            min_pos = current;

            scan = current;
            ++scan;

            // inner: find the minimum element in the unsorted suffix
            for (; scan != _last; ++scan)
            {
                if (_comparator(*scan, *min_pos))
                {
                    min_pos = scan;
                }
            }

            // place the minimum at the boundary (skip self-swap)
            if (min_pos != current)
            {
                std::iter_swap(current, min_pos);
            }
        }

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    III. ENTRY POINTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// A.  selection_sort(first, last, comp)
// ----------------------------------------------------------------------------

// selection_sort
//   function: sorts the range [_first, _last) using selection sort with
// the comparator _comp.
template<typename _RandomIterator,
         typename _Comparator>
void selection_sort(_RandomIterator _first,
                    _RandomIterator _last,
                    _Comparator     _comparator)
{
    internal::selection_sort_apply(_first,
                                   _last,
                                   _comparator);

    return;
}

// ----------------------------------------------------------------------------
// B.  selection_sort(first, last)      (C++11+)
//     Uses operator< via less<value_type>.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// selection_sort
//   function: sorts the range [_first, _last) using selection sort with
// the default ascending comparator.
template<typename _RandomIterator>
void selection_sort(_RandomIterator _first,
                    _RandomIterator _last)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    internal::selection_sort_apply(_first,
                                   _last,
                                   less<value_type>());

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// C.  selection_sort_ordered(first, last, comp, order)
//     Wraps the comparator to honour sort_order at runtime.
// ----------------------------------------------------------------------------

// selection_sort_ordered
//   function: sorts the range [_first, _last) using selection sort,
// adapting _comp to the requested _order.
template<typename _RandomIterator,
         typename _Comparator>
void selection_sort_ordered(_RandomIterator _first,
                            _RandomIterator _last,
                            _Comparator     _comparator,
                            bool            _ascending)
{
    internal::order_comparator<_Comparator> wrapped(_comparator, _ascending);

    internal::selection_sort_apply(_first,
                                   _last,
                                   wrapped);

    return;
}


NS_END  // namespace sort


#endif  // DJINTERP_UTILITY_SORT_SELECTION_
