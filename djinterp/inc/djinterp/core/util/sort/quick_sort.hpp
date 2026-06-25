/******************************************************************************
* djinterp [container]                                          quick_sort.hpp
*
*   Quicksort algorithm implementation.
* Provides quick_sort() entry-point functions and compile-time traits.
* Quicksort is a comparison-based, divide-and-conquer algorithm that
* partitions the range around a pivot element, then recursively sorts
* the two resulting sub-ranges.
*
*   This implementation includes three standard optimisations:
*     1. Median-of-three pivot selection — the pivot is chosen as the
*        median of the first, middle, and last elements.  This avoids
*        the O(n^2) worst case on already-sorted or reverse-sorted
*        input that naive first-element pivoting suffers from.
*     2. Hoare partition scheme — two iterators converge from opposite
*        ends, producing fewer swaps on average than the Lomuto scheme.
*     3. Tail-call elimination — the recursive call is always made on
*        the smaller partition; the larger partition is handled by
*        looping.  This keeps the call stack depth at O(log n) even
*        when the partitions are unbalanced.
*
*   Sub-ranges of D_QUICK_SORT_INSERTION_THRESHOLD or fewer elements
* are finished with insertion sort, whose low overhead and adaptive
* behaviour make it faster than quicksort on small inputs.
*
*   Complexity:
*     best     O(n log n)
*     average  O(n log n)
*     worst    O(n^2)       (extremely unlikely with median-of-three)
*     space    O(log n)     (in-place; stack depth from recursion)
*     stable   no
*
* 
* path:      /inc/djinterp/core/util/sort/selection_sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_QUICK_
#define DJINTERP_UTILITY_SORT_QUICK_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"
#include "./insertion_sort.hpp"


NS_DJINTERP

// D_QUICK_SORT_INSERTION_THRESHOLD
//   constant: sub-ranges with this many elements or fewer are sorted
// with insertion sort instead of continuing to recurse.  16 is a common
// empirical sweet-spot that balances call overhead against insertion
// sort's quadratic growth.
#ifndef D_QUICK_SORT_INSERTION_THRESHOLD
    #define D_QUICK_SORT_INSERTION_THRESHOLD 16
#endif



///////////////////////////////////////////////////////////////////////////////
///                    III. IMPLEMENTATION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // --------------------------------------------------------------------
    // A.  Median-of-three pivot selection
    // --------------------------------------------------------------------

    // quick_sort_median_of_three
    //   function: rearranges the elements at _first, _mid, and _end so
    // that the median of the three resides at *_first (ready for use as
    // the partition pivot).
    //
    //   After the three conditional swaps:
    //     *_first <= *_mid <= *_end
    // The median (*_mid) is then swapped to *_first.
    template<typename _RandomIterator,
             typename _Comparator>
    void quick_sort_median_of_three(_RandomIterator _first,
                                    _RandomIterator _mid,
                                    _RandomIterator _end,
                                    _Comparator  _comp)
    {
        // sort the triple in place: *_first <= *_mid <= *_end
        if (_comp(*_mid, *_first))
        {
            std::iter_swap(_first, _mid);
        }

        if (_comp(*_end, *_first))
        {
            std::iter_swap(_first, _end);
        }

        if (_comp(*_end, *_mid))
        {
            std::iter_swap(_mid, _end);
        }

        // move the median to _first where the partitioner expects
        // the pivot
        std::iter_swap(_first, _mid);

        return;
    }

    // --------------------------------------------------------------------
    // B.  Hoare partition
    // --------------------------------------------------------------------

    // quick_sort_partition
    //   function: partitions [_first, _last) around the pivot at *_first
    // using a Hoare-style two-pointer convergence.
    //
    //   On return, the pivot is in its final sorted position and the
    // returned iterator points to it.  Every element in [_first, pivot)
    // compares less than or equal to the pivot, and every element in
    // (pivot, _last) compares greater than or equal.
    //
    //   The caller is responsible for placing the pivot value at *_first
    // before calling this function (typically via median_of_three).
    template<typename _RandomIterator,
             typename _Comparator>
    _RandomIterator quick_sort_partition(_RandomIterator _first,
                                   _RandomIterator _last,
                                   _Comparator  _comp)
    {
        _RandomIterator lo;
        _RandomIterator hi;

        lo = _first + 1;
        hi = _last  - 1;

        // converge from both ends
        for (;;)
        {
            // advance lo past elements that belong in the left
            // partition (strictly less than pivot)
            while ( (lo <= hi) &&
                    (_comp(*lo, *_first)) )
            {
                ++lo;
            }

            // retreat hi past elements that belong in the right
            // partition (strictly greater than pivot)
            while ( (lo <= hi) &&
                    (_comp(*_first, *hi)) )
            {
                --hi;
            }

            // pointers have crossed — partition is complete
            if (lo > hi)
            {
                break;
            }

            std::iter_swap(lo, hi);
            ++lo;
            --hi;
        }

        // place the pivot in its final position
        std::iter_swap(_first, hi);

        return hi;
    }

    // --------------------------------------------------------------------
    // C.  Recursive driver
    // --------------------------------------------------------------------

    // quick_sort_apply
    //   function: sorts [_first, _last) using quicksort with median-of-
    // three pivot selection and insertion-sort fallback for small ranges.
    //
    //   Tail-call elimination is achieved by always recursing into the
    // smaller partition and looping back for the larger one.  This
    // guarantees O(log n) maximum recursion depth regardless of how
    // balanced the partitions are.
    template<typename _RandomIterator,
             typename _Comparator>
    void quick_sort_apply(_RandomIterator _first,
                          _RandomIterator _last,
                          _Comparator  _comp)
    {
        _RandomIterator mid;
        _RandomIterator pivot;

        // tail-call elimination loop — _first and _last are narrowed
        // on each iteration to cover only the larger partition
        while ((_last - _first) > D_QUICK_SORT_INSERTION_THRESHOLD)
        {
            // select median-of-three pivot and place it at _first
            mid = _first + ((_last - _first) / 2);
            quick_sort_median_of_three(_first,
                                       mid,
                                       _last - 1,
                                       _comp);

            // partition around the pivot
            pivot = quick_sort_partition(_first,
                                         _last,
                                         _comp);

            // recurse on the smaller half, loop on the larger
            if ((pivot - _first) < (_last - (pivot + 1)))
            {
                // left partition is smaller — recurse left
                quick_sort_apply(_first,
                                 pivot,
                                 _comp);
                _first = pivot + 1;
            }
            else
            {
                // right partition is smaller — recurse right
                quick_sort_apply(pivot + 1,
                                 _last,
                                 _comp);
                _last = pivot;
            }
        }

        // finish the small remaining range with insertion sort
        insertion_sort_apply(_first,
                             _last,
                             _comp);

        return;
    }

    // quick_sort_apply_98
    //   function: C++98-safe variant that accepts an explicit value type
    // parameter, bypassing std::iterator_traits.
    template<typename _RandomIterator,
             typename _Comparator,
             typename _ValueType>
    void quick_sort_apply_98(_RandomIterator  _first,
                             _RandomIterator  _last,
                             _Comparator   _comp,
                             _ValueType*)
    {
        _RandomIterator mid;
        _RandomIterator pivot;

        while ((_last - _first) > D_QUICK_SORT_INSERTION_THRESHOLD)
        {
            mid = _first + ((_last - _first) / 2);
            quick_sort_median_of_three(_first,
                                       mid,
                                       _last - 1,
                                       _comp);

            pivot = quick_sort_partition(_first,
                                         _last,
                                         _comp);

            if ((pivot - _first) < (_last - (pivot + 1)))
            {
                quick_sort_apply_98(_first,
                                    pivot,
                                    _comp,
                                    static_cast<_ValueType*>(0));
                _first = pivot + 1;
            }
            else
            {
                quick_sort_apply_98(pivot + 1,
                                    _last,
                                    _comp,
                                    static_cast<_ValueType*>(0));
                _last = pivot;
            }
        }

        // finish the small remaining range with insertion sort
        insertion_sort_apply_98(_first,
                                _last,
                                _comp,
                                static_cast<_ValueType*>(0));

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    IV.  ENTRY POINTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// A.  quick_sort(first, last, comp)
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// quick_sort
//   function: sorts the range [_first, _last) using quicksort with the
// comparator _comp.
template<typename _RandomIterator,
         typename _Comparator>
void quick_sort(_RandomIterator _first,
                _RandomIterator _last,
                _Comparator  _comp)
{
    // guard: ranges of 0 or 1 elements are trivially sorted
    if ((_last - _first) <= 1)
    {
        return;
    }

    internal::quick_sort_apply(_first,
                                _last,
                                _comp);

    return;
}

#else  // C++98

// quick_sort
//   function: sorts the range [_first, _last) using quicksort with the
// comparator _comp.  The _value_type_hint parameter is used only for
// type deduction — pass a null pointer of the element type.
//   e.g.  quick_sort(v, v + n, my_comp, (int*)0);
template<typename _RandomIterator,
         typename _Comparator,
         typename _ValueType>
void quick_sort(_RandomIterator   _first,
                _RandomIterator   _last,
                _Comparator    _comp,
                _ValueType* _value_type_hint)
{
    if ((_last - _first) <= 1)
    {
        return;
    }

    internal::quick_sort_apply_98(_first,
                                   _last,
                                   _comp,
                                   _value_type_hint);

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// B.  quick_sort(first, last)      (C++11+)
//     Uses operator< via less<value_type>.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// quick_sort
//   function: sorts the range [_first, _last) using quicksort with the
// default ascending comparator.
template<typename _RandomIterator>
void quick_sort(_RandomIterator _first,
                _RandomIterator _last)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    if ((_last - _first) <= 1)
    {
        return;
    }

    internal::quick_sort_apply(_first,
                                _last,
                                less<value_type>());

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// C.  quick_sort_ordered(first, last, comp, order)
//     Wraps the comparator to honour sort_order at runtime.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// quick_sort_ordered
//   function: sorts the range [_first, _last) using quicksort, adapting
// _comp to the requested _order.
template<typename _RandomIterator,
         typename _Comparator>
void quick_sort_ordered(_RandomIterator  _first,
                        _RandomIterator  _last,
                        _Comparator   _comp,
                        sort_order _order)
{
    if ((_last - _first) <= 1)
    {
        return;
    }

    internal::order_comparator<_Comparator> wrapped(_comp, _order);

    internal::quick_sort_apply(_first,
                                _last,
                                wrapped);

    return;
}

#else  // C++98

// quick_sort_ordered
//   function: sorts the range [_first, _last) using quicksort, adapting
// _comp to the requested _order.  The _value_type_hint parameter is used
// only for type deduction.
template<typename _RandomIterator,
         typename _Comparator,
         typename _ValueType>
void quick_sort_ordered(_RandomIterator   _first,
                        _RandomIterator   _last,
                        _Comparator    _comp,
                        sort_order  _order,
                        _ValueType* _value_type_hint)
{
    if ((_last - _first) <= 1)
    {
        return;
    }

    internal::order_comparator<_Comparator> wrapped(_comp, _order);

    internal::quick_sort_apply_98(_first,
                                   _last,
                                   wrapped,
                                   _value_type_hint);

    return;
}

#endif  // C++11


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_QUICK_
