/*******************************************************************************
* djinterp [cpp]                                           quick_sort_common.hpp
*
*   The primitives every quicksort is built from.  A quicksort is a driver
* wrapped around two operations: choose a pivot and move it to the front of the
* range, then partition the rest around it.  What distinguishes one quicksort
* from another is how it recurses -- which side it descends, which it loops
* on, and where it stops subdividing.
*
*     - The sequential driver (quick_sort.hpp) partitions, recurses into the
*       smaller side and loops on the larger, and hands any sub-range of
*       insertion_threshold elements or fewer to insertion sort.
*     - A task-parallel driver partitions the same way and gives the two sides
*       to different workers; the partition is a primitive here so that driver
*       can share it.
*     - A selection (nth-element) driver partitions and descends into one side
*       only.
*
*   MEDIAN OF THREE, MOVED TO THE FRONT.  The pivot is the median of the
* first, middle and last elements, which keeps ordered and reversed input --
* the cases a first-element pivot turns quadratic -- at O(n log n).  The three
* are put in order in place, so the middle one is the median, and that median
* is then exchanged to the front, where the partition expects its pivot.
*
*   HOARE CONVERGENCE, STOPPING ON EQUALS.  Two indices close in from the ends
* of the range; each stops at an element equivalent to the pivot, and the two
* stopped elements are exchanged.  Stopping on equals costs exchanges that a
* run of equivalent elements does not need, but it splits such a run down the
* middle -- a partitioner that skipped equals would put the whole run on one
* side, and an input of one repeated value would go quadratic.
*
*
* path:      /inc/djinterp/cpp/util/sort/quick_sort_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_UTIL_SORT_QUICK_SORT_COMMON_HPP
#define DJINTERP_UTIL_SORT_QUICK_SORT_COMMON_HPP 1

// std
#include <algorithm>                                // std::iter_swap
// djinterp
#include "../../djinterp.hpp"                    // framework root
#include "../../../config/cpp/util/sort/cfg_sort.h" // insertion threshold


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   ALGORITHM TRAITS                                ///
///////////////////////////////////////////////////////////////////////////////

// quick_sort_traits
//   struct: compile-time properties of the quicksort family, so a caller
// choosing between algorithms can branch on the property rather than the name.
//   - is_stable:   no   (the partition exchanges across the whole range)
//   - is_in_place: yes  (no buffer; the recursion stack is O(log n) deep,
//                        because only the smaller side is recursed into)
//   - is_adaptive: no   (ordered input is partitioned like any other; the
//                        median-of-three pivot keeps it O(n log n), not O(n))
//   - insertion_threshold: sub-ranges of this many elements or fewer are
//                        finished with insertion sort.
//                        D_CFG_SORT_QUICK_INSERTION_THRESHOLD; 16 by default.
struct quick_sort_traits
{
    static const bool is_stable   = false;
    static const bool is_in_place = true;
    static const bool is_adaptive = false;

    static const int  insertion_threshold =
        D_INTERNAL_SORT_QUICK_INSERTION_THRESHOLD;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  PRIMITIVES                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // quick_sort_median_of_three
    //   function: orders the elements at _first, _mid and _end so that
    // *_first, *_mid, *_end ascend under _comparator, then exchanges the
    // median -- now at _mid -- to _first, where quick_sort_partition expects
    // the pivot.  On return *_mid holds the smallest of the three and *_end
    // the largest.
    //
    //   The three positions may coincide; an exchange of a position with
    // itself leaves it unchanged, so a degenerate triple is harmless.
    template<typename RandomIterator,
             typename Comparator>
    void quick_sort_median_of_three(
        RandomIterator _first,
        RandomIterator _mid,
        RandomIterator _end,
        Comparator     _comparator
    )
    {
        // put the triple in order: *_first, *_mid, *_end
        if (_comparator(*_mid, *_first))
        {
            std::iter_swap(_first, _mid);
        }

        if (_comparator(*_end, *_first))
        {
            std::iter_swap(_first, _end);
        }

        if (_comparator(*_end, *_mid))
        {
            std::iter_swap(_mid, _end);
        }

        // the median goes to the front, where the partition takes its pivot
        std::iter_swap(_first, _mid);

        return;
    }

    // quick_sort_partition
    //   function: partitions [_first, _last) around the pivot at *_first and
    // returns the pivot's final position.  On return no element before it
    // follows the pivot and no element after it precedes the pivot.
    //
    //   _low advances past elements that strictly precede the pivot and _high
    // retreats past elements the pivot strictly precedes; both stop at an
    // equivalent, and the two stopped elements are exchanged.  When the
    // indices cross, _high is the last slot of the left side, and the pivot
    // is exchanged into it.
    //
    //   The range must not be empty: the pivot is read from *_first and the
    // scan starts at _last - 1.  quick_sort_apply never calls it on one.
    template<typename RandomIterator,
             typename Comparator>
    RandomIterator quick_sort_partition(
        RandomIterator _first,
        RandomIterator _last,
        Comparator     _comparator
    )
    {
        RandomIterator low;
        RandomIterator high;

        low  = _first + 1;
        high = _last - 1;

        for (;;)
        {
            // the left side takes everything that strictly precedes the pivot
            while ( (low <= high) &&
                    (_comparator(*low, *_first)) )
            {
                ++low;
            }

            // the right side, everything the pivot strictly precedes
            while ( (low <= high) &&
                    (_comparator(*_first, *high)) )
            {
                --high;
            }

            // crossed: the partition is complete
            if (low > high)
            {
                break;
            }

            // two misplaced elements -- or two equivalents of the pivot,
            // which this splits between the sides
            std::iter_swap(low, high);

            ++low;
            --high;
        }

        // the pivot into its final position
        std::iter_swap(_first, high);

        return high;
    }

NS_END  // internal


NS_END  // djinterp


#endif  // DJINTERP_UTIL_SORT_QUICK_SORT_COMMON_HPP
