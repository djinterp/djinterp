/*******************************************************************************
* djinterp [re_std]                                              upper_bound.hpp
*
* upper_bound algorithm header:
*   Binary search on a partitioned/sorted range [_first, _last).
* Returns the first iterator at which *iter is strictly greater than
* _value (per _comp or operator<). Equivalently: the rightmost
* insertion point for _value that preserves the partition w.r.t.
* operator<.
*
*   PORTABILITY:
*   - std::upper_bound is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - O(log N) comparisons; O(log N) overall on random access, O(N)
*     stepping on forward.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/upper_bound.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_UPPER_BOUND_HPP
#define RE_STD_ALGORITHM_UPPER_BOUND_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "../iterator/iterator_traits.hpp"
#include "../iterator/advance.hpp"
#include "../iterator/distance.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   UPPER_BOUND (DEFAULT operator<)
// ===========================================================================

// upper_bound
//   function: returns the first iterator in [_first, _last) at which
// *iter is strictly greater than _value (per operator<). Returns _last
// if no element is greater.
template<typename ForwardIt,
         typename Type>
RE_STD_CONSTEXPR_CPP14 ForwardIt
upper_bound(
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
        if (_value < *_mid)
        {
            _len = _half;
        }
        else
        {
            _first = _mid;
            ++_first;
            _len  -= _half + 1;
        }
    }
    return _first;
}


// ===========================================================================
// II.  UPPER_BOUND (COMPARATOR)
// ===========================================================================

// upper_bound (comparator)
//   function: as above but comparison is via _comp(_value, *iter).
template<typename ForwardIt,
         typename Type,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 ForwardIt
upper_bound(
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
        if (_comp(_value, *_mid))
        {
            _len = _half;
        }
        else
        {
            _first = _mid;
            ++_first;
            _len  -= _half + 1;
        }
    }
    return _first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_UPPER_BOUND_HPP
