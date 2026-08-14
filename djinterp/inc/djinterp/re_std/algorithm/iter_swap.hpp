/******************************************************************************
* djinterp [restd]                                                 iter_swap.hpp
*
* iter_swap algorithm header:
*   Swaps the values pointed to by two iterators. Uses the ADL-friendly
* idiom (using restd::swap; swap(*a, *b);) so that a user-defined
* swap for the value type, if found by ADL, is preferred over
* restd::swap.
*
*   PORTABILITY:
*   - std::iter_swap is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/iter_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_ITER_SWAP_
#define DJINTERP_RESTD_ALGORITHM_ITER_SWAP_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
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
// restd::swap if none is found.
template<typename _ForwardIt1,
         typename _ForwardIt2>
D_CONSTEXPR_CPP14 void
iter_swap(
    _ForwardIt1 _a,
    _ForwardIt2 _b
)
{
    using restd::swap;
    swap(*_a, *_b);
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_ITER_SWAP_
