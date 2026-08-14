/******************************************************************************
* djinterp [restd]                                            is_partitioned.hpp
*
* is_partitioned algorithm header:
*   Returns true if [_first, _last) is partitioned w.r.t. _pred — that
* is, every element for which _pred returns true precedes every element
* for which _pred returns false. Vacuously true for empty / one-element
* ranges.
*
*   PORTABILITY:
*   - std::is_partitioned is C++11; restd back-ports to C++98 (no
*     language blocker — just two linear scans).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/is_partitioned.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_IS_PARTITIONED_
#define DJINTERP_RESTD_ALGORITHM_IS_PARTITIONED_ 1

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
// I.   IS_PARTITIONED
// ===========================================================================

// is_partitioned
//   function: returns true if every true-element precedes every
// false-element under _pred. Skip past the leading run of trues, then
// verify the remaining suffix is all falses.
template<typename _InputIt,
         typename _Pred>
D_CONSTEXPR_CPP14 bool
is_partitioned(
    _InputIt _first,
    _InputIt _last,
    _Pred    _pred
)
{
    // skip the leading run of true-elements
    while ( (_first != _last) &&
            _pred(*_first) )
    {
        ++_first;
    }

    // every subsequent element must be false
    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            return false;
        }
    }

    return true;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_IS_PARTITIONED_
