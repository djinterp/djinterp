/******************************************************************************
* djinterp [restd]                                               equal_range.hpp
*
* equal_range algorithm header:
*   Returns a pair (lower_bound, upper_bound) bounding the subrange of
* [_first, _last) consisting of elements equivalent to _value (per
* operator< or _comp). The returned subrange [first.lo, first.hi) is
* the half-open range of elements that compare equivalent to _value.
*
*   PORTABILITY:
*   - std::equal_range is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - The classical implementation drops out of the binary search on
*     the first probe whose value is neither less than nor greater than
*     _value, then runs lower_bound on the left half and upper_bound on
*     the right half — total O(log N), strictly fewer comparisons than
*     calling lower_bound + upper_bound separately.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/djinterp/re_std/algorithm/equal_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_EQUAL_RANGE_
#define DJINTERP_RESTD_ALGORITHM_EQUAL_RANGE_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./lower_bound.hpp"
#include "./upper_bound.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../iterator/advance.hpp"
#include "../iterator/distance.hpp"
#include "../utility/pair.hpp"


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
// I.   EQUAL_RANGE (DEFAULT operator<)
// ===========================================================================

// equal_range
//   function: returns pair(lo, hi) bounding the contiguous block of
// elements equivalent to _value. lo == lower_bound; hi == upper_bound.
template<typename _ForwardIt,
         typename _Type>
D_CONSTEXPR_CPP14 pair<_ForwardIt, _ForwardIt>
equal_range(
    _ForwardIt   _first,
    _ForwardIt   _last,
    const _Type& _value
)
{
    typedef typename iterator_traits<_ForwardIt>::difference_type _Diff;

    _Diff _len = restd::distance(_first, _last);
    while (_len > 0)
    {
        _Diff      _half = _len / 2;
        _ForwardIt _mid  = _first;
        restd::advance(_mid, _half);

        if (*_mid < _value)
        {
            _first = _mid;
            ++_first;
            _len  -= _half + 1;
        }
        else if (_value < *_mid)
        {
            _len = _half;
        }
        else
        {
            // *_mid equivalent to _value; pivot to two bounded scans
            _ForwardIt _left_end = _first;
            restd::advance(_left_end, _half);
            _ForwardIt _lo = restd::lower_bound(_first, _left_end, _value);

            _ForwardIt _right_begin = _mid;
            ++_right_begin;
            _ForwardIt _right_end = _first;
            restd::advance(_right_end, _len);
            _ForwardIt _hi = restd::upper_bound(_right_begin, _right_end,
                                                _value);

            return pair<_ForwardIt, _ForwardIt>(_lo, _hi);
        }
    }
    return pair<_ForwardIt, _ForwardIt>(_first, _first);
}


// ===========================================================================
// II.  EQUAL_RANGE (COMPARATOR)
// ===========================================================================

// equal_range (comparator)
//   function: as above but comparison is via _comp.
template<typename _ForwardIt,
         typename _Type,
         typename _Compare>
D_CONSTEXPR_CPP14 pair<_ForwardIt, _ForwardIt>
equal_range(
    _ForwardIt   _first,
    _ForwardIt   _last,
    const _Type& _value,
    _Compare     _comp
)
{
    typedef typename iterator_traits<_ForwardIt>::difference_type _Diff;

    _Diff _len = restd::distance(_first, _last);
    while (_len > 0)
    {
        _Diff      _half = _len / 2;
        _ForwardIt _mid  = _first;
        restd::advance(_mid, _half);

        if (_comp(*_mid, _value))
        {
            _first = _mid;
            ++_first;
            _len  -= _half + 1;
        }
        else if (_comp(_value, *_mid))
        {
            _len = _half;
        }
        else
        {
            _ForwardIt _left_end = _first;
            restd::advance(_left_end, _half);
            _ForwardIt _lo = restd::lower_bound(_first, _left_end,
                                                _value, _comp);

            _ForwardIt _right_begin = _mid;
            ++_right_begin;
            _ForwardIt _right_end = _first;
            restd::advance(_right_end, _len);
            _ForwardIt _hi = restd::upper_bound(_right_begin, _right_end,
                                                _value, _comp);

            return pair<_ForwardIt, _ForwardIt>(_lo, _hi);
        }
    }
    return pair<_ForwardIt, _ForwardIt>(_first, _first);
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_EQUAL_RANGE_
