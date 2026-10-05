/*******************************************************************************
* djinterp [re_std]                                                  copy_if.hpp
*
* copy_if algorithm header:
*   Copies elements from [_first, _last) for which _pred returns true
* into the output range starting at _d_first. Returns the iterator one
* past the last copied element.
*
*   PORTABILITY:
*   - std::copy_if is C++11; re_std back-ports to C++98 (no language
*     blocker — just predicate + conditional assign).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/copy_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_COPY_IF_HPP
#define RE_STD_ALGORITHM_COPY_IF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   COPY_IF
// ===========================================================================

// copy_if
//   function: copies every element of [_first, _last) for which
// _pred(*it) holds into the output range starting at _d_first. Returns
// the iterator one past the last element written.
template<typename InputIt,
         typename OutputIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 OutputIt
copy_if(
    InputIt  _first,
    InputIt  _last,
    OutputIt _d_first,
    Pred     _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            *_d_first = *_first;
            ++_d_first;
        }
    }

    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_COPY_IF_HPP
