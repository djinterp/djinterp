/******************************************************************************
* djinterp [restd]                                                   reverse.hpp
*
* reverse algorithm header:
*   In-place reversal of the elements in [_first, _last). Walks
* inward from both ends and iter_swaps each pair.
*
*   PORTABILITY:
*   - std::reverse is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Requires bidirectional iterators.
*
*
* path:      /inc/djinterp/restd/algorithm/reverse.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_REVERSE_
#define DJINTERP_RESTD_ALGORITHM_REVERSE_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./iter_swap.hpp"


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
// I.   REVERSE
// ===========================================================================

// reverse
//   function: reverses the elements of [_first, _last) in place. The
// loop terminates when the two-pointer walk meets or crosses.
template<typename _BidirIt>
D_CONSTEXPR_CPP14 void
reverse(
    _BidirIt _first,
    _BidirIt _last
)
{
    while (_first != _last)
    {
        --_last;
        if (_first == _last)
        {
            break;
        }
        iter_swap(_first, _last);
        ++_first;
    }
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_REVERSE_
