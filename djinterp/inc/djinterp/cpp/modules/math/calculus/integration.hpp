/*******************************************************************************
* djinterp [cpp][math]                                           integration.hpp
*
* Numerical definite integration.
*   Quadrature rules over [a, b] for an arbitrary callable f. Each rule is a
* function template, so it is constexpr-capable whenever the integrand is
* (pass a constexpr functor; lambdas are not constexpr before C++17). An
* expression overload integrates a univariate expression directly.
*
*   trapezoid(f, a, b, n)     - composite trapezoidal rule, n subintervals
*   simpson(f, a, b, n)       - composite Simpson's rule (n forced even)
*   gauss_legendre5(f, a, b)  - 5-point Gauss-Legendre (exact to degree 9)
*   romberg<Levels>(f, a, b)  - Romberg / Richardson extrapolation
*   integrate(expr, a, b, n)  - Simpson over a univariate expression (arg 0)
*
* LEVELS (step 31, 5c)
*   C++11 is the floor, as for expression.hpp, and the header is empty below
* it. From C++11 gauss_legendre5 is a constant expression wherever its
* integrand is, one return over its five points, summed in their order as
* the loop did, with its bits wherever double is evaluated as double (on a
* 32-bit x87 the sum is kept wider, as the loop's was not). trapezoid, simpson, romberg and integrate loop as many times
* as their subintervals or levels say: constant expressions from C++14, and
* at C++11 the same loops at run time, with the same bits.
*
*
* path:      /inc/djinterp/cpp/modules/math/calculus/integration.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.23
*******************************************************************************/

#ifndef DJINTERP_MATH_CALCULUS_INTEGRATION_HPP
#define DJINTERP_MATH_CALCULUS_INTEGRATION_HPP 1

// djinterp
#include "../../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../expression.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    QUADRATURE RULES (callable integrand)
// ============================================================================

// trapezoid
//   composite trapezoidal rule with _n subintervals.
template<typename _F>
D_CONSTEXPR_CPP14
double trapezoid(
    _F          _f,
    double      _a,
    double      _b,
    std::size_t _n = 128
)
{
    if (_n < 1) { _n = 1; }

    const double h   = (_b - _a) / static_cast<double>(_n);
    double       sum = 0.5 * (_f(_a) + _f(_b));

    for (std::size_t i = 1; i < _n; ++i)
    {
        sum += _f(_a + static_cast<double>(i) * h);
    }

    return sum * h;
}

// simpson
//   composite Simpson's rule; _n is rounded up to the next even number.
template<typename _F>
D_CONSTEXPR_CPP14
double simpson(
    _F          _f,
    double      _a,
    double      _b,
    std::size_t _n = 128
)
{
    if (_n < 2)      { _n = 2; }
    if (_n % 2 != 0) { _n += 1; }

    const double h   = (_b - _a) / static_cast<double>(_n);
    double       sum = _f(_a) + _f(_b);

    for (std::size_t i = 1; i < _n; ++i)
    {
        const double w = (i % 2 != 0) ? 4.0 : 2.0;
        sum += w * _f(_a + static_cast<double>(i) * h);
    }

    return sum * h / 3.0;
}

NS_INTERNAL

    // gauss_legendre5_on
    //   the 5-point rule about _mid with half-width _half: the loop's sum,
    // from 0.0, each weighted point added in order, then times _half; the
    // loop's bits wherever double is evaluated as double, fused or not.
    template<typename _F>
    D_CONSTEXPR
    double gauss_legendre5_on(
        _F     _f,
        double _mid,
        double _half
    )
    {
        return _half *
            ( ( ( ( ( 0.0
                    + 0.236926885056189087514264
                    * _f(_mid + _half * -0.906179845938663992797627) )
                  + 0.478628670499366468041292
                  * _f(_mid + _half * -0.538469310105683091036314) )
                + 0.568888888888888888888889
                * _f(_mid + _half * 0.0) )
              + 0.478628670499366468041292
              * _f(_mid + _half * 0.538469310105683091036314) )
            + 0.236926885056189087514264
            * _f(_mid + _half * 0.906179845938663992797627) );
    }

NS_END  // internal

// gauss_legendre5
//   5-point Gauss-Legendre on [a, b]; exact for polynomials up to degree 9.
template<typename _F>
D_CONSTEXPR
double gauss_legendre5(
    _F     _f,
    double _a,
    double _b
)
{
    return internal::gauss_legendre5_on(_f, 0.5 * (_a + _b), 0.5 * (_b - _a));
}

// romberg
//   Romberg integration: _Levels rows of trapezoidal refinement combined by
// Richardson extrapolation. Higher _Levels -> higher accuracy.
template<std::size_t _Levels = 6, typename _F>
D_CONSTEXPR_CPP14 double
romberg(_F _f, double _a, double _b)
{
    static_assert(_Levels >= 1, "romberg: need at least one level.");

    double row[_Levels]  = {};
    double prev[_Levels] = {};

    double h = _b - _a;
    prev[0]  = 0.5 * h * (_f(_a) + _f(_b));

    for (std::size_t k = 1; k < _Levels; ++k)
    {
        // refined trapezoid: add the new midpoints
        h *= 0.5;
        std::size_t points = static_cast<std::size_t>(1) << (k - 1);
        double      acc    = 0.0;

        for (std::size_t i = 1; i <= points; ++i)
        {
            acc += _f(_a + static_cast<double>(2 * i - 1) * h);
        }

        row[0] = 0.5 * prev[0] + h * acc;

        // Richardson extrapolation across the row
        double factor = 1.0;
        for (std::size_t j = 1; j <= k; ++j)
        {
            factor *= 4.0;
            row[j]  = row[j - 1] + (row[j - 1] - prev[j - 1]) / (factor - 1.0);
        }

        for (std::size_t j = 0; j <= k; ++j) { prev[j] = row[j]; }
    }

    return prev[_Levels - 1];
}


// ============================================================================
// II.   EXPRESSION CONVENIENCE
// ============================================================================

NS_INTERNAL

    // expr_eval1: turn a univariate expression into a constexpr-friendly unary
    // functor (avoids lambdas, which are not constexpr before C++17).
    template<typename _Expr>
    struct expr_eval1
    {
        _Expr expr;

        D_CONSTEXPR
        double operator()(
            double _x
        ) const
        {
            return static_cast<double>(expr(_x));
        }
    };

NS_END  // internal

// integrate
//   definite integral of a univariate expression (argument 0) over [a, b]
// using composite Simpson's rule.
template<typename _Expr,
         typename std::enable_if<is_expression<_Expr>::value, int>::type = 0>
D_CONSTEXPR_CPP14 double
integrate(const _Expr& _e, double _a, double _b, std::size_t _n = 128)
{
    return simpson(internal::expr_eval1<_Expr>{_e}, _a, _b, _n);
}

NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_CALCULUS_INTEGRATION_HPP
