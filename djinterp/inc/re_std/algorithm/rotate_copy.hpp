/*******************************************************************************
* djinterp [re_std]                                              rotate_copy.hpp
*
* rotate_copy algorithm header:
*   Out-of-place sibling of rotate. Copies [_middle, _last) followed
* by [_first, _middle) into the range starting at _d_first. Returns the
* iterator one past the last element written.
*
*   PORTABILITY:
*   - std::rotate_copy is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Composes copy() twice — once for the [middle, last) prefix of the
*     output, once for the [first, middle) suffix. Implemented inline
*     to avoid the include dependency on copy.hpp; the loops are
*     trivial.
*
*
* path:      /inc/re_std/algorithm/rotate_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_ROTATE_COPY_HPP
#define RE_STD_ALGORITHM_ROTATE_COPY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   ROTATE_COPY
// ===========================================================================

// rotate_copy
//   function: writes [_middle, _last) followed by [_first, _middle)
// into the output range starting at _d_first. Returns the iterator one
// past the last element written.
template<typename ForwardIt,
         typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt
rotate_copy(
    ForwardIt _first,
    ForwardIt _middle,
    ForwardIt _last,
    OutputIt  _d_first
)
{
    // first segment: [_middle, _last)
    for (ForwardIt _it = _middle; _it != _last; ++_it, (void)++_d_first)
    {
        *_d_first = *_it;
    }

    // second segment: [_first, _middle)
    for (; _first != _middle; ++_first, (void)++_d_first)
    {
        *_d_first = *_first;
    }

    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_ROTATE_COPY_HPP
