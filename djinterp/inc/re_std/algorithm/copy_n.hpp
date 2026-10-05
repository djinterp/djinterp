/*******************************************************************************
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
* path:      /inc/re_std/algorithm/copy_n.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_COPY_N_HPP
#define RE_STD_ALGORITHM_COPY_N_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   COPY_N
// ===========================================================================

// copy_n
//   function: copies the first _n elements starting at _first to
// the output range starting at _d_first. Returns the iterator one past
// the last element written.
template<typename InputIt,
         typename Size,
         typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt
copy_n(
    InputIt  _first,
    Size     _n,
    OutputIt _d_first
)
{
    for (; _n > 0; --_n, (void)++_first, (void)++_d_first)
    {
        *_d_first = *_first;
    }

    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_COPY_N_HPP
