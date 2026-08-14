/******************************************************************************
* djinterp [restd]                                                   find_if.hpp
*
* find_if algorithm header:
*   Linear search for the first element in [_first, _last) satisfying
* the unary predicate _pred. Returns _last on no-match.
*
*   PORTABILITY:
*   - std::find_if is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/find_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_FIND_IF_
#define DJINTERP_RESTD_ALGORITHM_FIND_IF_ 1

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
// I.   FIND_IF
// ===========================================================================

// find_if
//   function: returns the first iterator it in [_first, _last) such
// that _pred(*it) holds, or _last on no-match.
template<typename _InputIt,
         typename _Pred>
D_CONSTEXPR_CPP14 _InputIt
find_if(
    _InputIt _first,
    _InputIt _last,
    _Pred    _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            return _first;
        }
    }

    return _last;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_FIND_IF_
