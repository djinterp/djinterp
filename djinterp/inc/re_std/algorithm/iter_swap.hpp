/******************************************************************************
* djinterp [re_std]                                                iter_swap.hpp
*
* iter_swap algorithm header:
*   Swaps the values pointed to by two iterators. Uses the ADL-friendly
* idiom (using re_std::swap; swap(*a, *b);) so that a user-defined
* swap for the value type, if found by ADL, is preferred over
* re_std::swap.
*
*   PORTABILITY:
*   - std::iter_swap is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/iter_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_ITER_SWAP_
#define DJINTERP_RE_STD_ALGORITHM_ITER_SWAP_ 1

// djinterp
#include "../../core/djinterp.hpp"
// re_std
#include "../utility/swap.hpp"


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
// I.   ITER_SWAP
// ===========================================================================

// iter_swap
//   function: exchanges *_a and *_b. The unqualified swap call picks
// up user-supplied swap overloads via ADL, falling back to
// re_std::swap if none is found.
template<typename _ForwardIt1,
         typename _ForwardIt2>
D_CONSTEXPR_CPP14 void
iter_swap(
    _ForwardIt1 _a,
    _ForwardIt2 _b
)
{
    using re_std::swap;
    swap(*_a, *_b);
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_ITER_SWAP_
