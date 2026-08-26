/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/rotate_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_ROTATE_COPY_
#define DJINTERP_RE_STD_ALGORITHM_ROTATE_COPY_ 1

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
// I.   ROTATE_COPY
// ===========================================================================

// rotate_copy
//   function: writes [_middle, _last) followed by [_first, _middle)
// into the output range starting at _d_first. Returns the iterator one
// past the last element written.
template<typename _ForwardIt,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
rotate_copy(
    _ForwardIt _first,
    _ForwardIt _middle,
    _ForwardIt _last,
    _OutputIt  _d_first
)
{
    // first segment: [_middle, _last)
    for (_ForwardIt _it = _middle; _it != _last; ++_it, (void)++_d_first)
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


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_ROTATE_COPY_
