/*******************************************************************************
* djinterp [re_std]                                             replace_copy.hpp
*
* replace_copy algorithm header:
*   Out-of-place sibling of replace. Copies elements from
* [_first, _last) into the output range starting at _d_first, substituting
* _new_value for every input element equal to _old_value. Returns the
* iterator one past the last element written.
*
*   PORTABILITY:
*   - std::replace_copy is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/replace_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_REPLACE_COPY_HPP
#define RE_STD_ALGORITHM_REPLACE_COPY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   REPLACE_COPY
// ===========================================================================

// replace_copy
//   function: copies elements from [_first, _last) into _d_first,
// substituting _new_value for elements equal to _old_value. Returns
// the iterator one past the last element written.
template<typename InputIt,
         typename OutputIt,
         typename Type>
RE_STD_CONSTEXPR_CPP14 OutputIt
replace_copy(
    InputIt     _first,
    InputIt     _last,
    OutputIt    _d_first,
    const Type& _old_value,
    const Type& _new_value
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        if (*_first == _old_value)
        {
            *_d_first = _new_value;
        }
        else
        {
            *_d_first = *_first;
        }
    }

    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_REPLACE_COPY_HPP
