/******************************************************************************
* djinterp [restd]                                             adjacent_find.hpp
*
* adjacent_find algorithm header:
*   Returns the first iterator it in [_first, _last) such that *it ==
* *(it + 1) (or _pred(*it, *(it + 1)) holds, in the predicate overload).
* Returns _last on no-match, including the empty-range case.
*
*   PORTABILITY:
*   - std::adjacent_find is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/restd/algorithm/adjacent_find.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_ADJACENT_FIND_
#define DJINTERP_RESTD_ALGORITHM_ADJACENT_FIND_ 1

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
// I.   ADJACENT_FIND (DEFAULT ==)
// ===========================================================================

// adjacent_find
//   function: returns the first iterator it in [_first, _last) such
// that *it == *(it + 1). Returns _last for empty or one-element ranges
// and on no-match.
template<typename _ForwardIt>
D_CONSTEXPR_CPP14 _ForwardIt
adjacent_find(
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
template<typename _ForwardIt,
         typename _BinaryPred>
D_CONSTEXPR_CPP14 _ForwardIt
adjacent_find(
    _ForwardIt  _first,
    _ForwardIt  _last,
    _BinaryPred _pred
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
        if (_pred(*_first, *_next))
        {
            return _first;
        }
    }

    return _last;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_ADJACENT_FIND_
