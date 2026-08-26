/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/move_backward.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_MOVE_BACKWARD_
#define DJINTERP_RE_STD_ALGORITHM_MOVE_BACKWARD_ 1

#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   GATE: rvalue references required
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// re_std
#include "../utility/move.hpp"


// ===========================================================================
// 1.   COMPATIBILITY MACROS
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
// I.   MOVE_BACKWARD
// ===========================================================================

// move_backward
//   function: moves [_first, _last) into the range ending at _d_last,
// proceeding from the back. Returns the iterator one before the first
// element written.
template<typename _BidirIt1,
         typename _BidirIt2>
D_CONSTEXPR_CPP14 _BidirIt2
move_backward(
    _BidirIt1 _first,
    _BidirIt1 _last,
    _BidirIt2 _d_last
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


NS_END  // re_std


#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


#endif  // DJINTERP_RE_STD_ALGORITHM_MOVE_BACKWARD_
