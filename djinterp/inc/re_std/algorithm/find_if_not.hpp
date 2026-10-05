/*******************************************************************************
* djinterp [re_std]                                              find_if_not.hpp
*
* find_if_not algorithm header:
*   Linear search for the first element in [_first, _last) for which
* the unary predicate _pred returns false. Inverse of find_if. Returns
* _last on no-match.
*
*   PORTABILITY:
*   - std::find_if_not is C++11; re_std back-ports to C++98 (pure
*     predicate negation; no language blocker).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/find_if_not.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_FIND_IF_NOT_HPP
#define RE_STD_ALGORITHM_FIND_IF_NOT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   FIND_IF_NOT
// ===========================================================================

// find_if_not
//   function: returns the first iterator it in [_first, _last) such
// that _pred(*it) is false, or _last on no-match.
template<typename InputIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 InputIt
find_if_not(
    InputIt _first,
    InputIt _last,
    Pred    _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (!_pred(*_first))
        {
            return _first;
        }
    }

    return _last;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_FIND_IF_NOT_HPP
