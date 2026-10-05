/*******************************************************************************
* djinterp [re_std]                                                   all_of.hpp
*
* all_of algorithm header:
*   Returns true if the unary predicate _pred holds for every element in
* the range [_first, _last). Vacuously true for an empty range. Short-
* circuits on the first false.
*
*   PORTABILITY:
*   - std::all_of is C++11; re_std back-ports to C++98 (no language blocker).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14 via relaxed
*     constexpr (the loop body needs mutable iteration).
*
*
* path:      /inc/re_std/algorithm/all_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_ALL_OF_HPP
#define RE_STD_ALGORITHM_ALL_OF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================
// RE_STD_CONSTEXPR_CPP14: constexpr from C++14 onward; empty on C++98/03/11.
// Required by every loop-bodied algorithm in this module. Local
// redefinition pending a global qualifier-macro-table entry.


namespace re_std
{


// ===========================================================================
// I.   ALL_OF
// ===========================================================================

// all_of
//   function: returns true if _pred holds for every element in
// [_first, _last), and true for an empty range.
template<typename InputIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 bool
all_of(
    InputIt _first,
    InputIt _last,
    Pred    _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (!_pred(*_first))
        {
            return false;
        }
    }

    return true;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_ALL_OF_HPP
