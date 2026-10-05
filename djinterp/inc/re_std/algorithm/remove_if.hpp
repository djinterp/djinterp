/*******************************************************************************
* djinterp [re_std]                                                remove_if.hpp
*
* remove_if algorithm header:
*   In-place compaction. Walks [_first, _last) and pulls every element
* for which _pred returns false forward to overwrite the removed
* positions. Returns the iterator one past the last KEPT element.
*
*   PORTABILITY:
*   - std::remove_if is C++98. C++11 strengthened to move assignment.
*     re_std matches per-tier (copy on C++98, move on C++11+).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Forwards through find_if for the skip-prefix scan.
*
*
* path:      /inc/re_std/algorithm/remove_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_REMOVE_IF_HPP
#define RE_STD_ALGORITHM_REMOVE_IF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./find_if.hpp"
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "../utility/move.hpp"
#endif


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   REMOVE_IF
// ===========================================================================

// remove_if
//   function: in-place compaction by predicate. Returns the iterator
// one past the last kept element. Kept elements retain their relative
// order; the tail [returned, _last) is in valid-but-unspecified state.
template<typename ForwardIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 ForwardIt
remove_if(
    ForwardIt _first,
    ForwardIt _last,
    Pred      _pred
)
{
    // skip the matchless prefix
    _first = re_std::find_if(_first, _last, _pred);
    if (_first == _last)
    {
        return _first;
    }

    ForwardIt _it = _first;
    ++_it;

    for (; _it != _last; ++_it)
    {
        if (!_pred(*_it))
        {
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
            *_first = re_std::move(*_it);
#else
            *_first = *_it;
#endif
            ++_first;
        }
    }

    return _first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_REMOVE_IF_HPP
