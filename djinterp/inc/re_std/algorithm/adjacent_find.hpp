/*******************************************************************************
* djinterp [re_std]                                            adjacent_find.hpp
*
* adjacent_find algorithm header:
*   Returns the first iterator it in [_first, _last) such that *it ==
* *(it + 1) (or _pred(*it, *(it + 1)) holds, in the predicate overload).
* Returns _last on no-match, including the empty-range case.
*
*   PORTABILITY:
*   - std::adjacent_find is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/adjacent_find.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_ADJACENT_FIND_HPP
#define RE_STD_ALGORITHM_ADJACENT_FIND_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   ADJACENT_FIND (DEFAULT ==)
// ===========================================================================

// adjacent_find
//   function: returns the first iterator it in [_first, _last) such
// that *it == *(it + 1). Returns _last for empty or one-element ranges
// and on no-match.
template<typename ForwardIt>
RE_STD_CONSTEXPR_CPP14 ForwardIt
adjacent_find(
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
        if (*_first == *_next)
        {
            return _first;
        }
    }

    return _last;
}


// ===========================================================================
// II.  ADJACENT_FIND (CUSTOM PRED)
// ===========================================================================

// adjacent_find (predicate)
//   function: as above but adjacent equality is determined by the
// user-supplied binary predicate _pred.
template<typename ForwardIt,
         typename BinaryPred>
RE_STD_CONSTEXPR_CPP14 ForwardIt
adjacent_find(
    ForwardIt  _first,
    ForwardIt  _last,
    BinaryPred _pred
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
        if (_pred(*_first, *_next))
        {
            return _first;
        }
    }

    return _last;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_ADJACENT_FIND_HPP
