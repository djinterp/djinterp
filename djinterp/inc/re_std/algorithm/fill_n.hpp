/*******************************************************************************
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
* path:      /inc/re_std/algorithm/fill_n.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_FILL_N_HPP
#define RE_STD_ALGORITHM_FILL_N_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   FILL_N
// ===========================================================================

// fill_n
//   function: assigns _value to the first _n elements starting at
// _first. Returns the iterator one past the last element assigned, or
// _first unchanged for non-positive _n.
template<typename OutputIt,
         typename Size,
         typename Type>
RE_STD_CONSTEXPR_CPP14 OutputIt
fill_n(
    OutputIt    _first,
    Size        _n,
    const Type& _value
)
{
    for (; _n > 0; --_n, (void)++_first)
    {
        *_first = _value;
    }

    return _first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_FILL_N_HPP
