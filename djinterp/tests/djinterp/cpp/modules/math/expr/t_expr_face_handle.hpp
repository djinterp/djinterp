/*******************************************************************************
* djinterp [cpp][math]                                    t_expr_face_handle.hpp
*
* One part of t_expr_face.cpp, the expression language's face test (S438), its
* sections 12 to 14: calculus, the handle and the entries' types. The test
* includes the parts in order, one program; each part includes the one before
* it.
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_face_handle.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_HANDLE_HPP
#define DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_HANDLE_HPP 1

#include "./t_expr_face_algebra.hpp"    // the part before


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


namespace
{

    //==========================================================================
    // 12. CALCULUS
    //==========================================================================

    // d_tests_numeric
    //   the stubs of Phase 4's numeric entries, in one type.
    template<typename Type>
    bool
    d_tests_numeric(
        const m::expr_term& _term
    )
    {
        const m::expr_bindings y = m::at("y", 1);

        return ( (d_tests_not_yet(m::expr_integrate<Type>(_term,
                                                          "x",
                                                          Type(0),
                                                          Type(1)),
                                  "expr_integrate<Type>: not yet (step 26, "
                                  "4b)"))                                  &&
                 (d_tests_not_yet(m::expr_integrate<Type>(
                                      _term,
                                      "x",
                                      Type(0),
                                      Type(1),
                                      y,
                                      m::expr_quadrature::simpson),
                                  "expr_integrate<Type>: not yet (step 26, "
                                  "4b)"))                                  &&
                 (d_tests_not_yet(m::expr_derivative_at<Type>(_term,
                                                              "x",
                                                              Type(2)),
                                  "expr_derivative_at<Type>: not yet (step "
                                  "26, 4b)"))                              &&
                 (d_tests_not_yet(m::expr_sum<Type>(_term, "x", 1, 10, y),
                                  "expr_sum<Type>: not yet (step 27, 4c)")) &&
                 (d_tests_not_yet(m::expr_product<Type>(_term, "x", 1, 10),
                                  "expr_product<Type>: not yet (step 27, "
                                  "4c)"))                                  &&
                 (d_tests_not_yet(m::expr_limit<Type>(_term, "x", Type(0)),
                                  "expr_limit<Type>: not yet (step 27, "
                                  "4c)")) );
    }

    // d_tests_units
    //   im, jm and km (S271, S275): from step 22 (3d) names of the constant
    // table, each read as a constant and never a variable; im, from step 23
    // (3e), the exact imaginary unit as a value and a NaN in each type, so
    // that 3 + 4im, by implied multiplication, is 3 + 4im; jm and km, from
    // step 24 (3f), the exact units of the quaternions as values, so that
    // jm km is im and km jm is -im (S277), and NaNs in each type; a binding
    // of the name no variable's; and the handle's.
    void
    d_tests_units()
    {
        const m::value         i  = m::expr_complex(m::value(0), m::value(1));
        const m::value         j  = m::expr_quaternion(m::value(0),
                                                       m::value(0),
                                                       m::value(1),
                                                       m::value(0));
        const m::value         k2 = m::expr_quaternion(m::value(0),
                                                       m::value(0),
                                                       m::value(0),
                                                       m::value(2));
        const m::expr_bindings at = m::at("im", 2.0);
        m::expr_syntax         implied;

        implied.implied_multiplication = true;

        const m::expr_term four_im = d_tests_parsed("3 + 4im", implied);

        D_TESTS_EXPECT( (d_tests_parses("im", m::expr_constant("im"))) &&
                        (d_tests_parses("jm", m::expr_constant("jm"))) &&
                        (d_tests_parses("km", m::expr_constant("km"))) &&
                        (d_tests_parses("ima", m::expr_variable("ima"))) &&
                        (d_tests_renders("3 + 4im", "3 + 4 * im", implied)),
                        "im, jm and km: constants of the table, never "
                        "variables");
        const m::expr_result<m::value> as_im =
            m::expr_evaluate(d_tests_parsed("im"), at, m::expr_context());
        const m::expr_result<m::value> as_sum =
            m::expr_evaluate(four_im, at, m::expr_context());
        const m::expr_result<float>    im_float =
            m::expr_evaluate<float>(d_tests_parsed("im"));
        const m::expr_result<double>   sum_double =
            m::expr_evaluate<double>(four_im, at);

        const m::expr_result<m::value> as_jm =
            m::expr_evaluate(d_tests_parsed("jm"),
                             m::expr_bindings(),
                             m::expr_context());
        const m::expr_result<m::value> as_jk =
            m::expr_evaluate(d_tests_parsed("jm * km"),
                             m::expr_bindings(),
                             m::expr_context());
        const m::expr_result<m::value> as_kj =
            m::expr_evaluate(d_tests_parsed("km * jm"),
                             m::expr_bindings(),
                             m::expr_context());
        const m::expr_result<long double> jk_long =
            m::expr_evaluate<long double>(d_tests_parsed("jm * km"));
        const m::expr_result<double>      km_double =
            m::expr_evaluate<double>(d_tests_parsed("km"));
        const m::expr_result<double>      km_handle =
            m::parse("2 * km").eval<double>();

        D_TESTS_EXPECT( (m::expr_parse("3 + 4im", implied).is_ok()) &&
                        (as_im.is_ok()) &&
                        (as_im.value() == i) &&
                        (as_im.value().is_exact()) &&
                        (as_sum.is_ok()) &&
                        (as_sum.value() ==
                             m::expr_complex(m::value(3), m::value(4))) &&
                        (as_jm.is_ok()) &&
                        (as_jm.value() == j) &&
                        (as_jm.value().is_exact()) &&
                        (as_jk.is_ok()) &&
                        (m::expr_is_complex(as_jk.value())) &&
                        (as_jk.value() == i) &&
                        (as_kj.is_ok()) &&
                        (as_kj.value() ==
                             m::expr_complex(m::value(0), m::value(-1))),
                        "as a value: im the unit, bound or not, and 3 + 4im; "
                        "jm the exact unit; jm km im, a complex number, and "
                        "km jm -im");
        D_TESTS_EXPECT( (im_float.is_ok()) &&
                        (im_float.value() != im_float.value()) &&
                        (sum_double.is_ok()) &&
                        (sum_double.value() != sum_double.value()) &&
                        (jk_long.is_ok()) &&
                        (jk_long.value() != jk_long.value()) &&
                        (km_double.is_ok()) &&
                        (km_double.value() != km_double.value()),
                        "in float, double and long double: im, jm and km "
                        "NaNs");
        D_TESTS_EXPECT( (m::parse("im").eval().is_ok()) &&
                        (m::parse("im").eval().value() == i) &&
                        (m::parse("2 * km").eval().is_ok()) &&
                        (m::parse("2 * km").eval().value() == k2) &&
                        (km_handle.is_ok()) &&
                        (km_handle.value() != km_handle.value()),
                        "the handle: im the unit, 2 km the exact quaternion, "
                        "a NaN in double");

        return;
    }

    // d_tests_calculus
    //   the stubs of Phase 4.
    void
    d_tests_calculus()
    {
        const m::expr_term sum = d_tests_x_plus_1();

        D_TESTS_EXPECT( (static_cast<int>(m::expr_quadrature::trapezoid) ==
                             1) &&
                        (static_cast<int>(m::expr_quadrature::romberg) == 4),
                        "the quadrature rules numbered 1 to 4");
        D_TESTS_EXPECT( d_tests_numeric<float>(sum),
                        "the numeric entries in float: not yet");
        D_TESTS_EXPECT( d_tests_numeric<double>(sum),
                        "the numeric entries in double: not yet");
        D_TESTS_EXPECT( d_tests_numeric<long double>(sum),
                        "the numeric entries in long double: not yet");
        D_TESTS_EXPECT( d_tests_not_yet(m::expr_taylor(sum,
                                                       "x",
                                                       m::expr_number(
                                                           m::value(0)),
                                                       4),
                                        "expr_taylor: not yet (step 27, "
                                        "4c)"),
                        "expr_taylor: not yet");
        D_TESTS_EXPECT( d_tests_not_yet(m::expr_antiderivative(sum, "x"),
                                        "expr_antiderivative: not yet (step "
                                        "28, 4d)"),
                        "expr_antiderivative: not yet");

        return;
    }


    //==========================================================================
    // 13. THE HANDLE
    //==========================================================================

    // d_tests_handle_in
    //   whether a handle's value in Type, at _bindings, is _want.
    template<typename Type>
    bool
    d_tests_handle_in(
        const m::expr&          _handle,
        const m::expr_bindings& _bindings,
        Type                    _want
    )
    {
        const m::expr_result<Type> got = _handle.eval<Type>(_bindings);

        return ( (got.is_ok()) &&
                 (d_tests_same_real(got.value(), _want)) );
    }

    // d_tests_is_value
    //   whether a value a step gave is _want: of its kind, and the same
    // number (expr_same_number).
    bool
    d_tests_is_value(
        const m::expr_result<m::value>& _got,
        const m::value&                 _want
    )
    {
        return ( (_got.is_ok()) &&
                 (m::internal::expr_same_number(_got.value(), _want)) );
    }

    // d_tests_agree
    //   whether two answers of a fold are the same: equal errors, or the
    // same number, once stored in a type, or by expr_same_number as values.
    template<typename Type>
    bool
    d_tests_agree(
        const m::expr_result<Type>& _a,
        const m::expr_result<Type>& _b
    )
    {
        // one an error and the other not
        if (_a.is_ok() != _b.is_ok())
        {
            return false;
        }

        if (_a.is_err())
        {
            return (_a.error() == _b.error());
        }

        return d_tests_same_real(_a.value(), _b.value());
    }

    bool
    d_tests_agree(
        const m::expr_result<m::value>& _a,
        const m::expr_result<m::value>& _b
    )
    {
        // one an error and the other not
        if (_a.is_ok() != _b.is_ok())
        {
            return false;
        }

        if (_a.is_err())
        {
            return (_a.error() == _b.error());
        }

        return m::internal::expr_same_number(_a.value(), _b.value());
    }

    // d_tests_handle_context
    //   math::expr's eval<Type> under a context, which each evaluator takes
    // (S340).
    void
    d_tests_handle_context()
    {
        typedef m::complex<double> cd;

        const m::expr_context    real(true);
        const m::expr_result<cd> strays =
            m::parse("sqrt(-4)").eval<cd>(m::expr_bindings(), real);
        const m::expr_result<cd> by_default =
            m::parse("sqrt(-4)").eval<cd>();

        D_TESTS_EXPECT( (strays.is_ok()) &&
                        (strays.value().real() != strays.value().real()) &&
                        (strays.value().imag() == 0.0) &&
                        (by_default.is_ok()) &&
                        (by_default.value() == cd(0.0, 2.0)),
                        "eval<Type> under a context: sqrt(-4) a real NaN in "
                        "math::complex<double> where real results are "
                        "asked, 2i by default (S340)");

        return;
    }

    // d_tests_handle
    //   math::expr: what it holds; str(), and eval in each type and as a
    // value, each its entry's, and each passing on the error held.
    void
    d_tests_handle()
    {
        const m::expr_term     sum     = d_tests_x_plus_1();
        const m::expr          held(sum);
        const m::expr_error    bracket(m::expr_status::unbalanced,
                                       3,
                                       "a bracket");
        const m::expr          failed(bracket);
        const m::expr          none    = m::expr(m::expr_term());
        const m::expr_bindings two     = m::at("x", 2);
        const m::expr_bindings quarter = m::at("x", 0.25);
        const m::expr_bindings third   = m::at("x", m::value::ratio(1, 3));

        // what it holds
        D_TESTS_EXPECT( (held.ok()) &&
                        (d_tests_same(held.term(), sum)),
                        "a handle of a term holds it");
        D_TESTS_EXPECT( (!failed.ok()) &&
                        (failed.error() == bracket),
                        "a handle of an error holds it");

        // str: the term rendered, or the error's message
        D_TESTS_EXPECT( (held.str() == "x + 1") &&
                        (held.str() == m::expr_render(sum)),
                        "str: the term rendered");
        D_TESTS_EXPECT( (failed.str() == "a bracket"),
                        "str: the error's message");
        D_TESTS_EXPECT( (none.ok()) &&
                        (none.str().empty()),
                        "str: a term made by default the empty string, as "
                        "expr_render gives it");

        // eval in each type
        D_TESTS_EXPECT( (d_tests_handle_in<float>(held, two, 3.0f)) &&
                        (d_tests_handle_in<double>(held, quarter, 1.25)) &&
                        (d_tests_handle_in<long double>(held, two, 3.0L)),
                        "eval<Type>: the term's value in each type");
        D_TESTS_EXPECT( (d_tests_fails_with(held.eval<double>(),
                                            m::expr_evaluate<double>(sum)
                                                .error())) &&
                        (held.eval<double>().error().status ==
                             m::expr_status::unbound_variable),
                        "eval<Type>: no binding by default, and a variable "
                        "unbound the fold's error");
        D_TESTS_EXPECT( (d_tests_fails_with(failed.eval<float>(two),
                                            bracket)) &&
                        (d_tests_fails_with(failed.eval<double>(two),
                                            bracket)) &&
                        (d_tests_fails_with(failed.eval<long double>(two),
                                            bracket)),
                        "eval<Type>: the error held, in each type");
        D_TESTS_EXPECT( (d_tests_fails_with(none.eval<double>(),
                                            m::expr_evaluate<double>(
                                                m::expr_term()).error())) &&
                        (none.eval<double>().error().status ==
                             m::expr_status::missing_operand),
                        "eval<Type>: a term made by default, the fold's "
                        "error");

        // eval<Type> under a context (S340)
        d_tests_handle_context();

        // eval as a value
        D_TESTS_EXPECT( d_tests_is_value(held.eval(third),
                                         m::value::ratio(4, 3)),
                        "eval: the term's value, exactly");
        D_TESTS_EXPECT( d_tests_is_value(held.eval(two, m::expr_context()),
                                         m::value(3)),
                        "eval: under the context given");
        D_TESTS_EXPECT( (d_tests_fails_with(held.eval(),
                                            m::expr_evaluate(
                                                sum,
                                                m::expr_bindings(),
                                                m::expr_context())
                                                .error())) &&
                        (held.eval().error().status ==
                             m::expr_status::unbound_variable),
                        "eval: no binding by default, and a variable "
                        "unbound the fold's error");
        D_TESTS_EXPECT( (d_tests_fails_with(failed.eval(), bracket)) &&
                        (d_tests_fails_with(failed.eval(two,
                                                        m::expr_context()),
                                            bracket)),
                        "eval: the error held");
        D_TESTS_EXPECT( (none.eval().is_err()) &&
                        (none.eval().error().status ==
                             m::expr_status::missing_operand),
                        "eval: a term made by default, the fold's error");

        return;
    }

    // d_tests_handle_rewrites
    //   subs, simplify and diff: each a handle of its entry's answer, and
    // each passing on the error held; subs the replacement's too.
    void
    d_tests_handle_rewrites()
    {
        const m::expr_term  sum = d_tests_x_plus_1();
        const m::expr       held(sum);
        const m::expr       two(m::expr_number(m::value(2), "2"));
        const m::expr_error bracket(m::expr_status::unbalanced,
                                    3,
                                    "a bracket");
        const m::expr_error character(m::expr_status::bad_character,
                                      0,
                                      "a character");
        const m::expr       failed(bracket);
        const m::expr       other(character);

        // subs
        D_TESTS_EXPECT( (held.subs("x", two).ok()) &&
                        (d_tests_same(held.subs("x", two).term(),
                                      m::expr_substitute(sum,
                                                         "x",
                                                         two.term()))),
                        "subs: a handle of expr_substitute's term");
        D_TESTS_EXPECT( (d_tests_same(held.subs("x", two).term(),
                                      d_tests_apply(m::expr_op_kind::add,
                                                    two.term(),
                                                    m::expr_number(
                                                        m::value(1),
                                                        "1")))),
                        "subs: x replaced by 2, x + 1 being 2 + 1");
        D_TESTS_EXPECT( d_tests_holds(failed.subs("x", two), bracket),
                        "subs: the error held");
        D_TESTS_EXPECT( (d_tests_holds(held.subs("x", other), character)) &&
                        (d_tests_holds(held.subs("y", other), character)),
                        "subs: the replacement's error, whether its "
                        "variable occurs or not");
        D_TESTS_EXPECT( d_tests_holds(failed.subs("x", other), bracket),
                        "subs: of two errors, the handle's own");

        // simplify
        D_TESTS_EXPECT( (held.simplify().ok()) &&
                        (d_tests_same(held.simplify().term(),
                                      m::expr_simplify(sum))) &&
                        (d_tests_same(held.simplify().term(), sum)) &&
                        (m::expr(d_tests_apply(m::expr_op_kind::add,
                                               m::expr_variable("x"),
                                               m::expr_variable("x")))
                             .simplify().str() == "2 * x"),
                        "simplify: a handle of expr_simplify's term, x + 1 "
                        "as it was, and x + x 2 * x");
        D_TESTS_EXPECT( d_tests_holds(failed.simplify(), bracket),
                        "simplify: the error held");

        // diff
        D_TESTS_EXPECT( (d_tests_holds(held.diff("x"),
                                       m::expr_derivative(sum, "x")
                                           .error())) &&
                        (d_tests_holds_not_yet(held.diff("x"),
                                               "expr_derivative: not yet "
                                               "(step 25, 4a)")),
                        "diff: a handle of expr_derivative's error, not_yet "
                        "until step 25");
        D_TESTS_EXPECT( d_tests_holds_not_yet(held.diff("x", 3),
                                              "expr_derivative: not yet "
                                              "(step 25, 4a)"),
                        "diff of order 3: the same");
        D_TESTS_EXPECT( (d_tests_holds(failed.diff("x"), bracket)) &&
                        (d_tests_holds(failed.diff("x", 3), bracket)),
                        "diff: the error held, not the derivative's");

        return;
    }

    // d_tests_handle_entries
    //   parse and try_parse: a handle of expr_parse's term or error, by the
    // syntax given.
    void
    d_tests_handle_entries()
    {
        const m::expr_result<m::expr_term> read  =
            m::expr_parse("x^2 * sin(x)");
        const m::expr_result<m::expr_term> wrong = m::expr_parse("1 + (2");
        const m::expr                      f     = m::parse("x^2 * sin(x)");
        m::expr_syntax                     implied;
        m::expr_syntax                     bars;

        implied.implied_multiplication = true;
        bars.absolute_bars             = true;

        // parse
        D_TESTS_EXPECT( (f.ok()) &&
                        (d_tests_same(f.term(), read.value())),
                        "parse: a handle of expr_parse's term");
        D_TESTS_EXPECT( (f.str() == "x ^ 2 * sin(x)"),
                        "parse, then str: the text as expr_render writes "
                        "it");
        D_TESTS_EXPECT( (d_tests_holds(m::parse("1 + (2"), wrong.error())) &&
                        (m::parse("1 + (2").error() ==
                             m::expr_error(m::expr_status::unbalanced,
                                           4,
                                           "'(' is never closed")),
                        "parse: a handle of expr_parse's error, at its "
                        "place");
        D_TESTS_EXPECT( d_tests_holds(m::parse(""),
                                      m::expr_parse("").error()),
                        "parse: of no text, expr_parse's error");
        D_TESTS_EXPECT( (m::parse("2x", implied).str() == "2 * x") &&
                        (d_tests_holds(m::parse("2x"),
                                       m::expr_parse("2x").error())),
                        "parse: implied multiplication by the syntax "
                        "given, off by default");
        D_TESTS_EXPECT( (m::parse("|x - 3|", bars).str() == "abs(x - 3)") &&
                        (!m::parse("|x - 3|").ok()),
                        "parse: bars by the syntax given, off by default");

        // try_parse
        const m::expr_result<m::expr> tried = m::try_parse("x^2 * sin(x)");

        D_TESTS_EXPECT( (tried.is_ok()) &&
                        (tried.value().ok()) &&
                        (d_tests_same(tried.value().term(), read.value())),
                        "try_parse: parse's handle, holding expr_parse's "
                        "term");
        D_TESTS_EXPECT( (d_tests_fails_with(m::try_parse("1 + (2"),
                                            wrong.error())) &&
                        (d_tests_fails_with(m::try_parse(""),
                                            m::expr_parse("").error())),
                        "try_parse: expr_parse's error");
        D_TESTS_EXPECT( (m::try_parse("2x", implied).is_ok()) &&
                        (m::try_parse("2x", implied).value().str() ==
                             "2 * x") &&
                        (m::try_parse("2x").is_err()),
                        "try_parse: by the syntax given");

        return;
    }

    // d_tests_handle_chains
    //   members chained, roadmap Phase 2's examples among them, each as it
    // answers until steps 18 and 22; and an error passed through every
    // member to the chain's end.
    void
    d_tests_handle_chains()
    {
        const m::expr          f       = m::parse("x^2 * sin(x)");
        const m::expr          broken  = m::parse("1 + (2");
        const m::expr_error    bracket = broken.error();
        const m::expr_bindings at      = m::at("x", 2.0);
        const char* const      later   = "expr_derivative: not yet (step 25, "
                                         "4a)";

        // roadmap Phase 2's examples
        D_TESTS_EXPECT( (f.diff("x").simplify().str() == later),
                        "f.diff(x).simplify().str(): the derivative's "
                        "message, until step 25");
        D_TESTS_EXPECT( d_tests_not_yet(f.diff("x").simplify()
                                            .eval<double>(at),
                                        later),
                        "f.diff(x).simplify().eval<double>(at x = 2): the "
                        "derivative's error, until step 25");
        D_TESTS_EXPECT( d_tests_is_value(m::parse("1/3 + 1/6").eval(),
                                         m::value::ratio(1, 2)),
                        "parse(1/3 + 1/6).eval(): exactly 1/2");
        D_TESTS_EXPECT( (f.subs("x", m::parse("t + 1")).ok()) &&
                        (f.subs("x", m::parse("t + 1")).str() ==
                             "(t + 1) ^ 2 * sin(t + 1)"),
                        "f.subs(x, parse(t + 1)): (t + 1) ^ 2 * "
                        "sin(t + 1)");

        const auto diff_x = [](const m::expr& _e) { return _e.diff("x"); };
        const m::expr_result<m::expr> mapped =
            m::try_parse("x^2 * sin(x)").map(diff_x);

        D_TESTS_EXPECT( (mapped.is_ok()) &&
                        (d_tests_holds_not_yet(mapped.value(), later)),
                        "try_parse(text).map(diff): the handle of the "
                        "derivative's error, until step 25");
        D_TESTS_EXPECT( d_tests_fails_with(m::try_parse("1 + (2")
                                               .map(diff_x),
                                           bracket),
                        "try_parse(text).map(diff) of a text in error: the "
                        "parse's error");

        // a chain that computes
        D_TESTS_EXPECT( d_tests_agree(f.simplify().eval<double>(at),
                                      m::expr_evaluate<double>(f.term(),
                                                               at)),
                        "f.simplify().eval<double>(at x = 2): the fold's "
                        "value");

        // an error through every member
        D_TESTS_EXPECT( (broken.simplify().subs("x", f).diff("x")
                             .diff("x", 2).str() == "'(' is never closed"),
                        "an error through simplify, subs and diff: str() "
                        "its message");
        D_TESTS_EXPECT( (d_tests_fails_with(broken.subs("x", f).simplify()
                                                .eval<float>(),
                                            bracket)) &&
                        (d_tests_fails_with(broken.diff("x")
                                                .eval<long double>(at),
                                            bracket)) &&
                        (d_tests_fails_with(broken.simplify().eval(),
                                            bracket)),
                        "an error through the members: eval its error, in "
                        "a type or as a value");
        D_TESTS_EXPECT( d_tests_holds(f.subs("x", broken).simplify()
                                          .diff("x", 2),
                                      bracket),
                        "a replacement's error, through every member after "
                        "it");

        return;
    }

    // d_tests_handle_agrees
    //   the handle and the entries, on 1,000 random terms of 2d's language
    // (d_tests_language): str() the text expr_render writes, and parse and
    // try_parse of it handles of the term; eval in double and as a value
    // expr_evaluate's answers, each a value or an error, at bindings that
    // leave some names unbound.
    void
    d_tests_handle_agrees()
    {
        const d_tests_alphabet alphabet = d_tests_language();
        const m::expr_bindings at       =
            m::at("a", m::value::ratio(1, 3)).at("x", 2).at("when", -0.5);
        d_tests_rng            rng(20261024u);
        std::size_t            texts    = 0;
        std::size_t            reads    = 0;
        std::size_t            typed    = 0;
        std::size_t            valued   = 0;
        std::size_t            failing  = 0;

        // 1,000 terms, up to five operators deep
        for (unsigned i = 0; i < 1000; ++i)
        {
            const m::expr_term            term   =
                d_tests_random_term(rng,
                                    alphabet,
                                    1 + (i % 5));
            const std::string             text   = m::expr_render(term);
            const m::expr                 handle = m::expr(term);
            const m::expr                 read   = m::parse(text);
            const m::expr_result<m::expr> tried  = m::try_parse(text);
            const m::expr_result<double>  want   =
                m::expr_evaluate<double>(term, at);

            texts   += ( (handle.str() == text) ? 1 : 0 );
            reads   += ( ( (read.ok()) &&
                           (d_tests_same(read.term(), term)) &&
                           (tried.is_ok()) &&
                           (d_tests_same(tried.value().term(), term)) )
                             ? 1 : 0 );
            typed   += ( d_tests_agree(handle.eval<double>(at), want)
                             ? 1 : 0 );
            valued  += ( d_tests_agree(handle.eval(at),
                                       m::expr_evaluate(term,
                                                        at,
                                                        m::expr_context()))
                             ? 1 : 0 );
            failing += ( want.is_err() ? 1 : 0 );
        }

        D_TESTS_EXPECT( (texts == 1000) &&
                        (reads == 1000),
                        "1,000 random terms: str() expr_render's text, and "
                        "parse and try_parse of it handles of the term");
        D_TESTS_EXPECT( (typed == 1000) &&
                        (valued == 1000),
                        "the same terms: eval in double and as a value "
                        "expr_evaluate's answers");
        D_TESTS_EXPECT( (failing > 0) &&
                        (failing < 1000),
                        "some of them failing in the fold, an unbound "
                        "variable or an unknown function, and some not");

        return;
    }


    //==========================================================================
    // 14. THE ENTRIES' TYPES
    //==========================================================================

    // d_tests_types
    //   each entry's type, by static assertion, so that a change to one is
    // a change to this test.
    void
    d_tests_types()
    {
        const m::expr_term     t;
        const m::expr_bindings b;
        const m::value         v;
        const m::expr          h = m::expr(m::expr_number(v));

        D_STATIC_ASSERT( (std::is_same<decltype(m::expr_tokenize("")),
                                       m::expr_result<std::vector<
                                           m::expr_token> > >::value),
                         "expr_tokenize");
        D_STATIC_ASSERT( (std::is_same<decltype(m::expr_parse("")),
                                       m::expr_result<m::expr_term> >::value) &&
                         (std::is_same<decltype(m::expr_parse_tokens(
                                           std::vector<m::expr_token>())),
                                       m::expr_result<m::expr_term> >::value),
                         "expr_parse, expr_parse_tokens");
        D_STATIC_ASSERT( (std::is_same<decltype(m::expr_render(t)),
                                       std::string>::value),
                         "expr_render");
        D_STATIC_ASSERT( (std::is_same<decltype(m::expr_evaluate<double>(t)),
                                       m::expr_result<double> >::value) &&
                         (std::is_same<decltype(m::expr_evaluate(
                                           t, b, m::expr_context())),
                                       m::expr_result<m::value> >::value),
                         "expr_evaluate");
        D_STATIC_ASSERT( (std::is_same<decltype(m::expr_substitute(t, "x", t)),
                                       m::expr_term>::value) &&
                         (std::is_same<decltype(m::expr_simplify(t)),
                                       m::expr_term>::value),
                         "expr_substitute, expr_simplify");
        D_STATIC_ASSERT( (std::is_same<decltype(m::expr_derivative(t, "x")),
                                       m::expr_result<m::expr_term> >::value) &&
                         (std::is_same<decltype(m::expr_gradient(
                                           t, std::vector<std::string>())),
                                       m::expr_result<std::vector<
                                           m::expr_term> > >::value),
                         "expr_derivative, expr_gradient");
        D_STATIC_ASSERT( (std::is_same<decltype(m::expr_integrate<float>(
                                           t, "x", 0.0f, 1.0f)),
                                       m::expr_result<float> >::value) &&
                         (std::is_same<decltype(m::expr_limit<long double>(
                                           t, "x", 0.0L)),
                                       m::expr_result<long double> >::value) &&
                         (std::is_same<decltype(m::expr_taylor(t, "x", t, 2)),
                                       m::expr_result<m::expr_term> >::value),
                         "the calculus entries");
        D_STATIC_ASSERT( (std::is_same<decltype(m::expr_matrix(1, 1,
                                           std::vector<m::value>())),
                                       m::expr_result<m::value> >::value) &&
                         (std::is_same<decltype(m::expr_shape_of(v)),
                                       m::expr_result<
                                           m::expr_shape> >::value) &&
                         (std::is_same<decltype(m::expr_is_matrix(v)),
                                       bool>::value),
                         "the matrix entries");
        D_STATIC_ASSERT( (std::is_same<decltype(h.str()),
                                       std::string>::value) &&
                         (std::is_same<decltype(h.eval<double>()),
                                       m::expr_result<double> >::value) &&
                         (std::is_same<decltype(h.eval()),
                                       m::expr_result<m::value> >::value) &&
                         (std::is_same<decltype(h.eval(b, m::expr_context())),
                                       m::expr_result<m::value> >::value) &&
                         (std::is_same<decltype(h.subs("x", h)),
                                       m::expr>::value) &&
                         (std::is_same<decltype(h.simplify()),
                                       m::expr>::value) &&
                         (std::is_same<decltype(h.diff("x")),
                                       m::expr>::value) &&
                         (std::is_same<decltype(h.diff("x", 2)),
                                       m::expr>::value) &&
                         (std::is_same<decltype(m::parse("")),
                                       m::expr>::value) &&
                         (std::is_same<decltype(m::try_parse("")),
                                       m::expr_result<m::expr> >::value) &&
                         (std::is_same<decltype(m::at("x", v)),
                                       m::expr_bindings>::value),
                         "the handle and its entries");

        static_cast<void>(t);
        static_cast<void>(b);

        D_TESTS_EXPECT( (h.ok()),
                        "the entries' types asserted");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_HANDLE_HPP
