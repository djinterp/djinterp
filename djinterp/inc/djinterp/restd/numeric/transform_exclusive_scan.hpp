/***********************************************************************
* restd                                          transform_exclusive_scan.hpp
*
* like exclusive_scan but applies _unary_op to each input before
* folding:
*
*   d[0] = init
*   d[1] = bin_op(init,    unary(src[0]))
*   d[i] = bin_op(d[i-1],  unary(src[i-1]))
*
*
* path:      /inc/restd/numeric/transform_exclusive_scan.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.09
***********************************************************************/

#ifndef RESTD_NUMERIC_TRANSFORM_EXCLUSIVE_SCAN_
#define RESTD_NUMERIC_TRANSFORM_EXCLUSIVE_SCAN_ 1

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

template<typename _InputIt, typename _OutputIt, typename _T,
         typename _BinOp, typename _UnaryOp>
D_CONSTEXPR_CPP14 _OutputIt transform_exclusive_scan
(
    _InputIt    _first,
    _InputIt    _last,
    _OutputIt   _d_first,
    _T          _init,
    _BinOp      _bin_op,
    _UnaryOp    _unary_op
)
{
    while (_first != _last)
    {
        _T _next = _bin_op(_init, _unary_op(*_first));
        *_d_first = restd::move(_init);
        _init = restd::move(_next);
        ++_first;
        ++_d_first;
    }
    return _d_first;
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_NUMERIC_TRANSFORM_EXCLUSIVE_SCAN_
