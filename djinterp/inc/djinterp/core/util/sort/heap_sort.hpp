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
*     worst:    O(n log n)  (guaranteed)
*     space:    O(1)        (in-place)
*     stable:   no
*
* 
* path:      /inc/djinterp/core/util/sort/heap_sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_HEAP_
#define DJINTERP_UTILITY_SORT_HEAP_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///                    I.   ALGORITHM TRAITS                                ///
///////////////////////////////////////////////////////////////////////////////

// heap_sort_traits
//   struct: compile-time properties of the heap sort algorithm.
//   - is_stable:   no   (root-to-end swap disrupts equal-element ordering)
//   - is_in_place: yes  (O(1) auxiliary memory)
//   - is_adaptive: no   (always O(n log n) regardless of existing order)
struct heap_sort_traits
{
    static const bool is_stable   = false;
    static const bool is_in_place = true;
    static const bool is_adaptive = false;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  IMPLEMENTATION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // heap_parent
    //   function: returns the parent index of the node at _index in a
    // zero-based implicit binary heap.
    template<typename _DiffType>
    _DiffType heap_parent(_DiffType _index)
    {
        return ((_index - 1) / 2);
    }

    // heap_left_child
    //   function: returns the left-child index of the node at _index
    // in a zero-based implicit binary heap.
    template<typename _DiffType>
    _DiffType heap_left_child(_DiffType _index)
    {
        return ((2 * _index) + 1);
    }

    // ------------------------------------------------------------------------
    // sift_down
    // ------------------------------------------------------------------------

    // heap_sift_down
    //   function: restores the max-heap invariant for the sub-tree
    // rooted at _root within the range [_first, _first + _size).
    //
    //   Starting at _root, the function compares the node with its
    // children.  If the largest child is greater (per _comp) than the
    // node, they are swapped and the process repeats at the child
    // position.  The loop terminates when the node is at least as large
    // as both children or when a leaf is reached.
    //
    //   All index arithmetic is performed on the difference type of the
    // iterator, and elements are accessed via (_first + index) to stay
    // within random-access iterator semantics.
    template<typename _RandomIterator,
             typename _Comparator,
             typename _DiffType>
    void heap_sift_down(_RandomIterator _first,
                        _DiffType _size,
                        _DiffType _root,
                        _Comparator  _comp)
    {
        _DiffType current;
        _DiffType left;
        _DiffType largest;

        current = _root;

        // walk down the tree until we reach a leaf
        for (;;)
        {
            left = heap_left_child(current);

            // no children — current is a leaf
            if (left >= _size)
            {
                break;
            }

            largest = current;

            // compare with left child
            if (_comp(*(_first + largest), *(_first + left)))
            {
                largest = left;
            }

            // compare with right child (if it exists)
            if ( ((left + 1) < _size) &&
                 (_comp(*(_first + largest), *(_first + (left + 1)))) )
            {
                largest = left + 1;
            }

            // heap property satisfied — done
            if (largest == current)
            {
                break;
            }

            std::iter_swap(_first + current, _first + largest);
            current = largest;
        }

        return;
    }

    // ------------------------------------------------------------------------
    // heap_sort_apply
    // ------------------------------------------------------------------------

    // heap_sort_apply
    //   function: performs heap sort on [_first, _last).
    //
    //   Phase 1 — heapify:  builds a max-heap by calling sift_down on
    // every internal node from the last parent down to the root.  The
    // bottom-up ordering guarantees that each sub-tree is already a
    // valid heap before its root is sifted, so the overall cost is O(n).
    //
    //   Phase 2 — sort:  repeatedly swaps the root (the current
    // maximum) with the last element of the unsorted region, shrinks
    // the heap by one, and sifts the new root down.  After all
    // extractions the range is sorted in ascending order (relative to
    // _comp).
    template<typename _RandomIterator,
             typename _Comparator>
    void heap_sort_apply(_RandomIterator _first,
                         _RandomIterator _last,
                         _Comparator  _comp)
    {
        typedef typename std::iterator_traits<_RandomIterator>::difference_type
            diff_type;

        diff_type size;
        diff_type i;

        size = _last - _first;

        // nothing to sort for ranges of 0 or 1 elements
        if (size <= 1)
        {
            return;
        }

        // phase 1: build the max-heap (bottom-up)
        //
        // start from the parent of the last element and work backward
        // to the root
        i = heap_parent(size - 1);

        for (;;)
        {
            heap_sift_down(_first, size, i, _comp);

            // break after processing the root (index 0)
            if (i == 0)
            {
                break;
            }

            --i;
        }

        // phase 2: extract elements from the heap
        //
        // swap the root (maximum) to the end, shrink the heap, and
        // restore the heap property on the new root
        i = size - 1;

        while (i > 0)
        {
            std::iter_swap(_first, _first + i);
            heap_sift_down(_first, i, static_cast<diff_type>(0), _comp);
            --i;
        }

        return;
    }

