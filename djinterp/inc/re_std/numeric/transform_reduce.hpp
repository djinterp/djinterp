/***********************************************************************
* re_std                                                 transform_reduce.hpp
*
* generalisation of reduce that fuses a transformation step:
*
*   transform_reduce(f1, l1, f2, init)
*       == reduce(zip-with(*, [f1..l1), [f2..)), init, +)   conceptually
*
*   transform_reduce(f1, l1, f2, init, reduce_op, transform_op)
*       == like above but with the supplied ops
*
*   transform_reduce(first, last, init, reduce_op, unary_op)
*       == reduce(transform([first..last), unary_op), init, reduce_op)
*
* like reduce(), the reduce_op is required to be associative AND
* commutative; re_std's implementation is currently serial.
*
* added in std C++17.
*
*
* path:      /inc/djinterp/re_std/numeric/transform_reduce.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.09
***********************************************************************/

#ifndef DJINTERP_RE_STD_NUMERIC_TRANSFORM_REDUCE_
#define DJINTERP_RE_STD_NUMERIC_TRANSFORM_REDUCE_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/utility/move.hpp"


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace re_std
{

// Two-range, custom ops.
template<typename _InputIt1, typename _InputIt2, typename _T,
         typename _BinReduceOp, typename _BinTransformOp>
D_CONSTEXPR_CPP14 _T transform_reduce
(
    _InputIt1        _first1,
    _InputIt1        _last1,
    _InputIt2        _first2,
    _T               _init,
    _BinReduceOp     _reduce,
    _BinTransformOp  _transform
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        _init = _reduce(re_std::move(_init),
                        _transform(*_first1, *_first2));
    }
    return _init;
}

// Two-range, default ops (+ and *).
template<typename _InputIt1, typename _InputIt2, typename _T>
D_CONSTEXPR_CPP14 _T transform_reduce
(
    _InputIt1 _first1,
    _InputIt1 _last1,
    _InputIt2 _first2,
    _T        _init
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        _init = re_std::move(_init) + (*_first1 * *_first2);
    }
    return _init;
}

// Single-range, custom ops.
template<typename _InputIt, typename _T,
         typename _BinReduceOp, typename _UnaryTransformOp>
D_CONSTEXPR_CPP14 _T transform_reduce
(
    _InputIt           _first,
    _InputIt           _last,
    _T                 _init,
    _BinReduceOp       _reduce,
    _UnaryTransformOp  _transform
)
{
    for (; _first != _last; ++_first)
    {
        _init = _reduce(re_std::move(_init), _transform(*_first));
    }
    return _init;
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_NUMERIC_TRANSFORM_REDUCE_
