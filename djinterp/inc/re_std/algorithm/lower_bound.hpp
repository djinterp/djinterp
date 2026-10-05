/*******************************************************************************
* djinterp [re_std]                                              lower_bound.hpp
*
* lower_bound algorithm header:
*   Binary search on a partitioned/sorted range [_first, _last).
* Returns the first iterator at which *iter is NOT less than _value (per
* _comp or operator<). Equivalently: the leftmost insertion point for
* _value that preserves the partition w.r.t. operator<.
*
*   PORTABILITY:
*   - std::lower_bound is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14 (the
*     mutable len/half locals need relaxed constexpr).
*   - Works on forward iterators in O(log N) comparisons but O(N)
*     stepping; on random access it is O(log N) overall.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/lower_bound.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_LOWER_BOUND_HPP
#define RE_STD_ALGORITHM_LOWER_BOUND_HPP 1

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
// I.   LOWER_BOUND (DEFAULT operator<)
// ===========================================================================

// lower_bound
//   function: returns the first iterator in [_first, _last) at which
// *iter is NOT less than _value (per operator<). Returns _last if
// _value compares greater than every element.
template<typename ForwardIt,
         typename Type>
RE_STD_CONSTEXPR_CPP14 ForwardIt
lower_bound(
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
        else
        {
            _len = _half;
        }
    }
    return _first;
}


// ===========================================================================
// II.  LOWER_BOUND (COMPARATOR)
// ===========================================================================

// lower_bound (comparator)
//   function: as above but element-vs-value comparison is via _comp.
// The range must be partitioned with respect to _comp(*iter, _value).
template<typename ForwardIt,
         typename Type,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 ForwardIt
lower_bound(
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
        else
        {
            _len = _half;
        }
    }
    return _first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_LOWER_BOUND_HPP
