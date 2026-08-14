/******************************************************************************
* djinterp [restd]                                                    all_of.hpp
*
* all_of algorithm header:
*   Returns true if the unary predicate _pred holds for every element in
* the range [_first, _last). Vacuously true for an empty range. Short-
* circuits on the first false.
*
*   PORTABILITY:
*   - std::all_of is C++11; restd back-ports to C++98 (no language blocker).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14 via relaxed
*     constexpr (the loop body needs mutable iteration).
*
*
* path:      /inc/djinterp/re_std/algorithm/all_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_ALL_OF_
#define DJINTERP_RESTD_ALGORITHM_ALL_OF_ 1

#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================
// D_CONSTEXPR_CPP14: constexpr from C++14 onward; empty on C++98/03/11.
// Required by every loop-bodied algorithm in this module. Local
// redefinition pending a global qualifier-macro-table entry.

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   ALL_OF
// ===========================================================================

// all_of
//   function: returns true if _pred holds for every element in
// [_first, _last), and true for an empty range.
template<typename _InputIt,
         typename _Pred>
D_CONSTEXPR_CPP14 bool
all_of(
    _InputIt _first,
    _InputIt _last,
    _Pred    _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (!_pred(*_first))
        {
            return false;
        }
    }

    return true;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_ALL_OF_
