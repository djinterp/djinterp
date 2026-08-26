/******************************************************************************
* djinterp [re_std]                                                   copy_n.hpp
*
* copy_n algorithm header:
*   Copies the first _n elements starting at _first into the output
* range starting at _d_first. Returns the iterator one past the last
* element written. Non-positive _n is a no-op (returns _d_first).
*
*   PORTABILITY:
*   - std::copy_n is C++11; re_std back-ports to C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/copy_n.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_COPY_N_
#define DJINTERP_RE_STD_ALGORITHM_COPY_N_ 1

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
// I.   COPY_N
// ===========================================================================

// copy_n
//   function: copies the first _n elements starting at _first to
// the output range starting at _d_first. Returns the iterator one past
// the last element written.
template<typename _InputIt,
         typename _Size,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
copy_n(
    _InputIt  _first,
    _Size     _n,
    _OutputIt _d_first
)
{
    for (; _n > 0; --_n, (void)++_first, (void)++_d_first)
    {
        *_d_first = *_first;
    }

    return _d_first;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_COPY_N_
