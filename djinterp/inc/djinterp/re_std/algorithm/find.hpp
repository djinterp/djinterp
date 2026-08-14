/******************************************************************************
* djinterp [restd]                                                      find.hpp
*
* find algorithm header:
*   Linear search for the first element in [_first, _last) equal to
* _value (via operator==). Returns _last on no-match.
*
*   PORTABILITY:
*   - std::find is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/find.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_FIND_
#define DJINTERP_RESTD_ALGORITHM_FIND_ 1

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
// I.   FIND
// ===========================================================================

// find
//   function: returns the first iterator it in [_first, _last) such
// that *it == _value, or _last on no-match.
template<typename _InputIt,
         typename _Type>
D_CONSTEXPR_CPP14 _InputIt
find(
    _InputIt     _first,
    _InputIt     _last,
    const _Type& _value
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


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_FIND_
