/******************************************************************************
* djinterp [restd]                                               find_if_not.hpp
*
* find_if_not algorithm header:
*   Linear search for the first element in [_first, _last) for which
* the unary predicate _pred returns false. Inverse of find_if. Returns
* _last on no-match.
*
*   PORTABILITY:
*   - std::find_if_not is C++11; restd back-ports to C++98 (pure
*     predicate negation; no language blocker).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/find_if_not.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_FIND_IF_NOT_
#define DJINTERP_RESTD_ALGORITHM_FIND_IF_NOT_ 1

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
// I.   FIND_IF_NOT
// ===========================================================================

// find_if_not
//   function: returns the first iterator it in [_first, _last) such
// that _pred(*it) is false, or _last on no-match.
template<typename _InputIt,
         typename _Pred>
D_CONSTEXPR_CPP14 _InputIt
find_if_not(
    _InputIt _first,
    _InputIt _last,
    _Pred    _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (!_pred(*_first))
        {
            return _first;
        }
    }

    return _last;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_FIND_IF_NOT_
