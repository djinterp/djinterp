/******************************************************************************
* djinterp [utility]                                  selection_sort_common.hpp
*
*   The primitives every selection sort is built from.
* A selection sort is a driver wrapped around two operations: find the element
* that belongs first in a run, and exchange it into the run's front.
*
*     - The sequential driver (selection_sort.hpp) scans [i, count) for each i,
*       exchanging the winner into position i.
*
*     - A concurrent driver parallelises the SCAN, not the sort: the search for
*       a run's minimum is a reduction, so each worker takes a chunk, reports
*       its chunk's winner, and one comparison per chunk settles the overall
*       winner.  That is why the search is a separate primitive with its own
*       range rather than an inner loop -- it is the piece that partitions.
*
*   WHY THIS ALGORITHM EXISTS: MINIMAL WRITES.  It always performs n(n-1)/2
* comparisons, so insertion sort beats it on every input.  What it buys is
* exchanges: at most n-1, one per position, regardless of how disordered the
* input is.  An element travelling from the end of the range to the front costs
* one exchange, where bubble and insertion would touch every slot between.
* That trade pays when elements are large and comparison is cheap, and not
* otherwise.
*
*   THIS FAMILY IS NOT STABLE, AND THAT CHANGES WHAT AGREEMENT COSTS.
* Exchanging the winner into position i evicts whatever was at i to the
* winner's old slot, arbitrarily far away, past any number of elements equal to
* it.  No scan order fixes this; it is the exchange itself.
*
*   The consequence matters for this codebase specifically.  Bubble and
* insertion are stable, so "sorted and stable" names exactly one arrangement
* and the C and C++ modules agree however each is written.  Here the sorted
* arrangement is not unique, so agreement between the two modules is a
* discipline rather than a consequence, and it rests on two choices that must
* be made the same way in both:
*
*     1. THE SCAN RUNS FORWARD, from the run's front toward its end.
*     2. A CANDIDATE MUST STRICTLY PRECEDE the incumbent to displace it, so
*        among equal elements the EARLIEST wins.
*
*   Both are normative for every implementation in this family, in either
* language, and the parity check tests them -- unlike the stable algorithms,
* where the equivalent check could not fail.
*
*   If a caller needs stability, the fix is not to patch this algorithm: a
* stable selection sort must rotate the run rather than exchange, spending the
* O(n) writes that were the entire reason to choose it.  Use insertion_sort,
* which is stable and faster besides.
*
*
* path:      /djinterp/cpp/util/sort/selection_sort_common.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_SELECTION_COMMON_HPP_
#define DJINTERP_UTILITY_SORT_SELECTION_COMMON_HPP_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   ALGORITHM TRAITS                                ///
///////////////////////////////////////////////////////////////////////////////

// selection_sort_traits
//   struct: compile-time properties of the selection sort family, so a caller
// choosing between algorithms can branch on the property rather than the name.
// The C module publishes the same three as D_SELECTION_SORT_IS_STABLE,
// _IS_IN_PLACE and _IS_ADAPTIVE.
//   - is_stable:   no   (the exchange evicts an element past its equals)
//   - is_in_place: yes  (O(1) auxiliary memory)
//   - is_adaptive: no   (the scan must see every remaining element, so
//                        ordered input costs what reversed input costs)
struct selection_sort_traits
{
    static const bool is_stable   = false;
    static const bool is_in_place = true;
    static const bool is_adaptive = false;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  PRIMITIVES                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // selection_min_index
    //   function: the index of the element that belongs first in the run
    // [_begin, _end).
    //
    //   "Minimum" is with respect to the comparator, so a reversed comparator
    // finds the largest element without a second code path here.
    //
    //   A candidate must STRICTLY precede the incumbent to displace it, so
    // among equal elements the earliest wins.  That, with the forward scan, is
    // what pins the arrangement this family produces -- see the header note.
    // It is also why the scan cannot stop early: nothing learned about the
    // first k elements constrains the rest, which is the whole of why this
    // algorithm is not adaptive.
    template<typename _RandomIterator,
             typename _Difference,
             typename _Comparator>
    _Difference selection_min_index(_RandomIterator _first,
                                    _Difference     _begin,
                                    _Difference     _end,
                                    _Comparator     _comparator)
    {
        _Difference winner;
        _Difference index;

        // an empty run has no minimum to report
        if (_end <= _begin)
        {
            return _begin;
        }

        winner = _begin;

        // only a STRICT precedence displaces the incumbent, so the earliest
        // of several equal elements is the one that wins
        for (index = _begin + 1; index < _end; ++index)
        {
            if (_comparator(_first[index], _first[winner]))
            {
                winner = index;
            }
        }

        return winner;
    }

    // selection_pass
    //   function: sorts the run [_begin, _end) by repeatedly selecting its
    // minimum and exchanging it to the front of the unsorted remainder.
    //
    //   The exchange is skipped when the winner is already in place.  That is
    // not only an optimisation: it is what holds the exchange count to at most
    // n-1, which is this algorithm's entire reason for existing.
    //
    //   The final position needs no scan of its own -- once everything else is
    // placed, the one element remaining is the largest, with nothing left to
    // compare against -- so the loop stops one short of the end.
    template<typename _RandomIterator,
             typename _Difference,
             typename _Comparator>
    void selection_pass(_RandomIterator _first,
                        _Difference     _begin,
                        _Difference     _end,
                        _Comparator     _comparator)
    {
        _Difference position;
        _Difference winner;

        // a run of 0 or 1 elements is already sorted
        if ((_end - _begin) < 2)
        {
            return;
        }

        // the last position takes whatever is left, so it needs no scan
        for (position = _begin; position < (_end - 1); ++position)
        {
            winner = selection_min_index(_first,
                                         position,
                                         _end,
                                         _comparator);

            // skipping a self-exchange is what keeps the count at n-1
            if (winner != position)
            {
                std::iter_swap(_first + position, _first + winner);
            }
        }

        return;
    }

NS_END  // internal


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_SELECTION_COMMON_HPP_
