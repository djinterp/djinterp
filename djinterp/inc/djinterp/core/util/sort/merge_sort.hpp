/******************************************************************************
* djinterp [utility]                                             heap_sort.hpp
*
*   heap sort algorithm implementation.
* Provides heap_sort() entry-point functions and compile-time traits.
* heap sort: in-place, comparison-based O(n log n) best, average, and worst
* case algorithm. The algorithm progressively builds a heap from the data,
* then repeatedly moves the root element to its final position and restores
* the heap property over the remaining unsorted portion; the sorted subsection
* grows one element at a time until the entire collection is sorted.
*
*   complexity:
*     best:     O(n log n)
*     average:  O(n log n)
*     worst:    O(n log n)   (guaranteed)
*     space:    O(1)         (in-place)
*     stable:   no
*
* 
* path:      /inc/djinterp/core/util/sort/heap_sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_MERGE_
#define DJINTERP_UTILITY_SORT_MERGE_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///                    I.   ALGORITHM TRAITS                                ///
///////////////////////////////////////////////////////////////////////////////

// merge_sort_traits
//   struct: compile-time properties of the merge sort algorithm.
//   - is_stable:   yes  (equal elements preserve their relative order
//                         because the merge step favours the left half)
//   - is_in_place: no   (requires O(n) auxiliary memory)
//   - is_adaptive: no   (always O(n log n) regardless of existing order)
struct merge_sort_traits
{
    static const bool is_stable   = true;
    static const bool is_in_place = false;
    static const bool is_adaptive = false;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  IMPLEMENTATION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // ------------------------------------------------------------------------
    // merge
    // ------------------------------------------------------------------------

    // merge_halves
    //   function: merges two adjacent sorted sub-ranges
    // [_first, _mid) and [_mid, _last) into a single sorted range
    // [_first, _last), using _buffer as temporary storage.
    //
    //   The entire range [_first, _last) is first copied into _buffer.
    // Two read cursors then walk the left and right halves inside the
    // buffer while a write cursor advances through the original range.
    // When elements compare equal, the left-half element is chosen,
    // preserving stability.  When one half is exhausted the remainder
    // of the other half is copied in bulk.
    template<typename _RandomIterator,
             typename _ValueType,
             typename _Comparator,
             typename _DiffType>
    void merge_halves(_RandomIterator  _first,
                      _RandomIterator  _mid,
                      _RandomIterator  _last,
                      _ValueType* _buffer,
                      _Comparator   _comp)
    {
        _DiffType total;
        _DiffType left_size;
        _DiffType right_size;
        _DiffType i;
        _DiffType li;
        _DiffType ri;
        _DiffType wi;

        total      = _last - _first;
        left_size  = _mid  - _first;
        right_size = _last - _mid;

        // copy the full range into the buffer
        for (i = 0; i < total; ++i)
        {
            _buffer[i] = *(_first + i);
        }

        li = 0;                // left-half read cursor  (buffer)
        ri = left_size;        // right-half read cursor  (buffer)
        wi = 0;                // write cursor            (original)

        // merge: interleave from buffer back into original range
        while ( (li < left_size) &&
                (ri < total) )
        {
            // favour left when equal — this preserves stability
            if (_comp(_buffer[ri], _buffer[li]))
            {
                *(_first + wi) = _buffer[ri];
                ++ri;
            }
            else
            {
                *(_first + wi) = _buffer[li];
                ++li;
            }

            ++wi;
        }

        // copy remaining left-half elements (if any)
        while (li < left_size)
        {
            *(_first + wi) = _buffer[li];
            ++li;
            ++wi;
        }

        // copy remaining right-half elements (if any)
        while (ri < total)
        {
            *(_first + wi) = _buffer[ri];
            ++ri;
            ++wi;
        }

        return;
    }

    // ------------------------------------------------------------------------
    // recursive driver
    // ------------------------------------------------------------------------

    // merge_sort_recurse
    //   function: recursively sorts [_first, _last) by splitting,
    // sorting each half, and merging.  The _buffer is shared across
    // all recursion levels to avoid per-level allocation.
    //
    //   Base case: ranges of 0 or 1 element are already sorted.
    //   Recursive case: split at the midpoint, recurse on both halves,
    // then merge.
    template<typename _RandomIterator,
             typename _ValueType,
             typename _Comparator,
             typename _DiffType>
    void merge_sort_recurse(_RandomIterator   _first,
                            _RandomIterator   _last,
                            _ValueType* _buffer,
                            _Comparator    _comp)
    {
        _DiffType size;
        _DiffType half;
        _RandomIterator mid;

        size = _last - _first;

        // base case: 0 or 1 elements
        if (size <= 1)
        {
            return;
        }

        half = size / 2;
        mid  = _first + half;

        // sort the left half
        merge_sort_recurse<_RandomIterator,
                           _ValueType,
                           _Comparator,
                           _DiffType>(_first,
                                      mid,
                                      _buffer,
                                      _comp);

        // sort the right half
        merge_sort_recurse<_RandomIterator,
                           _ValueType,
                           _Comparator,
                           _DiffType>(mid,
                                      _last,
                                      _buffer,
                                      _comp);

        // merge the two sorted halves
        merge_halves<_RandomIterator,
                     _ValueType,
                     _Comparator,
                     _DiffType>(_first,
                                mid,
                                _last,
                                _buffer,
                                _comp);

        return;
    }

    // ------------------------------------------------------------------------
    // top-level apply
    // ------------------------------------------------------------------------

    // merge_sort_apply
    //   function: allocates the auxiliary buffer, invokes the recursive
    // driver, and frees the buffer.  This is the C++11+ variant that
    // deduces value_type and difference_type from iterator_traits.
    template<typename _RandomIterator,
             typename _Comparator>
    void merge_sort_apply(_RandomIterator _first,
                          _RandomIterator _last,
                          _Comparator  _comp)
    {
        typedef typename std::iterator_traits<_RandomIterator>::value_type
            value_type;
        typedef typename std::iterator_traits<_RandomIterator>::difference_type
            diff_type;

        diff_type   size;
        value_type* buffer;

        size = _last - _first;

        // nothing to sort for ranges of 0 or 1 elements
        if (size <= 1)
        {
            return;
        }

        buffer = new value_type[static_cast<size_t>(size)];

        merge_sort_recurse<_RandomIterator,
                           value_type,
                           _Comparator,
                           diff_type>(_first,
                                      _last,
                                      buffer,
                                      _comp);

        delete[] buffer;

        return;
    }

    // merge_sort_apply_98
    //   function: C++98-safe variant that accepts explicit value type
    // and difference type via pointer hints, bypassing
    // std::iterator_traits.
    template<typename _RandomIterator,
             typename _Comparator,
             typename _ValueType,
             typename _DiffType>
    void merge_sort_apply_98(_RandomIterator   _first,
                             _RandomIterator   _last,
                             _Comparator    _comp,
                             _ValueType*,
                             _DiffType*)
    {
        _DiffType   size;
        _ValueType* buffer;

        size = static_cast<_DiffType>(_last - _first);

        // nothing to sort for ranges of 0 or 1 elements
        if (size <= 1)
        {
            return;
        }

        buffer = new _ValueType[static_cast<size_t>(size)];

        merge_sort_recurse<_RandomIterator,
                           _ValueType,
                           _Comparator,
                           _DiffType>(_first,
                                      _last,
                                      buffer,
                                      _comp);

        delete[] buffer;

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    III. ENTRY POINTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// A.  merge_sort(first, last, comp)
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// merge_sort
//   function: sorts the range [_first, _last) using merge sort with the
// comparator _comp.
template<typename _RandomIterator,
         typename _Comparator>
void merge_sort(_RandomIterator _first,
                _RandomIterator _last,
                _Comparator  _comp)
{
    internal::merge_sort_apply(_first,
                               _last,
                               _comp);

    return;
}

#else  // C++98

// merge_sort
//   function: sorts the range [_first, _last) using merge sort with the
// comparator _comp.  The _value_type_hint and _diff_type_hint parameters
// are used only for type deduction — pass null pointers of the
// appropriate types.
//   e.g.  merge_sort(v, v + n, my_comp, (int*)0, (ptrdiff_t*)0);
template<typename _RandomIterator,
         typename _Comparator,
         typename _ValueType,
         typename _DiffType>
void merge_sort(_RandomIterator   _first,
                _RandomIterator   _last,
                _Comparator    _comp,
                _ValueType* _value_type_hint,
                _DiffType*  _diff_type_hint)
{
    internal::merge_sort_apply_98(_first,
                                  _last,
                                  _comp,
                                  _value_type_hint,
                                  _diff_type_hint);

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// B.  merge_sort(first, last)      (C++11+)
//     Uses operator< via less<value_type>.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// merge_sort
//   function: sorts the range [_first, _last) using merge sort with the
// default ascending comparator.
template<typename _RandomIterator>
void merge_sort(_RandomIterator _first,
                _RandomIterator _last)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    internal::merge_sort_apply(_first,
                               _last,
                               less<value_type>());

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// C.  merge_sort_ordered(first, last, comp, order)
//     Wraps the comparator to honour sort_order at runtime.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// merge_sort_ordered
//   function: sorts the range [_first, _last) using merge sort, adapting
// _comp to the requested _order.
template<typename _RandomIterator,
         typename _Comparator>
void merge_sort_ordered(_RandomIterator  _first,
                        _RandomIterator  _last,
                        _Comparator   _comp,
                        sort_order _order)
{
    internal::order_comparator<_Comparator> wrapped(_comp, _order);

    internal::merge_sort_apply(_first,
                               _last,
                               wrapped);

    return;
}

#else  // C++98

// merge_sort_ordered
//   function: sorts the range [_first, _last) using merge sort, adapting
// _comp to the requested _order.  The _value_type_hint and
// _diff_type_hint parameters are used only for type deduction.
template<typename _RandomIterator,
         typename _Comparator,
         typename _ValueType,
         typename _DiffType>
void merge_sort_ordered(_RandomIterator   _first,
                        _RandomIterator   _last,
                        _Comparator    _comp,
                        sort_order  _order,
                        _ValueType* _value_type_hint,
                        _DiffType*  _diff_type_hint)
{
    internal::order_comparator<_Comparator> wrapped(_comp, _order);

    internal::merge_sort_apply_98(_first,
                                  _last,
                                  wrapped,
                                  _value_type_hint,
                                  _diff_type_hint);

    return;
}

#endif  // C++11


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_MERGE_
