/*******************************************************************************
* djinterp [cpp][math]                                       t_calculus_face.cpp
*
*   Tests for math/calculus/, the calculus subframework, at every level from
* C++98 (step 31, 5c). Below C++11 each header is empty and the umbrella
* brings the constants alone. From C++11: symbolic derivatives, higher
* derivatives and partials of polynomials, numeric_derivative, the 5-point
* Gauss-Legendre rule, Taylor and Maclaurin evaluations of polynomials, the
* arithmetic sequence and sums, and the scalar helpers that are one return,
* each a constant expression; at run time, the derivatives of exp, sin, cos,
* tan, sqrt, abs, log and atan bit for bit the C core's kernels composed by
* the rules, log and atan d_math_log_c's and d_math_atan_c's, the
* quadrature rules, the sums and the sequences, the gradient and the
* numeric partial. From C++14: the transcendentals, the loops and
* std::array's reads constant expressions too, with the run time's bits.
* Where double is evaluated wider than double, a 32-bit x87, exp and log
* refuse to compile (S418, S426), and their checks are left out.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/t_calculus_face.cpp                  \
*         -o t_calculus -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_calculus_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/calculus/calculus.hpp"  // the
                                                                    // tested
                                                                    // umbrella
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/elementary.h"   // sqrt_c,
                                                       // sin_c, cos_c
#include "../../../../../inc/djinterp/c/modules/math/exponential.h"  // exp_c,
                                                       // log_c
#include "../../../../../inc/djinterp/c/modules/math/tangent.h"      // tan_c,
                                                       // atan_c
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstring>  // std::memcpy


namespace
{
    long g_checks   = 0;
    long g_failures = 0;

    void
    d_tests_expect(
        bool        _ok,
        const char* _what,
        int         _line
    )
    {
        ++g_checks;

        // report the first few failures with what was checked, and where
        if (!_ok)
        {
            ++g_failures;

            if (g_failures <= 40)
            {
                std::printf("  FAIL line %d: %s\n", _line, _what);
            }
        }

        return;
    }

    #define D_TESTS_EXPECT(ok, what)                                          \
        d_tests_expect((ok), (what), __LINE__)

}  // namespace


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <array>    // std::array

namespace
{
    namespace dm = djinterp::math;
    namespace dv = djinterp::math::variables;

    // d_tests_same_bits
    //   whether two doubles have the same bits.
    bool
    d_tests_same_bits(
        double _a,
        double _b
    )
    {
        unsigned char a[sizeof(double)];
        unsigned char b[sizeof(double)];

        std::memcpy(a, &_a, sizeof(double));
        std::memcpy(b, &_b, sizeof(double));

        return (std::memcmp(a, b, sizeof(double)) == 0);
    }

    // d_tests_near
    //   whether _a is within _tol of _b.
    bool
    d_tests_near(
        double _a,
        double _b,
        double _tol
    )
    {
        double d = _a - _b;

        return ( (d <= _tol) && (-d <= _tol) );
    }

    // the integrands and summands: functors, constant expressions where
    // lambdas are not before C++17
    struct d_tests_square
    {
        D_CONSTEXPR double operator()(double _x) const { return _x * _x; }
    };

    struct d_tests_cubic
    {
        D_CONSTEXPR double
        operator()(double _x) const
        {
            return ((_x * _x) * _x) - (2.0 * _x) + 1.0;
        }
    };

    struct d_tests_identity
    {
        D_CONSTEXPR double
        operator()(long _k) const
        {
            return static_cast<double>(_k);
        }
    };

    struct d_tests_n_squared
    {
        D_CONSTEXPR double
        operator()(std::size_t _n) const
        {
            return static_cast<double>(_n) * static_cast<double>(_n);
        }
    };

    // d_tests_gauss_loop
    //   the 5-point rule as its loop was written before step 31: the one
    // return must give its bits.
    template<typename _F>
    double
    d_tests_gauss_loop(
        _F     _f,
        double _a,
        double _b
    )
    {
        const double nodes[5] = {
            -0.906179845938663992797627,
            -0.538469310105683091036314,
             0.0,
             0.538469310105683091036314,
             0.906179845938663992797627
        };
        const double weights[5] = {
            0.236926885056189087514264,
            0.478628670499366468041292,
            0.568888888888888888888889,
            0.478628670499366468041292,
            0.236926885056189087514264
        };
        const double mid  = 0.5 * (_a + _b);
        const double half = 0.5 * (_b - _a);
        double       sum  = 0.0;

        for (int i = 0; i < 5; ++i)
        {
            sum += weights[i] * _f(mid + half * nodes[i]);
        }

        return half * sum;
    }


    //==========================================================================
    // 1.  CONSTANT EXPRESSIONS FROM C++11
    //==========================================================================

    // derivatives of polynomials: the trees, built and evaluated
    D_CONSTEXPR double g_derivatives[8] = {
        dm::derivative<0>((dv::x * dv::x) * dv::x + 2.0 * dv::x)(2.0),
        dm::derivative<0>(1.0 / dv::x)(2.0),
        dm::derivative<1>(dv::x * dv::y + dv::y * dv::y)(3.0, 4.0),
        dm::derivative<0>(dv::x * dv::y + dv::y * dv::y)(3.0, 4.0),
        dm::nth_derivative<0, 3>(((dv::x * dv::x) * dv::x) * dv::x)(1.0),
        dm::second_derivative<0>((dv::x * dv::x) * dv::x)(2.0),
        dm::derivative<0>(dm::constant(5.0))(1.0),
        dm::derivative<1>(dv::x)(1.0, 2.0) };

    // numeric_derivative, the 5-point rule, Taylor and Maclaurin
    D_CONSTEXPR double g_numeric =
        dm::numeric_derivative(d_tests_square(), 3.0, 0.5);
    D_CONSTEXPR double g_gauss_square =
        dm::gauss_legendre5(d_tests_square(), 0.0, 1.0);
    D_CONSTEXPR double g_gauss_cubic =
        dm::gauss_legendre5(d_tests_cubic(), -1.0, 2.0);
    D_CONSTEXPR double g_taylor =
        dm::taylor_eval<3>((dv::x * dv::x) * dv::x, 1.0, 2.0);
    D_CONSTEXPR double g_maclaurin =
        dm::maclaurin_eval<2>(1.0 + dv::x + dv::x * dv::x, 0.5);

    // sequences and sums whose forms are one return
    D_CONSTEXPR dm::arithmetic_sequence<double> g_arith(2.0, 3.0);
    D_CONSTEXPR double g_arith_term = g_arith.term(4);
    D_CONSTEXPR double g_arith_sum  = g_arith.partial_sum(5);
    D_CONSTEXPR double g_arith_free = dm::arithmetic_sum(2.0, 3.0, 5);
    D_CONSTEXPR double g_geom_inf   = dm::geometric_sum_infinite(1.0, 0.5);
    D_CONSTEXPR double g_general    =
        dm::make_sequence(d_tests_n_squared()).term(6);

    // the scalar helpers that are one return
    D_CONSTEXPR double g_rounding[10] = {
        dm::fn::abs(-2.5),     dm::fn::floor(2.7),  dm::fn::floor(-2.7),
        dm::fn::ceil(2.1),     dm::fn::ceil(-2.1),  dm::fn::trunc(-2.7),
        dm::fn::round(2.5),    dm::fn::round(-2.5), dm::fn::fract(2.25),
        dm::fn::floor(3.0) };
    D_CONSTEXPR double g_clamps[4] = {
        dm::fn::clamp(5.0, 0.0, 1.0), dm::fn::saturate(-0.5),
        dm::fn::lerp(2.0, 6.0, 0.25), dm::fn::degrees(dm::fn::radians(90.0)) };
    D_CONSTEXPR int g_signs[3] = {
        dm::fn::sign(-3.0), dm::fn::sign(0.0), dm::fn::sign(7) };
    D_CONSTEXPR unsigned long long g_factorials[5] = {
        dm::fn::factorial(0), dm::fn::factorial(1), dm::fn::factorial(5),
        dm::fn::factorial(20), dm::fn::factorial(25) };
    D_CONSTEXPR int g_gcds[6] = {
        dm::fn::gcd(12, 18), dm::fn::gcd(-12, 18), dm::fn::gcd(0, 5),
        dm::fn::gcd(1071, 462), dm::fn::lcm(4, 6), dm::fn::lcm(0, 5) };

    // d_tests_cpp11
    //   the forms that are constant expressions from C++11.
    void
    d_tests_cpp11()
    {
        D_TESTS_EXPECT( (g_derivatives[0] == 14.0) &&
                        (g_derivatives[1] == -0.25) &&
                        (g_derivatives[2] == 11.0) &&
                        (g_derivatives[3] == 4.0),
                        "derivatives of polynomials, the quotient rule and "
                        "partials, constant expressions from C++11");
        D_TESTS_EXPECT( (g_derivatives[4] == 24.0) &&
                        (g_derivatives[5] == 12.0) &&
                        (g_derivatives[6] == 0.0) &&
                        (g_derivatives[7] == 0.0),
                        "nth_derivative and second_derivative; a constant's "
                        "and another variable's derivatives 0");
        D_TESTS_EXPECT( (g_numeric == 6.0),
                        "numeric_derivative of a constexpr functor: the "
                        "central difference, exact for a square");
        D_TESTS_EXPECT( d_tests_near(g_gauss_square, 1.0 / 3.0, 1e-15) &&
                        d_tests_near(g_gauss_cubic, 3.75, 1e-14),
                        "gauss_legendre5 a constant expression from C++11, "
                        "exact for a square and a cubic");
        D_TESTS_EXPECT( (g_taylor == 8.0) && (g_maclaurin == 1.75),
                        "taylor_eval and maclaurin_eval of polynomials, "
                        "exact at their degree");
        D_TESTS_EXPECT( (g_arith_term == 14.0) && (g_arith_sum == 40.0) &&
                        (g_arith_free == 40.0) && (g_geom_inf == 2.0) &&
                        (g_general == 36.0),
                        "the arithmetic sequence and sum, the infinite "
                        "geometric sum, a functor's sequence");
        D_TESTS_EXPECT( (g_rounding[0] == 2.5) && (g_rounding[1] == 2.0) &&
                        (g_rounding[2] == -3.0) && (g_rounding[3] == 3.0) &&
                        (g_rounding[4] == -2.0) && (g_rounding[5] == -2.0) &&
                        (g_rounding[6] == 3.0) && (g_rounding[7] == -3.0) &&
                        (g_rounding[8] == 0.25) && (g_rounding[9] == 3.0),
                        "abs, floor, ceil, trunc, round and fract, one "
                        "return each");
        D_TESTS_EXPECT( (g_clamps[0] == 1.0) && (g_clamps[1] == 0.0) &&
                        (g_clamps[2] == 3.0) &&
                        d_tests_near(g_clamps[3], 90.0, 1e-12) &&
                        (g_signs[0] == -1) && (g_signs[1] == 0) &&
                        (g_signs[2] == 1),
                        "clamp, saturate, lerp, radians and degrees; sign");

        // factorial(25) wraps modulo 2^64, as the loop's product did
        unsigned long long wrapped = 1;

        for (unsigned i = 2; i <= 25; ++i)
        {
            wrapped *= i;
        }

        D_TESTS_EXPECT( (g_factorials[0] == 1ull) &&
                        (g_factorials[1] == 1ull) &&
                        (g_factorials[2] == 120ull) &&
                        (g_factorials[3] == 2432902008176640000ull) &&
                        (g_factorials[4] == wrapped),
                        "factorial by halving, the loop's product at 0, 1, "
                        "5 and 20, and modulo 2^64 at 25");
        D_TESTS_EXPECT( (g_gcds[0] == 6) && (g_gcds[1] == 6) &&
                        (g_gcds[2] == 5) && (g_gcds[3] == 21) &&
                        (g_gcds[4] == 12) && (g_gcds[5] == 0),
                        "gcd by Euclid's steps, a negative's magnitude, "
                        "lcm, 0 where an argument is");

        return;
    }


    //==========================================================================
    // 2.  AT RUN TIME, AT EVERY LEVEL FROM C++11
    //==========================================================================

    // d_tests_run_time
    //   the derivatives of the transcendentals as the C core's kernels
    // composed by the rules; log and atan the kernels themselves; the
    // quadrature rules, the sums, the sequences, the gradient.
    void
    d_tests_run_time()
    {
        const double a = 0.7;
        const double b = 2.5;

    #if !D_MATH_DBL_EVAL_WIDE
        // the one return's bits the loop's, at run time, the two compiled
        // alike (fused or not); on a 32-bit x87 the sum is kept wider
        volatile double lo = -1.0;
        bool            same = true;

        for (int k = 0; k < 200; ++k)
        {
            const double l = lo + 0.01 * k;
            const double h = 2.0 + 0.007 * k;

            same = same &&
                   d_tests_same_bits(dm::gauss_legendre5(d_tests_cubic(), l, h),
                                     d_tests_gauss_loop(d_tests_cubic(), l, h));
        }

        D_TESTS_EXPECT( same,
                        "gauss_legendre5 one return, the loop's bits at 200 "
                        "intervals");
    #endif  // !D_MATH_DBL_EVAL_WIDE

        D_TESTS_EXPECT( d_tests_same_bits(dm::derivative<0>(dm::sin(dv::x))(a),
                                          d_math_cos_c(a) * 1.0) &&
                        d_tests_same_bits(dm::derivative<0>(dm::cos(dv::x))(a),
                                          -d_math_sin_c(a) * 1.0) &&
                        d_tests_same_bits(dm::derivative<0>(dm::tan(dv::x))(a),
                                          1.0 / (d_math_cos_c(a) *
                                                 d_math_cos_c(a))) &&
                        d_tests_same_bits(dm::derivative<0>(dm::sqrt(dv::x))(b),
                                          1.0 / (2.0 * d_math_sqrt_c(b))),
                        "d/dx of sin, cos, tan and sqrt: the C core's kernels "
                        "composed by the chain rule, bit for bit");
        D_TESTS_EXPECT( (dm::derivative<0>(dm::abs(dv::x))(3.0) == 1.0) &&
                        (dm::derivative<0>(dm::abs(dv::x))(-3.0) == -1.0) &&
                        d_tests_same_bits(
                            dm::derivative<0>(dm::atan(dv::x))(0.8),
                            1.0 / (1.0 + 0.8 * 0.8)) &&
                        d_tests_same_bits(
                            dm::derivative<0>(dm::log(dv::x))(b), 1.0 / b),
                        "d/dx of abs, atan and log by their rules");
        D_TESTS_EXPECT( d_tests_same_bits(dm::fn::atan(0.8),
                                          d_math_atan_c(0.8)) &&
                        d_tests_same_bits(dm::fn::atan(-37.5),
                                          d_math_atan_c(-37.5)) &&
                        d_tests_same_bits(dm::atan(dv::x)(1e-300),
                                          d_math_atan_c(1e-300)),
                        "atan is the C core's d_math_atan_c, bit for bit, "
                        "where this header's own kernel was");

    #if !D_MATH_DBL_EVAL_WIDE
        D_TESTS_EXPECT( d_tests_same_bits(dm::fn::log(b), d_math_log_c(b)) &&
                        d_tests_same_bits(dm::fn::log(1e300),
                                          d_math_log_c(1e300)) &&
                        d_tests_same_bits(dm::fn::log(4.9e-324),
                                          d_math_log_c(4.9e-324)) &&
                        d_tests_same_bits(dm::fn::log2(8.0),
                                          d_math_log_c(8.0) /
                                          dm::constants::of<double>::ln2()),
                        "log is the double family's correctly rounded "
                        "d_math_log_c, bit for bit, at 1e300 and the least "
                        "subnormal too; log2 its quotient by ln 2");
        D_TESTS_EXPECT( d_tests_same_bits(dm::derivative<0>(dm::exp(dv::x))(a),
                                          d_math_exp_c(a) * 1.0) &&
                        d_tests_near(dm::derivative<0>(
                                         dm::pow(dv::x, dm::constant(3.0)))(2.0),
                                     12.0, 1e-12) &&
                        d_tests_near(dm::derivative<0>(dm::sinh(dv::x))(0.6),
                                     dm::fn::cosh(0.6), 1e-15),
                        "d/dx of exp, of pow through exp and log, and of "
                        "sinh, whose derivative is cosh");
        D_TESTS_EXPECT( d_tests_near(dm::maclaurin_eval<8>(dm::exp(dv::x), 1.0),
                                     2.718281828459045, 1e-5) &&
                        d_tests_near(dm::fn::pow(2.0, 10.0), 1024.0, 1e-12),
                        "a Maclaurin evaluation of exp; pow as exp of a log");
    #endif  // !D_MATH_DBL_EVAL_WIDE

        D_TESTS_EXPECT( d_tests_near(dm::maclaurin_eval<7>(dm::sin(dv::x), 0.5),
                                     d_math_sin_c(0.5), 1e-8) &&
                        d_tests_near(dm::fn::atan2(1.0, 1.0),
                                     dm::constants::of<double>::pi() / 4.0,
                                     1e-15) &&
                        d_tests_near(dm::fn::atan2(1.0, -1.0),
                                     3.0 * dm::constants::of<double>::pi() /
                                     4.0, 1e-15) &&
                        (dm::fn::atan2(0.0, 0.0) == 0.0),
                        "a Maclaurin evaluation of sin; atan2 in its "
                        "quadrants");

        // quadrature, sums and sequences, the same loops at every level
        D_TESTS_EXPECT( d_tests_near(dm::trapezoid(d_tests_square(), 0.0, 1.0,
                                                   2000), 1.0 / 3.0, 1e-6) &&
                        d_tests_near(dm::simpson(d_tests_square(), 0.0, 1.0,
                                                 200), 1.0 / 3.0, 1e-15) &&
                        d_tests_near(dm::romberg<7>(d_tests_square(), 0.0,
                                                    1.0), 1.0 / 3.0, 1e-15) &&
                        d_tests_near(dm::integrate(dm::sin(dv::x), 0.0,
                                         dm::constants::of<double>::pi(), 256),
                                     2.0, 1e-8),
                        "trapezoid, simpson, romberg and integrate");
        D_TESTS_EXPECT( (dm::sum(d_tests_identity(), 1, 100) == 5050.0) &&
                        (dm::partial_sum(g_arith, 5) == 40.0) &&
                        (dm::geometric_sum(1.0, 2.0, 10) == 1023.0) &&
                        (dm::geometric_sequence<double>(1.0, 2.0).term(10) ==
                             1024.0) &&
                        (dm::power_sequence<double>(2).term(5) == 25.0) &&
                        (dm::fibonacci_sequence().term(10) == 55ull) &&
                        (dm::make_sequence(d_tests_n_squared())
                             .partial_sum(4) == 14.0),
                        "sum, partial_sum, geometric_sum, the geometric, "
                        "power and Fibonacci terms, a sequence's partial sum");

        std::array<double, 3> coeffs = {{ 3.0, 2.0, 1.0 }};
        std::array<double, 2> point  = {{ 3.0, 4.0 }};
        std::array<double, 2> grad   =
            dm::gradient<2>(dv::x * dv::x + dv::y * dv::y)(3.0, 4.0);

        D_TESTS_EXPECT( (dm::fn::binomial(6, 2) == 15ull) &&
                        (dm::fn::binomial(4, 7) == 0ull) &&
                        (dm::fn::horner(coeffs, 2.0) == 17.0) &&
                        (grad[0] == 6.0) && (grad[1] == 8.0) &&
                        d_tests_near(dm::numeric_partial<1>(
                                         dv::x * dv::x + dv::y * dv::y, point),
                                     8.0, 1e-6),
                        "binomial, horner, the gradient and a numeric "
                        "partial");

        return;
    }


    //==========================================================================
    // 3.  CONSTANT EXPRESSIONS FROM C++14
    //==========================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CONSTEXPR double g_dsin  = dm::derivative<0>(dm::sin(dv::x))(0.0);
    D_CONSTEXPR double g_atan  = dm::fn::atan(0.8);
    D_CONSTEXPR double g_simpson =
        dm::simpson(d_tests_square(), 0.0, 1.0, 200);
    D_CONSTEXPR double g_romberg = dm::romberg<6>(d_tests_square(), 0.0, 1.0);
    D_CONSTEXPR double g_integral =
        dm::integrate(dm::sin(dv::x), 0.0, dm::constants::of<double>::pi(),
                      256);
    D_CONSTEXPR double g_sum = dm::sum(d_tests_identity(), 1, 100);
    D_CONSTEXPR double g_geom_term =
        dm::geometric_sequence<double>(1.0, 2.0).term(10);
    D_CONSTEXPR unsigned long long g_fib = dm::fibonacci_sequence().term(10);
    D_CONSTEXPR unsigned long long g_binomial = dm::fn::binomial(6, 2);
    D_CONSTEXPR double g_horner =
        dm::fn::horner(std::array<double, 3>{{ 3.0, 2.0, 1.0 }}, 2.0);
    D_CONSTEXPR double g_partial =
        dm::numeric_partial<0>(dv::x * dv::y,
                               std::array<double, 2>{{ 3.0, 4.0 }}, 0.5);
    #if !D_MATH_DBL_EVAL_WIDE
    D_CONSTEXPR double g_log   = dm::fn::log(2.5);
    D_CONSTEXPR double g_maclaurin_exp =
        dm::maclaurin_eval<8>(dm::exp(dv::x), 1.0);
    #endif  // !D_MATH_DBL_EVAL_WIDE

    // d_tests_cpp14
    //   the transcendentals, the loops and std::array's reads folded at
    // compile time from C++14, with the run time's bits.
    void
    d_tests_cpp14()
    {
        volatile double at = 0.0;

        D_TESTS_EXPECT( d_tests_same_bits(g_dsin,
                            dm::derivative<0>(dm::sin(dv::x))(at)) &&
                        d_tests_same_bits(g_atan,
                            dm::fn::atan(at + 0.8)),
                        "a transcendental's derivative and atan constant "
                        "expressions from C++14, the run time's bits");
        D_TESTS_EXPECT( d_tests_same_bits(g_simpson,
                            dm::simpson(d_tests_square(), at, 1.0, 200)) &&
                        d_tests_same_bits(g_romberg,
                            dm::romberg<6>(d_tests_square(), at, 1.0)) &&
                        d_tests_same_bits(g_integral,
                            dm::integrate(dm::sin(dv::x), at,
                                          dm::constants::of<double>::pi(),
                                          256)),
                        "simpson, romberg and integrate constant expressions "
                        "from C++14, the run time's bits");
        D_TESTS_EXPECT( (g_sum == 5050.0) && (g_geom_term == 1024.0) &&
                        (g_fib == 55ull) && (g_binomial == 15ull) &&
                        (g_horner == 17.0) && (g_partial == 4.0),
                        "the loops' sums and terms, binomial, horner and a "
                        "numeric partial constant expressions from C++14");

    #if !D_MATH_DBL_EVAL_WIDE
        D_TESTS_EXPECT( d_tests_same_bits(g_log, d_math_log_c(at + 2.5)) &&
                        d_tests_same_bits(g_maclaurin_exp,
                            dm::maclaurin_eval<8>(dm::exp(dv::x), at + 1.0)),
                        "log and a Maclaurin evaluation of exp constant "
                        "expressions from C++14, the run time's bits");
    #endif  // !D_MATH_DBL_EVAL_WIDE

        return;
    }
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    D_TESTS_EXPECT( (DJINTERP_MATH_CALCULUS_CALCULUS_HPP == 1) &&
                    (DJINTERP_MATH_CALCULUS_DIFFERENTIATION_HPP == 1) &&
                    (DJINTERP_MATH_CALCULUS_ELEMENTARY_HPP == 1) &&
                    (DJINTERP_MATH_CALCULUS_INTEGRATION_HPP == 1) &&
                    (DJINTERP_MATH_CALCULUS_SEQUENCE_HPP == 1) &&
                    (DJINTERP_MATH_CALCULUS_SERIES_HPP == 1),
                    "every calculus header reads at every level, empty "
                    "below C++11");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_cpp11();
    d_tests_run_time();
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    d_tests_cpp14();
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

    std::printf("t_calculus_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
