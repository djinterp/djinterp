/******************************************************************************
* djinterp [restd]                                              replace_copy.hpp
*
* replace_copy algorithm header:
*   Out-of-place sibling of replace. Copies elements from
* [_first, _last) into the output range starting at _d_first, substituting
* _new_value for every input element equal to _old_value. Returns the
* iterator one past the last element written.
*
*   PORTABILITY:
*   - std::replace_copy is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/replace_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_REPLACE_COPY_
#define DJINTERP_RESTD_ALGORITHM_REPLACE_COPY_ 1

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
// I.   REPLACE_COPY
// ===========================================================================

// replace_copy
//   function: copies elements from [_first, _last) into _d_first,
// substituting _new_value for elements equal to _old_value. Returns
// the iterator one past the last element written.
template<typename _InputIt,
         typename _OutputIt,
         typename _Type>
D_CONSTEXPR_CPP14 _OutputIt
replace_copy(
    _InputIt     _first,
    _InputIt     _last,
    _OutputIt    _d_first,
    const _Type& _old_value,
    const _Type& _new_value
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


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_REPLACE_COPY_
