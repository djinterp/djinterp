/******************************************************************************
* djinterp [restd]                                                 remove_if.hpp
*
* remove_if algorithm header:
*   In-place compaction. Walks [_first, _last) and pulls every element
* for which _pred returns false forward to overwrite the removed
* positions. Returns the iterator one past the last KEPT element.
*
*   PORTABILITY:
*   - std::remove_if is C++98. C++11 strengthened to move assignment.
*     restd matches per-tier (copy on C++98, move on C++11+).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Forwards through find_if for the skip-prefix scan.
*
*
* path:      /inc/djinterp/re_std/algorithm/remove_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_REMOVE_IF_
#define DJINTERP_RESTD_ALGORITHM_REMOVE_IF_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./find_if.hpp"
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
// I.   REMOVE_IF
// ===========================================================================

// remove_if
//   function: in-place compaction by predicate. Returns the iterator
// one past the last kept element. Kept elements retain their relative
// order; the tail [returned, _last) is in valid-but-unspecified state.
template<typename _ForwardIt,
         typename _Pred>
D_CONSTEXPR_CPP14 _ForwardIt
remove_if(
    _ForwardIt _first,
    _ForwardIt _last,
    _Pred      _pred
)
{
    // skip the matchless prefix
    _first = restd::find_if(_first, _last, _pred);
    if (_first == _last)
    {
        return _first;
    }

    _ForwardIt _it = _first;
    ++_it;

    for (; _it != _last; ++_it)
    {
        if (!_pred(*_it))
        {
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
            *_first = restd::move(*_it);
#else
            *_first = *_it;
#endif
            ++_first;
        }
    }

    return _first;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_REMOVE_IF_
