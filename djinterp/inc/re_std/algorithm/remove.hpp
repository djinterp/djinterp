/*******************************************************************************
* djinterp [re_std]                                                   remove.hpp
*
* remove algorithm header:
*   In-place compaction. Walks [_first, _last) and pulls every element
* not equal to _value forward to overwrite the removed positions.
* Returns the iterator one past the last KEPT element; elements in
* [returned, _last) are in valid-but-unspecified state.
*
*   PORTABILITY:
*   - std::remove is C++98. C++11 strengthened the kept-element transfer
*     from copy assignment to move assignment. re_std honours the same
*     evolution: copy on C++98/03, move on C++11+ (gated on
*     RE_STD_LANG_HAS_RVALUE_REFERENCES).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Implementation forwards through find for the skip-prefix scan, so
*     this header includes find.hpp.
*
*
* path:      /inc/re_std/algorithm/remove.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_REMOVE_HPP
#define RE_STD_ALGORITHM_REMOVE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./find.hpp"
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "../utility/move.hpp"
#endif


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   REMOVE
// ===========================================================================

// remove
//   function: in-place compaction by value. Returns the iterator one
// past the last kept element. Kept elements retain their relative
// order. The unspecified tail [returned, _last) must be erased by the
// caller if a true size reduction is desired (the "erase-remove"
// idiom).
template<typename ForwardIt,
         typename Type>
RE_STD_CONSTEXPR_CPP14 ForwardIt
remove(
    ForwardIt   _first,
    ForwardIt   _last,
    const Type& _value
)
{
    // skip the matchless prefix
    _first = re_std::find(_first, _last, _value);
    if (_first == _last)
    {
        return _first;
    }

    // _first now points at the first removable element; pull subsequent
    // non-matching elements forward over it
    ForwardIt _it = _first;
    ++_it;

    for (; _it != _last; ++_it)
    {
        if (!(*_it == _value))
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


#endif  // RE_STD_ALGORITHM_REMOVE_HPP
