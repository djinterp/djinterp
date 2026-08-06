/******************************************************************************
* djinterp [math]                                         calculus/series.hpp
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
* path:      /inc/djinterp/math/calculus/series.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.06.20
******************************************************************************/

#ifndef DJINTERP_MATH_CALCULUS_SERIES_
#define DJINTERP_MATH_CALCULUS_SERIES_ 1

#include <cstddef>

#include "../../djinterp.hpp"
#include "./sequence.hpp"
#include "./differentiation.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    FINITE SUMMATION
// ============================================================================

// sum
//   sum_{k=lo}^{hi} f(k), inclusive. Constexpr when f is (use a functor; a
// lambda is not constexpr before C++17).
template<typename _F>
D_CONSTEXPR auto
sum(_F _f, long _lo, long _hi) -> decltype(_f(_lo))
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
template<typename _Seq>
D_CONSTEXPR typename _Seq::value_type
partial_sum(const _Seq& _s, std::size_t _count)
{
    typename _Seq::value_type acc = typename _Seq::value_type(0);

    for (std::size_t n = 0; n < _count; ++n)
    {
        acc += _s.term(n);
    }

    return acc;
}


// ============================================================================
// II.   CLOSED FORMS
// ============================================================================

// geometric_sum: a0 (1 - r^count) / (1 - r), or count*a0 when r == 1.
template<typename _T>
D_CONSTEXPR _T
geometric_sum(_T _a0, _T _r, std::size_t _count)
{
    if (_r == _T(1)) { return static_cast<_T>(_count) * _a0; }

    _T rn = _T(1);
    for (std::size_t i = 0; i < _count; ++i) { rn *= _r; }
    return _a0 * (_T(1) - rn) / (_T(1) - _r);
}

// geometric_sum_infinite: limit of the geometric series, valid for |r| < 1.
template<typename _T>
D_CONSTEXPR _T
geometric_sum_infinite(_T _a0, _T _r)
{
    return _a0 / (_T(1) - _r);
}

// arithmetic_sum: n a0 + d n(n-1)/2.
template<typename _T>
D_CONSTEXPR _T
arithmetic_sum(_T _a0, _T _d, std::size_t _count)
{
    _T n = static_cast<_T>(_count);
    return n * _a0 + _d * static_cast<_T>(_count * (_count - 1) / 2);
}


// ============================================================================
// III.  TAYLOR / MACLAURIN  (exact derivatives, evaluated numerically)
// ============================================================================

NS_INTERNAL

    // taylor_helper: accumulate sum_{k=K}^{N} f^(k)(c)/k! * (x-c)^k, carrying
    // the running power (x-c)^k and factorial k! to avoid recomputation.
    template<std::size_t _K, std::size_t _N>
    struct taylor_helper
    {
        template<typename _E>
        static D_CONSTEXPR double
        apply(const _E& _e, double _c, double _x, double _pow, double _fact)
        {
            const double term = (nth_derivative<0, _K>(_e)(_c) / _fact) * _pow;
            return term
                 + taylor_helper<_K + 1, _N>::apply(
                       _e, _c, _x,
                       _pow * (_x - _c),
                       _fact * static_cast<double>(_K + 1));
        }
    };

    template<std::size_t _N>
    struct taylor_helper<_N, _N>
    {
        template<typename _E>
        static D_CONSTEXPR double
        apply(const _E& _e, double _c, double /*_x*/, double _pow, double _fact)
        {
            return (nth_derivative<0, _N>(_e)(_c) / _fact) * _pow;
        }
    };

NS_END  // internal

// taylor_eval<N>(e, center, x): value at x of the degree-N Taylor polynomial
// of the univariate expression e (argument 0) expanded about center.
template<std::size_t _N, typename _E>
D_CONSTEXPR double
taylor_eval(const _E& _e, double _center, double _x)
{
    return internal::taylor_helper<0, _N>::apply(_e, _center, _x, 1.0, 1.0);
}

// maclaurin_eval<N>(e, x): Taylor expansion about 0.
template<std::size_t _N, typename _E>
D_CONSTEXPR double
maclaurin_eval(const _E& _e, double _x)
{
    return taylor_eval<_N>(_e, 0.0, _x);
}

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_CALCULUS_SERIES_
