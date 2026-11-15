/*******************************************************************************
* djinterp [cpp/math]                                    t_expr_program_face.cpp
*
* Tests for a term compiled once (math/expr/program.hpp, S395):
*   - the fold's answers: 2,000 random terms of every operator, call,
*     constant and literal, at values of every sort, in float, double, long
*     double and the math::complex of each, each run the fold's, bit for
*     bit, or the fold's error, its status and its text;
*   - the runs: of Type's own numbers, of math::values, of a vector of them,
*     and of bindings, one answer;
*   - the variables: expr_variables' order; names given in another order, a
*     name the term does not read, a name read and not given; and the first
*     variable unbound, in the fold's order, at a run of bindings;
*   - the edges: a term made by default, a program made by default, a term
*     deeper than the frame's stack, terms as deep as it and one deeper, a
*     call of more arguments than an operation keeps in the frame, the
*     context given to each evaluator (S340), and a program's size and
*     depth; terms the parser does not make, each refused as the fold
*     refuses it: a constant the table has not, indexing of three indices,
*     a sum of one operand and of three;
*   - the handle: expr's compile, and the error it holds passed on.
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/expr/t_expr_program_face.cpp         \
*         -o t_program
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_program_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.14
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/program.hpp"  // unit under test
// std
#include <cstdio>  // std::printf


// D_TESTS_HEADER_READ
//   1 where the header was read: at every level, as it defines its guard
// before its floor.
#if defined(DJINTERP_MATH_EXPR_PROGRAM_HPP)
    #define D_TESTS_HEADER_READ 1
#else
    #define D_TESTS_HEADER_READ 0
#endif


namespace
{
    long g_checks   = 0;
    long g_failures = 0;


    //==========================================================================
    // 1.  REPORTING
    //==========================================================================

    // d_tests_expect
    //   one check, reported where it fails.
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
#include <cmath>    // std::signbit
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/complex_kind.hpp"  // the kind
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/expr.hpp"     // the handle
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/parse.hpp"    // expr_parse

namespace m = djinterp::math;

namespace
{
    //==========================================================================
    // 2.  HELPERS
    //==========================================================================

    // d_tests_same_bits
    //   whether two numbers of a type are the same: equal with one sign, or
    // both NaNs.
    template<typename Type>
    bool
    d_tests_same_bits(
        Type _a,
        Type _b
    )
    {
        // two NaNs
        if ( (_a != _a) ||
             (_b != _b) )
        {
            return ( (_a != _a) &&
                     (_b != _b) );
        }

        return ( (_a == _b) &&
                 (std::signbit(_a) == std::signbit(_b)) );
    }

    template<typename Type>
    bool
    d_tests_same_bits(
        const m::complex<Type>& _a,
        const m::complex<Type>& _b
    )
    {
        return ( (d_tests_same_bits(_a.real(), _b.real())) &&
                 (d_tests_same_bits(_a.imag(), _b.imag())) );
    }

    // d_tests_same_answer
    //   whether two answers are one: two numbers of the same bits, or two
    // errors of the same status and text.
    template<typename Type>
    bool
    d_tests_same_answer(
        const m::expr_result<Type>& _a,
        const m::expr_result<Type>& _b
    )
    {
        // two errors
        if ( (_a.is_err()) ||
             (_b.is_err()) )
        {
            return ( (_a.is_err()) &&
                     (_b.is_err()) &&
                     (_a.error().status == _b.error().status) &&
                     (_a.error().message == _b.error().message) );
        }

        return d_tests_same_bits(_a.value(), _b.value());
    }

    // d_tests_term
    //   the term of a text, implied multiplication off; a term made by
    // default where it does not parse.
    m::expr_term
    d_tests_term(
        const std::string& _text
    )
    {
        const m::expr_result<m::expr_term> parsed =
            m::expr_parse(_text, m::expr_syntax());

        return ( parsed.is_ok() ? parsed.value()
                                : m::expr_term() );
    }

    // d_tests_rng
    //   struct: a small generator of the test's own, a 64-bit linear
    // congruential one, so that every build draws the same terms.
    struct d_tests_rng
    {
        unsigned long long m_state;

        explicit d_tests_rng(
            unsigned long long _seed
        )
            : m_state(_seed)
        {}

        // below
        //   a number in [0, _n).
        unsigned
        below(
            unsigned _n
        )
        {
            m_state = ( (m_state * 6364136223846793005ull) +
                        1442695040888963407ull );

            return static_cast<unsigned>((m_state >> 33) % _n);
        }
    };

    // d_tests_random_call
    //   a call of a function drawn, its arguments _depth operations deep:
    // three times in four as many as the function takes, else none to
    // three, so that each wrong count is drawn too, and f, which is no
    // function, now and then.
    std::string
    d_tests_random_call(
        d_tests_rng& _rng,
        unsigned     _depth
    );

    // d_tests_random_text
    //   a term's text, at most _depth operations deep, each operand in
    // brackets: a literal, a name, a constant; a prefix or postfix
    // operator, a binary one, a call, indexing, or a vector literal, by
    // sixteenths: a leaf 2, a prefix 2, a postfix 1, a binary operator 5,
    // a call 3, indexing 2, a vector literal 1.
    std::string
    d_tests_random_text(
        d_tests_rng& _rng,
        unsigned     _depth
    )
    {
        static const char* const leaves[] = {
            "0", "1", "2", "0.5", "1.25", "3e2", "1e-3", "123456789", "0.1",
            "x", "y", "z", "x", "y", "pi", "e", "true", "false", "im" };
        static const char* const prefixes[] = { "-", "+", "not " };
        static const char* const postfixes[] = { "!", "'" };
        static const char* const binaries[] = {
            " + ", " - ", " * ", " / ", " mod ", " ^ ", " = ", " != ",
            " < ", " <= ", " > ", " >= ", " and ", " or " };
        const unsigned pick = ( (_depth == 0) ? 0u : _rng.below(16) );

        // a leaf
        if (pick < 2)
        {
            return leaves[_rng.below(19)];
        }

        // a call: its arguments its own
        if (pick < 5)
        {
            return d_tests_random_call(_rng, _depth);
        }

        const std::string a = "(" + d_tests_random_text(_rng, _depth - 1) +
                              ")";

        // a prefix operator
        if (pick < 7)
        {
            return prefixes[_rng.below(3)] + a;
        }

        // a postfix operator
        if (pick < 8)
        {
            return a + postfixes[_rng.below(2)];
        }

        // a binary operator
        if (pick < 13)
        {
            return a + binaries[_rng.below(14)] + "(" +
                   d_tests_random_text(_rng, _depth - 1) + ")";
        }

        // indexing
        if (pick < 15)
        {
            return a + "[" + d_tests_random_text(_rng, _depth - 1) + "]";
        }

        return "[" + a + ", 2]";
    }

    std::string
    d_tests_random_call(
        d_tests_rng& _rng,
        unsigned     _depth
    )
    {
        static const char* const functions[] = {
            "sin", "cos", "sqrt", "log", "exp", "atan2", "hypot", "max",
            "min", "abs", "floor", "sign", "f", "det", "rotmat", "log1p" };
        static const unsigned takes[] = {
            1, 1, 1, 1, 1, 2, 2, 3, 2, 1, 1, 1, 1, 1, 1, 1 };
        const unsigned which = _rng.below(16);
        const unsigned count = ( (_rng.below(4) != 0) ? takes[which]
                                                      : _rng.below(4) );
        std::string    call  = std::string(functions[which]) + "(";

        // each argument, a term of its own
        for (unsigned i = 0; i < count; ++i)
        {
            call += ( (i == 0) ? "" : ", " ) +
                    d_tests_random_text(_rng, _depth - 1);
        }

        return call + ")";
    }

    // d_tests_point
    //   one of the values the random terms are folded at, by _which: real
    // numbers of every sort, a complex one, a boolean, and a matrix.
    m::value
    d_tests_point(
        unsigned _which
    )
    {
        const double inf = std::numeric_limits<double>::infinity();

        switch (_which % 9)
        {
            case 0:  return m::value(0.7);
            case 1:  return m::value(-2.5);
            case 2:  return m::value(-0.0);
            case 3:  return m::value(3);
            case 4:  return m::value(1.0e300);
            case 5:  return m::value(-inf);
            case 6:  return m::expr_complex_of(m::complex<double>(1.0, -2.0));
            case 7:  return m::value(true);
            default: break;
        }

        return d_tests_term("[1, 2]").empty()
                   ? m::value()
                   : m::expr_evaluate(d_tests_term("[1, 2]"),
                                      m::expr_bindings(),
                                      m::expr_context()).value();
    }


    //==========================================================================
    // 3.  THE FOLD'S ANSWERS
    //==========================================================================

    // d_tests_tally
    //   struct: what the random terms came to in one type: the terms the
    // compiler took and refused, and the runs that gave a number and an
    // error (with all three names bound a run cannot fail: the fold's only
    // error past the term's shape is a name unbound, z at the second run).
    struct d_tests_tally
    {
        unsigned compiled;
        unsigned refused;
        unsigned numbers;
        unsigned errors;
    };

    // d_tests_against_fold
    //   in Type, each of _count random terms compiled, with x, y and z, and
    // run at bindings of all three and of x and y alone, against the fold
    // at the same: the fold's answer, bit for bit, or its error; a term the
    // compiler refuses, the fold's error with all three bound. One check a
    // term, named by _type and the term's text where it fails.
    template<typename Type>
    d_tests_tally
    d_tests_against_fold(
        unsigned    _count,
        const char* _type
    )
    {
        const std::vector<std::string> names = { "x", "y", "z" };
        d_tests_rng                    rng(20261113u);
        d_tests_tally                  tally = { 0, 0, 0, 0 };

        for (unsigned i = 0; i < _count; ++i)
        {
            const std::string      text = d_tests_random_text(rng, 4);
            const m::expr_term     term = d_tests_term(text);
            const m::expr_bindings all  =
                m::at("x", d_tests_point(i)).at("y", d_tests_point(i / 9))
                    .at("z", d_tests_point(i / 81));
            const m::expr_bindings two  =
                m::at("x", d_tests_point(i)).at("y", d_tests_point(i / 9));
            const m::expr_result<m::expr_program<Type> > program =
                m::expr_compile<Type>(term, names);
            const m::expr_result<Type> fold_all =
                m::expr_evaluate<Type>(term, all);
            const std::string what = std::string(_type) + ", " + text;
            bool              ok   = false;

            // refused: the fold's error, every name bound
            if (program.is_err())
            {
                ++tally.refused;
                ok = ( (fold_all.is_err()) &&
                       (fold_all.error().status ==
                            program.error().status) &&
                       (fold_all.error().message ==
                            program.error().message) );
            }
            // taken: each run the fold's
            else
            {
                const m::expr_result<Type> run_all =
                    program.value().run(all);
                const m::expr_result<Type> run_two =
                    program.value().run(two);

                ++tally.compiled;
                ++( run_all.is_ok() ? tally.numbers : tally.errors );
                ++( run_two.is_ok() ? tally.numbers : tally.errors );
                ok = ( (d_tests_same_answer(run_all, fold_all)) &&
                       (d_tests_same_answer(run_two,
                                            m::expr_evaluate<Type>(term,
                                                                   two))) );
            }

            d_tests_expect(ok, what.c_str(), __LINE__);
        }

        return tally;
    }

    // d_tests_counted
    //   whether a type's terms came to a fair share of each: a third to
    // nine tenths taken, a tenth or more refused, and of the runs, two
    // thirds or more numbers, and a twentieth or more errors.
    bool
    d_tests_counted(
        const d_tests_tally& _tally,
        unsigned             _count
    )
    {
        return ( (_tally.compiled + _tally.refused == _count) &&
                 (_tally.compiled * 3 > _count) &&
                 (_tally.compiled * 10 < _count * 9) &&
                 (_tally.refused * 10 > _count) &&
                 ( (_tally.numbers * 3) > (_tally.compiled * 2 * 2) ) &&
                 (_tally.errors * 20 > _tally.compiled * 2) );
    }

    // d_tests_fold_answers
    //   the random terms in each type: 2,000 a type, each a check; then
    // whether each type's came to a fair share of each, and the complex
    // types took the terms the real ones did.
    void
    d_tests_fold_answers()
    {
        const unsigned      n  = 2000;
        const d_tests_tally f  = d_tests_against_fold<float>(n, "float");
        const d_tests_tally d  = d_tests_against_fold<double>(n, "double");
        const d_tests_tally l  =
            d_tests_against_fold<long double>(n, "long double");
        const d_tests_tally cf =
            d_tests_against_fold<m::complex<float> >(n, "complex<float>");
        const d_tests_tally cd =
            d_tests_against_fold<m::complex<double> >(n, "complex<double>");
        const d_tests_tally cl = d_tests_against_fold<
            m::complex<long double> >(n, "complex<long double>");

        D_TESTS_EXPECT( (d_tests_counted(f, n)) &&
                        (d_tests_counted(d, n)) &&
                        (d_tests_counted(l, n)) &&
                        (d_tests_counted(cf, n)) &&
                        (d_tests_counted(cd, n)) &&
                        (d_tests_counted(cl, n)),
                        "in each type, a third or more of the terms taken "
                        "and a tenth or more refused; of the runs, two "
                        "thirds or more numbers and a twentieth or more "
                        "errors: no check above vacuous");
        D_TESTS_EXPECT( (f.compiled == d.compiled) &&
                        (d.compiled == l.compiled) &&
                        (cf.compiled == f.compiled) &&
                        (cd.compiled == f.compiled) &&
                        (cl.compiled == f.compiled),
                        "every type took the same terms: what the compiler "
                        "refuses depends on the term, not the type");

        return;
    }


    //==========================================================================
    // 4.  THE RUNS
    //==========================================================================

    // d_tests_runs
    //   one program's runs of each form, one answer: of doubles, of
    // math::values, of a vector of them, and of bindings; and in a complex
    // type, of math::values and of bindings.
    void
    d_tests_runs()
    {
        const m::expr_term term =
            d_tests_term("sin(x) ^ 2 + cos(y) ^ 2 * x + sqrt(x + 2)");
        const m::expr_program<double> program =
            m::expr_compile<double>(term).value();
        const m::expr_program<m::complex<double> > complex =
            m::expr_compile<m::complex<double> >(term).value();
        const double                  doubles[] = { 0.25, -1.5 };
        const std::vector<m::value>   values    = { m::value(0.25),
                                                    m::value(-1.5) };
        const m::expr_bindings        bindings  =
            m::at("y", m::value(-1.5)).at("x", m::value(0.25));
        const double                  folded    =
            m::expr_evaluate<double>(term, bindings).value();
        const std::vector<m::value>   negative  = { m::value(-4),
                                                    m::value(0) };
        const m::expr_result<m::complex<double> > root =
            m::expr_evaluate<m::complex<double> >(
                term, m::at("x", m::value(-4)).at("y", m::value(0)));

        D_TESTS_EXPECT( (program.variables().size() == 2) &&
                        (program.variables()[0] == "x") &&
                        (program.variables()[1] == "y") &&
                        (d_tests_same_bits(program.run(doubles), folded)) &&
                        (d_tests_same_bits(program.run(values.data()),
                                           folded)) &&
                        (d_tests_same_bits(program.run(values), folded)) &&
                        (d_tests_same_bits(program.run(bindings).value(),
                                           folded)),
                        "a run of doubles, of math::values, of a vector, "
                        "and of bindings in another order: the fold's "
                        "answer, bit for bit");
        D_TESTS_EXPECT( (d_tests_same_bits(complex.run(negative),
                                           root.value())) &&
                        (d_tests_same_bits(
                             complex.run(m::at("x", m::value(-4))
                                             .at("y", m::value(0))).value(),
                             root.value())) &&
                        (root.value().imag() != 0.0),
                        "in math::complex<double>, sqrt(-4 + 2) by the "
                        "fold's rule: a run of math::values and of "
                        "bindings, the fold's complex answer");

        return;
    }


    //==========================================================================
    // 5.  THE VARIABLES
    //==========================================================================

    // d_tests_variables
    //   the slots: the term's own order, names given in another, a name not
    // read, a name read and not given, and the first unbound at a run.
    void
    d_tests_variables()
    {
        const m::expr_term term = d_tests_term("b - a * c + b");
        const std::vector<std::string> own = m::expr_variables(term);
        const m::expr_program<double>  given =
            m::expr_compile<double>(term, { "c", "a", "unread", "b" })
                .value();
        const double                   values[] = { 2.0, 3.0, 100.0, 7.0 };
        const m::expr_result<m::expr_program<double> > short_of =
            m::expr_compile<double>(term, { "a", "b" });
        const m::expr_result<double>   fold_short = m::expr_evaluate<double>(
            term, m::at("a", m::value(1)).at("b", m::value(1)));
        const m::expr_result<double>   unbound = given.run(m::at("c", 1.0));
        const m::expr_result<double>   fold_unbound =
            m::expr_evaluate<double>(term, m::at("c", 1.0));

        D_TESTS_EXPECT( (own.size() == 3) &&
                        (own[0] == "b") &&
                        (own[1] == "a") &&
                        (own[2] == "c") &&
                        (m::expr_variables(m::expr_term()).empty()) &&
                        (m::expr_variables(d_tests_term("1 + pi")).empty()),
                        "expr_variables: each once, in the order first "
                        "read, b, a, c; none of a term made by default, or "
                        "of one of no variable");
        D_TESTS_EXPECT( (given.variables().size() == 4) &&
                        (d_tests_same_bits(given.run(values),
                                           (7.0 - (3.0 * 2.0)) + 7.0)),
                        "names given in another order, one the term does "
                        "not read: each value in its slot, b - a c + b");
        D_TESTS_EXPECT( (short_of.is_err()) &&
                        (short_of.error().status ==
                            m::expr_status::unbound_variable) &&
                        (fold_short.is_err()) &&
                        (short_of.error().message ==
                            fold_short.error().message),
                        "a name read and not given, c: refused, the fold's "
                        "error where no binding names it");
        D_TESTS_EXPECT( (unbound.is_err()) &&
                        (unbound.error().status ==
                            m::expr_status::unbound_variable) &&
                        (unbound.error().message ==
                            fold_unbound.error().message) &&
                        (unbound.error().message.find("b") !=
                            std::string::npos),
                        "a run of bindings naming c alone: unbound_variable "
                        "for b, the first read, as the fold's error, not a, "
                        "which comes first among the slots");

        return;
    }


    //==========================================================================
    // 6.  THE EDGES
    //==========================================================================

    // d_tests_deep
    //   a term _n sums deep, x + (x + (... + x)): its program deeper than
    // the frame's stack where _n is past the knob.
    m::expr_term
    d_tests_deep(
        unsigned _n
    )
    {
        m::expr_term term = m::expr_variable("x");

        for (unsigned i = 0; i < _n; ++i)
        {
            term = m::expr_application(m::expr_op_kind::add,
                                       m::expr_application(
                                           m::expr_op_kind::mul,
                                           m::expr_variable("x"),
                                           m::expr_number(m::value(i))),
                                       term);
        }

        return term;
    }

    // d_tests_chain
    //   x + (x + (... + x)), _n sums: a program _n + 1 deep.
    m::expr_term
    d_tests_chain(
        unsigned _n
    )
    {
        m::expr_term term = m::expr_variable("x");

        for (unsigned i = 0; i < _n; ++i)
        {
            term = m::expr_application(m::expr_op_kind::add,
                                       m::expr_variable("x"), term);
        }

        return term;
    }

    // d_tests_at_the_frame
    //   terms as deep as the frame's stack and one deeper, each in double
    // and in math::complex<double>: the fold's answer, bit for bit, the
    // first run in the frame, the second on the heap.
    bool
    d_tests_at_the_frame()
    {
        const unsigned knob = D_CFG_MATH_EXPR_PROGRAM_STACK;
        const m::value seven[] = { m::value(0.7) };
        const double   x       = 0.7;
        bool           ok      = true;

        // the frame's depth, and one past it
        for (unsigned n = knob - 1; n <= knob; ++n)
        {
            const m::expr_term term = d_tests_chain(n);
            const m::expr_program<double> real =
                m::expr_compile<double>(term).value();
            const m::expr_program<m::complex<double> > complex =
                m::expr_compile<m::complex<double> >(term).value();

            ok = ( (ok) &&
                   (real.depth() == n + 1) &&
                   (complex.depth() == n + 1) &&
                   (d_tests_same_bits(real.run(&x),
                        m::expr_evaluate<double>(term,
                                                 m::at("x", 0.7)).value())) &&
                   (d_tests_same_bits(complex.run(seven),
                        m::expr_evaluate<m::complex<double> >(
                            term, m::at("x", 0.7)).value())) );
        }

        return ok;
    }

    // d_tests_edges
    //   a term made by default; a program made by default; a deep term;
    // terms at the frame's depth; a call of more arguments than an
    // operation keeps in the frame; the context given to each evaluator; a
    // program's size and depth.
    void
    d_tests_edges()
    {
        const m::expr_term deep  = d_tests_deep(200);
        const m::expr_result<m::expr_program<double> > made_deep =
            m::expr_compile<double>(deep);
        const double x = 0.375;
        const m::expr_result<double> fold_deep =
            m::expr_evaluate<double>(deep, m::at("x", m::value(x)));
        const m::expr_program<double>  none;
        const m::expr_context          real(true);
        const m::expr_term             root = d_tests_term("sqrt(x)");
        const m::value                 minus4[] = { m::value(-4) };
        const m::expr_program<m::complex<double> > complex =
            m::expr_compile<m::complex<double> >(root).value();
        const m::expr_program<double>  small =
            m::expr_compile<double>(d_tests_term("1 + 2")).value();
        const m::expr_term             ten   = d_tests_term(
            "max(x, 2, 3, 4, 5, 6, 7, 8, 9, 10, x - 1)");
        const m::value                 bound[] = { m::value(11.5) };
        const m::expr_result<m::complex<double> > ten_fold =
            m::expr_evaluate<m::complex<double> >(ten, m::at("x", 11.5));

        D_TESTS_EXPECT( (m::expr_compile<double>(m::expr_term()).is_err()) &&
                        (m::expr_compile<double>(m::expr_term()).error()
                             .status == m::expr_status::missing_operand),
                        "a term made by default: missing_operand, as the "
                        "fold's");
        D_TESTS_EXPECT( (none.variables().empty()) &&
                        (none.run(static_cast<const double*>(D_NULLPTR)) !=
                             none.run(static_cast<const double*>(
                                 D_NULLPTR))),
                        "a program made by default: no variable, a NaN");
        D_TESTS_EXPECT( (made_deep.is_ok()) &&
                        (made_deep.value().depth() >
                             D_CFG_MATH_EXPR_PROGRAM_STACK) &&
                        (d_tests_same_bits(made_deep.value().run(&x),
                                           fold_deep.value())),
                        "a term 200 sums deep, its stack past the frame's: "
                        "the fold's answer, bit for bit");
        D_TESTS_EXPECT(d_tests_at_the_frame(),
                       "terms as deep as the frame's stack and one deeper, "
                       "in double and math::complex<double>: the fold's "
                       "answer, the first in the frame, the second on the "
                       "heap");
        D_TESTS_EXPECT( (ten_fold.is_ok()) &&
                        (ten_fold.value().real() == 11.5) &&
                        (ten_fold.value().imag() == 0.0) &&
                        (d_tests_same_bits(
                             m::expr_compile<m::complex<double> >(ten)
                                 .value().run(bound),
                             ten_fold.value())),
                        "max of eleven arguments in math::complex<double>, "
                        "more than an operation keeps in the frame: 11.5, "
                        "by the fold and by a run");
        D_TESTS_EXPECT( (d_tests_same_bits(
                             complex.run(minus4, real),
                             m::expr_evaluate<m::complex<double> >(
                                 root, m::at("x", m::value(-4)), real)
                                 .value())) &&
                        (complex.run(minus4, real).real() !=
                             complex.run(minus4, real).real()) &&
                        (complex.run(minus4).imag() == 2.0),
                        "the context given to each evaluator: sqrt(-4) in "
                        "math::complex<double> 2i, and a NaN under "
                        "real_results, as the fold's (S340)");
        D_TESTS_EXPECT( (small.size() == 3) &&
                        (small.depth() == 2) &&
                        (d_tests_same_bits(small.run(
                             static_cast<const double*>(D_NULLPTR)), 3.0)),
                        "1 + 2: three steps, two deep, 3");

        return;
    }


    // d_tests_same_error
    //   in Type, whether _term compiled is refused with the error the fold
    // gives, x bound: its status and its text.
    template<typename Type>
    bool
    d_tests_same_error(
        const m::expr_term& _term
    )
    {
        const m::expr_result<m::expr_program<Type> > made =
            m::expr_compile<Type>(_term, { "x" });
        const m::expr_result<Type> folded =
            m::expr_evaluate<Type>(_term, m::at("x", 0.5));

        return ( (made.is_err()) &&
                 (folded.is_err()) &&
                 (made.error().status == folded.error().status) &&
                 (made.error().message == folded.error().message) );
    }

    // d_tests_shapes
    //   terms the parser does not make, built by hand, each refused as the
    // fold refuses it, in double and in math::complex<double>: a constant
    // the table has not, indexing of three indices, a sum of one operand
    // and of three.
    void
    d_tests_shapes()
    {
        typedef m::complex<double> cd;

        const m::expr_term x = m::expr_variable("x");
        const m::expr_term one = m::expr_number(m::value(1));
        const m::expr_term unknown = m::expr_application(
            m::expr_op_kind::add, x, m::expr_constant("tau"));
        const m::expr_term indexed = m::expr_application(
            m::expr_op_kind::index,
            std::vector<m::expr_term>({ x, one, one, one }));
        const m::expr_term lonely =
            m::expr_application(m::expr_op_kind::add, x);
        const m::expr_term crowded = m::expr_application(
            m::expr_op_kind::add, std::vector<m::expr_term>({ x, x, x }));

        D_TESTS_EXPECT( (d_tests_same_error<double>(unknown)) &&
                        (d_tests_same_error<cd>(unknown)) &&
                        (m::expr_compile<double>(unknown, { "x" })
                             .error().status ==
                            m::expr_status::unknown_name),
                        "a constant the table has not, tau: unknown_name, "
                        "the fold's error, in double and complex<double>");
        D_TESTS_EXPECT( (d_tests_same_error<double>(indexed)) &&
                        (d_tests_same_error<cd>(indexed)),
                        "indexing of three indices: the fold's error, in "
                        "double and complex<double>");
        D_TESTS_EXPECT( (d_tests_same_error<double>(lonely)) &&
                        (d_tests_same_error<cd>(lonely)) &&
                        (d_tests_same_error<double>(crowded)) &&
                        (d_tests_same_error<cd>(crowded)),
                        "a sum of one operand and of three: the fold's "
                        "error, in double and complex<double>");

        return;
    }


    //==========================================================================
    // 7.  THE HANDLE
    //==========================================================================

    // d_tests_handle
    //   expr's compile: its term's program, and the error it holds passed
    // on.
    void
    d_tests_handle()
    {
        const m::expr f = m::parse("x ^ 2 - y");
        const m::expr bad = m::parse("x +");
        const double  values[] = { 3.0, 1.0 };

        D_TESTS_EXPECT( (f.compile<double>().is_ok()) &&
                        (d_tests_same_bits(
                             f.compile<double>().value().run(values),
                             8.0)) &&
                        (f.compile<double>({ "y", "x" }).value()
                             .variables()[0] == "y") &&
                        (bad.compile<float>().is_err()) &&
                        (bad.compile<float>().error().message ==
                             bad.error().message) &&
                        (bad.compile<double>({ "x" }).is_err()) &&
                        (bad.compile<double>({ "x" }).error().message ==
                             bad.error().message),
                        "expr's compile: x ^ 2 - y at 3 and 1, 8; names "
                        "given; a handle's error passed on, with names "
                        "given or not");

        return;
    }


    //==========================================================================
    // 8.  THE MODULE
    //==========================================================================

    // d_tests_module
    //   every part, from the module's floor.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_HEADER_READ == 1),
                        "the module's headers read");

        d_tests_fold_answers();
        d_tests_runs();
        d_tests_variables();
        d_tests_edges();
        d_tests_shapes();
        d_tests_handle();

        return;
    }
}  // namespace

#else

namespace
{
    // d_tests_module
    //   below C++11 the module is empty: its headers read, nothing more.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_HEADER_READ == 1),
                        "the module's headers read, empty below C++11");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main(void)
{
    d_tests_module();

    std::printf("t_expr_program_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return ( (g_failures == 0) ? 0 : 1 );
}
