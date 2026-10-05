/*******************************************************************************
* djinterp [re_std]                                                 generate.hpp
*
* generate algorithm header:
*   Assigns the result of successive _g() calls to every element of
* [_first, _last). _g is invoked _last - _first times in sequence; it
* may carry state across calls.
*
*   PORTABILITY:
*   - std::generate is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/generate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_GENERATE_HPP
#define RE_STD_ALGORITHM_GENERATE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   GENERATE
// ===========================================================================

// generate
//   function: assigns the result of _g() to every element of
// [_first, _last) in sequence.
template<typename ForwardIt,
         typename Gen>
RE_STD_CONSTEXPR_CPP14 void
generate(
    ForwardIt _first,
    ForwardIt _last,
    Gen       _g
)
{
    for (; _first != _last; ++_first)
    {
        *_first = _g();
    }
}


}  // re_std


#endif  // RE_STD_ALGORITHM_GENERATE_HPP
