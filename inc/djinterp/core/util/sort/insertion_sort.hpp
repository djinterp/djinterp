/******************************************************************************
* djinterp [utility]                                        insertion_sort.hpp
*
*   insertion sort algorithm implementation.
* Provides insertion_sort() entry-point functions and compile-time traits.
* insertion sort: in-place, iterative, stable, comparison-based O(n) best case,
* O(n^2) average and worst case algorithm. The algorithm progressively takes
* the next unsorted element and shifts larger elements in the sorted portion
* rightward until the correct insertion position is found; the array in effect
* grows the sorted portion of the data one element at a time, until the sorted
* subsection constitutes the entire collection.
*
*   complexity:
*     best:     O(n)        (already sorted — one comparison per element)
*     average:  O(n^2)
*     worst:    O(n^2)
*     space:    O(1)        (in-place)
*     stable:   yes
*
*
* path:      /inc/core/util/sort/insertion_sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_INSERTION_
#define DJINTERP_UTILITY_SORT_INSERTION_ 1

#include "../../djinterp.hpp"
#include "./sort.hpp"

NS_SORT

// implementation
NS_INTERNAL

    // insertion_sort_apply
    //   function: performs insertion sort on [_first, _last).
    //
    //   The outer iterator _current starts at (_first + 1) and advances
    // to _last.  At each step the value at _current is saved into a
    // temporary, and the inner loop shifts sorted elements rightward
    // until the correct insertion position is found.  The temporary is
    // then written into the vacated slot.
    //
    //   The shift-based approach avoids per-element swaps, giving a
    // lower constant factor than a swap-based variant while preserving
    // stability.  On already-sorted input the inner loop body never
    // executes, yielding O(n) best-case performance.
    template<typename _RandomIterator,
             typename _Comparator>
    void insertion_sort_apply(_RandomIterator _first,
                              _RandomIterator _last,
                              _Comparator     _comparator)
    {
        // nothing to sort for empty or single-element ranges
        if (_first == _last)
        {
            return;
        }

        _RandomIterator current;
        _RandomIterator hole;
        _RandomIterator prev;

        current = _first;
        ++current;

        // outer: extend the sorted prefix one element at a time
        for (; current != _last; ++current)
        {
            // skip elements that are already in position — this is
            // the fast path that gives O(n) on sorted input
            if (!_comparator(*current, *(current - 1)))
            {
                continue;
            }

            // save the element to be inserted
            typename std::iterator_traits<_RandomIterator>::value_type
                key = *current;

            hole = current;

            // inner: shift sorted elements rightward until we find
            // the insertion point for key
            while (hole != _first)
            {
                prev = hole;
                --prev;

                // stop shifting once we find an element that
                // belongs before key
                if (!_comparator(key, *prev))
                {
                    break;
                }

                *hole = *prev;
                hole  = prev;
            }

            // place key into the vacated slot
            *hole = key;
        }

        return;
    }

    // insertion_sort_apply  (C++98 overload)
    //   function: C++98-safe variant that accepts an explicit value type
    // parameter, bypassing std::iterator_traits.
    template<typename _RandomIterator,
             typename _Comparator,
             typename _ValueType>
    void insertion_sort_apply_98(_RandomIterator _first,
                                _RandomIterator  _last,
                                _Comparator      _comparator,
                                _ValueType*)
    {
        // nothing to sort for empty or single-element ranges
        if (_first == _last)
        {
            return;
        }

        _RandomIterator current;
        _RandomIterator hole;
        _RandomIterator prev;

        current = _first;
        ++current;

        // outer: extend the sorted prefix one element at a time
        for (; current != _last; ++current)
        {
            // skip elements that are already in position
            if (!_comparator(*current, *(current - 1)))
            {
                continue;
            }

            // save the element to be inserted
            _ValueType key = *current;

            hole = current;

            // inner: shift sorted elements rightward
            while (hole != _first)
            {
                prev = hole;
                --prev;

                if (!_comparator(key, *prev))
                {
                    break;
                }

                *hole = *prev;
                hole  = prev;
            }

            // place key into the vacated slot
            *hole = key;
        }

        return;
    }

NS_END  // internal


// insertion_sort(first, last, comparator)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// insertion_sort
//   function: sorts the range [_first, _last) using insertion sort with
// the comparator _comparator.
template<typename _RandomIterator,
         typename _Comparator>
void insertion_sort(_RandomIterator _first,
                    _RandomIterator _last,
                    _Comparator  _comparator)
{
    internal::insertion_sort_apply(_first,
                                   _last,
                                   _comparator);

    return;
}

#else  // C++98

// insertion_sort
//   function: sorts the range [_first, _last) using insertion sort with
// the comparator _comparator.  The _value_type_hint parameter is used only for
// type deduction — pass a null pointer of the element type.
//   e.g.  insertion_sort(v, v + n, my_comp, (int*)0);
template<typename _RandomIterator,
         typename _Comparator,
         typename _ValueType>
void insertion_sort(_RandomIterator _first,
                    _RandomIterator _last,
                    _Comparator     _comparator,
                    _ValueType*     _value_type_hint)
{
    internal::insertion_sort_apply_98(_first,
                                      _last,
                                      _comparator,
                                      _value_type_hint);

    return;
}

#endif  // C++11

//  insertion_sort(first, last)      (C++11+)
//    uses operator< via less<value_type>.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// insertion_sort
//   function: sorts the range [_first, _last) using insertion sort with
// the default ascending comparator.
template<typename _RandomIterator>
void insertion_sort(_RandomIterator _first,
                    _RandomIterator _last)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    internal::insertion_sort_apply(_first,
                                   _last,
                                   less<value_type>());

    return;
}

#endif  // C++11

// insertion_sort_ordered(first, last, comp, order)
//    wraps the comparator to honour `_ascending` at runtime.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// insertion_sort_ordered
//   function: sorts the range [_first, _last) using insertion sort,
// adapting _comparator to the requested _order.
template<typename _RandomIterator,
         typename _Comparator>
void insertion_sort_ordered(_RandomIterator _first,
                            _RandomIterator _last,
                            _Comparator     _comparator,
                            bool            _ascending)
{
    internal::order_comparator<_Comparator> wrapped(_comparator, _ascending);

    internal::insertion_sort_apply(_first,
                                   _last,
                                   wrapped);

    return;
}

#else  // C++98

// insertion_sort_ordered
//   function: sorts the range [_first, _last) using insertion sort,
// adapting _comparator to the requested _order.  The _value_type_hint parameter
// is used only for type deduction.
template<typename _RandomIterator,
         typename _Comparator,
         typename _ValueType>
void insertion_sort_ordered(_RandomIterator _first,
                            _RandomIterator _last,
                            _Comparator     _comparator,
                            bool            _ascending,
                            _ValueType*     _value_type_hint)
{
    internal::order_comparator<_Comparator> wrapped(_comparator, _ascending);

    internal::insertion_sort_apply_98(_first,
                                      _last,
                                      wrapped,
                                      _value_type_hint);

    return;
}

#endif  // C++11


NS_END  // namespace sort


#endif  // DJINTERP_UTILITY_SORT_INSERTION_
