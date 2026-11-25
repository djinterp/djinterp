/*******************************************************************************
* djinterp [cpp][math]                                     t_expression_face.cpp
*
*   Tests for math/expression.hpp, the compile-time core, at every level from
* C++98 (step 29, 5a; S430). Below C++11 the header is empty. From C++11:
* the leaves, the nodes and their arithmetic, + - * / and unary minus with
* scalars lifted, squared, abs and dot, each a constant expression; the
* named placeholders x to phi at their indices, in djinterp::math::variables,
* so that a function of djinterp::math may name a local x under -Wshadow
* -Werror (S403); exp, sin, cos, tan and sqrt the C core's `_c` kernels bit
* for bit, where the header's own kernels gave sqrt(1e300) as 7.89e269 and
* exp up to 10,800 ulps off; abs exact. From C++14: each transcendental a
* constant expression with the run time's bits, one answer at every level,
* where a C++20 program had two (N3), and var<I, T>. The double exp where
* double is evaluated as double: on the x87 it refuses to compile (S418).
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/t_expression_face.cpp                \
*         -o t_expression -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_expression_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/expression.hpp"  // the
                                                          // tested header
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/elementary.h"   // sqrt_c,
                                                       // sin_c, cos_c
#include "../../../../../inc/djinterp/c/modules/math/exponential.h"  // exp_c
#include "../../../../../inc/djinterp/c/modules/math/tangent.h"      // tan_c
// std
#include <cfloat>   // DBL_EPSILON
#include <cmath>    // std::ldexp
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

    // d_tests_same_bits
    //   whether two doubles have the same bits: one sign for zeros, both NaNs
    // for NaNs.
    bool
    d_tests_same_bits(
        double _a,
        double _b
    )
    {
        unsigned char a[sizeof(double)];
        unsigned char b[sizeof(double)];

        if ( (_a != _a) ||
             (_b != _b) )
        {
            return ( (_a != _a) &&
                     (_b != _b) );
        }

        std::memcpy(a, &_a, sizeof(double));
        std::memcpy(b, &_b, sizeof(double));

        return (std::memcmp(a, b, sizeof(double)) == 0);
    }

}  // namespace


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

//==============================================================================
// 1.  NO SHADOWING (S403)
//==============================================================================
// A function of djinterp::math with locals named as the placeholders: under
// -Wshadow -Werror this compiles only where the placeholders are not
// djinterp::math's own.
NS_DJINTERP
NS_MATH

    // d_tests_locals_named_as_placeholders
    //   a sum of eight locals named x to phi.
    inline double
    d_tests_locals_named_as_placeholders(
        double _v
    )
    {
        const double x     = _v;
        const double y     = (2.0 * _v);
        const double z     = (3.0 * _v);
        const double t     = (4.0 * _v);
        const double r     = (5.0 * _v);
        const double rho   = (6.0 * _v);
        const double theta = (7.0 * _v);
        const double phi   = (8.0 * _v);

        return ( ( (x + y) + (z + t) ) + ( (r + rho) + (theta + phi) ) );
    }

NS_END  // math
NS_END  // djinterp


namespace
{
    namespace dm = djinterp::math;
    namespace dv = djinterp::math::variables;

    //==========================================================================
    // 2.  THE NODES, CONSTANT EXPRESSIONS FROM C++11
    //==========================================================================

    // each a constant expression from C++11: the arithmetic of the leaves,
    // with scalars lifted
    D_CONSTEXPR double g_quadratic =
        (dm::constant(2.0) * dm::squared(dv::x))(3.0);
    D_CONSTEXPR double g_mixed =
        ( ( (dv::x + 1.0) * (2.0 - dv::y) ) / (dv::z - dv::t) )(1.0, 5.0, 3.0,
                                                                 7.0);
    D_CONSTEXPR double g_negated = (-(dv::x * 4))(2.5);
    D_CONSTEXPR double g_abs     = dm::abs(dv::x - 10.0)(3.0);
    D_CONSTEXPR double g_dot     =
        dm::dot(dm::constant(dm::vec3{ 1.0, 2.0, 3.0 }),
                dm::constant(dm::vec3{ 4.0, -5.0, 6.0 }))();
    D_CONSTEXPR double g_at[8] = {
        dv::x(1.0, 2.0, 3.0, 4.0),     dv::y(1.0, 2.0, 3.0, 4.0),
        dv::z(1.0, 2.0, 3.0, 4.0),     dv::t(1.0, 2.0, 3.0, 4.0),
        dv::r(1.0, 2.0, 3.0, 4.0),     dv::rho(1.0, 2.0, 3.0, 4.0),
        dv::theta(1.0, 2.0, 3.0, 4.0), dv::phi(1.0, 2.0, 3.0, 4.0) };

    // d_tests_nodes
    //   the nodes' values, each folded at compile time; their arities and
    // value types.
    void
    d_tests_nodes()
    {
        D_TESTS_EXPECT( (g_quadratic == 18.0) &&
                        (g_mixed == 1.5) &&
                        (g_negated == -10.0) &&
                        (g_abs == 7.0) &&
                        (g_dot == 12.0),
                        "2 x^2 at 3, (x + 1)(2 - y) / (z - t), -(4 x), "
                        "abs(x - 10) and a dot of two vec3s, each a constant "
                        "expression from C++11, scalars lifted on either side "
                        "(S430)");
        D_TESTS_EXPECT( (g_at[0] == 1.0) && (g_at[1] == 2.0) &&
                        (g_at[2] == 3.0) && (g_at[3] == 4.0) &&
                        (g_at[4] == 1.0) && (g_at[5] == 1.0) &&
                        (g_at[6] == 2.0) && (g_at[7] == 3.0),
                        "x, y, z, t at arguments 0 to 3; r and rho at 0, "
                        "theta at 1, phi at 2; constant expressions from "
                        "C++11, in djinterp::math::variables (S403)");
        D_TESTS_EXPECT(dm::d_tests_locals_named_as_placeholders(1.0) == 36.0,
                       "a function of djinterp::math names locals x to phi "
                       "under -Wshadow -Werror (S403)");

        typedef dm::binary_node<dm::internal::op_add,
                                dm::variable_node<0>,
                                dm::constant_node<double> > sum_type;
        typedef dm::unary_node<dm::internal::fn_sqrt,
                               dm::variable_node<2> > root_type;

        D_TESTS_EXPECT( (sum_type::arity == 1) &&
                        (root_type::arity == 3) &&
                        (dm::constant_node<double>::arity == 0) &&
                        (dm::is_expression<sum_type>::value) &&
                        (!dm::is_expression<double>::value) &&
                        (sizeof(sum_type::value_type) == sizeof(double)) &&
                        (sizeof((dv::x + 1)(2.0)) == sizeof(double)),
                        "arity the most arguments a node reads; "
                        "is_expression of nodes alone; x + 1's value a "
                        "double");

        return;
    }


    //==========================================================================
    // 3.  THE TRANSCENDENTALS: THE C CORE'S KERNELS
    //==========================================================================

    const double g_args[] = {
        -745.25, -20.5, -1.0, -0.5, -1e-300, -0.0, 0.0, 1e-310, 1e-8, 0.25,
        0.5,     1.0,   2.5,  3.0,  10.0,    100.0, 709.75, 1e6,  1e22, 1e300 };

    const std::size_t g_count = (sizeof(g_args) / sizeof(g_args[0]));

    // d_tests_kernels
    //   each transcendental of the language the C core's `_c` kernel's bits,
    // over arguments small and large; the header's own kernels' failures.
    void
    d_tests_kernels()
    {
        bool sines   = true;
        bool roots   = true;
        bool tangent = true;

        for (std::size_t i = 0; i < g_count; ++i)
        {
            const double a = g_args[i];

            sines   = ( (sines) &&
                        (d_tests_same_bits(dm::sin(dv::x)(a),
                                           d_math_sin_c(a))) &&
                        (d_tests_same_bits(dm::cos(dv::x)(a),
                                           d_math_cos_c(a))) );
            tangent = ( (tangent) &&
                        (d_tests_same_bits(dm::tan(dv::x)(a),
                                           d_math_tan_c(a))) );
            roots   = ( (roots) &&
                        (d_tests_same_bits(dm::sqrt(dv::x)(a),
                                           d_math_sqrt_c(a))) );
        }

        D_TESTS_EXPECT( (sines) &&
                        (tangent) &&
                        (roots),
                        "sin, cos, tan and sqrt the C core's d_math_sin_c, "
                        "cos_c, tan_c and sqrt_c, bit for bit, from -745.25 "
                        "to 1e300 (N3, S430)");

        // 2^998's root is 2^499; 1e300's within an ulp of 1e150; -1's NaN
        const double root = dm::sqrt(dv::x)(1e300);

        D_TESTS_EXPECT( (dm::sqrt(dv::x)(std::ldexp(1.0, 998)) ==
                             std::ldexp(1.0, 499)) &&
                        (root > (1e150 * (1.0 - DBL_EPSILON))) &&
                        (root < (1e150 * (1.0 + DBL_EPSILON))) &&
                        (dm::sqrt(dv::x)(-1.0) != dm::sqrt(dv::x)(-1.0)) &&
                        (d_tests_same_bits(dm::sqrt(dv::x)(-0.0), -0.0)),
                        "sqrt(2^998) 2^499, sqrt(1e300) 1e150 within an ulp "
                        "where it was 7.89e269, sqrt(-1) NaN where it was 0, "
                        "sqrt(-0) -0 (S430)");
        D_TESTS_EXPECT( (d_tests_same_bits(dm::abs(dv::x)(-0.0), 0.0)) &&
                        (dm::abs(dv::x)(-2.5) == 2.5) &&
                        (dm::abs(dv::x)(2.5) == 2.5) &&
                        (dm::abs(dv::x)(-g_args[0]) == 745.25) &&
                        (dm::abs(dv::x)(dm::sqrt(dv::x)(-1.0)) !=
                             dm::abs(dv::x)(dm::sqrt(dv::x)(-1.0))),
                        "abs exact: of -0 +0, of a NaN a NaN (S430)");

#if !D_MATH_DBL_EVAL_WIDE
        bool exps = true;

        for (std::size_t i = 0; i < g_count; ++i)
        {
            exps = ( (exps) &&
                     (d_tests_same_bits(dm::exp(dv::x)(g_args[i]),
                                        d_math_exp_c(g_args[i]))) );
        }

        D_TESTS_EXPECT( (exps) &&
                        (dm::exp(dv::x)(1.0) == 2.718281828459045) &&
                        (dm::exp(dv::x)(0.0) == 1.0) &&
                        (dm::exp(dv::x)(-745.25) == 0.0),
                        "exp the C core's correctly rounded d_math_exp_c, "
                        "bit for bit, where it was 10,800 ulps off; exp(1) "
                        "e's double (N3, S430)");
#endif  // !D_MATH_DBL_EVAL_WIDE

        return;
    }


    //==========================================================================
    // 4.  ONE ANSWER: CONSTANT EXPRESSIONS FROM C++14
    //==========================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CONSTEXPR double g_sin_c  = dm::sin(dv::x)(1e22);
    D_CONSTEXPR double g_cos_c  = dm::cos(dv::y)(0.0, 3.0);
    D_CONSTEXPR double g_tan_c  = dm::tan(dv::x * 2.0)(0.75);
    D_CONSTEXPR double g_sqrt_c = dm::sqrt(dv::x + dv::y)(1.0, 1.0);
    D_CONSTEXPR double g_var_c  = dm::var<2>(1.0, 2.0, 3.0);

    #if !D_MATH_DBL_EVAL_WIDE
    D_CONSTEXPR double g_exp_c  = (dm::constant(2.0) * dm::exp(-dv::x))(0.5);
    #endif  // !D_MATH_DBL_EVAL_WIDE

    // d_tests_one_answer
    //   each transcendental folded at compile time the run time's bits, the
    // run time's arguments read through volatiles.
    void
    d_tests_one_answer()
    {
        volatile double big  = 1e22;
        volatile double zero = 0.0;
        volatile double tan_at = 0.75;
        volatile double one  = 1.0;

        D_TESTS_EXPECT( (d_tests_same_bits(g_sin_c, dm::sin(dv::x)(big))) &&
                        (d_tests_same_bits(g_cos_c,
                                           dm::cos(dv::y)(zero, 3.0))) &&
                        (d_tests_same_bits(g_tan_c,
                                           dm::tan(dv::x * 2.0)(tan_at))) &&
                        (d_tests_same_bits(g_sqrt_c,
                                           dm::sqrt(dv::x + dv::y)(one,
                                                                   one))) &&
                        (d_tests_same_bits(g_sqrt_c, d_math_sqrt_c(2.0))) &&
                        (g_var_c == 3.0),
                        "sin, cos, tan and sqrt constant expressions from "
                        "C++14 with the run time's bits: one answer, where a "
                        "C++20 program had two; var<2> the third argument "
                        "(N3, S430)");

    #if !D_MATH_DBL_EVAL_WIDE
        volatile double half = 0.5;

        D_TESTS_EXPECT(d_tests_same_bits(g_exp_c,
                                         (dm::constant(2.0) *
                                          dm::exp(-dv::x))(half)),
                       "exp a constant expression from C++14 with the run "
                       "time's bits (N3, S430)");
    #endif  // !D_MATH_DBL_EVAL_WIDE

        return;
    }
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    D_TESTS_EXPECT(DJINTERP_MATH_EXPRESSION_HPP == 1,
                   "the header reads at every level, empty below C++11 "
                   "(N1, S430)");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_nodes();
    d_tests_kernels();
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    d_tests_one_answer();
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

    // the helper is the C++11 section's
    (void)&d_tests_same_bits;

    std::printf("t_expression_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
