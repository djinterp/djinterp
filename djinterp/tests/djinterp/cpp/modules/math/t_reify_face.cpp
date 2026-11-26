/*******************************************************************************
* djinterp [cpp][math]                                          t_reify_face.cpp
*
*   Tests for math/expr/reify.hpp, the bridge from a compile-time expression
* to a term of the run-time language (step 32, 5d), at every level from
* C++98. Below C++11 the header is empty. From C++11: each leaf's term, the
* numbers' kinds (an integer below 2^53 exact, -0 and any other its double)
* and the variables' names, by index and given; each operator's, function's,
* relation's and logic's term, piecewise's and a vector's, by the text
* expr_render writes of it, which expr_parse reads back to the same value;
* the term's value in double the expression's, bit for bit, where its
* arithmetic, sqrt and abs are all it holds, and each function's within an
* ulp of the C core's kernel, the plain functions' being the language's
* (ruling 12), exp's bit for bit, as the plain exp is the kernel (S478); a
* derivative reified, simplified to its text, and its value
* the compile-time derivative's; the handle math::expr over a reified term.
* From C++14: constants.hpp's rational, an exact fraction. Where double is
* evaluated wider than double, a 32-bit x87, the compile-time face's run time
* keeps its sums wider than the language's fold, and the bit-for-bit checks
* of the arithmetic are left out, with those of exp and log, which refuse to
* compile there (S418, S426).
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/t_reify_face.cpp                     \
*         -o t_reify -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_reify_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.24
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/expr/reify.hpp"  // the
                                                                // tested
                                                                // header
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
#include <cmath>   // std::ldexp, std::fabs, std::ilogb
#include <limits>  // std::numeric_limits
#include <string>  // std::string
// djinterp
#include "../../../../../inc/djinterp/cpp/modules/math/calculus/differentiation.hpp"  // derivative
#include "../../../../../inc/djinterp/cpp/modules/math/expr/evaluate.hpp"  // expr_evaluate
#include "../../../../../inc/djinterp/cpp/modules/math/expr/expr.hpp"      // math::expr
#include "../../../../../inc/djinterp/cpp/modules/math/expr/parse.hpp"     // expr_parse
#include "../../../../../inc/djinterp/cpp/modules/math/expr/render.hpp"    // expr_render
#include "../../../../../inc/djinterp/cpp/modules/math/expr/simplify.hpp"  // expr_simplify

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

    // d_tests_within_ulp
    //   whether _a is _b or a neighbour of it.
    bool
    d_tests_within_ulp(
        double _a,
        double _b
    )
    {
        if (_a == _b)
        {
            return true;
        }

        return ( std::fabs(_a - _b) <=
                 std::ldexp(1.0, std::ilogb(_b) - 52) );
    }

    // d_tests_text
    //   the text of a term.
    std::string
    d_tests_text(
        const dm::expr_term& _term
    )
    {
        return dm::expr_render(_term);
    }

    // d_tests_kind
    //   the value kind of a term that is a number.
    int
    d_tests_kind(
        const dm::expr_term& _term
    )
    {
        return ( dm::expr_is_number(_term)
                 ? static_cast<int>(dm::expr_atom_of(_term).number.kind())
                 : -1 );
    }

    // d_tests_value
    //   a term's value in double at x, y and z, NaN for an error.
    double
    d_tests_value(
        const dm::expr_term& _term,
        double               _x,
        double               _y,
        double               _z
    )
    {
        const dm::expr_result<double> r = dm::expr_evaluate<double>(
            _term, dm::at("x", _x).at("y", _y).at("z", _z));

        return ( r.is_ok() ? r.value()
                           : std::numeric_limits<double>::quiet_NaN() );
    }

    // d_tests_reads_back
    //   whether a term's text reads back as a term of the same value, in
    // double, at x, y and z.
    bool
    d_tests_reads_back(
        const dm::expr_term& _term,
        double               _x,
        double               _y,
        double               _z
    )
    {
        const dm::expr_result<dm::expr_term> back =
            dm::expr_parse(d_tests_text(_term));

        return ( (back.is_ok()) &&
                 (d_tests_same_bits(d_tests_value(back.value(), _x, _y, _z),
                                    d_tests_value(_term, _x, _y, _z))) );
    }

    // d_tests_leaves
    //   each leaf's term: numbers and their kinds, variables and their
    // names.
    void
    d_tests_leaves()
    {
        const dm::expr_term half = dm::reify(dm::constant(2.5));
        const dm::expr_term two  = dm::reify(dm::constant(2.0));

        D_TESTS_EXPECT( (d_tests_text(half) == "2.5") &&
                        (d_tests_kind(half) == D_MATH_VALUE_DOUBLE) &&
                        (d_tests_text(two) == "2") &&
                        (d_tests_kind(two) == D_MATH_VALUE_INTEGER),
                        "a double its double; an integral one the exact "
                        "integer");
        D_TESTS_EXPECT( (d_tests_text(dm::reify(dm::constant(-0.0))) ==
                             "(-0)") &&
                        (d_tests_kind(dm::reify(dm::constant(-0.0))) ==
                             D_MATH_VALUE_DOUBLE) &&
                        (d_tests_kind(dm::reify(dm::constant(
                             9007199254740992.0))) == D_MATH_VALUE_DOUBLE) &&
                        (d_tests_kind(dm::reify(dm::constant(
                             -9007199254740991.0))) == D_MATH_VALUE_INTEGER) &&
                        (d_tests_text(dm::reify(dm::constant(2.5e300))) ==
                             "2.5e+300"),
                        "-0, 2^53 and 2.5e300 doubles; -(2^53 - 1) exact");
        D_TESTS_EXPECT( (d_tests_kind(dm::reify(dm::constant(7))) ==
                             D_MATH_VALUE_INTEGER) &&
                        (d_tests_kind(dm::reify(dm::constant(0.75f))) ==
                             D_MATH_VALUE_FLOAT) &&
                        (d_tests_text(dm::reify(dm::constant(true))) ==
                             "true") &&
                        (dm::expr_is_constant(
                             dm::reify(dm::constant(false)))),
                        "an int exact, a float its float, a bool the "
                        "constant true or false");
        D_TESTS_EXPECT( d_tests_text(dm::reify(dm::constant(
                            dm::vec3{ 1.0, 2.5, -3.0 }))) == "[1, 2.5, (-3)]",
                        "a vec3 a vector literal");
        D_TESTS_EXPECT( (d_tests_text(dm::reify(dv::x)) == "x") &&
                        (d_tests_text(dm::reify(dv::y)) == "y") &&
                        (d_tests_text(dm::reify(dv::z)) == "z") &&
                        (d_tests_text(dm::reify(dv::t)) == "t"),
                        "x, y, z and t by index");
        D_TESTS_EXPECT( (d_tests_text(dm::reify(dv::rho * dm::cos(dv::theta)
                                                + dv::phi,
                                                "r", "theta", "phi")) ==
                             "r * cos(theta) + phi") &&
                        (d_tests_text(dm::reify(dv::y * dv::x,
                                                std::string("u"),
                                                std::string("v"))) ==
                             "v * u"),
                        "the names given, in index order");

        return;
    }

    // d_tests_operators
    //   each operator's, function's, relation's and logic's term, by its
    // text, which reads back to the same value.
    void
    d_tests_operators()
    {
        const dm::expr_term arith =
            dm::reify(dm::constant(2.0) * dm::squared(dv::x) + 1.0);
        const dm::expr_term mixed = dm::reify(-(dv::x + dv::y) * dv::z);
        const dm::expr_term fns   =
            dm::reify(dm::exp(dm::sin(dv::x)) + dm::log(dv::y) -
                      dm::cos(dv::z) * dm::tan(dv::x));
        const dm::expr_term more  =
            dm::reify(dm::atan(dv::x) / dm::sqrt(dm::abs(dv::y)));
        const dm::expr_term rel   =
            dm::reify(((dv::x < 0.0) && !(dv::y >= 1.0)) ||
                      (dv::z == 2.0));
        const dm::expr_term pw    =
            dm::reify(dm::piecewise(dm::when(dv::x < 0.0, -dv::x),
                                    dm::when(dv::x < 1.0, dv::x * dv::x),
                                    dm::otherwise(dv::y)));

        D_TESTS_EXPECT( (d_tests_text(arith) == "2 * (x * x) + 1") &&
                        (d_tests_text(mixed) == "-(x + y) * z") &&
                        (d_tests_text(dm::reify(2 * dv::x)) == "2 * x") &&
                        (d_tests_text(dm::reify(dv::x / dv::y - 0.1)) ==
                             "x / y - 0.1"),
                        "+ - * / and unary -, in the expression's shape");
        D_TESTS_EXPECT( (d_tests_text(fns) ==
                             "exp(sin(x)) + log(y) - cos(z) * tan(x)") &&
                        (d_tests_text(more) == "atan(x) / sqrt(abs(y))"),
                        "exp, sin, cos, tan, log, atan, sqrt and abs calls");
        D_TESTS_EXPECT( d_tests_text(rel) ==
                            "x < 0 and not y >= 1 or z = 2",
                        "the relations, and, or and not");
        D_TESTS_EXPECT( d_tests_text(pw) ==
                            "piecewise(when(x < 0, -x), when(x < 1, x * x), "
                            "otherwise(y))",
                        "piecewise, its last branch its otherwise");
        D_TESTS_EXPECT( (d_tests_text(dm::reify(dm::piecewise(
                             dm::when(dv::x < 0.0, -dv::x),
                             dm::when(dv::x > 5.0, dv::y)))) ==
                             "piecewise(when(x < 0, -x), otherwise(y))") &&
                        (d_tests_text(dm::reify(dm::vec(dv::x,
                                                        dv::y * 2.0,
                                                        3.0))) ==
                             "[x, y * 2, 3]") &&
                        (d_tests_text(dm::reify(dm::dot(
                             dm::constant(dm::vec3{ 1.0, 2.5, -3.0 }),
                             dm::variable_node<0, dm::vec3>()))) ==
                             "dot([1, 2.5, (-3)], x)"),
                        "a last condition the compile-time face never "
                        "tests dropped; a vector; dot");

        bool back = true;

        // each text read back, at points of both signs
        for (int i = 0; i < 40; ++i)
        {
            const double a = -2.0 + (0.1 * i);
            const double b = 0.35 + (0.07 * i);
            const double c = 2.0 - (0.13 * i);

            back = back &&
                   d_tests_reads_back(arith, a, b, c) &&
                   d_tests_reads_back(mixed, a, b, c) &&
                   d_tests_reads_back(fns, a, b, c) &&
                   d_tests_reads_back(more, a, b, c) &&
                   d_tests_reads_back(rel, a, b, c) &&
                   d_tests_reads_back(pw, a, b, c) &&
                   d_tests_reads_back(dm::reify(dv::x / dv::y - 0.1),
                                      a, b, c);
        }

        D_TESTS_EXPECT( back,
                        "each text reads back to the same value, at 40 "
                        "points");
        D_TESTS_EXPECT( dm::expr(arith).str() == "2 * (x * x) + 1",
                        "math::expr over a reified term");

        return;
    }

    // d_tests_values
    //   a reified term's value in double: the expression's, bit for bit,
    // where its arithmetic, sqrt and abs are all it holds; each function's
    // within an ulp of the C core's kernel; a relation 1 or 0; a piecewise
    // its branch's.
    void
    d_tests_values()
    {
        const auto e1 = dm::constant(2.0) * dv::x * dv::x / (dv::y - 0.3) + 1.0;
        const auto e2 = -(dv::x + dv::y) * dv::z / dm::constant(7.0);
        const auto e3 = dm::sqrt(dm::abs(dv::x)) / (dv::y * dv::y);
        const auto e4 = dv::x / dv::y - 0.1;
        const dm::expr_term t1 = dm::reify(e1);
        const dm::expr_term t2 = dm::reify(e2);
        const dm::expr_term t3 = dm::reify(e3);
        const dm::expr_term t4 = dm::reify(e4);
        const auto rel = (dv::x < dv::y) || (dv::z >= 1.0);
        const dm::expr_term trel = dm::reify(rel);
        const auto pw = dm::piecewise(dm::when(dv::x < 0.0, -dv::x),
                                      dm::otherwise(dv::x * dv::y));
        const dm::expr_term tpw = dm::reify(pw);

        bool arith = true;
        bool logic = true;
        bool fns   = true;

        // 200 points of both signs
        for (int i = 0; i < 200; ++i)
        {
            const double a = -3.0 + (0.0301 * i);
            const double b = 0.71 + (0.0137 * i);
            const double c = 1.9 - (0.0173 * i);

            arith = arith &&
                    d_tests_same_bits(d_tests_value(t1, a, b, c),
                                      e1(a, b, c)) &&
                    d_tests_same_bits(d_tests_value(t2, a, b, c),
                                      e2(a, b, c)) &&
                    d_tests_same_bits(d_tests_value(t3, a, b, c),
                                      e3(a, b, c)) &&
                    d_tests_same_bits(d_tests_value(t4, a, b, c),
                                      e4(a, b, c));
            logic = logic &&
                    (d_tests_value(trel, a, b, c) ==
                         (rel(a, b, c) ? 1.0 : 0.0)) &&
                    d_tests_same_bits(d_tests_value(tpw, a, b, c),
                                      pw(a, b, c));
            fns   = fns &&
                    d_tests_within_ulp(d_tests_value(dm::reify(dm::sin(dv::x)),
                                                     a, b, c),
                                       dm::sin(dv::x)(a)) &&
                    d_tests_within_ulp(d_tests_value(dm::reify(dm::cos(dv::x)),
                                                     a, b, c),
                                       dm::cos(dv::x)(a)) &&
                    d_tests_within_ulp(d_tests_value(dm::reify(dm::tan(dv::x)),
                                                     a, b, c),
                                       dm::tan(dv::x)(a)) &&
                    d_tests_within_ulp(d_tests_value(dm::reify(dm::atan(dv::x)),
                                                     a, b, c),
                                       dm::atan(dv::x)(a));
    #if !D_MATH_DBL_EVAL_WIDE
            // exp bit for bit: the plain exp is exp_c (ruling 12, S478)
            fns   = fns &&
                    d_tests_same_bits(d_tests_value(dm::reify(dm::exp(dv::x)),
                                                    a, b, c),
                                      dm::exp(dv::x)(a)) &&
                    d_tests_within_ulp(d_tests_value(dm::reify(dm::log(dv::y)),
                                                     a, b, c),
                                       dm::log(dv::y)(a, b));
    #endif  // !D_MATH_DBL_EVAL_WIDE
        }

    #if !D_MATH_DBL_EVAL_WIDE
        D_TESTS_EXPECT( arith,
                        "arithmetic, sqrt and abs: the expression's value, "
                        "bit for bit, at 200 points");
    #endif  // !D_MATH_DBL_EVAL_WIDE
        D_TESTS_EXPECT( logic,
                        "a relation 1 or 0, a piecewise its branch's value, "
                        "at 200 points");
        D_TESTS_EXPECT( fns,
                        "sin, cos, tan and atan, and log where double is "
                        "evaluated as double, within an ulp of the C core's "
                        "kernels, and there exp bit for bit, at 200 points");

        return;
    }

    // d_tests_derivatives
    //   a compile-time derivative reified: simplified to its text, and its
    // value the derivative's.
    void
    d_tests_derivatives()
    {
        const auto d1 = dm::derivative<0>(dv::x * dm::sin(dv::x));
        const auto d2 = dm::derivative<0>(dm::constant(3.0) * dv::x * dv::x +
                                          dm::constant(2.0) * dv::x + 1.0);
        const auto d3 = dm::derivative<1>(dv::x * dv::y * dv::y / dv::x);
        const dm::expr_term t1 = dm::reify(d1);
        const dm::expr_term t2 = dm::reify(d2);

        D_TESTS_EXPECT( (d_tests_text(t1) ==
                             "1 * sin(x) + x * (cos(x) * 1)") &&
                        (d_tests_text(dm::expr_simplify(t1)) ==
                             "sin(x) + x * cos(x)") &&
                        (d_tests_text(dm::expr_simplify(t2)) == "6 * x + 2"),
                        "a derivative's tree as the rules built it, and "
                        "simplified, its coefficients exact");

        bool same = true;

        // a polynomial's derivative, and a partial, at 50 points
        for (int i = 0; i < 50; ++i)
        {
            const double a = -1.7 + (0.071 * i);
            const double b = 0.4 + (0.05 * i);

            same = same &&
                   d_tests_same_bits(d_tests_value(t2, a, b, 0.0),
                                     d2(a)) &&
                   d_tests_same_bits(d_tests_value(dm::reify(d3), a, b, 0.0),
                                     d3(a, b));
        }

    #if !D_MATH_DBL_EVAL_WIDE
        D_TESTS_EXPECT( same,
                        "a reified derivative's value the derivative's, bit "
                        "for bit, at 50 points");
    #else
        static_cast<void>(same);
    #endif  // !D_MATH_DBL_EVAL_WIDE

        return;
    }

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // d_tests_cpp14
    //   constants.hpp's rational, from C++14: the exact fraction.
    void
    d_tests_cpp14()
    {
        const dm::expr_term third = dm::reify(dm::rational<2, 6>() * dv::x);
        const dm::expr_term whole = dm::reify(dm::rational<4, 2>() + dv::x);

        D_TESTS_EXPECT( (d_tests_text(third) == "(1/3) * x") &&
                        (d_tests_kind(dm::expr_operands_of(third)[0]) ==
                             D_MATH_VALUE_RATIONAL) &&
                        (d_tests_text(whole) == "2 + x"),
                        "a rational an exact fraction, an integer where its "
                        "denominator is 1");

        bool same = true;

        // its value, in double, the rational's
        for (int i = 0; i < 50; ++i)
        {
            const double a = -2.0 + (0.083 * i);

            same = same &&
                   d_tests_same_bits(d_tests_value(third, a, 0.0, 0.0),
                                     (dm::rational<2, 6>() * dv::x)(a));
        }

        D_TESTS_EXPECT( same,
                        "a rational's term's value the rational's, bit for "
                        "bit, at 50 points");

        return;
    }
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    D_TESTS_EXPECT( DJINTERP_MATH_EXPR_REIFY_HPP == 1,
                    "reify.hpp reads at every level, empty below C++11");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_leaves();
    d_tests_operators();
    d_tests_values();
    d_tests_derivatives();
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    d_tests_cpp14();
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

    std::printf("t_reify_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
