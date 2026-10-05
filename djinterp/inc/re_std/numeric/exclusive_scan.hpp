/*******************************************************************************
* djinterp [re_std]                                           exclusive_scan.hpp
*
* exclusive_scan writes the running fold up to but EXCLUDING the
* corresponding input:
*   d[0] = init
*   d[1] = op(init, src[0])
*   d[i] = op(d[i-1], src[i-1])
*
* the standard requires init be passed explicitly — there is no
* default-init overload on exclusive_scan.
*
* op must be associative (parallel-friendly). serial here.
*
* return value: iterator past the last destination written.
*
*
* path:      /inc/re_std/numeric/exclusive_scan.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_EXCLUSIVE_SCAN_HPP
#define RE_STD_NUMERIC_EXCLUSIVE_SCAN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/utility/move.hpp"


namespace re_std
{

// Default-op (operator+).
template<typename InputIt, typename OutputIt, typename T>
RE_STD_CONSTEXPR_CPP14 OutputIt exclusive_scan
(
    InputIt   _first,
    InputIt   _last,
    OutputIt  _d_first,
    T         _init
)
{
    while (_first != _last)
    {
        T _next = _init + *_first;
        *_d_first = re_std::move(_init);
        _init = re_std::move(_next);
        ++_first;
        ++_d_first;
    }
    return _d_first;
}

// Custom-op overload.
template<typename InputIt, typename OutputIt, typename T, typename BinOp>
RE_STD_CONSTEXPR_CPP14 OutputIt exclusive_scan
(
    InputIt   _first,
    InputIt   _last,
    OutputIt  _d_first,
    T         _init,
    BinOp     _op
)
{
    while (_first != _last)
    {
        T _next = _op(_init, *_first);
        *_d_first = re_std::move(_init);
        _init = re_std::move(_next);
        ++_first;
        ++_d_first;
    }
    return _d_first;
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_NUMERIC_EXCLUSIVE_SCAN_HPP
