/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/replace.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_REPLACE_
#define DJINTERP_RE_STD_ALGORITHM_REPLACE_ 1

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
// I.   REPLACE
// ===========================================================================

// replace
//   function: in-place replacement. For every element in
// [_first, _last) equal to _old_value, assign _new_value.
template<typename _ForwardIt,
         typename _Type>
D_CONSTEXPR_CPP14 void
replace(
    _ForwardIt   _first,
    _ForwardIt   _last,
    const _Type& _old_value,
    const _Type& _new_value
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


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_REPLACE_
