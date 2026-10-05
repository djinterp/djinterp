/*******************************************************************************
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
* path:      /inc/re_std/algorithm/is_sorted_until.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_IS_SORTED_UNTIL_HPP
#define RE_STD_ALGORITHM_IS_SORTED_UNTIL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   IS_SORTED_UNTIL (DEFAULT operator<)
// ===========================================================================

// is_sorted_until
//   function: returns the first iterator at which the sorted invariant
// is violated, comparing adjacent pairs via operator<. Returns _last
// if the range is sorted.
template<typename ForwardIt>
RE_STD_CONSTEXPR_CPP14 ForwardIt
is_sorted_until(
    ForwardIt _first,
    ForwardIt _last
)
{
    if (_first == _last)
    {
        return _last;
    }

    ForwardIt _next = _first;
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
template<typename ForwardIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 ForwardIt
is_sorted_until(
    ForwardIt _first,
    ForwardIt _last,
    Compare   _comp
)
{
    if (_first == _last)
    {
        return _last;
    }

    ForwardIt _next = _first;
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


}  // re_std


#endif  // RE_STD_ALGORITHM_IS_SORTED_UNTIL_HPP
