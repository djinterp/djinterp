/*******************************************************************************
* djinterp [re_std]                                                  none_of.hpp
*
* none_of algorithm header:
*   Returns true if the unary predicate _pred holds for no element in
* the range [_first, _last). Vacuously true for an empty range. Short-
* circuits on the first true.
*
*   PORTABILITY:
*   - std::none_of is C++11; re_std back-ports to C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/none_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_NONE_OF_HPP
#define RE_STD_ALGORITHM_NONE_OF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   NONE_OF
// ===========================================================================

// none_of
//   function: returns true if _pred holds for no element in
// [_first, _last); true for an empty range.
template<typename InputIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 bool
none_of(
    InputIt _first,
    InputIt _last,
    Pred    _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            return false;
        }
    }

    return true;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_NONE_OF_HPP
