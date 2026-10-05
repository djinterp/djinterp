/*******************************************************************************
* djinterp [re_std]                                            copy_backward.hpp
*
* copy_backward algorithm header:
*   Copies elements from [_first, _last) into the range ending at
* _d_last, walking in reverse so that elements adjacent to the source's
* tail end up adjacent to the destination's tail. Useful when source
* and destination overlap and _d_last lies inside (_first, _last].
* Returns the iterator pointing to the FIRST element of the destination
* range (i.e. _d_last - (_last - _first)).
*
*   PORTABILITY:
*   - std::copy_backward is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/copy_backward.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_COPY_BACKWARD_HPP
#define RE_STD_ALGORITHM_COPY_BACKWARD_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   COPY_BACKWARD
// ===========================================================================

// copy_backward
//   function: copies [_first, _last) into the range ending at _d_last,
// proceeding from the back so that the last source element lands at
// _d_last - 1. Returns the iterator one before the first element
// written (i.e. the new beginning of the destination range).
template<typename BidirIt1,
         typename BidirIt2>
RE_STD_CONSTEXPR_CPP14 BidirIt2
copy_backward(
    BidirIt1 _first,
    BidirIt1 _last,
    BidirIt2 _d_last
)
{
    while (_first != _last)
    {
        --_last;
        --_d_last;
        *_d_last = *_last;
    }

    return _d_last;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_COPY_BACKWARD_HPP
