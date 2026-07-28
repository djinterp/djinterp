/******************************************************************************
* djinterp [restd]                                             copy_backward.hpp
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
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/restd/algorithm/copy_backward.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_COPY_BACKWARD_
#define DJINTERP_RESTD_ALGORITHM_COPY_BACKWARD_ 1

#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
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
// I.   COPY_BACKWARD
// ===========================================================================

// copy_backward
//   function: copies [_first, _last) into the range ending at _d_last,
// proceeding from the back so that the last source element lands at
// _d_last - 1. Returns the iterator one before the first element
// written (i.e. the new beginning of the destination range).
template<typename _BidirIt1,
         typename _BidirIt2>
D_CONSTEXPR_CPP14 _BidirIt2
copy_backward(
    _BidirIt1 _first,
    _BidirIt1 _last,
    _BidirIt2 _d_last
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


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_COPY_BACKWARD_
