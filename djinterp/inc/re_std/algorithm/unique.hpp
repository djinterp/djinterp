/*******************************************************************************
* djinterp [re_std]                                                   unique.hpp
*
* unique algorithm header:
*   In-place elimination of consecutive duplicates from [_first, _last).
* Returns the iterator one past the last KEPT element; elements in
* [returned, _last) are in valid-but-unspecified state. Two overloads:
*   - default operator==
*   - custom binary predicate
*
*   PORTABILITY:
*   - std::unique is C++98. C++11 strengthened the kept-element
*     transfer from copy to move. re_std matches per-tier (copy on
*     C++98/03, move on C++11+ via RE_STD_LANG_HAS_RVALUE_REFERENCES).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/unique.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_UNIQUE_HPP
#define RE_STD_ALGORITHM_UNIQUE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "../utility/move.hpp"
#endif


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   UNIQUE (DEFAULT ==)
// ===========================================================================

// unique
//   function: collapses consecutive runs of equal elements in
// [_first, _last) to a single representative each. Returns one past
// the last kept element. The tail [returned, _last) is in
// valid-but-unspecified state.
template<typename ForwardIt>
RE_STD_CONSTEXPR_CPP14 ForwardIt
unique(
    ForwardIt _first,
    ForwardIt _last
)
{
    if (_first == _last)
    {
        return _last;
    }

    ForwardIt _result = _first;
    ForwardIt _it     = _first;
    ++_it;

    for (; _it != _last; ++_it)
    {
        if (!(*_result == *_it))
        {
            ++_result;
            if (_result != _it)
            {
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
                *_result = re_std::move(*_it);
#else
                *_result = *_it;
#endif
            }
        }
    }

    ++_result;
    return _result;
}


// ===========================================================================
// II.  UNIQUE (CUSTOM PRED)
// ===========================================================================

// unique (predicate)
//   function: as above but adjacent equality is determined by the
// user-supplied binary predicate _pred.
template<typename ForwardIt,
         typename BinaryPred>
RE_STD_CONSTEXPR_CPP14 ForwardIt
unique(
    ForwardIt  _first,
    ForwardIt  _last,
    BinaryPred _pred
)
{
    if (_first == _last)
    {
        return _last;
    }

    ForwardIt _result = _first;
    ForwardIt _it     = _first;
    ++_it;

    for (; _it != _last; ++_it)
    {
        if (!_pred(*_result, *_it))
        {
            ++_result;
            if (_result != _it)
            {
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
                *_result = re_std::move(*_it);
#else
                *_result = *_it;
#endif
            }
        }
    }

    ++_result;
    return _result;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_UNIQUE_HPP
