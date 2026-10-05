/*******************************************************************************
* djinterp [re_std]                                                     move.hpp
*
* move algorithm header:
*   Moves elements from [_first, _last) to the output range starting
* at _d_first via re_std::move (the utility cast). Returns the iterator
* one past the last element written.
*
*   PORTABILITY:
*   - std::move (algorithm) is C++11. Requires rvalue references; cannot
*     be back-ported with correct semantics to C++98 (a copy fallback
*     would silently change meaning).
*   - Coexists with re_std::move (the utility cast in
*     re_std/utility/move.hpp) via overload resolution: the cast takes
*     one argument; the algorithm takes three iterator arguments.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/move.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_MOVE_HPP
#define RE_STD_ALGORITHM_MOVE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   GATE: rvalue references required
// ===========================================================================

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

// re_std
#include "../utility/move.hpp"


// ===========================================================================
// 1.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   MOVE (ALGORITHM)
// ===========================================================================

// move (algorithm)
//   function: moves [_first, _last) into [_d_first, _d_first + N) via
// re_std::move-cast on each element. Returns one past the last element
// written.
template<typename InputIt,
         typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt
move(
    InputIt  _first,
    InputIt  _last,
    OutputIt _d_first
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        *_d_first = re_std::move(*_first);
    }

    return _d_first;
}


}  // re_std


#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES


#endif  // RE_STD_ALGORITHM_MOVE_HPP
