/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/replace_copy_if.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_REPLACE_COPY_IF_
#define DJINTERP_RE_STD_ALGORITHM_REPLACE_COPY_IF_ 1

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
// I.   REPLACE_COPY_IF
// ===========================================================================

// replace_copy_if
//   function: copies elements from [_first, _last) into _d_first,
// substituting _new_value for elements where _pred returns true.
// Returns the iterator one past the last element written.
template<typename _InputIt,
         typename _OutputIt,
         typename _Pred,
         typename _Type>
D_CONSTEXPR_CPP14 _OutputIt
replace_copy_if(
    _InputIt     _first,
    _InputIt     _last,
    _OutputIt    _d_first,
    _Pred        _pred,
    const _Type& _new_value
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


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_REPLACE_COPY_IF_
