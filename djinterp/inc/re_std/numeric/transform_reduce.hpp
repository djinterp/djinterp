/*******************************************************************************
* djinterp [re_std]                                         transform_reduce.hpp
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
* path:      /inc/re_std/numeric/transform_reduce.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_TRANSFORM_REDUCE_HPP
#define RE_STD_NUMERIC_TRANSFORM_REDUCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/utility/move.hpp"


namespace re_std
{

// Two-range, custom ops.
template<typename InputIt1, typename InputIt2, typename T,
         typename BinReduceOp, typename BinTransformOp>
RE_STD_CONSTEXPR_CPP14 T transform_reduce
(
    InputIt1        _first1,
    InputIt1        _last1,
    InputIt2        _first2,
    T               _init,
    BinReduceOp     _reduce,
    BinTransformOp  _transform
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
template<typename InputIt1, typename InputIt2, typename T>
RE_STD_CONSTEXPR_CPP14 T transform_reduce
(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    T        _init
)
{
    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        _init = re_std::move(_init) + (*_first1 * *_first2);
    }
    return _init;
}

// Single-range, custom ops.
template<typename InputIt, typename T,
         typename BinReduceOp, typename UnaryTransformOp>
RE_STD_CONSTEXPR_CPP14 T transform_reduce
(
    InputIt           _first,
    InputIt           _last,
    T                 _init,
    BinReduceOp       _reduce,
    UnaryTransformOp  _transform
)
{
    for (; _first != _last; ++_first)
    {
        _init = _reduce(re_std::move(_init), _transform(*_first));
    }
    return _init;
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_NUMERIC_TRANSFORM_REDUCE_HPP
