/******************************************************************************
* djinterp [re_std]                                                 generate.hpp
*
* generate algorithm header:
*   Assigns the result of successive _g() calls to every element of
* [_first, _last). _g is invoked _last - _first times in sequence; it
* may carry state across calls.
*
*   PORTABILITY:
*   - std::generate is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/generate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_GENERATE_
#define DJINTERP_RE_STD_ALGORITHM_GENERATE_ 1

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
// I.   GENERATE
// ===========================================================================

// generate
//   function: assigns the result of _g() to every element of
// [_first, _last) in sequence.
template<typename _ForwardIt,
         typename _Gen>
D_CONSTEXPR_CPP14 void
generate(
    _ForwardIt _first,
    _ForwardIt _last,
    _Gen       _g
)
{
    for (; _first != _last; ++_first)
    {
        *_first = _g();
    }
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_GENERATE_
