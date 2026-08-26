/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/replace_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_REPLACE_IF_
#define DJINTERP_RE_STD_ALGORITHM_REPLACE_IF_ 1

#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   REPLACE_IF
// ===========================================================================

// replace_if
//   function: in-place replacement by predicate. For every element in
// [_first, _last) where _pred returns true, assign _new_value.
template<typename _ForwardIt,
         typename _Pred,
         typename _Type>
D_CONSTEXPR_CPP14 void
replace_if(
    _ForwardIt   _first,
    _ForwardIt   _last,
    _Pred        _pred,
    const _Type& _new_value
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


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_REPLACE_IF_
