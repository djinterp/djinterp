/*******************************************************************************
* djinterp [re_std]                                                  reverse.hpp
*
* reverse algorithm header:
*   In-place reversal of the elements in [_first, _last). Walks
* inward from both ends and iter_swaps each pair.
*
*   PORTABILITY:
*   - std::reverse is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Requires bidirectional iterators.
*
*
* path:      /inc/re_std/algorithm/reverse.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_REVERSE_HPP
#define RE_STD_ALGORITHM_REVERSE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./iter_swap.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   REVERSE
// ===========================================================================

// reverse
//   function: reverses the elements of [_first, _last) in place. The
// loop terminates when the two-pointer walk meets or crosses.
template<typename BidirIt>
RE_STD_CONSTEXPR_CPP14 void
reverse(
    BidirIt _first,
    BidirIt _last
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


}  // re_std


#endif  // RE_STD_ALGORITHM_REVERSE_HPP
