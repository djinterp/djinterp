/*******************************************************************************
* djinterp [re_std]                                              equal_range.hpp
*
* equal_range algorithm header:
*   Returns a pair (lower_bound, upper_bound) bounding the subrange of
* [_first, _last) consisting of elements equivalent to _value (per
* operator< or _comp). The returned subrange [first.lo, first.hi) is
* the half-open range of elements that compare equivalent to _value.
*
*   PORTABILITY:
*   - std::equal_range is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - The classical implementation drops out of the binary search on
*     the first probe whose value is neither less than nor greater than
*     _value, then runs lower_bound on the left half and upper_bound on
*     the right half — total O(log N), strictly fewer comparisons than
*     calling lower_bound + upper_bound separately.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/equal_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_EQUAL_RANGE_HPP
#define RE_STD_ALGORITHM_EQUAL_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./lower_bound.hpp"
#include "./upper_bound.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../iterator/advance.hpp"
#include "../iterator/distance.hpp"
#include "../utility/pair.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   EQUAL_RANGE (DEFAULT operator<)
// ===========================================================================

// equal_range
//   function: returns pair(lo, hi) bounding the contiguous block of
// elements equivalent to _value. lo == lower_bound; hi == upper_bound.
template<typename ForwardIt,
         typename Type>
RE_STD_CONSTEXPR_CPP14 pair<ForwardIt, ForwardIt>
equal_range(
    ForwardIt   _first,
    ForwardIt   _last,
    const Type& _value
)
{
    typedef typename iterator_traits<ForwardIt>::difference_type _Diff;

    _Diff _len = re_std::distance(_first, _last);
    while (_len > 0)
    {
        _Diff      _half = _len / 2;
        ForwardIt _mid  = _first;
        re_std::advance(_mid, _half);

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
            ForwardIt _left_end = _first;
            re_std::advance(_left_end, _half);
            ForwardIt _lo = re_std::lower_bound(_first, _left_end, _value);

            ForwardIt _right_begin = _mid;
            ++_right_begin;
            ForwardIt _right_end = _first;
            re_std::advance(_right_end, _len);
            ForwardIt _hi = re_std::upper_bound(_right_begin, _right_end,
                                                _value);

            return pair<ForwardIt, ForwardIt>(_lo, _hi);
        }
    }
    return pair<ForwardIt, ForwardIt>(_first, _first);
}


// ===========================================================================
// II.  EQUAL_RANGE (COMPARATOR)
// ===========================================================================

// equal_range (comparator)
//   function: as above but comparison is via _comp.
template<typename ForwardIt,
         typename Type,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 pair<ForwardIt, ForwardIt>
equal_range(
    ForwardIt   _first,
    ForwardIt   _last,
    const Type& _value,
    Compare     _comp
)
{
    typedef typename iterator_traits<ForwardIt>::difference_type _Diff;

    _Diff _len = re_std::distance(_first, _last);
    while (_len > 0)
    {
        _Diff      _half = _len / 2;
        ForwardIt _mid  = _first;
        re_std::advance(_mid, _half);

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
            ForwardIt _left_end = _first;
            re_std::advance(_left_end, _half);
            ForwardIt _lo = re_std::lower_bound(_first, _left_end,
                                                _value, _comp);

            ForwardIt _right_begin = _mid;
            ++_right_begin;
            ForwardIt _right_end = _first;
            re_std::advance(_right_end, _len);
            ForwardIt _hi = re_std::upper_bound(_right_begin, _right_end,
                                                _value, _comp);

            return pair<ForwardIt, ForwardIt>(_lo, _hi);
        }
    }
    return pair<ForwardIt, ForwardIt>(_first, _first);
}


}  // re_std


#endif  // RE_STD_ALGORITHM_EQUAL_RANGE_HPP
