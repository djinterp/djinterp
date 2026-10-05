/*******************************************************************************
* djinterp [re_std]                                           remove_copy_if.hpp
*
* remove_copy_if algorithm header:
*   Out-of-place sibling of remove_if. Copies elements from
* [_first, _last) to the output range starting at _d_first, skipping
* those for which _pred returns true. Returns the iterator one past the
* last element written.
*
*   PORTABILITY:
*   - std::remove_copy_if is C++98.
*   - Pure copy semantics.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/remove_copy_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_REMOVE_COPY_IF_HPP
#define RE_STD_ALGORITHM_REMOVE_COPY_IF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   REMOVE_COPY_IF
// ===========================================================================

// remove_copy_if
//   function: copies every element of [_first, _last) for which
// _pred returns false into the output range starting at _d_first.
// Returns the iterator one past the last element written.
template<typename InputIt,
         typename OutputIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 OutputIt
remove_copy_if(
    InputIt  _first,
    InputIt  _last,
    OutputIt _d_first,
    Pred     _pred
)
{
    for (; _first != _last; ++_first)
    {
        if (!_pred(*_first))
        {
            *_d_first = *_first;
            ++_d_first;
        }
    }

    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_REMOVE_COPY_IF_HPP
