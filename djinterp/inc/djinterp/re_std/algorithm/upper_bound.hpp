/******************************************************************************
* djinterp [restd]                                               upper_bound.hpp
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
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - O(log N) comparisons; O(log N) overall on random access, O(N)
*     stepping on forward.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/djinterp/re_std/algorithm/upper_bound.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_UPPER_BOUND_
#define DJINTERP_RESTD_ALGORITHM_UPPER_BOUND_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "../iterator/iterator_traits.hpp"
#include "../iterator/advance.hpp"
#include "../iterator/distance.hpp"


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
// I.   UPPER_BOUND (DEFAULT operator<)
// ===========================================================================

// upper_bound
//   function: returns the first iterator in [_first, _last) at which
// *iter is strictly greater than _value (per operator<). Returns _last
// if no element is greater.
template<typename _ForwardIt,
         typename _Type>
D_CONSTEXPR_CPP14 _ForwardIt
upper_bound(
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
template<typename _ForwardIt,
         typename _Type,
         typename _Compare>
D_CONSTEXPR_CPP14 _ForwardIt
upper_bound(
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


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_UPPER_BOUND_
