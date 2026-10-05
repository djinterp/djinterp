/*******************************************************************************
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
* path:      /inc/re_std/algorithm/swap_ranges.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_SWAP_RANGES_HPP
#define RE_STD_ALGORITHM_SWAP_RANGES_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./iter_swap.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   SWAP_RANGES
// ===========================================================================

// swap_ranges
//   function: swaps each element in [_first1, _last1) with the
// corresponding element starting at _first2. Returns the iterator one
// past the last swapped element of the second range.
template<typename ForwardIt1,
         typename ForwardIt2>
RE_STD_CONSTEXPR_CPP14 ForwardIt2
swap_ranges(
    ForwardIt1 _first1,
    ForwardIt1 _last1,
    ForwardIt2 _first2
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        iter_swap(_first1, _first2);
    }

    return _first2;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_SWAP_RANGES_HPP
