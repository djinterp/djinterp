/******************************************************************************
* djinterp [re_std]                                         next_permutation.hpp
*
* next_permutation algorithm header:
*   Rearranges [_first, _last) into the next permutation in
* lexicographic order. Returns true on success; returns false and
* rewinds the range to the smallest permutation (ascending order)
* when the input was already the largest.
*
*   ALGORITHM (Pandita's, O(N)):
*     1. scan right-to-left for the rightmost ascent, i.e. the last
*        position i with *i < *(i+1);
*     2. if none exists the range is descending -- reverse it and
*        report false;
*     3. otherwise scan right-to-left for the rightmost j with
*        *i < *j, swap i and j, and reverse the suffix after i.
*
*   PORTABILITY:
*   - std::next_permutation is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*   - Bidirectional iterators suffice.
*
*
* path:      /inc/djinterp/re_std/algorithm/next_permutation.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.24
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_NEXT_PERMUTATION_
#define DJINTERP_RE_STD_ALGORITHM_NEXT_PERMUTATION_ 1

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
// I.   NEXT_PERMUTATION (DEFAULT operator<)
// ===========================================================================

// next_permutation
//   function: advances to the next lexicographic permutation. False (and
// a rewind to ascending order) when the input was the last one.
template<typename _BidirIt>
D_CONSTEXPR_CPP14 bool
next_permutation(
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
        _BidirIt _ascent = _i;
        --_i;

        if (*_i < *_ascent)
        {
            // rightmost element greater than *_i
            _BidirIt _j = _last;
            while (!(*_i < *--_j))
            {
                // empty -- the scan itself does the work
            }
            re_std::iter_swap(_i, _j);
            re_std::reverse(_ascent, _last);
            return true;
        }

        if (_i == _first)
        {
            // wholly descending: rewind to the smallest permutation
            re_std::reverse(_first, _last);
            return false;
        }
    }
}


// ===========================================================================
// II.  NEXT_PERMUTATION (COMPARATOR)
// ===========================================================================

// next_permutation (comparator)
//   function: as above but ordering is decided by _comp.
template<typename _BidirIt,
         typename _Compare>
D_CONSTEXPR_CPP14 bool
next_permutation(
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
        _BidirIt _ascent = _i;
        --_i;

        if (_comp(*_i, *_ascent))
        {
            _BidirIt _j = _last;
            while (!_comp(*_i, *--_j))
            {
                // empty
            }
            re_std::iter_swap(_i, _j);
            re_std::reverse(_ascent, _last);
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


#endif  // DJINTERP_RE_STD_ALGORITHM_NEXT_PERMUTATION_
