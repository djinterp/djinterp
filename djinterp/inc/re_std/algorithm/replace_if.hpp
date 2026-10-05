/*******************************************************************************
* djinterp [re_std]                                               replace_if.hpp
*
* replace_if algorithm header:
*   Walks [_first, _last) and replaces every element for which _pred
* returns true with _new_value.
*
*   PORTABILITY:
*   - std::replace_if is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/replace_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_REPLACE_IF_HPP
#define RE_STD_ALGORITHM_REPLACE_IF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   REPLACE_IF
// ===========================================================================

// replace_if
//   function: in-place replacement by predicate. For every element in
// [_first, _last) where _pred returns true, assign _new_value.
template<typename ForwardIt,
         typename Pred,
         typename Type>
RE_STD_CONSTEXPR_CPP14 void
replace_if(
    ForwardIt   _first,
    ForwardIt   _last,
    Pred        _pred,
    const Type& _new_value
)
{
    for (; _first != _last; ++_first)
    {
        if (_pred(*_first))
        {
            *_first = _new_value;
        }
    }
}


}  // re_std


#endif  // RE_STD_ALGORITHM_REPLACE_IF_HPP
