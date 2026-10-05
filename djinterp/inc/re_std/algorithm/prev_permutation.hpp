/*******************************************************************************
* djinterp [re_std]                                         prev_permutation.hpp
*
* prev_permutation algorithm header:
*   Rearranges [_first, _last) into the previous permutation in
* lexicographic order. Returns true on success; returns false and
* rewinds the range to the largest permutation (descending order)
* when the input was already the smallest.
*
*   Exact mirror of next_permutation: every comparison is flipped,
* so the scan looks for the rightmost DESCENT rather than the
* rightmost ascent, and the swap partner is the rightmost element
* LESS than the pivot.
*
*   PORTABILITY:
*   - std::prev_permutation is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*   - Bidirectional iterators suffice.
*
*
* path:      /inc/re_std/algorithm/prev_permutation.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.24
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_PREV_PERMUTATION_HPP
#define RE_STD_ALGORITHM_PREV_PERMUTATION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./iter_swap.hpp"
#include "./reverse.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   PREV_PERMUTATION (DEFAULT operator<)
// ===========================================================================

// prev_permutation
//   function: steps back to the previous lexicographic permutation.
// False (and a rewind to descending order) when the input was the first.
template<typename BidirIt>
RE_STD_CONSTEXPR_CPP14 bool
prev_permutation(
    BidirIt _first,
    BidirIt _last
)
{
    if (_first == _last)
    {
        return false;
    }

    BidirIt _i = _last;
    --_i;
    if (_first == _i)
    {
        return false;                       // single element
    }

    for (;;)
    {
        BidirIt _descent = _i;
        --_i;

        if (*_descent < *_i)
        {
            // rightmost element less than *_i
            BidirIt _j = _last;
            while (!(*--_j < *_i))
            {
                // empty
            }
            re_std::iter_swap(_i, _j);
            re_std::reverse(_descent, _last);
            return true;
        }

        if (_i == _first)
        {
            // wholly ascending: rewind to the largest permutation
            re_std::reverse(_first, _last);
            return false;
        }
    }
}


// ===========================================================================
// II.  PREV_PERMUTATION (COMPARATOR)
// ===========================================================================

// prev_permutation (comparator)
//   function: as above but ordering is decided by _comp.
template<typename BidirIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 bool
prev_permutation(
    BidirIt _first,
    BidirIt _last,
    Compare _comp
)
{
    if (_first == _last)
    {
        return false;
    }

    BidirIt _i = _last;
    --_i;
    if (_first == _i)
    {
        return false;
    }

    for (;;)
    {
        BidirIt _descent = _i;
        --_i;

        if (_comp(*_descent, *_i))
        {
            BidirIt _j = _last;
            while (!_comp(*--_j, *_i))
            {
                // empty
            }
            re_std::iter_swap(_i, _j);
            re_std::reverse(_descent, _last);
            return true;
        }

        if (_i == _first)
        {
            re_std::reverse(_first, _last);
            return false;
        }
    }
}


}  // re_std


#endif  // RE_STD_ALGORITHM_PREV_PERMUTATION_HPP
