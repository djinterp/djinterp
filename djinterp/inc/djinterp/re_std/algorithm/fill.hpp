/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/fill.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_FILL_
#define DJINTERP_RE_STD_ALGORITHM_FILL_ 1

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
// I.   FILL
// ===========================================================================

// fill
//   function: assigns _value to every element in [_first, _last).
template<typename _ForwardIt,
         typename _Type>
D_CONSTEXPR_CPP14 void
fill(
    _ForwardIt   _first,
    _ForwardIt   _last,
    const _Type& _value
)
{
    for (; _first != _last; ++_first)
    {
        *_first = _value;
    }
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_FILL_
