/******************************************************************************
* djinterp [restd]                                                    any_of.hpp
*
* any_of algorithm header:
*   Returns true if the unary predicate _pred holds for at least one
* element in the range [_first, _last). False for an empty range. Short-
* circuits on the first true.
*
*   PORTABILITY:
*   - std::any_of is C++11; restd back-ports to C++98 (no language blocker).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/restd/algorithm/any_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_ANY_OF_
#define DJINTERP_RESTD_ALGORITHM_ANY_OF_ 1

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
// I.   ANY_OF
// ===========================================================================

// any_of
//   function: returns true if _pred holds for at least one element in
// [_first, _last); false for an empty range.
template<typename _InputIt,
         typename _Pred>
D_CONSTEXPR_CPP14 bool
any_of(
    _InputIt _first,
    _InputIt _last,
    _Pred    _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            return true;
        }
    }

    return false;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_ANY_OF_
