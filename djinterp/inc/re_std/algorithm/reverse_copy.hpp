/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/reverse_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_REVERSE_COPY_
#define DJINTERP_RE_STD_ALGORITHM_REVERSE_COPY_ 1

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
// I.   REVERSE_COPY
// ===========================================================================

// reverse_copy
//   function: copies [_first, _last) to _d_first in reverse order.
// Returns the iterator one past the last element written. Source and
// destination must not overlap.
template<typename _BidirIt,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
reverse_copy(
    _BidirIt  _first,
    _BidirIt  _last,
    _OutputIt _d_first
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


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_REVERSE_COPY_
