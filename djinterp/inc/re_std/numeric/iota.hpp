/*******************************************************************************
* djinterp [re_std]                                                     iota.hpp
*
* iota algorithm header:
* iota(_first, _last, _value) fills [_first, _last) with values
* _value, _value+1, _value+2, .... Each subsequent element is the
* result of ++_value (pre-increment), so any value type with
* operator++ is acceptable, not only integers.
*
* added in std C++11; constexpr in C++20. re_std back-ports the
* function to C++98+ tier and the constexpr to C++14+.
*
*
* path:      /inc/re_std/numeric/iota.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_IOTA_HPP
#define RE_STD_NUMERIC_IOTA_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{

template<typename ForwardIt, typename T>
RE_STD_CONSTEXPR_CPP14 void iota
(
    ForwardIt _first,
    ForwardIt _last,
    T         _value
)
{
    for (; _first != _last; ++_first, (void)++_value)
    {
        *_first = _value;
    }
}


}  // re_std
#endif  // RE_STD_NUMERIC_IOTA_HPP
