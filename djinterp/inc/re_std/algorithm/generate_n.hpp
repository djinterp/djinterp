/*******************************************************************************
* djinterp [re_std]                                               generate_n.hpp
*
* generate_n algorithm header:
*   Assigns the result of successive _g() calls to the first _n
* elements of the range starting at _first. Returns the iterator one
* past the last element written. Non-positive _n is a no-op.
*
*   PORTABILITY:
*   - std::generate_n is C++98 but did not return an iterator until
*     C++11; re_std ships the C++11 signature on every tier.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/generate_n.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_GENERATE_N_HPP
#define RE_STD_ALGORITHM_GENERATE_N_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   GENERATE_N
// ===========================================================================

// generate_n
//   function: assigns the result of _g() to the first _n elements
// starting at _first in sequence. Returns the iterator one past the
// last element written, or _first unchanged for non-positive _n.
template<typename OutputIt,
         typename Size,
         typename Gen>
RE_STD_CONSTEXPR_CPP14 OutputIt
generate_n(
    OutputIt _first,
    Size     _n,
    Gen      _g
)
{
    for (; _n > 0; --_n, (void)++_first)
    {
        *_first = _g();
    }

    return _first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_GENERATE_N_HPP
