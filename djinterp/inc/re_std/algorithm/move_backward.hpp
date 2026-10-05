/*******************************************************************************
* djinterp [re_std]                                            move_backward.hpp
*
* move_backward algorithm header:
*   Moves elements from [_first, _last) into the range ending at
* _d_last, walking in reverse so that the last source element lands at
* _d_last - 1. Useful when source and destination overlap and _d_last
* lies inside (_first, _last]. Returns the iterator pointing to the
* first element of the destination range (i.e. _d_last - (_last - _first)).
*
*   PORTABILITY:
*   - std::move_backward is C++11. Same rationale as move (algorithm):
*     gated on rvalue references; not back-ported to C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/move_backward.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_MOVE_BACKWARD_HPP
#define RE_STD_ALGORITHM_MOVE_BACKWARD_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   GATE: rvalue references required
// ===========================================================================

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

// re_std
#include "../utility/move.hpp"


// ===========================================================================
// 1.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   MOVE_BACKWARD
// ===========================================================================

// move_backward
//   function: moves [_first, _last) into the range ending at _d_last,
// proceeding from the back. Returns the iterator one before the first
// element written.
template<typename BidirIt1,
         typename BidirIt2>
RE_STD_CONSTEXPR_CPP14 BidirIt2
move_backward(
    BidirIt1 _first,
    BidirIt1 _last,
    BidirIt2 _d_last
)
{
    while (_first != _last)
    {
        --_last;
        --_d_last;
        *_d_last = re_std::move(*_last);
    }

    return _d_last;
}


}  // re_std


#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES


#endif  // RE_STD_ALGORITHM_MOVE_BACKWARD_HPP
