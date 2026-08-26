/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/prev_permutation.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.24
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_PREV_PERMUTATION_
#define DJINTERP_RE_STD_ALGORITHM_PREV_PERMUTATION_ 1

// djinterp
#include "../../core/djinterp.hpp"
// re_std
#include "./iter_swap.hpp"
#include "./reverse.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   PREV_PERMUTATION (DEFAULT operator<)
// ===========================================================================

// prev_permutation
//   function: steps back to the previous lexicographic permutation.
// False (and a rewind to descending order) when the input was the first.
template<typename _BidirIt>
D_CONSTEXPR_CPP14 bool
prev_permutation(
    _BidirIt _first,
    _BidirIt _last
)
{
    if (_first == _last)
    {
        return false;
    }

    _BidirIt _i = _last;
    --_i;
    if (_first == _i)
    {
        return false;                       // single element
    }

    for (;;)
    {
        _BidirIt _descent = _i;
        --_i;

        if (*_descent < *_i)
        {
            // rightmost element less than *_i
            _BidirIt _j = _last;
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
template<typename _BidirIt,
         typename _Compare>
D_CONSTEXPR_CPP14 bool
prev_permutation(
    _BidirIt _first,
    _BidirIt _last,
    _Compare _comp
)
{
    if (_first == _last)
    {
        return false;
    }

    _BidirIt _i = _last;
    --_i;
    if (_first == _i)
    {
        return false;
    }

    for (;;)
    {
        _BidirIt _descent = _i;
        --_i;

        if (_comp(*_descent, *_i))
        {
            _BidirIt _j = _last;
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


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_PREV_PERMUTATION_
