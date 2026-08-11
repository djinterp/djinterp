/******************************************************************************
* djinterp [utility]                                      merge_sort_common.hpp
*
*   The primitives every merge sort is built from.
* A merge sort is a driver wrapped around two operations: merge two adjacent
* ordered runs into one, and sweep a whole range doing that at a fixed run
* width.
*
*   BOTTOM-UP, NOT RECURSIVE.  Runs start at width 1 -- every single element is
* trivially ordered -- and each pass doubles the width until one run covers the
* range.  Same comparisons as the recursive formulation, and it buys two
* things: no call stack, and a shape that partitions.  Within a pass the run
* pairs are DISJOINT, so any subset may be merged in parallel with any other,
* which is why a concurrent driver hands each worker a slice of the same pass
* rather than a branch of a recursion tree.  It is also the formulation
* sort_monoid.hpp describes: mconcat over a foldable of singleton runs IS this
* loop, and sorted_run's combine IS merge_runs.
*
*   SOURCE AND DESTINATION ARE SEPARATE, AND THE DRIVER SWAPS THEM.  A merge
* cannot write into what it reads, so each pass reads one buffer and writes the
* other.  The two are independent template parameters because the driver
* alternates their roles, and on the first pass they are genuinely different
* types: the caller's iterator and the buffer's.
*
*   STABILITY IS ONE COMPARISON'S DIRECTION.  The merge takes from the right
* run only when the right element STRICTLY precedes the left; on equivalence it
* takes from the left, which holds the earlier elements.  Reversing that test
* still merges correctly and destroys stability.
*
*   ALREADY-ORDERED RUN PAIRS ARE COPIED, NOT MERGED.  If the right run's first
* element does not precede the left run's last, every left element belongs
* first and the span is a single copy.  One comparison replaces a whole span of
* them, so a range arriving ordered costs O(n) comparisons instead of
* O(n log n).
*
*
* path:      /djinterp/cpp/util/sort/merge_sort_common.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_MERGE_COMMON_HPP_
#define DJINTERP_UTILITY_SORT_MERGE_COMMON_HPP_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   ALGORITHM TRAITS                                ///
///////////////////////////////////////////////////////////////////////////////

// merge_sort_traits
//   struct: compile-time properties of the merge sort family, so a caller
// choosing between algorithms can branch on the property rather than the name.
// The C module publishes the same three as D_MERGE_SORT_IS_STABLE,
// _IS_IN_PLACE and _IS_ADAPTIVE.
//   - is_stable:   yes  (the merge takes from the right only on a strict
//                        precedence, so equal elements keep their order)
//   - is_in_place: no   (one element of scratch per element of the range)
//   - is_adaptive: yes  (in comparisons only: an ordered run pair is detected
//                        in one comparison and copied rather than merged)
struct merge_sort_traits
{
    static const bool is_stable   = true;
    static const bool is_in_place = false;
    static const bool is_adaptive = true;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  PRIMITIVES                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // merge_runs
    //   function: merges the ordered runs [_begin, _mid) and [_mid, _end) of
    // _src into [_begin, _end) of _dst.
    //
    //   Taking from the right run only on a STRICT precedence is the whole of
    // this family's stability: on equivalence the left element goes first, and
    // the left run holds the earlier elements.
    //
    //   Nothing is compared when a run is empty, and nothing is compared at
    // all when the pair is already in order -- one test settles that and the
    // span becomes a straight copy.
    //
    //   _src and _dst must not overlap over [_begin, _end).
    template<typename _SrcIterator,
             typename _DstIterator,
             typename _Difference,
             typename _Comparator>
    void merge_runs(_SrcIterator _src,
                    _DstIterator _dst,
                    _Difference  _begin,
                    _Difference  _mid,
                    _Difference  _end,
                    _Comparator  _comparator)
    {
        _Difference left;
        _Difference right;
        _Difference out;

        // an empty span has nothing to merge
        if (_end <= _begin)
        {
            return;
        }

        // one run only, or a pair already in order: the span is a copy.  The
        // second test asks whether the right run's first element precedes the
        // left run's last; when it does not, every left element belongs first
        if ( (_mid <= _begin) ||
             (_mid >= _end)   ||
             (!_comparator(_src[_mid], _src[_mid - 1])) )
        {
            for (out = _begin; out < _end; ++out)
            {
                _dst[out] = _src[out];
            }

            return;
        }

        left  = _begin;
        right = _mid;

        // interleave while both runs have elements left
        for (out = _begin; out < _end; ++out)
        {
            // the left run is spent, or the right element strictly precedes
            // -- the strictness is what keeps equivalents in input order
            if ( (left >= _mid) ||
                 ( (right < _end) &&
                   (_comparator(_src[right], _src[left])) ) )
            {
                _dst[out] = _src[right];

                ++right;
            }
            else
            {
                _dst[out] = _src[left];

                ++left;
            }
        }

        return;
    }

    // merge_pass
    //   function: merges every adjacent pair of _width-element runs from _src
    // into _dst, covering all _count elements.
    //
    //   A range whose length is not a multiple of twice the width ends in a
    // short run, or in a lone run with no partner.  Both fall out of clamping
    // the two bounds to the count: a lone run has an empty right side, which
    // merge_runs copies through untouched.
    //
    //   The run pairs are DISJOINT, which is the property a concurrent driver
    // exploits: no two iterations below touch the same element.
    template<typename _SrcIterator,
             typename _DstIterator,
             typename _Difference,
             typename _Comparator>
    void merge_pass(_SrcIterator _src,
                    _DstIterator _dst,
                    _Difference  _count,
                    _Difference  _width,
                    _Comparator  _comparator)
    {
        _Difference begin;
        _Difference mid;
        _Difference end;

        // a zero width would not advance, and an empty range has no runs
        if ( (_width < 1) ||
             (_count < 1) )
        {
            return;
        }

        // each iteration is independent of every other one
        for (begin = 0; begin < _count; begin += (2 * _width))
        {
            mid = begin + _width;
            end = begin + (2 * _width);

            // a short or absent partner run clamps to the end of the range
            if (mid > _count)
            {
                mid = _count;
            }

            if (end > _count)
            {
                end = _count;
            }

            merge_runs(_src,
                       _dst,
                       begin,
                       mid,
                       end,
                       _comparator);
        }

        return;
    }

NS_END  // internal


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_MERGE_COMMON_HPP_
