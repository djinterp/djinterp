/*******************************************************************************
* djinterp [cpp]                                                  quick_sort.hpp
*
*   Quicksort: the sequential driver.
* In-place, divide-and-conquer, unstable, comparison-based.  The range is
* partitioned around a median-of-three pivot; the smaller side is sorted by
* recursion and the larger by looping, and any sub-range of
* quick_sort_traits::insertion_threshold elements or fewer is finished with
* insertion sort.  The pivot choice and the partition live in
* quick_sort_common.hpp, and the small-range finish is insertion_sort.hpp's own
* driver, so the three algorithms share their primitives rather than restating
* them.
*
*   RECURSE SMALL, LOOP LARGE.  Each step recurses into the smaller side of the
* partition and narrows the loop to the larger one.  The smaller side holds at
* most half the range, so the recursion is at most log2(n) deep however
* unbalanced the partitions are -- the quadratic worst case can cost time, but
* never the stack.
*
*   NO C TWIN.  The C sort module has no quicksort.  If one is added, the
* parity case selection_sort.hpp makes applies here too: quicksort is not
* stable, and where it leaves equivalent elements depends on every choice in
* quick_sort_common.hpp -- the pivot, the stop-on-equals scan and the
* threshold -- so two implementations agree only by making the same ones.
*
*   complexity:
*     best:     O(n log n)
*     average:  O(n log n)
*     worst:    O(n^2)      (needs an adversarial input; median-of-three
*                            keeps ordered, reversed and constant input fast)
*     space:    O(log n)    (recursion into the smaller side only)
*     stable:   no
*
*   REQUIREMENTS.  RandomIterator must be a random-access iterator; the
* element type must be copy-constructible, copy-assignable and swappable
* through std::iter_swap.  Comparator must be a std::sort-convention binary
* predicate -- which every model of is_comparator is, so the composed
* comparators from the functional layer drop in unchanged:
*
*       quick_sort(v.begin(), v.end(),
*                  by_key(&person::age) | then(by_member(&person::name)));
*
*
* path:      /inc/djinterp/cpp/util/sort/quick_sort.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.22
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_UTIL_SORT_QUICK_SORT_HPP
#define DJINTERP_UTIL_SORT_QUICK_SORT_HPP 1

// std
#include <functional>               // std::less
#include <iterator>                 // std::iterator_traits
// djinterp
#include "../../djinterp.hpp"    // framework root
#include "./insertion_sort.hpp"     // internal::insertion_sort_apply
#include "./quick_sort_common.hpp"  // quick_sort_traits,
                                    // internal::quick_sort_partition
#include "./sort_common.hpp"        // sort_order,
                                    // internal::DOrderComparator


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   IMPLEMENTATION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // quick_sort_apply
    //   function: performs quicksort on [_first, _last).
    //
    //   While the range is longer than the threshold it is partitioned; the
    // smaller side is sorted by a recursive call and the loop continues on
    // the larger.  What remains when the loop ends -- the whole range, if it
    // was short to begin with -- goes to insertion sort.
    //
    //   An empty range never reaches the partitioner: the loop requires more
    // than insertion_threshold elements, and the threshold is never negative.
    template<typename RandomIterator,
             typename Comparator>
    void quick_sort_apply(
        RandomIterator _first,
        RandomIterator _last,
        Comparator     _comparator
    )
    {
        RandomIterator pivot;

        while ((_last - _first) > quick_sort_traits::insertion_threshold)
        {
            quick_sort_median_of_three(_first,
                                       _first + ((_last - _first) / 2),
                                       _last - 1,
                                       _comparator);

            pivot = quick_sort_partition(_first,
                                         _last,
                                         _comparator);

            // recurse into the smaller side, loop on the larger
            if ((pivot - _first) < (_last - (pivot + 1)))
            {
                quick_sort_apply(_first,
                                 pivot,
                                 _comparator);

                _first = pivot + 1;
            }
            else
            {
                quick_sort_apply(pivot + 1,
                                 _last,
                                 _comparator);

                _last = pivot;
            }
        }

        // the short remainder, finished by insertion sort
        insertion_sort_apply(_first,
                             _last,
                             _comparator);

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    II.  ENTRY POINTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// A.  quick_sort(first, last, comp)
// ----------------------------------------------------------------------------

// quick_sort
//   function: sorts the range [_first, _last) using quicksort with the
// comparator _comparator.
template<typename RandomIterator,
         typename Comparator>
void quick_sort(
    RandomIterator _first,
    RandomIterator _last,
    Comparator     _comparator
)
{
    internal::quick_sort_apply(_first,
                               _last,
                               _comparator);

    return;
}

// ----------------------------------------------------------------------------
// B.  quick_sort(first, last)      (C++11+)
//     Uses operator< via std::less<value_type>.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// quick_sort
//   function: sorts the range [_first, _last) using quicksort with the
// default ascending comparator.
template<typename RandomIterator>
void quick_sort(
    RandomIterator _first,
    RandomIterator _last
)
{
    typedef typename std::iterator_traits<RandomIterator>::value_type
        value_type;

    internal::quick_sort_apply(_first,
                               _last,
                               std::less<value_type>());

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// C.  quick_sort_ordered(first, last, comp, order)
//     Wraps the comparator to honour sort_order at runtime.
// ----------------------------------------------------------------------------

// quick_sort_ordered
//   function: sorts the range [_first, _last) using quicksort, adapting
// _comparator to the requested _order.
template<typename RandomIterator,
         typename Comparator>
void quick_sort_ordered(
    RandomIterator    _first,
    RandomIterator    _last,
    Comparator        _comparator,
    sort_order::value _order
)
{
    internal::DOrderComparator<Comparator> wrapped(_comparator, _order);

    internal::quick_sort_apply(_first,
                               _last,
                               wrapped);

    return;
}


NS_END  // djinterp


#endif  // DJINTERP_UTIL_SORT_QUICK_SORT_HPP
