/*******************************************************************************
* djinterp [re_std]                                                  replace.hpp
*
* replace algorithm header:
*   Walks [_first, _last) and replaces every element equal to
* _old_value (via operator==) with _new_value.
*
*   PORTABILITY:
*   - std::replace is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/replace.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_REPLACE_HPP
#define RE_STD_ALGORITHM_REPLACE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   REPLACE
// ===========================================================================

// replace
//   function: in-place replacement. For every element in
// [_first, _last) equal to _old_value, assign _new_value.
template<typename ForwardIt,
         typename Type>
RE_STD_CONSTEXPR_CPP14 void
replace(
    ForwardIt   _first,
    ForwardIt   _last,
    const Type& _old_value,
    const Type& _new_value
)
{
    for (; _first != _last; ++_first)
    {
        if (*_first == _old_value)
        {
            *_first = _new_value;
        }
    }
}


}  // re_std


#endif  // RE_STD_ALGORITHM_REPLACE_HPP
