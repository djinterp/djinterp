/*******************************************************************************
* djinterp [re_std]                                 transform_inclusive_scan.hpp
*
* like inclusive_scan but applies _unary_op to each input before
* folding via _bin_op:
*
*   d[0] = unary(src[0])
*   d[i] = bin_op(d[i-1], unary(src[i]))
*
* with explicit init:
*   d[0] = bin_op(init, unary(src[0]))
*   d[i] = bin_op(d[i-1], unary(src[i]))
*
*
* path:      /inc/re_std/numeric/transform_inclusive_scan.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_TRANSFORM_INCLUSIVE_SCAN_HPP
#define RE_STD_NUMERIC_TRANSFORM_INCLUSIVE_SCAN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/utility/move.hpp"


namespace re_std
{

// Without explicit init.
template<typename InputIt, typename OutputIt,
         typename BinOp, typename UnaryOp>
RE_STD_CONSTEXPR_CPP14 OutputIt transform_inclusive_scan
(
    InputIt    _first,
    InputIt    _last,
    OutputIt   _d_first,
    BinOp      _bin_op,
    UnaryOp    _unary_op
)
{
    if (_first == _last) return _d_first;
    auto _acc = _unary_op(*_first);
    *_d_first = _acc;
    for (++_first, (void)++_d_first; _first != _last;
         ++_first, (void)++_d_first)
    {
        _acc = _bin_op(re_std::move(_acc), _unary_op(*_first));
        *_d_first = _acc;
    }
    return _d_first;
}

// With explicit init.
template<typename InputIt, typename OutputIt,
         typename BinOp, typename UnaryOp, typename T>
RE_STD_CONSTEXPR_CPP14 OutputIt transform_inclusive_scan
(
    InputIt    _first,
    InputIt    _last,
    OutputIt   _d_first,
    BinOp      _bin_op,
    UnaryOp    _unary_op,
    T          _init
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        _init = _bin_op(re_std::move(_init), _unary_op(*_first));
        *_d_first = _init;
    }
    return _d_first;
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_NUMERIC_TRANSFORM_INCLUSIVE_SCAN_HPP
