/*******************************************************************************
* djinterp [cpp][math]                                                series.hpp
*
* Series and summation.
*   Finite summation of a callable, partial sums over any sequence, the usual
* geometric / arithmetic closed forms, and a Taylor (Maclaurin) evaluator that
* reuses the exact symbolic derivatives from differentiation.hpp.
*
*   sum(f, lo, hi)               - sum_{k=lo}^{hi} f(k)
*   partial_sum(seq, count)      - sum of the first count terms of a sequence
*   geometric_sum(a0, r, n)      - a0 (1 - r^n)/(1 - r)
*   geometric_sum_infinite(a0,r) - a0 / (1 - r)        (|r| < 1)
*   arithmetic_sum(a0, d, n)     - n a0 + d n(n-1)/2
*   taylor_eval<N>(e, c, x)      - degree-N Taylor approx of e about c, at x
*   maclaurin_eval<N>(e, x)      - taylor_eval<N>(e, 0, x)
*
* LEVELS (step 31, 5c)
*   C++11 is the floor, as for expression.hpp, and the header is empty below
* it. From C++11 the infinite geometric sum, the arithmetic sum and the
* Taylor and Maclaurin evaluations are constant expressions, the last two
* wherever the expression's derivatives evaluate as one (a polynomial from
* C++11; exp, sin, cos, tan, sqrt, log and atan from C++14, where the C
* core's kernels are). sum, partial_sum and geometric_sum loop as many times
* as their bounds or count say: constant expressions from C++14, and at
* C++11 the same loops at run time, with the same bits.
*
*
* path:      /inc/djinterp/cpp/modules/math/calculus/series.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FINITE SUMMATION
    ----------------
2.  CLOSED FORMS
    ------------
3.  TAYLOR / MACLAURIN
    ------------------
*/

#ifndef DJINTERP_MATH_CALCULUS_SERIES_HPP
#define DJINTERP_MATH_CALCULUS_SERIES_HPP 1

// djinterp
#include "../../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
// djinterp
#include "./sequence.hpp"
#include "./differentiation.hpp"


NS_DJINTERP
NS_MATH

//==============================================================================
// 1.  FINITE SUMMATION
//==============================================================================

// sum
//   sum_{k=lo}^{hi} f(k), inclusive. Constexpr when f is (use a functor; a
// lambda is not constexpr before C++17).
template<typename F>
D_CONSTEXPR_CPP14
auto sum(
    F    _f,
    long _lo,
    long _hi
) -> decltype(_f(_lo))
{
    using result_type = decltype(_f(_lo));
    result_type acc = result_type(0);

    for (long k = _lo; k <= _hi; ++k)
    {
        acc += _f(k);
    }

    return acc;
}

// partial_sum
//   sum of the first _count terms (indices 0 .. _count-1) of any sequence
// exposing value_type and term().
template<typename Seq>
D_CONSTEXPR_CPP14
typename Seq::value_type partial_sum(
    const Seq& _s,
    std::size_t _count
)
{
    typename Seq::value_type acc = typename Seq::value_type(0);

    for (std::size_t n = 0; n < _count; ++n)
    {
        acc += _s.term(n);
    }

    return acc;
}


//==============================================================================
// 2.  CLOSED FORMS
//==============================================================================

// geometric_sum: a0 (1 - r^count) / (1 - r), or count*a0 when r == 1.
template<typename T>
D_CONSTEXPR_CPP14
T geometric_sum(
    T           _a0,
    T           _r,
    std::size_t _count
)
{
    if (_r == T(1)) { return static_cast<T>(_count) * _a0; }

    T rn = T(1);
    for (std::size_t i = 0; i < _count; ++i) { rn *= _r; }
    return _a0 * (T(1) - rn) / (T(1) - _r);
}

// geometric_sum_infinite: limit of the geometric series, valid for |r| < 1.
template<typename T>
D_CONSTEXPR
T geometric_sum_infinite(
    T _a0,
    T _r
)
{
    return _a0 / (T(1) - _r);
}

// arithmetic_sum: n a0 + d n(n-1)/2.
template<typename T>
D_CONSTEXPR
T arithmetic_sum(
    T           _a0,
    T           _d,
    std::size_t _count
)
{
    return static_cast<T>(_count) * _a0
         + _d * static_cast<T>(_count * (_count - 1) / 2);
}


//==============================================================================
// 3.  TAYLOR / MACLAURIN
//==============================================================================
// exact derivatives, evaluated numerically.

NS_INTERNAL

    // taylor_helper: accumulate sum_{k=K}^{N} f^(k)(c)/k! * (x-c)^k, carrying
    // the running power (x-c)^k and factorial k! to avoid recomputation; one
    // return each, so that each is a constant expression from C++11.
    template<std::size_t K, std::size_t N>
    struct taylor_helper
    {
        template<typename E>
        static D_CONSTEXPR
        double apply(
            const E&  _e,
            double    _c,
            double    _x,
            double    _pow,
            double    _fact
        )
        {
            return static_cast<double>(
                       (nth_derivative<0, K>(_e)(_c) / _fact) * _pow)
                 + taylor_helper<K + 1, N>::apply(
                       _e, _c, _x,
                       _pow * (_x - _c),
                       _fact * static_cast<double>(K + 1));
        }
    };

    template<std::size_t N>
    struct taylor_helper<N, N>
    {
        template<typename E>
        static D_CONSTEXPR double
        apply(const E& _e, double _c, double /*_x*/, double _pow, double _fact)
        {
            return (nth_derivative<0, N>(_e)(_c) / _fact) * _pow;
        }
    };

NS_END  // internal

// taylor_eval<N>(e, center, x): value at x of the degree-N Taylor polynomial
// of the univariate expression e (argument 0) expanded about center.
template<std::size_t N, typename E>
D_CONSTEXPR
double taylor_eval(
    const E&  _e,
    double    _center,
    double    _x
)
{
    return internal::taylor_helper<0, N>::apply(_e, _center, _x, 1.0, 1.0);
}

// maclaurin_eval<N>(e, x): Taylor expansion about 0.
template<std::size_t N, typename E>
D_CONSTEXPR
double maclaurin_eval(
    const E&  _e,
    double    _x
)
{
    return taylor_eval<N>(_e, 0.0, _x);
}

NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_CALCULUS_SERIES_HPP
