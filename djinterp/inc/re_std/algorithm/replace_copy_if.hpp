/*******************************************************************************
* djinterp [re_std]                                          replace_copy_if.hpp
*
* replace_copy_if algorithm header:
*   Out-of-place sibling of replace_if. Copies elements from
* [_first, _last) into the output range starting at _d_first,
* substituting _new_value for every input element where _pred returns
* true. Returns the iterator one past the last element written.
*
*   PORTABILITY:
*   - std::replace_copy_if is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/replace_copy_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_REPLACE_COPY_IF_HPP
#define RE_STD_ALGORITHM_REPLACE_COPY_IF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   REPLACE_COPY_IF
// ===========================================================================

// replace_copy_if
//   function: copies elements from [_first, _last) into _d_first,
// substituting _new_value for elements where _pred returns true.
// Returns the iterator one past the last element written.
template<typename InputIt,
         typename OutputIt,
         typename Pred,
         typename Type>
RE_STD_CONSTEXPR_CPP14 OutputIt
replace_copy_if(
    InputIt     _first,
    InputIt     _last,
    OutputIt    _d_first,
    Pred        _pred,
    const Type& _new_value
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        if (_pred(*_first))
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


#endif  // RE_STD_ALGORITHM_REPLACE_COPY_IF_HPP
