/******************************************************************************
* djinterp [re_std]                                          partition_point.hpp
*
* partition_point algorithm header:
*   Given a range [_first, _last) already partitioned w.r.t. _pred
* (all trues precede all falses), returns the iterator to the first
* false-element (the partition boundary). Equivalent to lower_bound
* but driven by a unary predicate rather than a value comparison.
*
*   PORTABILITY:
*   - std::partition_point is C++11; re_std back-ports to C++98 (binary
*     search with a predicate; no language blocker).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - O(log N) predicate evaluations; O(log N) iterator stepping on
*     random access, O(N) on forward.
*
*
* path:      /inc/djinterp/re_std/algorithm/partition_point.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_PARTITION_POINT_
#define DJINTERP_RE_STD_ALGORITHM_PARTITION_POINT_ 1

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
// I.   PARTITION_POINT
// ===========================================================================

// partition_point
//   function: returns the first iterator it in [_first, _last) for
// which _pred(*it) is false. Range must be partitioned w.r.t. _pred.
template<typename _ForwardIt,
         typename _Pred>
D_CONSTEXPR_CPP14 _ForwardIt
partition_point(
    _ForwardIt _first,
    _ForwardIt _last,
    _Pred      _pred
)
{
    typedef typename iterator_traits<_ForwardIt>::difference_type _Diff;

    _Diff _len = re_std::distance(_first, _last);
    while (_len > 0)
    {
        _Diff      _half = _len / 2;
        _ForwardIt _mid  = _first;
        re_std::advance(_mid, _half);
        if (_pred(*_mid))
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


#endif  // DJINTERP_RE_STD_ALGORITHM_PARTITION_POINT_
