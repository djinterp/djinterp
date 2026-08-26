/******************************************************************************
* djinterp [re_std]                                                   fill_n.hpp
*
* fill_n algorithm header:
*   Assigns _value to the first _n elements of the range starting at
* _first. Returns the iterator one past the last element assigned.
* Non-positive _n is a no-op (returns _first).
*
*   PORTABILITY:
*   - std::fill_n is C++98 but did not return an iterator until C++11.
*     re_std ships the C++11 (iterator-returning) signature on every
*     tier; users wanting the void-returning C++98 form can ignore the
*     return.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/fill_n.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_FILL_N_
#define DJINTERP_RE_STD_ALGORITHM_FILL_N_ 1

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
// I.   FILL_N
// ===========================================================================

// fill_n
//   function: assigns _value to the first _n elements starting at
// _first. Returns the iterator one past the last element assigned, or
// _first unchanged for non-positive _n.
template<typename _OutputIt,
         typename _Size,
         typename _Type>
D_CONSTEXPR_CPP14 _OutputIt
fill_n(
    _OutputIt    _first,
    _Size        _n,
    const _Type& _value
)
{
    for (; _n > 0; --_n, (void)++_first)
    {
        *_first = _value;
    }

    return _first;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_FILL_N_
