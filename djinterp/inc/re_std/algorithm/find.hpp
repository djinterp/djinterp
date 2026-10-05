/*******************************************************************************
* djinterp [re_std]                                                     find.hpp
*
* find algorithm header:
*   Linear search for the first element in [_first, _last) equal to
* _value (via operator==). Returns _last on no-match.
*
*   PORTABILITY:
*   - std::find is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/find.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_FIND_HPP
#define RE_STD_ALGORITHM_FIND_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   FIND
// ===========================================================================

// find
//   function: returns the first iterator it in [_first, _last) such
// that *it == _value, or _last on no-match.
template<typename InputIt,
         typename Type>
RE_STD_CONSTEXPR_CPP14 InputIt
find(
    InputIt     _first,
    InputIt     _last,
    const Type& _value
)
{
    for (; _first != _last; ++_first)
    {
        if (*_first == _value)
        {
            return _first;
        }
    }

    return _last;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_FIND_HPP
