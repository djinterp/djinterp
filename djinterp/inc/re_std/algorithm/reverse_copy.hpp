/*******************************************************************************
* djinterp [re_std]                                             reverse_copy.hpp
*
* reverse_copy algorithm header:
*   Out-of-place sibling of reverse. Copies elements from
* [_first, _last) into the range starting at _d_first in reverse
* order. Returns the iterator one past the last element written.
*
*   PORTABILITY:
*   - std::reverse_copy is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Requires bidirectional input iterators.
*
*
* path:      /inc/re_std/algorithm/reverse_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_REVERSE_COPY_HPP
#define RE_STD_ALGORITHM_REVERSE_COPY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   REVERSE_COPY
// ===========================================================================

// reverse_copy
//   function: copies [_first, _last) to _d_first in reverse order.
// Returns the iterator one past the last element written. Source and
// destination must not overlap.
template<typename BidirIt,
         typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt
reverse_copy(
    BidirIt  _first,
    BidirIt  _last,
    OutputIt _d_first
)
{
    while (_first != _last)
    {
        --_last;
        *_d_first = *_last;
        ++_d_first;
    }

    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_REVERSE_COPY_HPP
