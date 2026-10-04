/******************************************************************************
* djinterp [re_std]                                               generate_n.hpp
*
* generate_n algorithm header:
*   Assigns the result of successive _g() calls to the first _n
* elements of the range starting at _first. Returns the iterator one
* past the last element written. Non-positive _n is a no-op.
*
*   PORTABILITY:
*   - std::generate_n is C++98 but did not return an iterator until
*     C++11; re_std ships the C++11 signature on every tier.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/generate_n.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_GENERATE_N_
#define DJINTERP_RE_STD_ALGORITHM_GENERATE_N_ 1

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
// I.   GENERATE_N
// ===========================================================================

// generate_n
//   function: assigns the result of _g() to the first _n elements
// starting at _first in sequence. Returns the iterator one past the
// last element written, or _first unchanged for non-positive _n.
template<typename _OutputIt,
         typename _Size,
         typename _Gen>
D_CONSTEXPR_CPP14 _OutputIt
generate_n(
    _OutputIt _first,
    _Size     _n,
    _Gen      _g
)
{
    for (; _n > 0; --_n, (void)++_first)
    {
        *_first = _g();
    }

    return _first;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_GENERATE_N_
