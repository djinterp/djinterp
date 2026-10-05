/*******************************************************************************
* djinterp [re_std]                                              remove_copy.hpp
*
* remove_copy algorithm header:
*   Out-of-place sibling of remove. Copies elements from
* [_first, _last) to the output range starting at _d_first, skipping
* those equal to _value. Returns the iterator one past the last element
* written.
*
*   PORTABILITY:
*   - std::remove_copy is C++98.
*   - Pure copy semantics (writes to a separate output range); no
*     conditional move dance like in-place remove.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/remove_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_REMOVE_COPY_HPP
#define RE_STD_ALGORITHM_REMOVE_COPY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   REMOVE_COPY
// ===========================================================================

// remove_copy
//   function: copies every element of [_first, _last) not equal to
// _value into the output range starting at _d_first. Returns the
// iterator one past the last element written.
template<typename InputIt,
         typename OutputIt,
         typename Type>
RE_STD_CONSTEXPR_CPP14 OutputIt
remove_copy(
    InputIt     _first,
    InputIt     _last,
    OutputIt    _d_first,
    const Type& _value
)
{
    for (; _first != _last; ++_first)
    {
        if (!(*_first == _value))
        {
            *_d_first = *_first;
            ++_d_first;
        }
    }

    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_REMOVE_COPY_HPP
