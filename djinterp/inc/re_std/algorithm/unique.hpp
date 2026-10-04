/******************************************************************************
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
*     C++98/03, move on C++11+ via D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/unique.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_UNIQUE_
#define DJINTERP_RE_STD_ALGORITHM_UNIQUE_ 1

// djinterp
#include "../../core/djinterp.hpp"
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "../utility/move.hpp"
#endif


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
// I.   UNIQUE (DEFAULT ==)
// ===========================================================================

// unique
//   function: collapses consecutive runs of equal elements in
// [_first, _last) to a single representative each. Returns one past
// the last kept element. The tail [returned, _last) is in
// valid-but-unspecified state.
template<typename _ForwardIt>
D_CONSTEXPR_CPP14 _ForwardIt
unique(
    _ForwardIt _first,
    _ForwardIt _last
)
{
    if (_first == _last)
    {
        return _last;
    }

    _ForwardIt _result = _first;
    _ForwardIt _it     = _first;
    ++_it;

    for (; _it != _last; ++_it)
    {
        if (!(*_result == *_it))
        {
            ++_result;
            if (_result != _it)
            {
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
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
template<typename _ForwardIt,
         typename _BinaryPred>
D_CONSTEXPR_CPP14 _ForwardIt
unique(
    _ForwardIt  _first,
    _ForwardIt  _last,
    _BinaryPred _pred
)
{
    if (_first == _last)
    {
        return _last;
    }

    _ForwardIt _result = _first;
    _ForwardIt _it     = _first;
    ++_it;

    for (; _it != _last; ++_it)
    {
        if (!_pred(*_result, *_it))
        {
            ++_result;
            if (_result != _it)
            {
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
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


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_UNIQUE_
