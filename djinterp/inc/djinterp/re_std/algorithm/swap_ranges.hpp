/******************************************************************************
* djinterp [re_std]                                              swap_ranges.hpp
*
* swap_ranges algorithm header:
*   Exchanges elements in [_first1, _last1) with the parallel range
* starting at _first2. Returns the iterator one past the last swapped
* element in the second range. The ranges must not overlap.
*
*   PORTABILITY:
*   - std::swap_ranges is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/swap_ranges.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_SWAP_RANGES_
#define DJINTERP_RE_STD_ALGORITHM_SWAP_RANGES_ 1

// djinterp
#include "../../core/djinterp.hpp"
// re_std
#include "./iter_swap.hpp"


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
// I.   SWAP_RANGES
// ===========================================================================

// swap_ranges
//   function: swaps each element in [_first1, _last1) with the
// corresponding element starting at _first2. Returns the iterator one
// past the last swapped element of the second range.
template<typename _ForwardIt1,
         typename _ForwardIt2>
D_CONSTEXPR_CPP14 _ForwardIt2
swap_ranges(
    _ForwardIt1 _first1,
    _ForwardIt1 _last1,
    _ForwardIt2 _first2
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        iter_swap(_first1, _first2);
    }

    return _first2;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_SWAP_RANGES_
