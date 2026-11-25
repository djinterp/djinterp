/*******************************************************************************
* djinterp [cpp][math]                                   t_expr_face_helpers.hpp
*
* One part of t_expr_face.cpp, the expression language's face test (S438), its
* sections 1 and 2: reporting a check, and the helpers the other parts share.
* The test includes the parts in order, one program; each part includes the one
* before it.
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_face_helpers.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_HELPERS_HPP
#define DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_HELPERS_HPP 1

// the module's headers: t_expr_face.cpp includes them, before this part
// std
#include <cstdio>  // std::printf

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
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_HELPERS_HPP
