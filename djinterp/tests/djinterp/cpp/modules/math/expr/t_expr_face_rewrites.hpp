/*******************************************************************************
* djinterp [cpp][math]                                  t_expr_face_rewrites.hpp
*
* One part of t_expr_face.cpp, the expression language's face test (S438), its
* section 10: substitution and simplification. The test includes the parts in
* order, one program; each part includes the one before it.
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_face_rewrites.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_REWRITES_HPP
#define DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_REWRITES_HPP 1

#include "./t_expr_face_parsing.hpp"    // the part before


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


namespace
{

    //==========================================================================
    // 10. REWRITES
    //==========================================================================

    // d_tests_substitutes
    //   whether _text, with the variable _variable replaced by the term of
    // _by, is written _want.
    bool
    d_tests_substitutes(
        const char* _text,
        const char* _variable,
        const char* _by,
        const char* _want
    )
    {
        const m::expr_result<m::expr_term> term = m::expr_parse(_text);
        const m::expr_result<m::expr_term> by   = m::expr_parse(_by);

        return ( (term.is_ok()) &&
                 (by.is_ok()) &&
                 (m::expr_render(m::expr_substitute(term.value(),
                                                    _variable,
                                                    by.value())) == _want) );
    }

    // d_tests_simplifies
    //   whether _text, simplified, is written _want.
    bool
    d_tests_simplifies(
        const char* _text,
        const char* _want
    )
    {
        const m::expr_result<m::expr_term> term = m::expr_parse(_text);

        return ( (term.is_ok()) &&
                 (m::expr_render(m::expr_simplify(term.value())) == _want) );
    }

    // d_tests_finite
    //   whether a fold's answer is a finite number or a boolean.
    bool
    d_tests_finite(
        const m::expr_result<m::value>& _answer
    )
    {
        bool        truth = false;
        long double real  = 0.0L;

        // no value
        if (_answer.is_err())
        {
            return false;
        }

        return ( (_answer.value().to(truth)) ||
                 ( (_answer.value().is_number()) &&
                   (_answer.value().to(real)) &&
                   (std::isfinite(real)) ) );
    }

    // d_tests_parts_finite
    //   whether each part of _term has a finite value at _bindings, as a
    // value: a number or a boolean, no error, infinity or NaN.
    bool
    d_tests_parts_finite(
        const m::expr_term&     _term,
        const m::expr_bindings& _bindings
    )
    {
        // each operand's parts
        if (m::expr_is_application(_term))
        {
            const std::vector<m::expr_term> operands =
                m::expr_operands_of(_term);

            for (std::size_t i = 0; i < operands.size(); ++i)
            {
                if (!d_tests_parts_finite(operands[i], _bindings))
                {
                    return false;
                }
            }
        }

        return d_tests_finite(m::expr_evaluate(_term,
                                               _bindings,
                                               m::expr_context()));
    }

    // d_tests_kept
    //   whether a simplified term's value is _was, the term's, as
    // simplify.hpp's banner has it: the same boolean; the same number,
    // exactly, where _was is exact; and where it is not, a number within a
    // billionth of it, relatively.
    bool
    d_tests_kept(
        const m::value&                 _was,
        const m::expr_result<m::value>& _now
    )
    {
        bool        truth_was = false;
        bool        truth_now = false;
        long double was       = 0.0L;
        long double now       = 0.0L;

        // no value now
        if (_now.is_err())
        {
            return false;
        }

        if (_was.to(truth_was))
        {
            return ( (_now.value().to(truth_now)) &&
                     (truth_now == truth_was) );
        }

        if (_was.is_exact())
        {
            return m::internal::expr_same_number(_was, _now.value());
        }

        return ( (_was.to(was)) &&
                 (_now.value().is_number()) &&
                 (_now.value().to(now)) &&
                 (std::fabs(now - was) <=
                      1e-9L * std::fmax(1.0L, std::fabs(was))) );
    }

    // d_tests_random_bindings
    //   each name of the alphabets bound to an exact number n / d, n from
    // -4 to 4 and d from 1 to 3, drawn in turn.
    m::expr_bindings
    d_tests_random_bindings(
        d_tests_rng& _rng
    )
    {
        static const char* const names[] = { "a", "b", "x", "y_1", "mod2",
                                             "when" };
        m::expr_bindings         bindings;

        for (std::size_t i = 0; i < 6; ++i)
        {
            const dj_math_imax numerator   =
                static_cast<dj_math_imax>(_rng.below(9)) - 4;
            const dj_math_imax denominator =
                static_cast<dj_math_imax>(_rng.below(3)) + 1;

            bindings.at(names[i],
                        m::value::ratio(numerator, denominator));
        }

        return bindings;
    }

    // d_tests_arithmetic
    //   the alphabet of arithmetic: the round trip's literals and names,
    // + - * / mod ^ and the unary - and +, and the constants pi and e,
    // whose values are inexact.
    d_tests_alphabet
    d_tests_arithmetic()
    {
        static const char* const     literals[]  = { "0", "1", "2", "10",
                                                     "0.5", "1.25", "3e2",
                                                     "007", ".5", "5.",
                                                     "1e-3", "123456789" };
        static const char* const     names[]     = { "a", "b", "x", "y_1",
                                                     "mod2", "when" };
        static const m::expr_op_kind kinds[]     = {
            m::expr_op_kind::add, m::expr_op_kind::sub, m::expr_op_kind::mul,
            m::expr_op_kind::div, m::expr_op_kind::mod, m::expr_op_kind::pow,
            m::expr_op_kind::neg, m::expr_op_kind::pos };
        static const char* const     constants[] = { "pi", "e" };
        const d_tests_alphabet       alphabet    = { literals, 12,
                                                     names, 6,
                                                     kinds, 8,
                                                     constants, 2,
                                                     D_NULLPTR, 0 };

        return alphabet;
    }

    // d_tests_substitute
    //   expr_substitute: the variable replaced, every other node as it was;
    // a constant and a function's name no variable; the replacement put in
    // whole; terms made by default.
    void
    d_tests_substitute()
    {
        const m::expr_term sum   = d_tests_x_plus_1();
        const m::expr_term two   = m::expr_number(m::value(2), "2");
        const m::expr_term none;

        D_TESTS_EXPECT( (d_tests_same(m::expr_substitute(sum, "x", two),
                                      d_tests_apply(m::expr_op_kind::add,
                                                    two,
                                                    m::expr_number(
                                                        m::value(1),
                                                        "1")))),
                        "the variable replaced, every other node as it "
                        "was");
        D_TESTS_EXPECT( (d_tests_same(m::expr_substitute(sum, "y", two),
                                      sum)),
                        "a variable the term has not: the term as it was");
        D_TESTS_EXPECT( (d_tests_substitutes("x ^ 2 * sin(x)",
                                             "x",
                                             "t + 1",
                                             "(t + 1) ^ 2 * sin(t + 1)")),
                        "roadmap Phase 2's example: x ^ 2 * sin(x), x by "
                        "t + 1");
        D_TESTS_EXPECT( (d_tests_substitutes("x * x",
                                             "x",
                                             "x + 1",
                                             "(x + 1) * (x + 1)")),
                        "the replacement put in whole, and not searched "
                        "again");
        D_TESTS_EXPECT( (d_tests_substitutes("pi * f(x) + true",
                                             "pi",
                                             "3",
                                             "pi * f(x) + true")) &&
                        (d_tests_substitutes("pi * f(x) + true",
                                             "f",
                                             "3",
                                             "pi * f(x) + true")) &&
                        (d_tests_substitutes("pi * f(x) + true",
                                             "true",
                                             "3",
                                             "pi * f(x) + true")),
                        "a constant, and a function's name, no variable");
        D_TESTS_EXPECT( (d_tests_substitutes(
                             "piecewise(when(x < 0, -x), otherwise(x))",
                             "x",
                             "y - 1",
                             "piecewise(when(y - 1 < 0, -(y - 1)), "
                             "otherwise(y - 1))")),
                        "in a call's arguments, and in piecewise's "
                        "branches");
        D_TESTS_EXPECT( (m::expr_substitute(none, "x", two).empty()) &&
                        (m::expr_substitute(sum, "x", none).empty()) &&
                        (d_tests_same(m::expr_substitute(sum, "y", none),
                                      sum)) &&
                        (m::expr_render(m::expr_substitute(
                             d_tests_parsed("pi + 1"),
                             "pi",
                             none)) == "pi + 1"),
                        "terms made by default: none in none; none put in "
                        "for a variable the term has, none; for one it has "
                        "not, or a constant, the term");

        return;
    }

    // d_tests_substitute_random
    //   1,000 random terms of 2d's language (d_tests_language), x replaced
    // in each by a random term of arithmetic (d_tests_arithmetic): at a
    // random binding to exact numbers, each with x put in has the value the
    // term has with x bound to the replacement's value, or fails as it.
    void
    d_tests_substitute_random()
    {
        const d_tests_alphabet language   = d_tests_language();
        const d_tests_alphabet arithmetic = d_tests_arithmetic();
        d_tests_rng            rng(20261025u);
        std::size_t            same       = 0;

        for (unsigned i = 0; i < 1000; ++i)
        {
            const m::expr_term             term        =
                d_tests_random_term(rng,
                                    language,
                                    1 + (i % 5));
            const m::expr_term             replacement =
                d_tests_random_term(rng,
                                    arithmetic,
                                    i % 3);
            m::expr_bindings               at          =
                d_tests_random_bindings(rng);
            const m::expr_result<m::value> put         =
                m::expr_evaluate(replacement, at, m::expr_context());
            const m::expr_result<m::value> replaced    =
                m::expr_evaluate(m::expr_substitute(term, "x", replacement),
                                 at,
                                 m::expr_context());

            // x bound to the replacement's value, which it has
            if (put.is_err())
            {
                continue;
            }

            at.at("x", put.value());

            const m::expr_result<m::value> bound =
                m::expr_evaluate(term, at, m::expr_context());

            same += ( ( ( (replaced.is_err()) &&
                          (bound.is_err()) &&
                          (replaced.error() == bound.error()) ) ||
                        ( (replaced.is_ok()) &&
                          (bound.is_ok()) &&
                          (m::internal::expr_same_number(
                               replaced.value(),
                               bound.value())) ) ) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (same == 1000),
                        "1,000 random terms, x replaced: each with the "
                        "value it has with x bound to the replacement's");

        return;
    }

    // d_tests_simplify_constants
    //   the constants folded, exactly, booleans as true and false; where a
    // value is inexact, an error, or none, the node as it is.
    void
    d_tests_simplify_constants()
    {
        const m::expr_result<m::expr_term> sixth =
            m::expr_parse("1/3 + 1/6");
        const m::expr_result<m::expr_term> tenths =
            m::expr_parse("0.1 + 0.2");

        D_TESTS_EXPECT( (sixth.is_ok()) &&
                        (d_tests_same(m::expr_simplify(sixth.value()),
                                      m::expr_number(
                                          m::value::ratio(1, 2)))),
                        "1/3 + 1/6: the number 1/2, exact, with no "
                        "literal");
        D_TESTS_EXPECT( (d_tests_simplifies("sqrt(4)", "2")) &&
                        (d_tests_simplifies("sin(0)", "0")) &&
                        (d_tests_simplifies("4 ^ 0.5", "2")) &&
                        (d_tests_simplifies("7.5 mod 2", "(3/2)")) &&
                        (d_tests_simplifies("10!", "3628800")) &&
                        (d_tests_simplifies("-(3)", "(-3)")),
                        "calls, ^, mod, the factorial and -, folded where "
                        "exact");
        D_TESTS_EXPECT( (d_tests_same(m::expr_simplify(
                                          d_tests_parsed("1 < 2")),
                                      m::expr_constant("true"))) &&
                        (d_tests_simplifies("not true", "false")) &&
                        (d_tests_simplifies("true and false", "false")) &&
                        (d_tests_simplifies("1 = 1.0", "true")) &&
                        (d_tests_simplifies("all_of(true, 1 < 2)",
                                            "true")) &&
                        (d_tests_simplifies("piecewise(when(false, 1), "
                                            "otherwise(2))",
                                            "2")),
                        "relations, logic and piecewise, folded to true, "
                        "false or a number");
        D_TESTS_EXPECT( (d_tests_simplifies("sqrt(2)", "sqrt(2)")) &&
                        (d_tests_simplifies("25!", "25!")) &&
                        (d_tests_simplifies("2 ^ 100", "2 ^ 100")) &&
                        (d_tests_simplifies("2 * pi", "2 * pi")),
                        "an inexact value not folded: sqrt(2), 25!, 2 ^ 100, "
                        "2 * pi");
        D_TESTS_EXPECT( (d_tests_simplifies("pi < 4", "pi < 4")) &&
                        (d_tests_simplifies("1e400 = 1e400",
                                            "1e400 = 1e400")),
                        "no operand of a fold but an exact number, true or "
                        "false: pi < 4, and 1e400 = 1e400, as they are");
        D_TESTS_EXPECT( (d_tests_simplifies("1 / 0", "1 / 0")) &&
                        (d_tests_simplifies("log(0)", "log(0)")) &&
                        (d_tests_simplifies("true + 1", "true + 1")) &&
                        (d_tests_simplifies("f(1)", "f(1)")) &&
                        (d_tests_simplifies("atan2(1)", "atan2(1)")),
                        "an error's node, or a fold that fails, as it is");
        D_TESTS_EXPECT( (d_tests_simplifies("(1 + 2) * x", "3 * x")) &&
                        (d_tests_simplifies("abs(-3) + x", "3 + x")),
                        "a constant folded inside a term");
        D_TESTS_EXPECT( (tenths.is_ok()) &&
                        (d_tests_typed<double>("0.1 + 0.2",
                                               0.30000000000000004)) &&
                        (m::expr_evaluate<double>(
                             m::expr_simplify(tenths.value())).is_ok()) &&
                        (d_tests_same_real(
                             m::expr_evaluate<double>(
                                 m::expr_simplify(tenths.value())).value(),
                             0.3)),
                        "0.1 + 0.2 folded to 3/10: 0.3 in double, where the "
                        "doubles' sum is 0.30000000000000004");

        return;
    }

    // d_tests_simplify_powers
    //   x ^ 1, x ^ 0 and 1 ^ x; and the last two only where x is well
    // formed.
    void
    d_tests_simplify_powers()
    {
        D_TESTS_EXPECT( (d_tests_simplifies("x ^ 1", "x")) &&
                        (d_tests_simplifies("(x + y) ^ 1", "x + y")),
                        "x ^ 1 is x");
        D_TESTS_EXPECT( (d_tests_simplifies("x ^ 0", "1")) &&
                        (d_tests_simplifies("1 ^ x", "1")) &&
                        (d_tests_simplifies("sqrt(x) ^ 0", "1")),
                        "x ^ 0 and 1 ^ x are 1");
        D_TESTS_EXPECT( (d_tests_simplifies("f(x) ^ 0", "f(x) ^ 0")) &&
                        (d_tests_simplifies("1 ^ atan2(x)", "1 ^ atan2(x)")),
                        "x ^ 0 and 1 ^ x as they are where x is not well "
                        "formed");
        D_TESTS_EXPECT( (d_tests_simplifies("x ^ 2", "x ^ 2")) &&
                        (d_tests_simplifies("0 ^ x", "0 ^ x")),
                        "no other power changed");

        return;
    }

    // d_tests_simplify_sums
    //   a sum's terms collected: the identities of + and -, like terms, the
    // exact numbers, terms of coefficient 0, in the order each first
    // stands.
    void
    d_tests_simplify_sums()
    {
        D_TESTS_EXPECT( (d_tests_simplifies("x + 0", "x")) &&
                        (d_tests_simplifies("0 + x", "x")) &&
                        (d_tests_simplifies("x - 0", "x")) &&
                        (d_tests_simplifies("0 - x", "-x")) &&
                        (d_tests_simplifies("-(-x)", "x")) &&
                        (d_tests_simplifies("+x", "x")),
                        "x + 0, 0 + x and x - 0 are x, 0 - x is -x, -(-x) "
                        "and +x are x");
        D_TESTS_EXPECT( (d_tests_simplifies("x + x", "2 * x")) &&
                        (d_tests_simplifies("2 * x + 3 * x", "5 * x")) &&
                        (d_tests_simplifies("x * 2 + x", "3 * x")) &&
                        (d_tests_simplifies("3 * x - 2 * x", "x")) &&
                        (d_tests_simplifies("x - 3 * x", "(-2) * x")),
                        "like terms collected, their coefficients summed");
        D_TESTS_EXPECT( (d_tests_simplifies("x * y + y * x",
                                            "2 * x * y")) &&
                        (d_tests_simplifies("(x + 1) * 2 + (x + 1) * 3",
                                            "5 * (x + 1)")) &&
                        (d_tests_simplifies("x * 0.5 + x * 0.25",
                                            "(3/4) * x")) &&
                        (d_tests_simplifies("x / 2 + x / 2", "x")) &&
                        (d_tests_simplifies("2 / y + 1 / y", "3 / y")) &&
                        (d_tests_simplifies("x / y + x / y",
                                            "2 * x / y")) &&
                        (d_tests_simplifies("x / 0 + x / 0",
                                            "2 * x / 0")) &&
                        (d_tests_simplifies("1 / y + 0", "1 / y")),
                        "factors in any order, sums for factors, rational "
                        "coefficients, and division");
        D_TESTS_EXPECT( (d_tests_simplifies("x * y + x / y",
                                            "x * y + x / y")) &&
                        (d_tests_simplifies("x + x * y", "x + x * y")),
                        "terms alike only in all their factors, each "
                        "dividing as it does");
        D_TESTS_EXPECT( (d_tests_simplifies("x + 1 + x", "2 * x + 1")) &&
                        (d_tests_simplifies("1 + x + 2", "3 + x")) &&
                        (d_tests_simplifies("1 + 2 + 3", "6")) &&
                        (d_tests_simplifies("x - (y - x)", "2 * x - y")) &&
                        (d_tests_simplifies("-(x + y) + x", "-y")) &&
                        (d_tests_simplifies("x + -y", "x - y")),
                        "the exact numbers summed into one; in the order "
                        "each first stands; through -, brackets and the "
                        "unary -");
        D_TESTS_EXPECT( (d_tests_simplifies("x - x", "0")) &&
                        (d_tests_simplifies("x + y - x - y", "0")) &&
                        (d_tests_simplifies("sin(x) - sin(x)", "0")) &&
                        (d_tests_simplifies("pi - pi", "0")),
                        "a term of coefficient 0 dropped");
        D_TESTS_EXPECT( (d_tests_simplifies("f(x) - f(x)", "0 * f(x)")) &&
                        (d_tests_simplifies("atan2(x) + 1 - atan2(x)",
                                            "0 * atan2(x) + 1")),
                        "a term of coefficient 0 kept where it is not well "
                        "formed");
        D_TESTS_EXPECT( (d_tests_simplifies("1 + x", "1 + x")) &&
                        (d_tests_simplifies("a - b + c", "a - b + c")) &&
                        (d_tests_simplifies("a - (b - c)", "a - (b - c)")),
                        "a sum with nothing to collect as it was");
        D_TESTS_EXPECT( (d_tests_simplifies(
                             "4611686018427387904 * x * 4 + y",
                             "4611686018427387904 * x * 4 + y")) &&
                        (d_tests_simplifies(
                             "4611686018427387904 * x + "
                             "4611686018427387904 * x",
                             "4611686018427387904 * x + "
                             "4611686018427387904 * x")),
                        "a sum as it was where a term's coefficient, or a "
                        "sum of them, passes the family's integers");

        return;
    }

    // d_tests_simplify_products
    //   a product's exact numbers multiplied into one coefficient, first;
    // 1 * x, x / 1 and 0 * x.
    void
    d_tests_simplify_products()
    {
        D_TESTS_EXPECT( (d_tests_simplifies("2 * x * 3", "6 * x")) &&
                        (d_tests_simplifies("x / 2 * 4", "2 * x")) &&
                        (d_tests_simplifies("2 * x / 4", "(1/2) * x")) &&
                        (d_tests_simplifies("2 * -x", "(-2) * x")) &&
                        (d_tests_simplifies("-(2 * x)", "(-2) * x")),
                        "the exact numbers multiplied into one coefficient, "
                        "first, and the signs");
        D_TESTS_EXPECT( (d_tests_simplifies("1 * x", "x")) &&
                        (d_tests_simplifies("x * 1", "x")) &&
                        (d_tests_simplifies("x / 1", "x")),
                        "1 * x, x * 1 and x / 1 are x");
        D_TESTS_EXPECT( (d_tests_simplifies("0 * x", "0")) &&
                        (d_tests_simplifies("x * 0", "0")) &&
                        (d_tests_simplifies("0 / x", "0")) &&
                        (d_tests_simplifies("2 * x * 0 * sin(y)", "0")),
                        "a product of an exact 0 is 0");
        D_TESTS_EXPECT( (d_tests_simplifies("0 * f(x)", "0 * f(x)")) &&
                        (d_tests_simplifies("x / 0", "x / 0")),
                        "0 * f(x) kept, f not well formed; x / 0 as it is");
        D_TESTS_EXPECT( (d_tests_simplifies("x * 2", "x * 2")) &&
                        (d_tests_simplifies("x * (2 * y)", "x * (2 * y)")) &&
                        (d_tests_simplifies("2 ^ 62 * x * 4",
                                            "4611686018427387904 * x * 4")),
                        "a product as it was where the collected one is no "
                        "smaller, or its coefficient passes the family's "
                        "integers");
        D_TESTS_EXPECT( (d_tests_simplifies("2 * x ^ (2 - 1) * 1",
                                            "2 * x")) &&
                        (d_tests_simplifies("2 * (x + x)", "4 * x")),
                        "the rules together: a derivative's shape, and a "
                        "sum inside a product");
        D_TESTS_EXPECT( (d_tests_simplifies("-1 * (x * 2 - (5 - y))",
                                            "(-2) * x + 5 - y")),
                        "a sum a product's collection makes, collected in "
                        "the next pass");

        return;
    }

    // d_tests_simplify_values
    //   what the value kept means, by examples: where each part is finite,
    // the same value; where a part is an error or infinite, another; a term
    // made by default; and the handle.
    void
    d_tests_simplify_values()
    {
        long double infinity = 0.0L;

        D_TESTS_EXPECT( (d_tests_simplifies("0 * (1 / x)", "0")) &&
                        (d_tests_valued("0 * (1 / x)",
                                        m::value(0),
                                        m::at("x", 2))) &&
                        (d_tests_value_of("0 * (1 / x)",
                                          m::at("x", 0)).kind() ==
                             D_MATH_VALUE_ERROR),
                        "0 * (1 / x) is 0: its value where x is not 0, and "
                        "at x = 0, where it was a pole error, 0");
        D_TESTS_EXPECT( (d_tests_simplifies("1 / (pi - pi)", "1 / 0")) &&
                        (d_tests_value_of("1 / (pi - pi)").to(infinity)) &&
                        (std::isinf(infinity)) &&
                        (d_tests_value_of("1 / 0").kind() ==
                             D_MATH_VALUE_ERROR),
                        "1 / (pi - pi) is 1 / 0, pi - pi the exact 0: a "
                        "pole error, where it was an infinity");
        D_TESTS_EXPECT( (m::expr_simplify(m::expr_term()).empty()),
                        "a term made by default: one made by default");
        D_TESTS_EXPECT( (m::parse("x + x").simplify().str() == "2 * x") &&
                        (m::parse("x ^ 2 * sin(x)").subs(
                             "x",
                             m::parse("t + 1")).simplify().str() ==
                             "(t + 1) ^ 2 * sin(t + 1)"),
                        "the handle's simplify and subs, its entries'");

        return;
    }

    // d_tests_simplify_random
    //   random terms of 2d's language and of arithmetic, each simplified:
    // no larger; a fixed point; its text, read back and simplified,
    // written the same; and at three random bindings to exact numbers,
    // where each part of the term has a finite value, the value it had.
    void
    d_tests_simplify_random()
    {
        const d_tests_alphabet alphabets[2] = { d_tests_language(),
                                                d_tests_arithmetic() };
        d_tests_rng            rng(20261024u);
        std::size_t            smaller      = 0;
        std::size_t            fixed        = 0;
        std::size_t            read         = 0;
        std::size_t            compared     = 0;
        std::size_t            kept         = 0;
        std::size_t            exact        = 0;
        std::size_t            inexact      = 0;
        std::size_t            truths       = 0;

        // 1,000 terms of each alphabet, up to five operators deep
        for (unsigned i = 0; i < 2000; ++i)
        {
            const m::expr_term                 term       =
                d_tests_random_term(rng,
                                    alphabets[i % 2],
                                    1 + ((i / 2) % 5));
            const m::expr_term                 simplified =
                m::expr_simplify(term);
            const m::expr_result<m::expr_term> back       =
                m::expr_parse(m::expr_render(simplified));

            smaller += ( (dj::size<m::expr_op, m::expr_atom>(simplified) <=
                          dj::size<m::expr_op, m::expr_atom>(term)) ? 1 : 0 );
            fixed   += ( (d_tests_same(m::expr_simplify(simplified),
                                       simplified)) ? 1 : 0 );
            read    += ( ( (back.is_ok()) &&
                           (m::expr_render(m::expr_simplify(back.value())) ==
                                m::expr_render(simplified)) ) ? 1 : 0 );

            // three bindings, each where every part is finite
            for (unsigned j = 0; j < 3; ++j)
            {
                const m::expr_bindings at = d_tests_random_bindings(rng);
                bool                   truth = false;

                if (!d_tests_parts_finite(term, at))
                {
                    continue;
                }

                const m::value was = m::expr_evaluate(term,
                                                      at,
                                                      m::expr_context())
                                         .value();

                ++compared;
                kept    += ( (d_tests_kept(was,
                                           m::expr_evaluate(
                                               simplified,
                                               at,
                                               m::expr_context()))) ? 1 : 0 );
                truths  += ( (was.to(truth)) ? 1 : 0 );
                exact   += ( (was.is_exact()) ? 1 : 0 );
                inexact += ( ( (!was.to(truth)) &&
                               (!was.is_exact()) ) ? 1 : 0 );
            }
        }

        D_TESTS_EXPECT( (smaller == 2000) &&
                        (fixed == 2000),
                        "2,000 random terms simplified: none larger, each a "
                        "fixed point");
        D_TESTS_EXPECT( (read == 2000),
                        "each simplified term's text, read back and "
                        "simplified, written the same");
        D_TESTS_EXPECT( (compared > 0) &&
                        (kept == compared),
                        "at random exact bindings where each part of the "
                        "term is finite, each simplified term's value the "
                        "term's");
        D_TESTS_EXPECT( (exact > 0) &&
                        (inexact > 0) &&
                        (truths > 0),
                        "among the values compared, exact ones, inexact "
                        "ones and booleans");

        return;
    }

    // d_tests_rewrites
    //   the derivative's stubs.
    void
    d_tests_rewrites()
    {
        const m::expr_term sum = d_tests_x_plus_1();

        D_TESTS_EXPECT( d_tests_not_yet(m::expr_derivative(sum, "x"),
                                        "expr_derivative: not yet (step 25, "
                                        "4a)"),
                        "expr_derivative: not yet");
        D_TESTS_EXPECT( d_tests_not_yet(m::expr_derivative(sum, "x", 2),
                                        "expr_derivative: not yet (step 25, "
                                        "4a)"),
                        "expr_derivative of order 2: not yet");
        D_TESTS_EXPECT( d_tests_not_yet(m::expr_gradient(
                                            sum,
                                            std::vector<std::string>{ "x",
                                                                      "y" }),
                                        "expr_gradient: not yet (step 25, "
                                        "4a)"),
                        "expr_gradient: not yet");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_REWRITES_HPP
