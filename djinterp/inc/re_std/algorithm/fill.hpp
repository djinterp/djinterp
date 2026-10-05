/*******************************************************************************
* djinterp [re_std]                                                     fill.hpp
*
* fill algorithm header:
*   Assigns _value to every element in [_first, _last).
*
*   PORTABILITY:
*   - std::fill is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/fill.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_FILL_HPP
#define RE_STD_ALGORITHM_FILL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   FILL
// ===========================================================================

// fill
//   function: assigns _value to every element in [_first, _last).
template<typename ForwardIt,
         typename Type>
RE_STD_CONSTEXPR_CPP14 void
fill(
    ForwardIt   _first,
    ForwardIt   _last,
    const Type& _value
)
{
    for (; _first != _last; ++_first)
    {
        *_first = _value;
    }
}


}  // re_std


#endif  // RE_STD_ALGORITHM_FILL_HPP
