/*******************************************************************************
* djinterp [re_std]                                                  find_if.hpp
*
* find_if algorithm header:
*   Linear search for the first element in [_first, _last) satisfying
* the unary predicate _pred. Returns _last on no-match.
*
*   PORTABILITY:
*   - std::find_if is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/find_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_FIND_IF_HPP
#define RE_STD_ALGORITHM_FIND_IF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   FIND_IF
// ===========================================================================

// find_if
//   function: returns the first iterator it in [_first, _last) such
// that _pred(*it) holds, or _last on no-match.
template<typename InputIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 InputIt
find_if(
    InputIt _first,
    InputIt _last,
    Pred    _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            return _first;
        }
    }

    return _last;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_FIND_IF_HPP
