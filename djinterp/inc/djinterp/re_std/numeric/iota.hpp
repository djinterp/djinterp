/***********************************************************************
* re_std                                                             iota.hpp
*
* iota(_first, _last, _value) fills [_first, _last) with values
* _value, _value+1, _value+2, .... Each subsequent element is the
* result of ++_value (pre-increment), so any value type with
* operator++ is acceptable, not only integers.
*
* added in std C++11; constexpr in C++20. re_std back-ports the
* function to C++98+ tier and the constexpr to C++14+.
*
*
* path:      /inc/djinterp/re_std/numeric/iota.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.09
***********************************************************************/

#ifndef DJINTERP_RE_STD_NUMERIC_IOTA_
#define DJINTERP_RE_STD_NUMERIC_IOTA_ 1

#include "djinterp.hpp"


// D_CONSTEXPR_CPP14 — `constexpr` on C++14+, empty on earlier tiers.
//   constexpr functions on C++11 must consist of a single return; the
//   loop body of iota requires the C++14 relaxation.
#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace re_std
{

template<typename _ForwardIt, typename _T>
D_CONSTEXPR_CPP14 void iota
(
    _ForwardIt _first,
    _ForwardIt _last,
    _T         _value
)
{
    for (; _first != _last; ++_first, (void)++_value)
    {
        *_first = _value;
    }
}


}  // namespace re_std

#endif  // DJINTERP_RE_STD_NUMERIC_IOTA_
