/*******************************************************************************
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
* path:      /inc/re_std/algorithm/iter_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_ITER_SWAP_HPP
#define RE_STD_ALGORITHM_ITER_SWAP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "../utility/swap.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   ITER_SWAP
// ===========================================================================

// iter_swap
//   function: exchanges *_a and *_b. The unqualified swap call picks
// up user-supplied swap overloads via ADL, falling back to
// re_std::swap if none is found.
template<typename ForwardIt1,
         typename ForwardIt2>
RE_STD_CONSTEXPR_CPP14 void
iter_swap(
    ForwardIt1 _a,
    ForwardIt2 _b
)
{
    using re_std::swap;
    swap(*_a, *_b);
}


}  // re_std


#endif  // RE_STD_ALGORITHM_ITER_SWAP_HPP
