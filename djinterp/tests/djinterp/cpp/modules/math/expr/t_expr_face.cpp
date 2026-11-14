/*******************************************************************************
* djinterp [cpp/math]                                            t_expr_face.cpp
*
* Tests for math/expr/, the run-time expression language, as steps 13 to 22 (2a
* to 3d) and step 23's first part (3e) made it: every header in place, its types
* whole; the arithmetic, the functions and the constants, the relations, logic
* and piecewise parsed, printed and evaluated; the handle, math::expr;
* substitution and simplification; and vectors and matrices, their kind, their
* literals, their arithmetic and indexing, and linear algebra's functions.
*   - every header read, at every level, and empty below C++11;
*   - the types: the error, its statuses and the answer of a step; the
*     syntax switches; the token; the operator and the operator table; the
*     atom, the term and its constructors and observers, and the term under
*     parse/expression's queries and transforms; the function and constant
*     tables; the bindings and the context; the shape of a matrix; the
*     handle;
*   - 2b's steps: the tokenizer; the operator table's arithmetic; the parser
*     layer, its grammar, implied multiplication and every error at its
*     place; the renderer, numbers with and without literals; both folds,
*     in each type and as a value, exact where they are; and the round trip,
*     random terms rendered and parsed back as themselves;
*   - 2c's: the comma; the factorial; the standard functions and constants,
*     in each type and as values, exact wherever a function's value is
*     rational; calls and bars in the parser and the renderer; and calls,
*     constants and the factorial in the round trip;
*   - 2d's: the relations and logic at their levels, logic below the
*     relations; chains; brackets as one operand; piecewise, its branches
*     and their errors; true, false, all_of and any_of; both folds, 1 and 0
*     in each type and booleans as values; and all of it in the round trip;
*   - 2e's: the handle, what it holds; str() and eval, in each type and as
*     a value; subs, simplify and diff, each its entry's; parse and
*     try_parse, by the syntax given; chains, roadmap Phase 2's examples
*     among them; an error passed through every member; and the handle
*     agreeing with the entries on random terms;
*   - 2f's: substitution, a variable replaced and nothing else, and on
*     random terms the value it gives; simplification, each rule, what it
*     leaves as it is, and on random terms no larger, a fixed point, its
*     text read back the same, and the value kept wherever each part of the
*     term is finite;
*   - 3a's: the square brackets and the semicolon as tokens; the matrix
*     kind, its storage keeping each element as given, its table and entries,
*     and its holders; expr_matrix, expr_vector, expr_is_matrix,
*     expr_shape_of and expr_element, and their errors; the literals in the
*     parser, each error at its place; the literals and the matrices a
*     fold makes in the renderer, read back; both folds, a literal's matrix
*     as a value and no value in a type; the operators, the rewrites and
*     the handle beside them; and random literals, read back, folded,
*     simplified and substituted in, as their elements are;
*   - 3b's: the kind's arithmetic, sums, scalar products and the product,
*     each result's storage, and what it refuses; the transpose, powers and
*     indexing as values, and two matrices compared; indexing and the
*     transpose in the parser, every error at its place, and in the
*     renderer, read back; both folds of matrix texts; simplification
*     keeping matrix factors in order and no error of matrices made a
*     value; and 1,000 random terms of matrices, each folded as the random
*     part's own arithmetic gives it, read back, and simplified to its
*     value;
*   - 3c's: linear algebra's functions, det, inv, trace, norm, eig, dot,
*     cross and solve, their entries and the table's, of matrices exact,
*     dense in each type, the C core's bit for bit, and of values with an
*     inexact element, and of numbers as scalars, in each type and as
*     values; a norm past its type's range scaled, not an infinity; the
*     eigenvalues exact where rational, repeated, beside irrational ones,
*     a dense symmetric matrix's by Jacobi where QR would find a complex
*     pair, a complex pair complex numbers, exact where they are Gaussian
*     rationals (S352); transpose, in the table; negative powers, and
*     every dense power the C core's; the empty matrix; literals of
*     blocks, in the parser, the folds, the renderer and simplification;
*     and 1,000 random exact matrices against the test's own cofactors and
*     arithmetic;
*   - 3d's: im, jm and km, constants of the table and never variables, as
*     values, in each type and through the handle;
*   - 3e's first part: im the exact imaginary unit as a value and a NaN in
*     each type, complex, real, imag, conj and arg in the table with their
*     four evaluators (t_expr_complex_face.cpp has the rest);
*   - 3f's second part: jm and km the exact units of the quaternions as
*     values, their product in order, and NaNs in each type; quat, parts and
*     rotate in the table with their seven evaluators, and its third part's
*     rotmat, rotquat and slerp, 58 functions (t_expr_quaternion_face.cpp
*     and t_expr_quaternion_matrix_face.cpp have the rest);
*   - every stub of a later step: the answer its contract names until its
*     step, the error not_yet with its message;
*   - each entry's type, by static assertion.
* Each later step of Phases 2 to 4 turns its stubs' checks into checks of
* what it fills.
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/expr/t_expr_face.cpp -o t_expr_face
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/antiderivative.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/complex.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/derivative.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/evaluate.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/expr.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/expr_common.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/function.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/matrix.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/numeric.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/operator.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/parse.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/quaternion.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/render.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/series.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/simplify.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/substitute.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/term.hpp"
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/token.hpp"
// std
#include <cstdio>  // std::printf


// D_TESTS_MODULE_READ
//   1 where each of math/expr/'s eighteen headers was read: at every level,
// as each defines its guard before its floor.
#if ( defined(DJINTERP_MATH_EXPR_ANTIDERIVATIVE_HPP) &&                       \
      defined(DJINTERP_MATH_EXPR_COMPLEX_HPP)        &&                       \
      defined(DJINTERP_MATH_EXPR_DERIVATIVE_HPP)     &&                       \
      defined(DJINTERP_MATH_EXPR_EVALUATE_HPP)       &&                       \
      defined(DJINTERP_MATH_EXPR_EXPR_HPP)           &&                       \
      defined(DJINTERP_MATH_EXPR_EXPR_COMMON_HPP)    &&                       \
      defined(DJINTERP_MATH_EXPR_FUNCTION_HPP)       &&                       \
      defined(DJINTERP_MATH_EXPR_MATRIX_HPP)         &&                       \
      defined(DJINTERP_MATH_EXPR_NUMERIC_HPP)        &&                       \
      defined(DJINTERP_MATH_EXPR_OPERATOR_HPP)       &&                       \
      defined(DJINTERP_MATH_EXPR_PARSE_HPP)          &&                       \
      defined(DJINTERP_MATH_EXPR_QUATERNION_HPP)     &&                       \
      defined(DJINTERP_MATH_EXPR_RENDER_HPP)         &&                       \
      defined(DJINTERP_MATH_EXPR_SERIES_HPP)         &&                       \
      defined(DJINTERP_MATH_EXPR_SIMPLIFY_HPP)       &&                       \
      defined(DJINTERP_MATH_EXPR_SUBSTITUTE_HPP)     &&                       \
      defined(DJINTERP_MATH_EXPR_TERM_HPP)           &&                       \
      defined(DJINTERP_MATH_EXPR_TOKEN_HPP) )
    #define D_TESTS_MODULE_READ 1
#else
    #define D_TESTS_MODULE_READ 0
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

// std, from the module's floor
#include <algorithm>    // std::sort
#include <cfloat>       // FLT_EPSILON
#include <cmath>        // std::signbit, std::isinf, std::fabs
#include <cstddef>      // std::size_t
#include <cstring>      // std::strlen
#include <limits>       // std::numeric_limits
#include <string>       // std::string
#include <type_traits>  // std::is_same, std::is_default_constructible
#include <vector>       // std::vector
// djinterp
#include "../../../../../../inc/djinterp/c/modules/math/literal.h"        // literal_to_value
#include "../../../../../../inc/djinterp/c/modules/math/value_wide.h"     // a boxed kind
#include "../../../../../../inc/djinterp/cpp/modules/math/constants.hpp"  // constants::of
#include "../../../../../../inc/djinterp/cpp/modules/parse/expression/expression_ops.hpp"


namespace
{
    namespace dj = djinterp;
    namespace m  = djinterp::math;


    //==========================================================================
    // 2.  HELPERS
    //==========================================================================

    // d_tests_not_yet
    //   whether a step answered as a stub does: the error not_yet, with no
    // place, and the message given.
    template<typename Type>
    bool
    d_tests_not_yet(
        const m::expr_result<Type>& _result,
        const char*                 _message
    )
    {
        return ( (_result.is_err()) &&
                 (_result.error() ==
                      m::expr_error(m::expr_status::not_yet,
                                    m::expr_no_offset,
                                    _message)) );
    }

    // d_tests_holds_not_yet
    //   whether a handle holds the error a stub gives.
    bool
    d_tests_holds_not_yet(
        const m::expr& _handle,
        const char*    _message
    )
    {
        return ( (!_handle.ok()) &&
                 (_handle.error() ==
                      m::expr_error(m::expr_status::not_yet,
                                    m::expr_no_offset,
                                    _message)) );
    }

    // d_tests_fails_with
    //   whether a step failed with the error given.
    template<typename Type>
    bool
    d_tests_fails_with(
        const m::expr_result<Type>& _result,
        const m::expr_error&        _error
    )
    {
        return ( (_result.is_err()) &&
                 (_result.error() == _error) );
    }

    // d_tests_holds
    //   whether a handle holds the error given.
    bool
    d_tests_holds(
        const m::expr&       _handle,
        const m::expr_error& _error
    )
    {
        return ( (!_handle.ok()) &&
                 (_handle.error() == _error) );
    }

    // d_tests_same
    //   whether two terms are the same, by parse/expression's structural
    // equality; a term made by default, which has no node, the same only as
    // another.
    bool
    d_tests_same(
        const m::expr_term& _a,
        const m::expr_term& _b
    )
    {
        // a term made by default
        if ( (_a.empty()) ||
             (_b.empty()) )
        {
            return ( (_a.empty()) &&
                     (_b.empty()) );
        }

        return dj::structural_equal<m::expr_op, m::expr_atom>(_a, _b);
    }

    // d_tests_x_plus_1
    //   x + 1, the 1 written "1".
    m::expr_term
    d_tests_x_plus_1()
    {
        return m::expr_application(m::expr_op_kind::add,
                                   m::expr_variable("x"),
                                   m::expr_number(m::value(1), "1"));
    }

    // d_tests_is_error
    //   whether a step failed with the error given: its status, its place
    // and its message.
    template<typename Type>
    bool
    d_tests_is_error(
        const m::expr_result<Type>& _result,
        m::expr_status              _status,
        std::size_t                 _offset,
        const char*                 _message
    )
    {
        return ( (_result.is_err()) &&
                 (_result.error() == m::expr_error(_status,
                                                   _offset,
                                                   _message)) );
    }

    // d_tests_literal
    //   a number as expr_parse makes it of _text: its value read exactly,
    // and the text its literal.
    m::expr_term
    d_tests_literal(
        const char* _text
    )
    {
        dj_math_value exact = d_math_value_none();

        static_cast<void>(d_math_literal_to_value(&exact,
                                                  _text,
                                                  std::strlen(_text)));

        return m::expr_number(m::value::adopt(exact), _text);
    }

    // d_tests_apply
    //   an operator of a kind on one operand, or on two.
    m::expr_term
    d_tests_apply(
        m::expr_op_kind     _kind,
        const m::expr_term& _operand
    )
    {
        return m::expr_application(_kind, _operand);
    }

    m::expr_term
    d_tests_apply(
        m::expr_op_kind     _kind,
        const m::expr_term& _left,
        const m::expr_term& _right
    )
    {
        return m::expr_application(_kind, _left, _right);
    }

    // d_tests_parses
    //   whether a text parses, with _syntax, to the term _want.
    bool
    d_tests_parses(
        const char*           _text,
        const m::expr_term&   _want,
        const m::expr_syntax& _syntax = m::expr_syntax()
    )
    {
        const m::expr_result<m::expr_term> parsed = m::expr_parse(_text,
                                                                  _syntax);

        return ( (parsed.is_ok()) &&
                 (d_tests_same(parsed.value(), _want)) );
    }

    // d_tests_parsed
    //   a text's term, where it parses; a term made by default where it does
    // not, so that a check of it fails rather than read what the parse
    // does not hold.
    m::expr_term
    d_tests_parsed(
        const char*           _text,
        const m::expr_syntax& _syntax = m::expr_syntax()
    )
    {
        const m::expr_result<m::expr_term> parsed = m::expr_parse(_text,
                                                                  _syntax);

        return ( parsed.is_ok() ? parsed.value() : m::expr_term() );
    }

    // d_tests_renders
    //   whether a text parses, and renders as _want.
    bool
    d_tests_renders(
        const char*           _text,
        const char*           _want,
        const m::expr_syntax& _syntax = m::expr_syntax()
    )
    {
        const m::expr_result<m::expr_term> parsed = m::expr_parse(_text,
                                                                  _syntax);

        return ( (parsed.is_ok()) &&
                 (m::expr_render(parsed.value()) == _want) );
    }

    // d_tests_same_real
    //   two floating-point numbers the same once stored, so that the x87's
    // excess precision does not tell them apart: equal with one sign, or
    // both NaN.
    template<typename Real>
    bool
    d_tests_same_real(
        Real _a,
        Real _b
    )
    {
        volatile Real a = _a;
        volatile Real b = _b;

        return ( ( (a == b) &&
                   (std::signbit(a) == std::signbit(b)) ) ||
                 ( (a != a) &&
                   (b != b) ) );
    }

    // d_tests_stored
    //   _x stored in its type and read back, so that the x87 holds no more
    // of it than the type has: the number a value of that kind holds.
    template<typename Real>
    Real
    d_tests_stored(
        Real _x
    )
    {
        volatile Real stored = _x;

        return stored;
    }

    // d_tests_typed
    //   whether a text's value in Type, at _bindings, is _want.
    template<typename Type>
    bool
    d_tests_typed(
        const char*             _text,
        Type                    _want,
        const m::expr_bindings& _bindings = m::expr_bindings()
    )
    {
        const m::expr_result<m::expr_term> parsed = m::expr_parse(_text);

        // the text parses, and its value is the one wanted
        if (parsed.is_err())
        {
            return false;
        }

        const m::expr_result<Type> got =
            m::expr_evaluate<Type>(parsed.value(), _bindings);

        return ( (got.is_ok()) &&
                 (d_tests_same_real(got.value(), _want)) );
    }

    // d_tests_valued
    //   whether a text's value as a value, at _bindings, is _want: of its
    // kind, and the same number (expr_same_number).
    bool
    d_tests_valued(
        const char*             _text,
        const m::value&         _want,
        const m::expr_bindings& _bindings = m::expr_bindings()
    )
    {
        const m::expr_result<m::expr_term> parsed = m::expr_parse(_text);

        // the text parses, and its value is the one wanted
        if (parsed.is_err())
        {
            return false;
        }

        const m::expr_result<m::value> got =
            m::expr_evaluate(parsed.value(), _bindings, m::expr_context());

        return ( (got.is_ok()) &&
                 (m::internal::expr_same_number(got.value(), _want)) );
    }

    // d_tests_value_of
    //   a text's value as a value, at _bindings; none where the text does
    // not parse or has no value.
    m::value
    d_tests_value_of(
        const char*             _text,
        const m::expr_bindings& _bindings = m::expr_bindings()
    )
    {
        const m::expr_result<m::expr_term> parsed = m::expr_parse(_text);

        // the text parses
        if (parsed.is_err())
        {
            return m::value();
        }

        const m::expr_result<m::value> got =
            m::expr_evaluate(parsed.value(), _bindings, m::expr_context());

        return ( got.is_ok() ? got.value() : m::value() );
    }

    // d_tests_wide_of
    //   a wide integer value, a boxed kind's, of a decimal text.
    m::value
    d_tests_wide_of(
        const char* _digits
    )
    {
        dj_math_wide wide = d_math_wide_from_imax(0);

        static_cast<void>(d_math_wide_from_text(&wide,
                                                _digits,
                                                std::strlen(_digits)));

        return m::value::adopt(d_math_value_from_wide(wide));
    }

    // d_tests_real_*
    //   a boxed kind of the test's own, a number that is not exact, of rank
    // 600, past long double's: its box points to the long double it stands
    // for, which convert gives in each inline inexact kind. The folds ask
    // nothing else of it: hold does nothing, the arithmetic gives none, the
    // comparison no order, and the text the kind's name.
    void
    d_tests_real_hold(
        void* _box
    )
    {
        static_cast<void>(_box);

        return;
    }

    dj_math_value
    d_tests_real_unary(
        dj_math_value_op _op,
        dj_math_value    _x
    )
    {
        static_cast<void>(_op);
        static_cast<void>(_x);

        return d_math_value_none();
    }

    dj_math_value
    d_tests_real_binary(
        dj_math_value_op _op,
        dj_math_value    _a,
        dj_math_value    _b
    )
    {
        static_cast<void>(_op);
        static_cast<void>(_a);
        static_cast<void>(_b);

        return d_math_value_none();
    }

    dj_math_value_order
    d_tests_real_cmp(
        dj_math_value _a,
        dj_math_value _b
    )
    {
        static_cast<void>(_a);
        static_cast<void>(_b);

        return D_MATH_VALUE_UNORDERED;
    }

    bool
    d_tests_real_convert(
        dj_math_value*     _out,
        int*               _direction,
        dj_math_value      _x,
        dj_math_value_kind _to
    )
    {
        const long double real    = *static_cast<const long double*>(_x.box);
        long double       nearest = real;

        // each inline inexact kind its nearest; no exact kind
        switch (_to)
        {
            case D_MATH_VALUE_FLOAT:
                nearest = static_cast<float>(real);
                *_out   = d_math_value_from_f(static_cast<float>(real));
                break;

            case D_MATH_VALUE_DOUBLE:
                nearest = static_cast<double>(real);
                *_out   = d_math_value_from_d(static_cast<double>(real));
                break;

            case D_MATH_VALUE_LONG_DOUBLE:
                *_out = d_math_value_from_ld(real);
                break;

            default:
                return false;
        }

        *_direction = ( (nearest < real) ? -1 : ( (nearest > real) ? 1 : 0 ) );

        return true;
    }

    std::size_t
    d_tests_real_text(
        char*         _buffer,
        std::size_t   _size,
        dj_math_value _x
    )
    {
        static_cast<void>(_x);

        return static_cast<std::size_t>(std::snprintf(_buffer,
                                                      _size,
                                                      "real"));
    }

    const dj_math_boxed_kind g_real_kind = { "real",
                                             600,
                                             D_MATH_BOXED_NUMBER,
                                             &d_tests_real_hold,
                                             &d_tests_real_hold,
                                             &d_tests_real_unary,
                                             &d_tests_real_binary,
                                             &d_tests_real_cmp,
                                             &d_tests_real_convert,
                                             &d_tests_real_text,
                                             D_NULLPTR };

    // d_tests_real_of
    //   a value of the test's boxed kind, standing for *_x.
    m::value
    d_tests_real_of(
        const long double* _x
    )
    {
        dj_math_value boxed = d_math_value_none();

        boxed.kind = D_MATH_VALUE_BOXED;
        boxed.type = &g_real_kind;
        boxed.box  = const_cast<long double*>(_x);

        return m::value::adopt(boxed);
    }

    // d_tests_tokens_are
    //   whether a text's tokens are those given: kinds, texts and offsets,
    // in order.
    bool
    d_tests_tokens_are(
        const char*                        _text,
        const std::vector<m::expr_token>&  _want,
        const m::expr_syntax&              _syntax = m::expr_syntax()
    )
    {
        const m::expr_result<std::vector<m::expr_token> > got =
            m::expr_tokenize(_text, _syntax);

        return ( (got.is_ok()) &&
                 (got.value() == _want) );
    }


    //==========================================================================
    // 3.  ERRORS AND SYNTAX
    //==========================================================================

    // d_tests_errors
    //   expr_status, expr_error, expr_result, expr_not_yet and expr_syntax.
    void
    d_tests_errors()
    {
        // the statuses, each its own
        const m::expr_status all[] =
        {
            m::expr_status::not_yet,
            m::expr_status::bad_character,
            m::expr_status::unexpected_token,
            m::expr_status::missing_operand,
            m::expr_status::unbalanced,
            m::expr_status::unbound_variable,
            m::expr_status::unknown_name,
            m::expr_status::wrong_arity,
            m::expr_status::bad_shape,
            m::expr_status::not_differentiable,
            m::expr_status::no_convergence,
            m::expr_status::no_closed_form
        };
        const std::size_t count = sizeof(all) / sizeof(all[0]);
        bool              apart = true;

        for (std::size_t i = 0; i < count; ++i)
        {
            for (std::size_t j = i + 1; j < count; ++j)
            {
                apart = (apart && (all[i] != all[j]));
            }
        }

        D_TESTS_EXPECT( (count == 12) && apart,
                        "twelve statuses, each its own");
        D_TESTS_EXPECT( (static_cast<int>(m::expr_status::not_yet) == 1) &&
                        (static_cast<int>(m::expr_status::no_closed_form) ==
                             12),
                        "the statuses numbered 1 to 12");

        // the error
        const m::expr_error by_default;
        const m::expr_error full(m::expr_status::unbalanced, 7, "a bracket");

        D_TESTS_EXPECT( (m::expr_no_offset ==
                             static_cast<std::size_t>(-1)),
                        "expr_no_offset is the greatest size");
        D_TESTS_EXPECT( (by_default.status == m::expr_status::not_yet) &&
                        (by_default.offset == m::expr_no_offset) &&
                        (by_default.message.empty()),
                        "an error made by default: not_yet, no place, no "
                        "message");
        D_TESTS_EXPECT( (full.status == m::expr_status::unbalanced) &&
                        (full.offset == 7) &&
                        (full.message == "a bracket"),
                        "an error's three fields");
        D_TESTS_EXPECT( (full == m::expr_error(m::expr_status::unbalanced,
                                               7,
                                               "a bracket")) &&
                        (!(full != m::expr_error(m::expr_status::unbalanced,
                                                 7,
                                                 "a bracket"))),
                        "errors alike are equal");
        D_TESTS_EXPECT( (full != m::expr_error(m::expr_status::unexpected_token,
                                               7,
                                               "a bracket")) &&
                        (full != m::expr_error(m::expr_status::unbalanced,
                                               8,
                                               "a bracket")) &&
                        (full != m::expr_error(m::expr_status::unbalanced,
                                               7,
                                               "a brace")),
                        "errors differing in any field are unequal");

        // the answer of a step
        D_STATIC_ASSERT( (std::is_same<m::expr_result<int>,
                                       dj::result<int,
                                                  m::expr_error> >::value),
                         "expr_result is result<T, expr_error>");

        const m::expr_result<int> stub =
            m::internal::expr_not_yet<int>("a step: not yet");

        D_TESTS_EXPECT( d_tests_not_yet(stub, "a step: not yet"),
                        "expr_not_yet: not_yet, no place, its message");
        D_TESTS_EXPECT( (m::internal::expr_not_yet_error("a step") ==
                             m::expr_error(m::expr_status::not_yet,
                                           m::expr_no_offset,
                                           "a step")),
                        "expr_not_yet_error: the error alone");

        // the syntax switches, off by default
        const m::expr_syntax syntax;

        D_TESTS_EXPECT( (!syntax.implied_multiplication) &&
                        (!syntax.absolute_bars),
                        "gate G5's two options off by default");

        // the floating-point types
        D_STATIC_ASSERT( (m::internal::expr_floating<float>::value) &&
                         (m::internal::expr_floating<double>::value) &&
                         (m::internal::expr_floating<long double>::value),
                         "float, double and long double are typed folds' "
                         "types");
        D_STATIC_ASSERT( (!m::internal::expr_floating<int>::value) &&
                         (!m::internal::expr_floating<const double>::value) &&
                         (!m::internal::expr_floating<m::value>::value),
                         "no other type is");

        return;
    }


    //==========================================================================
    // 4.  TOKENS
    //==========================================================================

    // d_tests_tokens
    //   expr_token.
    void
    d_tests_tokens()
    {
        const m::expr_token by_default;
        const m::expr_token number(m::expr_token_kind::number, "0.1", 4);

        D_TESTS_EXPECT( (by_default.kind == m::expr_token_kind::end) &&
                        (by_default.text.empty()) &&
                        (by_default.offset == 0),
                        "a token made by default: the end, no text, 0");
        D_TESTS_EXPECT( (number.kind == m::expr_token_kind::number) &&
                        (number.text == "0.1") &&
                        (number.offset == 4),
                        "a token's three fields");
        D_TESTS_EXPECT( (number == m::expr_token(m::expr_token_kind::number,
                                                 "0.1",
                                                 4)) &&
                        (!(number != m::expr_token(m::expr_token_kind::number,
                                                   "0.1",
                                                   4))),
                        "tokens alike are equal");
        D_TESTS_EXPECT( (number != m::expr_token(m::expr_token_kind::name,
                                                 "0.1",
                                                 4)) &&
                        (number != m::expr_token(m::expr_token_kind::number,
                                                 "0.2",
                                                 4)) &&
                        (number != m::expr_token(m::expr_token_kind::number,
                                                 "0.1",
                                                 5)),
                        "tokens differing in any field are unequal");

        // the kinds, each its own
        D_TESTS_EXPECT( (static_cast<int>(m::expr_token_kind::number) == 1) &&
                        (static_cast<int>(m::expr_token_kind::end) == 8),
                        "the token kinds numbered 1 to 8");

        return;
    }

    // d_tests_tokenize
    //   expr_tokenize: numbers, names, symbols, brackets, round and square,
    // commas and semicolons, each at its offset; the spaces between; the
    // bars by the option; and no token for any other character.
    void
    d_tests_tokenize()
    {
        typedef m::expr_token      tok;
        const m::expr_token_kind   number = m::expr_token_kind::number;
        const m::expr_token_kind   name   = m::expr_token_kind::name;
        const m::expr_token_kind   symbol = m::expr_token_kind::symbol;
        const m::expr_token_kind   open   = m::expr_token_kind::open;
        const m::expr_token_kind   close  = m::expr_token_kind::close;
        m::expr_syntax             bars;

        bars.absolute_bars = true;

        D_TESTS_EXPECT( d_tests_tokens_are("x + 1",
                                           { tok(name, "x", 0),
                                             tok(symbol, "+", 2),
                                             tok(number, "1", 4) }),
                        "x + 1: a name, a symbol and a number, at 0, 2, 4");
        D_TESTS_EXPECT( d_tests_tokens_are("0.1 .5 5. 6.02e23 1e-3 007 1e+5",
                                           { tok(number, "0.1", 0),
                                             tok(number, ".5", 4),
                                             tok(number, "5.", 7),
                                             tok(number, "6.02e23", 10),
                                             tok(number, "1e-3", 18),
                                             tok(number, "007", 23),
                                             tok(number, "1e+5", 27) }),
                        "numbers by decimal.h's grammar, as written");
        D_TESTS_EXPECT( d_tests_tokens_are("1e 2x x2 _a1",
                                           { tok(number, "1", 0),
                                             tok(name, "e", 1),
                                             tok(number, "2", 3),
                                             tok(name, "x", 4),
                                             tok(name, "x2", 6),
                                             tok(name, "_a1", 9) }),
                        "a number ends where its grammar does; a name runs "
                        "on through digits and _");
        D_TESTS_EXPECT( d_tests_tokens_are("+-*/^!'=<>",
                                           { tok(symbol, "+", 0),
                                             tok(symbol, "-", 1),
                                             tok(symbol, "*", 2),
                                             tok(symbol, "/", 3),
                                             tok(symbol, "^", 4),
                                             tok(symbol, "!", 5),
                                             tok(symbol, "'", 6),
                                             tok(symbol, "=", 7),
                                             tok(symbol, "<", 8),
                                             tok(symbol, ">", 9) }),
                        "the ten symbols of one character");
        D_TESTS_EXPECT( d_tests_tokens_are("!=<=>===",
                                           { tok(symbol, "!=", 0),
                                             tok(symbol, "<=", 2),
                                             tok(symbol, ">=", 4),
                                             tok(symbol, "=", 6),
                                             tok(symbol, "=", 7) }),
                        "!= <= >= the longer first; == two symbols");
        D_TESTS_EXPECT( d_tests_tokens_are("(a)",
                                           { tok(m::expr_token_kind::open,
                                                 "(",
                                                 0),
                                             tok(name, "a", 1),
                                             tok(m::expr_token_kind::close,
                                                 ")",
                                                 2) }),
                        "the round brackets");
        D_TESTS_EXPECT( d_tests_tokens_are(" \t\n\v\f\rx",
                                           { tok(name, "x", 6) }) &&
                        d_tests_tokens_are("", {}) &&
                        d_tests_tokens_are("   ", {}),
                        "the six spaces between tokens; none in no text");
        D_TESTS_EXPECT( d_tests_tokens_are("|x|",
                                           { tok(symbol, "|", 0),
                                             tok(name, "x", 1),
                                             tok(symbol, "|", 2) },
                                           bars),
                        "| a symbol with the bars on");

        // no token
        D_TESTS_EXPECT( d_tests_is_error(m::expr_tokenize("|x|"),
                                         m::expr_status::bad_character,
                                         0,
                                         "no token begins with '|'"),
                        "| no token with the bars off");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_tokenize("1 + #"),
                                         m::expr_status::bad_character,
                                         4,
                                         "no token begins with '#'") &&
                        d_tests_is_error(m::expr_tokenize("a\x01"),
                                         m::expr_status::bad_character,
                                         1,
                                         "no token begins with the byte "
                                         "0x01") &&
                        d_tests_is_error(m::expr_tokenize("\xc3\xa9"),
                                         m::expr_status::bad_character,
                                         0,
                                         "no token begins with the byte "
                                         "0xC3"),
                        "bad_character at the first character no token "
                        "begins with, printable or not");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_tokenize("1.."),
                                         m::expr_status::bad_character,
                                         2,
                                         "no token begins with '.'"),
                        "a point before no digit begins no number");

        // the comma, step 15's
        D_TESTS_EXPECT( d_tests_tokens_are("f(x,y)",
                                           { tok(name, "f", 0),
                                             tok(m::expr_token_kind::open,
                                                 "(",
                                                 1),
                                             tok(name, "x", 2),
                                             tok(m::expr_token_kind::comma,
                                                 ",",
                                                 3),
                                             tok(name, "y", 4),
                                             tok(m::expr_token_kind::close,
                                                 ")",
                                                 5) }),
                        "the comma, at its place");

        // the square brackets and the semicolon, step 19's
        D_TESTS_EXPECT( d_tests_tokens_are(
                            "[1, 2; x]a]",
                            { tok(open, "[", 0),
                              tok(number, "1", 1),
                              tok(m::expr_token_kind::comma, ",", 2),
                              tok(number, "2", 4),
                              tok(m::expr_token_kind::semicolon, ";", 5),
                              tok(name, "x", 7),
                              tok(close, "]", 8),
                              tok(name, "a", 9),
                              tok(close, "]", 10) }),
                        "square brackets, open and close, and the "
                        "semicolon, at their places");

        return;
    }


    //==========================================================================
    // 5.  OPERATORS
    //==========================================================================

    // d_tests_operators
    //   expr_op_kind, expr_op and the operator table.
    void
    d_tests_operators()
    {
        // the kinds
        D_TESTS_EXPECT( (m::expr_op_kind_count == 25) &&
                        (static_cast<std::size_t>(m::expr_op_kind::row) ==
                             m::expr_op_kind_count - 1) &&
                        (static_cast<int>(m::expr_op_kind::none) == 0),
                        "25 kinds, none 0 and row the last");

        // the operator
        const m::expr_op none;
        const m::expr_op add = m::expr_op_kind::add;
        const m::expr_op sine = m::expr_op::call("sin");

        D_TESTS_EXPECT( (none.kind == m::expr_op_kind::none) &&
                        (none.function.empty()),
                        "an operator made by default: none");
        D_TESTS_EXPECT( (add.kind == m::expr_op_kind::add) &&
                        (add.function.empty()),
                        "an operator made from a kind, implicitly");
        D_TESTS_EXPECT( (sine.kind == m::expr_op_kind::call) &&
                        (sine.function == "sin"),
                        "a call: its kind and its function");
        D_TESTS_EXPECT( (add == m::expr_op(m::expr_op_kind::add)) &&
                        (sine == m::expr_op::call("sin")) &&
                        (!(sine != m::expr_op::call("sin"))),
                        "operators alike are equal");
        D_TESTS_EXPECT( (add != m::expr_op(m::expr_op_kind::sub)) &&
                        (sine != m::expr_op::call("cos")) &&
                        (sine != m::expr_op(m::expr_op_kind::call)),
                        "operators differing in kind or function are "
                        "unequal");

        return;
    }

    // d_tests_described_with
    //   whether the standard table describes a kind as given, each of its
    // three algebraic flags among it.
    bool
    d_tests_described_with(
        m::expr_op_kind   _kind,
        unsigned          _arity,
        int               _precedence,
        dj::associativity _assoc,
        dj::fixity        _fix,
        const char*       _spelling,
        bool              _commutative,
        bool              _associative,
        bool              _idempotent
    )
    {
        const dj::operator_descriptor* described =
            m::expr_signature::standard().describe(_kind);

        return ( (described != D_NULLPTR)                       &&
                 (described->arity == _arity)                   &&
                 (described->precedence == _precedence)         &&
                 (described->assoc == _assoc)                   &&
                 (described->fix == _fix)                       &&
                 (std::string(described->spelling) == _spelling) &&
                 (described->commutative == _commutative)       &&
                 (described->associative == _associative)       &&
                 (described->idempotent == _idempotent) );
    }

    // d_tests_described_as
    //   whether the standard table describes a kind as given: commutative
    // and associative both, or neither, where _algebraic says, and not
    // idempotent.
    bool
    d_tests_described_as(
        m::expr_op_kind   _kind,
        unsigned          _arity,
        int               _precedence,
        dj::associativity _assoc,
        dj::fixity        _fix,
        const char*       _spelling,
        bool              _algebraic
    )
    {
        return d_tests_described_with(_kind,
                                      _arity,
                                      _precedence,
                                      _assoc,
                                      _fix,
                                      _spelling,
                                      _algebraic,
                                      _algebraic,
                                      false);
    }

    // d_tests_table
    //   the operator table: G5's levels; the standard table's arithmetic,
    // relations and logic, and nothing else yet; define.
    void
    d_tests_table()
    {
        // G5's levels, loosest first, ten apart, logic below the relations
        // (S213)
        D_TESTS_EXPECT( (m::expr_precedence_or == 10) &&
                        (m::expr_precedence_and == 20) &&
                        (m::expr_precedence_not == 30) &&
                        (m::expr_precedence_relation == 40) &&
                        (m::expr_precedence_additive == 50) &&
                        (m::expr_precedence_multiplicative == 60) &&
                        (m::expr_precedence_unary == 70) &&
                        (m::expr_precedence_power == 80) &&
                        (m::expr_precedence_postfix == 90),
                        "G5's nine levels, loosest first, logic below the "
                        "relations");

        // the arithmetic, each as G5 has it
        D_TESTS_EXPECT( d_tests_described_as(m::expr_op_kind::add, 2,
                                             m::expr_precedence_additive,
                                             dj::DAssocLeft, dj::DFixInfix,
                                             "+", true) &&
                        d_tests_described_as(m::expr_op_kind::sub, 2,
                                             m::expr_precedence_additive,
                                             dj::DAssocLeft, dj::DFixInfix,
                                             "-", false),
                        "+ and -: infix, left, additive; + commutative "
                        "and associative");
        D_TESTS_EXPECT( d_tests_described_as(m::expr_op_kind::mul, 2,
                                             m::expr_precedence_multiplicative,
                                             dj::DAssocLeft, dj::DFixInfix,
                                             "*", true) &&
                        d_tests_described_as(m::expr_op_kind::div, 2,
                                             m::expr_precedence_multiplicative,
                                             dj::DAssocLeft, dj::DFixInfix,
                                             "/", false) &&
                        d_tests_described_as(m::expr_op_kind::mod, 2,
                                             m::expr_precedence_multiplicative,
                                             dj::DAssocLeft, dj::DFixInfix,
                                             "mod", false),
                        "* / mod: infix, left, multiplicative; * "
                        "commutative and associative");
        D_TESTS_EXPECT( d_tests_described_as(m::expr_op_kind::pow, 2,
                                             m::expr_precedence_power,
                                             dj::DAssocRight, dj::DFixInfix,
                                             "^", false),
                        "^: infix, right, the tightest of the binary");
        D_TESTS_EXPECT( d_tests_described_as(m::expr_op_kind::neg, 1,
                                             m::expr_precedence_unary,
                                             dj::DAssocRight, dj::DFixPrefix,
                                             "-", false) &&
                        d_tests_described_as(m::expr_op_kind::pos, 1,
                                             m::expr_precedence_unary,
                                             dj::DAssocRight, dj::DFixPrefix,
                                             "+", false),
                        "the unary - and +: prefix, below ^");
        D_TESTS_EXPECT( d_tests_described_as(m::expr_op_kind::factorial, 1,
                                             m::expr_precedence_postfix,
                                             dj::DAssocLeft, dj::DFixPostfix,
                                             "!", false) &&
                        d_tests_described_as(m::expr_op_kind::transpose, 1,
                                             m::expr_precedence_transpose,
                                             dj::DAssocLeft, dj::DFixPostfix,
                                             "'", false) &&
                        (m::expr_precedence_unary <
                             m::expr_precedence_transpose) &&
                        (m::expr_precedence_transpose <
                             m::expr_precedence_power) &&
                        (m::expr_precedence_index >
                             m::expr_precedence_postfix),
                        "!: postfix, left, above ^; ': postfix, left, "
                        "between the unary - and ^; indexing above them");

        // the relations and logic
        const int relation = m::expr_precedence_relation;

        D_TESTS_EXPECT( d_tests_described_with(m::expr_op_kind::eq, 2,
                                               relation, dj::DAssocNone,
                                               dj::DFixInfix, "=", true,
                                               false, false) &&
                        d_tests_described_with(m::expr_op_kind::ne, 2,
                                               relation, dj::DAssocNone,
                                               dj::DFixInfix, "!=", true,
                                               false, false) &&
                        d_tests_described_as(m::expr_op_kind::lt, 2,
                                             relation, dj::DAssocNone,
                                             dj::DFixInfix, "<", false) &&
                        d_tests_described_as(m::expr_op_kind::le, 2,
                                             relation, dj::DAssocNone,
                                             dj::DFixInfix, "<=", false) &&
                        d_tests_described_as(m::expr_op_kind::gt, 2,
                                             relation, dj::DAssocNone,
                                             dj::DFixInfix, ">", false) &&
                        d_tests_described_as(m::expr_op_kind::ge, 2,
                                             relation, dj::DAssocNone,
                                             dj::DFixInfix, ">=", false),
                        "= != < <= > >=: infix, non-associative, below "
                        "the arithmetic; = and != commutative");
        D_TESTS_EXPECT( d_tests_described_with(m::expr_op_kind::logical_or,
                                               2, m::expr_precedence_or,
                                               dj::DAssocLeft,
                                               dj::DFixInfix, "or", true,
                                               true, true) &&
                        d_tests_described_with(m::expr_op_kind::logical_and,
                                               2, m::expr_precedence_and,
                                               dj::DAssocLeft,
                                               dj::DFixInfix, "and", true,
                                               true, true) &&
                        d_tests_described_as(m::expr_op_kind::logical_not, 1,
                                             m::expr_precedence_not,
                                             dj::DAssocRight,
                                             dj::DFixPrefix, "not", false),
                        "or and and: infix, left, commutative, associative "
                        "and idempotent; not: prefix, above them");

        // nothing else yet
        const m::expr_signature& table     = m::expr_signature::standard();
        std::size_t              described = 0;

        for (std::size_t k = 0; k < m::expr_op_kind_count; ++k)
        {
            const m::expr_op op(static_cast<m::expr_op_kind>(k));

            // described, and has, agree
            if (table.describe(op) != D_NULLPTR)
            {
                described += ( table.has(op) ? 1 : 0 );
            }
        }

        D_TESTS_EXPECT( (described == 19) &&
                        (table.has(m::expr_op_kind::factorial)) &&
                        (table.has(m::expr_op_kind::eq)) &&
                        (table.has(m::expr_op_kind::logical_not)) &&
                        (table.has(m::expr_op_kind::transpose)) &&
                        (!table.has(m::expr_op_kind::none)) &&
                        (!table.has(m::expr_op_kind::index)) &&
                        (!table.has(m::expr_op_kind::row)),
                        "the standard table describes the eighteen kinds "
                        "of 2b to 2d and 3b's transpose, and no other: "
                        "indexing is the parser layer's");
        D_TESTS_EXPECT( (table.describe(m::expr_op::call("sin")) ==
                             D_NULLPTR) &&
                        (table.describe(m::expr_op(static_cast<m::expr_op_kind>(
                             m::expr_op_kind_count))) == D_NULLPTR),
                        "nor any call, which the parser layer reads, nor a "
                        "kind out of range");
        D_TESTS_EXPECT( (&m::expr_signature::standard() == &table),
                        "the standard table is one, built once");

        // define, anew, and out of range
        m::expr_signature own;
        const dj::operator_descriptor first(2, 5, dj::DAssocLeft,
                                            dj::DFixInfix, "&");
        const dj::operator_descriptor second(2, 6, dj::DAssocRight,
                                             dj::DFixInfix, "&&");

        D_TESTS_EXPECT( (!own.has(m::expr_op_kind::add)) &&
                        (&own.define(m::expr_op_kind::add, first) == &own) &&
                        (own.has(m::expr_op_kind::add)) &&
                        (own.describe(m::expr_op_kind::add)->precedence ==
                             5),
                        "define describes a kind, and returns the table");
        own.define(m::expr_op_kind::add, second)
           .define(static_cast<m::expr_op_kind>(m::expr_op_kind_count),
                   first);
        D_TESTS_EXPECT( (own.has(m::expr_op_kind::add)) &&
                        (own.describe(m::expr_op_kind::add)->precedence ==
                             6) &&
                        (std::string(own.describe(m::expr_op_kind::add)
                                         ->spelling) == "&&") &&
                        (!own.has(m::expr_op_kind::sub)) &&
                        (!own.has(m::expr_op(static_cast<m::expr_op_kind>(
                             m::expr_op_kind_count)))),
                        "define describes a kind anew, and leaves a kind "
                        "out of range undescribed");

        return;
    }


    //==========================================================================
    // 6.  ATOMS AND TERMS
    //==========================================================================

    // d_tests_atoms
    //   expr_atom: its fields, and when two atoms are equal.
    void
    d_tests_atoms()
    {
        const m::expr_atom by_default;

        D_TESTS_EXPECT( (by_default.kind == m::expr_atom_kind::number) &&
                        (by_default.number.kind() == D_MATH_VALUE_NONE) &&
                        (by_default.text.empty()),
                        "an atom made by default: a number of none, no "
                        "text");

        const m::expr_atom one(m::expr_atom_kind::number, m::value(1), "1");
        const m::expr_atom x(m::expr_atom_kind::variable, m::value(), "x");

        D_TESTS_EXPECT( (one.kind == m::expr_atom_kind::number) &&
                        (one.number == m::value(1)) &&
                        (one.text == "1"),
                        "an atom's three fields");

        // equal: kinds, texts and numbers alike
        D_TESTS_EXPECT( (one == m::expr_atom(m::expr_atom_kind::number,
                                             m::value(1),
                                             "1")) &&
                        (!(one != m::expr_atom(m::expr_atom_kind::number,
                                               m::value(1),
                                               "1"))) &&
                        (x == m::expr_atom(m::expr_atom_kind::variable,
                                           m::value(),
                                           "x")),
                        "atoms alike are equal");

        // unequal: a kind, a text or a number apart
        D_TESTS_EXPECT( (x != m::expr_atom(m::expr_atom_kind::constant,
                                           m::value(),
                                           "x")) &&
                        (x != m::expr_atom(m::expr_atom_kind::variable,
                                           m::value(),
                                           "y")),
                        "atoms of other kinds or names are unequal");
        D_TESTS_EXPECT( (one != m::expr_atom(m::expr_atom_kind::number,
                                             m::value(1),
                                             "1.0")) &&
                        (one != m::expr_atom(m::expr_atom_kind::number,
                                             m::value(2),
                                             "1")),
                        "numbers of other texts or values are unequal");

        // the same number: of one kind, then equal, or two NaNs
        const double nan  = std::numeric_limits<double>::quiet_NaN();
        const double zero = 0.0;

        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(1),
                                      "") !=
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(1.0),
                                          "")),
                        "1 and 1.0 are two numbers: two kinds");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value::ratio(1, 3),
                                      "") ==
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value::ratio(2, 6),
                                          "")),
                        "1/3 and 2/6 are one rational");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(zero),
                                      "") ==
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(-zero),
                                          "")),
                        "0.0 and -0.0 are the same number");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(nan),
                                      "") ==
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(nan),
                                          "")),
                        "two NaNs of one type are the same number");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(static_cast<float>(nan)),
                                      "") !=
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(nan),
                                          "")),
                        "NaNs of two types are not");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(D_MATH_VALUE_ERROR_POLE),
                                      "") ==
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(D_MATH_VALUE_ERROR_POLE),
                                          "")) &&
                        (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(D_MATH_VALUE_ERROR_POLE),
                                      "") !=
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(
                                              D_MATH_VALUE_ERROR_DOMAIN),
                                          "")),
                        "errors are the same number by their codes");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(true),
                                      "") !=
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(false),
                                          "")) &&
                        (by_default == m::expr_atom()),
                        "booleans by their truth; none is none");

        return;
    }

    // d_tests_terms
    //   the term: its constructors and observers, and parse/expression's
    // queries and transforms over it.
    void
    d_tests_terms()
    {
        D_STATIC_ASSERT( (dj::is_expression<m::expr_term>::value),
                         "expr_term is a parse/expression term");
        D_STATIC_ASSERT( (std::is_same<m::expr_term,
                                       dj::expression<m::expr_op,
                                                      m::expr_atom> >::value),
                         "expr_term is expression<expr_op, expr_atom>");
        D_STATIC_ASSERT( (std::is_default_constructible<m::expr_op>::value) &&
                         (std::is_default_constructible<m::expr_atom>::value),
                         "the term's two types made by default, as "
                         "parse/expression asks");

        // the leaves
        const m::expr_term made = m::expr_number(m::value::ratio(1, 2));
        const m::expr_term half = m::expr_number(m::value::ratio(1, 2),
                                                 "0.5");
        const m::expr_term x    = m::expr_variable("x");
        const m::expr_term pi   = m::expr_constant("pi");

        D_TESTS_EXPECT( (m::expr_is_number(made)) &&
                        (m::expr_atom_of(made).text.empty()) &&
                        (m::expr_atom_of(made).number ==
                             m::value::ratio(1, 2)),
                        "a number a fold made: its value, no literal");
        D_TESTS_EXPECT( (m::expr_is_number(half)) &&
                        (m::expr_atom_of(half).text == "0.5") &&
                        (m::expr_atom_of(half).number ==
                             m::value::ratio(1, 2)),
                        "a number as written: its value and its literal");
        D_TESTS_EXPECT( (m::expr_is_variable(x)) &&
                        (!m::expr_is_number(x)) &&
                        (!m::expr_is_constant(x)) &&
                        (!m::expr_is_application(x)) &&
                        (m::expr_atom_of(x).text == "x") &&
                        (m::expr_atom_of(x).number.kind() ==
                             D_MATH_VALUE_NONE),
                        "a variable, by name");
        D_TESTS_EXPECT( (m::expr_is_constant(pi)) &&
                        (!m::expr_is_variable(pi)) &&
                        (m::expr_atom_of(pi).text == "pi"),
                        "a named constant, by name");

        // the applications, of 0, 1, 2 and 3 operands, and a call
        const m::expr_term none = m::expr_application(m::expr_op_kind::vector);
        const m::expr_term neg  = m::expr_application(m::expr_op_kind::neg, x);
        const m::expr_term sum  = d_tests_x_plus_1();
        const m::expr_term row  =
            m::expr_application(m::expr_op_kind::row,
                                std::vector<m::expr_term>{ x, half, pi });
        const m::expr_term call =
            m::expr_call("atan2", std::vector<m::expr_term>{ x, sum });

        D_TESTS_EXPECT( (m::expr_is_application(none)) &&
                        (m::expr_op_of(none) == m::expr_op_kind::vector) &&
                        (m::expr_operands_of(none).empty()),
                        "an application of no operand");
        D_TESTS_EXPECT( (m::expr_op_of(neg) == m::expr_op_kind::neg) &&
                        (m::expr_operands_of(neg).size() == 1) &&
                        (d_tests_same(m::expr_operands_of(neg)[0], x)),
                        "an application of one");
        D_TESTS_EXPECT( (m::expr_op_of(sum) == m::expr_op_kind::add) &&
                        (m::expr_operands_of(sum).size() == 2) &&
                        (m::expr_is_variable(m::expr_operands_of(sum)[0])) &&
                        (m::expr_is_number(m::expr_operands_of(sum)[1])),
                        "an application of two, in order");
        D_TESTS_EXPECT( (m::expr_op_of(row) == m::expr_op_kind::row) &&
                        (m::expr_operands_of(row).size() == 3) &&
                        (d_tests_same(m::expr_operands_of(row)[2], pi)),
                        "an application of three");
        D_TESTS_EXPECT( (m::expr_op_of(call) == m::expr_op::call("atan2")) &&
                        (m::expr_operands_of(call).size() == 2) &&
                        (d_tests_same(m::expr_operands_of(call)[1], sum)),
                        "a call: its function's name, its arguments");
        D_TESTS_EXPECT( (!m::expr_is_number(call)) &&
                        (!m::expr_is_variable(call)) &&
                        (!m::expr_is_constant(call)),
                        "an application is no leaf");

        // parse/expression over the term
        D_TESTS_EXPECT( (dj::size<m::expr_op, m::expr_atom>(call) == 5) &&
                        (dj::depth<m::expr_op, m::expr_atom>(call) == 3),
                        "size and depth");
        D_TESTS_EXPECT( (d_tests_same(call,
                                      m::expr_call("atan2",
                                                   std::vector<m::expr_term>{
                                                       m::expr_variable("x"),
                                                       d_tests_x_plus_1() }))),
                        "structural equality of terms alike");
        D_TESTS_EXPECT( (!d_tests_same(call,
                                       m::expr_call("atan", std::vector<
                                           m::expr_term>{ x, sum }))) &&
                        (!d_tests_same(sum,
                                       m::expr_application(
                                           m::expr_op_kind::add,
                                           x,
                                           m::expr_number(m::value(1),
                                                          "1.0")))),
                        "and of terms a function or a literal apart");
        D_TESTS_EXPECT( (dj::contains_operator<m::expr_op, m::expr_atom>(
                             call, m::expr_op_kind::add)) &&
                        (!dj::contains_operator<m::expr_op, m::expr_atom>(
                             call, m::expr_op_kind::mul)),
                        "contains_operator");

        const m::expr_term moved =
            dj::substitute<m::expr_op, m::expr_atom>(
                sum,
                [](const m::expr_atom& _atom) -> m::expr_term
                {
                    return (_atom == m::expr_atom(m::expr_atom_kind::variable,
                                                  m::value(),
                                                  "x"))
                        ? m::expr_constant("pi")
                        : dj::expr_leaf<m::expr_op, m::expr_atom>(_atom);
                });

        D_TESTS_EXPECT( (d_tests_same(moved,
                                      m::expr_application(
                                          m::expr_op_kind::add,
                                          m::expr_constant("pi"),
                                          m::expr_number(m::value(1),
                                                         "1")))),
                        "parse/expression's substitute");

        return;
    }


    //==========================================================================
    // 7.  TABLES
    //==========================================================================

    // d_tests_standard_function
    //   whether the standard function table has _name, of _arity arguments,
    // with its seven evaluators, a complex type's among them from chunk 41
    // (S339), and no rule of its derivatives (4a's).
    bool
    d_tests_standard_function(
        const char* _name,
        std::size_t _arity
    )
    {
        const m::expr_function* function =
            m::expr_standard_functions().find(_name);

        return ( (function != D_NULLPTR)                       &&
                 (function->name == _name)                     &&
                 (function->min_arity == _arity)               &&
                 (function->max_arity == _arity)               &&
                 (function->evaluate_float != D_NULLPTR)       &&
                 (function->evaluate_double != D_NULLPTR)      &&
                 (function->evaluate_long_double != D_NULLPTR) &&
                 (function->evaluate_complex_float != D_NULLPTR)  &&
                 (function->evaluate_complex_double != D_NULLPTR) &&
                 (function->evaluate_complex_long_double !=
                      D_NULLPTR)                               &&
                 (function->evaluate_value != D_NULLPTR)       &&
                 (function->partial == D_NULLPTR) );
    }

    // d_tests_standard_variadic
    //   whether the standard function table has _name, of any number of
    // arguments from _least, with an evaluator for each type and none for
    // its derivatives.
    bool
    d_tests_standard_variadic(
        const char* _name,
        std::size_t _least
    )
    {
        const m::expr_function* function =
            m::expr_standard_functions().find(_name);

        return ( (function != D_NULLPTR)                       &&
                 (function->name == _name)                     &&
                 (function->min_arity == _least)               &&
                 (function->max_arity == m::expr_variadic)     &&
                 (function->evaluate_float != D_NULLPTR)       &&
                 (function->evaluate_double != D_NULLPTR)      &&
                 (function->evaluate_long_double != D_NULLPTR) &&
                 (function->evaluate_complex_float != D_NULLPTR)  &&
                 (function->evaluate_complex_double != D_NULLPTR) &&
                 (function->evaluate_complex_long_double !=
                      D_NULLPTR)                               &&
                 (function->evaluate_value != D_NULLPTR)       &&
                 (function->partial == D_NULLPTR) );
    }

    // d_tests_standard_boolean
    //   whether the standard constant table has _name, 1 or 0 in each type
    // as _truth is, and as a value the boolean _truth.
    bool
    d_tests_standard_boolean(
        const char* _name,
        bool        _truth
    )
    {
        const m::expr_named_constant* constant =
            m::expr_standard_constants().find(_name);
        bool                          truth    = !_truth;

        return ( (constant != D_NULLPTR) &&
                 (constant->as_float == (_truth ? 1.0f : 0.0f)) &&
                 (constant->as_double == (_truth ? 1.0 : 0.0)) &&
                 (constant->as_long_double == (_truth ? 1.0L : 0.0L)) &&
                 (constant->as_value.kind() == D_MATH_VALUE_BOOLEAN) &&
                 (constant->as_value.to(truth)) &&
                 (truth == _truth) );
    }

    // d_tests_standard_constant
    //   whether the standard constant table has _name, of the values given
    // in each type, and as a value the long double, of its kind.
    bool
    d_tests_standard_constant(
        const char* _name,
        float       _as_float,
        double      _as_double,
        long double _as_long_double
    )
    {
        const m::expr_named_constant* constant =
            m::expr_standard_constants().find(_name);

        return ( (constant != D_NULLPTR) &&
                 (d_tests_same_real(constant->as_float, _as_float)) &&
                 (d_tests_same_real(constant->as_double, _as_double)) &&
                 (d_tests_same_real(constant->as_long_double,
                                    _as_long_double)) &&
                 (constant->as_value.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                 (constant->as_value == m::value(_as_long_double)) );
    }

    // d_tests_tables
    //   expr_function, expr_named_constant and expr_table; the standard
    // tables are d_tests_standard_functions' and
    // d_tests_standard_constants'.
    void
    d_tests_tables()
    {
        // a function
        const m::expr_function by_default;
        const m::expr_function two_args("atan2", 2, 2);
        const m::expr_function variadic("max", 1, m::expr_variadic);

        D_TESTS_EXPECT( (by_default.name.empty()) &&
                        (by_default.min_arity == 0) &&
                        (by_default.max_arity == 0) &&
                        (by_default.evaluate_float == D_NULLPTR) &&
                        (by_default.evaluate_double == D_NULLPTR) &&
                        (by_default.evaluate_long_double == D_NULLPTR) &&
                        (by_default.evaluate_value == D_NULLPTR) &&
                        (by_default.partial == D_NULLPTR),
                        "a function made by default: no name, no "
                        "argument, every slot null");
        D_TESTS_EXPECT( (two_args.name == "atan2") &&
                        (two_args.min_arity == 2) &&
                        (two_args.max_arity == 2) &&
                        (two_args.evaluate_double == D_NULLPTR) &&
                        (two_args.partial == D_NULLPTR),
                        "a function's name and arity, its slots null");
        D_TESTS_EXPECT( (!two_args.takes(1)) &&
                        (two_args.takes(2)) &&
                        (!two_args.takes(3)),
                        "takes: its arity alone");
        D_TESTS_EXPECT( (!variadic.takes(0)) &&
                        (variadic.takes(1)) &&
                        (variadic.takes(1000)) &&
                        (variadic.takes(m::expr_variadic)),
                        "takes: any number from the least, where variadic");

        // a named constant
        const m::expr_named_constant zero;
        const m::expr_named_constant half("half",
                                          0.5f,
                                          0.5,
                                          0.5L,
                                          m::value::ratio(1, 2));

        D_TESTS_EXPECT( (zero.name.empty()) &&
                        (zero.as_float == 0.0f) &&
                        (zero.as_double == 0.0) &&
                        (zero.as_long_double == 0.0L) &&
                        (zero.as_value.kind() == D_MATH_VALUE_NONE),
                        "a constant made by default: no name, 0, none");
        D_TESTS_EXPECT( (half.name == "half") &&
                        (half.as_float == 0.5f) &&
                        (half.as_double == 0.5) &&
                        (half.as_long_double == 0.5L) &&
                        (half.as_value == m::value::ratio(1, 2)),
                        "a constant's name and values");

        // a table: defined, found, replaced, in order
        m::expr_function_table table;

        D_TESTS_EXPECT( (table.empty()) &&
                        (table.size() == 0) &&
                        (table.find("atan2") == D_NULLPTR),
                        "a table made by default: empty");

        table.define(two_args).define(variadic);

        D_TESTS_EXPECT( (table.size() == 2) &&
                        (table.find("atan2") != D_NULLPTR) &&
                        (table.find("atan2")->max_arity == 2) &&
                        (table.find("max") != D_NULLPTR) &&
                        (table.find("min") == D_NULLPTR),
                        "define adds, find finds by name");

        table.define(m::expr_function("atan2", 1, 2));

        D_TESTS_EXPECT( (table.size() == 2) &&
                        (table.find("atan2") != D_NULLPTR) &&
                        (table.find("atan2")->min_arity == 1),
                        "define puts an entry in place of its name's");

        m::expr_constant_table constants;
        constants.define(half);

        D_TESTS_EXPECT( (constants.size() == 1) &&
                        (constants.find("half") != D_NULLPTR) &&
                        (constants.find("half")->as_double == 0.5),
                        "the constant table");

        constants.define(m::expr_named_constant("half",
                                                0.5f,
                                                0.5,
                                                0.5L,
                                                m::value(0.5)));

        D_TESTS_EXPECT( (constants.size() == 1) &&
                        (constants.find("half") != D_NULLPTR) &&
                        (constants.find("half")->as_value.kind() ==
                             D_MATH_VALUE_DOUBLE),
                        "a constant put in place of its name's");

        return;
    }

    // d_tests_standard_functions
    //   the standard function table: the functions of steps 15 to 24, each
    // with its evaluators, and no other.
    void
    d_tests_standard_functions()
    {
        // the standard functions: the C core's 25, abs and factorial
        static const char* const ones[] = { "sqrt", "cbrt", "sin", "cos",
                                            "tan", "asin", "acos", "atan",
                                            "sinh", "cosh", "tanh", "exp",
                                            "exp2", "log", "log2", "log10",
                                            "floor", "ceil", "round",
                                            "trunc", "abs", "factorial" };
        static const char* const twos[] = { "atan2", "pow", "hypot", "fmod",
                                            "remainder" };
        std::size_t              found  = 0;

        for (std::size_t i = 0; i < 22; ++i)
        {
            found += ( d_tests_standard_function(ones[i], 1) ? 1 : 0 );
        }

        for (std::size_t i = 0; i < 5; ++i)
        {
            found += ( d_tests_standard_function(twos[i], 2) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (found == 27),
                        "the C core's 25 functions, abs and factorial, "
                        "each of its arity and its evaluators, no rule of "
                        "derivatives before 4a");
        D_TESTS_EXPECT( (d_tests_standard_variadic("piecewise", 1)) &&
                        (d_tests_standard_variadic("all_of", 1)) &&
                        (d_tests_standard_variadic("any_of", 1)),
                        "piecewise, all_of and any_of, each of any number "
                        "of arguments from 1");
        D_TESTS_EXPECT( (d_tests_standard_function("det", 1)) &&
                        (d_tests_standard_function("inv", 1)) &&
                        (d_tests_standard_function("trace", 1)) &&
                        (d_tests_standard_function("norm", 1)) &&
                        (d_tests_standard_function("eig", 1)) &&
                        (d_tests_standard_function("dot", 2)) &&
                        (d_tests_standard_function("cross", 2)) &&
                        (d_tests_standard_function("solve", 2)),
                        "linear algebra's det, inv, trace, norm and eig of "
                        "one argument, dot, cross and solve of two, each "
                        "with its four evaluators");
        D_TESTS_EXPECT( (d_tests_standard_function("complex", 2)) &&
                        (d_tests_standard_function("real", 1)) &&
                        (d_tests_standard_function("imag", 1)) &&
                        (d_tests_standard_function("conj", 1)) &&
                        (d_tests_standard_function("arg", 1)),
                        "the complex numbers' complex of two arguments, "
                        "real, imag, conj and arg of one, each with its seven "
                        "evaluators");
        D_TESTS_EXPECT( (d_tests_standard_function("log1p", 1)) &&
                        (d_tests_standard_function("expm1", 1)) &&
                        (d_tests_standard_function("asinh", 1)) &&
                        (d_tests_standard_function("acosh", 1)) &&
                        (d_tests_standard_function("atanh", 1)) &&
                        (d_tests_standard_function("sign", 1)) &&
                        (d_tests_standard_variadic("min", 1)) &&
                        (d_tests_standard_variadic("max", 1)) &&
                        (d_tests_standard_function("transpose", 1)),
                        "log1p, expm1, asinh, acosh, atanh and sign of one "
                        "argument, min and max of any number from 1, each "
                        "with its seven evaluators (S203, S338), and "
                        "transpose (S350)");
        D_TESTS_EXPECT( (d_tests_standard_function("quat", 4)) &&
                        (d_tests_standard_function("parts", 1)) &&
                        (d_tests_standard_function("rotate", 2)) &&
                        (d_tests_standard_function("rotmat", 1)) &&
                        (d_tests_standard_function("rotquat", 1)) &&
                        (d_tests_standard_function("slerp", 3)) &&
                        (m::expr_standard_functions().size() == 58),
                        "the quaternions' quat of four arguments, parts of "
                        "one and rotate of two (S275), rotmat and rotquat "
                        "of one and slerp of three (S391), each with its "
                        "seven evaluators: 58 functions");
        D_TESTS_EXPECT( (m::expr_standard_functions().find("ln") ==
                             D_NULLPTR) &&
                        (m::expr_standard_functions().find("gamma") ==
                             D_NULLPTR) &&
                        (m::expr_standard_functions().find("erf") ==
                             D_NULLPTR),
                        "and no other: no ln, no gamma, no erf");

        return;
    }

    // d_tests_standard_constants
    //   the standard constant table: pi, e, true and false, im, jm and km;
    // and each standard table one, built once.
    void
    d_tests_standard_constants()
    {
        // the standard constants: pi and e, as constants::of<T> has them,
        // and the booleans
        D_TESTS_EXPECT( (d_tests_standard_constant(
                             "pi",
                             m::constants::of<float>::pi(),
                             m::constants::of<double>::pi(),
                             m::constants::of<long double>::pi())) &&
                        (d_tests_standard_constant(
                             "e",
                             m::constants::of<float>::e(),
                             m::constants::of<double>::e(),
                             m::constants::of<long double>::e())),
                        "the standard constants: pi and e, constants::of<T>'s "
                        "in each type, the long double as a value");
        D_TESTS_EXPECT( (d_tests_standard_boolean("true", true)) &&
                        (d_tests_standard_boolean("false", false)),
                        "true and false: 1 and 0 in each type, the "
                        "booleans as values");

        // im, from step 23 (3e), the imaginary unit, a NaN in each type;
        // jm and km, from step 24 (3f), the quaternions' units, NaNs in
        // each type (d_tests_units)
        const m::expr_named_constant* const units[3] = {
            m::expr_standard_constants().find("im"),
            m::expr_standard_constants().find("jm"),
            m::expr_standard_constants().find("km") };

        D_TESTS_EXPECT( (units[0] != D_NULLPTR) &&
                        (units[1] != D_NULLPTR) &&
                        (units[2] != D_NULLPTR) &&
                        (units[0]->not_yet.empty()) &&
                        (m::expr_is_complex(units[0]->as_value)) &&
                        (units[0]->as_value.is_exact()) &&
                        (units[0]->as_value ==
                             m::expr_complex(m::value(0), m::value(1))) &&
                        (units[0]->as_float != units[0]->as_float) &&
                        (units[0]->as_double != units[0]->as_double) &&
                        (units[0]->as_long_double !=
                             units[0]->as_long_double) &&
                        (units[1]->not_yet.empty()) &&
                        (units[2]->not_yet.empty()) &&
                        (m::expr_is_quaternion(units[1]->as_value)) &&
                        (units[1]->as_value.is_exact()) &&
                        (units[1]->as_value ==
                             m::expr_quaternion(m::value(0), m::value(0),
                                                m::value(1), m::value(0))) &&
                        (units[2]->as_value ==
                             m::expr_quaternion(m::value(0), m::value(0),
                                                m::value(0), m::value(1))) &&
                        (units[1]->as_double != units[1]->as_double) &&
                        (units[2]->as_long_double !=
                             units[2]->as_long_double) &&
                        (m::expr_standard_constants().find("pi")->not_yet
                             .empty()) &&
                        (m::expr_standard_constants().size() == 7),
                        "im the exact unit, a NaN in each type; jm and km "
                        "the exact units of the quaternions, NaNs in each "
                        "type; 7 constants");
        D_TESTS_EXPECT( (&m::expr_standard_functions() ==
                             &m::expr_standard_functions()) &&
                        (&m::expr_standard_constants() ==
                             &m::expr_standard_constants()),
                        "each standard table is one, built once");

        return;
    }


    //==========================================================================
    // 8.  FOLDS
    //==========================================================================

    // d_tests_bindings
    //   expr_bindings, at, and expr_context.
    void
    d_tests_bindings()
    {
        const m::expr_bindings none;

        D_TESTS_EXPECT( (none.empty()) &&
                        (none.size() == 0) &&
                        (none.find("x") == D_NULLPTR),
                        "bindings made by default: none");

        m::expr_bindings two = m::at("x", 2.0).at("n", 3);

        D_TESTS_EXPECT( (two.size() == 2) &&
                        (two.find("x") != D_NULLPTR) &&
                        (*two.find("x") == m::value(2.0)) &&
                        (two.find("x")->kind() == D_MATH_VALUE_DOUBLE) &&
                        (two.find("n") != D_NULLPTR) &&
                        (*two.find("n") == m::value(3)) &&
                        (two.find("y") == D_NULLPTR),
                        "at binds, and chains");

        two.at("x", m::value::ratio(1, 3));

        D_TESTS_EXPECT( (two.size() == 2) &&
                        (two.find("x") != D_NULLPTR) &&
                        (*two.find("x") == m::value::ratio(1, 3)),
                        "at binds a bound name anew");

        const m::expr_context context;
        static_cast<void>(context);

        D_TESTS_EXPECT( (std::is_default_constructible<
                             m::expr_context>::value),
                        "a context made by default");

        return;
    }

    // d_tests_each_type
    //   whether each operation, and each way a leaf has its value, gives in
    // Type what it gives in double: - and the unary +, mod with and without
    // its corrections, a NaN for a value with no nearest, a literal no
    // decimal, and no value for a term made by default.
    template<typename Type>
    bool
    d_tests_each_type()
    {
        const Type nan = std::numeric_limits<Type>::quiet_NaN();
        const Type one = Type(1);
        const Type six = Type(6);

        return ( (d_tests_typed<Type>("7 - 2", Type(5)))                     &&
                 (d_tests_typed<Type>("+x", Type(3), m::at("x", 3)))         &&
                 (d_tests_typed<Type>("-7 mod 3", Type(2)))                  &&
                 (d_tests_typed<Type>("6 mod -3", -Type(0)))                 &&
                 (d_tests_typed<Type>("x", one, m::at("x", true)))           &&
                 (d_tests_typed<Type>("x", nan, m::at("x", m::value())))     &&
                 (m::expr_evaluate<Type>(m::expr_number(
                      m::value::ratio(1, 6), "1/6")).is_ok())                &&
                 (d_tests_same_real(
                      m::expr_evaluate<Type>(m::expr_number(
                          m::value::ratio(1, 6), "1/6")).value(),
                      one / six))                                            &&
                 (d_tests_is_error(m::expr_evaluate<Type>(m::expr_term()),
                                   m::expr_status::missing_operand,
                                   m::expr_no_offset,
                                   "a term made by default has no value")) );
    }

    // d_tests_fold_typed
    //   expr_evaluate<Type>: each literal rounded once; + - * / the type's;
    // ^ the C core's pow; mod floored; a variable its value's nearest.
    void
    d_tests_fold_typed()
    {
        const m::expr_bindings x3 = m::at("x", 3);
        volatile float         f1 = 1.0f;
        volatile float         f3 = 3.0f;
        volatile double        d1 = 1.0;
        volatile double        d3 = 3.0;
        volatile long double   l1 = 1.0L;
        volatile long double   l3 = 3.0L;
        volatile double        a  = 0.1;
        volatile double        b  = 0.2;

        D_TESTS_EXPECT( d_tests_typed<float>("2 * x ^ 2 + 1", 19.0f, x3) &&
                        d_tests_typed<double>("2 * x ^ 2 + 1", 19.0, x3) &&
                        d_tests_typed<long double>("2 * x ^ 2 + 1",
                                                   19.0L,
                                                   x3),
                        "2 * x ^ 2 + 1 at x = 3: 19 in each type");
        D_TESTS_EXPECT( d_tests_typed<float>("1 / 3", f1 / f3) &&
                        d_tests_typed<double>("1 / 3", d1 / d3) &&
                        d_tests_typed<long double>("1 / 3", l1 / l3),
                        "1 / 3: the type's own quotient");
        D_TESTS_EXPECT( d_tests_typed<double>("0.1 + 0.2", a + b),
                        "0.1 + 0.2 in double: each literal rounded, then "
                        "the sum");
        D_TESTS_EXPECT( d_tests_typed<float>(
                            "1.00000005960464477539062500000001",
                            1.0f + FLT_EPSILON),
                        "a literal rounded once to float, not through "
                        "double");
        D_TESTS_EXPECT( d_tests_typed<double>("7 mod 3", 1.0) &&
                        d_tests_typed<double>("-7 mod 3", 2.0) &&
                        d_tests_typed<double>("7 mod -3", -2.0) &&
                        d_tests_typed<double>("-7 mod -3", -1.0) &&
                        d_tests_typed<float>("7.5 mod 2", 1.5f) &&
                        d_tests_typed<long double>("-0.5 mod 1", 0.5L),
                        "mod floored: the divisor's sign");
        D_TESTS_EXPECT( d_tests_typed<double>("6 mod 3", 0.0) &&
                        d_tests_typed<double>("6 mod -3", -0.0) &&
                        d_tests_typed<double>(
                            "5 mod 0",
                            std::numeric_limits<double>::quiet_NaN()),
                        "mod: no remainder 0 with the divisor's sign; by 0 a "
                        "NaN");
        D_TESTS_EXPECT( d_tests_typed<double>(
                            "x mod y",
                            std::numeric_limits<double>::infinity(),
                            m::at("x", -1).at(
                                "y",
                                std::numeric_limits<double>::infinity())),
                        "mod: fmod plus the divisor, as Python's: -1 mod inf "
                        "is inf");

        volatile double two  = 2.0;
        volatile double half = 0.5;

        D_TESTS_EXPECT( d_tests_typed<double>("2 ^ 10", 1024.0) &&
                        d_tests_typed<double>("2 ^ -2", 0.25) &&
                        d_tests_typed<double>("0 ^ 0", 1.0) &&
                        d_tests_typed<double>("2 ^ 0.5",
                                              d_math_pow(two, half)),
                        "^ the C core's pow");
        D_TESTS_EXPECT( d_tests_typed<double>(
                            "(-8) ^ (1 / 3)",
                            std::numeric_limits<double>::quiet_NaN()) &&
                        d_tests_typed<double>(
                            "0 ^ -1",
                            std::numeric_limits<double>::infinity()) &&
                        d_tests_typed<double>(
                            "1 / 0",
                            std::numeric_limits<double>::infinity()) &&
                        d_tests_typed<double>(
                            "0 / 0",
                            std::numeric_limits<double>::quiet_NaN()),
                        "the type's IEEE answers where arithmetic fails");
        D_TESTS_EXPECT( d_tests_typed<double>("-x", -0.0, m::at("x", 0)) &&
                        d_tests_typed<double>("+x", 3.0, x3) &&
                        d_tests_typed<double>("--x", 3.0, x3),
                        "the unary - and +");
        D_TESTS_EXPECT( d_tests_typed<double>("x",
                                              d1 / d3,
                                              m::at("x",
                                                    m::value::ratio(1, 3))) &&
                        d_tests_typed<float>("x", 0.1f, m::at("x", 0.1)),
                        "a variable: its value's nearest in the type");
        D_TESTS_EXPECT( d_tests_typed<double>(
                            "x + 1",
                            std::numeric_limits<double>::quiet_NaN(),
                            m::at("x", D_MATH_VALUE_ERROR_POLE)) &&
                        d_tests_typed<double>(
                            "x + 1",
                            std::numeric_limits<double>::quiet_NaN(),
                            m::at("x", m::value())),
                        "a value with no nearest, an error or none: a NaN");
        D_TESTS_EXPECT( d_tests_typed<double>("x + 1", 2.0, m::at("x", true)) &&
                        d_tests_typed<float>("x + 1",
                                             1.0f,
                                             m::at("x", false)) &&
                        d_tests_typed<double>("x",
                                              0.0,
                                              m::at("x", false)) &&
                        d_tests_typed<long double>(
                            "x",
                            1.0L,
                            m::at("x", m::value(true))),
                        "a boolean: 1 or 0, as true and false are (2d)");

        // a number a fold made, and one whose literal is no decimal
        const m::expr_term third = m::expr_number(m::value::ratio(1, 3));
        const m::expr_term odd   = m::expr_number(m::value::ratio(1, 3),
                                                  "1/3");
        const m::expr_result<double> got_third =
            m::expr_evaluate<double>(third);
        const m::expr_result<double> got_odd = m::expr_evaluate<double>(odd);

        D_TESTS_EXPECT( (got_third.is_ok()) &&
                        (d_tests_same_real(got_third.value(), d1 / d3)) &&
                        (got_odd.is_ok()) &&
                        (d_tests_same_real(got_odd.value(), d1 / d3)),
                        "a number with no decimal literal: its value's "
                        "nearest");
        D_TESTS_EXPECT( (d_tests_each_type<float>()) &&
                        (d_tests_each_type<double>()) &&
                        (d_tests_each_type<long double>()),
                        "each operation, and each leaf's way, in each "
                        "type");

        return;
    }

    // d_tests_fold_value
    //   expr_evaluate to a value: exact where the value layer is; ^ exact
    // for whole exponents and for rational ones of whole roots; mod exact;
    // the tower's kind otherwise.
    void
    d_tests_fold_value()
    {
        const m::value third = m::value::ratio(1, 3);

        D_TESTS_EXPECT( d_tests_valued("1/3 + 1/6", m::value::ratio(1, 2)) &&
                        d_tests_valued("0.1 + 0.2", m::value::ratio(3, 10)),
                        "1/3 + 1/6 exactly 1/2; 0.1 + 0.2 exactly 3/10");
        D_TESTS_EXPECT( d_tests_valued("2 * x ^ 2 + 1",
                                       m::value(19),
                                       m::at("x", 3)),
                        "2 * x ^ 2 + 1 at x = 3: the integer 19");

        const m::value overflow = d_tests_value_of("9223372036854775807 + 1");

        D_TESTS_EXPECT( (overflow.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        ((overflow.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0),
                        "an exact sum past the family: G2's long double, "
                        "flagged");

        // ^, exact
        D_TESTS_EXPECT( d_tests_valued("2 ^ 10", m::value(1024)) &&
                        d_tests_valued("2 ^ -2", m::value::ratio(1, 4)) &&
                        d_tests_valued("(2 / 3) ^ 3", m::value::ratio(8, 27)) &&
                        d_tests_valued("(-2) ^ 3", m::value(-8)) &&
                        d_tests_valued("0 ^ 0", m::value(1)),
                        "^ to a whole power: exact");
        D_TESTS_EXPECT( d_tests_valued("4 ^ 0.5", m::value(2)) &&
                        d_tests_valued("8 ^ (1 / 3)", m::value(2)) &&
                        d_tests_valued("(1 / 4) ^ 0.5",
                                       m::value::ratio(1, 2)) &&
                        d_tests_valued("4 ^ 1.5", m::value(8)) &&
                        d_tests_valued("(4 / 9) ^ -0.5",
                                       m::value::ratio(3, 2)) &&
                        d_tests_valued("0 ^ 0.5", m::value(0)),
                        "^ to a rational power of whole roots: exact");
        D_TESTS_EXPECT( d_tests_valued("0 ^ -1",
                                       m::value(D_MATH_VALUE_ERROR_POLE)) &&
                        d_tests_valued("0 ^ -0.5",
                                       m::value(D_MATH_VALUE_ERROR_POLE)) &&
                        m::expr_is_complex(
                            d_tests_value_of("(-8) ^ (1 / 3)")) &&
                        d_tests_valued("(-4) ^ (1 / 2)",
                                       m::expr_complex(m::value(0),
                                                       m::value(2))),
                        "^: exact 0 to a negative power a pole; a negative "
                        "base to a power not whole its principal complex "
                        "value, exact where it is a Gaussian rational (S272, "
                        "S337)");

        volatile long double two  = 2.0L;
        volatile long double half = 0.5L;
        volatile long double some = 0.333L;
        const m::value       big  = d_tests_value_of("3 ^ 40");

        D_TESTS_EXPECT( d_tests_valued("2 ^ 0.5",
                                       m::value(d_math_pow(two, half))) &&
                        d_tests_valued("2 ^ 0.333",
                                       m::value(d_math_pow(two, some))),
                        "^ of exact operands not exact: the C core's pow in "
                        "long double");
        D_TESTS_EXPECT( (big.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        ((big.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0),
                        "3 ^ 40, past the family: G2's long double, flagged");
        D_TESTS_EXPECT( d_tests_valued("x ^ 2",
                                       m::value(2.25),
                                       m::at("x", 1.5)) &&
                        d_tests_valued("2 ^ x",
                                       m::value(d_math_pow(2.0f, 0.5f)),
                                       m::at("x", 0.5f)) &&
                        d_tests_valued("x ^ y",
                                       m::value(d_math_pow(2.0, 0.5)),
                                       m::at("x", 2).at("y", 0.5)),
                        "^ with an inexact operand: in the wider inexact "
                        "kind");

        // mod
        D_TESTS_EXPECT( d_tests_valued("7 mod 3", m::value(1)) &&
                        d_tests_valued("-7 mod 3", m::value(2)) &&
                        d_tests_valued("7 mod -3", m::value(-2)) &&
                        d_tests_valued("7.5 mod 2", m::value::ratio(3, 2)) &&
                        d_tests_valued("-7.5 mod 2", m::value::ratio(1, 2)),
                        "mod of exact operands: exact, floored");
        D_TESTS_EXPECT( d_tests_valued("5 mod 0",
                                       m::value(D_MATH_VALUE_ERROR_DOMAIN)),
                        "mod by exact 0: a domain error");
        D_TESTS_EXPECT( d_tests_valued("x mod 2",
                                       m::value(1.5f),
                                       m::at("x", 7.5f)) &&
                        d_tests_valued("x mod 2",
                                       m::value(0.5f),
                                       m::at("x", -7.5f)) &&
                        d_tests_valued("x mod y",
                                       m::value(0.5),
                                       m::at("x", third * m::value(3) +
                                                      m::value::ratio(1, 2))
                                           .at("y", 1.0)),
                        "mod with an inexact operand: in its kind, "
                        "floored");

        const m::value far_mod = d_tests_value_of(
            "9223372036854775807 mod (1 / 2)");

        D_TESTS_EXPECT( (far_mod.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (far_mod == m::value(0.0L)),
                        "mod whose exact quotient is past the family: in "
                        "long double");

        return;
    }

    // d_tests_fold_passing
    //   errors and what is no number, through + ^ and mod; flags carried;
    // and a boxed operand.
    void
    d_tests_fold_passing()
    {
        const m::value pole   = m::value(D_MATH_VALUE_ERROR_POLE);
        const m::value domain = m::value(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( d_tests_valued("x + 1", pole, m::at("x", pole)) &&
                        d_tests_valued("x ^ 2", pole, m::at("x", pole)) &&
                        d_tests_valued("2 ^ x", pole, m::at("x", pole)) &&
                        d_tests_valued("x mod 2", pole, m::at("x", pole)) &&
                        d_tests_valued("2 mod x", pole, m::at("x", pole)) &&
                        d_tests_valued("+x", pole, m::at("x", pole)),
                        "an error passes through each operation");
        D_TESTS_EXPECT( d_tests_valued("x ^ y",
                                       pole,
                                       m::at("x", pole).at("y", domain)) &&
                        d_tests_valued("x mod y",
                                       domain,
                                       m::at("x", domain).at("y", pole)),
                        "the first error, of two");
        D_TESTS_EXPECT( d_tests_valued("x + 1", domain, m::at("x", true)) &&
                        d_tests_valued("x ^ 2", domain, m::at("x", true)) &&
                        d_tests_valued("2 ^ x",
                                       domain,
                                       m::at("x", m::value())) &&
                        d_tests_valued("x mod 2",
                                       domain,
                                       m::at("x", m::value())) &&
                        d_tests_valued("2 mod x", domain, m::at("x", false)) &&
                        d_tests_valued("+x", domain, m::at("x", true)),
                        "a boolean or none in arithmetic: a domain error");
        D_TESTS_EXPECT( d_tests_valued("+x", m::value(2), m::at("x", 2)) &&
                        d_tests_valued("-x", m::value(-2), m::at("x", 2)),
                        "the unary + a number itself, and -");

        const m::value flagged = d_tests_value_of("9223372036854775807 + 1");
        const m::expr_bindings at_flagged = m::at("x", flagged);

        D_TESTS_EXPECT( ((d_tests_value_of("x ^ 0.5", at_flagged).flags() &
                          D_MATH_VALUE_FLAG_INEXACT) != 0) &&
                        ((d_tests_value_of("x mod 3", at_flagged).flags() &
                          D_MATH_VALUE_FLAG_INEXACT) != 0) &&
                        ((d_tests_value_of("2 ^ x", at_flagged).flags() &
                          D_MATH_VALUE_FLAG_INEXACT) != 0),
                        "^ and mod in the tower's kind carry their "
                        "operands' flags");

        // a boxed operand, a wide integer
        const m::value wide = d_tests_wide_of("100000000000000000000");
        const m::value square = d_tests_value_of("x ^ 2", m::at("x", wide));
        const m::value rest   = d_tests_value_of("x mod 7", m::at("x", wide));

        D_TESTS_EXPECT( (square == d_tests_wide_of(
                             "10000000000000000000000000000000000000000")) &&
                        (square.is_exact()),
                        "a wide integer to a whole power: exact, by its "
                        "kind's products");
        D_TESTS_EXPECT( (rest.kind() == D_MATH_VALUE_LONG_DOUBLE),
                        "a wide integer mod an integer: in long double");

        return;
    }

    // d_tests_functions_in
    //   whether each standard function in Type is the C core's plain form
    // in Type, at arguments each type holds exactly; abs |x|; factorial n!.
    template<typename Type>
    bool
    d_tests_functions_in()
    {
        volatile Type half    = Type(0.5);
        volatile Type three_2 = Type(1.5);
        volatile Type two     = Type(2);
        volatile Type minus   = Type(-2.5);

        return ( (d_tests_typed<Type>("sqrt(2)", d_math_sqrt(two)))         &&
                 (d_tests_typed<Type>("cbrt(-2.5)", d_math_cbrt(minus)))    &&
                 (d_tests_typed<Type>("sin(0.5)", d_math_sin(half)))        &&
                 (d_tests_typed<Type>("cos(0.5)", d_math_cos(half)))        &&
                 (d_tests_typed<Type>("tan(0.5)", d_math_tan(half)))        &&
                 (d_tests_typed<Type>("asin(0.5)", d_math_asin(half)))      &&
                 (d_tests_typed<Type>("acos(0.5)", d_math_acos(half)))      &&
                 (d_tests_typed<Type>("atan(0.5)", d_math_atan(half)))      &&
                 (d_tests_typed<Type>("sinh(0.5)", d_math_sinh(half)))      &&
                 (d_tests_typed<Type>("cosh(0.5)", d_math_cosh(half)))      &&
                 (d_tests_typed<Type>("tanh(0.5)", d_math_tanh(half)))      &&
                 (d_tests_typed<Type>("exp(0.5)", d_math_exp(half)))        &&
                 (d_tests_typed<Type>("exp2(0.5)", d_math_exp2(half)))      &&
                 (d_tests_typed<Type>("log(1.5)", d_math_log(three_2)))     &&
                 (d_tests_typed<Type>("log2(1.5)", d_math_log2(three_2)))   &&
                 (d_tests_typed<Type>("log10(1.5)", d_math_log10(three_2))) &&
                 (d_tests_typed<Type>("floor(-2.5)", Type(-3)))             &&
                 (d_tests_typed<Type>("ceil(-2.5)", Type(-2)))              &&
                 (d_tests_typed<Type>("round(-2.5)", Type(-3)))             &&
                 (d_tests_typed<Type>("trunc(-2.5)", Type(-2)))             &&
                 (d_tests_typed<Type>("abs(-2.5)", Type(2.5)))              &&
                 (d_tests_typed<Type>("factorial(5)", Type(120)))           &&
                 (d_tests_typed<Type>("atan2(1.5, -2.5)",
                                      d_math_atan2(three_2, minus)))        &&
                 (d_tests_typed<Type>("hypot(1.5, 2)",
                                      d_math_hypot(three_2, two)))          &&
                 (d_tests_typed<Type>("pow(2, 0.5)", d_math_pow(two, half))) &&
                 (d_tests_typed<Type>("fmod(-8, 3)", Type(-2)))             &&
                 (d_tests_typed<Type>("remainder(7, 2)", Type(-1))) );
    }

    // d_tests_fold_functions
    //   the fold in a type: each standard function the C core's; the
    // factorial; the constants; calls in the arithmetic.
    void
    d_tests_fold_functions()
    {
        const double inf = std::numeric_limits<double>::infinity();
        const double nan = std::numeric_limits<double>::quiet_NaN();

        D_TESTS_EXPECT( d_tests_functions_in<float>(),
                        "each standard function in float: the C core's");
        D_TESTS_EXPECT( d_tests_functions_in<double>(),
                        "each standard function in double: the C core's");
        D_TESTS_EXPECT( d_tests_functions_in<long double>(),
                        "each standard function in long double: the C "
                        "core's");
        D_TESTS_EXPECT( d_tests_typed<double>("0!", 1.0) &&
                        d_tests_typed<double>("3!", 6.0) &&
                        d_tests_typed<double>("171!", inf) &&
                        d_tests_typed<float>("35!",
                                             std::numeric_limits<float>::
                                                 infinity()) &&
                        d_tests_typed<long double>(
                            "3000!",
                            std::numeric_limits<long double>::infinity()) &&
                        d_tests_typed<double>("x!", inf, m::at("x", inf)) &&
                        d_tests_typed<double>("2.5!", nan) &&
                        d_tests_typed<float>(
                            "(-1)!",
                            std::numeric_limits<float>::quiet_NaN()) &&
                        d_tests_typed<long double>(
                            "x!",
                            std::numeric_limits<long double>::quiet_NaN(),
                            m::at("x", nan)),
                        "n!: the product, an infinity past the type; a NaN "
                        "for a fraction, a negative number, a NaN");

        volatile double     big       = 1.0;
        volatile long double big_long = 1.0L;

        for (int factor = 2; factor <= 170; ++factor)
        {
            big      = big * factor;
            big_long = big_long * factor;
        }

        D_TESTS_EXPECT( d_tests_typed<double>("170!", big) &&
                        d_tests_typed<long double>("170!", big_long),
                        "170!: the product rounded as it is made");
        D_TESTS_EXPECT( d_tests_typed<double>("abs(x)", 0.0,
                                              m::at("x", -0.0)) &&
                        d_tests_typed<float>("pi",
                                             m::constants::of<float>::pi()) &&
                        d_tests_typed<double>("e",
                                              m::constants::of<double>::e()) &&
                        d_tests_typed<long double>(
                            "pi",
                            m::constants::of<long double>::pi()),
                        "abs(-0) is 0; pi and e, each type's");
        D_TESTS_EXPECT( d_tests_typed<double>("2 ^ 3! - sqrt(16) * e ^ 0",
                                              60.0) &&
                        d_tests_typed<double>("sin(x) ^ 2", 0.0,
                                              m::at("x", 0)),
                        "calls and the factorial in the arithmetic");

        return;
    }

    // d_tests_fold_functions_value
    //   the fold to a value: each function exact wherever its value is
    // rational, its domain's errors, and the tower's kind elsewhere.
    void
    d_tests_fold_functions_value()
    {
        const m::value domain = m::value(D_MATH_VALUE_ERROR_DOMAIN);
        const m::value pole   = m::value(D_MATH_VALUE_ERROR_POLE);
        const m::value zero   = m::value(0);
        const m::value one    = m::value(1);

        // the arguments of the C core's answers the folds are held to:
        // volatile, so that each answer is computed as the test runs, by the
        // C library as the fold's is, never by the compiler, which rounds
        // correctly where the library may be an ulp away (G6)
        volatile long double two_l       = 2.0L;
        volatile long double minus_two_l = -2.0L;
        volatile long double one_l       = 1.0L;
        volatile long double minus_one_l = -1.0L;
        volatile long double half_l      = 0.5L;
        volatile long double zero_l      = 0.0L;

        D_TESTS_EXPECT( d_tests_valued("sqrt(4/9)", m::value::ratio(2, 3)) &&
                        d_tests_valued("sqrt(0)", zero) &&
                        d_tests_valued("cbrt(-27/8)",
                                       m::value::ratio(-3, 2)) &&
                        d_tests_valued("hypot(3, 4)", m::value(5)) &&
                        d_tests_valued("hypot(5/2, 6)",
                                       m::value::ratio(13, 2)) &&
                        d_tests_valued("pow(4, 1/2)", m::value(2)),
                        "roots and powers: exact where the root is");
        D_TESTS_EXPECT( d_tests_valued("sqrt(2)",
                                       m::value(d_math_sqrt(two_l))) &&
                        d_tests_valued("cbrt(2)",
                                       m::value(d_math_cbrt(two_l))) &&
                        d_tests_valued("cbrt(-2)",
                                       m::value(d_math_cbrt(minus_two_l))) &&
                        d_tests_valued("hypot(1, 1)",
                                       m::value(d_math_hypot(one_l, one_l))),
                        "and a long double where it is not");
        D_TESTS_EXPECT( d_tests_valued("sin(0)", zero) &&
                        d_tests_valued("tan(0)", zero) &&
                        d_tests_valued("asin(0)", zero) &&
                        d_tests_valued("atan(0)", zero) &&
                        d_tests_valued("sinh(0)", zero) &&
                        d_tests_valued("tanh(0)", zero) &&
                        d_tests_valued("cos(0)", one) &&
                        d_tests_valued("cosh(0)", one) &&
                        d_tests_valued("exp(0)", one) &&
                        d_tests_valued("acos(1)", zero) &&
                        d_tests_valued("log(1)", zero) &&
                        d_tests_valued("atan2(0, 5)", zero),
                        "the transcendental functions exact at their one "
                        "rational point each");
        D_TESTS_EXPECT( d_tests_valued("sin(1/2)",
                                       m::value(d_math_sin(half_l))) &&
                        d_tests_valued("cos(2)",
                                       m::value(d_math_cos(two_l))) &&
                        d_tests_valued("cosh(1)",
                                       m::value(d_math_cosh(one_l))) &&
                        d_tests_valued("acos(0)",
                                       m::value(d_math_acos(zero_l))) &&
                        d_tests_valued("acos(1/2)",
                                       m::value(d_math_acos(half_l))) &&
                        d_tests_valued("log(1/2)",
                                       m::value(d_math_log(half_l))) &&
                        d_tests_valued("exp(1)",
                                       m::value(d_math_exp(one_l))) &&
                        d_tests_valued("atan2(0, -1)",
                                       m::value(d_math_atan2(zero_l,
                                                             minus_one_l))) &&
                        d_tests_valued("atan2(1, 1)",
                                       m::value(d_math_atan2(one_l, one_l))),
                        "and a long double elsewhere, beside the point too");
        D_TESTS_EXPECT( d_tests_valued("exp2(10)", m::value(1024)) &&
                        d_tests_valued("exp2(-2)", m::value::ratio(1, 4)) &&
                        d_tests_valued("log2(8)", m::value(3)) &&
                        d_tests_valued("log2(1/8)", m::value(-3)) &&
                        d_tests_valued("log10(1000)", m::value(3)) &&
                        d_tests_valued("log10(1/100)", m::value(-2)) &&
                        d_tests_valued("log10(2)",
                                       m::value(d_math_log10(two_l))) &&
                        d_tests_valued("exp2(1/2)",
                                       m::value(d_math_exp2(half_l))),
                        "exp2 at a whole number, log2 and log10 at a power "
                        "of 2 or 10: exact");
        D_TESTS_EXPECT( d_tests_valued("floor(-7/2)", m::value(-4)) &&
                        d_tests_valued("ceil(-7/2)", m::value(-3)) &&
                        d_tests_valued("round(-7/2)", m::value(-4)) &&
                        d_tests_valued("trunc(-7/2)", m::value(-3)) &&
                        d_tests_valued("round(5/2)", m::value(3)) &&
                        d_tests_valued("round(7/3)", m::value(2)) &&
                        d_tests_valued("ceil(7/3)", m::value(3)) &&
                        d_tests_valued("floor(3)", m::value(3)) &&
                        d_tests_valued("floor(-3)", m::value(-3)) &&
                        d_tests_valued("ceil(3)", m::value(3)) &&
                        d_tests_valued("abs(-3/4)", m::value::ratio(3, 4)),
                        "floor, ceil, round (halves away from 0), trunc and "
                        "abs: exact");
        D_TESTS_EXPECT( d_tests_valued("fmod(7, -3)", one) &&
                        d_tests_valued("fmod(-7, 3)", m::value(-1)) &&
                        d_tests_valued("fmod(7/2, 1)", m::value::ratio(1, 2)) &&
                        d_tests_valued("remainder(7, 2)", m::value(-1)) &&
                        d_tests_valued("remainder(5, 2)", one) &&
                        d_tests_valued("remainder(-7, 2)", one) &&
                        d_tests_valued("remainder(8, 3)", m::value(-1)),
                        "fmod truncated, remainder to the nearest, ties to "
                        "even: exact");
        D_TESTS_EXPECT( d_tests_valued("factorial(0)", one) &&
                        d_tests_valued("5!", m::value(120)) &&
                        d_tests_valued("20!",
                                       m::value(static_cast<dj_math_imax>(
                                           2432902008176640000LL))),
                        "factorial: exact, of a whole number");

        const m::value past     = d_tests_value_of("21!");
        const m::value past_all = d_tests_value_of("factorial(3000)");

        D_TESTS_EXPECT( (past.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        ((past.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0) &&
                        (past == m::value(51090942171709440000.0L)) &&
                        (past_all.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        ((past_all.flags() & D_MATH_VALUE_FLAG_INEXACT) !=
                             0) &&
                        (past_all ==
                             m::value(std::numeric_limits<long double>::
                                          infinity())),
                        "21!, past the family: G2's long double, flagged; "
                        "3000!, past long double's range, its infinity");
        D_TESTS_EXPECT( d_tests_valued("factorial(-1)", domain) &&
                        d_tests_valued("(1/2)!", domain) &&
                        d_tests_valued("fmod(1, 0)", domain) &&
                        d_tests_valued("remainder(1, 0)", domain) &&
                        d_tests_valued("atan2(0, 0)", domain),
                        "an exact argument outside the domain, where no "
                        "complex value is either: a domain error");
        D_TESTS_EXPECT( d_tests_valued("sqrt(-4)",
                                       m::expr_complex(m::value(0),
                                                       m::value(2))) &&
                        m::expr_is_complex(d_tests_value_of("asin(2)")) &&
                        m::expr_is_complex(d_tests_value_of("acos(-3/2)")) &&
                        m::expr_is_complex(d_tests_value_of("log(-1)")),
                        "where the value is complex alone, the principal "
                        "complex one (S272)");
        D_TESTS_EXPECT( d_tests_valued("log(0)", pole) &&
                        d_tests_valued("log10(0)", pole),
                        "the logarithm of exact 0: a pole");

        // the tower's kind: the widest inexact argument's
        volatile float  f_half = 0.5f;
        volatile double d_half = 0.5;
        volatile double d_two  = 2.0;

        D_TESTS_EXPECT( d_tests_valued("sin(x)",
                                       m::value(d_tests_stored(
                                           d_math_sin(f_half))),
                                       m::at("x", f_half)) &&
                        d_tests_valued("sin(x)",
                                       m::value(d_tests_stored(
                                           d_math_sin(d_half))),
                                       m::at("x", d_half)) &&
                        d_tests_valued("atan2(x, y)",
                                       m::value(d_tests_stored(
                                           d_math_atan2(d_half, d_two))),
                                       m::at("x", f_half).at("y", 2.0)) &&
                        d_tests_valued("sqrt(x)",
                                       m::expr_complex(m::value(0.0),
                                                       m::value(1.0)),
                                       m::at("x", -1.0)) &&
                        d_tests_valued("fmod(x, 0)",
                                       m::value(d_math_fmod(1.5, 0.0)),
                                       m::at("x", 1.5)) &&
                        d_tests_valued("x mod 0",
                                       m::value(d_math_fmod(1.5, 0.0)),
                                       m::at("x", 1.5)),
                        "an inexact argument: its kind, the wider of two; a "
                        "NaN where the C core gives one, by 0 among them, "
                        "but where the value is complex alone (S272)");

        const m::value flagged = d_tests_value_of("9223372036854775807 + 1");

        D_TESTS_EXPECT( ((d_tests_value_of("sin(x)", m::at("x", flagged))
                              .flags() & D_MATH_VALUE_FLAG_INEXACT) != 0) &&
                        ((d_tests_value_of("atan2(1, x)",
                                           m::at("x", flagged)).flags() &
                          D_MATH_VALUE_FLAG_INEXACT) != 0),
                        "a function's result carries its arguments' flags");
        D_TESTS_EXPECT( d_tests_valued("sin(x)", pole, m::at("x", pole)) &&
                        d_tests_valued("atan2(x, y)", pole,
                                       m::at("x", pole).at("y", domain)) &&
                        d_tests_valued("abs(x)", pole, m::at("x", pole)) &&
                        d_tests_valued("floor(x)", domain,
                                       m::at("x", true)) &&
                        d_tests_valued("hypot(1, x)", domain,
                                       m::at("x", m::value())),
                        "an error passes, the first; a boolean or none, a "
                        "domain error");
        D_TESTS_EXPECT( d_tests_valued("pi",
                                       m::value(m::constants::of<long double>::
                                                    pi())) &&
                        d_tests_valued("e",
                                       m::value(m::constants::of<long double>::
                                                    e())) &&
                        d_tests_valued("2 * pi",
                                       m::value(2.0L *
                                                m::constants::of<long double>::
                                                    pi())),
                        "pi and e: the long double nearest each");

        // a boxed argument, a wide integer
        const m::value wide = d_tests_wide_of("-100000000000000000000");

        D_TESTS_EXPECT( (d_tests_value_of("abs(x)", m::at("x", wide)) ==
                             d_tests_wide_of("100000000000000000000")) &&
                        (d_tests_value_of("sqrt(-x)",
                                          m::at("x", wide)).kind() ==
                             D_MATH_VALUE_LONG_DOUBLE),
                        "a wide integer: abs exact, by its kind; sqrt in "
                        "long double");

        // a boxed argument not exact, of the test's own kind
        static const long double nine_quarters   = 2.25L;
        volatile long double     nine_quarters_l = nine_quarters;
        const m::value           real = d_tests_real_of(&nine_quarters);

        D_TESTS_EXPECT( d_tests_valued("sqrt(x)", m::value(1.5L),
                                       m::at("x", real)) &&
                        d_tests_valued("atan2(x, y)",
                                       m::value(d_math_atan2(nine_quarters_l,
                                                             half_l)),
                                       m::at("x", real).at("y", f_half)) &&
                        d_tests_typed<double>("x", 2.25, m::at("x", real)),
                        "a boxed kind not exact: in long double, beside a "
                        "float too; in a type, its nearest");

        return;
    }

    // d_tests_relations_in
    //   the relations in Type: C's comparisons, 1 where one holds and 0
    // where it does not, none holding of a NaN but !=.
    template<typename Type>
    bool
    d_tests_relations_in()
    {
        const m::expr_bindings unknown =
            m::at("u", std::numeric_limits<double>::quiet_NaN());
        const Type             one     = Type(1);
        const Type             zero    = Type(0);

        return ( (d_tests_typed<Type>("1 < 2", one))            &&
                 (d_tests_typed<Type>("2 < 1", zero))           &&
                 (d_tests_typed<Type>("1 < 1", zero))           &&
                 (d_tests_typed<Type>("1 <= 1", one))           &&
                 (d_tests_typed<Type>("2 <= 1", zero))          &&
                 (d_tests_typed<Type>("2 > 1", one))            &&
                 (d_tests_typed<Type>("1 > 1", zero))           &&
                 (d_tests_typed<Type>("1 >= 1", one))           &&
                 (d_tests_typed<Type>("1 >= 2", zero))          &&
                 (d_tests_typed<Type>("1 = 1", one))            &&
                 (d_tests_typed<Type>("1 = 2", zero))           &&
                 (d_tests_typed<Type>("1 != 2", one))           &&
                 (d_tests_typed<Type>("1 != 1", zero))          &&
                 (d_tests_typed<Type>("u < 1", zero, unknown))  &&
                 (d_tests_typed<Type>("u >= 1", zero, unknown)) &&
                 (d_tests_typed<Type>("u = u", zero, unknown))  &&
                 (d_tests_typed<Type>("u != u", one, unknown)) );
    }

    // d_tests_logic_in
    //   and, or, not, all_of and any_of in Type, Kleene's: 0 false, a NaN
    // unknown, any other number true; true and false 1 and 0.
    template<typename Type>
    bool
    d_tests_logic_in()
    {
        const m::expr_bindings unknown =
            m::at("u", std::numeric_limits<double>::quiet_NaN());
        const Type             nan     = std::numeric_limits<Type>::quiet_NaN();
        const Type             one     = Type(1);
        const Type             zero    = Type(0);

        return ( (d_tests_typed<Type>("1 and 0", zero))                     &&
                 (d_tests_typed<Type>("2 and 3", one))                      &&
                 (d_tests_typed<Type>("0 or 0", zero))                      &&
                 (d_tests_typed<Type>("0 or -2", one))                      &&
                 (d_tests_typed<Type>("not 0", one))                        &&
                 (d_tests_typed<Type>("not 4", zero))                       &&
                 (d_tests_typed<Type>("u and 0", zero, unknown))            &&
                 (d_tests_typed<Type>("0 and u", zero, unknown))            &&
                 (d_tests_typed<Type>("u and 1", nan, unknown))             &&
                 (d_tests_typed<Type>("u or 1", one, unknown))              &&
                 (d_tests_typed<Type>("1 or u", one, unknown))              &&
                 (d_tests_typed<Type>("u or 0", nan, unknown))              &&
                 (d_tests_typed<Type>("not u", nan, unknown))               &&
                 (d_tests_typed<Type>("all_of(1, 2, 3)", one))              &&
                 (d_tests_typed<Type>("all_of(1, u, 0)", zero, unknown))    &&
                 (d_tests_typed<Type>("all_of(1, u)", nan, unknown))        &&
                 (d_tests_typed<Type>("any_of(0, 0, 0)", zero))             &&
                 (d_tests_typed<Type>("any_of(0, u, 5)", one, unknown))     &&
                 (d_tests_typed<Type>("any_of(0, u)", nan, unknown))        &&
                 (d_tests_typed<Type>("true and not false", one))           &&
                 (d_tests_typed<Type>("true + true", Type(2))) );
    }

    // d_tests_piecewise_in
    //   piecewise in Type: the value of the first branch whose condition is
    // true, a nonzero number; a NaN where a NaN condition comes first, or
    // where no branch holds.
    template<typename Type>
    bool
    d_tests_piecewise_in()
    {
        const m::expr_bindings unknown =
            m::at("u", std::numeric_limits<double>::quiet_NaN());
        const Type             nan     = std::numeric_limits<Type>::quiet_NaN();
        const char* const      abs_x   =
            "piecewise(when(x < 0, -x), otherwise(x))";
        const char* const      sign_x  =
            "piecewise(when(x > 0, 1), when(x < 0, -1))";

        return ( (d_tests_typed<Type>(abs_x, Type(3), m::at("x", -3)))      &&
                 (d_tests_typed<Type>(abs_x, Type(2), m::at("x", 2)))       &&
                 (d_tests_typed<Type>(sign_x, Type(-1), m::at("x", -5)))    &&
                 (d_tests_typed<Type>(sign_x, nan, m::at("x", 0)))          &&
                 (d_tests_typed<Type>("piecewise(when(u, 1), otherwise(2))",
                                      nan,
                                      unknown))                             &&
                 (d_tests_typed<Type>(
                      "piecewise(when(0, 1), when(u, 2), otherwise(3))",
                      nan,
                      unknown))                                             &&
                 (d_tests_typed<Type>("piecewise(when(1, 5), when(u, 6))",
                                      Type(5),
                                      unknown))                             &&
                 (d_tests_typed<Type>("piecewise(when(2, 5))", Type(5)))    &&
                 (d_tests_typed<Type>("piecewise(otherwise(7))", Type(7))) );
    }

    // d_tests_fold_logic
    //   the relations, logic and piecewise in both folds (2d): in a type,
    // 1 and 0; as a value, booleans, by value.h's exact order and Kleene's
    // logic.
    void
    d_tests_fold_logic()
    {
        // in a type
        D_TESTS_EXPECT( (d_tests_relations_in<float>()) &&
                        (d_tests_relations_in<double>()) &&
                        (d_tests_relations_in<long double>()),
                        "the relations in each type: C's comparisons, 1 "
                        "and 0, none holding of a NaN but !=");
        D_TESTS_EXPECT( (d_tests_logic_in<float>()) &&
                        (d_tests_logic_in<double>()) &&
                        (d_tests_logic_in<long double>()),
                        "and, or, not, all_of and any_of in each type: "
                        "Kleene's, 0 false, a NaN unknown; true and false 1 "
                        "and 0");
        D_TESTS_EXPECT( (d_tests_piecewise_in<float>()) &&
                        (d_tests_piecewise_in<double>()) &&
                        (d_tests_piecewise_in<long double>()),
                        "piecewise in each type: the first branch that "
                        "holds; a NaN where a NaN condition comes first, or "
                        "none holds");
        D_TESTS_EXPECT( (d_tests_typed<double>("0.1 + 0.2 = 0.3", 0.0)) &&
                        (d_tests_typed<double>("0.1 + 0.2 != 0.3", 1.0)),
                        "0.1 + 0.2 = 0.3 in double: false, each literal "
                        "rounded");

        // as a value: relations
        const m::value         yes     = m::value(true);
        const m::value         no      = m::value(false);
        const m::value         domain  = m::value(D_MATH_VALUE_ERROR_DOMAIN);
        const m::value         pole    = m::value(D_MATH_VALUE_ERROR_POLE);
        const m::expr_bindings unknown =
            m::at("u", std::numeric_limits<double>::quiet_NaN());
        const m::expr_bindings doubles = m::at("x", 0.5).at("y", 0.1);

        D_TESTS_EXPECT( (d_tests_valued("0.1 + 0.2 = 0.3", yes)) &&
                        (d_tests_valued("1/3 < 0.34", yes)) &&
                        (d_tests_valued("2/4 = 1/2", yes)) &&
                        (d_tests_valued("1 != 1.0", no)) &&
                        (d_tests_valued("3 >= 3", yes)) &&
                        (d_tests_valued("3 > 3", no)) &&
                        (d_tests_valued("1/2 <= 0.5", yes)) &&
                        (d_tests_valued("-1 <= -2", no)),
                        "a relation of two exact numbers: their exact "
                        "order, so 0.1 + 0.2 = 0.3 holds");
        D_TESTS_EXPECT( (d_tests_valued("x = 1/2", yes, doubles)) &&
                        (d_tests_valued("y = 1/10", no, doubles)) &&
                        (d_tests_valued("y > 1/10", yes, doubles)),
                        "across kinds, exactly: the double 0.1 is more than "
                        "1/10");
        D_TESTS_EXPECT( (d_tests_valued("u = u", no, unknown)) &&
                        (d_tests_valued("u != u", yes, unknown)) &&
                        (d_tests_valued("u < 1", no, unknown)) &&
                        (d_tests_valued("u >= 1", no, unknown)),
                        "a NaN: unordered, none holding but !=");

        const m::expr_bindings wide =
            m::at("a", d_tests_wide_of("1267650600228229401496703205376"))
                .at("b", d_tests_wide_of("1267650600228229401496703205377"));

        D_TESTS_EXPECT( (d_tests_valued("a < b", yes, wide)) &&
                        (d_tests_valued("a = b - 1", yes, wide)) &&
                        (d_tests_valued("a = 2 ^ 100", yes, wide)) &&
                        (d_tests_valued("b > 1e30", yes, wide)),
                        "wide integers past 64 bits, a boxed kind: by "
                        "value.h's exact order");
        D_TESTS_EXPECT( (d_tests_valued("true = true", yes)) &&
                        (d_tests_valued("true = false", no)) &&
                        (d_tests_valued("false != true", yes)) &&
                        (d_tests_valued("(1 < 2) = (2 < 3)", yes)),
                        "two booleans: = and != by their truth");
        D_TESTS_EXPECT( (d_tests_valued("sqrt(-1) < 1", domain)) &&
                        (d_tests_valued("1 < 1 / 0", pole)) &&
                        (d_tests_valued("1 / 0 = sqrt(-1)", pole)) &&
                        (d_tests_valued("1 < true", domain)) &&
                        (d_tests_valued("true < false", domain)) &&
                        (d_tests_valued("x = 1", domain, m::at("x",
                                                               m::value()))),
                        "an error passes, the first; a boolean ordered, or "
                        "a boolean or none against a number, a domain "
                        "error");

        // as a value: logic
        D_TESTS_EXPECT( (d_tests_valued("true and false", no)) &&
                        (d_tests_valued("true or false", yes)) &&
                        (d_tests_valued("not false", yes)) &&
                        (d_tests_valued("not not true", yes)) &&
                        (d_tests_valued("true and true", yes)) &&
                        (d_tests_valued("false or false", no)),
                        "and, or and not of booleans");
        D_TESTS_EXPECT( (d_tests_valued("false and sqrt(-1) > 0", no)) &&
                        (d_tests_valued("sqrt(-1) > 0 and false", no)) &&
                        (d_tests_valued("true or 1 / 0 > 0", yes)) &&
                        (d_tests_valued("false and 1", no)),
                        "Kleene's: a false operand makes and false, a true "
                        "one or true, whatever the other is");
        D_TESTS_EXPECT( (d_tests_valued("true and 1 / 0 > 0", pole)) &&
                        (d_tests_valued("1 / 0 > 0 or sqrt(-1) > 0", pole)) &&
                        (d_tests_valued("1 and true", domain)) &&
                        (d_tests_valued("not 1", domain)) &&
                        (d_tests_valued("not 1 / 0 > 0", pole)),
                        "otherwise an error passes, the first; a number in "
                        "logic, a domain error");
        D_TESTS_EXPECT( (d_tests_valued("all_of(true, 1 < 2, 2 < 3)", yes)) &&
                        (d_tests_valued("all_of(true, sqrt(-1) > 0, false)",
                                        no)) &&
                        (d_tests_valued("all_of(true, sqrt(-1) > 0)",
                                        domain)) &&
                        (d_tests_valued("any_of(false, false)", no)) &&
                        (d_tests_valued("any_of(1 / 0 > 0, true)", yes)) &&
                        (d_tests_valued("any_of(false, 2)", domain)),
                        "all_of and any_of: and and or of any number of "
                        "operands");

        // as a value: piecewise, the constants
        const char* const abs_x = "piecewise(when(x < 0, -x), otherwise(x))";

        D_TESTS_EXPECT( (d_tests_valued(abs_x,
                                        m::value::ratio(1, 3),
                                        m::at("x", m::value::ratio(-1, 3)))) &&
                        (d_tests_valued(abs_x,
                                        m::value::ratio(2, 3),
                                        m::at("x", m::value::ratio(2, 3)))),
                        "piecewise as a value: the first branch that holds, "
                        "exactly");
        D_TESTS_EXPECT( (d_tests_valued("piecewise(when(x > 0, 1), "
                                        "when(x < 0, -1))",
                                        domain,
                                        m::at("x", 0))) &&
                        (d_tests_valued("piecewise(when(1 / 0 > 0, 1), "
                                        "otherwise(2))",
                                        pole)) &&
                        (d_tests_valued("piecewise(when(1, 5), "
                                        "otherwise(3))",
                                        domain)) &&
                        (d_tests_valued("piecewise(when(true, fmod(1, 0)))",
                                        domain)),
                        "no branch holding, a domain error; a condition's "
                        "error passes, a number for one a domain error; the "
                        "branch's value as it is");
        D_TESTS_EXPECT( (d_tests_valued("piecewise(when(false, 1 / 0), "
                                        "otherwise(3))",
                                        m::value(3))) &&
                        (d_tests_valued("piecewise(when(true, 3), "
                                        "when(1 / 0 > 0, 4))",
                                        m::value(3))),
                        "what no branch reaches before the one that holds "
                        "matters not");
        D_TESTS_EXPECT( (d_tests_valued("true", yes)) &&
                        (d_tests_valued("false", no)) &&
                        (d_tests_valued("x", yes, m::at("x", true))),
                        "true and false, and a bound boolean, as values");

        return;
    }

    // d_tests_fold_errors
    //   what neither fold computes: an unbound variable, a constant or a
    // function the tables have not, an operator or a call on the wrong
    // number of operands, indexing by none or by three among them, an
    // operator of no kind, and a term made by default; and in a type, what
    // a literal, indexing and a number's transpose give.
    void
    d_tests_fold_errors()
    {
        const m::expr_term sum = d_tests_x_plus_1();
        const m::expr_term one = m::expr_number(m::value(1), "1");
        const m::expr_term two = m::expr_number(m::value(2), "2");

        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(sum),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable x is bound to no "
                                         "value") &&
                        d_tests_is_error(m::expr_evaluate(sum,
                                                          m::expr_bindings(),
                                                          m::expr_context()),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable x is bound to no "
                                         "value"),
                        "an unbound variable, in either fold");

        const m::expr_result<m::expr_term> both = m::expr_parse("q + r");

        D_TESTS_EXPECT( (both.is_ok()) &&
                        (d_tests_is_error(m::expr_evaluate<float>(
                                              both.value()),
                                          m::expr_status::unbound_variable,
                                          m::expr_no_offset,
                                          "the variable q is bound to no "
                                          "value")),
                        "the first error, left to right");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_constant("tau")),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no constant of the language is "
                                         "named tau") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_constant("tau"),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no constant of the language is "
                                         "named tau"),
                        "a constant the table has not: unknown_name");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_call("foo", { one })),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no function of the language is "
                                         "named foo") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_call("foo", { one }),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no function of the language is "
                                         "named foo"),
                        "a call of a function the table has not: "
                        "unknown_name");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<float>(
                                             m::expr_call("atan2", { one })),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "atan2 takes 2 arguments, not 1") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_call("sin", {}),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "sin takes 1 argument, not 0") &&
                        d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_call("sin",
                                                          { one, two })),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "sin takes 1 argument, not 2") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_call("atan2",
                                                          { one, two, one }),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "atan2 takes 2 arguments, not 3") &&
                        d_tests_is_error(m::expr_evaluate<long double>(
                                             d_tests_apply(
                                                 m::expr_op_kind::factorial,
                                                 one,
                                                 two)),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "'!' takes 1 operand, not 2"),
                        "a call, or the factorial, of a number of operands "
                        "it does not take, fewer or more: wrong_arity");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_call(
                                                 "foo",
                                                 { m::expr_variable("y") })),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable y is bound to no "
                                         "value"),
                        "an argument's error before the call's own");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<float>(
                                             d_tests_apply(
                                                 m::expr_op_kind::vector,
                                                 one,
                                                 two)),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_evaluate<Type>: a vector or a "
                                         "matrix has no value in a type"),
                        "a literal: no value in a type");

        const m::expr_result<double> indexed    =
            m::expr_evaluate<double>(d_tests_apply(m::expr_op_kind::index,
                                                   one,
                                                   two));
        const m::expr_result<double> transposed =
            m::expr_evaluate<double>(d_tests_apply(
                                         m::expr_op_kind::transpose,
                                         two));

        D_TESTS_EXPECT( (indexed.is_ok()) &&
                        (indexed.value() != indexed.value()) &&
                        (transposed.is_ok()) &&
                        (transposed.value() == 2.0),
                        "in a type, indexing a NaN, as no number takes "
                        "it, and a number's transpose the number");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_application(
                                                 m::expr_op_kind::index,
                                                 { one })),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "indexing takes 1 or 2 indices, "
                                         "not 0") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_application(
                                                 m::expr_op_kind::index,
                                                 { one, two, two, two }),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "indexing takes 1 or 2 indices, "
                                         "not 3") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_application(
                                                 m::expr_op_kind::index,
                                                 std::vector<m::expr_term>()),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "indexing takes 1 or 2 indices, "
                                         "not 0"),
                        "indexing by none, or by three, made by hand: "
                        "wrong_arity, in both folds");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_apply(
                                                 m::expr_op_kind::none,
                                                 one,
                                                 two)),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no operator of the language is of "
                                         "kind 0") &&
                        d_tests_is_error(m::expr_evaluate(
                                             d_tests_apply(
                                                 m::expr_op_kind::none,
                                                 one),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no operator of the language is of "
                                         "kind 0"),
                        "an operator of no kind the language has: "
                        "unknown_name, in both folds");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_apply(m::expr_op_kind::add,
                                                           one)),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "'+' takes 2 operands, not 1") &&
                        d_tests_is_error(m::expr_evaluate(
                                             d_tests_apply(m::expr_op_kind::neg,
                                                           one,
                                                           two),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "'-' takes 1 operand, not 2"),
                        "an operator on a number of operands it does not "
                        "take: wrong_arity");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_apply(m::expr_op_kind::lt,
                                                           one)),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "'<' takes 2 operands, not 1") &&
                        d_tests_is_error(m::expr_evaluate(
                                             d_tests_apply(
                                                 m::expr_op_kind::logical_not,
                                                 one,
                                                 two),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "'not' takes 1 operand, not 2"),
                        "a relation, or logic, on a number of operands it "
                        "does not take: wrong_arity");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_call("all_of", {})),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "all_of takes at least 1 argument, "
                                         "not 0") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_call("piecewise", {}),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "piecewise takes at least 1 "
                                         "argument, not 0"),
                        "a function of any number of arguments from 1, of "
                        "none: wrong_arity");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_application(m::expr_op(),
                                                                 one)),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no operator of the language is of "
                                         "kind 0"),
                        "an operator of no kind: unknown_name");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_term()),
                                         m::expr_status::missing_operand,
                                         m::expr_no_offset,
                                         "a term made by default has no "
                                         "value") &&
                        d_tests_is_error(m::expr_evaluate(m::expr_term(),
                                                          m::expr_bindings(),
                                                          m::expr_context()),
                                         m::expr_status::missing_operand,
                                         m::expr_no_offset,
                                         "a term made by default has no "
                                         "value"),
                        "a term made by default: missing_operand");

        return;
    }


    //==========================================================================
    // 9.  PARSING AND RENDERING
    //==========================================================================

    // d_tests_parse_numbers
    //   a number read exactly, its text its literal; a name a variable, or
    // a constant the standard table has.
    void
    d_tests_parse_numbers()
    {
        const m::expr_result<m::expr_term> tenth = m::expr_parse("0.1");
        const m::expr_result<m::expr_term> far   = m::expr_parse("1e30");
        const m::expr_result<m::expr_term> past  = m::expr_parse("1e5000");

        D_TESTS_EXPECT( d_tests_parses("42",
                                       m::expr_number(m::value(42), "42")),
                        "42: the integer 42, written 42");
        D_TESTS_EXPECT( (tenth.is_ok()) &&
                        (m::expr_is_number(tenth.value())) &&
                        (m::expr_atom_of(tenth.value()).text == "0.1") &&
                        (m::internal::expr_same_number(
                             m::expr_atom_of(tenth.value()).number,
                             m::value::ratio(1, 10))),
                        "0.1: exactly 1/10, written 0.1 (G3)");
        D_TESTS_EXPECT( (far.is_ok()) &&
                        (m::expr_atom_of(far.value()).number.kind() ==
                             D_MATH_VALUE_LONG_DOUBLE) &&
                        ( (m::expr_atom_of(far.value()).number.flags() &
                           D_MATH_VALUE_FLAG_INEXACT) != 0 ) &&
                        (past.is_ok()) &&
                        (m::internal::expr_same_number(
                             m::expr_atom_of(past.value()).number,
                             m::value(D_MATH_VALUE_ERROR_RANGE))),
                        "a literal past the family: G2's long double; past "
                        "every long double, a range error");
        D_TESTS_EXPECT( d_tests_parses("x", m::expr_variable("x")) &&
                        d_tests_parses("_a1", m::expr_variable("_a1")) &&
                        d_tests_parses("pie", m::expr_variable("pie")) &&
                        d_tests_parses("E", m::expr_variable("E")),
                        "a name: a variable");
        D_TESTS_EXPECT( d_tests_parses("pi", m::expr_constant("pi")) &&
                        d_tests_parses("e", m::expr_constant("e")),
                        "pi and e: the standard table's constants");

        return;
    }

    // d_tests_parse_grammar
    //   G5's precedence and associativity; the unary - and + by place; the
    // brackets.
    void
    d_tests_parse_grammar()
    {
        const m::expr_term a = m::expr_variable("a");
        const m::expr_term b = m::expr_variable("b");
        const m::expr_term c = m::expr_variable("c");
        const m::expr_term x = m::expr_variable("x");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");
        const m::expr_term n3 = d_tests_literal("3");

        D_TESTS_EXPECT( d_tests_parses("1 + 2 * 3",
                                       d_tests_apply(m::expr_op_kind::add, n1,
                                           d_tests_apply(m::expr_op_kind::mul,
                                                         n2, n3))) &&
                        d_tests_parses("1 - 2 - 3",
                                       d_tests_apply(m::expr_op_kind::sub,
                                           d_tests_apply(m::expr_op_kind::sub,
                                                         n1, n2),
                                           n3)) &&
                        d_tests_parses("a / b / c",
                                       d_tests_apply(m::expr_op_kind::div,
                                           d_tests_apply(m::expr_op_kind::div,
                                                         a, b),
                                           c)),
                        "* over +; - and / to the left");
        D_TESTS_EXPECT( d_tests_parses("2 ^ 3 ^ 2",
                                       d_tests_apply(m::expr_op_kind::pow, n2,
                                           d_tests_apply(m::expr_op_kind::pow,
                                                         n3, n2))),
                        "^ to the right: 2 ^ 3 ^ 2 is 2 ^ 9");
        D_TESTS_EXPECT( d_tests_parses("a mod b * c",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(m::expr_op_kind::mod,
                                                         a, b),
                                           c)) &&
                        d_tests_parses("a + b mod c",
                                       d_tests_apply(m::expr_op_kind::add, a,
                                           d_tests_apply(m::expr_op_kind::mod,
                                                         b, c))),
                        "mod as tight as *, to the left");
        D_TESTS_EXPECT( d_tests_parses("-x ^ 2",
                                       d_tests_apply(m::expr_op_kind::neg,
                                           d_tests_apply(m::expr_op_kind::pow,
                                                         x, n2))) &&
                        d_tests_parses("-a * b",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         a),
                                           b)) &&
                        d_tests_parses("2 ^ -3",
                                       d_tests_apply(m::expr_op_kind::pow, n2,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         n3))),
                        "the unary - below ^ and above *: -x ^ 2 is "
                        "-(x ^ 2)");
        D_TESTS_EXPECT( d_tests_parses("a - -b",
                                       d_tests_apply(m::expr_op_kind::sub, a,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         b))) &&
                        d_tests_parses("a * -b",
                                       d_tests_apply(m::expr_op_kind::mul, a,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         b))) &&
                        d_tests_parses("--a",
                                       d_tests_apply(m::expr_op_kind::neg,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         a))) &&
                        d_tests_parses("(-a)",
                                       d_tests_apply(m::expr_op_kind::neg,
                                                     a)),
                        "- unary at the start, after an operator and after "
                        "an open bracket; binary after an operand");
        D_TESTS_EXPECT( d_tests_parses("+a",
                                       d_tests_apply(m::expr_op_kind::pos,
                                                     a)) &&
                        d_tests_parses("a + +b",
                                       d_tests_apply(m::expr_op_kind::add, a,
                                           d_tests_apply(m::expr_op_kind::pos,
                                                         b))),
                        "+ unary and binary, as -");
        D_TESTS_EXPECT( d_tests_parses("((x))", x) &&
                        d_tests_parses("(1 + 2) * 3",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(m::expr_op_kind::add,
                                                         n1, n2),
                                           n3)),
                        "brackets group, and leave no node");

        return;
    }

    // d_tests_parse_implied
    //   G5's implied multiplication: on, a * between an operand's end and
    // the next one's start, as tight as *; never between two numbers, nor
    // before a call's bracket; off, none.
    void
    d_tests_parse_implied()
    {
        m::expr_syntax     implied;
        const m::expr_term a  = m::expr_variable("a");
        const m::expr_term b  = m::expr_variable("b");
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term y  = m::expr_variable("y");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");

        implied.implied_multiplication = true;

        D_TESTS_EXPECT( d_tests_parses("2x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     n2, x),
                                       implied) &&
                        d_tests_parses("2(x + 1)",
                                       d_tests_apply(m::expr_op_kind::mul, n2,
                                           d_tests_apply(m::expr_op_kind::add,
                                                         x, n1)),
                                       implied) &&
                        d_tests_parses("(a)(b)",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     a, b),
                                       implied) &&
                        d_tests_parses("x y",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     x, y),
                                       implied) &&
                        d_tests_parses("(a)2",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     a, n2),
                                       implied),
                        "2x, 2(x + 1), (a)(b), x y, (a)2: products");
        D_TESTS_EXPECT( d_tests_parses("a mod b",
                                       d_tests_apply(m::expr_op_kind::mod,
                                                     a, b),
                                       implied),
                        "mod an operator still: no product before it");
        D_TESTS_EXPECT( d_tests_parses("2x^2",
                                       d_tests_apply(m::expr_op_kind::mul, n2,
                                           d_tests_apply(m::expr_op_kind::pow,
                                                         x, n2)),
                                       implied) &&
                        d_tests_parses("-2x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         n2),
                                           x),
                                       implied) &&
                        d_tests_parses("1/2x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(m::expr_op_kind::div,
                                                         n1, n2),
                                           x),
                                       implied),
                        "as tight as *: 2x^2 is 2 * x ^ 2, 1/2x is (1/2) "
                        "* x");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("2 3", implied),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected '3'") &&
                        d_tests_parses("x(y)",
                                       m::expr_call("x", { y }),
                                       implied),
                        "never between two numbers, nor before a call's "
                        "bracket: x(y) is a call");
        D_TESTS_EXPECT( d_tests_parses("2pi",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     n2,
                                                     m::expr_constant("pi")),
                                       implied) &&
                        d_tests_parses("2sin(x)",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     n2,
                                                     m::expr_call("sin",
                                                                  { x })),
                                       implied) &&
                        d_tests_parses("sin(x)cos(x)",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     m::expr_call("sin",
                                                                  { x }),
                                                     m::expr_call("cos",
                                                                  { x })),
                                       implied) &&
                        d_tests_parses("3!x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(
                                               m::expr_op_kind::factorial,
                                               m::expr_number(m::value(3),
                                                              "3")),
                                           x),
                                       implied),
                        "2pi, 2sin(x), sin(x)cos(x), 3!x: products before "
                        "a constant and a call, after a call and the "
                        "factorial");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("2x"),
                                         m::expr_status::unexpected_token,
                                         1,
                                         "unexpected 'x'") &&
                        d_tests_is_error(m::expr_parse("(a)(b)"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected '('"),
                        "off: no product implied");

        return;
    }

    // d_tests_parse_calls
    //   calls, their arguments and their place in the grammar; the
    // factorial's place; the constants.
    void
    d_tests_parse_calls()
    {
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term y  = m::expr_variable("y");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");
        const m::expr_term n3 = d_tests_literal("3");
        const m::expr_term sx = m::expr_call("sin", { x });

        D_TESTS_EXPECT( d_tests_parses("sin(x)", sx) &&
                        d_tests_parses("atan2(y, x)",
                                       m::expr_call("atan2", { y, x })) &&
                        d_tests_parses("f()", m::expr_call("f", {})) &&
                        d_tests_parses("g(1, 2, 3)",
                                       m::expr_call("g", { n1, n2, n3 })),
                        "a call: its function, and its arguments, none to "
                        "three");
        D_TESTS_EXPECT( d_tests_parses("f(sin(x), g(y, (1)))",
                                       m::expr_call("f",
                                           { sx,
                                             m::expr_call("g",
                                                          { y, n1 }) })) &&
                        d_tests_parses("f(-1, -x)",
                                       m::expr_call("f",
                                           { d_tests_apply(
                                                 m::expr_op_kind::neg, n1),
                                             d_tests_apply(
                                                 m::expr_op_kind::neg,
                                                 x) })),
                        "calls inside calls and brackets; a minus after a "
                        "comma unary");
        D_TESTS_EXPECT( d_tests_parses("sin(x) ^ 2",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     sx, n2)) &&
                        d_tests_parses("-sin(x)",
                                       d_tests_apply(m::expr_op_kind::neg,
                                                     sx)) &&
                        d_tests_parses("2 * sin(x + 1)",
                                       d_tests_apply(m::expr_op_kind::mul, n2,
                                           m::expr_call("sin",
                                               { d_tests_apply(
                                                     m::expr_op_kind::add,
                                                     x, n1) }))),
                        "a call a primary: sin(x) ^ 2 is (sin(x)) ^ 2");

        const m::expr_term three_fact = d_tests_apply(
                                            m::expr_op_kind::factorial, n3);

        D_TESTS_EXPECT( d_tests_parses("2 ^ 3!",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     n2, three_fact)) &&
                        d_tests_parses("-3!",
                                       d_tests_apply(m::expr_op_kind::neg,
                                                     three_fact)) &&
                        d_tests_parses("3! ^ 2",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     three_fact, n2)) &&
                        d_tests_parses("3!!",
                                       d_tests_apply(
                                           m::expr_op_kind::factorial,
                                           three_fact)) &&
                        d_tests_parses("sin(x)!",
                                       d_tests_apply(
                                           m::expr_op_kind::factorial,
                                           sx)) &&
                        d_tests_parses("3! - 1",
                                       d_tests_apply(m::expr_op_kind::sub,
                                                     three_fact, n1)),
                        "! postfix, tighter than ^ and the unary minus; n!! "
                        "is (n!)!; a minus after it binary");
        D_TESTS_EXPECT( d_tests_parses("pi(2)",
                                       m::expr_call("pi", { n2 })) &&
                        d_tests_parses("e ^ x",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     m::expr_constant("e"),
                                                     x)) &&
                        d_tests_parses("sin",
                                       m::expr_variable("sin")),
                        "a name before a bracket a call, a constant's too; "
                        "a function's name alone a variable");
        D_TESTS_EXPECT( d_tests_parses("f[1]",
                                       m::expr_application(
                                           m::expr_op_kind::index,
                                           m::expr_variable("f"),
                                           n1)) &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { m::expr_token(
                                                   m::expr_token_kind::name,
                                                   "f",
                                                   0),
                                               m::expr_token(
                                                   m::expr_token_kind::open,
                                                   "[",
                                                   1) }),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'[' is never closed"),
                        "a name before a square bracket no call: a "
                        "variable indexed");

        return;
    }

    // d_tests_parse_bars
    //   G5's bars: |x| the call abs(x), each bar opening or closing by where
    // it stands.
    void
    d_tests_parse_bars()
    {
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term y  = m::expr_variable("y");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");
        const m::expr_term ax = m::expr_call("abs", { x });
        m::expr_syntax     bars;
        m::expr_syntax     both;

        bars.absolute_bars          = true;
        both.absolute_bars          = true;
        both.implied_multiplication = true;

        D_TESTS_EXPECT( d_tests_parses("|x - 1|",
                                       m::expr_call("abs",
                                           { d_tests_apply(
                                                 m::expr_op_kind::sub,
                                                 x, n1) }),
                                       bars) &&
                        d_tests_parses("||x| - 1|",
                                       m::expr_call("abs",
                                           { d_tests_apply(
                                                 m::expr_op_kind::sub,
                                                 ax, n1) }),
                                       bars) &&
                        d_tests_parses("|-x|",
                                       m::expr_call("abs",
                                           { d_tests_apply(
                                                 m::expr_op_kind::neg,
                                                 x) }),
                                       bars),
                        "|x - 1| is abs(x - 1); a bar opens after a bar or "
                        "an operator, and closes after an operand");
        D_TESTS_EXPECT( d_tests_parses("-|x| ^ 2",
                                       d_tests_apply(m::expr_op_kind::neg,
                                           d_tests_apply(
                                               m::expr_op_kind::pow,
                                               ax, n2)),
                                       bars) &&
                        d_tests_parses("|x|!",
                                       d_tests_apply(
                                           m::expr_op_kind::factorial, ax),
                                       bars) &&
                        d_tests_parses("f(|x|, 2)",
                                       m::expr_call("f", { ax, n2 }),
                                       bars) &&
                        d_tests_parses("|(x)|", ax, bars),
                        "bars a primary, in a call, around brackets");
        D_TESTS_EXPECT( d_tests_parses("|x| |y|",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     ax,
                                                     m::expr_call("abs",
                                                                  { y })),
                                       both) &&
                        d_tests_parses("2|x|",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     n2, ax),
                                       both) &&
                        d_tests_parses("(2|x|)",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     n2, ax),
                                       both),
                        "with implied multiplication: |x| |y| and 2|x| "
                        "products, in brackets too");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("|x, y|", bars),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected ','") &&
                        d_tests_is_error(m::expr_parse("||", bars),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'|' is never closed") &&
                        d_tests_is_error(m::expr_parse("|1 + |", bars),
                                         m::expr_status::unbalanced,
                                         5,
                                         "'|' is never closed"),
                        "no comma between bars; a bar after a bar or an "
                        "operator opens, and an open never closed is "
                        "unbalanced");

        return;
    }

    // d_tests_parse_logic
    //   2d's relations and logic in the parser layer: logic below the
    // relations (S213); a chain the conjunction of its links (S214); a
    // bracketed relation one operand (S215); the words operators wherever
    // they stand; true and false constants.
    void
    d_tests_parse_logic()
    {
        const m::expr_term a  = m::expr_variable("a");
        const m::expr_term b  = m::expr_variable("b");
        const m::expr_term c  = m::expr_variable("c");
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term y  = m::expr_variable("y");
        const m::expr_term n0 = d_tests_literal("0");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");
        const m::expr_term n3 = d_tests_literal("3");
        const m::expr_term n6 = d_tests_literal("6");
        m::expr_syntax     implied;

        implied.implied_multiplication = true;

        D_TESTS_EXPECT( d_tests_parses("x > 0 and x < 1",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::gt,
                                                         x, n0),
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         x, n1))) &&
                        d_tests_parses("a + 1 < b * 2",
                                       d_tests_apply(m::expr_op_kind::lt,
                                           d_tests_apply(m::expr_op_kind::add,
                                                         a, n1),
                                           d_tests_apply(m::expr_op_kind::mul,
                                                         b, n2))) &&
                        d_tests_parses("-a <= b",
                                       d_tests_apply(m::expr_op_kind::le,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         a),
                                           b)),
                        "the relations below the arithmetic, and logic "
                        "below the relations: x > 0 and x < 1");
        D_TESTS_EXPECT( d_tests_parses("a or b and c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_or, a,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_and,
                                               b, c))) &&
                        d_tests_parses("a and b and c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_and,
                                               a, b),
                                           c)) &&
                        d_tests_parses("a or b or c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_or,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_or,
                                               a, b),
                                           c)),
                        "and over or, each to the left");
        D_TESTS_EXPECT( d_tests_parses("not a = b",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_not,
                                           d_tests_apply(m::expr_op_kind::eq,
                                                         a, b))) &&
                        d_tests_parses("not a and b",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_not,
                                               a),
                                           b)) &&
                        d_tests_parses("not not a",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_not,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_not,
                                               a))) &&
                        d_tests_parses("a = not b",
                                       d_tests_apply(m::expr_op_kind::eq, a,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_not,
                                               b))),
                        "not between and and the relations: not a = b is "
                        "not (a = b); after an operator, at its own level");
        D_TESTS_EXPECT( d_tests_parses("a != b",
                                       d_tests_apply(m::expr_op_kind::ne,
                                                     a, b)) &&
                        d_tests_parses("a>=b",
                                       d_tests_apply(m::expr_op_kind::ge,
                                                     a, b)) &&
                        d_tests_parses("3!=6",
                                       d_tests_apply(m::expr_op_kind::ne,
                                                     n3, n6)) &&
                        d_tests_parses("3! = 6",
                                       d_tests_apply(m::expr_op_kind::eq,
                                           d_tests_apply(
                                               m::expr_op_kind::factorial,
                                               n3),
                                           n6)),
                        "the longer symbol first: 3!=6 is 3 != 6");
        D_TESTS_EXPECT( d_tests_parses("not(a)",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_not,
                                           a)) &&
                        d_tests_parses("a and(b)",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           a, b)) &&
                        d_tests_parses("a mod(b)",
                                       d_tests_apply(m::expr_op_kind::mod,
                                                     a, b)),
                        "a word before a bracket still its operator, never "
                        "a call");
        D_TESTS_EXPECT( d_tests_parses("a < b <= c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         a, b),
                                           d_tests_apply(m::expr_op_kind::le,
                                                         b, c))) &&
                        d_tests_parses("a = b = c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::eq,
                                                         a, b),
                                           d_tests_apply(m::expr_op_kind::eq,
                                                         b, c))) &&
                        d_tests_parses("a < b > c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         a, b),
                                           d_tests_apply(m::expr_op_kind::gt,
                                                         b, c))) &&
                        d_tests_parses("a >= b != c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::ge,
                                                         a, b),
                                           d_tests_apply(m::expr_op_kind::ne,
                                                         b, c))),
                        "a chain, the conjunction of its links: a = b = c "
                        "is a = b and b = c");
        D_TESTS_EXPECT( d_tests_parses("0 < 1 < 2 < 3",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_and,
                                               d_tests_apply(
                                                   m::expr_op_kind::lt,
                                                   n0, n1),
                                               d_tests_apply(
                                                   m::expr_op_kind::lt,
                                                   n1, n2)),
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         n2, n3))) &&
                        d_tests_parses("0 <= x + 1 < 2 * y",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::le,
                                               n0,
                                               d_tests_apply(
                                                   m::expr_op_kind::add,
                                                   x, n1)),
                                           d_tests_apply(m::expr_op_kind::lt,
                                               d_tests_apply(
                                                   m::expr_op_kind::add,
                                                   x, n1),
                                               d_tests_apply(
                                                   m::expr_op_kind::mul,
                                                   n2, y)))),
                        "the links joined to the left, the operand between "
                        "two in both");
        D_TESTS_EXPECT( d_tests_parses("(a < b) < c",
                                       d_tests_apply(m::expr_op_kind::lt,
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         a, b),
                                           c)) &&
                        d_tests_parses("a < (b < c)",
                                       d_tests_apply(m::expr_op_kind::lt, a,
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         b, c))) &&
                        d_tests_parses("(a < b) = (b < c)",
                                       d_tests_apply(m::expr_op_kind::eq,
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         a, b),
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         b, c))),
                        "a bracketed relation one operand, no link of a "
                        "chain");
        D_TESTS_EXPECT( d_tests_parses("not a < b < c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_not,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_and,
                                               d_tests_apply(
                                                   m::expr_op_kind::lt,
                                                   a, b),
                                               d_tests_apply(
                                                   m::expr_op_kind::lt,
                                                   b, c)))) &&
                        d_tests_parses("f(a = b = c)",
                                       m::expr_call("f", {
                                           d_tests_apply(
                                               m::expr_op_kind::logical_and,
                                               d_tests_apply(
                                                   m::expr_op_kind::eq,
                                                   a, b),
                                               d_tests_apply(
                                                   m::expr_op_kind::eq,
                                                   b, c)) })) &&
                        d_tests_parses("a < not b < c",
                                       d_tests_apply(m::expr_op_kind::lt, a,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_not,
                                               d_tests_apply(
                                                   m::expr_op_kind::lt,
                                                   b, c)))) &&
                        d_tests_parses("a < not b < c < x",
                                       d_tests_apply(m::expr_op_kind::lt, a,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_not,
                                               d_tests_apply(
                                                   m::expr_op_kind::
                                                       logical_and,
                                                   d_tests_apply(
                                                       m::expr_op_kind::lt,
                                                       b, c),
                                                   d_tests_apply(
                                                       m::expr_op_kind::lt,
                                                       c, x))))),
                        "a chain under not, or in an argument, joined "
                        "there; not ends a chain");
        D_TESTS_EXPECT( d_tests_parses("2x < 3",
                                       d_tests_apply(m::expr_op_kind::lt,
                                           d_tests_apply(m::expr_op_kind::mul,
                                                         n2, x),
                                           n3),
                                       implied) &&
                        d_tests_parses("x y = y x",
                                       d_tests_apply(m::expr_op_kind::eq,
                                           d_tests_apply(m::expr_op_kind::mul,
                                                         x, y),
                                           d_tests_apply(m::expr_op_kind::mul,
                                                         y, x)),
                                       implied) &&
                        d_tests_is_error(m::expr_parse("2 not x", implied),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected 'not'"),
                        "implied multiplication around the relations, and "
                        "never at a word");
        D_TESTS_EXPECT( d_tests_parses("true or false",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_or,
                                           m::expr_constant("true"),
                                           m::expr_constant("false"))) &&
                        d_tests_parses("true(1)",
                                       m::expr_call("true", { n1 })),
                        "true and false constants; before a bracket, a "
                        "call");

        return;
    }

    // d_tests_parse_piecewise
    //   piecewise in the vocabulary of math/function/function.hpp: its
    // branches read into its arguments (S216), and their errors at their
    // places.
    void
    d_tests_parse_piecewise()
    {
        const m::expr_term a  = m::expr_variable("a");
        const m::expr_term b  = m::expr_variable("b");
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term n0 = d_tests_literal("0");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");
        const m::expr_term n3 = d_tests_literal("3");
        const m::expr_term n5 = d_tests_literal("5");

        D_TESTS_EXPECT( d_tests_parses(
                            "piecewise(when(x < 0, -x), otherwise(x))",
                            m::expr_call("piecewise", {
                                d_tests_apply(m::expr_op_kind::lt, x, n0),
                                d_tests_apply(m::expr_op_kind::neg, x),
                                x })) &&
                        d_tests_parses("piecewise(when(a, 1), when(b, 2))",
                                       m::expr_call("piecewise",
                                                    { a, n1, b, n2 })) &&
                        d_tests_parses("piecewise(otherwise(5))",
                                       m::expr_call("piecewise", { n5 })) &&
                        d_tests_parses("piecewise()",
                                       m::expr_call("piecewise", {})),
                        "piecewise(when(c, v), otherwise(d)): the call "
                        "piecewise(c, v, d)");
        D_TESTS_EXPECT( d_tests_parses(
                            "piecewise(when(a, piecewise(when(b, 1), "
                            "otherwise(2))), otherwise(3))",
                            m::expr_call("piecewise", {
                                a,
                                m::expr_call("piecewise", { b, n1, n2 }),
                                n3 })) &&
                        d_tests_parses("when + otherwise",
                                       d_tests_apply(m::expr_op_kind::add,
                                           m::expr_variable("when"),
                                           m::expr_variable("otherwise"))),
                        "piecewise in a branch's value; when and otherwise "
                        "with no bracket, variables");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("piecewise(x)"),
                                         m::expr_status::unexpected_token,
                                         10,
                                         "an argument of piecewise is "
                                         "when(c, v) or otherwise(v)") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(when(a, 1), x + 1)"),
                                         m::expr_status::unexpected_token,
                                         22,
                                         "an argument of piecewise is "
                                         "when(c, v) or otherwise(v)"),
                        "an argument of piecewise no branch: unexpected, at "
                        "its first token");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("piecewise(when(a))"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "when takes 2 arguments, not 1") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(when(a, b, x))"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "when takes 2 arguments, not 3") &&
                        d_tests_is_error(m::expr_parse("piecewise(when())"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "when takes 2 arguments, not 0") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(otherwise())"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "otherwise takes 1 argument, not "
                                         "0") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(otherwise(1, 2))"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "otherwise takes 1 argument, not "
                                         "2"),
                        "a branch of a number of arguments it does not take: "
                        "wrong_arity, at the branch");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse(
                                             "piecewise(otherwise(1), "
                                             "when(a, 2))"),
                                         m::expr_status::unexpected_token,
                                         10,
                                         "otherwise is the last branch of "
                                         "piecewise"),
                        "an otherwise before the last argument: unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("when(a, b)"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "when stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse("1 + otherwise(a)"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "otherwise stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(when(a, b) + 1)"),
                                         m::expr_status::unexpected_token,
                                         10,
                                         "when stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise((when(a, b)))"),
                                         m::expr_status::unexpected_token,
                                         11,
                                         "when stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(-when(a, b))"),
                                         m::expr_status::unexpected_token,
                                         11,
                                         "when stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse("f(when(a, b))"),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "when stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(when(otherwise(1), "
                                             "2))"),
                                         m::expr_status::unexpected_token,
                                         15,
                                         "otherwise stands only as a whole "
                                         "argument of piecewise"),
                        "a when or an otherwise anywhere else: unexpected, "
                        "at its name");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse(
                                             "piecewise(when(a), x)"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "when takes 2 arguments, not 1") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(when(a +, 1))"),
                                         m::expr_status::missing_operand,
                                         18,
                                         "an operand is missing before "
                                         "','") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(x, when(a +, 1))"),
                                         m::expr_status::unexpected_token,
                                         10,
                                         "an argument of piecewise is "
                                         "when(c, v) or otherwise(v)"),
                        "of two errors the further left; a branch's own "
                        "error, at its place");

        return;
    }

    // d_tests_parse_tokens
    //   expr_parse_tokens: the same terms from tokens; an end token ends
    // them; a token of no role, unexpected.
    void
    d_tests_parse_tokens()
    {
        typedef m::expr_token tok;
        const m::expr_token_kind  number = m::expr_token_kind::number;
        const m::expr_token_kind  name   = m::expr_token_kind::name;
        const m::expr_token_kind  symbol = m::expr_token_kind::symbol;
        const std::vector<tok>    sum    = { tok(name, "x", 0),
                                             tok(symbol, "+", 2),
                                             tok(number, "1", 4) };
        std::vector<tok>          ended  = sum;

        ended.push_back(tok());
        ended.back().offset = 5;

        const m::expr_result<m::expr_term> from_tokens =
            m::expr_parse_tokens(sum);
        const m::expr_result<m::expr_term> from_ended =
            m::expr_parse_tokens(ended);

        D_TESTS_EXPECT( (from_tokens.is_ok()) &&
                        (d_tests_same(from_tokens.value(),
                                      d_tests_x_plus_1())) &&
                        (from_ended.is_ok()) &&
                        (d_tests_same(from_ended.value(),
                                      d_tests_x_plus_1())),
                        "x + 1 from its tokens, with an end token or not");

        ended.push_back(tok(number, "2", 7));

        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(ended),
                                         m::expr_status::unexpected_token,
                                         7,
                                         "unexpected '2'"),
                        "a token after an end token: unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(
                                             { tok(number, "1/2", 0) }),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '1/2'") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(number, "-1", 0) }),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '-1'") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(symbol, "%", 0) }),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '%'") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(m::expr_token_kind::open,
                                                   "{",
                                                   0) }),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '{'"),
                        "a number no unsigned decimal, and a symbol and a "
                        "bracket of no role: unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(
                                             { tok(symbol, "|", 0) }),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '|'") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(m::expr_token_kind::comma,
                                                   ",",
                                                   3) }),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected ','") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(m::expr_token_kind::
                                                       semicolon,
                                                   ";",
                                                   4) }),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected ';'"),
                        "| with the bars off, no operator; a comma outside "
                        "a call, and a semicolon outside a literal, "
                        "unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(
                                             { tok(static_cast<
                                                       m::expr_token_kind>(
                                                       99),
                                                   "?",
                                                   1) }),
                                         m::expr_status::unexpected_token,
                                         1,
                                         "unexpected '?'"),
                        "a token of no kind: unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(
                                             std::vector<tok>()),
                                         m::expr_status::missing_operand,
                                         0,
                                         "an operand is missing at the "
                                         "end") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(number, "1", 0),
                                               tok(symbol, "+", 2) }),
                                         m::expr_status::missing_operand,
                                         3,
                                         "an operand is missing at the "
                                         "end"),
                        "the end past the last token's text");

        return;
    }

    // d_tests_parse_errors
    //   each error at its place: the first a parse meets, a token of a later
    // step, then a bracket, then the grammar.
    void
    d_tests_parse_errors()
    {
        m::expr_syntax bars;

        bars.absolute_bars = true;

        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse(""),
                                         m::expr_status::missing_operand,
                                         0,
                                         "an operand is missing at the "
                                         "end") &&
                        d_tests_is_error(m::expr_parse("1 +  "),
                                         m::expr_status::missing_operand,
                                         5,
                                         "an operand is missing at the "
                                         "end") &&
                        d_tests_is_error(m::expr_parse("x mod"),
                                         m::expr_status::missing_operand,
                                         5,
                                         "an operand is missing at the "
                                         "end"),
                        "the text ending where an operand must stand");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("()"),
                                         m::expr_status::missing_operand,
                                         1,
                                         "an operand is missing before "
                                         "')'") &&
                        d_tests_is_error(m::expr_parse("(1 +)"),
                                         m::expr_status::missing_operand,
                                         4,
                                         "an operand is missing before "
                                         "')'"),
                        "a bracket closing where an operand must stand");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("(1 +"),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'(' is never closed") &&
                        d_tests_is_error(m::expr_parse("(1 + (2"),
                                         m::expr_status::unbalanced,
                                         5,
                                         "'(' is never closed") &&
                        d_tests_is_error(m::expr_parse("1 + 2)"),
                                         m::expr_status::unbalanced,
                                         5,
                                         "')' closes no bracket") &&
                        d_tests_is_error(m::expr_parse(")("),
                                         m::expr_status::unbalanced,
                                         0,
                                         "')' closes no bracket"),
                        "unbalanced: a close that opens nothing, or the "
                        "innermost open never closed");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("(1 2)"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected '2'") &&
                        d_tests_is_error(m::expr_parse("1 2"),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected '2'") &&
                        d_tests_is_error(m::expr_parse("* 2"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '*'") &&
                        d_tests_is_error(m::expr_parse("1 * * 2"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '*'") &&
                        d_tests_is_error(m::expr_parse("mod x"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected 'mod'") &&
                        d_tests_is_error(m::expr_parse("1.2.3"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected '.3'"),
                        "a token out of place: unexpected, at its offset");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("1 + #"),
                                         m::expr_status::bad_character,
                                         4,
                                         "no token begins with '#'"),
                        "a character no token begins with, from the "
                        "tokenizer");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("sin(,)"),
                                         m::expr_status::missing_operand,
                                         4,
                                         "an operand is missing before "
                                         "','") &&
                        d_tests_is_error(m::expr_parse("f(1,)"),
                                         m::expr_status::missing_operand,
                                         4,
                                         "an operand is missing before "
                                         "')'") &&
                        d_tests_is_error(m::expr_parse("f(1 +)"),
                                         m::expr_status::missing_operand,
                                         5,
                                         "an operand is missing before "
                                         "')'"),
                        "an argument missing, or ending where an operand "
                        "must stand: before its comma or its close");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("(1, 2)"),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected ','") &&
                        d_tests_is_error(m::expr_parse("1, 2"),
                                         m::expr_status::unexpected_token,
                                         1,
                                         "unexpected ','") &&
                        d_tests_is_error(m::expr_parse("!x"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '!'") &&
                        d_tests_is_error(m::expr_parse("f (x"),
                                         m::expr_status::unbalanced,
                                         2,
                                         "'(' is never closed"),
                        "a comma outside a call's brackets, and the "
                        "factorial before an operand: unexpected; a call "
                        "never closed: unbalanced");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("|x", bars),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'|' is never closed") &&
                        d_tests_is_error(m::expr_parse("(|x)", bars),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'|' is never closed") &&
                        d_tests_is_error(m::expr_parse("|x| |y|", bars),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '|'"),
                        "a bar never closed, or open where a bracket "
                        "closes: unbalanced; a bar after an operand, with "
                        "no product implied: unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("1 + * 2 + sin(,)"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '*'") &&
                        d_tests_is_error(m::expr_parse("sin(,) + 1 + * 2"),
                                         m::expr_status::missing_operand,
                                         4,
                                         "an operand is missing before "
                                         "','"),
                        "of an argument's error and the climb's, the "
                        "further left");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("a <"),
                                         m::expr_status::missing_operand,
                                         3,
                                         "an operand is missing at the "
                                         "end") &&
                        d_tests_is_error(m::expr_parse("< a"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '<'") &&
                        d_tests_is_error(m::expr_parse("a < < b"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '<'") &&
                        d_tests_is_error(m::expr_parse("a and"),
                                         m::expr_status::missing_operand,
                                         5,
                                         "an operand is missing at the "
                                         "end") &&
                        d_tests_is_error(m::expr_parse("a not b"),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected 'not'") &&
                        d_tests_is_error(m::expr_parse("or b"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected 'or'"),
                        "a relation or a word out of place: unexpected, or "
                        "an operand missing");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("'"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '''") &&
                        d_tests_is_error(m::expr_parse("x + '"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '''") &&
                        d_tests_is_error(m::expr_parse("(')"),
                                         m::expr_status::unexpected_token,
                                         1,
                                         "unexpected '''"),
                        "a transpose where no operand ends before it: "
                        "unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(
                                             { m::expr_token(
                                                   m::expr_token_kind::open,
                                                   "[",
                                                   0) }),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'[' is never closed") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { m::expr_token(
                                                   m::expr_token_kind::close,
                                                   "]",
                                                   2) }),
                                         m::expr_status::unbalanced,
                                         2,
                                         "']' closes no bracket"),
                        "a square bracket alone: unbalanced");

        return;
    }

    // d_tests_render
    //   expr_render: G5's spellings, brackets only where operands regroup,
    // literals as written.
    void
    d_tests_render()
    {
        m::expr_syntax bars;

        bars.absolute_bars = true;

        D_TESTS_EXPECT( d_tests_renders("2*x^2+1", "2 * x ^ 2 + 1") &&
                        d_tests_renders("x mod 3", "x mod 3") &&
                        d_tests_renders("((x))", "x"),
                        "infix with a space each side; no bracket kept "
                        "that needs none");
        D_TESTS_EXPECT( d_tests_renders("-x^2", "-x ^ 2") &&
                        d_tests_renders("(-x)^2", "(-x) ^ 2") &&
                        d_tests_renders("2^-3", "2 ^ (-3)") &&
                        d_tests_renders("-(a*b)", "-(a * b)") &&
                        d_tests_renders("--x", "--x") &&
                        d_tests_renders("a--b", "a - -b") &&
                        d_tests_renders("a*+b", "a * +b"),
                        "the unary - and + against their operands, "
                        "bracketed where they would regroup");
        D_TESTS_EXPECT( d_tests_renders("a-(b-c)", "a - (b - c)") &&
                        d_tests_renders("(a-b)-c", "a - b - c") &&
                        d_tests_renders("(a^b)^c", "(a ^ b) ^ c") &&
                        d_tests_renders("a^(b^c)", "a ^ b ^ c") &&
                        d_tests_renders("a/(b*c)", "a / (b * c)") &&
                        d_tests_renders("(1+2)*3", "(1 + 2) * 3"),
                        "brackets on the side an operator does not group "
                        "to");
        D_TESTS_EXPECT( d_tests_renders("0.10 + 1e3 + 007",
                                        "0.10 + 1e3 + 007"),
                        "literals as written");
        D_TESTS_EXPECT( (m::expr_render(m::expr_application(
                             m::expr_op_kind::pow,
                             m::expr_number(m::value(-1), "-1"),
                             d_tests_literal("2"))) == "(-1) ^ 2") &&
                        (m::expr_render(m::expr_number(m::value::ratio(1, 3),
                                                       "1/3")) == "(1/3)"),
                        "a literal no unsigned decimal, in brackets");
        D_TESTS_EXPECT( d_tests_renders("sin(x)", "sin(x)") &&
                        d_tests_renders("atan2(y,x)", "atan2(y, x)") &&
                        d_tests_renders("f()", "f()") &&
                        d_tests_renders("foo(a+b,-c)", "foo(a + b, -c)") &&
                        d_tests_renders("sin(x)^2", "sin(x) ^ 2") &&
                        d_tests_renders("-f(x)", "-f(x)"),
                        "a call: its function's name and its arguments, a "
                        "primary, never bracketed");
        D_TESTS_EXPECT( d_tests_renders("n!", "n!") &&
                        d_tests_renders("(x+1)!", "(x + 1)!") &&
                        d_tests_renders("(-x)!", "(-x)!") &&
                        d_tests_renders("x!!", "x!!") &&
                        d_tests_renders("2^3!", "2 ^ 3!") &&
                        d_tests_renders("(2^3)!", "(2 ^ 3)!") &&
                        d_tests_renders("-3!", "-3!"),
                        "the factorial after its operand, bracketed where "
                        "it would regroup");
        D_TESTS_EXPECT( d_tests_renders("2*pi*e", "2 * pi * e") &&
                        d_tests_renders("|x-1|", "abs(x - 1)", bars),
                        "a constant its name; bars the call abs");
        D_TESTS_EXPECT( (m::expr_render(m::expr_term()).empty()) &&
                        (m::expr_render(d_tests_apply(
                             m::expr_op_kind::none,
                             d_tests_literal("1"),
                             d_tests_literal("2"))) == "?(1, 2)") &&
                        (m::expr_render(m::expr_application(
                             m::expr_op_kind::index,
                             std::vector<m::expr_term>())) == "?()"),
                        "a term made by default, the empty string; an "
                        "operator of no kind the table has, ?(1, 2), "
                        "indexing of no operand among them");
        D_TESTS_EXPECT( d_tests_renders("a<b", "a < b") &&
                        d_tests_renders("a!=b", "a != b") &&
                        d_tests_renders("a<=b", "a <= b") &&
                        d_tests_renders("a>=b", "a >= b") &&
                        d_tests_renders("a=b", "a = b") &&
                        d_tests_renders("a>b", "a > b") &&
                        d_tests_renders("x > 0 and x < 1",
                                        "x > 0 and x < 1") &&
                        d_tests_renders("a or b and c", "a or b and c") &&
                        d_tests_renders("(a or b) and c", "(a or b) and c"),
                        "the relations and logic as G5 spells them, "
                        "bracketed only where they would regroup");
        D_TESTS_EXPECT( d_tests_renders("not(a)", "not a") &&
                        d_tests_renders("not (a and b)", "not (a and b)") &&
                        d_tests_renders("not a = b", "not a = b") &&
                        d_tests_renders("-(not a)", "-(not a)") &&
                        d_tests_renders("not -a", "not -a") &&
                        d_tests_renders("a = not b", "a = (not b)"),
                        "not before its operand, spaced as a word");
        D_TESTS_EXPECT( d_tests_renders("(a < b) < c", "(a < b) < c") &&
                        d_tests_renders("a < (b < c)", "a < (b < c)") &&
                        d_tests_renders("a < b <= c", "a < b and b <= c") &&
                        d_tests_renders("true and false", "true and false"),
                        "a relation of a relation, bracketed; a chain, as "
                        "its links");
        D_TESTS_EXPECT( d_tests_renders("piecewise(when(x<0,-x),otherwise(x))",
                                        "piecewise(when(x < 0, -x), "
                                        "otherwise(x))") &&
                        (m::expr_render(m::expr_call(
                             "piecewise",
                             { m::expr_variable("a"),
                               m::expr_variable("b"),
                               m::expr_variable("c") })) ==
                             "piecewise(when(a, b), otherwise(c))") &&
                        (m::expr_render(m::expr_call(
                             "piecewise",
                             { m::expr_variable("a"),
                               m::expr_variable("b") })) ==
                             "piecewise(when(a, b))") &&
                        (m::expr_render(m::expr_call("piecewise", {})) ==
                             "piecewise()"),
                        "piecewise in its branches: its arguments two by "
                        "two, an odd last one otherwise");

        return;
    }

    // d_tests_renders_number
    //   whether a number with no literal renders as _want.
    bool
    d_tests_renders_number(
        const m::value& _number,
        const char*     _want
    )
    {
        return (m::expr_render(m::expr_number(_number)) == _want);
    }

    // d_tests_render_numbers
    //   numbers a fold made: exact where exact, the shortest that reads back
    // where floating, and names for what has no literal.
    void
    d_tests_render_numbers()
    {
        const double inf = std::numeric_limits<double>::infinity();

        D_TESTS_EXPECT( d_tests_renders_number(m::value(5), "5") &&
                        d_tests_renders_number(m::value(-5), "(-5)") &&
                        d_tests_renders_number(
                            m::value(std::numeric_limits<dj_math_imax>::min()),
                            "(-9223372036854775807 - 1)") &&
                        d_tests_renders_number(m::value::ratio(1, 3),
                                               "(1/3)") &&
                        d_tests_renders_number(m::value::ratio(-1, 3),
                                               "(-1/3)") &&
                        d_tests_renders_number(
                            m::value::ratio(
                                std::numeric_limits<dj_math_imax>::min(),
                                3),
                            "((-9223372036854775807 - 1)/3)"),
                        "exact: an integer, a fraction, in brackets where "
                        "negative or a fraction; the least numerator as one "
                        "more less 1");
        D_TESTS_EXPECT( d_tests_renders_number(m::value(0.1), "0.1") &&
                        d_tests_renders_number(m::value(0.1f), "0.1") &&
                        d_tests_renders_number(m::value(0.1L), "0.1") &&
                        d_tests_renders_number(m::value(-2.5), "(-2.5)") &&
                        d_tests_renders_number(m::value(1e30), "1e+30") &&
                        d_tests_renders_number(m::value(-0.0), "(-0)"),
                        "floating: the shortest that reads back in its "
                        "type");
        D_TESTS_EXPECT( d_tests_renders_number(m::value(inf), "inf") &&
                        d_tests_renders_number(m::value(-inf), "(-inf)") &&
                        d_tests_renders_number(
                            m::value(std::numeric_limits<double>::quiet_NaN()),
                            "nan") &&
                        d_tests_renders_number(
                            m::value(std::numeric_limits<float>::
                                         infinity()),
                            "inf") &&
                        d_tests_renders_number(
                            m::value(-std::numeric_limits<long double>::
                                          infinity()),
                            "(-inf)") &&
                        d_tests_renders_number(
                            m::value(std::numeric_limits<float>::
                                         quiet_NaN()),
                            "nan") &&
                        d_tests_renders_number(
                            m::value(std::numeric_limits<long double>::
                                         quiet_NaN()),
                            "nan") &&
                        d_tests_renders_number(m::value(true), "true") &&
                        d_tests_renders_number(m::value(false), "false") &&
                        d_tests_renders_number(m::value(), "none"),
                        "inf, nan, the booleans and none by name");
        D_TESTS_EXPECT( d_tests_renders_number(
                            m::value(D_MATH_VALUE_ERROR_DOMAIN),
                            "domain_error") &&
                        d_tests_renders_number(
                            m::value(D_MATH_VALUE_ERROR_POLE),
                            "pole_error") &&
                        d_tests_renders_number(
                            m::value(D_MATH_VALUE_ERROR_RANGE),
                            "range_error") &&
                        d_tests_renders_number(
                            m::value(D_MATH_VALUE_ERROR_INVALID),
                            "invalid_error") &&
                        d_tests_renders_number(
                            m::value(D_MATH_VALUE_ERROR_MEMORY),
                            "memory_error"),
                        "each error by name");
        D_TESTS_EXPECT( d_tests_renders_number(
                            d_tests_wide_of("123456789012345678901234567890"),
                            "123456789012345678901234567890") &&
                        d_tests_renders_number(
                            d_tests_value_of("-x",
                                             m::at("x",
                                                   d_tests_wide_of(
                                                       "12345678901234567890"
                                                       "1"))),
                            "(-123456789012345678901)"),
                        "a boxed number as its kind writes it, in brackets "
                        "where negative");

        return;
    }


    // d_tests_rng
    //   struct: a small generator of the test's own, a 64-bit linear
    // congruential one, so that every build draws the same terms.
    struct d_tests_rng
    {
        dj_math_umax m_state;

        explicit d_tests_rng(
            dj_math_umax _seed
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
            const dj_math_umax word = 0xFFFFFFFFFFFFFFFFu;

            m_state = ( (m_state * 6364136223846793005u) +
                        1442695040888963407u ) & word;

            return static_cast<unsigned>((m_state >> 33) % _n);
        }
    };

    // d_tests_alphabet
    //   struct: what a random term is drawn from: literals, names and the
    // kinds of operator; and, where any, constants and the functions of
    // calls.
    struct d_tests_alphabet
    {
        const char* const*     m_literals;
        unsigned               m_literal_count;
        const char* const*     m_names;
        unsigned               m_name_count;
        const m::expr_op_kind* m_kinds;
        unsigned               m_kind_count;
        const char* const*     m_constants;
        unsigned               m_constant_count;
        const char* const*     m_functions;
        unsigned               m_function_count;
    };

    // d_tests_random_term
    //   a term drawn from _alphabet, at most _depth operators deep; its left
    // operand drawn before its right, so that the draw is one on every
    // compiler.
    m::expr_term
    d_tests_random_term(
        d_tests_rng&            _rng,
        const d_tests_alphabet& _alphabet,
        unsigned                _depth
    )
    {
        const unsigned leaves = ( (_alphabet.m_constant_count != 0) ? 3u
                                                                    : 2u );
        const unsigned calls  = ( (_alphabet.m_function_count != 0) ? 1u
                                                                    : 0u );
        const unsigned pick   = _rng.below( (_depth == 0)
                                                ? leaves
                                                : (leaves + 4u + calls) );

        // a leaf: a literal, a name, or a constant
        if (pick == 0)
        {
            return d_tests_literal(
                _alphabet.m_literals[_rng.below(_alphabet.m_literal_count)]);
        }

        if (pick == 1)
        {
            return m::expr_variable(
                _alphabet.m_names[_rng.below(_alphabet.m_name_count)]);
        }

        if (pick + 1 == leaves)
        {
            return m::expr_constant(
                _alphabet.m_constants[_rng.below(
                    _alphabet.m_constant_count)]);
        }

        // a call, of none to three arguments, each drawn in turn
        if (pick == leaves + 4u)
        {
            const char* const      function =
                _alphabet.m_functions[_rng.below(
                    _alphabet.m_function_count)];
            const unsigned         count    = _rng.below(4u);
            std::vector<m::expr_term> arguments;

            for (unsigned i = 0; i < count; ++i)
            {
                arguments.push_back(d_tests_random_term(_rng,
                                                        _alphabet,
                                                        _depth - 1));
            }

            return m::expr_call(function, arguments);
        }

        const m::expr_op_kind kind =
            _alphabet.m_kinds[_rng.below(_alphabet.m_kind_count)];
        const m::expr_term    left = d_tests_random_term(_rng,
                                                         _alphabet,
                                                         _depth - 1);

        // the unary, not and the factorial, on one operand
        if ( (kind == m::expr_op_kind::neg) ||
             (kind == m::expr_op_kind::pos) ||
             (kind == m::expr_op_kind::logical_not) ||
             (kind == m::expr_op_kind::factorial) )
        {
            return d_tests_apply(kind, left);
        }

        const m::expr_term right = d_tests_random_term(_rng,
                                                       _alphabet,
                                                       _depth - 1);

        return d_tests_apply(kind, left, right);
    }

    // d_tests_language
    //   the alphabet of 2d's language, calls, constants, the factorial, the
    // relations, logic and piecewise among it: what the round trip, and the
    // handle's agreement with the entries, draw their terms from.
    d_tests_alphabet
    d_tests_language()
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
            m::expr_op_kind::neg, m::expr_op_kind::pos,
            m::expr_op_kind::factorial, m::expr_op_kind::eq,
            m::expr_op_kind::ne, m::expr_op_kind::lt, m::expr_op_kind::le,
            m::expr_op_kind::gt, m::expr_op_kind::ge,
            m::expr_op_kind::logical_and, m::expr_op_kind::logical_or,
            m::expr_op_kind::logical_not };
        static const char* const     constants[] = { "pi", "e", "true",
                                                     "false" };
        static const char* const     functions[] = { "sin", "atan2", "f",
                                                     "g_2", "piecewise",
                                                     "all_of" };
        const d_tests_alphabet       alphabet    = { literals, 12,
                                                     names, 6,
                                                     kinds, 18,
                                                     constants, 4,
                                                     functions, 6 };

        return alphabet;
    }

    // d_tests_round_trip
    //   roadmap Phase 2's round trip: random terms of 2d's language
    // (d_tests_language) rendered and parsed back as themselves, implied
    // multiplication on or off, and rendered the same again.
    void
    d_tests_round_trip()
    {
        const d_tests_alphabet alphabet     = d_tests_language();
        d_tests_rng            rng(20261023u);
        m::expr_syntax         implied;
        std::size_t            back         = 0;
        std::size_t            back_implied = 0;
        std::size_t            stable       = 0;

        implied.implied_multiplication = true;

        // 2,000 terms, up to five operators deep
        for (unsigned i = 0; i < 2000; ++i)
        {
            const m::expr_term term = d_tests_random_term(rng,
                                                          alphabet,
                                                          1 + (i % 5));
            const std::string  text = m::expr_render(term);
            const m::expr_result<m::expr_term> parsed = m::expr_parse(text);
            const m::expr_result<m::expr_term> parsed_implied =
                m::expr_parse(text, implied);

            back         += ( ( (parsed.is_ok()) &&
                                (d_tests_same(parsed.value(), term)) )
                                  ? 1 : 0 );
            back_implied += ( ( (parsed_implied.is_ok()) &&
                                (d_tests_same(parsed_implied.value(),
                                              term)) )
                                  ? 1 : 0 );
            stable       += ( ( (parsed.is_ok()) &&
                                (m::expr_render(parsed.value()) == text) )
                                  ? 1 : 0 );
        }

        D_TESTS_EXPECT( (back == 2000),
                        "2,000 random terms rendered and parsed back as "
                        "themselves");
        D_TESTS_EXPECT( (back_implied == 2000),
                        "the same with implied multiplication on");
        D_TESTS_EXPECT( (stable == 2000),
                        "each parsed back renders as it did");

        return;
    }

    // d_tests_folds_agree
    //   random terms of whole numbers under + - * and the unary - and +:
    // each type's value the exact one, as each is exact there.
    void
    d_tests_folds_agree()
    {
        static const char* const     literals[] = { "0", "1", "2", "3", "4",
                                                    "5", "6", "7", "8",
                                                    "9" };
        static const char* const     names[]    = { "a", "b" };
        static const m::expr_op_kind kinds[]    = {
            m::expr_op_kind::add, m::expr_op_kind::sub, m::expr_op_kind::mul,
            m::expr_op_kind::neg, m::expr_op_kind::pos };
        const d_tests_alphabet       alphabet   = { literals, 10,
                                                    names, 2,
                                                    kinds, 5,
                                                    D_NULLPTR, 0,
                                                    D_NULLPTR, 0 };
        const m::expr_bindings       at         = m::at("a", 7).at("b", -3);
        d_tests_rng                  rng(1429u);
        std::size_t                  agree      = 0;

        // 1,000 terms, up to three operators deep, each value below 2^53
        for (unsigned i = 0; i < 1000; ++i)
        {
            const m::expr_term term = d_tests_random_term(rng,
                                                          alphabet,
                                                          1 + (i % 3));
            const m::expr_result<m::value> exact =
                m::expr_evaluate(term, at, m::expr_context());
            const m::expr_result<double> in_double =
                m::expr_evaluate<double>(term, at);
            const m::expr_result<long double> in_long =
                m::expr_evaluate<long double>(term, at);
            dj_math_imax whole = 0;

            agree += ( ( (exact.is_ok()) &&
                         (exact.value().to(whole)) &&
                         (in_double.is_ok()) &&
                         (d_tests_same_real(in_double.value() + 0.0,
                                            static_cast<double>(whole))) &&
                         (in_long.is_ok()) &&
                         (d_tests_same_real(in_long.value() + 0.0L,
                                            static_cast<long double>(
                                                whole))) )
                           ? 1 : 0 );
        }

        D_TESTS_EXPECT( (agree == 1000),
                        "1,000 random whole terms: double's and long "
                        "double's values the exact ones");

        return;
    }

    // d_tests_numbers_read_back
    //   a number a fold made, rendered and parsed back: the same number,
    // exactly where exact, in its type where floating.
    void
    d_tests_numbers_read_back()
    {
        const m::value exact[] = {
            m::value(0), m::value(5), m::value(-5),
            m::value(std::numeric_limits<dj_math_imax>::min()),
            m::value::ratio(1, 3), m::value::ratio(-7, 2),
            m::value::ratio(std::numeric_limits<dj_math_imax>::min(), 3) };
        const double   doubles[] = { 0.1, -2.5, 1e30, 1e-310,
                                     std::numeric_limits<double>::max() };
        const float    floats[]  = { 0.1f, 3.4028235e38f, -1e-40f };
        std::size_t    same      = 0;

        // exact
        for (std::size_t i = 0; i < 7; ++i)
        {
            const std::string text = m::expr_render(m::expr_number(exact[i]));

            same += ( d_tests_valued(text.c_str(), exact[i]) ? 1 : 0 );
        }

        // floating, in their own types
        for (std::size_t i = 0; i < 5; ++i)
        {
            const std::string text = m::expr_render(m::expr_number(
                                         m::value(doubles[i])));

            same += ( d_tests_typed<double>(text.c_str(), doubles[i])
                          ? 1 : 0 );
        }

        for (std::size_t i = 0; i < 3; ++i)
        {
            const std::string text = m::expr_render(m::expr_number(
                                         m::value(floats[i])));

            same += ( d_tests_typed<float>(text.c_str(), floats[i]) ? 1 : 0 );
        }

        const long double longs[] = { 0.1L,
                                      std::numeric_limits<long double>::max(),
                                      -std::numeric_limits<long double>::
                                          denorm_min() };

        for (std::size_t i = 0; i < 3; ++i)
        {
            const std::string text = m::expr_render(m::expr_number(
                                         m::value(longs[i])));

            same += ( d_tests_typed<long double>(text.c_str(), longs[i])
                          ? 1 : 0 );
        }

        D_TESTS_EXPECT( (same == 18),
                        "numbers a fold made read back as the same number: "
                        "exactly where exact, in their types where "
                        "floating");

        return;
    }


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


    //==========================================================================
    // 11. MATRICES
    //==========================================================================

    // d_tests_same_values
    //   whether two values are the same, as two atoms' numbers are
    // (expr_same_number): of one kind, and equal, a matrix by its kind's
    // comparison.
    bool
    d_tests_same_values(
        const m::value& _a,
        const m::value& _b
    )
    {
        return m::internal::expr_same_number(_a, _b);
    }

    // d_tests_holds_value
    //   whether a fold gave a value, the same as _want.
    bool
    d_tests_holds_value(
        const m::expr_result<m::value>& _result,
        const m::value&                 _want
    )
    {
        return ( (_result.is_ok()) &&
                 (d_tests_same_values(_result.value(), _want)) );
    }

    // d_tests_made
    //   what a step made, where it made a value; none where it failed, so
    // that a check of it fails rather than read what the step does not
    // hold.
    m::value
    d_tests_made(
        const m::expr_result<m::value>& _result
    )
    {
        return ( _result.is_ok() ? _result.value() : m::value() );
    }

    // d_tests_shape
    //   a value's shape, where it is a matrix; 0 by 0 where it is not.
    m::expr_shape
    d_tests_shape(
        const m::value& _x
    )
    {
        const m::expr_result<m::expr_shape> shape = m::expr_shape_of(_x);

        return ( shape.is_ok() ? shape.value() : m::expr_shape() );
    }

    // d_tests_storage
    //   a matrix's storage, as a number; 0, no storage's, where the value
    // is no matrix.
    int
    d_tests_storage(
        const m::value& _x
    )
    {
        return ( m::expr_is_matrix(_x)
                     ? static_cast<int>(m::internal::expr_matrix_box_of(
                           _x.c_value()).storage)
                     : 0 );
    }

    // d_tests_stored_as
    //   whether a value is a matrix of the storage given.
    bool
    d_tests_stored_as(
        const m::value&                   _x,
        m::internal::expr_matrix_storage _storage
    )
    {
        return (d_tests_storage(_x) == static_cast<int>(_storage));
    }

    // d_tests_holders
    //   a matrix's holders, as its box counts them; 0 where the value is no
    // matrix.
    std::size_t
    d_tests_holders(
        const m::value& _matrix
    )
    {
        return ( m::expr_is_matrix(_matrix)
                     ? m::internal::expr_matrix_box_of(
                           _matrix.c_value()).holders
                     : 0 );
    }

    // d_tests_kind_text
    //   a matrix as the kind writes it, in a buffer of _size; empty where
    // the value is no matrix.
    std::string
    d_tests_kind_text(
        const m::value& _x,
        std::size_t     _size
    )
    {
        const dj_math_value& held = _x.c_value();
        std::string          text(_size + 1, '#');

        // no matrix
        if (!m::expr_is_matrix(_x))
        {
            return std::string();
        }

        static_cast<void>(held.type->text(&text[0],
                                          _size,
                                          held));

        return text.substr(0, text.find('\0'));
    }

    // d_tests_two_by_two
    //   the exact matrix [1, 2; 3, 4].
    m::value
    d_tests_two_by_two()
    {
        return d_tests_made(m::expr_matrix(2,
                                           2,
                                           { 1, 2, 3, 4 }));
    }

    // d_tests_matrices
    //   expr_shape; the kind, its table and its entries; expr_matrix,
    // expr_vector, expr_is_matrix, expr_shape_of and expr_element, the
    // storage that keeps each element as given, and their errors; what the
    // kind's arithmetic does not take; expr_transpose; and step 21's (3c)
    // functions' entries, each as a matrix's, a number's and bad_shape.
    void
    d_tests_matrices()
    {
        const m::expr_shape by_default;
        const m::expr_shape two_by_three(2, 3);

        D_TESTS_EXPECT( (by_default.rows == 0) &&
                        (by_default.columns == 0) &&
                        (two_by_three.rows == 2) &&
                        (two_by_three.columns == 3),
                        "a shape: 0 by 0 by default, or its two numbers");
        D_TESTS_EXPECT( (two_by_three == m::expr_shape(2, 3)) &&
                        (two_by_three != m::expr_shape(3, 2)) &&
                        (two_by_three != m::expr_shape(2, 2)) &&
                        (!(two_by_three != m::expr_shape(2, 3))),
                        "shapes equal when both numbers are");

        const m::value                  one(1);
        const m::value                  a    = d_tests_two_by_two();
        const dj_math_boxed_kind* const kind =
            m::internal::expr_matrix_kind();
        const m::value                  row  = d_tests_made(
                                                   m::expr_vector(
                                                       { 1, 2, 3 }));
        const m::value                  col  = d_tests_made(
                                                   m::expr_matrix(
                                                       3,
                                                       1,
                                                       { 1, 2, 3 }));

        D_TESTS_EXPECT( (m::expr_is_matrix(a)) &&
                        (a.kind() == D_MATH_VALUE_BOXED) &&
                        (a.c_value().type == kind) &&
                        (std::strcmp(kind->name, "matrix") == 0) &&
                        (std::strcmp(a.kind_name(), "matrix") == 0) &&
                        (kind->rank == 1000) &&
                        (m::expr_matrix_rank == 1000) &&
                        (a.rank() == 1000) &&
                        (kind->traits == 0u) &&
                        (!a.is_number()) &&
                        (!a.is_exact()),
                        "a matrix: a boxed value of the kind matrix, of rank "
                        "1000, no number");
        D_TESTS_EXPECT( (d_tests_shape(a) == m::expr_shape(2, 2)) &&
                        (d_tests_shape(row) == m::expr_shape(1, 3)) &&
                        (d_tests_shape(col) == m::expr_shape(3, 1)),
                        "its shape: a vector one row");
        D_TESTS_EXPECT( (d_tests_holds_value(m::expr_element(a, 0, 0), 1)) &&
                        (d_tests_holds_value(m::expr_element(a, 0, 1), 2)) &&
                        (d_tests_holds_value(m::expr_element(a, 1, 0), 3)) &&
                        (d_tests_holds_value(m::expr_element(a, 1, 1), 4)) &&
                        (d_tests_holds_value(m::expr_element(col, 2, 0), 3)) &&
                        (d_tests_stored_as(
                             a,
                             m::internal::expr_matrix_storage::values)),
                        "its elements, row by row, exact: held as values");
        D_TESTS_EXPECT( (!m::expr_is_matrix(one)) &&
                        (!m::expr_is_matrix(m::value(2.5))) &&
                        (!m::expr_is_matrix(m::value())) &&
                        (!m::expr_is_matrix(m::value(true))) &&
                        (!m::expr_is_matrix(d_tests_wide_of("12345"))),
                        "no number, boolean or none is a matrix");

        const long double third     = 1.0L / 3.0L;
        const m::value    with_half = d_tests_made(
                                          m::expr_vector(
                                              { 1,
                                                m::value(0.5),
                                                m::value::ratio(1, 3) }));
        const m::value    floats    = d_tests_made(
                                          m::expr_vector(
                                              { m::value(0.5f),
                                                2 }));
        const m::value    pure      = d_tests_made(
                                          m::expr_vector(
                                              { m::value(0.5f),
                                                m::value(0.25f) }));
        const m::value    doubles   = d_tests_made(
                                          m::expr_vector(
                                              { m::value(0.25),
                                                m::value(0.5f) }));
        const m::value    widest    = d_tests_made(
                                          m::expr_vector(
                                              { m::value(0.5f),
                                                m::value(0.25),
                                                m::value(third) }));
        const m::value    real      = d_tests_made(
                                          m::expr_vector(
                                              { 1,
                                                d_tests_real_of(&third) }));
        const m::value    wide      = d_tests_made(
                                          m::expr_vector(
                                              { d_tests_wide_of(
                                                    "123456789012345678901"
                                                    "234567890"),
                                                m::value::ratio(-1, 2) }));

        D_TESTS_EXPECT( (d_tests_stored_as(
                             pure,
                             m::internal::expr_matrix_storage::floats)) &&
                        (d_tests_holds_value(m::expr_element(pure, 0, 1),
                                             m::value(0.25f))) &&
                        (d_tests_stored_as(
                             doubles,
                             m::internal::expr_matrix_storage::doubles)) &&
                        (d_tests_holds_value(m::expr_element(doubles, 0, 1),
                                             m::value(0.5))) &&
                        (d_tests_stored_as(
                             widest,
                             m::internal::expr_matrix_storage::
                                 long_doubles)) &&
                        (d_tests_holds_value(m::expr_element(widest, 0, 0),
                                             m::value(0.5L))) &&
                        (d_tests_holds_value(m::expr_element(widest, 0, 2),
                                             m::value(third))),
                        "every element a float, a double or a long double: "
                        "dense, in the widest one's type, each widened "
                        "exactly");
        D_TESTS_EXPECT( (d_tests_stored_as(
                             with_half,
                             m::internal::expr_matrix_storage::values)) &&
                        (d_tests_holds_value(m::expr_element(with_half, 0, 0),
                                             1)) &&
                        (d_tests_holds_value(m::expr_element(with_half, 0, 1),
                                             m::value(0.5))) &&
                        (d_tests_holds_value(m::expr_element(with_half, 0, 2),
                                             m::value::ratio(1, 3))) &&
                        (d_tests_stored_as(
                             floats,
                             m::internal::expr_matrix_storage::values)) &&
                        (d_tests_holds_value(m::expr_element(floats, 0, 0),
                                             m::value(0.5f))) &&
                        (d_tests_holds_value(m::expr_element(floats, 0, 1),
                                             2)) &&
                        (d_tests_stored_as(
                             real,
                             m::internal::expr_matrix_storage::values)) &&
                        (d_tests_holds_value(m::expr_element(real, 0, 1),
                                             d_tests_real_of(&third))) &&
                        (d_tests_stored_as(
                             wide,
                             m::internal::expr_matrix_storage::values)),
                        "an element exact or boxed: values, each element as "
                        "it is given, none rounded");

        const m::value big = d_tests_made(
                                 m::expr_vector(
                                     { m::value(18446744073709551615ull),
                                       1 }));

        D_TESTS_EXPECT( ((big.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0u) &&
                        ((a.flags() & D_MATH_VALUE_FLAG_INEXACT) == 0u),
                        "a matrix carries its elements' flags");

        const m::value pole(D_MATH_VALUE_ERROR_POLE);
        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( (d_tests_holds_value(m::expr_vector({ 1,
                                                              pole,
                                                              domain }),
                                             pole)) &&
                        (d_tests_holds_value(m::expr_vector({ true,
                                                              pole }),
                                             pole)) &&
                        (d_tests_holds_value(m::expr_vector({ 1,
                                                              true }),
                                             domain)) &&
                        (d_tests_holds_value(m::expr_vector({ m::value(),
                                                              1 }),
                                             domain)) &&
                        (d_tests_holds_value(m::expr_vector({ a,
                                                              1 }),
                                             domain)),
                        "an element's error passes, the first; an element "
                        "that is no number a domain error");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_matrix(0, 2, {}),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: a matrix has a row "
                                         "and a column at least, or "
                                         "neither, not 0 by 2") &&
                        d_tests_is_error(m::expr_matrix(1, 0, {}),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: a matrix has a row "
                                         "and a column at least, or "
                                         "neither, not 1 by 0") &&
                        d_tests_is_error(m::expr_matrix(0, 0, { 1 }),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: 1 elements, not "
                                         "those of a matrix of 0 by 0") &&
                        d_tests_is_error(m::expr_matrix(2, 2, { 1, 2, 3 }),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: 3 elements, not "
                                         "those of a matrix of 2 by 2") &&
                        d_tests_is_error(m::expr_matrix(2, 1, { 1, 2, 3 }),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: 3 elements, not "
                                         "those of a matrix of 2 by 1") &&
                        d_tests_is_error(m::expr_matrix(1, 2, { 1, 2, 3 }),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: 3 elements, not "
                                         "those of a matrix of 1 by 2"),
                        "a shape of no element but 0 by 0, or elements not "
                        "of the shape: bad_shape");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_shape_of(one),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_shape_of: the value is no "
                                         "matrix") &&
                        d_tests_is_error(m::expr_element(one, 0, 0),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_element: the value is no "
                                         "matrix") &&
                        d_tests_is_error(m::expr_element(a, 2, 0),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_element: no element at row "
                                         "2, column 0 of a matrix of 2 by "
                                         "2") &&
                        d_tests_is_error(m::expr_element(a, 0, 2),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_element: no element at row "
                                         "0, column 2 of a matrix of 2 by "
                                         "2"),
                        "the shape or an element of no matrix, or of no "
                        "such place: bad_shape");

        // the holders: a copy one more, its end one fewer, a move none
        bool held = (d_tests_holders(a) == 1);

        {
            m::value copy = a;
            m::value other(a.c_value());

            held = ( (held) &&
                     (d_tests_holders(a) == 3) );

            m::value moved(std::move(copy));

            held = ( (held) &&
                     (d_tests_holders(a) == 3) &&
                     (copy.kind() == D_MATH_VALUE_NONE) );
        }

        D_TESTS_EXPECT( (held) &&
                        (d_tests_holders(a) == 1),
                        "a box held as an int holds its number: a copy one "
                        "holder more, a value's end one fewer");

        const m::value same  = d_tests_made(
                                   m::expr_matrix(2,
                                                  2,
                                                  { 1.0, 2.0, 3.0, 4.0 }));
        const m::value other = d_tests_made(
                                   m::expr_matrix(2,
                                                  2,
                                                  { 1, 2, 3, 5 }));

        D_TESTS_EXPECT( (a == d_tests_two_by_two()) &&
                        (a == same) &&
                        (!(a == other)) &&
                        (!(a == row)) &&
                        (!(a == one)) &&
                        (!(one == a)) &&
                        (kind->cmp(a.c_value(), one.c_value()) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (kind->cmp(one.c_value(), a.c_value()) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (kind->cmp(a.c_value(), other.c_value()) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (kind->cmp(a.c_value(), same.c_value()) ==
                             D_MATH_VALUE_EQUAL),
                        "two matrices equal where of one shape and their "
                        "elements equal, whatever their storage; no order "
                        "otherwise");

        double        number    = 7.0;
        dj_math_value converted = d_math_value_none();
        int           direction = 5;

        D_TESTS_EXPECT( (d_tests_same_values(abs(a), domain)) &&
                        (d_tests_same_values(a + one, domain)) &&
                        (d_tests_same_values(one - a, domain)) &&
                        (d_tests_same_values(one / a, domain)) &&
                        (d_tests_same_values(a / a, domain)) &&
                        (d_tests_same_values(a - row, domain)) &&
                        (d_tests_same_values(row * row, domain)) &&
                        (!a.to(number)) &&
                        (number == 7.0) &&
                        (!kind->convert(&converted,
                                        &direction,
                                        a.c_value(),
                                        D_MATH_VALUE_DOUBLE)) &&
                        (converted.kind == D_MATH_VALUE_NONE) &&
                        (direction == 5),
                        "what the kind's arithmetic does not take a "
                        "domain error (S246): |A|, a matrix and a number "
                        "added or subtracted, a matrix under a number or "
                        "under a matrix, shapes that do not fit; no matrix "
                        "converts to a number");
        D_TESTS_EXPECT( (d_tests_kind_text(a, 64) == "[1, 2; 3, 4]") &&
                        (d_tests_kind_text(row, 64) == "[1, 2, 3]") &&
                        (d_tests_kind_text(col, 64) == "[1; 2; 3]") &&
                        (d_tests_kind_text(with_half, 64) ==
                             "[1, 0.5, 1/3]") &&
                        (d_tests_kind_text(floats, 64) == "[0.5, 2]") &&
                        (d_tests_kind_text(widest, 64).substr(0, 18) ==
                             "[0.5, 0.25, 0.3333") &&
                        (d_tests_kind_text(wide, 64) ==
                             "[123456789012345678901234567890, -1/2]"),
                        "its text: its rows, each element exact, by its "
                        "kind, or in max_digits10 digits, as it is held");
        D_TESTS_EXPECT( (d_tests_kind_text(a, 4) == "[1,") &&
                        (d_tests_kind_text(a, 1).empty()) &&
                        (m::expr_is_matrix(a)) &&
                        (kind->text(D_NULLPTR, 0, a.c_value()) == 12),
                        "its text cut to the buffer, its length whole");

        D_TESTS_EXPECT( d_tests_is_error(m::expr_transpose(one),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_transpose: the value is no "
                                         "matrix") &&
                        (d_tests_made(m::expr_transpose(a)) ==
                             d_tests_made(m::expr_matrix(2,
                                                         2,
                                                         { 1, 3, 2, 4 }))) &&
                        (d_tests_made(m::expr_transpose(row)) == col),
                        "expr_transpose: a matrix's, its columns its rows; "
                        "of no matrix bad_shape");
        const m::value wide3 = d_tests_made(m::expr_matrix(2,
                                                           3,
                                                           { 1, 2, 3,
                                                             4, 5, 6 }));
        const m::value flat  = d_tests_made(m::expr_matrix(2,
                                                           2,
                                                           { 1, 2, 2, 4 }));

        D_TESTS_EXPECT( d_tests_holds_value(m::expr_det(a),
                                            m::value(-2)) &&
                        d_tests_holds_value(m::expr_det(one),
                                            one) &&
                        d_tests_is_error(m::expr_det(wide3),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_det: a matrix of 2 by 3 is "
                                         "not square"),
                        "expr_det: an exact matrix's, exact; a number's, "
                        "itself; of no square matrix, bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(
                            m::expr_inv(a),
                            d_tests_made(m::expr_matrix(
                                2,
                                2,
                                { -2,
                                  1,
                                  m::value::ratio(3, 2),
                                  m::value::ratio(-1, 2) }))) &&
                        d_tests_holds_value(m::expr_inv(m::value(4)),
                                            m::value::ratio(1, 4)) &&
                        d_tests_holds_value(m::expr_inv(flat),
                                            m::value(
                                                D_MATH_VALUE_ERROR_POLE)) &&
                        d_tests_is_error(m::expr_inv(wide3),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_inv: a matrix of 2 by 3 is "
                                         "not square"),
                        "expr_inv: exact; a number's reciprocal; of a "
                        "singular matrix a pole; of no square, bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(m::expr_trace(a),
                                            m::value(5)) &&
                        d_tests_holds_value(m::expr_trace(one),
                                            one) &&
                        d_tests_is_error(m::expr_trace(row),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_trace: a matrix of 1 by 3 is "
                                         "not square"),
                        "expr_trace: the diagonal's sum; a number's itself; "
                        "of no square, bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(
                            m::expr_norm(d_tests_made(m::expr_vector(
                                { 3, 4 }))),
                            m::value(5)) &&
                        d_tests_holds_value(m::expr_norm(flat),
                                            m::value(5)) &&
                        d_tests_holds_value(m::expr_norm(m::value(-2)),
                                            m::value(2)) &&
                        d_tests_holds_value(m::expr_norm(wide3),
                                            m::internal::value_root_of(
                                                m::value(91))),
                        "expr_norm: Euclidean of a vector, Frobenius of a "
                        "matrix of any shape, exact where its square's "
                        "root is; a number's magnitude");
        D_TESTS_EXPECT( d_tests_holds_value(
                            m::expr_eig(d_tests_made(m::expr_matrix(
                                2,
                                2,
                                { 2, 1, 1, 2 }))),
                            d_tests_made(m::expr_vector({ 1, 3 }))) &&
                        d_tests_holds_value(m::expr_eig(one),
                                            one) &&
                        d_tests_is_error(m::expr_eig(col),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_eig: a matrix of 3 by 1 is "
                                         "not square"),
                        "expr_eig: the eigenvalues, ascending, a row; a "
                        "number's itself; of no square, bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(m::expr_dot(row,
                                                        col),
                                            m::value(14)) &&
                        d_tests_holds_value(m::expr_dot(m::value(2),
                                                        m::value(3)),
                                            m::value(6)) &&
                        d_tests_is_error(m::expr_dot(row,
                                                     one),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_dot: two vectors of one "
                                         "length, not 1 by 3 and 1 by 1") &&
                        d_tests_is_error(m::expr_dot(row,
                                                     a),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_dot: two vectors of one "
                                         "length, not 1 by 3 and 2 by 2"),
                        "expr_dot: two vectors of one length, a row and a "
                        "column among them; two numbers' product; else "
                        "bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(
                            m::expr_cross(d_tests_made(m::expr_vector(
                                              { 1, 0, 0 })),
                                          d_tests_made(m::expr_vector(
                                              { 0, 1, 0 }))),
                            d_tests_made(m::expr_vector({ 0, 0, 1 }))) &&
                        d_tests_holds_value(
                            m::expr_cross(col,
                                          row),
                            d_tests_made(m::expr_matrix(3,
                                                        1,
                                                        { 0, 0, 0 }))) &&
                        d_tests_is_error(m::expr_cross(one,
                                                       one),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_cross: two vectors of 3 "
                                         "elements, not 1 by 1 and 1 by 1"),
                        "expr_cross: the vector product, of the first's "
                        "shape; of no vectors of 3, bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(
                            m::expr_solve(a,
                                          d_tests_made(m::expr_vector(
                                              { 5, 6 }))),
                            d_tests_made(m::expr_vector(
                                { -4, m::value::ratio(9, 2) }))) &&
                        d_tests_holds_value(
                            m::expr_solve(m::value(2),
                                          row),
                            d_tests_made(m::expr_vector(
                                { m::value::ratio(1, 2),
                                  1,
                                  m::value::ratio(3, 2) }))) &&
                        d_tests_holds_value(
                            m::expr_solve(flat,
                                          d_tests_made(m::expr_vector(
                                              { 1, 1 }))),
                            m::value(D_MATH_VALUE_ERROR_POLE)) &&
                        d_tests_is_error(m::expr_solve(a,
                                                       row),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_solve: a square matrix and "
                                         "one of its rows, not 2 by 2 and "
                                         "1 by 3") &&
                        d_tests_is_error(m::expr_solve(
                                             d_tests_made(m::expr_matrix(
                                                 2,
                                                 3,
                                                 { 1, 2, 3, 4, 5, 6 })),
                                             d_tests_made(m::expr_matrix(
                                                 2,
                                                 1,
                                                 { 1, 2 }))),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_solve: a square matrix and "
                                         "one of its rows, not 2 by 3 and "
                                         "2 by 1"),
                        "expr_solve: X of A X = B, a row solved as a "
                        "column and given as a row; B / a of a number a; "
                        "a singular A a pole; else bad_shape, an A not "
                        "square among them");

        return;
    }

    // d_tests_vector_of, d_tests_matrix_of
    //   the term of a vector literal of its elements; of a matrix literal of
    // its rows, each of its elements.
    m::expr_term
    d_tests_vector_of(
        const std::vector<m::expr_term>& _elements
    )
    {
        return m::expr_application(m::expr_op_kind::vector,
                                   _elements);
    }

    m::expr_term
    d_tests_matrix_of(
        const std::vector<std::vector<m::expr_term> >& _rows
    )
    {
        std::vector<m::expr_term> rows;

        for (std::size_t r = 0; r < _rows.size(); ++r)
        {
            rows.push_back(m::expr_application(m::expr_op_kind::row,
                                               _rows[r]));
        }

        return m::expr_application(m::expr_op_kind::matrix,
                                   rows);
    }

    // d_tests_parse_literals
    //   step 19's literals in the parser layer: a vector of its elements, a
    // matrix of its rows, each element any term; one operand where it
    // stands; a [ after an operand indexing, and no implied product; and
    // every error at its place.
    void
    d_tests_parse_literals()
    {
        const m::expr_term one   = d_tests_literal("1");
        const m::expr_term two   = d_tests_literal("2");
        const m::expr_term three = d_tests_literal("3");
        const m::expr_term four  = d_tests_literal("4");
        const m::expr_term x     = m::expr_variable("x");
        m::expr_syntax     implied;
        m::expr_syntax     bars;

        implied.implied_multiplication = true;
        bars.absolute_bars             = true;

        D_TESTS_EXPECT( d_tests_parses("[1, 2, 3]",
                                       d_tests_vector_of({ one,
                                                           two,
                                                           three })) &&
                        d_tests_parses("[ 1 ]",
                                       d_tests_vector_of({ one })),
                        "[1, 2, 3]: the term vector of its elements");
        D_TESTS_EXPECT( d_tests_parses("[1, 2; 3, 4]",
                                       d_tests_matrix_of({ { one, two },
                                                           { three,
                                                             four } })) &&
                        d_tests_parses("[1; 2]",
                                       d_tests_matrix_of({ { one },
                                                           { two } })),
                        "[1, 2; 3, 4]: the term matrix of its rows, each "
                        "the term row of its elements");
        D_TESTS_EXPECT( d_tests_parses(
                            "[x, 2 * x; f(1, 2), (3); [1], -x]",
                            d_tests_matrix_of(
                                { { x,
                                    d_tests_apply(m::expr_op_kind::mul,
                                                  two,
                                                  x) },
                                  { m::expr_call("f",
                                                 { one,
                                                   two }),
                                    three },
                                  { d_tests_vector_of({ one }),
                                    d_tests_apply(m::expr_op_kind::neg,
                                                  x) } })),
                        "an element any term: a call's commas and what "
                        "brackets hold no element's end");
        D_TESTS_EXPECT( d_tests_parses("-[1] ^ 2 + [1]!",
                                       d_tests_apply(
                                           m::expr_op_kind::add,
                                           d_tests_apply(
                                               m::expr_op_kind::neg,
                                               d_tests_apply(
                                                   m::expr_op_kind::pow,
                                                   d_tests_vector_of(
                                                       { one }),
                                                   two)),
                                           d_tests_apply(
                                               m::expr_op_kind::factorial,
                                               d_tests_vector_of(
                                                   { one })))) &&
                        d_tests_parses("[1] x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     d_tests_vector_of(
                                                         { one }),
                                                     x),
                                       implied) &&
                        d_tests_parses("[|x|]",
                                       d_tests_vector_of(
                                           { m::expr_call("abs",
                                                          { x }) }),
                                       bars) &&
                        d_tests_parses("|[1]| - |x|",
                                       d_tests_apply(
                                           m::expr_op_kind::sub,
                                           m::expr_call(
                                               "abs",
                                               { d_tests_vector_of(
                                                     { one }) }),
                                           m::expr_call("abs",
                                                        { x })),
                                       bars),
                        "a literal one operand where it stands; a * "
                        "implied after it; bars in it");
        D_TESTS_EXPECT( d_tests_parses("x[1]",
                                       d_tests_apply(m::expr_op_kind::index,
                                                     x,
                                                     one)) &&
                        d_tests_parses("[1][2]",
                                       d_tests_apply(m::expr_op_kind::index,
                                                     d_tests_vector_of(
                                                         { one }),
                                                     two)) &&
                        d_tests_parses("2[1]",
                                       d_tests_apply(m::expr_op_kind::index,
                                                     two,
                                                     one),
                                       implied) &&
                        d_tests_is_error(m::expr_parse("x[1"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'[' is never closed"),
                        "a [ after an operand: indexing, and no implied "
                        "product");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("[,]"),
                                         m::expr_status::missing_operand,
                                         1,
                                         "an operand is missing before "
                                         "','") &&
                        d_tests_is_error(m::expr_parse("[1, ]"),
                                         m::expr_status::missing_operand,
                                         4,
                                         "an operand is missing before "
                                         "']'") &&
                        d_tests_is_error(m::expr_parse("[1;]"),
                                         m::expr_status::missing_operand,
                                         3,
                                         "an operand is missing before "
                                         "']'") &&
                        d_tests_is_error(m::expr_parse("[;1]"),
                                         m::expr_status::missing_operand,
                                         1,
                                         "an operand is missing before "
                                         "';'") &&
                        d_tests_is_error(m::expr_parse("[1,,2]"),
                                         m::expr_status::missing_operand,
                                         3,
                                         "an operand is missing before "
                                         "','"),
                        "an empty element: an operand missing before what "
                        "ends it");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("[1, 2; 3]"),
                                         m::expr_status::bad_shape,
                                         8,
                                         "a row of 1 element, where the "
                                         "first row has 2 elements") &&
                        d_tests_is_error(m::expr_parse("[1; 2, 3; 4]"),
                                         m::expr_status::bad_shape,
                                         8,
                                         "a row of 2 elements, where the "
                                         "first row has 1 element") &&
                        d_tests_is_error(m::expr_parse("[1, 2; 3, 4; 5]"),
                                         m::expr_status::bad_shape,
                                         14,
                                         "a row of 1 element, where the "
                                         "first row has 2 elements") &&
                        d_tests_is_error(m::expr_parse("[1; 2, ]"),
                                         m::expr_status::missing_operand,
                                         7,
                                         "an operand is missing before "
                                         "']'") &&
                        d_tests_is_error(m::expr_parse("[1; 2, 3] y"),
                                         m::expr_status::bad_shape,
                                         8,
                                         "a row of 2 elements, where the "
                                         "first row has 1 element"),
                        "a row of more or fewer elements than the first: "
                        "bad_shape, at what ends it, the further left error");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("[1, 2"),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'[' is never closed") &&
                        d_tests_is_error(m::expr_parse("1]"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "']' closes no bracket") &&
                        d_tests_is_error(m::expr_parse("[1)"),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'[' is never closed") &&
                        d_tests_is_error(m::expr_parse("(1]"),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'(' is never closed") &&
                        d_tests_is_error(m::expr_parse("|1]",
                                                       bars),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'|' is never closed") &&
                        d_tests_is_error(m::expr_parse("[(1]"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'(' is never closed"),
                        "a square bracket unmatched, or closing an open of "
                        "another kind: unbalanced");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("1; 2"),
                                         m::expr_status::unexpected_token,
                                         1,
                                         "unexpected ';'") &&
                        d_tests_is_error(m::expr_parse("f(1; 2)"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected ';'") &&
                        d_tests_is_error(m::expr_parse("[(1; 2)]"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected ';'") &&
                        d_tests_is_error(m::expr_parse("[1, 2 3]"),
                                         m::expr_status::unexpected_token,
                                         6,
                                         "unexpected '3'"),
                        "a semicolon outside a literal's brackets, and an "
                        "element's own error: unexpected, at its place");

        return;
    }

    // d_tests_render_literals
    //   literals written back, between square brackets, and read back as
    // themselves; a matrix a fold made written as a literal, and read back
    // to its value.
    void
    d_tests_render_literals()
    {
        D_TESTS_EXPECT( d_tests_renders("[1,2;3,4]", "[1, 2; 3, 4]") &&
                        d_tests_renders("[ x ,2*y ]", "[x, 2 * y]") &&
                        d_tests_renders("[1;2;3]", "[1; 2; 3]") &&
                        d_tests_renders("-[1]^2", "-[1] ^ 2") &&
                        d_tests_renders("([1, 2])!", "[1, 2]!") &&
                        d_tests_renders("[[1, 2], [3]; f([4]), 5]",
                                        "[[1, 2], [3]; f([4]), 5]") &&
                        d_tests_renders("[piecewise(when(x < 0, -x), "
                                        "otherwise(x)), a < b <= c]",
                                        "[piecewise(when(x < 0, -x), "
                                        "otherwise(x)), a < b and b <= c]"),
                        "a literal written as parse.hpp reads it, its "
                        "elements in turn, one operand with no brackets "
                        "of its own");

        const m::value exact = d_tests_made(
                                   m::expr_matrix(
                                       2,
                                       2,
                                       { 1,
                                         m::value::ratio(1, 2),
                                         -3,
                                         m::value::ratio(-1, 4) }));
        const m::value dense = d_tests_made(
                                   m::expr_vector({ m::value(0.1),
                                                    m::value(-2.5),
                                                    m::value(3.0) }));
        const std::string exact_text = m::expr_render(
                                           m::expr_number(exact));
        const std::string dense_text = m::expr_render(
                                           m::expr_number(dense));

        D_TESTS_EXPECT( (exact_text == "[1, (1/2); (-3), (-1/4)]") &&
                        (dense_text == "[0.1, (-2.5), 3]") &&
                        (d_tests_holds_value(
                            m::expr_evaluate(d_tests_parsed(
                                                 exact_text.c_str()),
                                             m::expr_bindings(),
                                             m::expr_context()),
                            exact)) &&
                        (d_tests_holds_value(
                            m::expr_evaluate(d_tests_parsed("[0.1, -2.5, "
                                                            "3]"),
                                             m::expr_bindings(),
                                             m::expr_context()),
                            d_tests_made(
                                m::expr_vector({ m::value::ratio(1, 10),
                                                 m::value::ratio(-5, 2),
                                                 3 })))),
                        "a matrix a fold made: a literal of its elements, "
                        "each as a number with no literal; exact, it reads "
                        "back to itself");
        D_TESTS_EXPECT( (m::expr_render(m::expr_application(
                             m::expr_op_kind::matrix,
                             { d_tests_literal("1"),
                               m::expr_application(
                                   m::expr_op_kind::row,
                                   d_tests_literal("2"),
                                   d_tests_literal("3")) })) ==
                             "[1; 2, 3]") &&
                        (m::expr_render(m::expr_application(
                             m::expr_op_kind::row,
                             d_tests_literal("1"),
                             d_tests_literal("2"))) == "[1, 2]"),
                        "a matrix of a term that is no row, and a row of no "
                        "matrix, as only a term made by hand has: written "
                        "as one element, and as a vector");

        return;
    }

    // d_tests_fold_literals
    //   literals folded: as a value, the matrix of their elements' values,
    // each element kept as it is, an element's error passing, the first;
    // in a type no value; and the language around them, the operators, the
    // rewrites and the handle, as step 19 leaves them.
    void
    d_tests_fold_literals()
    {
        m::expr_bindings at_two;

        static_cast<void>(at_two.at("x", 2));

        const m::expr_context context;
        const m::value        a = d_tests_two_by_two();

        D_TESTS_EXPECT( (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[1, 2; 3, "
                                                                "4]"),
                                                 m::expr_bindings(),
                                                 context),
                                             a)) &&
                        (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[x, 2 * x, "
                                                                "x ^ 2]"),
                                                 at_two,
                                                 context),
                                             d_tests_made(
                                                 m::expr_vector({ 2,
                                                                  4,
                                                                  4 })))) &&
                        (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[1; 2]"),
                                                 m::expr_bindings(),
                                                 context),
                                             d_tests_made(
                                                 m::expr_matrix(2,
                                                                1,
                                                                { 1, 2 })))),
                        "as a value: the matrix of its elements' values, a "
                        "vector one row");

        const m::expr_result<m::value> root = m::expr_evaluate(
                                                  d_tests_parsed("[sqrt(2), "
                                                                 "sqrt(3)]"),
                                                  m::expr_bindings(),
                                                  context);

        D_TESTS_EXPECT( (d_tests_stored_as(
                             d_tests_made(root),
                             m::internal::expr_matrix_storage::
                                 long_doubles)) &&
                        (d_tests_stored_as(
                             d_tests_made(m::expr_evaluate(
                                 d_tests_parsed("[sqrt(2), 0.5]"),
                                 m::expr_bindings(),
                                 context)),
                             m::internal::expr_matrix_storage::values)) &&
                        (d_tests_stored_as(
                             d_tests_made(m::expr_evaluate(
                                 d_tests_parsed("[0.5, 1]"),
                                 m::expr_bindings(),
                                 context)),
                             m::internal::expr_matrix_storage::values)),
                        "dense where each element is inexact, in the widest "
                        "one's type; values where any is exact");
        D_TESTS_EXPECT( (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[1, 1 / 0; "
                                                                "true, "
                                                                "2]"),
                                                 m::expr_bindings(),
                                                 context),
                                             m::value(
                                                 D_MATH_VALUE_ERROR_POLE))) &&
                        (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[1 < 2, "
                                                                "1]"),
                                                 m::expr_bindings(),
                                                 context),
                                             m::value(
                                                 D_MATH_VALUE_ERROR_DOMAIN))) &&
                        (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[[1; 2], "
                                                                "2]"),
                                                 m::expr_bindings(),
                                                 context),
                                             m::value(
                                                 D_MATH_VALUE_ERROR_DOMAIN))) &&
                        d_tests_is_error(m::expr_evaluate(
                                             d_tests_parsed("[1, y]"),
                                             at_two,
                                             context),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable y is bound to no "
                                         "value"),
                        "an element's error passes, the first; one that is "
                        "no number and no matrix, and blocks that do not "
                        "fit, a domain error; a fold's error before");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_parsed("[x, 1]"),
                                             at_two),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_evaluate<Type>: a vector or a "
                                         "matrix has no value in a type") &&
                        d_tests_is_error(m::expr_evaluate<float>(
                                             d_tests_parsed("[y, 1]"),
                                             at_two),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable y is bound to no "
                                         "value") &&
                        d_tests_is_error(m::expr_evaluate<float>(
                                             d_tests_parsed("[y; 1]"),
                                             at_two),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable y is bound to no "
                                         "value") &&
                        d_tests_is_error(m::expr_evaluate<float>(
                                             d_tests_parsed("[1; y]"),
                                             at_two),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_evaluate<Type>: a vector or a "
                                         "matrix has no value in a type"),
                        "in a type: no value, an operand's error first, as "
                        "at any node: a vector's element's, a matrix's first "
                        "row's, each row a literal");

        m::expr_bindings to_matrix;

        static_cast<void>(to_matrix.at("x", a));

        const m::expr_result<double> bound =
            m::expr_evaluate<double>(m::expr_variable("x"),
                                     to_matrix);

        D_TESTS_EXPECT( (bound.is_ok()) &&
                        (bound.value() != bound.value()) &&
                        (d_tests_holds_value(m::expr_evaluate(
                                                 m::expr_variable("x"),
                                                 to_matrix,
                                                 context),
                                             a)),
                        "a variable bound to a matrix: a NaN in a type, "
                        "the matrix as a value");

        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);
        bool           arithmetic = true;
        const char*    texts[]    = { "[1, 2] + 1", "1 / [1]", "[1] mod 2",
                                      "[3]!", "[1] < [2]", "[1] = 1",
                                      "not [1]", "abs([1])", "sin([1])",
                                      "atan2([1], 1)", "2 ^ [1]" };

        for (std::size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); ++i)
        {
            arithmetic = ( (arithmetic) &&
                           (d_tests_holds_value(m::expr_evaluate(
                                                    d_tests_parsed(texts[i]),
                                                    m::expr_bindings(),
                                                    context),
                                                domain)) );
        }

        D_TESTS_EXPECT( arithmetic,
                        "what the kind does not take, a matrix plus or "
                        "under a number, mod, the factorial, an order, "
                        "logic, the functions and a number to a matrix's "
                        "power: a domain error");
        const m::value empty = d_tests_made(m::expr_matrix(0,
                                                           0,
                                                           {}));

        D_TESTS_EXPECT( d_tests_holds_value(m::expr_evaluate(
                                                m::expr_application(
                                                    m::expr_op_kind::matrix,
                                                    std::vector<
                                                        m::expr_term>()),
                                                m::expr_bindings(),
                                                context),
                                            empty) &&
                        d_tests_holds_value(m::expr_evaluate(
                                                d_tests_vector_of({}),
                                                m::expr_bindings(),
                                                context),
                                            empty) &&
                        d_tests_holds_value(m::expr_evaluate(
                                                m::expr_application(
                                                    m::expr_op_kind::matrix,
                                                    d_tests_literal("1")),
                                                m::expr_bindings(),
                                                context),
                                            d_tests_made(
                                                m::expr_matrix(1,
                                                               1,
                                                               { 1 }))) &&
                        d_tests_holds_value(m::expr_evaluate(
                                                m::expr_application(
                                                    m::expr_op_kind::matrix,
                                                    d_tests_matrix_of(
                                                        { { d_tests_literal(
                                                                "1") },
                                                          { d_tests_literal(
                                                                "2") } })),
                                                m::expr_bindings(),
                                                context),
                                            d_tests_made(
                                                m::expr_matrix(2,
                                                               1,
                                                               { 1, 2 }))) &&
                        d_tests_holds_value(m::expr_evaluate(
                                                d_tests_matrix_of(
                                                    { { d_tests_literal("1"),
                                                        d_tests_literal(
                                                            "2") },
                                                      { d_tests_literal(
                                                          "3") } }),
                                                m::expr_bindings(),
                                                context),
                                            m::value(
                                                D_MATH_VALUE_ERROR_DOMAIN)) &&
                        d_tests_holds_value(m::expr_evaluate(
                                                d_tests_matrix_of(
                                                    { { d_tests_literal(
                                                            "1") },
                                                      { d_tests_apply(
                                                            m::expr_op_kind::
                                                                div,
                                                            d_tests_literal(
                                                                "1"),
                                                            d_tests_literal(
                                                                "0")) } }),
                                                m::expr_bindings(),
                                                context),
                                            m::value(
                                                D_MATH_VALUE_ERROR_POLE)),
                        "a term made by hand: a matrix of no row, or a "
                        "vector of no element, the empty matrix; a matrix "
                        "of a term that is no row, a block; rows unlike, a "
                        "domain error, as blocks that do not fit are; a "
                        "row's error passes");
        D_TESTS_EXPECT( (m::expr_render(m::expr_simplify(
                             d_tests_parsed("[x + x, 1 + 1; 2 * x * 3, "
                                            "x ^ 1]"))) ==
                             "[2 * x, 2; 6 * x, x]") &&
                        (m::expr_render(m::expr_simplify(
                             d_tests_parsed("[1, 2] + 0 * [3]"))) ==
                             "[1, 2] + 0 * [3]") &&
                        (m::expr_render(m::expr_substitute(
                             d_tests_parsed("[x, 2 * x]"),
                             "x",
                             d_tests_parsed("y + 1"))) ==
                             "[y + 1, 2 * (y + 1)]"),
                        "simplification inside a literal, never of it, and "
                        "no part dropped beside one; substitution in it");
        D_TESTS_EXPECT( (m::parse("[1,2;3,4]").str() == "[1, 2; 3, 4]") &&
                        (d_tests_holds_value(m::parse("[1, 2; 3, 4]").eval(),
                                             a)) &&
                        (d_tests_is_error(m::parse("[1]").eval<double>(),
                                          m::expr_status::bad_shape,
                                          m::expr_no_offset,
                                          "expr_evaluate<Type>: a vector or "
                                          "a matrix has no value in a type")),
                        "the handle: a literal's text and its value; no "
                        "value in a type");

        return;
    }

    // d_tests_literals_random
    //   1,000 random literals of one to three rows of one to three elements,
    // each element a random term of 2d's language (d_tests_language), drawn
    // by a generator of their own: each rendered and read back as itself,
    // implied multiplication off and on; folded as a value, the matrix of
    // its elements' values or their first error; in a type, its first row's
    // first error or bad_shape; simplified, the literal of its elements
    // simplified; and x replaced in it, the literal of its elements with x
    // replaced.
    void
    d_tests_literals_random()
    {
        const d_tests_alphabet alphabet = d_tests_language();
        const m::expr_term     put      = d_tests_apply(
                                              m::expr_op_kind::add,
                                              m::expr_variable("a"),
                                              d_tests_literal("1"));
        d_tests_rng            rng(20261025u);
        m::expr_syntax         implied;
        m::expr_bindings       bindings;
        const m::expr_context  context;
        std::size_t            back         = 0;
        std::size_t            back_implied = 0;
        std::size_t            valued       = 0;
        std::size_t            typed        = 0;
        std::size_t            simplified   = 0;
        std::size_t            substituted  = 0;
        const std::size_t      count        = 1000;

        implied.implied_multiplication = true;
        static_cast<void>(bindings.at("a", 1)
                                  .at("b", 2)
                                  .at("x", 3)
                                  .at("y_1", m::value::ratio(1, 2))
                                  .at("mod2", 5)
                                  .at("when", 0));

        for (std::size_t n = 0; n < count; ++n)
        {
            const unsigned                        rows    = 1 +
                                                            rng.below(3u);
            const unsigned                        columns = 1 +
                                                            rng.below(3u);
            std::vector<std::vector<m::expr_term> > drawn(rows);
            std::vector<std::vector<m::expr_term> > simple(rows);
            std::vector<std::vector<m::expr_term> > replaced(rows);
            std::vector<m::value>                 values;
            bool                                  failed  = false;
            m::expr_error                         first;
            bool                                  failed_typed = false;
            m::expr_error                         first_typed;
            bool                                  decided = false;
            m::expr_error                         expected;

            // its elements, row by row, each folded both ways in turn; the
            // literal's answer the first row's that fails, by its first
            // error, a complex element among the others' values since step
            // 23's third part (S348)
            for (unsigned r = 0; r < rows; ++r)
            {
                bool          row_failed  = false;
                m::expr_error row_first;

                for (unsigned c = 0; c < columns; ++c)
                {
                    const m::expr_term element =
                        d_tests_random_term(rng,
                                            alphabet,
                                            2u);
                    const m::expr_result<m::value> folded =
                        m::expr_evaluate(element,
                                         bindings,
                                         context);
                    const m::expr_result<double>   in_double =
                        m::expr_evaluate<double>(element,
                                                 bindings);

                    drawn[r].push_back(element);
                    simple[r].push_back(m::expr_simplify(element));
                    replaced[r].push_back(m::expr_substitute(element,
                                                             "x",
                                                             put));

                    if ( (folded.is_err()) &&
                         (!failed) )
                    {
                        failed = true;
                        first  = folded.error();
                    }

                    if ( (folded.is_err()) &&
                         (!row_failed) )
                    {
                        row_failed = true;
                        row_first  = folded.error();
                    }

                    if ( (in_double.is_err()) &&
                         (!failed_typed) &&
                         (r == 0) )
                    {
                        failed_typed = true;
                        first_typed  = in_double.error();
                    }

                    if (folded.is_ok())
                    {
                        values.push_back(folded.value());
                    }
                }

                // the first row that fails
                if ( (!decided) &&
                     (row_failed) )
                {
                    decided  = true;
                    expected = row_first;
                }
            }

            const bool         one_row = (rows == 1);
            const m::expr_term literal =
                ( one_row ? d_tests_vector_of(drawn[0])
                          : d_tests_matrix_of(drawn) );
            const std::string  text    = m::expr_render(literal);

            back += ( d_tests_parses(text.c_str(),
                                     literal) ? 1 : 0 );
            back_implied += ( d_tests_parses(text.c_str(),
                                             literal,
                                             implied) ? 1 : 0 );

            const m::expr_result<m::value> value =
                m::expr_evaluate(literal,
                                 bindings,
                                 context);

            if (decided)
            {
                valued += ( ( (value.is_err()) &&
                              (value.error() == expected) ) ? 1 : 0 );
            }
            else
            {
                const m::expr_result<m::value> want =
                    m::expr_matrix(rows,
                                   columns,
                                   values);

                valued += ( d_tests_holds_value(value,
                                                d_tests_made(want)) ? 1 : 0 );
            }

            const m::expr_result<double> in_type =
                m::expr_evaluate<double>(literal,
                                         bindings);

            typed += ( ( (in_type.is_err()) &&
                         ( (failed_typed)
                               ? (in_type.error() == first_typed)
                               : (in_type.error().status ==
                                      m::expr_status::bad_shape) ) )
                           ? 1 : 0 );
            simplified += ( d_tests_same(m::expr_simplify(literal),
                                         ( one_row
                                               ? d_tests_vector_of(simple[0])
                                               : d_tests_matrix_of(
                                                     simple) )) ? 1 : 0 );
            substituted += ( d_tests_same(m::expr_substitute(literal,
                                                             "x",
                                                             put),
                                          ( one_row
                                                ? d_tests_vector_of(
                                                      replaced[0])
                                                : d_tests_matrix_of(
                                                      replaced) )) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (back == count),
                        "1,000 random literals rendered and read back as "
                        "themselves");
        D_TESTS_EXPECT( (back_implied == count),
                        "and so with implied multiplication on");
        D_TESTS_EXPECT( (valued == count),
                        "as a value, the matrix of their elements' values, "
                        "complex ones among them (S348), or the first "
                        "error");
        D_TESTS_EXPECT( (typed == count),
                        "in a type, their first row's first error, or "
                        "bad_shape");
        D_TESTS_EXPECT( (simplified == count),
                        "simplified, the literal of their elements "
                        "simplified");
        D_TESTS_EXPECT( (substituted == count),
                        "x replaced, the literal of their elements with x "
                        "replaced");

        return;
    }

    // d_tests_is_matrix
    //   whether a value is the matrix of _rows by _columns of the elements
    // given, row by row, each equal to the value's, held in the storage
    // given.
    bool
    d_tests_is_matrix(
        const m::value&                  _x,
        std::size_t                      _rows,
        std::size_t                      _columns,
        const std::vector<m::value>&     _elements,
        m::internal::expr_matrix_storage _storage
    )
    {
        const m::value want = d_tests_made(m::expr_matrix(_rows,
                                                          _columns,
                                                          _elements));

        return ( (d_tests_same_values(_x,
                                      want)) &&
                 (d_tests_stored_as(_x,
                                    _storage)) );
    }

    // d_tests_all_exact
    //   whether every element of a matrix is exact; false where the value
    // is no matrix.
    bool
    d_tests_all_exact(
        const m::value& _x
    )
    {
        const m::expr_shape shape = d_tests_shape(_x);
        bool                exact = (shape.rows > 0);

        for (std::size_t r = 0; r < shape.rows; ++r)
        {
            for (std::size_t c = 0; c < shape.columns; ++c)
            {
                exact = ( (exact) &&
                          (d_tests_made(m::expr_element(_x,
                                                        r,
                                                        c)).is_exact()) );
            }
        }

        return exact;
    }

    // d_tests_dense_in
    //   whether the kind's arithmetic keeps dense matrices of Type in Type,
    // each answer as Type computes it: A + B, A - B, -A, A * 2, A / 2,
    // A * B, A', A ^ 0 and A ^ 2, of A = [1, 2; 3, 4] and
    // B = [0.5, 1; 1.5, 2], each in Type, held in _storage.
    template<typename Type>
    bool
    d_tests_dense_in(
        m::internal::expr_matrix_storage _storage
    )
    {
        const m::value a = d_tests_made(m::expr_matrix(2,
                                                       2,
                                                       { Type(1), Type(2),
                                                         Type(3),
                                                         Type(4) }));
        const m::value b = d_tests_made(m::expr_matrix(2,
                                                       2,
                                                       { Type(0.5), Type(1),
                                                         Type(1.5),
                                                         Type(2) }));

        return ( d_tests_is_matrix(a + b,
                                   2, 2,
                                   { Type(1.5), Type(3), Type(4.5),
                                     Type(6) },
                                   _storage) &&
                 d_tests_is_matrix(a - b,
                                   2, 2,
                                   { Type(0.5), Type(1), Type(1.5),
                                     Type(2) },
                                   _storage) &&
                 d_tests_is_matrix(-a,
                                   2, 2,
                                   { Type(-1), Type(-2), Type(-3),
                                     Type(-4) },
                                   _storage) &&
                 d_tests_is_matrix(a * m::value(2),
                                   2, 2,
                                   { Type(2), Type(4), Type(6), Type(8) },
                                   _storage) &&
                 d_tests_is_matrix(a / m::value(2),
                                   2, 2,
                                   { Type(0.5), Type(1), Type(1.5),
                                     Type(2) },
                                   _storage) &&
                 d_tests_is_matrix(a * b,
                                   2, 2,
                                   { Type(3.5), Type(5), Type(7.5),
                                     Type(11) },
                                   _storage) &&
                 d_tests_is_matrix(m::internal::expr_matrix_transposed(a),
                                   2, 2,
                                   { Type(1), Type(3), Type(2), Type(4) },
                                   _storage) &&
                 d_tests_is_matrix(m::internal::expr_matrix_power(
                                       a,
                                       m::value(0)),
                                   2, 2,
                                   { Type(1), Type(0), Type(0), Type(1) },
                                   _storage) &&
                 d_tests_is_matrix(m::internal::expr_matrix_power(
                                       a,
                                       m::value(2)),
                                   2, 2,
                                   { Type(7), Type(10), Type(15),
                                     Type(22) },
                                   _storage) );
    }

    // d_tests_matrix_arithmetic
    //   step 20's arithmetic of the kind, through the value's operators
    // (S246, S247): sums and differences of one shape, the negation, a
    // matrix and a number multiplied or a matrix divided by a number, and
    // the matrix product; each result's storage, exact elements kept exact
    // and dense ones in the wider type; errors passing; and what the kind
    // does not take.
    void
    d_tests_matrix_arithmetic()
    {
        typedef m::internal::expr_matrix_storage storage;

        const m::value a      = d_tests_two_by_two();
        const m::value b      = d_tests_made(m::expr_matrix(2,
                                                            2,
                                                            { 5, 6, 7, 8 }));
        const m::value floats = d_tests_made(
                                    m::expr_matrix(2,
                                                   2,
                                                   { 0.5f, 1.0f, 1.5f,
                                                     2.0f }));
        const m::value doubles = d_tests_made(
                                     m::expr_matrix(2,
                                                    2,
                                                    { 0.5, 1.0, 1.5, 2.0 }));
        const m::value longs   = d_tests_made(
                                     m::expr_matrix(2,
                                                    2,
                                                    { 0.5L, 1.0L, 1.5L,
                                                      2.0L }));
        const m::value row     = d_tests_made(m::expr_vector({ 1, 2, 3 }));
        const m::value col     = d_tests_made(m::expr_matrix(3,
                                                             1,
                                                             { 1, 2, 3 }));
        const m::value third   = d_tests_made(
                                     m::expr_vector({ m::value::ratio(1, 3),
                                                      m::value(2) }));
        const m::value wide    = d_tests_made(
                                     m::expr_matrix(2,
                                                    3,
                                                    { 1, 2, 3, 4, 5, 6 }));
        const m::value dense   = d_tests_made(
                                     m::expr_matrix(2,
                                                    3,
                                                    { 1.0, 2.0, 3.0, 4.0,
                                                      5.0, 6.0 }));
        const m::value pole(D_MATH_VALUE_ERROR_POLE);
        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( d_tests_is_matrix(a + b,
                                          2, 2,
                                          { 6, 8, 10, 12 },
                                          storage::values) &&
                        d_tests_is_matrix(a - b,
                                          2, 2,
                                          { -4, -4, -4, -4 },
                                          storage::values) &&
                        d_tests_is_matrix(b - a,
                                          2, 2,
                                          { 4, 4, 4, 4 },
                                          storage::values) &&
                        d_tests_is_matrix(-a,
                                          2, 2,
                                          { -1, -2, -3, -4 },
                                          storage::values) &&
                        d_tests_all_exact(a + b) &&
                        d_tests_all_exact(-a),
                        "A + B and A - B element by element, -A each "
                        "negated: exact matrices exact");
        D_TESTS_EXPECT( d_tests_is_matrix(a + floats,
                                          2, 2,
                                          { 1.5f, 3.0f, 4.5f, 6.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(floats + a,
                                          2, 2,
                                          { 1.5f, 3.0f, 4.5f, 6.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(floats + doubles,
                                          2, 2,
                                          { 1.0, 2.0, 3.0, 4.0 },
                                          storage::doubles) &&
                        d_tests_is_matrix(doubles - longs,
                                          2, 2,
                                          { 0.0L, 0.0L, 0.0L, 0.0L },
                                          storage::long_doubles) &&
                        d_tests_is_matrix(-floats,
                                          2, 2,
                                          { -0.5f, -1.0f, -1.5f, -2.0f },
                                          storage::floats),
                        "a dense sum in the wider of its operands' types, "
                        "an exact matrix taking the dense one's, as the "
                        "tower takes a number");
        D_TESTS_EXPECT( d_tests_is_matrix(a * m::value(2),
                                          2, 2,
                                          { 2, 4, 6, 8 },
                                          storage::values) &&
                        d_tests_is_matrix(m::value(2) * a,
                                          2, 2,
                                          { 2, 4, 6, 8 },
                                          storage::values) &&
                        d_tests_is_matrix(a / m::value(2),
                                          2, 2,
                                          { m::value::ratio(1, 2), 1,
                                            m::value::ratio(3, 2), 2 },
                                          storage::values) &&
                        d_tests_all_exact(a / m::value(2)) &&
                        d_tests_is_matrix(third * m::value(3),
                                          1, 2,
                                          { 1, 6 },
                                          storage::values) &&
                        d_tests_is_matrix(a * m::value(0.5f),
                                          2, 2,
                                          { 0.5f, 1.0f, 1.5f, 2.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(floats * m::value(0.5),
                                          2, 2,
                                          { 0.25, 0.5, 0.75, 1.0 },
                                          storage::doubles) &&
                        d_tests_is_matrix(floats * m::value(2.0f),
                                          2, 2,
                                          { 1.0f, 2.0f, 3.0f, 4.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(doubles / m::value(0.5f),
                                          2, 2,
                                          { 1.0, 2.0, 3.0, 4.0 },
                                          storage::doubles) &&
                        d_tests_is_matrix(longs * m::value(0.5f),
                                          2, 2,
                                          { 0.25L, 0.5L, 0.75L, 1.0L },
                                          storage::long_doubles) &&
                        d_tests_is_matrix(doubles * m::value(2),
                                          2, 2,
                                          { 1.0, 2.0, 3.0, 4.0 },
                                          storage::doubles),
                        "A * s, s * A and A / s each element by the number, "
                        "exactly for exact operands, otherwise in the "
                        "wider type, an exact number taking the matrix's");
        const long double quarter = 0.25L;

        D_TESTS_EXPECT( d_tests_dense_in<float>(storage::floats) &&
                        d_tests_dense_in<double>(storage::doubles) &&
                        d_tests_dense_in<long double>(
                            storage::long_doubles) &&
                        d_tests_is_matrix(floats + longs,
                                          2, 2,
                                          { 1.0L, 2.0L, 3.0L, 4.0L },
                                          storage::long_doubles) &&
                        d_tests_same_values(floats *
                                                d_tests_real_of(&quarter),
                                            domain),
                        "each dense type's arithmetic in that type: sums, "
                        "differences, negation, scalar products and "
                        "quotients, products, transposes and powers; a "
                        "float matrix widened to a long double one's type; "
                        "a boxed inexact number on the values' path, by "
                        "its own arithmetic, which the test's kind gives "
                        "as none");
        D_TESTS_EXPECT( d_tests_same_values(a / m::value(0),
                                            pole) &&
                        d_tests_is_matrix(doubles / m::value(0),
                                          2, 2,
                                          { std::numeric_limits<
                                                double>::infinity(),
                                            std::numeric_limits<
                                                double>::infinity(),
                                            std::numeric_limits<
                                                double>::infinity(),
                                            std::numeric_limits<
                                                double>::infinity() },
                                          storage::doubles),
                        "an exact matrix divided by 0 a pole, as each "
                        "element's quotient is; a dense one's elements "
                        "infinite, as in its type");
        D_TESTS_EXPECT( d_tests_is_matrix(a * b,
                                          2, 2,
                                          { 19, 22, 43, 50 },
                                          storage::values) &&
                        d_tests_is_matrix(b * a,
                                          2, 2,
                                          { 23, 34, 31, 46 },
                                          storage::values) &&
                        d_tests_is_matrix(row * col,
                                          1, 1,
                                          { 14 },
                                          storage::values) &&
                        d_tests_is_matrix(col * row,
                                          3, 3,
                                          { 1, 2, 3, 2, 4, 6, 3, 6, 9 },
                                          storage::values) &&
                        d_tests_is_matrix(third * d_tests_made(
                                                      m::expr_matrix(
                                                          2,
                                                          1,
                                                          { 3, 3 })),
                                          1, 1,
                                          { 7 },
                                          storage::values) &&
                        d_tests_is_matrix(a * floats,
                                          2, 2,
                                          { 3.5f, 5.0f, 7.5f, 11.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(floats * doubles,
                                          2, 2,
                                          { 1.75, 2.5, 3.75, 5.5 },
                                          storage::doubles) &&
                        d_tests_is_matrix(dense * d_tests_made(
                                                      m::expr_matrix(
                                                          3,
                                                          1,
                                                          { 1.0, 0.5,
                                                            0.25 })),
                                          2, 1,
                                          { 2.75, 8.0 },
                                          storage::doubles) &&
                        d_tests_is_matrix(wide * col,
                                          2, 1,
                                          { 14, 32 },
                                          storage::values),
                        "A * B the matrix product, A's columns B's rows: "
                        "AB and BA two matrices; a row by a column 1 by 1, "
                        "still a matrix; a column by a row their outer "
                        "product; a 2 by 3 by a 3 by 1 a 2 by 1, dense or "
                        "of values");

        bool refused = true;

        // what the kind does not take
        const m::value refusals[] = { abs(a), a + m::value(1),
                                      m::value(1) + a, a - m::value(1),
                                      m::value(1) - a, m::value(1) / a,
                                      a / b, a + row, a - col, row * row,
                                      a * col, col * a, a * m::value(true),
                                      m::value(true) * a, a * m::value(),
                                      a + m::value(true) };

        for (std::size_t i = 0;
             i < (sizeof(refusals) / sizeof(refusals[0]));
             ++i)
        {
            refused = ( (refused) &&
                        (d_tests_same_values(refusals[i],
                                             domain)) );
        }

        D_TESTS_EXPECT( refused,
                        "|A|, A + s, s + A, A - s, s - A, s / A and A / B, "
                        "shapes that do not fit, a boolean or none: each a "
                        "domain error (S246)");
        D_TESTS_EXPECT( d_tests_same_values(pole * a,
                                            pole) &&
                        d_tests_same_values(a * pole,
                                            pole) &&
                        d_tests_same_values(a + pole,
                                            pole) &&
                        d_tests_same_values(-m::value(pole),
                                            pole),
                        "an error passes, before the kind is asked");

        return;
    }

    // d_tests_matrix_powers
    //   the transpose; A ^ n by squaring, of a square matrix and a whole
    // n >= 0, exact or floating-point (S248); indexing, from 1 (S249); and
    // two matrices equal or not as a relation's value.
    void
    d_tests_matrix_powers()
    {
        typedef m::internal::expr_matrix_storage storage;

        const m::value a       = d_tests_two_by_two();
        const m::value fib     = d_tests_made(m::expr_matrix(2,
                                                             2,
                                                             { 1, 1, 1, 0 }));
        const m::value floats  = d_tests_made(
                                     m::expr_matrix(2,
                                                    2,
                                                    { 0.5f, 1.0f, 1.5f,
                                                      2.0f }));
        const m::value doubles = d_tests_made(
                                     m::expr_matrix(2,
                                                    2,
                                                    { 0.5, 1.0, 1.5, 2.0 }));
        const m::value row     = d_tests_made(m::expr_vector({ 1, 2, 3 }));
        const m::value col     = d_tests_made(m::expr_matrix(3,
                                                             1,
                                                             { 1, 2, 3 }));
        const m::value wide    = d_tests_made(
                                     m::expr_matrix(2,
                                                    3,
                                                    { 1, 2, 3, 4, 5, 6 }));
        const m::value pole(D_MATH_VALUE_ERROR_POLE);
        const m::value range(D_MATH_VALUE_ERROR_RANGE);
        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( d_tests_is_matrix(m::internal::expr_matrix_transposed(
                                              wide),
                                          3, 2,
                                          { 1, 4, 2, 5, 3, 6 },
                                          storage::values) &&
                        d_tests_is_matrix(m::internal::expr_matrix_transposed(
                                              floats),
                                          2, 2,
                                          { 0.5f, 1.5f, 1.0f, 2.0f },
                                          storage::floats) &&
                        (m::internal::expr_matrix_transposed(row) == col) &&
                        (m::internal::expr_matrix_transposed(
                             m::internal::expr_matrix_transposed(wide)) ==
                             wide) &&
                        d_tests_same_values(
                            m::internal::expr_transpose_value(m::value(2)),
                            m::value(2)) &&
                        d_tests_same_values(
                            m::internal::expr_transpose_value(pole),
                            pole) &&
                        d_tests_same_values(
                            m::internal::expr_transpose_value(
                                m::value(true)),
                            domain),
                        "the transpose: columns as rows, in the matrix's "
                        "storage, twice itself; a number's the number, an "
                        "error's the error, a boolean's a domain error");

        const m::value power5 = m::internal::expr_matrix_power(a,
                                                               m::value(5));
        const m::value fib90  = m::internal::expr_matrix_power(fib,
                                                               m::value(90));

        D_TESTS_EXPECT( d_tests_is_matrix(m::internal::expr_matrix_power(
                                              a,
                                              m::value(0)),
                                          2, 2,
                                          { 1, 0, 0, 1 },
                                          storage::values) &&
                        (m::internal::expr_matrix_power(a,
                                                        m::value(1)) == a) &&
                        d_tests_is_matrix(m::internal::expr_matrix_power(
                                              a,
                                              m::value(2)),
                                          2, 2,
                                          { 7, 10, 15, 22 },
                                          storage::values) &&
                        d_tests_is_matrix(power5,
                                          2, 2,
                                          { 1069, 1558, 2337, 3406 },
                                          storage::values) &&
                        (power5 == ((a * a) * (a * a)) * a) &&
                        d_tests_all_exact(fib90) &&
                        d_tests_same_values(
                            d_tests_made(m::expr_element(fib90, 0, 1)),
                            m::value(static_cast<dj_math_imax>(
                                2880067194370816120LL))) &&
                        d_tests_same_values(
                            d_tests_made(m::expr_element(fib90, 0, 0)),
                            m::value(static_cast<dj_math_imax>(
                                4660046610375530309LL))),
                        "A ^ n by squaring: A ^ 0 the identity, A ^ 1 A, "
                        "A ^ 5 as A's products give it, exact: the 90th "
                        "power of [1, 1; 1, 0] F(91) and F(90)");
        D_TESTS_EXPECT( d_tests_is_matrix(m::internal::expr_matrix_power(
                                              floats,
                                              m::value(0)),
                                          2, 2,
                                          { 1.0f, 0.0f, 0.0f, 1.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(m::internal::expr_matrix_power(
                                              doubles,
                                              m::value(3)),
                                          2, 2,
                                          { 4.625, 6.75, 10.125, 14.75 },
                                          storage::doubles) &&
                        d_tests_is_matrix(m::internal::expr_matrix_power(
                                              a,
                                              m::value(2.0)),
                                          2, 2,
                                          { 7.0, 10.0, 15.0, 22.0 },
                                          storage::doubles) &&
                        d_tests_is_matrix(m::internal::expr_matrix_power(
                                              a,
                                              m::value(0.0f)),
                                          2, 2,
                                          { 1.0f, 0.0f, 0.0f, 1.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(m::internal::expr_matrix_power(
                                              floats,
                                              m::value(1.0L)),
                                          2, 2,
                                          { 0.5L, 1.0L, 1.5L, 2.0L },
                                          storage::long_doubles),
                        "a dense matrix's power in its type, its identity "
                        "too; to a floating-point whole n, the matrix "
                        "first in the wider of its type and n's");

        bool refused = true;

        // what a power does not take
        const m::value refusals[] = {
            m::internal::expr_matrix_power(wide, m::value(2)),
            m::internal::expr_matrix_power(row, m::value(0)),
            m::internal::expr_matrix_power(row, m::value(-1)),
            m::internal::expr_matrix_power(a, m::value::ratio(1, 2)),
            m::internal::expr_matrix_power(a, m::value(2.5)),
            m::internal::expr_matrix_power(a, m::value(-1.5)),
            m::internal::expr_matrix_power(
                a,
                m::value(std::numeric_limits<double>::infinity())),
            m::internal::expr_matrix_power(
                a,
                m::value(std::numeric_limits<double>::quiet_NaN())),
            m::internal::expr_matrix_power(a, m::value(1e19)),
            m::internal::expr_matrix_power(a, m::value(true)),
            m::internal::expr_matrix_power(a, m::value()),
            m::internal::expr_matrix_power(a, a),
            m::internal::expr_matrix_power(m::value(2), a),
            m::internal::expr_matrix_power(wide, m::value(2.0)) };

        for (std::size_t i = 0;
             i < (sizeof(refusals) / sizeof(refusals[0]));
             ++i)
        {
            refused = ( (refused) &&
                        (d_tests_same_values(refusals[i],
                                             domain)) );
        }

        D_TESTS_EXPECT( refused,
                        "a power of a matrix not square, or to a "
                        "fractional, infinite, NaN or too great n, a "
                        "boolean, none or a matrix, and a number to a "
                        "matrix: a domain error");

        const long double two_real = 2.0L;
        const m::value    boxed    = d_tests_made(
                                         m::expr_matrix(1,
                                                        1,
                                                        { d_tests_real_of(
                                                              &two_real) }));

        D_TESTS_EXPECT( d_tests_same_values(
                            m::internal::expr_matrix_power(
                                a,
                                d_tests_real_of(&two_real)),
                            domain) &&
                        (d_tests_shape(m::internal::expr_matrix_power(
                                           boxed,
                                           m::value(1))) ==
                             m::expr_shape(1, 1)) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_power(boxed,
                                                           m::value(2)),
                            domain),
                        "a power by the arithmetic it asks of its operands: "
                        "to a boxed inexact whole number, or of a matrix "
                        "of one, the test's kind's arithmetic giving none, "
                        "a domain error; where no product is asked, A, a 1 "
                        "by 1 matrix");
        const m::value fib100 = m::internal::expr_matrix_power(
                                    fib,
                                    m::value(100));
        const m::value half   = d_tests_made(
                                    m::expr_matrix(2,
                                                   2,
                                                   { m::value::ratio(1, 2),
                                                     0, 0, 0 }));
        const m::value flagged = fib100 + half;

        D_TESTS_EXPECT( d_tests_stored_as(fib100,
                                          storage::long_doubles) &&
                        (fib100.flags() == D_MATH_VALUE_FLAG_INEXACT) &&
                        (d_tests_made(m::expr_element(fib100, 0, 0)).flags() ==
                             D_MATH_VALUE_FLAG_INEXACT) &&
                        (m::internal::expr_matrix_index(
                             { fib100, m::value(2), m::value(2) }).flags() ==
                             D_MATH_VALUE_FLAG_INEXACT) &&
                        d_tests_stored_as(flagged,
                                          storage::long_doubles) &&
                        (flagged.flags() == D_MATH_VALUE_FLAG_INEXACT) &&
                        (d_tests_made(m::expr_element(flagged,
                                                      1,
                                                      1)).flags() ==
                             D_MATH_VALUE_FLAG_INEXACT),
                        "a power past the exact integers' range long "
                        "doubles, flagged inexact (G2); each of its "
                        "elements, read or indexed, flagged so, and each "
                        "computed from one beside an exact matrix's, as "
                        "values, then dense again");
        D_TESTS_EXPECT( d_tests_same_values(
                            m::internal::expr_value_pow(a,
                                                        pole),
                            pole) &&
                        d_tests_same_values(
                            m::internal::expr_value_pow(pole,
                                                        a),
                            pole) &&
                        d_tests_same_values(
                            m::internal::expr_value_pow(a,
                                                        m::value(3)),
                            ((a * a) * a)),
                        "^ of a matrix the kind's power, an error passing "
                        "first");

        const m::value two(2);
        const m::value one(1);

        D_TESTS_EXPECT( d_tests_same_values(
                            m::internal::expr_matrix_index({ a, two, one }),
                            m::value(3)) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ row,
                                                             m::value(3) }),
                            m::value(3)) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ col, two }),
                            two) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ col,
                                                             m::value(2.0) }),
                            two) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ doubles,
                                                             two, two }),
                            m::value(2.0)) &&
                        d_tests_is_matrix(m::internal::expr_matrix_index(
                                              { wide, two }),
                                          1, 3,
                                          { 4, 5, 6 },
                                          storage::values) &&
                        d_tests_is_matrix(m::internal::expr_matrix_index(
                                              { floats, one }),
                                          1, 2,
                                          { 0.5f, 1.0f },
                                          storage::floats),
                        "A[i, j] an element, from 1; v[i] a vector's, a "
                        "row's or a column's; A[i] of any other matrix its "
                        "row i; an index whole in any kind");

        refused = true;

        // what indexing does not take
        const m::value bad[] = {
            m::internal::expr_matrix_index({ a, m::value(3) }),
            m::internal::expr_matrix_index({ a, m::value(0), one }),
            m::internal::expr_matrix_index({ a, one, m::value(3) }),
            m::internal::expr_matrix_index({ a, one, one, one }),
            m::internal::expr_matrix_index({ a }),
            m::internal::expr_matrix_index({ two, one }),
            m::internal::expr_matrix_index({ m::value(true), one }),
            m::internal::expr_matrix_index({ col, m::value(2.5) }),
            m::internal::expr_matrix_index({ col, m::value::ratio(1, 2) }),
            m::internal::expr_matrix_index({ col, m::value(true) }),
            m::internal::expr_matrix_index({ col, m::value() }),
            m::internal::expr_matrix_index({ col, a }),
            m::internal::expr_matrix_index(
                { col,
                  m::value(std::numeric_limits<double>::quiet_NaN()) }),
            m::internal::expr_matrix_index(
                { col,
                  m::value(std::numeric_limits<double>::infinity()) }),
            m::internal::expr_matrix_index({ col, m::value(-1) }) };

        for (std::size_t i = 0; i < (sizeof(bad) / sizeof(bad[0])); ++i)
        {
            refused = ( (refused) &&
                        (d_tests_same_values(bad[i],
                                             domain)) );
        }

        D_TESTS_EXPECT( refused,
                        "an index past the places, of 0 or below, not "
                        "whole, a boolean, none or a matrix; three indices, "
                        "or none; indexing what is no matrix: a domain "
                        "error");
        D_TESTS_EXPECT( d_tests_same_values(
                            m::internal::expr_matrix_index({ pole, range }),
                            pole) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ a, range }),
                            range) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ two, range }),
                            range) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ a, one,
                                                             pole }),
                            pole),
                        "an error passes, the first: the matrix's, then an "
                        "index's");

        const m::value same = d_tests_made(m::expr_matrix(2,
                                                          2,
                                                          { 1.0, 2.0, 3.0,
                                                            4.0 }));

        D_TESTS_EXPECT( d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::eq,
                                a,
                                same),
                            m::value(true)) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::ne,
                                a,
                                same),
                            m::value(false)) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::eq,
                                row,
                                col),
                            m::value(false)) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::ne,
                                row,
                                col),
                            m::value(true)) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::lt,
                                a,
                                same),
                            domain) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::eq,
                                a,
                                one),
                            domain) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::eq,
                                pole,
                                a),
                            pole),
                        "A = B and A != B by the kind's comparison, of one "
                        "shape and equal place by place (S242); an order "
                        "of matrices, or a matrix and a number, a domain "
                        "error; an error passes");

        return;
    }

    // d_tests_index_of
    //   the term of _operand indexed by the indices given.
    m::expr_term
    d_tests_index_of(
        const m::expr_term&              _operand,
        const std::vector<m::expr_term>& _indices
    )
    {
        std::vector<m::expr_term> operands(1, _operand);

        operands.insert(operands.end(),
                        _indices.begin(),
                        _indices.end());

        return m::expr_application(m::expr_op_kind::index,
                                   operands);
    }

    // d_tests_parse_indexing
    //   step 20's indexing and transpose in the parser layer (S252): v[i]
    // and A[i, j], each index any term, tighter than any operator; the
    // transpose postfix, as tight as the factorial; both with implied
    // multiplication and bars; and every error at its place.
    void
    d_tests_parse_indexing()
    {
        const m::expr_term one = d_tests_literal("1");
        const m::expr_term two = d_tests_literal("2");
        const m::expr_term a   = m::expr_variable("A");
        const m::expr_term b   = m::expr_variable("B");
        const m::expr_term v   = m::expr_variable("v");
        const m::expr_term x   = m::expr_variable("x");
        const m::expr_term v1  = d_tests_index_of(v,
                                                  { one });
        const m::expr_term at  = d_tests_apply(m::expr_op_kind::transpose,
                                               a);
        m::expr_syntax     implied;
        m::expr_syntax     bars;

        implied.implied_multiplication = true;
        bars.absolute_bars             = true;

        D_TESTS_EXPECT( d_tests_parses("v[1]",
                                       v1) &&
                        d_tests_parses("v [1]",
                                       v1) &&
                        d_tests_parses("A[1, 2]",
                                       d_tests_index_of(a,
                                                        { one, two })) &&
                        d_tests_parses("A[x + 1, 2]",
                                       d_tests_index_of(
                                           a,
                                           { d_tests_apply(
                                                 m::expr_op_kind::add,
                                                 x,
                                                 one),
                                             two })) &&
                        d_tests_parses("v[v[1]]",
                                       d_tests_index_of(v,
                                                        { v1 })) &&
                        d_tests_parses("A[1][2]",
                                       d_tests_index_of(
                                           d_tests_index_of(a,
                                                            { one }),
                                           { two })),
                        "v[i] and A[i, j], each index any term, an index "
                        "among them; A[i][j] the row's element");
        D_TESTS_EXPECT( d_tests_parses("-v[1]",
                                       d_tests_apply(m::expr_op_kind::neg,
                                                     v1)) &&
                        d_tests_parses("2 ^ v[1]",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     two,
                                                     v1)) &&
                        d_tests_parses("v[1] ^ 2",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     v1,
                                                     two)) &&
                        d_tests_parses("v[1]!",
                                       d_tests_apply(
                                           m::expr_op_kind::factorial,
                                           v1)) &&
                        d_tests_parses("x'[1]",
                                       d_tests_index_of(
                                           d_tests_apply(
                                               m::expr_op_kind::transpose,
                                               x),
                                           { one })) &&
                        d_tests_parses("f(x)[1]",
                                       d_tests_index_of(
                                           m::expr_call("f",
                                                        { x }),
                                           { one })) &&
                        d_tests_parses("(x + 1)[2]",
                                       d_tests_index_of(
                                           d_tests_apply(
                                               m::expr_op_kind::add,
                                               x,
                                               one),
                                           { two })) &&
                        d_tests_parses("[1, 2][2]",
                                       d_tests_index_of(
                                           d_tests_vector_of(
                                               { one, two }),
                                           { two })) &&
                        d_tests_parses("1[2]",
                                       d_tests_index_of(one,
                                                        { two })),
                        "indexing tighter than any operator: of a "
                        "variable, a transpose, a call, a group, a literal "
                        "or a number");
        D_TESTS_EXPECT( d_tests_parses("A'",
                                       at) &&
                        d_tests_parses("A''",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           at)) &&
                        d_tests_parses("A' ^ 2",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     at,
                                                     two)) &&
                        d_tests_parses("A ^ 2'",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           d_tests_apply(
                                               m::expr_op_kind::pow,
                                               a,
                                               two))) &&
                        d_tests_parses("-A'",
                                       d_tests_apply(m::expr_op_kind::neg,
                                                     at)) &&
                        d_tests_parses("x!'",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           d_tests_apply(
                                               m::expr_op_kind::factorial,
                                               x))) &&
                        d_tests_parses("[1, 2]'",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           d_tests_vector_of(
                                               { one, two }))) &&
                        d_tests_parses("A' * B",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     at,
                                                     b)) &&
                        d_tests_parses("v[1]'",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           v1)),
                        "A' postfix, between the unary - and ^: A' ^ 2 is "
                        "(A') ^ 2, A ^ 2' is (A ^ 2)', as MATLAB reads "
                        "both");
        D_TESTS_EXPECT( d_tests_parses("2v[1]",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     two,
                                                     v1),
                                       implied) &&
                        d_tests_parses("v[1]x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     v1,
                                                     x),
                                       implied) &&
                        d_tests_parses("A'B",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     at,
                                                     b),
                                       implied) &&
                        d_tests_parses("2A'",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     two,
                                                     at),
                                       implied) &&
                        d_tests_parses("x[1]",
                                       d_tests_index_of(x,
                                                        { one }),
                                       implied) &&
                        d_tests_parses("|v[1]|",
                                       m::expr_call("abs",
                                                    { v1 }),
                                       bars) &&
                        d_tests_parses("|A|'",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           m::expr_call("abs",
                                                        { a })),
                                       bars),
                        "with implied multiplication a * after an index "
                        "or a transpose, and none before an index; bars "
                        "around and before them");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("x[]"),
                                         m::expr_status::missing_operand,
                                         2,
                                         "an operand is missing before "
                                         "']'") &&
                        d_tests_is_error(m::expr_parse("x[1, ]"),
                                         m::expr_status::missing_operand,
                                         5,
                                         "an operand is missing before "
                                         "']'") &&
                        d_tests_is_error(m::expr_parse("x[, 1]"),
                                         m::expr_status::missing_operand,
                                         2,
                                         "an operand is missing before "
                                         "','") &&
                        d_tests_is_error(m::expr_parse("x[1, 2, 3]"),
                                         m::expr_status::wrong_arity,
                                         1,
                                         "indexing takes 1 or 2 indices, "
                                         "not 3") &&
                        d_tests_is_error(m::expr_parse("x[1; 2]"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected ';'") &&
                        d_tests_is_error(m::expr_parse("x[1 2]"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '2'") &&
                        d_tests_is_error(m::expr_parse("x[1)"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'[' is never closed") &&
                        d_tests_is_error(m::expr_parse("x]"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "']' closes no bracket") &&
                        d_tests_is_error(m::expr_parse("x[1]]"),
                                         m::expr_status::unbalanced,
                                         4,
                                         "']' closes no bracket") &&
                        d_tests_is_error(m::expr_parse("'x"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '''") &&
                        d_tests_is_error(m::expr_parse("x[1, 2, 3"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'[' is never closed"),
                        "an index missing, three indices, a semicolon or "
                        "two operands in an index, a bracket unbalanced, "
                        "a transpose of nothing: each error at its place, "
                        "the brackets' before the grammar's");

        return;
    }

    // d_tests_render_indexing
    //   indexing and the transpose in the renderer: an index after its
    // operand, bare where the operand is a leaf, a call, a literal, an
    // index or a postfix operator, and bracketed otherwise; the transpose
    // as the factorial is; and each read back as itself.
    void
    d_tests_render_indexing()
    {
        const char* texts[][2] = {
            { "v[1]", "v[1]" },             { "A[1,2]", "A[1, 2]" },
            { "(a+b)[1]", "(a + b)[1]" },   { "(-x)[1]", "(-x)[1]" },
            { "-x[1]", "-x[1]" },           { "(x^2)[1]", "(x ^ 2)[1]" },
            { "x'[1]", "x'[1]" },           { "(x!)[1]", "x![1]" },
            { "f(x)[1]", "f(x)[1]" },       { "[1,2][1]", "[1, 2][1]" },
            { "A[1][2]", "A[1][2]" },       { "(x[1])'", "x[1]'" },
            { "(a+b)'", "(a + b)'" },       { "A'^2", "(A') ^ 2" },
            { "A^2'", "A ^ 2'" },           { "(A^2)'", "A ^ 2'" },
            { "A^(2')", "A ^ (2')" },       { "x'!", "(x')!" },
            { "-A'", "-A'" },               { "(-A)'", "(-A)'" },
            { "2^v[1]", "2 ^ v[1]" },       { "1[2]", "1[2]" },
            { "A[i+1,j*2]", "A[i + 1, j * 2]" },
            { "v[v[1]]", "v[v[1]]" },       { "A''", "A''" },
            { "(1/3)[1]", "(1 / 3)[1]" },   { "[1;2]'[2]", "[1; 2]'[2]" } };
        bool rendered = true;
        bool back     = true;

        for (std::size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); ++i)
        {
            const m::expr_term term = d_tests_parsed(texts[i][0]);

            rendered = ( (rendered) &&
                         (d_tests_renders(texts[i][0],
                                          texts[i][1])) );
            back     = ( (back) &&
                         (d_tests_parses(texts[i][1],
                                         term)) );
        }

        D_TESTS_EXPECT( rendered,
                        "an index after its operand, bare after a leaf, a "
                        "call, a literal, an index or a postfix operator, "
                        "bracketed otherwise; the transpose as the "
                        "factorial");
        D_TESTS_EXPECT( back,
                        "and each read back as itself");
        D_TESTS_EXPECT( (m::expr_render(d_tests_index_of(
                             m::expr_number(m::value(-1), "-1"),
                             { d_tests_literal("2") })) == "(-1)[2]") &&
                        (m::expr_render(d_tests_index_of(
                             m::expr_number(m::value::ratio(1, 3), "1/3"),
                             { d_tests_literal("1") })) == "(1/3)[1]") &&
                        (m::expr_render(m::expr_application(
                             m::expr_op_kind::index,
                             { m::expr_variable("v") })) == "v[]"),
                        "a literal no unsigned decimal in brackets before "
                        "its index; an index of no index, made by hand, "
                        "its brackets alone");

        return;
    }

    // d_tests_fold_indexing
    //   both folds of matrix texts: as a value, the kind's arithmetic,
    // powers, the transpose, indexing and the relations (S246 to S249); in
    // a type, a NaN for indexing and a number's transpose the number.
    void
    d_tests_fold_indexing()
    {
        const m::value         a       = d_tests_two_by_two();
        const m::value         v       = d_tests_made(
                                             m::expr_vector({ 5, 6, 7 }));
        const m::value         wide    = d_tests_made(
                                             m::expr_matrix(2,
                                                            3,
                                                            { 1, 2, 3, 4, 5,
                                                              6 }));
        m::expr_bindings       bindings;
        const m::value         domain(D_MATH_VALUE_ERROR_DOMAIN);

        static_cast<void>(bindings.at("A", a)
                                  .at("v", v)
                                  .at("B", wide)
                                  .at("x", 2)
                                  .at("y", 3));

        D_TESTS_EXPECT( d_tests_valued("A[2, 1]", m::value(3), bindings) &&
                        d_tests_valued("v[3]", m::value(7), bindings) &&
                        d_tests_valued("v[x + 1]", m::value(7), bindings) &&
                        d_tests_valued("A[x][1]", m::value(3), bindings) &&
                        d_tests_valued("B[2]",
                                       d_tests_made(m::expr_vector(
                                           { 4, 5, 6 })),
                                       bindings) &&
                        d_tests_valued("[1, 2; 3, 4][1, 2]", m::value(2),
                                       bindings) &&
                        d_tests_valued("v[1] * v[2]", m::value(30),
                                       bindings) &&
                        d_tests_valued("-v[1]", m::value(-5), bindings),
                        "indexing from 1, an index any term: an element, "
                        "or another matrix's row");
        D_TESTS_EXPECT( d_tests_valued("A + A", a * m::value(2), bindings) &&
                        d_tests_valued("A * A", a * a, bindings) &&
                        d_tests_valued("2 * A - A", a, bindings) &&
                        d_tests_valued("A / 2", a / m::value(2), bindings) &&
                        d_tests_valued("-A", -a, bindings) &&
                        d_tests_valued("+A", a, bindings) &&
                        d_tests_valued("A ^ 3", (a * a) * a, bindings) &&
                        d_tests_valued("A ^ 0",
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 1, 0, 0, 1 })),
                                       bindings) &&
                        d_tests_valued("pow(A, 2)", a * a, bindings) &&
                        d_tests_valued("B * B'",
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 14, 32, 32, 77 })),
                                       bindings) &&
                        d_tests_valued("A'", m::internal::
                                                 expr_matrix_transposed(a),
                                       bindings) &&
                        d_tests_valued("x'", m::value(2), bindings) &&
                        d_tests_valued("[1, 2] * [3; 4]",
                                       d_tests_made(m::expr_vector({ 11 })),
                                       bindings) &&
                        d_tests_valued("[x, y] * 2",
                                       d_tests_made(m::expr_vector(
                                           { 4, 6 })),
                                       bindings),
                        "the kind's arithmetic as a value: sums, scalar "
                        "products, the product, the transpose and powers, "
                        "a 1 by 1 product a matrix");
        D_TESTS_EXPECT( d_tests_valued("A = A", m::value(true), bindings) &&
                        d_tests_valued("A != [1, 2; 3, 4]", m::value(false),
                                       bindings) &&
                        d_tests_valued("A = B", m::value(false), bindings) &&
                        d_tests_valued("A != B", m::value(true), bindings) &&
                        d_tests_valued("A' = A", m::value(false),
                                       bindings) &&
                        d_tests_valued("(A * A)' = A' * A'",
                                       m::value(true),
                                       bindings),
                        "two matrices equal, or not, as a relation's "
                        "value");

        bool        refused = true;
        const char* texts[] = { "A + 1", "1 - A", "1 / A", "A / B", "A * B'",
                                "B ^ 2", "B ^ (-1)", "A ^ (1 / 2)", "2 ^ A",
                                "A mod 2", "A!", "A < A", "A = 1", "abs(A)",
                                "v[0]", "v[4]", "v[1.5]", "v[true]", "x[1]",
                                "A[1, 2][1]", "B[3]", "sqrt(A)" };

        for (std::size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); ++i)
        {
            refused = ( (refused) &&
                        (d_tests_valued(texts[i],
                                        domain,
                                        bindings)) );
        }

        D_TESTS_EXPECT( refused,
                        "what the kind does not take, a power it does not "
                        "take, an index outside the matrix or not whole, "
                        "indexing a number: a domain error");

        const m::expr_result<double> indexed = m::expr_evaluate<double>(
                                                   d_tests_parsed("v[1]"),
                                                   bindings);
        const m::expr_result<double> number  = m::expr_evaluate<double>(
                                                   d_tests_parsed("x'"),
                                                   bindings);

        D_TESTS_EXPECT( (indexed.is_ok()) &&
                        (indexed.value() != indexed.value()) &&
                        (number.is_ok()) &&
                        (number.value() == 2.0) &&
                        d_tests_typed<float>("x'",
                                             2.0f,
                                             bindings) &&
                        d_tests_typed<long double>("x'",
                                                   2.0L,
                                                   bindings) &&
                        d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_parsed("[1, 2]'"),
                                             bindings),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_evaluate<Type>: a vector or a "
                                         "matrix has no value in a type"),
                        "in a type, indexing a NaN, as a variable bound to "
                        "a matrix is; a number's transpose the number; a "
                        "literal's bad_shape");

        return;
    }

    // d_tests_simplify_matrices
    //   simplification of terms of matrices (S251): a factor holding a
    // matrix keeps its place, so that A * B + B * A stays; numbers and
    // variables commute; no rule makes an error of matrices a value; and
    // each simplified term's value the term's.
    void
    d_tests_simplify_matrices()
    {
        const char* texts[][2] = {
            { "[1, 2; 3, 4] * [5, 6; 7, 8] + [5, 6; 7, 8] * [1, 2; 3, 4]",
              "[1, 2; 3, 4] * [5, 6; 7, 8] + [5, 6; 7, 8] * [1, 2; 3, 4]" },
            { "[1, 2; 3, 4] * [5, 6; 7, 8] + [1, 2; 3, 4] * [5, 6; 7, 8]",
              "2 * [1, 2; 3, 4] * [5, 6; 7, 8]" },
            { "2 * [1, 2] * x + x * [1, 2]", "3 * [1, 2] * x" },
            { "[1, 2] * [3; 4] - [3; 4] * [1, 2]",
              "[1, 2] * [3; 4] - [3; 4] * [1, 2]" },
            { "[1, 2] - [1, 2]", "0 * [1, 2]" },
            { "[1, 2] + [1, 2]", "2 * [1, 2]" },
            { "[1, 2] + 0", "[1, 2] + 0" },
            { "0 - [1, 2]", "0 - [1, 2]" },
            { "[1, 2] + 1 - 1", "[1, 2] + 0" },
            { "x - x + [1, 2]", "0 + [1, 2]" },
            { "[1, 2] + x - x + y - y", "[1, 2] + 0" },
            { "[1, 2] ^ 1", "[1, 2] ^ 1" },
            { "[1, 2; 3, 4] ^ 1", "[1, 2; 3, 4]" },
            { "[1, 2; 3, 4; 5, 6] ^ 1", "[1, 2; 3, 4; 5, 6] ^ 1" },
            { "[5] ^ 1", "[5]" },
            { "([1, 2] * [3; 4]) ^ 1", "([1, 2] * [3; 4]) ^ 1" },
            { "[1, 2; 3, 4] ^ 0", "[1, 2; 3, 4] ^ 0" },
            { "0 * [1, 2]", "0 * [1, 2]" },
            { "1 * [1, 2]", "[1, 2]" },
            { "[1, 2] / 1", "[1, 2]" },
            { "2 * [1, 2] * 3", "6 * [1, 2]" },
            { "-(-[1, 2])", "[1, 2]" },
            { "[1, 2]' + [1, 2]'", "2 * [1, 2]'" },
            { "x * y + y * x", "2 * x * y" },
            { "A ^ 0", "1" } };
        const char* const valued[] = {
            "[1, 2; 3, 4] * [5, 6; 7, 8] + [5, 6; 7, 8] * [1, 2; 3, 4]",
            "[1, 2; 3, 4] * [5, 6; 7, 8] + [1, 2; 3, 4] * [5, 6; 7, 8]",
            "2 * [1, 2] * x + x * [1, 2]", "[1, 2] - [1, 2]",
            "[1, 2] + [1, 2]", "[1, 2] + 0", "0 - [1, 2]", "[1, 2] + 1 - 1",
            "x - x + [1, 2]", "[1, 2] ^ 1", "[1, 2; 3, 4] ^ 1", "[5] ^ 1",
            "[1, 2; 3, 4; 5, 6] ^ 1",
            "([1, 2] * [3; 4]) ^ 1", "[1, 2; 3, 4] ^ 0", "0 * [1, 2]",
            "2 * [1, 2] * 3", "-(-[1, 2])", "[1, 2]' + [1, 2]'",
            "[1, 2] * [3; 4] - [3; 4] * [1, 2]" };
        bool             simplified = true;
        bool             kept       = true;
        m::expr_bindings bindings;

        static_cast<void>(bindings.at("x", 3)
                                  .at("y", m::value::ratio(1, 2)));

        for (std::size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); ++i)
        {
            simplified = ( (simplified) &&
                           (m::expr_render(m::expr_simplify(
                                d_tests_parsed(texts[i][0]))) ==
                                texts[i][1]) );
        }

        // each simplified term's value the term's, an error among them
        for (std::size_t i = 0;
             i < (sizeof(valued) / sizeof(valued[0]));
             ++i)
        {
            const m::expr_term             term   = d_tests_parsed(valued[i]);
            const m::expr_result<m::value> before =
                m::expr_evaluate(term,
                                 bindings,
                                 m::expr_context());
            const m::expr_result<m::value> after  =
                m::expr_evaluate(m::expr_simplify(term),
                                 bindings,
                                 m::expr_context());

            kept = ( (kept) &&
                     (before.is_ok()) &&
                     (d_tests_holds_value(after,
                                          before.value())) );
        }

        D_TESTS_EXPECT( simplified,
                        "a factor holding a matrix keeps its place, so "
                        "that AB + BA stays and AB + AB is 2AB; numbers "
                        "and variables commute; A ^ 1 is A where A is "
                        "square by its shape; a sum of a matrix keeps a "
                        "0; a variable taken for a number");
        D_TESTS_EXPECT( kept,
                        "each simplified term of matrices of the value of "
                        "the term, an error of shapes the same error");

        const m::expr_term square = m::expr_number(d_tests_two_by_two());
        const m::expr_term wide   = m::expr_number(
                                        d_tests_made(m::expr_vector(
                                            { 1, 2 })));
        const m::expr_term unit   = d_tests_literal("1");
        const m::expr_term zero   = d_tests_literal("0");

        D_TESTS_EXPECT( d_tests_same(m::expr_simplify(
                                         d_tests_apply(m::expr_op_kind::pow,
                                                       square,
                                                       unit)),
                                     square) &&
                        d_tests_same(m::expr_simplify(
                                         d_tests_apply(m::expr_op_kind::pow,
                                                       wide,
                                                       unit)),
                                     d_tests_apply(m::expr_op_kind::pow,
                                                   wide,
                                                   unit)) &&
                        d_tests_same(m::expr_simplify(
                                         d_tests_apply(m::expr_op_kind::add,
                                                       wide,
                                                       zero)),
                                     d_tests_apply(m::expr_op_kind::add,
                                                   wide,
                                                   zero)),
                        "a matrix a fold made: A ^ 1 A where it is square, "
                        "and kept where not; A + 0 kept");

        return;
    }

    // d_tests_reference
    //   struct: a matrix as the random part computes it, apart from the
    // kind: its shape and its elements, row by row, each an exact value; or
    // a domain error, where the operation takes no such operands.
    struct d_tests_reference
    {
        std::size_t           rows;
        std::size_t           columns;
        std::vector<m::value> elements;
        bool                  refused;

        // a domain error.
        d_tests_reference()
            : rows(0),
              columns(0),
              elements(),
              refused(true)
        {}

        // a matrix of _rows by _columns of 0s.
        d_tests_reference(
            std::size_t _rows,
            std::size_t _columns
        )
            : rows(_rows),
              columns(_columns),
              elements(_rows * _columns, m::value(0)),
              refused(false)
        {}

        // at
        //   the element at row _r, column _c.
        m::value&
        at(
            std::size_t _r,
            std::size_t _c
        )
        {
            return elements[(_r * columns) + _c];
        }

        const m::value&
        at(
            std::size_t _r,
            std::size_t _c
        ) const
        {
            return elements[(_r * columns) + _c];
        }
    };

    // d_tests_reference_sum, d_tests_reference_scaled,
    // d_tests_reference_product, d_tests_reference_transposed,
    // d_tests_reference_power
    //   functions: the random part's own arithmetic, by the definitions,
    // element by element in the value's exact arithmetic, apart from the
    // kind and its kernels: A + B or A - B, of one shape; s * A; A * B, by
    // the sums of products; A'; and A ^ n, by n - 1 products, not by
    // squaring.
    d_tests_reference
    d_tests_reference_sum(
        const d_tests_reference& _a,
        const d_tests_reference& _b,
        bool                     _minus
    )
    {
        // a refusal, or two shapes
        if ( (_a.refused) ||
             (_b.refused) ||
             (_a.rows != _b.rows) ||
             (_a.columns != _b.columns) )
        {
            return d_tests_reference();
        }

        d_tests_reference out(_a.rows,
                              _a.columns);

        for (std::size_t i = 0; i < out.elements.size(); ++i)
        {
            out.elements[i] = ( _minus ? (_a.elements[i] - _b.elements[i])
                                       : (_a.elements[i] + _b.elements[i]) );
        }

        return out;
    }

    d_tests_reference
    d_tests_reference_scaled(
        const m::value&          _s,
        const d_tests_reference& _a
    )
    {
        // a refusal
        if (_a.refused)
        {
            return _a;
        }

        d_tests_reference out(_a.rows,
                              _a.columns);

        for (std::size_t i = 0; i < out.elements.size(); ++i)
        {
            out.elements[i] = (_s * _a.elements[i]);
        }

        return out;
    }

    d_tests_reference
    d_tests_reference_product(
        const d_tests_reference& _a,
        const d_tests_reference& _b
    )
    {
        // a refusal, or shapes that do not fit
        if ( (_a.refused) ||
             (_b.refused) ||
             (_a.columns != _b.rows) )
        {
            return d_tests_reference();
        }

        d_tests_reference out(_a.rows,
                              _b.columns);

        for (std::size_t r = 0; r < out.rows; ++r)
        {
            for (std::size_t c = 0; c < out.columns; ++c)
            {
                m::value sum(0);

                for (std::size_t k = 0; k < _a.columns; ++k)
                {
                    sum = ( sum + (_a.at(r, k) * _b.at(k, c)) );
                }

                out.at(r, c) = sum;
            }
        }

        return out;
    }

    d_tests_reference
    d_tests_reference_transposed(
        const d_tests_reference& _a
    )
    {
        // a refusal
        if (_a.refused)
        {
            return _a;
        }

        d_tests_reference out(_a.columns,
                              _a.rows);

        for (std::size_t r = 0; r < _a.rows; ++r)
        {
            for (std::size_t c = 0; c < _a.columns; ++c)
            {
                out.at(c, r) = _a.at(r, c);
            }
        }

        return out;
    }

    d_tests_reference
    d_tests_reference_power(
        const d_tests_reference& _a,
        unsigned                 _n
    )
    {
        // a refusal, or no square
        if ( (_a.refused) ||
             (_a.rows != _a.columns) )
        {
            return d_tests_reference();
        }

        d_tests_reference out(_a.rows,
                              _a.rows);

        // the identity, then n products
        for (std::size_t i = 0; i < _a.rows; ++i)
        {
            out.at(i, i) = m::value(1);
        }

        for (unsigned k = 0; k < _n; ++k)
        {
            out = d_tests_reference_product(out,
                                            _a);
        }

        return out;
    }

    // d_tests_matrix_drawn
    //   struct: a random term of matrices and what the random part's own
    // arithmetic makes it.
    struct d_tests_matrix_drawn
    {
        m::expr_term      term;
        d_tests_reference value;
    };

    // d_tests_random_matrix
    //   a random term of _rows by _columns, of depth _depth at most: a
    // literal of whole numbers from 0 to 3 and halves, each perhaps
    // negated; a sum or a difference; a number times it; a product through
    // a random inner size; a transpose; a power, to 3 at most, of a square
    // one of depth 1 at most; or a row of a taller one; and, a draw in 27,
    // a sum or a product of shapes that do not fit, which the kind
    // refuses. Its elements stay far inside the exact integers' range.
    d_tests_matrix_drawn
    d_tests_random_matrix(
        d_tests_rng& _rng,
        std::size_t  _rows,
        std::size_t  _columns,
        unsigned     _depth
    )
    {
        unsigned             pick = ( (_depth == 0) ? 0u
                                                    : _rng.below(9u) );
        d_tests_matrix_drawn out;

        // a power of no square, or a row of no single row: a sum instead;
        // shapes that do not fit one draw in three of their ninth, a
        // product otherwise
        if ( ( (pick == 6) &&
               (_rows != _columns) ) ||
             ( (pick == 7) &&
               ( (_rows != 1) ||
                 (_columns == 1) ) ) )
        {
            pick = 1;
        }

        if ( (pick == 8) &&
             (_rng.below(3u) != 0) )
        {
            pick = 4;
        }

        // a literal
        if (pick == 0)
        {
            std::vector<std::vector<m::expr_term> > rows(_rows);

            out.value = d_tests_reference(_rows,
                                          _columns);

            for (std::size_t r = 0; r < _rows; ++r)
            {
                for (std::size_t c = 0; c < _columns; ++c)
                {
                    const unsigned whole = _rng.below(4u);
                    const bool     half  = (_rng.below(4u) == 0);
                    const bool     minus = (_rng.below(3u) == 0);
                    m::expr_term   element =
                        d_tests_literal(( half ? "0.5"
                                               : std::to_string(whole)
                                                     .c_str() ));
                    m::value       number  =
                        ( half ? m::value::ratio(1, 2)
                               : m::value(static_cast<int>(whole)) );

                    if (minus)
                    {
                        element = d_tests_apply(m::expr_op_kind::neg,
                                                element);
                        number  = -number;
                    }

                    rows[r].push_back(element);
                    out.value.at(r, c) = number;
                }
            }

            out.term = ( (_rows == 1) ? d_tests_vector_of(rows[0])
                                      : d_tests_matrix_of(rows) );

            return out;
        }

        // a sum or a difference
        if (pick <= 2)
        {
            const d_tests_matrix_drawn a = d_tests_random_matrix(_rng,
                                                                 _rows,
                                                                 _columns,
                                                                 _depth - 1);
            const d_tests_matrix_drawn b = d_tests_random_matrix(_rng,
                                                                 _rows,
                                                                 _columns,
                                                                 _depth - 1);

            out.term  = d_tests_apply( (pick == 1) ? m::expr_op_kind::add
                                                   : m::expr_op_kind::sub,
                                       a.term,
                                       b.term);
            out.value = d_tests_reference_sum(a.value,
                                              b.value,
                                              (pick == 2));

            return out;
        }

        // a number times it, on either side
        if (pick == 3)
        {
            const d_tests_matrix_drawn a     = d_tests_random_matrix(
                                                   _rng,
                                                   _rows,
                                                   _columns,
                                                   _depth - 1);
            const unsigned             whole = 1 + _rng.below(3u);
            const m::expr_term         s     = d_tests_literal(
                                                   std::to_string(whole)
                                                       .c_str());

            out.term  = ( (_rng.below(2u) == 0)
                              ? d_tests_apply(m::expr_op_kind::mul,
                                              s,
                                              a.term)
                              : d_tests_apply(m::expr_op_kind::mul,
                                              a.term,
                                              s) );
            out.value = d_tests_reference_scaled(
                            m::value(static_cast<int>(whole)),
                            a.value);

            return out;
        }

        // a product through an inner size
        if (pick == 4)
        {
            const std::size_t          inner = 1 + _rng.below(3u);
            const d_tests_matrix_drawn a     = d_tests_random_matrix(
                                                   _rng,
                                                   _rows,
                                                   inner,
                                                   _depth - 1);
            const d_tests_matrix_drawn b     = d_tests_random_matrix(
                                                   _rng,
                                                   inner,
                                                   _columns,
                                                   _depth - 1);

            out.term  = d_tests_apply(m::expr_op_kind::mul,
                                      a.term,
                                      b.term);
            out.value = d_tests_reference_product(a.value,
                                                  b.value);

            return out;
        }

        // a transpose
        if (pick == 5)
        {
            const d_tests_matrix_drawn a = d_tests_random_matrix(_rng,
                                                                 _columns,
                                                                 _rows,
                                                                 _depth - 1);

            out.term  = d_tests_apply(m::expr_op_kind::transpose,
                                      a.term);
            out.value = d_tests_reference_transposed(a.value);

            return out;
        }

        // a power of a square one
        if (pick == 6)
        {
            const d_tests_matrix_drawn a = d_tests_random_matrix(
                                               _rng,
                                               _rows,
                                               _rows,
                                               ( (_depth > 1) ? 1u
                                                              : 0u ));
            const unsigned             n = _rng.below(4u);

            out.term  = d_tests_apply(m::expr_op_kind::pow,
                                      a.term,
                                      d_tests_literal(std::to_string(n)
                                                          .c_str()));
            out.value = d_tests_reference_power(a.value,
                                                n);

            return out;
        }

        // a row of a taller matrix, of more than one column
        if (pick == 7)
        {
            const std::size_t          taller = 2 + _rng.below(2u);
            const std::size_t          chosen = _rng.below(
                                                    static_cast<unsigned>(
                                                        taller));
            const d_tests_matrix_drawn a      = d_tests_random_matrix(
                                                    _rng,
                                                    taller,
                                                    _columns,
                                                    _depth - 1);

            out.term  = d_tests_index_of(a.term,
                                         { d_tests_literal(
                                               std::to_string(chosen + 1)
                                                   .c_str()) });
            out.value = d_tests_reference(1,
                                          _columns);
            out.value.refused = a.value.refused;

            for (std::size_t c = 0;
                 ( (c < _columns) && (!a.value.refused) );
                 ++c)
            {
                out.value.at(0, c) = a.value.at(chosen, c);
            }

            return out;
        }

        // shapes that do not fit: A + B of B's columns not A's, or A * B
        // of B's rows not A's columns, each a sum or a product the kind
        // refuses
        const std::size_t          other = ( (_columns % 3) + 1 );
        const bool                 sum   = (_rng.below(2u) == 0);
        const d_tests_matrix_drawn a     = d_tests_random_matrix(_rng,
                                                                 _rows,
                                                                 _columns,
                                                                 _depth - 1);
        const d_tests_matrix_drawn b     = d_tests_random_matrix(
                                               _rng,
                                               ( sum ? _rows
                                                     : other ),
                                               ( sum ? other
                                                     : _columns ),
                                               _depth - 1);

        out.term  = d_tests_apply( sum ? m::expr_op_kind::add
                                       : m::expr_op_kind::mul,
                                   a.term,
                                   b.term);
        out.value = ( sum ? d_tests_reference_sum(a.value,
                                                  b.value,
                                                  false)
                          : d_tests_reference_product(a.value,
                                                      b.value) );

        return out;
    }

    // d_tests_agrees
    //   whether a fold's value is the reference: the same matrix, element
    // by element, each element exact as the reference's is; or a domain
    // error where the reference is refused.
    bool
    d_tests_agrees(
        const m::expr_result<m::value>& _folded,
        const d_tests_reference&        _want
    )
    {
        // a fold that failed
        if (_folded.is_err())
        {
            return false;
        }

        // a refusal: a domain error
        if (_want.refused)
        {
            return d_tests_same_values(_folded.value(),
                                       m::value(D_MATH_VALUE_ERROR_DOMAIN));
        }

        const m::expr_shape shape = d_tests_shape(_folded.value());
        bool                same  = ( (shape.rows == _want.rows) &&
                                      (shape.columns == _want.columns) );

        for (std::size_t r = 0; ( (same) && (r < _want.rows) ); ++r)
        {
            for (std::size_t c = 0; c < _want.columns; ++c)
            {
                same = ( (same) &&
                         (d_tests_same_values(
                              d_tests_made(m::expr_element(_folded.value(),
                                                           r,
                                                           c)),
                              _want.at(r, c))) );
            }
        }

        return same;
    }

    // d_tests_matrices_random
    //   1,000 random terms of matrices of one to three rows and columns,
    // drawn by a generator of their own (S254), each folded as a value and
    // compared with the random part's own arithmetic, by the definitions,
    // apart from the kind: the same matrix, exactly, or a domain error for
    // shapes that do not fit; each rendered and read back as itself;
    // simplified, no larger and of the same value; and in a type bad_shape.
    void
    d_tests_matrices_random()
    {
        d_tests_rng           rng(20261026u);
        const m::expr_context context;
        std::size_t           agreed     = 0;
        std::size_t           back       = 0;
        std::size_t           simplified = 0;
        std::size_t           smaller    = 0;
        std::size_t           typed      = 0;
        std::size_t           refused    = 0;
        const std::size_t     count      = 1000;

        for (std::size_t n = 0; n < count; ++n)
        {
            const std::size_t              rows    = 1 + rng.below(3u);
            const std::size_t              columns = 1 + rng.below(3u);
            const d_tests_matrix_drawn     drawn   =
                d_tests_random_matrix(rng,
                                      rows,
                                      columns,
                                      3u);
            const m::expr_result<m::value> folded  =
                m::expr_evaluate(drawn.term,
                                 m::expr_bindings(),
                                 context);
            const m::expr_term             simple  =
                m::expr_simplify(drawn.term);
            const m::expr_result<double>   in_type =
                m::expr_evaluate<double>(drawn.term);

            refused += ( drawn.value.refused ? 1 : 0 );
            agreed  += ( d_tests_agrees(folded,
                                        drawn.value) ? 1 : 0 );
            back    += ( d_tests_parses(m::expr_render(drawn.term).c_str(),
                                        drawn.term) ? 1 : 0 );
            smaller += ( (::djinterp::size<m::expr_op, m::expr_atom>(simple) <=
                          ::djinterp::size<m::expr_op, m::expr_atom>(
                              drawn.term)) ? 1 : 0 );
            simplified += ( d_tests_agrees(m::expr_evaluate(simple,
                                                            m::expr_bindings(),
                                                            context),
                                           drawn.value) ? 1 : 0 );
            typed   += ( ( (in_type.is_err()) &&
                           (in_type.error().status ==
                                m::expr_status::bad_shape) ) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (refused > (count / 20)) &&
                        (refused < (count / 2)),
                        "some of the random terms of shapes that do not "
                        "fit, most of shapes that do");
        D_TESTS_EXPECT( (agreed == count),
                        "1,000 random terms of matrices folded as values: "
                        "each the matrix the definitions give, exactly, or "
                        "a domain error where shapes do not fit");
        D_TESTS_EXPECT( (back == count),
                        "each rendered and read back as itself");
        D_TESTS_EXPECT( (smaller == count) &&
                        (simplified == count),
                        "each simplified no larger, and of the same value, "
                        "a domain error the same error");
        D_TESTS_EXPECT( (typed == count),
                        "in a type, each bad_shape");

        return;
    }

    // d_tests_dense_values
    //   a dense matrix's elements, row by row, as values of Type.
    template<typename Type>
    std::vector<m::value>
    d_tests_dense_values(
        const Type*  _entries,
        std::size_t  _count
    )
    {
        std::vector<m::value> elements;

        for (std::size_t i = 0; i < _count; ++i)
        {
            elements.push_back(m::value(_entries[i]));
        }

        return elements;
    }

    // d_tests_algebra_in
    //   whether the square algebra of the dense A = [4, 3, 1; 6, 3, 2;
    // 1, 5, 7] in Type is the C core's in Type, bit for bit: det the
    // parity times U's diagonal of d_decomp_lu, inv d_mat_inverse's, solve
    // of b = [1, 2, 3] by the LU's d_decomp_lu_solve, given as a row; and
    // trace 14, each a dense Type, held in _storage.
    template<typename Type>
    bool
    d_tests_algebra_in(
        m::internal::expr_matrix_storage _storage
    )
    {
        const Type     entries[9] = { Type(4), Type(3), Type(1),
                                      Type(6), Type(3), Type(2),
                                      Type(1), Type(5), Type(7) };
        const Type     right[3]   = { Type(1), Type(2), Type(3) };
        const m::value a          = d_tests_made(
                                        m::expr_matrix(3,
                                                       3,
                                                       d_tests_dense_values(
                                                           entries,
                                                           9)));
        const m::value b          = d_tests_made(
                                        m::expr_vector(d_tests_dense_values(
                                                           right,
                                                           3)));
        Type           lu[9]      = {};
        Type           inverse[9] = {};
        Type           scratch[9] = {};
        Type           x[3]       = {};
        std::size_t    perm[3]    = {};
        Type           sign       = Type(1);

        for (std::size_t i = 0; i < 9; ++i)
        {
            lu[i] = entries[i];
        }

        static_cast<void>(d_decomp_lu(lu,
                                      3,
                                      perm,
                                      &sign));
        d_decomp_lu_solve(lu,
                          perm,
                          3,
                          right,
                          x);
        static_cast<void>(d_mat_inverse(inverse,
                                        scratch,
                                        entries,
                                        3));

        volatile Type det = d_decomp_lu_det(lu,
                                            3,
                                            sign);

        const Type     flat[4]    = { Type(1), Type(2), Type(2), Type(4) };
        const Type     general[4] = { Type(2), Type(1), Type(0), Type(3) };
        const Type     turn[4]    = { Type(0), Type(-1), Type(1), Type(0) };
        const Type     zeros[4]   = { Type(0), Type(0), Type(0), Type(0) };
        const Type     nan[4]     = { std::numeric_limits<Type>::quiet_NaN(),
                                      Type(0), Type(0), Type(1) };
        const Type     across[3]  = { Type(0), Type(1), Type(0) };
        const Type     product[3] = { Type(-3), Type(0), Type(1) };
        const m::value singular   = d_tests_made(
                                        m::expr_matrix(2,
                                                       2,
                                                       d_tests_dense_values(
                                                           flat,
                                                           4)));
        const m::value pair       = d_tests_made(
                                        m::expr_vector(d_tests_dense_values(
                                                           right,
                                                           2)));
        const m::value other      = d_tests_made(
                                        m::expr_vector(d_tests_dense_values(
                                                           across,
                                                           3)));
        const m::value pole(D_MATH_VALUE_ERROR_POLE);
        const m::value nans       = d_tests_made(m::expr_eig(
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            d_tests_dense_values(nan,
                                                                 4)))));
        Type           first      = Type(0);
        Type           nan_norm   = Type(0);
        volatile Type  dot        = Type(14);
        volatile Type  norm       = d_math_sqrt(Type(14));

        static_cast<void>(d_tests_made(m::expr_element(nans,
                                                       0,
                                                       0)).to(first));

        return ( (d_tests_same_values(d_tests_made(m::expr_det(a)),
                                      m::value(static_cast<Type>(det)))) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_inv(a)),
                                    3, 3,
                                    d_tests_dense_values(inverse,
                                                         9),
                                    _storage)) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_solve(a,
                                                               b)),
                                    1, 3,
                                    d_tests_dense_values(x,
                                                         3),
                                    _storage)) &&
                 (d_tests_same_values(d_tests_made(m::expr_trace(a)),
                                      m::value(Type(14)))) &&
                 (d_tests_same_values(d_tests_made(m::expr_inv(singular)),
                                      pole)) &&
                 (d_tests_same_values(d_tests_made(m::expr_solve(singular,
                                                                 pair)),
                                      pole)) &&
                 (d_tests_same_values(d_tests_made(m::expr_dot(b,
                                                               b)),
                                      m::value(static_cast<Type>(dot)))) &&
                 (d_tests_same_values(d_tests_made(m::expr_norm(b)),
                                      m::value(static_cast<Type>(norm)))) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_cross(b,
                                                               other)),
                                    1, 3,
                                    d_tests_dense_values(product,
                                                         3),
                                    _storage)) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_cross(
                                        d_tests_made(m::expr_matrix(
                                            3,
                                            1,
                                            d_tests_dense_values(right,
                                                                 3))),
                                        other)),
                                    3, 1,
                                    d_tests_dense_values(product,
                                                         3),
                                    _storage)) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_eig(
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            d_tests_dense_values(general,
                                                                 4))))),
                                    1, 2,
                                    { m::value(Type(2)), m::value(Type(3)) },
                                    _storage)) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_eig(
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            d_tests_dense_values(zeros,
                                                                 4))))),
                                    1, 2,
                                    { m::value(Type(0)), m::value(Type(0)) },
                                    _storage)) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_eig(
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            d_tests_dense_values(turn,
                                                                 4))))),
                                    1, 2,
                                    { m::expr_complex_of(
                                          m::complex<Type>(Type(0),
                                                           Type(-1))),
                                      m::expr_complex_of(
                                          m::complex<Type>(Type(0),
                                                           Type(1))) },
                                    m::internal::expr_storage_complex_of(
                                        _storage))) &&
                 (first != first) &&
                 (d_tests_stored_as(nans,
                                    _storage)) &&
                 (d_tests_same_values(d_tests_made(m::expr_norm(
                                          d_tests_made(m::expr_matrix(
                                              2,
                                              2,
                                              d_tests_dense_values(zeros,
                                                                   4))))),
                                      m::value(Type(0)))) &&
                 (d_tests_made(m::expr_norm(
                      d_tests_made(m::expr_matrix(
                          2,
                          2,
                          d_tests_dense_values(nan,
                                               4))))).to(nan_norm)) &&
                 (nan_norm != nan_norm) );
    }

    // d_tests_cofactors
    //   the test's own determinant of an exact square matrix, of _n rows,
    // its elements row by row: the cofactor expansion along the first
    // row, apart from the kind and its kernels.
    m::value
    d_tests_cofactors(
        const std::vector<m::value>& _a,
        std::size_t                  _n
    )
    {
        m::value sum(0);

        // 0 by 0: 1; 1 by 1: the element
        if (_n == 0)
        {
            return m::value(1);
        }

        if (_n == 1)
        {
            return _a[0];
        }

        for (std::size_t j = 0; j < _n; ++j)
        {
            std::vector<m::value> minor;

            for (std::size_t r = 1; r < _n; ++r)
            {
                for (std::size_t c = 0; c < _n; ++c)
                {
                    if (c != j)
                    {
                        minor.push_back(_a[(r * _n) + c]);
                    }
                }
            }

            const m::value term = _a[j] * d_tests_cofactors(minor,
                                                            _n - 1);

            sum = ( ((j % 2) == 0) ? (sum + term) : (sum - term) );
        }

        return sum;
    }

    // d_tests_elements_of
    //   a matrix's elements, row by row; none where the value is no matrix.
    std::vector<m::value>
    d_tests_elements_of(
        const m::value& _x
    )
    {
        const m::expr_shape   shape = d_tests_shape(_x);
        std::vector<m::value> elements;

        for (std::size_t r = 0; r < shape.rows; ++r)
        {
            for (std::size_t c = 0; c < shape.columns; ++c)
            {
                elements.push_back(d_tests_made(m::expr_element(_x,
                                                                r,
                                                                c)));
            }
        }

        return elements;
    }

    // d_tests_near
    //   whether a value is a number within _tolerance of _want.
    bool
    d_tests_near(
        const m::value& _x,
        long double     _want,
        long double     _tolerance
    )
    {
        long double x = 0.0L;

        return ( (_x.is_number()) &&
                 (_x.to(x)) &&
                 (std::fabs(x - _want) <= _tolerance) );
    }

    // d_tests_element_near
    //   whether a matrix's element at _place, row by row, is within
    // _tolerance of _want.
    bool
    d_tests_element_near(
        const m::value& _x,
        std::size_t     _place,
        long double     _want,
        long double     _tolerance
    )
    {
        const std::vector<m::value> elements = d_tests_elements_of(_x);

        return ( (_place < elements.size()) &&
                 (d_tests_near(elements[_place],
                               _want,
                               _tolerance)) );
    }

    // d_tests_matrix_algebra
    //   step 21's square algebra as values: det, inv, trace and solve,
    // each dense type's the C core's in that type, bit for bit; a matrix of
    // values with an inexact element in the widest inexact type; an exact
    // matrix's exactly, the test's own cofactors its determinant; a
    // singular one's inverse and solution a pole; a boxed inexact element's
    // own arithmetic; the flags carried; and an error passing first, a
    // boolean or none a domain error.
    void
    d_tests_matrix_algebra()
    {
        typedef m::internal::expr_matrix_storage storage;

        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);
        const m::value pole(D_MATH_VALUE_ERROR_POLE);

        D_TESTS_EXPECT( (d_tests_algebra_in<float>(storage::floats)) &&
                        (d_tests_algebra_in<double>(storage::doubles)) &&
                        (d_tests_algebra_in<long double>(
                            storage::long_doubles)),
                        "each dense type's algebra in that type, the C "
                        "core's, bit for bit: det, inv and solve by LU with "
                        "partial pivoting and Gauss-Jordan, a singular "
                        "matrix's a pole; trace, dot, norm and cross, the "
                        "zero matrix's norm 0 and a NaN's a NaN; eig by QR, "
                        "of the zero matrix its zeros, of a turn -i and i, "
                        "complex (S352), of a NaN NaNs");

        const long double root_two = 1.4142135623730950488L;
        const long double mixed[4] = { root_two, 1.0L, 1.0L, 1.0L };
        const m::value    with_ld  = d_tests_made(
                                         m::expr_matrix(2,
                                                        2,
                                                        { m::value(root_two),
                                                          1,
                                                          1,
                                                          1 }));
        const m::value    with_f   = d_tests_made(
                                         m::expr_matrix(2,
                                                        2,
                                                        { m::value(0.5f),
                                                          1,
                                                          1,
                                                          2 }));
        long double       lu[4]    = { root_two, 1.0L, 1.0L, 1.0L };
        long double       inv[4]   = {};
        long double       work[4]  = {};
        std::size_t       perm[2]  = {};
        long double       sign     = 1.0L;
        float             lu_f[4]  = { 0.5f, 1.0f, 1.0f, 2.0f };
        std::size_t       perm_f[2] = {};
        float             sign_f   = 1.0f;
        const m::value    with_two = d_tests_made(
                                         m::expr_matrix(2,
                                                        2,
                                                        { m::value(0.1L),
                                                          1,
                                                          1,
                                                          m::value(0.25f) }));
        long double       lu_two[4] = { 0.1L, 1.0L, 1.0L, 0.25L };
        std::size_t       perm_two[2] = {};
        long double       sign_two = 1.0L;
        const m::value    exact_a  = d_tests_made(m::expr_matrix(2,
                                                                 2,
                                                                 { 3, 1,
                                                                   2, 7 }));
        const double      b_d[2]   = { 0.1, 0.3 };
        double            lu_d[4]  = { 3.0, 1.0, 2.0, 7.0 };
        double            x_d[2]   = {};
        std::size_t       perm_d[2] = {};
        double            sign_d   = 1.0;

        static_cast<void>(d_decomp_lu(lu,
                                      2,
                                      perm,
                                      &sign));
        static_cast<void>(d_decomp_lu(lu_f,
                                      2,
                                      perm_f,
                                      &sign_f));
        static_cast<void>(d_mat_inverse(inv,
                                        work,
                                        mixed,
                                        2));
        static_cast<void>(d_decomp_lu(lu_two,
                                      2,
                                      perm_two,
                                      &sign_two));
        static_cast<void>(d_decomp_lu(lu_d,
                                      2,
                                      perm_d,
                                      &sign_d));
        d_decomp_lu_solve(lu_d,
                          perm_d,
                          2,
                          b_d,
                          x_d);

        volatile long double det_ld  = d_decomp_lu_det(lu,
                                                       2,
                                                       sign);
        volatile long double det_two = d_decomp_lu_det(lu_two,
                                                       2,
                                                       sign_two);
        volatile float       det_f  = d_decomp_lu_det(lu_f,
                                                      2,
                                                      sign_f);

        D_TESTS_EXPECT( (d_tests_same_values(
                            d_tests_made(m::expr_det(with_ld)),
                            m::value(static_cast<long double>(det_ld)))) &&
                        (d_tests_is_matrix(d_tests_made(m::expr_inv(with_ld)),
                                           2, 2,
                                           d_tests_dense_values(inv,
                                                                4),
                                           storage::long_doubles)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_det(with_f)),
                            m::value(static_cast<float>(det_f)))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_det(with_two)),
                            m::value(static_cast<long double>(det_two)))) &&
                        (d_tests_is_matrix(
                            d_tests_made(m::expr_solve(
                                exact_a,
                                d_tests_made(m::expr_vector(
                                    d_tests_dense_values(b_d,
                                                         2))))),
                            1, 2,
                            d_tests_dense_values(x_d,
                                                 2),
                            storage::doubles)),
                        "a matrix of values with an inexact element: its "
                        "algebra in the widest inexact element's type, "
                        "wherever it stands, each exact element its "
                        "nearest there; an exact matrix's solution with a "
                        "dense one in the dense one's type");

        volatile float       norm_f  = d_math_sqrt(1.25f);
        volatile double      norm_d  = d_math_sqrt(1.25);
        volatile long double norm_ld = d_math_sqrt(1.25L);

        D_TESTS_EXPECT( (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(0.5f), 1 })))),
                            m::value(static_cast<float>(norm_f)))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(0.5), 1 })))),
                            m::value(static_cast<double>(norm_d)))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(0.5L), 1 })))),
                            m::value(static_cast<long double>(norm_ld)))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ 1, 1 })))),
                            m::value(d_math_sqrt(2.0L)))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(std::ldexp(1.0,
                                                                     -13)),
                                                 1 })))),
                            m::value(1.0 + std::ldexp(1.0,
                                                      -27)))),
                        "a norm of values, the root of the sum of squares "
                        "in the sum's own type, as the tower takes sqrt, a "
                        "double's 1 + 2^-26 its root in double; of an exact "
                        "sum not a square, in long double");

        const long double wide_ld  = std::ldexp(1.0L,
                                                9000);
        const bool        wide     =
            (std::numeric_limits<long double>::max_exponent > 9001);
        const m::value    big      = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(1e200),
                                               m::value(1e200) }))));
        const m::value    tiny     = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(1e-200),
                                               m::value(1e-200) }))));
        const m::value    big_f    = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(1e30f),
                                               m::value(1e30f) }))));
        const m::value    big_ld   = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(wide_ld),
                                               m::value(wide_ld) }))));
        const m::value    big_mix  = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(1e200),
                                               1 }))));
        const m::value    nan_big  = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(std::numeric_limits<
                                                   double>::quiet_NaN()),
                                               m::value(1e200) }))));
        double            nan_out  = 0.0;

        static_cast<void>(nan_big.to(nan_out));

        D_TESTS_EXPECT( (big.kind() == D_MATH_VALUE_DOUBLE) &&
                        (d_tests_near(big,
                                      1.4142135623730950488e200L,
                                      1e185L)) &&
                        (d_tests_near(tiny,
                                      1.4142135623730950488e-200L,
                                      1e-215L)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(
                                    std::numeric_limits<double>::
                                        denorm_min()) })))),
                            m::value(std::numeric_limits<double>::
                                         denorm_min()))) &&
                        (big_f.kind() == D_MATH_VALUE_FLOAT) &&
                        (d_tests_near(big_f,
                                      1.41421356e30L,
                                      1e24L)) &&
                        ( (!wide) ||
                          ( (big_ld.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                            (d_tests_near(big_ld,
                                          1.4142135623730950488L * wide_ld,
                                          wide_ld * 1e-15L)) ) ) &&
                        (big_mix.kind() == D_MATH_VALUE_DOUBLE) &&
                        (d_tests_near(big_mix,
                                      1e200L,
                                      1e185L)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(
                                    std::numeric_limits<double>::
                                        infinity()),
                                                 m::value(1.0) })))),
                            m::value(std::numeric_limits<double>::
                                         infinity()))) &&
                        (nan_out != nan_out),
                        "a norm whose squares pass the type's range, or "
                        "fall below its normal numbers' precision, scaled "
                        "by a power of two, dense or of values, in each "
                        "type: the norm the type holds, not an infinity or "
                        "0; of an infinity an infinity, of a NaN a NaN");

        const std::vector<m::value> exact_elements = {
            m::value::ratio(1, 2), m::value::ratio(1, 3), 1,
            2,                     0,                     m::value::ratio(1, 5),
            1,                     1,                     1 };
        const m::value exact  = d_tests_made(m::expr_matrix(3,
                                                            3,
                                                            exact_elements));
        const m::value right  = d_tests_made(m::expr_matrix(3,
                                                            1,
                                                            { 1, 2, 3 }));
        const m::value inv_a  = d_tests_made(m::expr_inv(exact));
        const m::value x      = d_tests_made(m::expr_solve(exact,
                                                           right));
        const m::value eye    = d_tests_made(m::expr_matrix(3,
                                                            3,
                                                            { 1, 0, 0,
                                                              0, 1, 0,
                                                              0, 0, 1 }));

        D_TESTS_EXPECT( (d_tests_same_values(d_tests_made(m::expr_det(exact)),
                                             d_tests_cofactors(
                                                 exact_elements,
                                                 3))) &&
                        (d_tests_same_values(inv_a * exact,
                                             eye)) &&
                        (d_tests_same_values(exact * inv_a,
                                             eye)) &&
                        (d_tests_same_values(exact * x,
                                             right)) &&
                        (d_tests_all_exact(inv_a)) &&
                        (d_tests_all_exact(x)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_trace(exact)),
                            m::value::ratio(3, 2))),
                        "an exact matrix's det, the test's own cofactors "
                        "exactly; inv and solve exact, A inv(A) = I and "
                        "A x = b; trace exact");

        const m::value singular = d_tests_made(m::expr_matrix(2,
                                                              2,
                                                              { 1, 2,
                                                                2, 4 }));
        const double   flat[4]  = { 1.0, 2.0, 2.0, 4.0 };
        const m::value dense    = d_tests_made(m::expr_matrix(
                                      2,
                                      2,
                                      d_tests_dense_values(flat,
                                                           4)));
        const m::value pair     = d_tests_made(m::expr_vector({ 1, 1 }));

        D_TESTS_EXPECT( (d_tests_same_values(
                            d_tests_made(m::expr_det(singular)),
                            m::value(0))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_inv(singular)),
                            pole)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_solve(singular,
                                                       pair)),
                            pole)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_inv(dense)),
                            pole)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_solve(dense,
                                                       pair)),
                            pole)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_det(dense)),
                            m::value(0.0))),
                        "a singular matrix's inverse and solution, exact "
                        "or dense, a pole, as an exact number over 0 is; "
                        "its determinant 0");

        const long double two_real = 2.0L;
        const m::value    boxed    = d_tests_made(
                                         m::expr_matrix(1,
                                                        1,
                                                        { d_tests_real_of(
                                                              &two_real) }));
        const m::value    beside   = d_tests_made(
                                         m::expr_matrix(2,
                                                        2,
                                                        { d_tests_real_of(
                                                              &two_real),
                                                          m::value(0.5),
                                                          1,
                                                          1 }));

        D_TESTS_EXPECT( (d_tests_same_values(m::internal::expr_matrix_det(
                                                 boxed),
                                             m::value())) &&
                        (m::internal::expr_matrix_det(beside).kind() !=
                             D_MATH_VALUE_DOUBLE) &&
                        (d_tests_same_values(m::internal::expr_matrix_inverse(
                                                 boxed),
                                             domain)) &&
                        (d_tests_same_values(m::internal::expr_matrix_solve(
                                                 boxed,
                                                 boxed),
                                             domain)),
                        "a boxed inexact element's algebra on the generic "
                        "path, by its own arithmetic, which the test's "
                        "kind gives as none: the determinant none, the "
                        "inverse and the solution a domain error; beside a "
                        "double, on the generic path still, not in double");

        dj_math_value   held    = m::value(2.0L).detach();

        held.flags |= D_MATH_VALUE_FLAG_INEXACT;

        const m::value flagged = d_tests_made(m::expr_matrix(
                                     2,
                                     2,
                                     { m::value::adopt(held),
                                       m::value(1.0L),
                                       m::value(1.0L),
                                       m::value(3.0L) }));
        const m::value column  = d_tests_made(m::expr_matrix(2,
                                                             1,
                                                             { 1, 2 }));
        const unsigned inexact = D_MATH_VALUE_FLAG_INEXACT;
        dj_math_value  held_b  = m::value(2.0L).detach();
        dj_math_value  held_r  = d_tests_real_of(&two_real).detach();

        held_b.flags |= D_MATH_VALUE_FLAG_INEXACT;
        held_r.flags |= D_MATH_VALUE_FLAG_INEXACT;

        const m::value right_f = d_tests_made(m::expr_matrix(
                                     2,
                                     1,
                                     { m::value::adopt(held_b),
                                       m::value(1.0L) }));
        const m::value real_f  = d_tests_made(m::expr_matrix(
                                     1,
                                     1,
                                     { m::value::adopt(held_r) }));

        D_TESTS_EXPECT( ((flagged.flags() & inexact) != 0u) &&
                        ((d_tests_made(m::expr_det(flagged)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_inv(flagged)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_trace(flagged)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_norm(flagged)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_eig(flagged)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_solve(flagged,
                                                     column)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_solve(d_tests_two_by_two(),
                                                     right_f)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_eig(real_f)).flags() &
                          inexact) != 0u) &&
                        (m::expr_is_matrix(d_tests_made(m::expr_solve(
                            d_tests_two_by_two(),
                            column)))) &&
                        ((d_tests_made(m::expr_solve(d_tests_two_by_two(),
                                                     column)).flags() &
                          inexact) == 0u),
                        "each result carries its operands' flags, a dense "
                        "matrix's of an element flagged inexact (G2), the "
                        "second's of solve, a boxed one's of eig");

        const m::value error_value(D_MATH_VALUE_ERROR_POLE);
        const m::value a = d_tests_two_by_two();
        bool           passed = true;
        const m::value answers[] = {
            m::internal::expr_matrix_det(error_value),
            m::internal::expr_matrix_trace(error_value),
            m::internal::expr_matrix_norm(error_value),
            m::internal::expr_matrix_eig(error_value),
            m::internal::expr_matrix_dot(error_value, m::value(true)),
            m::internal::expr_matrix_cross(a, error_value),
            m::internal::expr_matrix_solve(error_value, a) };
        const m::value refused[] = {
            m::internal::expr_matrix_det(m::value(true)),
            m::internal::expr_matrix_trace(m::value()),
            m::internal::expr_matrix_norm(m::value(false)),
            m::internal::expr_matrix_eig(m::value()),
            m::internal::expr_matrix_dot(a, m::value(true)),
            m::internal::expr_matrix_solve(m::value(true), a),
            m::internal::expr_matrix_solve(a, m::value(2)),
            m::internal::expr_matrix_dot(a, a),
            m::internal::expr_matrix_dot(a, m::value(2)),
            m::internal::expr_matrix_cross(m::value(1), m::value(2)) };

        for (std::size_t i = 0;
             i < (sizeof(answers) / sizeof(answers[0]));
             ++i)
        {
            passed = ( (passed) &&
                       (d_tests_same_values(answers[i],
                                            error_value)) );
        }

        for (std::size_t i = 0;
             i < (sizeof(refused) / sizeof(refused[0]));
             ++i)
        {
            passed = ( (passed) &&
                       (d_tests_same_values(refused[i],
                                            domain)) );
        }

        D_TESTS_EXPECT( passed,
                        "an error passes, the first; a boolean or none, a "
                        "matrix and a number where a function takes two "
                        "of one sort, a matrix dot a matrix and a number's "
                        "cross: a domain error");
        D_TESTS_EXPECT( (d_tests_holds_value(m::expr_det(m::value(true)),
                                             domain)) &&
                        (d_tests_holds_value(m::expr_inv(error_value),
                                             error_value)) &&
                        (d_tests_holds_value(m::expr_eig(m::value()),
                                             domain)) &&
                        (d_tests_holds_value(m::expr_dot(error_value,
                                                         a),
                                             error_value)) &&
                        (d_tests_holds_value(m::expr_cross(a,
                                                           m::value(true)),
                                             domain)) &&
                        (d_tests_holds_value(m::expr_solve(a,
                                                           error_value),
                                             error_value)),
                        "the entries: an operand's error, or a boolean or "
                        "none, the value's own answer, no bad_shape");

        return;
    }

    // d_tests_matrix_eigenvalues
    //   eig as a value: an exact matrix's eigenvalues exact where rational,
    // a repeated one as often as it is repeated, one found as a complex
    // pair's real part among them; irrational ones in long double beside
    // exact ones; a complex pair a domain error; a dense symmetric matrix's
    // by Jacobi, one of entries past the square of the type's range among
    // them, any other's by QR, each in its type, ascending; NaNs for a NaN.
    void
    d_tests_matrix_eigenvalues()
    {
        typedef m::internal::expr_matrix_storage storage;

        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( (d_tests_is_matrix(
                            d_tests_value_of("eig([2, 1; 1, 2])"),
                            1, 2,
                            { 1, 3 },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([1/2, 1/3; 1/3, 1/2])"),
                            1, 2,
                            { m::value::ratio(1, 6), m::value::ratio(5, 6) },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([2, 0, 0; 1, 3, 0; "
                                             "4, 5, 6])"),
                            1, 3,
                            { 2, 3, 6 },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([5])"),
                            1, 1,
                            { 5 },
                            storage::values)),
                        "an exact matrix's rational eigenvalues, exact, "
                        "ascending, a row");
        D_TESTS_EXPECT( (d_tests_is_matrix(
                            d_tests_value_of("eig([2, 1; 0, 2])"),
                            1, 2,
                            { 2, 2 },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([1, 1; -1, 3])"),
                            1, 2,
                            { 2, 2 },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([4, 1, 0; -1, 2, 0; "
                                             "0, 0, 3])"),
                            1, 3,
                            { 3, 3, 3 },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([0, 0; 0, 0])"),
                            1, 2,
                            { 0, 0 },
                            storage::values)),
                        "a repeated eigenvalue as often as it is repeated, "
                        "where it is found as a complex pair's real part "
                        "too");

        const m::value golden = d_tests_value_of("eig([2, 0, 0; 0, 1, 1; "
                                                 "0, 1, 0])");
        const std::vector<m::value> found = d_tests_elements_of(golden);

        D_TESTS_EXPECT( (found.size() == 3) &&
                        (found[0].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (found[1].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_same_values(found[2],
                                             m::value(2))) &&
                        (d_tests_near(found[0],
                                      -0.6180339887498948482L,
                                      1e-17L)) &&
                        (d_tests_near(found[1],
                                      1.6180339887498948482L,
                                      1e-17L)),
                        "irrational eigenvalues in long double, beside "
                        "an exact rational one, ascending");
        const m::value pair = d_tests_value_of("eig([1, 2; -3, 1])");
        m::complex<long double> low;
        m::complex<long double> high;

        static_cast<void>(m::expr_complex_to(d_tests_made(
                                                 m::expr_element(pair, 0, 0)),
                                             low));
        static_cast<void>(m::expr_complex_to(d_tests_made(
                                                 m::expr_element(pair, 0, 1)),
                                             high));

        D_TESTS_EXPECT( (d_tests_same_values(
                            d_tests_value_of("eig([0, -1; 1, 0])"),
                            d_tests_value_of("[-im, im]"))) &&
                        (d_tests_value_of("eig([0, -1; 1, 0])").kind() !=
                             D_MATH_VALUE_ERROR) &&
                        (d_tests_all_exact(
                            d_tests_value_of("eig([0, -1; 1, 0])"))) &&
                        (d_tests_shape(pair) == m::expr_shape(1, 2)) &&
                        (m::expr_is_complex(d_tests_made(
                             m::expr_element(pair, 0, 0)))) &&
                        (low.real() == high.real()) &&
                        (low.imag() == -high.imag()) &&
                        (std::fabs(high.imag() -
                                   2.4494897427831780982L) < 1e-17L) &&
                        (std::fabs(low.real() - 1.0L) < 1e-17L) &&
                        (d_tests_same_values(
                            m::internal::expr_matrix_eig(
                                d_tests_made(m::expr_matrix(
                                    2,
                                    2,
                                    { 0.0, -1.0, 1.0, 0.0 }))),
                            d_tests_made(m::expr_matrix(
                                1,
                                2,
                                { m::expr_complex_of(
                                      m::complex<double>(0.0, -1.0)),
                                  m::expr_complex_of(
                                      m::complex<double>(0.0, 1.0)) })))),
                        "a complex pair: of an exact matrix exact where it "
                        "is a Gaussian rational, -im and im, and a long "
                        "double pair otherwise, exactly conjugate, the "
                        "negative first; of a dense one its type's (S258, "
                        "S273, S352)");

        const m::value symmetric = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 2.0, 1.0, 1.0, 2.0 })));
        const m::value huge      = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 1e307, 1e307,
                                             1e307, 1e307 })));
        const m::value floats    = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 2.0f, 1.0f, 1.0f, 2.0f })));
        const m::value general   = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 1.0, 2.0, 3.0, 4.0 })));
        const m::value repeated  = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           4,
                                           4,
                                           { 10.0, 8.0, 8.0, 4.0,
                                             8.0, 10.0, 10.0, 8.0,
                                             8.0, 10.0, 10.0, 8.0,
                                             4.0, 8.0, 8.0, 8.0 })));

        D_TESTS_EXPECT( (d_tests_stored_as(symmetric,
                                           storage::doubles)) &&
                        (d_tests_element_near(symmetric,
                                              0,
                                              1.0L,
                                              1e-15L)) &&
                        (d_tests_element_near(symmetric,
                                              1,
                                              3.0L,
                                              1e-15L)) &&
                        (d_tests_stored_as(huge,
                                           storage::doubles)) &&
                        (d_tests_element_near(huge,
                                              0,
                                              0.0L,
                                              1e292L)) &&
                        (d_tests_element_near(huge,
                                              1,
                                              2e307L,
                                              2e292L)) &&
                        (d_tests_stored_as(floats,
                                           storage::floats)) &&
                        (d_tests_element_near(floats,
                                              1,
                                              3.0L,
                                              1e-6L)) &&
                        (d_tests_stored_as(repeated,
                                           storage::doubles)) &&
                        (d_tests_element_near(repeated,
                                              0,
                                              0.0L,
                                              1e-13L)) &&
                        (d_tests_element_near(repeated,
                                              1,
                                              0.0L,
                                              1e-13L)) &&
                        (d_tests_element_near(repeated,
                                              2,
                                              5.1075560105501954925L,
                                              1e-13L)) &&
                        (d_tests_element_near(repeated,
                                              3,
                                              32.89244398944980452L,
                                              1e-13L)),
                        "a dense symmetric matrix's by Jacobi, in its "
                        "type, scaled by a power of two, so that entries "
                        "whose squares pass the type's range are found; "
                        "a repeated eigenvalue, 0 twice, which Hessenberg "
                        "QR finds as a complex pair, real");
        D_TESTS_EXPECT( (d_tests_stored_as(general,
                                           storage::doubles)) &&
                        (d_tests_element_near(general,
                                              0,
                                              -0.3722813232690143L,
                                              1e-14L)) &&
                        (d_tests_element_near(general,
                                              1,
                                              5.3722813232690143L,
                                              1e-14L)),
                        "any other dense matrix's by Hessenberg QR, in "
                        "its type, ascending");

        const m::value wide_eig  = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { d_tests_wide_of(
                                                 "100000000000000000000"),
                                             0,
                                             0,
                                             1 })));
        const m::value past      = d_tests_value_of(
                                       "eig([1099511627776, 1; 1, "
                                       "1099511627776])");
        const m::value lcm       = d_tests_value_of(
                                       "eig([0, 1 / 4611686018427387904; "
                                       "0, 1 / 3])");
        const m::value far       = d_tests_value_of(
                                       "eig([9000000000000000000])");
        const std::vector<m::value> wide_values =
            d_tests_elements_of(wide_eig);
        const std::vector<m::value> past_values = d_tests_elements_of(past);
        const std::vector<m::value> lcm_values  = d_tests_elements_of(lcm);
        const std::vector<m::value> far_values  = d_tests_elements_of(far);

        D_TESTS_EXPECT( (wide_values.size() == 2) &&
                        (wide_values[0].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_near(wide_values[1],
                                      1e20L,
                                      1e5L)) &&
                        (past_values.size() == 2) &&
                        (past_values[1].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_near(past_values[1],
                                      1099511627777.0L,
                                      1e-6L)) &&
                        (lcm_values.size() == 2) &&
                        (lcm_values[1].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_near(lcm_values[1],
                                      1.0L / 3.0L,
                                      1e-18L)) &&
                        (far_values.size() == 1) &&
                        (far_values[0].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_near(far_values[0],
                                      9e18L,
                                      1.0L)),
                        "an exact matrix's eigenvalues in long double alone "
                        "where none can be made exact: of a wide integer, "
                        "a characteristic polynomial past the exact "
                        "integers, denominators whose multiple passes them, "
                        "or an eigenvalue whose multiple does");

        const m::value nan = m::internal::expr_matrix_eig(
                                 d_tests_made(m::expr_matrix(
                                     2,
                                     2,
                                     { std::numeric_limits<double>::
                                           quiet_NaN(),
                                       0.0,
                                       0.0,
                                       1.0 })));
        const std::vector<m::value> nans = d_tests_elements_of(nan);
        double                      first = 0.0;
        double                      second = 0.0;

        D_TESTS_EXPECT( (nans.size() == 2) &&
                        (nans[0].to(first)) &&
                        (nans[1].to(second)) &&
                        (first != first) &&
                        (second != second) &&
                        (d_tests_stored_as(nan,
                                           storage::doubles)),
                        "a NaN or an infinity in a dense matrix: NaNs, "
                        "in its type");

        return;
    }

    // d_tests_matrix_negative_powers
    //   A ^ (-n), the inverse's power: exact; a singular matrix's a pole;
    // dense, the C core's inverse then its power; and every dense power the
    // C core's d_mat_power, bit for bit.
    void
    d_tests_matrix_negative_powers()
    {
        typedef m::internal::expr_matrix_storage storage;

        const m::value a = d_tests_two_by_two();

        D_TESTS_EXPECT( (d_tests_same_values(
                            m::internal::expr_matrix_power(a,
                                                           m::value(-1)),
                            d_tests_made(m::expr_inv(a)))) &&
                        (d_tests_is_matrix(
                            m::internal::expr_matrix_power(a,
                                                           m::value(-2)),
                            2, 2,
                            { m::value::ratio(11, 2),
                              m::value::ratio(-5, 2),
                              m::value::ratio(-15, 4),
                              m::value::ratio(7, 4) },
                            storage::values)) &&
                        (d_tests_valued("[1, 1; 1, 0] ^ (-1)",
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            { 0, 1, 1, -1 })))) &&
                        (d_tests_valued("pow([1, 2; 3, 4], -1) = "
                                        "inv([1, 2; 3, 4])",
                                        m::value(true))),
                        "A ^ (-n), the inverse's power, exact, pow(A, -n) "
                        "the same (S259)");

        const m::value pole(D_MATH_VALUE_ERROR_POLE);
        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( (d_tests_valued("[1, 2; 2, 4] ^ (-1)",
                                        pole)) &&
                        (d_tests_valued("[1, 2; 2, 4] ^ (-3)",
                                        pole)) &&
                        (d_tests_valued("[1, 2, 3] ^ (-1)",
                                        domain)),
                        "a singular matrix's negative power a pole; a "
                        "matrix not square's a domain error");

        const double   entries[4] = { 1.0, 2.0, 3.0, 4.0 };
        double         inverse[4] = {};
        double         scratch[4] = {};
        double         squared[4] = {};
        const m::value dense      = d_tests_made(m::expr_matrix(
                                        2,
                                        2,
                                        d_tests_dense_values(entries,
                                                             4)));

        static_cast<void>(d_mat_inverse(inverse,
                                        scratch,
                                        entries,
                                        2));
        d_mat_power(squared,
                    scratch,
                    inverse,
                    2,
                    2);

        D_TESTS_EXPECT( (d_tests_is_matrix(
                            m::internal::expr_matrix_power(dense,
                                                           m::value(-2)),
                            2, 2,
                            d_tests_dense_values(squared,
                                                 4),
                            storage::doubles)) &&
                        (d_tests_is_matrix(
                            m::internal::expr_matrix_power(a,
                                                           m::value(-2.0)),
                            2, 2,
                            d_tests_dense_values(squared,
                                                 4),
                            storage::doubles)),
                        "a dense matrix's negative power, the C core's "
                        "inverse then its power; an exact one to a "
                        "floating-point -n, in its type");

        d_tests_rng rng(20261027u);
        double      raw[16] = {};
        bool        same    = true;

        for (std::size_t i = 0; i < 16; ++i)
        {
            raw[i] = ( static_cast<double>(rng.below(1000u)) / 997.0 ) - 0.5;
        }

        const m::value random = d_tests_made(m::expr_matrix(
                                    4,
                                    4,
                                    d_tests_dense_values(raw,
                                                         16)));

        // each power to 40, as the C core makes it
        for (std::size_t k = 0; k <= 40; ++k)
        {
            double out[16]  = {};
            double work[16] = {};

            d_mat_power(out,
                        work,
                        raw,
                        4,
                        k);

            same = ( (same) &&
                     (d_tests_is_matrix(
                         m::internal::expr_matrix_power(
                             random,
                             m::value(static_cast<long long>(k))),
                         4, 4,
                         d_tests_dense_values(out,
                                              16),
                         storage::doubles)) );
        }

        const double   infinite[4] = { std::numeric_limits<double>::
                                           infinity(),
                                       1.0,
                                       0.0,
                                       1.0 };
        const m::value with_inf    = d_tests_made(m::expr_matrix(
                                         2,
                                         2,
                                         d_tests_dense_values(infinite,
                                                              4)));

        D_TESTS_EXPECT( (same) &&
                        (d_tests_same_values(
                            m::internal::expr_matrix_power(with_inf,
                                                           m::value(1)),
                            with_inf)),
                        "every dense power the C core's d_mat_power, bit "
                        "for bit, each to 40, as both square from the "
                        "exponent's highest bit (S262); A ^ 1 A itself");

        return;
    }

    // d_tests_empty_matrix
    //   the empty matrix (S260): made, read and written; its arithmetic and
    // its functions, each by the rules of shapes; a block that is skipped.
    void
    d_tests_empty_matrix()
    {
        const m::value empty = d_tests_made(m::expr_matrix(0,
                                                           0,
                                                           {}));
        const m::value one   = d_tests_made(m::expr_vector({ 1 }));

        D_TESTS_EXPECT( (m::expr_is_matrix(empty)) &&
                        (d_tests_shape(empty) == m::expr_shape()) &&
                        (d_tests_stored_as(
                            empty,
                            m::internal::expr_matrix_storage::values)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_vector({})),
                            empty)) &&
                        (d_tests_kind_text(empty, 64) == "[]") &&
                        (m::expr_render(m::expr_number(empty)) == "[]") &&
                        (empty == empty) &&
                        (empty != one),
                        "the empty matrix: expr_matrix of 0 by 0, and "
                        "expr_vector of none, of values, its text [], "
                        "equal to itself alone");

        const m::expr_term parsed = d_tests_parsed("[]");

        // that [] parsed, asked first: where it does not, parsed is a term
        // made by default, which has no layer to read
        D_TESTS_EXPECT( (m::expr_parse("[]").is_ok()) &&
                        (m::expr_is_application(parsed)) &&
                        (m::expr_op_of(parsed).kind ==
                             m::expr_op_kind::vector) &&
                        (m::expr_operands_of(parsed).empty()) &&
                        (m::expr_render(parsed) == "[]") &&
                        (d_tests_valued("[]", empty)) &&
                        (d_tests_valued("[[]]", empty)) &&
                        (d_tests_is_error(m::expr_evaluate<double>(parsed),
                                          m::expr_status::bad_shape,
                                          m::expr_no_offset,
                                          "expr_evaluate<Type>: a vector or "
                                          "a matrix has no value in a "
                                          "type")),
                        "[], the empty literal: the term vector of no "
                        "element, written back as it is, the empty matrix "
                        "as a value and no value in a type");

        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);
        bool           kept    = true;
        bool           refused = true;
        const char*    empties[] = { "[] + []", "[] - []", "-[]", "2 * []",
                                     "[] / 2", "[] * []", "[]'", "[] ^ 0",
                                     "[] ^ 3", "[] ^ (-1)", "inv([])",
                                     "eig([])", "solve([], [])",
                                     "[[], []]", "[[]; []]" };
        const char*    refusals[] = { "[] + [1]", "[1, 2] * []", "[] * [1]",
                                      "[][1]", "cross([], [])",
                                      "dot([], [1])", "solve([], [1])" };

        for (std::size_t i = 0; i < (sizeof(empties) / sizeof(empties[0]));
             ++i)
        {
            kept = ( (kept) &&
                     (d_tests_valued(empties[i],
                                     empty)) );
        }

        for (std::size_t i = 0;
             i < (sizeof(refusals) / sizeof(refusals[0]));
             ++i)
        {
            refused = ( (refused) &&
                        (d_tests_valued(refusals[i],
                                        domain)) );
        }

        D_TESTS_EXPECT( kept,
                        "its sums, negation, scalar products, product, "
                        "transpose, powers, inverse, eigenvalues and "
                        "solution: the empty matrix");
        D_TESTS_EXPECT( (refused) &&
                        (d_tests_valued("det([])", m::value(1))) &&
                        (d_tests_valued("trace([])", m::value(0))) &&
                        (d_tests_valued("norm([])", m::value(0))) &&
                        (d_tests_valued("dot([], [])", m::value(0))) &&
                        (d_tests_valued("[] = []", m::value(true))) &&
                        (d_tests_valued("[] != [1]", m::value(true))),
                        "its determinant 1, trace, norm and dot product 0; "
                        "a shape of other matrices, an index, and cross of "
                        "it: a domain error");

        return;
    }

    // d_tests_blocks
    //   literals of blocks (S261): read with no shape the parser cannot
    // know; each block beside or above another as a value, its storage and
    // flags; blocks that do not fit, a domain error; written back; never
    // square by structure; no value in a type.
    void
    d_tests_blocks()
    {
        const m::value       a        = d_tests_two_by_two();
        const m::expr_bindings bindings = m::at("A", a).at("x", 1);
        const m::value       domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( (m::expr_parse("[A, b]").is_ok()) &&
                        (m::expr_parse("[A; B]").is_ok()) &&
                        (m::expr_parse("[[1, 2], 3; 4, 5, 6]").is_ok()) &&
                        (m::expr_parse("[x, 2; 3]").is_ok()) &&
                        (m::expr_parse("[1, 2; 3, x]").is_ok()) &&
                        (m::expr_parse("[pi, 1; 2]").is_ok()) &&
                        (m::expr_parse("[2 * x, 2; 3]").is_ok()) &&
                        (m::expr_parse("[[1, 2; 3, 4]; 5, 6]").is_ok()) &&
                        (m::expr_parse("[1, 2; A]").is_ok()) &&
                        (d_tests_is_error(m::expr_parse("[1, 2; 3]"),
                                          m::expr_status::bad_shape,
                                          8,
                                          "a row of 1 element, where the "
                                          "first row has 2 elements")) &&
                        (d_tests_is_error(m::expr_parse("[1, 2; x, 1; 3]"),
                                          m::expr_status::bad_shape,
                                          14,
                                          "a row of 1 element, where the "
                                          "first row has 2 elements")) &&
                        (d_tests_is_error(m::expr_parse("[1 + 2, 3!; "
                                                        "-4]"),
                                          m::expr_status::bad_shape,
                                          14,
                                          "a row of 1 element, where the "
                                          "first row has 2 elements")),
                        "a literal whose rows may hold a matrix, of a "
                        "variable, a constant, a literal or an operation "
                        "of one, read whatever its rows' numbers of "
                        "elements; rows of numbers alone of other numbers, "
                        "bad_shape where they end, each row judged anew");
        D_TESTS_EXPECT( (d_tests_valued("[A, A]",
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            4,
                                            { 1, 2, 1, 2, 3, 4, 3, 4 })),
                                        bindings)) &&
                        (d_tests_valued("[A; A]",
                                        d_tests_made(m::expr_matrix(
                                            4,
                                            2,
                                            { 1, 2, 3, 4, 1, 2, 3, 4 })),
                                        bindings)) &&
                        (d_tests_valued("[A, [5; 6]]",
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            3,
                                            { 1, 2, 5, 3, 4, 6 })),
                                        bindings)) &&
                        (d_tests_valued("[[1, 2], 3; 4, 5, 6]",
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            3,
                                            { 1, 2, 3, 4, 5, 6 })))) &&
                        (d_tests_valued("[[1; 2], [3; 4]]",
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            { 1, 3, 2, 4 })))) &&
                        (d_tests_valued("[A, []; [], A]",
                                        d_tests_made(m::expr_matrix(
                                            4,
                                            2,
                                            { 1, 2, 3, 4, 1, 2, 3, 4 })),
                                        bindings)) &&
                        (d_tests_valued("[[1, 2; 3, 4]; 5, 6]",
                                        d_tests_made(m::expr_matrix(
                                            3,
                                            2,
                                            { 1, 2, 3, 4, 5, 6 })))) &&
                        (d_tests_valued("[1, 2; A]",
                                        d_tests_made(m::expr_matrix(
                                            3,
                                            2,
                                            { 1, 2, 1, 2, 3, 4 })),
                                        bindings)) &&
                        (d_tests_valued("[A; []]", a, bindings)) &&
                        (d_tests_valued("[[]; A]", a, bindings)),
                        "blocks as values: a row's beside one another, a "
                        "matrix's rows one above another, a number a block "
                        "of 1 by 1, the empty matrix skipped, beside or "
                        "above");
        D_TESTS_EXPECT( (d_tests_valued("[A, 1]", domain, bindings)) &&
                        (d_tests_valued("[A; 1, 2, 3]", domain, bindings)) &&
                        (d_tests_valued("[x, 2; 3]", domain, bindings)) &&
                        (d_tests_valued("[[1; 2], 2]", domain)) &&
                        (d_tests_valued("[[1, 2]; [3, 4, 5]]", domain)) &&
                        (d_tests_valued("[[1, 2], [3, 4; 5, 6; 7, 8], "
                                        "[9; 10]]",
                                        domain)) &&
                        (d_tests_valued("[[1, 2, 3]; [4]; [5, 6]]",
                                        domain)) &&
                        (d_tests_valued("[A, true]", domain, bindings)) &&
                        (d_tests_valued("[A, 1 / 0]",
                                        m::value(D_MATH_VALUE_ERROR_POLE),
                                        bindings)),
                        "blocks that do not fit, of other rows beside one "
                        "another or other columns one above another, even "
                        "where their elements would fill a shape, and a "
                        "boolean among them: a domain error; an error "
                        "passes, the first");

        typedef m::internal::expr_matrix_storage storage;

        const m::value halves = d_tests_made(m::expr_vector({ 0.5f }));
        const m::value quarter = d_tests_made(m::expr_vector({ 0.25 }));
        const m::value fib100  = m::internal::expr_matrix_power(
                                     d_tests_made(m::expr_matrix(2,
                                                                 2,
                                                                 { 1, 1,
                                                                   1, 0 })),
                                     m::value(100));
        const m::value dense   = d_tests_value_of("[F, Q]",
                                                  m::at("F", halves)
                                                      .at("Q", quarter));
        const m::value mixed   = d_tests_value_of("[Q, 1]",
                                                  m::at("Q", quarter));
        const m::value flagged = d_tests_value_of("[G; G]",
                                                  m::at("G", fib100));

        D_TESTS_EXPECT( (d_tests_is_matrix(dense,
                                           1, 2,
                                           { 0.5, 0.25 },
                                           storage::doubles)) &&
                        (d_tests_stored_as(mixed,
                                           storage::values)) &&
                        ((flagged.flags() & D_MATH_VALUE_FLAG_INEXACT) !=
                             0u) &&
                        (d_tests_shape(flagged) == m::expr_shape(4, 2)),
                        "a matrix of blocks stored as S239 stores any, "
                        "dense where each element is floating, values "
                        "otherwise; its blocks' flags carried");
        D_TESTS_EXPECT( (d_tests_parses("[[1, 2], 3; 4, 5, 6]",
                                        d_tests_parsed("[[1, 2], 3; 4, 5, "
                                                       "6]"))) &&
                        (m::expr_render(d_tests_parsed("[A,[5;6]]")) ==
                             "[A, [5; 6]]") &&
                        (m::expr_render(m::expr_simplify(
                             d_tests_parsed("[[1, 2]] ^ 1"))) ==
                             "[[1, 2]] ^ 1") &&
                        (m::expr_render(m::expr_simplify(
                             d_tests_parsed("[[1; 2], [3; 4]] ^ 1"))) ==
                             "[[1; 2], [3; 4]] ^ 1") &&
                        (d_tests_valued("[[1, 2]] ^ 1", domain)),
                        "a literal of blocks written as it is read, read "
                        "back as itself; never square by its structure, "
                        "so that [[1, 2]] ^ 1, a domain error, stays");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_parsed("[A, b]"),
                                             m::at("A", 1).at("b", 2)),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_evaluate<Type>: a vector or a "
                                         "matrix has no value in a type"),
                        "in a type a literal of blocks has no value, as "
                        "any literal");

        return;
    }

    // d_tests_algebra_typed
    //   whether linear algebra's functions in Type are a scalar's at x = 2:
    // det, trace and eig 2, inv 1/2, norm(-x) 2, dot(x, 3) 6, solve(x, 6)
    // 3, and cross a NaN.
    template<typename Type>
    bool
    d_tests_algebra_typed()
    {
        const m::expr_bindings at = m::at("x", 2);

        return ( (d_tests_typed<Type>("det(x)", Type(2), at)) &&
                 (d_tests_typed<Type>("inv(x)", Type(0.5), at)) &&
                 (d_tests_typed<Type>("trace(x)", Type(2), at)) &&
                 (d_tests_typed<Type>("norm(-x)", Type(2), at)) &&
                 (d_tests_typed<Type>("eig(x)", Type(2), at)) &&
                 (d_tests_typed<Type>("dot(x, 3)", Type(6), at)) &&
                 (d_tests_typed<Type>("solve(x, 6)", Type(3), at)) &&
                 (d_tests_typed<Type>(
                     "cross(x, x)",
                     std::numeric_limits<Type>::quiet_NaN(),
                     at)) );
    }

    // d_tests_fold_algebra
    //   linear algebra's functions in the language: as values, of matrices
    // bound and of literals, and of numbers as scalars; in a type, a
    // scalar's; what each refuses; a call of a number of arguments it does
    // not take, wrong_arity.
    void
    d_tests_fold_algebra()
    {
        const m::value         a        = d_tests_two_by_two();
        const m::value         b        = d_tests_made(m::expr_matrix(
                                              2,
                                              3,
                                              { 1, 2, 3, 4, 5, 6 }));
        const m::value         v        = d_tests_made(m::expr_vector(
                                              { 5, 6, 7 }));
        const m::value         c        = d_tests_made(m::expr_matrix(
                                              2,
                                              3,
                                              { 0.5, 1.0, 1.5,
                                                2.0, 2.5, 3.0 }));
        const m::expr_bindings bindings = m::at("A", a).at("B", b)
                                               .at("C", c).at("v", v)
                                               .at("x", 2);

        D_TESTS_EXPECT( (d_tests_valued("det(A)", m::value(-2), bindings)) &&
                        (d_tests_valued("inv(A) * A = A ^ 0",
                                        m::value(true),
                                        bindings)) &&
                        (d_tests_valued("trace(A)", m::value(5), bindings)) &&
                        (d_tests_valued("norm([3, 4])", m::value(5))) &&
                        (d_tests_valued("dot(v, v)", m::value(110),
                                        bindings)) &&
                        (d_tests_valued("cross(v, [1, 0, 0])",
                                        d_tests_made(m::expr_vector(
                                            { 0, 7, -6 })),
                                        bindings)) &&
                        (d_tests_valued("solve(A, [1, 2])",
                                        d_tests_made(m::expr_vector(
                                            { 0, m::value::ratio(1, 2) })),
                                        bindings)) &&
                        (d_tests_valued("A * solve(A, B) = B",
                                        m::value(true),
                                        bindings)) &&
                        (d_tests_valued("eig([2, 1; 1, 2])",
                                        d_tests_made(m::expr_vector(
                                            { 1, 3 })))),
                        "as values: det, inv, trace, norm, dot, cross, "
                        "solve and eig of matrices bound and of literals, "
                        "exact");
        D_TESTS_EXPECT( (d_tests_valued("det(x)", m::value(2), bindings)) &&
                        (d_tests_valued("inv(x)",
                                        m::value::ratio(1, 2),
                                        bindings)) &&
                        (d_tests_valued("inv(0)",
                                        m::value(D_MATH_VALUE_ERROR_POLE))) &&
                        (d_tests_valued("trace(x)", m::value(2),
                                        bindings)) &&
                        (d_tests_valued("norm(-3)", m::value(3))) &&
                        (d_tests_valued("eig(x)", m::value(2), bindings)) &&
                        (d_tests_valued("dot(2, 3)", m::value(6))) &&
                        (d_tests_valued("solve(2, 6)", m::value(3))) &&
                        (d_tests_valued("solve(2, [4, 6])",
                                        d_tests_made(m::expr_vector(
                                            { 2, 3 })))),
                        "of numbers, scalars, as in Julia: det, trace and "
                        "eig itself, inv the reciprocal, norm the "
                        "magnitude, dot the product, solve(a, B) B / a");
        D_TESTS_EXPECT( (d_tests_algebra_typed<float>()) &&
                        (d_tests_algebra_typed<double>()) &&
                        (d_tests_algebra_typed<long double>()) &&
                        (d_tests_typed<double>(
                            "det(A)",
                            std::numeric_limits<double>::quiet_NaN(),
                            m::at("A", a))),
                        "in a type, a scalar's: det, trace and eig x, inv "
                        "1 / x, norm |x|, dot x y, solve b / a; cross a "
                        "NaN; of a variable bound to a matrix, a NaN");

        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);
        bool           refused = true;
        const char*    texts[] = { "det(B)", "inv(B)", "trace(v)", "eig(B)",
                                   "dot(v, A)", "dot(v, [1, 2])",
                                   "dot([1, 2], v)",
                                   "cross(v, [1, 2])", "cross(1, 2)",
                                   "solve(B, v)", "solve(A, 1)",
                                   "solve(A, v)", "det(C)", "inv(C)",
                                   "trace(C)", "eig(C)", "det(true)",
                                   "inv(true)",
                                   "norm(1 < 2)", "sqrt(det(A) * A)" };

        for (std::size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); ++i)
        {
            refused = ( (refused) &&
                        (d_tests_valued(texts[i],
                                        domain,
                                        bindings)) );
        }

        D_TESTS_EXPECT( (refused) &&
                        (d_tests_valued("det([1, 1 / 0])",
                                        m::value(D_MATH_VALUE_ERROR_POLE))),
                        "a shape a function does not take, a boolean, and "
                        "a matrix in any other function: a domain error; "
                        "an argument's error passes");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate(
                                             d_tests_parsed("det(A, A)"),
                                             bindings,
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "det takes 1 argument, not 2") &&
                        d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_parsed("dot(x)"),
                                             m::at("x", 2)),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "dot takes 2 arguments, not 1"),
                        "a call of a number of arguments the function does "
                        "not take: wrong_arity, in both folds");

        return;
    }

    // d_tests_random_square
    //   a random exact square matrix of _n rows: each element a whole number
    // from -3 to 3, a third of them 0, one in five a half of one; upper
    // triangular where _upper.
    std::vector<m::value>
    d_tests_random_square(
        d_tests_rng& _rng,
        std::size_t  _n,
        bool         _upper
    )
    {
        std::vector<m::value> elements;

        for (std::size_t r = 0; r < _n; ++r)
        {
            for (std::size_t c = 0; c < _n; ++c)
            {
                const unsigned zero  = _rng.below(3u);
                const long long whole =
                    static_cast<long long>(_rng.below(7u)) - 3;
                const bool      half  = (_rng.below(5u) == 0);

                elements.push_back( ( ( (_upper) && (c < r) ) ||
                                      (zero == 0) )
                                        ? m::value(0)
                                        : ( half ? m::value::ratio(whole, 2)
                                                 : m::value(whole) ) );
            }
        }

        return elements;
    }

    // d_tests_algebra_random
    //   linear algebra's functions on 1,000 random exact matrices of 1 to 4
    // rows, against the test's own arithmetic: det the cofactor expansion,
    // exactly; inv and solve exact, A inv(A) = I and A x = b, a pole where
    // det is 0; trace the diagonal's sum; each exact eigenvalue a root of
    // det(A - x I), exactly, and a real spectrum's sum the trace; and a
    // random upper triangular matrix's eigenvalues its diagonal, exact and
    // ascending (S263).
    void
    d_tests_algebra_random()
    {
        d_tests_rng       rng(20261028u);
        const std::size_t count      = 1000;
        std::size_t       determined = 0;
        std::size_t       inverted   = 0;
        std::size_t       solved     = 0;
        std::size_t       singular   = 0;
        std::size_t       traced     = 0;
        std::size_t       rooted     = 0;
        std::size_t       complex_spectra = 0;
        std::size_t       triangular = 0;

        for (std::size_t i = 0; i < count; ++i)
        {
            const std::size_t           n = 1 + rng.below(4u);
            const std::vector<m::value> elements =
                d_tests_random_square(rng,
                                      n,
                                      false);
            const m::value              a = d_tests_made(
                                                m::expr_matrix(n,
                                                               n,
                                                               elements));
            std::vector<m::value>       right;
            std::vector<m::value>       identity;
            m::value                    diagonal(0);

            for (std::size_t r = 0; r < n; ++r)
            {
                right.push_back(m::value(static_cast<long long>(r) + 1));
                diagonal = diagonal + elements[(r * n) + r];

                for (std::size_t c = 0; c < n; ++c)
                {
                    identity.push_back(m::value( (r == c) ? 1 : 0 ));
                }
            }

            const m::value b   = d_tests_made(m::expr_matrix(n,
                                                             1,
                                                             right));
            const m::value eye = d_tests_made(m::expr_matrix(n,
                                                             n,
                                                             identity));
            const m::value det = d_tests_cofactors(elements,
                                                   n);
            const m::value inv = d_tests_made(m::expr_inv(a));
            const m::value x   = d_tests_made(m::expr_solve(a,
                                                            b));

            determined += ( d_tests_same_values(d_tests_made(m::expr_det(a)),
                                                det) ? 1 : 0 );
            traced     += ( d_tests_same_values(
                                d_tests_made(m::expr_trace(a)),
                                diagonal) ? 1 : 0 );

            // invertible: exact; singular: a pole
            if (det == m::value(0))
            {
                const m::value pole(D_MATH_VALUE_ERROR_POLE);

                singular += ( ( (d_tests_same_values(inv,
                                                     pole)) &&
                                (d_tests_same_values(x,
                                                     pole)) ) ? 1 : 0 );
            }
            else
            {
                inverted += ( ( (d_tests_same_values(inv * a,
                                                     eye)) &&
                                (d_tests_all_exact(inv)) ) ? 1 : 0 );
                solved   += ( d_tests_same_values(a * x,
                                                  b) ? 1 : 0 );
            }

            const m::value              eig    = d_tests_made(
                                                     m::expr_eig(a));
            const std::vector<m::value> values = d_tests_elements_of(eig);
            bool                        roots  = true;
            long double                 sum    = 0.0L;
            long double                 turn   = 0.0L;
            long double                 trace  = 0.0L;
            bool                        pairs  = false;

            // each exact eigenvalue a root; the spectrum's sum the trace;
            // by real part, then by imaginary part (S352)
            for (std::size_t k = 0; k < values.size(); ++k)
            {
                m::complex<long double> part;
                std::vector<m::value>   shifted = elements;

                for (std::size_t r = 0; r < n; ++r)
                {
                    shifted[(r * n) + r] = shifted[(r * n) + r] - values[k];
                }

                roots = ( (roots) &&
                          ( (!values[k].is_exact()) ||
                            (d_tests_cofactors(shifted,
                                               n) == m::value(0)) ) &&
                          ( (k == 0) ||
                            (!m::internal::expr_eigenvalue_before(
                                 values[k],
                                 values[k - 1])) ) &&
                          (m::expr_complex_to(values[k],
                                              part)) );
                pairs = ( (pairs) ||
                          (m::expr_is_complex(values[k])) );
                sum  += part.real();
                turn += part.imag();
            }

            static_cast<void>(diagonal.to(trace));

            complex_spectra += ( pairs ? 1 : 0 );
            rooted += ( ( (eig.kind() != D_MATH_VALUE_ERROR) &&
                          (values.size() == n) &&
                          (roots) &&
                          (std::fabs(sum - trace) <=
                               (1e-12L * (1.0L + std::fabs(trace)))) &&
                          (std::fabs(turn) <=
                               (1e-12L * (1.0L + std::fabs(trace)))) )
                            ? 1 : 0 );

            // upper triangular: the diagonal, exact and ascending
            const std::vector<m::value> upper = d_tests_random_square(rng,
                                                                      n,
                                                                      true);
            std::vector<m::value>       sorted;

            for (std::size_t r = 0; r < n; ++r)
            {
                sorted.push_back(upper[(r * n) + r]);
            }

            std::sort(sorted.begin(),
                      sorted.end(),
                      [](const m::value& _x,
                         const m::value& _y) -> bool
                      {
                          return (_x < _y);
                      });

            triangular += ( d_tests_same_values(
                                d_tests_made(m::expr_eig(d_tests_made(
                                    m::expr_matrix(n,
                                                   n,
                                                   upper)))),
                                d_tests_made(m::expr_vector(sorted)))
                                ? 1 : 0 );
        }

        D_TESTS_EXPECT( (determined == count) &&
                        (traced == count),
                        "1,000 random exact matrices: det the test's own "
                        "cofactor expansion, and trace the diagonal's sum, "
                        "exactly");
        D_TESTS_EXPECT( (inverted + singular == count) &&
                        (singular > (count / 20)) &&
                        (solved == inverted),
                        "inv and solve exact, A inv(A) = I and A x = b, "
                        "each invertible matrix's; a singular one's a pole, "
                        "some of them");
        D_TESTS_EXPECT( (rooted == count) &&
                        (complex_spectra > (count / 20)),
                        "each exact eigenvalue a root of det(A - x I), "
                        "exactly, by real part, then by imaginary part; the "
                        "spectrum's sum the trace; complex pairs among them "
                        "(S352)");
        D_TESTS_EXPECT( (triangular == count),
                        "a random upper triangular matrix's eigenvalues "
                        "its diagonal, exact and ascending");

        return;
    }

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


    //==========================================================================
    // 15. THE MODULE
    //==========================================================================

    // d_tests_module
    //   every part of the module, from its floor.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_MODULE_READ == 1),
                        "the module's seventeen headers read");

        d_tests_errors();
        d_tests_tokens();
        d_tests_tokenize();
        d_tests_operators();
        d_tests_table();
        d_tests_atoms();
        d_tests_terms();
        d_tests_tables();
        d_tests_standard_functions();
        d_tests_standard_constants();
        d_tests_bindings();
        d_tests_fold_typed();
        d_tests_fold_value();
        d_tests_fold_passing();
        d_tests_fold_functions();
        d_tests_fold_functions_value();
        d_tests_fold_logic();
        d_tests_fold_errors();
        d_tests_parse_numbers();
        d_tests_parse_grammar();
        d_tests_parse_implied();
        d_tests_parse_calls();
        d_tests_parse_bars();
        d_tests_parse_logic();
        d_tests_parse_piecewise();
        d_tests_parse_tokens();
        d_tests_parse_errors();
        d_tests_render();
        d_tests_render_numbers();
        d_tests_round_trip();
        d_tests_folds_agree();
        d_tests_numbers_read_back();
        d_tests_substitute();
        d_tests_substitute_random();
        d_tests_simplify_constants();
        d_tests_simplify_powers();
        d_tests_simplify_sums();
        d_tests_simplify_products();
        d_tests_simplify_values();
        d_tests_simplify_random();
        d_tests_rewrites();
        d_tests_matrices();
        d_tests_parse_literals();
        d_tests_render_literals();
        d_tests_fold_literals();
        d_tests_literals_random();
        d_tests_matrix_arithmetic();
        d_tests_matrix_powers();
        d_tests_parse_indexing();
        d_tests_render_indexing();
        d_tests_fold_indexing();
        d_tests_simplify_matrices();
        d_tests_matrices_random();
        d_tests_matrix_algebra();
        d_tests_matrix_eigenvalues();
        d_tests_matrix_negative_powers();
        d_tests_empty_matrix();
        d_tests_blocks();
        d_tests_fold_algebra();
        d_tests_algebra_random();
        d_tests_units();
        d_tests_calculus();
        d_tests_handle();
        d_tests_handle_rewrites();
        d_tests_handle_entries();
        d_tests_handle_chains();
        d_tests_handle_agrees();
        d_tests_types();

        return;
    }
}  // namespace

#else

namespace
{
    //==========================================================================
    // 15. THE MODULE
    //==========================================================================

    // d_tests_module
    //   below the module's floor: each header read, and nothing more to
    // test, as each is empty here.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_MODULE_READ == 1),
                        "the module's seventeen headers read");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    d_tests_module();

    std::printf("t_expr_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
