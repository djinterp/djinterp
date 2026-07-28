/***********************************************************************
* restd                                          transform_inclusive_scan.hpp
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
* path:      /inc/restd/numeric/transform_inclusive_scan.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.09
***********************************************************************/

#ifndef RESTD_NUMERIC_TRANSFORM_INCLUSIVE_SCAN_
#define RESTD_NUMERIC_TRANSFORM_INCLUSIVE_SCAN_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/utility/move.hpp"


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace restd
{

// Without explicit init.
template<typename _InputIt, typename _OutputIt,
         typename _BinOp, typename _UnaryOp>
D_CONSTEXPR_CPP14 _OutputIt transform_inclusive_scan
(
    _InputIt    _first,
    _InputIt    _last,
    _OutputIt   _d_first,
    _BinOp      _bin_op,
    _UnaryOp    _unary_op
)
{
    if (_first == _last) return _d_first;
    auto _acc = _unary_op(*_first);
    *_d_first = _acc;
    for (++_first, (void)++_d_first; _first != _last;
         ++_first, (void)++_d_first)
    {
        _acc = _bin_op(restd::move(_acc), _unary_op(*_first));
        *_d_first = _acc;
    }
    return _d_first;
}

// With explicit init.
template<typename _InputIt, typename _OutputIt,
         typename _BinOp, typename _UnaryOp, typename _T>
D_CONSTEXPR_CPP14 _OutputIt transform_inclusive_scan
(
    _InputIt    _first,
    _InputIt    _last,
    _OutputIt   _d_first,
    _BinOp      _bin_op,
    _UnaryOp    _unary_op,
    _T          _init
)
{
    for (; _first != _last; ++_first, (void)++_d_first)
    {
        _init = _bin_op(restd::move(_init), _unary_op(*_first));
        *_d_first = _init;
    }
    return _d_first;
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_NUMERIC_TRANSFORM_INCLUSIVE_SCAN_
