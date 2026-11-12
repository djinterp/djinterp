/*******************************************************************************
* djinterp [cpp]                                                   heap_sort.hpp
*
*   Heap sort: the sequential driver.
* In-place, iterative, unstable, comparison-based.  The range is first arranged
* into an implicit binary max-heap, bottom-up; then the root -- the largest
* remaining element -- is exchanged with the last element of the heap, the
* heap shrinks by one, and the new root is sifted back down.  The sorted suffix
* grows by one element per extraction until it is the whole range.  The driver
* is two loops around heap_sift_down; everything else lives in
* heap_sort_common.hpp.
*
*   NO C TWIN.  The C sort module has no heap sort.  If one is added, the
* parity case selection_sort.hpp makes applies with more force: heap sort is
* not stable, and where it leaves equivalent elements depends on the heap's
* shape at every extraction, so two implementations agree only by making the
* same choices -- strict comparisons in the sift, and the left child kept when
* the two children are equivalent.
*
*   complexity:
*     best:     O(n log n)  (heapify reorders ordered input too)
*     average:  O(n log n)
*     worst:    O(n log n)  (guaranteed -- no adversarial input exists)
*     space:    O(1)        (one held element per sift)
*     stable:   no          (extraction jumps the root over equivalents)
*
*   REQUIREMENTS.  RandomIterator must be a random-access iterator; the
* element type must be copy-constructible and copy-assignable, and swappable
* through std::iter_swap.  Comparator must be a std::sort-convention binary
* predicate -- which every model of is_comparator is, so the composed
* comparators from the functional layer drop in unchanged:
*
*       heap_sort(v.begin(), v.end(),
*                 by_key(&person::age) | then(by_member(&person::name)));
*
*
* path:      /inc/djinterp/cpp/util/sort/heap_sort.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.22
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_UTIL_SORT_HEAP_SORT_HPP
#define DJINTERP_UTIL_SORT_HEAP_SORT_HPP 1

// std
#include <algorithm>               // std::iter_swap
#include <functional>              // std::less
#include <iterator>                // std::iterator_traits
// djinterp
#include "../../djinterp.hpp"   // framework root
#include "./heap_sort_common.hpp"  // internal::heap_sift_down
#include "./sort_common.hpp"       // sort_order,
                                   // internal::DOrderComparator


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   IMPLEMENTATION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // heap_sort_apply
    //   function: performs heap sort on [_first, _last).
    //
    //   HEAPIFY, BOTTOM-UP.  Every internal node is sifted, from the last
    // parent back to the root.  Each sub-tree is already a heap when its root
    // is sifted, which is what heap_sift_down assumes, and is why building the
    // heap this way costs O(n) rather than O(n log n).
    //
    //   EXTRACT.  The root is the largest element left in the heap.  It is
    // exchanged with the heap's last element, which is its final position;
    // the heap shrinks by one and the element that took the root's place is
    // sifted down.
    template<typename RandomIterator,
             typename Comparator>
    void heap_sort_apply(
        RandomIterator _first,
        RandomIterator _last,
        Comparator     _comparator
    )
    {
        typedef typename std::iterator_traits<RandomIterator>::difference_type
            difference_type;

        difference_type size;
        difference_type index;

        size = _last - _first;

        // a range of 0 or 1 elements is already sorted
        if (size < 2)
        {
            return;
        }

        // heapify: the last parent first, the root last
        index = heap_parent(size - 1);

        for (;;)
        {
            heap_sift_down(_first,
                           size,
                           index,
                           _comparator);

            if (index == 0)
            {
                break;
            }

            --index;
        }

        // extract: the root to the end of the shrinking heap
        for (index = size - 1; index > 0; --index)
        {
            std::iter_swap(_first, _first + index);

            heap_sift_down(_first,
                           index,
                           static_cast<difference_type>(0),
                           _comparator);
        }

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    II.  ENTRY POINTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// A.  heap_sort(first, last, comp)
// ----------------------------------------------------------------------------

// heap_sort
//   function: sorts the range [_first, _last) using heap sort with the
// comparator _comparator.
template<typename RandomIterator,
         typename Comparator>
void heap_sort(
    RandomIterator _first,
    RandomIterator _last,
    Comparator     _comparator
)
{
    internal::heap_sort_apply(_first,
                              _last,
                              _comparator);

    return;
}

// ----------------------------------------------------------------------------
// B.  heap_sort(first, last)      (C++11+)
//     Uses operator< via std::less<value_type>.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// heap_sort
//   function: sorts the range [_first, _last) using heap sort with the default
// ascending comparator.
template<typename RandomIterator>
void heap_sort(
    RandomIterator _first,
    RandomIterator _last
)
{
    typedef typename std::iterator_traits<RandomIterator>::value_type
        value_type;

    internal::heap_sort_apply(_first,
                              _last,
                              std::less<value_type>());

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// C.  heap_sort_ordered(first, last, comp, order)
//     Wraps the comparator to honour sort_order at runtime.
// ----------------------------------------------------------------------------

// heap_sort_ordered
//   function: sorts the range [_first, _last) using heap sort, adapting
// _comparator to the requested _order.  The C++ counterpart of the C module's
// _order parameter: there the direction is passed to the call, here it is
// folded into the comparator, which costs the same and composes better.
template<typename RandomIterator,
         typename Comparator>
void heap_sort_ordered(
    RandomIterator    _first,
    RandomIterator    _last,
    Comparator        _comparator,
    sort_order::value _order
)
{
    internal::DOrderComparator<Comparator> wrapped(_comparator, _order);

    internal::heap_sort_apply(_first,
                              _last,
                              wrapped);

    return;
}


NS_END  // djinterp


#endif  // DJINTERP_UTIL_SORT_HEAP_SORT_HPP
