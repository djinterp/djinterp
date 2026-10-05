/*******************************************************************************
* djinterp [re_std]                                                   any_of.hpp
*
* any_of algorithm header:
*   Returns true if the unary predicate _pred holds for at least one
* element in the range [_first, _last). False for an empty range. Short-
* circuits on the first true.
*
*   PORTABILITY:
*   - std::any_of is C++11; re_std back-ports to C++98 (no language blocker).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/any_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_ANY_OF_HPP
#define RE_STD_ALGORITHM_ANY_OF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   ANY_OF
// ===========================================================================

// any_of
//   function: returns true if _pred holds for at least one element in
// [_first, _last); false for an empty range.
template<typename InputIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 bool
any_of(
    InputIt _first,
    InputIt _last,
    Pred    _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            return true;
        }
    }

    return false;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_ANY_OF_HPP
