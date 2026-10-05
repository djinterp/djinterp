/*******************************************************************************
* djinterp [re_std]                                           is_partitioned.hpp
*
* is_partitioned algorithm header:
*   Returns true if [_first, _last) is partitioned w.r.t. _pred — that
* is, every element for which _pred returns true precedes every element
* for which _pred returns false. Vacuously true for empty / one-element
* ranges.
*
*   PORTABILITY:
*   - std::is_partitioned is C++11; re_std back-ports to C++98 (no
*     language blocker — just two linear scans).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/is_partitioned.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_IS_PARTITIONED_HPP
#define RE_STD_ALGORITHM_IS_PARTITIONED_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   IS_PARTITIONED
// ===========================================================================

// is_partitioned
//   function: returns true if every true-element precedes every
// false-element under _pred. Skip past the leading run of trues, then
// verify the remaining suffix is all falses.
template<typename InputIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 bool
is_partitioned(
    InputIt _first,
    InputIt _last,
    Pred    _pred
)
{
    // skip the leading run of true-elements
    while ( (_first != _last) &&
            _pred(*_first) )
    {
        ++_first;
    }

    // every subsequent element must be false
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


#endif  // RE_STD_ALGORITHM_IS_PARTITIONED_HPP
