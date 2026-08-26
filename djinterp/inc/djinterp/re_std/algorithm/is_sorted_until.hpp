/******************************************************************************
* djinterp [re_std]                                          is_sorted_until.hpp
*
* is_sorted_until algorithm header:
*   Returns the first iterator it in [_first, _last) such that the
* adjacent-pair (it - 1, it) is OUT OF ORDER (i.e., *(it - 1) > *it
* per operator<, or _comp(*it, *(it - 1)) holds). Returns _last if the
* whole range is sorted.
*
*   PORTABILITY:
*   - std::is_sorted_until is C++11; re_std back-ports to C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/djinterp/re_std/algorithm/is_sorted_until.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_IS_SORTED_UNTIL_
#define DJINTERP_RE_STD_ALGORITHM_IS_SORTED_UNTIL_ 1

#include "../../core/djinterp.hpp"


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
// I.   IS_SORTED_UNTIL (DEFAULT operator<)
// ===========================================================================

// is_sorted_until
//   function: returns the first iterator at which the sorted invariant
// is violated, comparing adjacent pairs via operator<. Returns _last
// if the range is sorted.
template<typename _ForwardIt>
D_CONSTEXPR_CPP14 _ForwardIt
is_sorted_until(
    _ForwardIt _first,
    _ForwardIt _last
)
{
    if (_first == _last)
    {
        return _last;
    }

    _ForwardIt _next = _first;
    ++_next;

    for (; _next != _last; ++_first, (void)++_next)
    {
        if (*_next < *_first)
        {
            return _next;
        }
    }

    return _last;
}


// ===========================================================================
// II.  IS_SORTED_UNTIL (COMPARATOR)
// ===========================================================================

// is_sorted_until (comparator)
//   function: as above but adjacent-pair ordering is via _comp(b, a).
template<typename _ForwardIt,
         typename _Compare>
D_CONSTEXPR_CPP14 _ForwardIt
is_sorted_until(
    _ForwardIt _first,
    _ForwardIt _last,
    _Compare   _comp
)
{
    if (_first == _last)
    {
        return _last;
    }

    _ForwardIt _next = _first;
    ++_next;

    for (; _next != _last; ++_first, (void)++_next)
    {
        if (_comp(*_next, *_first))
        {
            return _next;
        }
    }

    return _last;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_IS_SORTED_UNTIL_
