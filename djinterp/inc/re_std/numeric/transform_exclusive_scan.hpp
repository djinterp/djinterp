/*******************************************************************************
* djinterp [re_std]                                 transform_exclusive_scan.hpp
*
* like exclusive_scan but applies _unary_op to each input before
* folding:
*
*   d[0] = init
*   d[1] = bin_op(init,    unary(src[0]))
*   d[i] = bin_op(d[i-1],  unary(src[i-1]))
*
*
* path:      /inc/re_std/numeric/transform_exclusive_scan.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_TRANSFORM_EXCLUSIVE_SCAN_HPP
#define RE_STD_NUMERIC_TRANSFORM_EXCLUSIVE_SCAN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/utility/move.hpp"


namespace re_std
{

template<typename InputIt, typename OutputIt, typename T,
         typename BinOp, typename UnaryOp>
RE_STD_CONSTEXPR_CPP14 OutputIt transform_exclusive_scan
(
    InputIt    _first,
    InputIt    _last,
    OutputIt   _d_first,
    T          _init,
    BinOp      _bin_op,
    UnaryOp    _unary_op
)
{
    while (_first != _last)
    {
        T _next = _bin_op(_init, _unary_op(*_first));
        *_d_first = re_std::move(_init);
        _init = re_std::move(_next);
        ++_first;
        ++_d_first;
    }
    return _d_first;
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_NUMERIC_TRANSFORM_EXCLUSIVE_SCAN_HPP