    // heap_sort_apply_98
    //   function: C++98-safe variant that accepts an explicit difference
    // type, bypassing std::iterator_traits.
    template<typename _RandomIterator,
             typename _Comparator,
             typename _DiffType>
    void heap_sort_apply_98(_RandomIterator _first,
                            _RandomIterator _last,
                            _Comparator  _comp,
                            _DiffType*)
    {
        _DiffType size;
        _DiffType i;

        size = static_cast<_DiffType>(_last - _first);

        // nothing to sort for ranges of 0 or 1 elements
        if (size <= 1)
        {
            return;
        }

        // phase 1: build the max-heap (bottom-up)
        i = heap_parent(size - 1);

        for (;;)
        {
            heap_sift_down(_first, size, i, _comp);

            if (i == 0)
            {
                break;
            }

            --i;
        }

        // phase 2: extract elements from the heap
        i = size - 1;

        while (i > 0)
        {
            std::iter_swap(_first, _first + i);
            heap_sift_down(_first,
                           i,
                           static_cast<_DiffType>(0),
                           _comp);
            --i;
        }

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    III. ENTRY POINTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// A.  heap_sort(first, last, comp)
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// heap_sort
//   function: sorts the range [_first, _last) using heap sort with the
// comparator _comp.
template<typename _RandomIterator,
         typename _Comparator>
void heap_sort(_RandomIterator _first,
               _RandomIterator _last,
               _Comparator  _comp)
{
    internal::heap_sort_apply(_first,
                              _last,
                              _comp);

    return;
}

#else  // C++98

// heap_sort
//   function: sorts the range [_first, _last) using heap sort with the
// comparator _comp.  The _diff_type_hint parameter is used only for type
// deduction — pass a null pointer of the difference type (typically
// ptrdiff_t).
//   e.g.  heap_sort(v, v + n, my_comp, (ptrdiff_t*)0);
template<typename _RandomIterator,
         typename _Comparator,
         typename _DiffType>
void heap_sort(_RandomIterator  _first,
               _RandomIterator  _last,
               _Comparator   _comp,
               _DiffType* _diff_type_hint)
{
    internal::heap_sort_apply_98(_first,
                                 _last,
                                 _comp,
                                 _diff_type_hint);

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// B.  heap_sort(first, last)      (C++11+)
//     Uses operator< via less<value_type>.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// heap_sort
//   function: sorts the range [_first, _last) using heap sort with the
// default ascending comparator.
template<typename _RandomIterator>
void heap_sort(_RandomIterator _first,
               _RandomIterator _last)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    internal::heap_sort_apply(_first,
                              _last,
                              less<value_type>());

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// C.  heap_sort_ordered(first, last, comp, order)
//     Wraps the comparator to honour sort_order at runtime.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// heap_sort_ordered
//   function: sorts the range [_first, _last) using heap sort, adapting
// _comp to the requested _order.
template<typename _RandomIterator,
         typename _Comparator>
void heap_sort_ordered(_RandomIterator  _first,
                       _RandomIterator  _last,
                       _Comparator   _comp,
                       sort_order _order)
{
    internal::order_comparator<_Comparator> wrapped(_comp, _order);

    internal::heap_sort_apply(_first,
                              _last,
                              wrapped);

    return;
}

#else  // C++98

// heap_sort_ordered
//   function: sorts the range [_first, _last) using heap sort, adapting
// _comp to the requested _order.  The _diff_type_hint parameter is used
// only for type deduction.
template<typename _RandomIterator,
         typename _Comparator,
         typename _DiffType>
void heap_sort_ordered(_RandomIterator  _first,
                       _RandomIterator  _last,
                       _Comparator   _comp,
                       sort_order _order,
                       _DiffType* _diff_type_hint)
{
    internal::order_comparator<_Comparator> wrapped(_comp, _order);

    internal::heap_sort_apply_98(_first,
                                 _last,
                                 wrapped,
                                 _diff_type_hint);

    return;
}

#endif  // C++11


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_HEAP_