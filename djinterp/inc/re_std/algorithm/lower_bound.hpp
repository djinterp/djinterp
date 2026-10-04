/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/lower_bound.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_LOWER_BOUND_
#define DJINTERP_RE_STD_ALGORITHM_LOWER_BOUND_ 1

// djinterp
#include "../../core/djinterp.hpp"
// re_std
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
// I.   LOWER_BOUND (DEFAULT operator<)
// ===========================================================================

// lower_bound
//   function: returns the first iterator in [_first, _last) at which
// *iter is NOT less than _value (per operator<). Returns _last if
// _value compares greater than every element.
template<typename _ForwardIt,
         typename _Type>
D_CONSTEXPR_CPP14 _ForwardIt
lower_bound(
    _ForwardIt   _first,
    _ForwardIt   _last,
    const _Type& _value
)
{
    typedef typename iterator_traits<_ForwardIt>::difference_type _Diff;

    _Diff _len = re_std::distance(_first, _last);
    while (_len > 0)
    {
        _Diff      _half = _len / 2;
        _ForwardIt _mid  = _first;
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
template<typename _ForwardIt,
         typename _Type,
         typename _Compare>
D_CONSTEXPR_CPP14 _ForwardIt
lower_bound(
    _ForwardIt   _first,
    _ForwardIt   _last,
    const _Type& _value,
    _Compare     _comp
)
{
    typedef typename iterator_traits<_ForwardIt>::difference_type _Diff;

    _Diff _len = re_std::distance(_first, _last);
    while (_len > 0)
    {
        _Diff      _half = _len / 2;
        _ForwardIt _mid  = _first;
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


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_LOWER_BOUND_
