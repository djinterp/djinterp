/*******************************************************************************
* djinterp [re_std]                                                     copy.hpp
*
* copy algorithm header:
*   Copies elements from [_first, _last) to the output range starting
* at _d_first. Returns the iterator one past the last copied element
* (i.e. _d_first + (_last - _first)).
*
*   PORTABILITY:
*   - std::copy is C++98.
*   - The (void) cast on the output increment guards against
*     operator-comma overloads on proxy iterators.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_COPY_HPP
#define RE_STD_ALGORITHM_COPY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   COPY
// ===========================================================================

// copy
//   function: copies [_first, _last) into [_d_first, _d_first + N).
// Returns one past the last copied element. Source and destination
// must not overlap unless _d_first is outside [_first, _last).
template<typename InputIt,
         typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt
copy(
    InputIt  _first,
    InputIt  _last,
    OutputIt _d_first
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        *_d_first = *_first;
    }

    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_COPY_HPP
