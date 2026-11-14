/*******************************************************************************
* djinterp [cpp/math]                                 t_expr_quaternion_face.cpp
*
* Tests for math/expr/quaternion.hpp, the language's quaternions, and for what
* step 24's second part (3f) made of them in the language: the kind and the
* folds, the functions, the order of factors and the renderer.
*   - the kind: its table, its rank, a number exact or not value by value,
*     and its holders, the constants jm and km among them;
*   - making one: exact parts kept as values, a complex or real number where
*     they are exact and the jm and km parts are 0; floating-point parts
*     dense in the widest type, each its nearest there, an inexact one kept
*     whole, its zeros with their signs; an error passed, and what is no
*     real part refused;
*   - its arithmetic: exact, against the test's own quaternions of
*     rationals, on random operands, real and complex ones among them, a
*     quotient on the right, p inv(q), and by an exact 0 a pole; past the
*     family G2's long double, flagged; dense, every part quat.h's bit for
*     bit, through math::quaternion<T>, in float, double and long double, a
*     real operand taken as itself, a complex one with its im the first
*     unit, and an exact operand its nearest in the other's type;
*   - -q and |q|: exact where the norm is rational, the long double nearest
*     otherwise, and quat.h's scaled norm for dense parts;
*   - comparison by the parts, a missing part 0, unordered otherwise and for
*     a NaN part; conversion where the im, jm and km parts are 0, never of
*     an exact quaternion, with the rounding's direction;
*   - the kind's text, and a buffer too short;
*   - parts, real, conj, the inverse and the sign of every kind of number,
*     and -q, |q|, conj, inv and sign of a dense one in each type;
*     expr_quaternion_of and expr_quaternion_to;
*   - exp, log, sqrt and real powers in each type, quat.h's bit for bit;
*     exact roots and powers; the rotation, exact, against the test's own
*     rationals too, and dense, and its errors;
*   - the language: jm and km, their products in order, p / q = p inv(q),
*     quat, parts and rotate, the norm, the conjugate, the inverse and the
*     sign; the relations; every function a quaternion has no value of a
*     domain error; real_results, quat apart; and what step 24's third
*     part made values, linear algebra's functions of a quaternion, a
*     literal holding one and its products with a matrix, in their order;
*   - the folds in float, double and long double and in math::complex<T>:
*     jm and km NaNs, quat's real or complex value or a NaN, a bound
*     quaternion's nearest;
*   - the renderer: a quaternion as a sum, in brackets, every kind of part,
*     read back as the same number, on random exact ones too;
*   - simplification keeping a quaternion's factors in their order, and a
*     complex number's beside one (S277);
*   - parts of a boxed kind that is not exact, the test's own, kept as
*     values, a part's error passing; storage refused, by the test's own
*     nothrow operator new, a memory error; the entries asked what the
*     protocol never asks them; and every path no other check reaches, so
*     that each line of the header's a test runs.
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/expr/t_expr_quaternion_face.cpp      \
*         -o t_quat_face
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_quaternion_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.06
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/quaternion.hpp"
// std
#include <cstdio>  // std::printf


// D_TESTS_HEADER_READ
//   1 where the header was read: at every level, as it defines its guard
// before its floor.
#if defined(DJINTERP_MATH_EXPR_QUATERNION_HPP)
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

// std, from the module's floor
#include <cmath>    // std::signbit, std::ldexp, std::sqrt
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <new>      // std::nothrow_t, operator new
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/evaluate.hpp"  // the folds
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/function.hpp"  // the tables
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/matrix.hpp"  // matrices
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/parse.hpp"  // expr_parse
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/render.hpp"  // expr_render
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/simplify.hpp"  // expr_simplify


namespace
{
    // g_refuse_storage
    //   whether the nothrow operator new below refuses, as a host's arena
    // may: the quaternion kind's boxes are taken by it.
    bool g_refuse_storage = false;
}  // namespace

// operator new, nothrow
//   the program's own, which refuses while g_refuse_storage is set, and
// otherwise takes the standard operator new's storage, which the standard
// operator delete gives back.
void*
operator new(
    std::size_t _size,
    const std::nothrow_t&
) noexcept
{
    // refused
    if (g_refuse_storage)
    {
        return nullptr;
    }

    try
    {
        return ::operator new(_size);
    }
    catch (...)
    {
        return nullptr;
    }
}


namespace
{
    namespace m = djinterp::math;

    typedef m::internal::expr_quaternion_storage d_tests_storage_kind;


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

    // d_tests_quat
    //   the math::quaternion<T> _w + _x im + _y jm + _z km, its real part
    // named first, as the language names it.
    template<typename Type>
    m::quaternion<Type>
    d_tests_quat(
        Type _w,
        Type _x,
        Type _y,
        Type _z
    )
    {
        return m::quaternion<Type>(_x, _y, _z, _w);
    }

    // d_tests_same_quaternion
    //   whether two math::quaternion<T> are the same, part by part.
    template<typename Type>
    bool
    d_tests_same_quaternion(
        const m::quaternion<Type>& _a,
        const m::quaternion<Type>& _b
    )
    {
        return ( (d_tests_same_bits(_a.w, _b.w)) &&
                 (d_tests_same_bits(_a.x, _b.x)) &&
                 (d_tests_same_bits(_a.y, _b.y)) &&
                 (d_tests_same_bits(_a.z, _b.z)) );
    }

    // d_tests_storage
    //   a quaternion's storage, as the kind's box holds it.
    d_tests_storage_kind
    d_tests_storage(
        const m::value& _q
    )
    {
        return m::internal::expr_quaternion_box_of(_q.c_value()).storage;
    }

    // d_tests_is_dense
    //   whether _q is a quaternion of dense parts of Type, the same as
    // _expected, part by part.
    template<typename Type>
    bool
    d_tests_is_dense(
        const m::value&            _q,
        const m::quaternion<Type>& _expected
    )
    {
        m::quaternion<Type> held(Type(0), Type(0), Type(0), Type(0));

        return ( (m::expr_is_quaternion(_q)) &&
                 (d_tests_storage(_q) ==
                      m::internal::expr_quaternion_storage_for<Type>()) &&
                 (!_q.is_exact()) &&
                 (m::expr_quaternion_to(_q, held)) &&
                 (d_tests_same_quaternion(held, _expected)) );
    }

    // d_tests_exact
    //   the exact number _w + _x im + _y jm + _z km of integers, as the kind
    // makes it.
    m::value
    d_tests_exact(
        dj_math_imax _w,
        dj_math_imax _x,
        dj_math_imax _y,
        dj_math_imax _z
    )
    {
        return m::expr_quaternion(m::value(_w),
                                  m::value(_x),
                                  m::value(_y),
                                  m::value(_z));
    }

    // d_tests_text
    //   a value's text: a boxed one's by its kind's text entry.
    std::string
    d_tests_text(
        const m::value& _x
    )
    {
        const dj_math_value& held = _x.c_value();

        // inline: none of this test's
        if (held.kind != D_MATH_VALUE_BOXED)
        {
            return "(inline)";
        }

        const std::size_t length = held.type->text(D_NULLPTR, 0, held);
        std::string       text(length + 1, '\0');

        static_cast<void>(held.type->text(&text[0], length + 1, held));
        text.resize(length);

        return text;
    }

    // d_tests_is_error
    //   whether a value is the error of the code given.
    bool
    d_tests_is_error(
        const m::value&     _x,
        dj_math_value_error _code
    )
    {
        dj_math_value_error code = D_MATH_VALUE_ERROR_MEMORY;

        return ( (_x.kind() == D_MATH_VALUE_ERROR) &&
                 (_x.to(code)) &&
                 (code == _code) );
    }

    // d_tests_parsed
    //   the term of a text, implied multiplication and the bars on where
    // _implied; a term made by default where it fails.
    m::expr_term
    d_tests_parsed(
        const char* _text,
        bool        _implied = true
    )
    {
        m::expr_syntax syntax;

        syntax.implied_multiplication = _implied;
        syntax.absolute_bars          = _implied;

        const m::expr_result<m::expr_term> parsed =
            m::expr_parse(_text, syntax);

        return ( parsed.is_ok() ? parsed.value() : m::expr_term() );
    }

    // d_tests_folded
    //   a text's fold to a value, at _bindings, under _context.
    m::expr_result<m::value>
    d_tests_folded(
        const char*             _text,
        const m::expr_bindings& _bindings = m::expr_bindings(),
        const m::expr_context&  _context  = m::expr_context()
    )
    {
        return m::expr_evaluate(d_tests_parsed(_text), _bindings, _context);
    }

    // d_tests_value_of
    //   a text's value, with x bound to _x; none where the fold fails.
    m::value
    d_tests_value_of(
        const char*     _text,
        const m::value& _x = m::value(0)
    )
    {
        const m::expr_result<m::value> folded =
            d_tests_folded(_text, m::at("x", _x));

        return ( folded.is_ok() ? folded.value() : m::value() );
    }

    // d_tests_is
    //   whether a text's value is _expected, exactly: equal by the parts,
    // and exact as it is.
    bool
    d_tests_is(
        const char*     _text,
        const m::value& _expected
    )
    {
        const m::value got = d_tests_value_of(_text);

        return ( (got.is_number()) &&
                 (got.is_exact() == _expected.is_exact()) &&
                 (got.compare(_expected) == D_MATH_VALUE_EQUAL) );
    }

    // d_tests_domain
    //   whether a text's value is a domain error.
    bool
    d_tests_domain(
        const char* _text
    )
    {
        return d_tests_is_error(d_tests_value_of(_text),
                                D_MATH_VALUE_ERROR_DOMAIN);
    }

    // d_tests_fails
    //   whether a text's fold fails with _status, and where _message is not
    // empty, with that message.
    bool
    d_tests_fails(
        const char*        _text,
        m::expr_status     _status,
        const std::string& _message = std::string()
    )
    {
        const m::expr_result<m::value> folded = d_tests_folded(_text);

        return ( (folded.is_err()) &&
                 (folded.error().status == _status) &&
                 ( (_message.empty()) ||
                   (folded.error().message == _message) ) );
    }

    // d_tests_in
    //   a text's value in Type, x bound to _x; minus an infinity where the
    // fold fails.
    template<typename Type>
    Type
    d_tests_in(
        const char*     _text,
        const m::value& _x = m::value(0)
    )
    {
        const m::expr_result<Type> folded =
            m::expr_evaluate<Type>(d_tests_parsed(_text), m::at("x", _x));

        return ( folded.is_ok() ? folded.value()
                                : -std::numeric_limits<Type>::infinity() );
    }

    // d_tests_in_complex
    //   a text's value in math::complex<T>, x bound to _x; minus an
    // infinity in both parts where the fold fails.
    template<typename Type>
    m::complex<Type>
    d_tests_in_complex(
        const char*     _text,
        const m::value& _x = m::value(0)
    )
    {
        const Type                             fail =
            -std::numeric_limits<Type>::infinity();
        const m::expr_result<m::complex<Type> > folded =
            m::expr_evaluate<m::complex<Type> >(d_tests_parsed(_text),
                                                m::at("x", _x));

        return ( folded.is_ok() ? folded.value()
                                : m::complex<Type>(fail, fail) );
    }

    // d_tests_rendered
    //   a value's text as the renderer writes a number a fold made.
    std::string
    d_tests_rendered(
        const m::value& _x
    )
    {
        return m::expr_render(m::expr_number(_x));
    }

    // d_tests_reads_back
    //   whether a value's rendered text, read by the default syntax,
    // implied multiplication off, folds to the same value, exactly.
    bool
    d_tests_reads_back(
        const m::value& _x
    )
    {
        const m::expr_result<m::expr_term> parsed =
            m::expr_parse(d_tests_rendered(_x));

        if (parsed.is_err())
        {
            return false;
        }

        const m::expr_result<m::value> folded =
            m::expr_evaluate(parsed.value(),
                             m::expr_bindings(),
                             m::expr_context());

        return ( (folded.is_ok()) &&
                 (folded.value().is_exact() == _x.is_exact()) &&
                 (folded.value().compare(_x) == D_MATH_VALUE_EQUAL) );
    }

    // d_tests_element_is
    //   whether a matrix's element (_row, _column) is _expected, of its
    // kind, by the parts.
    bool
    d_tests_element_is(
        const m::value& _matrix,
        std::size_t     _row,
        std::size_t     _column,
        const m::value& _expected
    )
    {
        const m::expr_result<m::value> element =
            m::expr_element(_matrix, _row, _column);

        return ( (element.is_ok()) &&
                 (element.value().kind() == _expected.kind()) &&
                 (element.value().is_exact() == _expected.is_exact()) &&
                 (element.value().compare(_expected) ==
                      D_MATH_VALUE_EQUAL) );
    }

    // d_tests_row_is
    //   whether a value is the matrix of _rows by _columns whose elements,
    // row by row, are _expected's four or three, each of its kind.
    bool
    d_tests_row_is(
        const m::value&              _matrix,
        std::size_t                  _rows,
        std::size_t                  _columns,
        const std::vector<m::value>& _expected
    )
    {
        const m::expr_result<m::expr_shape> shape = m::expr_shape_of(_matrix);
        bool                                all   =
            ( (shape.is_ok()) &&
              (shape.value().rows == _rows) &&
              (shape.value().columns == _columns) );

        // each element, row by row
        for (std::size_t i = 0; ( (all) && (i < _expected.size()) ); ++i)
        {
            all = d_tests_element_is(_matrix,
                                     ( (_rows == 1) ? 0 : i ),
                                     ( (_rows == 1) ? i : 0 ),
                                     _expected[i]);
        }

        return all;
    }

    // the test's own random numbers: a 32-bit linear congruential generator,
    // the same in every build
    unsigned long g_state = 27182818UL;

    unsigned long
    d_tests_random()
    {
        g_state = ((g_state * 1103515245UL) + 12345UL) & 0xFFFFFFFFUL;

        return (g_state >> 8) & 0xFFFFFFUL;
    }

    // d_tests_random_real
    //   a random number of Type in about (-8, 8), of every exponent down
    // to 2^-20, the sign either.
    template<typename Type>
    Type
    d_tests_random_real()
    {
        const Type mantissa = static_cast<Type>(d_tests_random()) /
                              static_cast<Type>(0x1000000UL);
        const int  exponent = static_cast<int>(d_tests_random() % 24u) - 20;
        const Type x        = std::ldexp(mantissa, exponent) *
                              static_cast<Type>(8);

        return ( ((d_tests_random() & 1u) != 0u) ? -x : x );
    }

    // d_tests_random_quaternion
    //   a random math::quaternion<T>, each part d_tests_random_real's.
    template<typename Type>
    m::quaternion<Type>
    d_tests_random_quaternion()
    {
        const Type w = d_tests_random_real<Type>();
        const Type x = d_tests_random_real<Type>();
        const Type y = d_tests_random_real<Type>();
        const Type z = d_tests_random_real<Type>();

        return d_tests_quat(w, x, y, z);
    }


    //==========================================================================
    // 3.  THE TEST'S QUATERNIONS OF RATIONALS
    //==========================================================================
    // Exact quaternions of small terms, computed in long long: the reference
    // for the kind's exact arithmetic.

    struct d_tests_q
    {
        long long n;
        long long d;
    };

    struct d_tests_h
    {
        d_tests_q w;
        d_tests_q x;
        d_tests_q y;
        d_tests_q z;
    };

    long long
    d_tests_gcd(
        long long _a,
        long long _b
    )
    {
        _a = ( (_a < 0) ? -_a : _a );
        _b = ( (_b < 0) ? -_b : _b );

        // Euclid's
        while (_b != 0)
        {
            const long long r = _a % _b;

            _a = _b;
            _b = r;
        }

        return _a;
    }

    d_tests_q
    d_tests_q_of(
        long long _n,
        long long _d
    )
    {
        const long long g = d_tests_gcd(_n, _d);
        d_tests_q       q = { _n / g, _d / g };

        // the sign on the numerator
        if (q.d < 0)
        {
            q.n = -q.n;
            q.d = -q.d;
        }

        return q;
    }

    d_tests_q
    d_tests_q_add(
        d_tests_q _a,
        d_tests_q _b
    )
    {
        return d_tests_q_of( (_a.n * _b.d) + (_b.n * _a.d), _a.d * _b.d);
    }

    d_tests_q
    d_tests_q_sub(
        d_tests_q _a,
        d_tests_q _b
    )
    {
        return d_tests_q_of( (_a.n * _b.d) - (_b.n * _a.d), _a.d * _b.d);
    }

    d_tests_q
    d_tests_q_mul(
        d_tests_q _a,
        d_tests_q _b
    )
    {
        return d_tests_q_of(_a.n * _b.n, _a.d * _b.d);
    }

    d_tests_q
    d_tests_q_div(
        d_tests_q _a,
        d_tests_q _b
    )
    {
        return d_tests_q_of(_a.n * _b.d, _a.d * _b.n);
    }

    // d_tests_q_value
    //   the exact value of a rational.
    m::value
    d_tests_q_value(
        const d_tests_q& _q
    )
    {
        return m::value::ratio(static_cast<dj_math_imax>(_q.n),
                               static_cast<dj_math_imax>(_q.d));
    }

    // d_tests_h_value
    //   the value of a quaternion of rationals, made as the kind makes it.
    m::value
    d_tests_h_value(
        const d_tests_h& _h
    )
    {
        return m::expr_quaternion(
            m::value::ratio(static_cast<dj_math_imax>(_h.w.n),
                            static_cast<dj_math_imax>(_h.w.d)),
            m::value::ratio(static_cast<dj_math_imax>(_h.x.n),
                            static_cast<dj_math_imax>(_h.x.d)),
            m::value::ratio(static_cast<dj_math_imax>(_h.y.n),
                            static_cast<dj_math_imax>(_h.y.d)),
            m::value::ratio(static_cast<dj_math_imax>(_h.z.n),
                            static_cast<dj_math_imax>(_h.z.d)));
    }

    // d_tests_h_product
    //   Hamilton's product of two quaternions of rationals, im jm = km.
    d_tests_h
    d_tests_h_product(
        const d_tests_h& _a,
        const d_tests_h& _b
    )
    {
        d_tests_h p;

        p.w = d_tests_q_sub(d_tests_q_sub(d_tests_q_mul(_a.w, _b.w),
                                          d_tests_q_mul(_a.x, _b.x)),
                            d_tests_q_add(d_tests_q_mul(_a.y, _b.y),
                                          d_tests_q_mul(_a.z, _b.z)));
        p.x = d_tests_q_add(d_tests_q_add(d_tests_q_mul(_a.w, _b.x),
                                          d_tests_q_mul(_a.x, _b.w)),
                            d_tests_q_sub(d_tests_q_mul(_a.y, _b.z),
                                          d_tests_q_mul(_a.z, _b.y)));
        p.y = d_tests_q_add(d_tests_q_sub(d_tests_q_mul(_a.w, _b.y),
                                          d_tests_q_mul(_a.x, _b.z)),
                            d_tests_q_add(d_tests_q_mul(_a.y, _b.w),
                                          d_tests_q_mul(_a.z, _b.x)));
        p.z = d_tests_q_add(d_tests_q_add(d_tests_q_mul(_a.w, _b.z),
                                          d_tests_q_mul(_a.x, _b.y)),
                            d_tests_q_sub(d_tests_q_mul(_a.z, _b.w),
                                          d_tests_q_mul(_a.y, _b.x)));

        return p;
    }

    // d_tests_h_op
    //   _a _op _b, by the textbook formulas, exactly, a quotient on the
    // right, _a _b* / |_b|^2; false where _b is 0 for a quotient.
    bool
    d_tests_h_op(
        dj_math_value_op _op,
        const d_tests_h& _a,
        const d_tests_h& _b,
        d_tests_h&       _out
    )
    {
        // by operation
        switch (_op)
        {
            case D_MATH_VALUE_OP_ADD:
            case D_MATH_VALUE_OP_SUB:
            {
                const bool minus = (_op == D_MATH_VALUE_OP_SUB);

                _out.w = ( minus ? d_tests_q_sub(_a.w, _b.w)
                                 : d_tests_q_add(_a.w, _b.w) );
                _out.x = ( minus ? d_tests_q_sub(_a.x, _b.x)
                                 : d_tests_q_add(_a.x, _b.x) );
                _out.y = ( minus ? d_tests_q_sub(_a.y, _b.y)
                                 : d_tests_q_add(_a.y, _b.y) );
                _out.z = ( minus ? d_tests_q_sub(_a.z, _b.z)
                                 : d_tests_q_add(_a.z, _b.z) );

                return true;
            }

            case D_MATH_VALUE_OP_MUL:
                _out = d_tests_h_product(_a, _b);

                return true;

            case D_MATH_VALUE_OP_DIV:
            default:
                break;
        }

        const d_tests_q n =
            d_tests_q_add(d_tests_q_add(d_tests_q_mul(_b.w, _b.w),
                                        d_tests_q_mul(_b.x, _b.x)),
                          d_tests_q_add(d_tests_q_mul(_b.y, _b.y),
                                        d_tests_q_mul(_b.z, _b.z)));

        // no quotient by 0
        if (n.n == 0)
        {
            return false;
        }

        d_tests_h conj = _b;

        conj.x = d_tests_q_of(-_b.x.n, _b.x.d);
        conj.y = d_tests_q_of(-_b.y.n, _b.y.d);
        conj.z = d_tests_q_of(-_b.z.n, _b.z.d);

        const d_tests_h top = d_tests_h_product(_a, conj);

        _out.w = d_tests_q_div(top.w, n);
        _out.x = d_tests_q_div(top.x, n);
        _out.y = d_tests_q_div(top.y, n);
        _out.z = d_tests_q_div(top.z, n);

        return true;
    }

    // d_tests_random_q
    //   a random rational of terms below 10.
    d_tests_q
    d_tests_random_q()
    {
        return d_tests_q_of(
            static_cast<long long>(d_tests_random() % 19u) - 9,
            static_cast<long long>(d_tests_random() % 6u) + 1);
    }

    // d_tests_random_h
    //   a random quaternion of rationals; one time in six a real number and
    // one time in six a complex one, so that such operands come too.
    d_tests_h
    d_tests_random_h()
    {
        const d_tests_q     zero = { 0, 1 };
        const unsigned long pick = (d_tests_random() % 6u);
        d_tests_h           h;

        h.w = d_tests_random_q();
        h.x = ( (pick == 0u) ? zero : d_tests_random_q() );
        h.y = ( (pick <= 1u) ? zero : d_tests_random_q() );
        h.z = ( (pick <= 1u) ? zero : d_tests_random_q() );

        return h;
    }

    // d_tests_apply
    //   _a _op _b, as values.
    m::value
    d_tests_apply(
        dj_math_value_op _op,
        const m::value& _a,
        const m::value& _b
    )
    {
        // by operation
        switch (_op)
        {
            case D_MATH_VALUE_OP_ADD: return (_a + _b);
            case D_MATH_VALUE_OP_SUB: return (_a - _b);
            case D_MATH_VALUE_OP_MUL: return (_a * _b);
            case D_MATH_VALUE_OP_DIV:
            default:
                break;
        }

        return (_a / _b);
    }


    //==========================================================================
    // 4.  THE KIND
    //==========================================================================

    // d_tests_holders
    //   a quaternion's holders, as its box counts them.
    std::size_t
    d_tests_holders(
        const m::value& _q
    )
    {
        return m::internal::expr_quaternion_box_of(_q.c_value()).holders;
    }

    // d_tests_kind
    //   the table, the rank, the traits; the exactness entry; holders.
    void
    d_tests_kind()
    {
        const dj_math_boxed_kind* kind = m::internal::expr_quaternion_kind();
        const m::value            q    = d_tests_exact(1, 2, -3, 4);
        const m::value            p    =
            m::expr_quaternion_of(d_tests_quat(0.5, -0.25, 1.0, 2.0));

        D_TESTS_EXPECT( (kind != D_NULLPTR) &&
                        (std::string(kind->name) == "quaternion") &&
                        (kind->rank == 950) &&
                        (m::expr_quaternion_rank == 950) &&
                        (m::expr_quaternion_rank > m::expr_complex_rank) &&
                        (kind->traits == D_MATH_BOXED_NUMBER) &&
                        (kind->retain != D_NULLPTR) &&
                        (kind->release != D_NULLPTR) &&
                        (kind->unary != D_NULLPTR) &&
                        (kind->binary != D_NULLPTR) &&
                        (kind->cmp != D_NULLPTR) &&
                        (kind->convert != D_NULLPTR) &&
                        (kind->text != D_NULLPTR) &&
                        (kind->exact != D_NULLPTR) &&
                        (kind == m::internal::expr_quaternion_kind()),
                        "the table: quaternion, rank 950, past the complex "
                        "numbers' 900, a number, every entry and the exact "
                        "entry set, one for the program");
        D_TESTS_EXPECT( (m::expr_is_quaternion(q)) &&
                        (m::expr_is_quaternion(p)) &&
                        (q.is_number()) &&
                        (q.is_exact()) &&
                        (d_math_value_is_exact(q.c_value())) &&
                        (p.is_number()) &&
                        (!p.is_exact()) &&
                        (!d_math_value_is_exact(p.c_value())) &&
                        (d_math_value_rank(q.c_value()) == 950) &&
                        (std::string(d_math_value_kind_name(q.c_value())) ==
                             "quaternion") &&
                        (!m::expr_is_complex(q)) &&
                        (!m::expr_is_quaternion(m::value(1))) &&
                        (!m::expr_is_quaternion(m::expr_complex(
                             m::value(1), m::value(2)))) &&
                        (!m::expr_is_quaternion(m::value(true))) &&
                        (!m::expr_is_quaternion(m::value())),
                        "exact or not value by value, as the exact entry "
                        "says (S325); no other kind a quaternion");

        // holders: a copy holds the box, and lets it go
        std::size_t held = 0;

        {
            const m::value copy = q;

            held = d_tests_holders(q);
            static_cast<void>(copy);
        }

        const m::expr_named_constant* jm =
            m::expr_standard_constants().find("jm");
        const std::size_t before = d_tests_holders(jm->as_value);
        std::size_t       during = 0;

        {
            const m::expr_result<m::value> folded = d_tests_folded("jm");

            during = d_tests_holders(jm->as_value);
            static_cast<void>(folded);
        }

        D_TESTS_EXPECT( (held == 2) &&
                        (d_tests_holders(q) == 1) &&
                        (during == before + 1) &&
                        (d_tests_holders(jm->as_value) == before),
                        "holders counted: a copy one more, and one fewer "
                        "after; jm's box held by the value a fold gives, "
                        "and let go with it (S134)");

        return;
    }


    //==========================================================================
    // 5.  MAKING ONE
    //==========================================================================

    // d_tests_making
    //   exact parts as values, made complex or real where they come to it;
    // floating-point parts dense in the widest type; errors and refusals.
    void
    d_tests_making()
    {
        const m::value q = d_tests_exact(1, 2, -3, 4);
        const m::value half =
            m::expr_quaternion(m::value::ratio(1, 2), m::value(0),
                               m::value(0), m::value::ratio(-3, 4));
        const m::value c = d_tests_exact(1, 2, 0, 0);
        const m::value r = d_tests_exact(5, 0, 0, 0);

        D_TESTS_EXPECT( (m::expr_is_quaternion(q)) &&
                        (d_tests_storage(q) == d_tests_storage_kind::values) &&
                        (m::expr_is_quaternion(half)) &&
                        (half.is_exact()) &&
                        (m::expr_is_complex(c)) &&
                        (c == m::expr_complex(m::value(1), m::value(2))) &&
                        (!m::expr_is_quaternion(r)) &&
                        (!m::expr_is_complex(r)) &&
                        (r.kind() == D_MATH_VALUE_INTEGER) &&
                        (r == m::value(5)),
                        "exact parts kept as values; jm and km parts of 0: "
                        "the complex number, and the real one where the im "
                        "part is 0 too (S275)");

        const m::value d  = m::expr_quaternion(m::value(1.5), m::value(0.0),
                                               m::value(-0.0), m::value(2));
        const m::value f  = m::expr_quaternion(m::value(0.5f), m::value(1),
                                               m::value(2), m::value(3));
        const m::value fd = m::expr_quaternion(m::value(0.5f), m::value(0.1),
                                               m::value(0), m::value(0));
        const m::value l  = m::expr_quaternion(m::value(1),
                                               m::value::ratio(1, 3),
                                               m::value(0.25L), m::value(0));

        D_TESTS_EXPECT( (d_tests_is_dense(d, d_tests_quat(1.5, 0.0, -0.0,
                                                          2.0))) &&
                        (d_tests_is_dense(f, d_tests_quat(0.5f, 1.0f, 2.0f,
                                                          3.0f))) &&
                        (d_tests_is_dense(fd, d_tests_quat(0.5, 0.1, 0.0,
                                                           0.0))) &&
                        (d_tests_is_dense(l, d_tests_quat(
                             1.0L, 1.0L / 3.0L, 0.25L, 0.0L))),
                        "floating-point parts dense in the widest type, each "
                        "its nearest there, an inexact one kept whole, its "
                        "zeros with their signs, though its jm and km parts "
                        "are 0");

        const m::value pole  = m::value(1) / m::value(0);
        const m::value range = m::value(D_MATH_VALUE_ERROR_RANGE);
        const m::value m2    = m::expr_vector(std::vector<m::value>(
                                   2, m::value(1))).value();

        D_TESTS_EXPECT( (d_tests_is_error(m::expr_quaternion(m::value(1),
                                                             pole,
                                                             m::value(1),
                                                             range),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(m::expr_quaternion(m::value(true),
                                                             m::value(0),
                                                             m::value(1),
                                                             m::value(0)),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_quaternion(m::value(),
                                                             m::value(0),
                                                             m::value(1),
                                                             m::value(0)),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_quaternion(m2,
                                                             m::value(0),
                                                             m::value(1),
                                                             m::value(0)),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_quaternion(c,
                                                             m::value(0),
                                                             m::value(1),
                                                             m::value(0)),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_quaternion(m::value(0),
                                                             m::value(0),
                                                             q,
                                                             m::value(0)),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "an error passes, the first; a boolean, none, a "
                        "matrix, a complex number or a quaternion as a part "
                        "a domain error");

        return;
    }


    //==========================================================================
    // 6.  ARITHMETIC
    //==========================================================================

    // d_tests_exact_arithmetic
    //   random quaternions of rationals, real and complex ones among them,
    // each operation exact, against the test's own.
    void
    d_tests_exact_arithmetic()
    {
        const dj_math_value_op ops[4] = { D_MATH_VALUE_OP_ADD,
                                          D_MATH_VALUE_OP_SUB,
                                          D_MATH_VALUE_OP_MUL,
                                          D_MATH_VALUE_OP_DIV };
        long                   tried  = 0;
        long                   agreed = 0;
        long                   fewer  = 0;

        // pairs of random operands, each operation with a quaternion
        for (int trial = 0; trial < 3000; ++trial)
        {
            const d_tests_h a  = d_tests_random_h();
            const d_tests_h b  = d_tests_random_h();
            const m::value  va = d_tests_h_value(a);
            const m::value  vb = d_tests_h_value(b);

            // only operations with a quaternion operand are the kind's
            if ( (!m::expr_is_quaternion(va)) &&
                 (!m::expr_is_quaternion(vb)) )
            {
                continue;
            }

            for (int k = 0; k < 4; ++k)
            {
                d_tests_h      expected;
                const bool     defined = d_tests_h_op(ops[k], a, b, expected);
                const m::value got     = d_tests_apply(ops[k], va, vb);

                ++tried;

                if (!defined)
                {
                    agreed += ( d_tests_is_error(got,
                                                 D_MATH_VALUE_ERROR_POLE)
                                    ? 1 : 0 );
                    continue;
                }

                const bool quaternion = ( (expected.y.n != 0) ||
                                          (expected.z.n != 0) );

                fewer  += ( (!quaternion) ? 1 : 0 );
                agreed += ( ( (got.is_exact()) &&
                              (got.flags() == 0u) &&
                              (m::expr_is_quaternion(got) == quaternion) &&
                              (got.compare(d_tests_h_value(expected)) ==
                                   D_MATH_VALUE_EQUAL) )
                                ? 1 : 0 );
            }
        }

        D_TESTS_EXPECT( (agreed == tried) &&
                        (tried > 9000) &&
                        (fewer > 0),
                        "random quaternions of rationals, real and complex "
                        "operands among them: each sum, difference, product "
                        "and quotient on the right exact, a complex or real "
                        "number where its jm and km parts come to 0, a "
                        "quotient by an exact 0 a pole");

        return;
    }

    // d_tests_exact_cases
    //   the textbook cases, and a part past the family.
    void
    d_tests_exact_cases()
    {
        const m::value i = m::expr_complex(m::value(0), m::value(1));
        const m::value j = d_tests_exact(0, 0, 1, 0);
        const m::value k = d_tests_exact(0, 0, 0, 1);

        D_TESTS_EXPECT( (i * j == k) &&
                        (j * k == i) &&
                        (k * i == j) &&
                        (j * i == -k) &&
                        (k * j == -i) &&
                        (i * k == -j) &&
                        (j * j == m::value(-1)) &&
                        ((j * j).kind() == D_MATH_VALUE_INTEGER) &&
                        (m::expr_is_complex(j * k)) &&
                        ((i * j) * k == m::value(-1)) &&
                        (d_tests_exact(1, 0, 2, 0) * d_tests_exact(3, 0, 0, -1)
                             == d_tests_exact(3, -2, 6, -1)) &&
                        ((m::value(1) + i) / j == d_tests_exact(0, 0, -1, -1))
                        && (j / k == m::expr_complex(m::value(0),
                                                     m::value(-1))) &&
                        (m::value(2) - j == d_tests_exact(2, 0, -1, 0)) &&
                        (d_tests_is_error(j / m::value(0),
                                          D_MATH_VALUE_ERROR_POLE)),
                        "im jm = km, jm km = im, km im = jm and their "
                        "reverses negated; jm^2 = -1, an integer; im jm km = "
                        "-1; (1 + 2jm)(3 - km) = 3 - 2im + 6jm - km; (1 + im) "
                        "/ jm = -jm - km, on the right; jm / km = -im; 2 - jm; "
                        "jm / 0 a pole");

        // past the family: G2's long double, flagged, the number then of
        // long double parts
        const dj_math_imax big  = std::numeric_limits<dj_math_imax>::max();
        const m::value     wide = d_tests_exact(big, 1, 1, 0);
        const m::value     prod = wide * wide;

        D_TESTS_EXPECT( (m::expr_is_quaternion(prod)) &&
                        (!prod.is_exact()) &&
                        (d_tests_storage(prod) ==
                             d_tests_storage_kind::long_doubles) &&
                        ((prod.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0u),
                        "a part past the family: G2's long double, flagged, "
                        "the number then of long double parts");

        return;
    }

    // d_tests_operand
    //   a random operand of Type, as a value and as the quaternion an
    // operation reads it as: a real number's w alone, a complex number's w
    // and x, the others +0; _held 1, 2 or 4.
    template<typename Type>
    m::value
    d_tests_operand(
        const m::quaternion<Type>& _q,
        std::size_t                _held,
        m::quaternion<Type>&       _read
    )
    {
        _read = _q;

        // a real or complex operand: its other parts +0
        if (_held < 4)
        {
            _read.y = Type(0);
            _read.z = Type(0);
        }

        if (_held < 2)
        {
            _read.x = Type(0);
        }

        return ( (_held == 1) ? m::value(_q.w)
               : (_held == 2) ? m::expr_complex_of(m::complex<Type>(_q.w,
                                                                    _q.x))
                              : m::expr_quaternion_of(_q) );
    }

    // d_tests_part_sum
    //   one part of _a + _b, or _a - _b where _minus: each operand's part
    // where it holds it, the other's missing part not taken.
    template<typename Type>
    Type
    d_tests_part_sum(
        Type _a,
        bool _in_a,
        Type _b,
        bool _in_b,
        bool _minus
    )
    {
        // both hold it, or one alone
        if ( (_in_a) &&
             (_in_b) )
        {
            return ( _minus ? (_a - _b) : (_a + _b) );
        }

        return ( _in_a ? _a : ( _minus ? -_b : _b ) );
    }

    // d_tests_dense_apply
    //   _a _op _b as math::quaternion<T> computes it, a real operand
    // scaling each part, a sum changing only the parts an operand holds.
    template<typename Type>
    m::quaternion<Type>
    d_tests_dense_apply(
        dj_math_value_op           _op,
        const m::quaternion<Type>& _a,
        std::size_t                _held_a,
        const m::quaternion<Type>& _b,
        std::size_t                _held_b
    )
    {
        const bool minus = (_op == D_MATH_VALUE_OP_SUB);

        // by operation, and which operand is real
        switch (_op)
        {
            case D_MATH_VALUE_OP_ADD:
            case D_MATH_VALUE_OP_SUB:
                return d_tests_quat(
                    d_tests_part_sum(_a.w, true, _b.w, true, minus),
                    d_tests_part_sum(_a.x, (_held_a > 1), _b.x,
                                     (_held_b > 1), minus),
                    d_tests_part_sum(_a.y, (_held_a > 2), _b.y,
                                     (_held_b > 2), minus),
                    d_tests_part_sum(_a.z, (_held_a > 2), _b.z,
                                     (_held_b > 2), minus));

            case D_MATH_VALUE_OP_MUL:
                if (_held_a == 1)
                {
                    return d_tests_quat(_b.w * _a.w, _b.x * _a.w,
                                        _b.y * _a.w, _b.z * _a.w);
                }

                if (_held_b == 1)
                {
                    return d_tests_quat(_a.w * _b.w, _a.x * _b.w,
                                        _a.y * _b.w, _a.z * _b.w);
                }

                return (_a * _b);

            case D_MATH_VALUE_OP_DIV:
            default:
                break;
        }

        // by a real number each part; any other on the right
        if (_held_b == 1)
        {
            return d_tests_quat(_a.w / _b.w, _a.x / _b.w,
                                _a.y / _b.w, _a.z / _b.w);
        }

        return (_a / _b);
    }

    // d_tests_dense_arithmetic_in
    //   random operands of Type, real and complex ones among them, each
    // operation the bits of math::quaternion<T>'s, quat.h's.
    template<typename Type>
    bool
    d_tests_dense_arithmetic_in()
    {
        const dj_math_value_op  ops[4] = { D_MATH_VALUE_OP_ADD,
                                           D_MATH_VALUE_OP_SUB,
                                           D_MATH_VALUE_OP_MUL,
                                           D_MATH_VALUE_OP_DIV };
        const std::size_t       held[5][2] = { { 4, 4 }, { 1, 4 }, { 4, 1 },
                                               { 2, 4 }, { 4, 2 } };
        bool                    all    = true;

        // pairs of random operands, a real or complex one now and then
        for (int trial = 0; trial < 1500; ++trial)
        {
            const unsigned long pick = (d_tests_random() % 5u);
            m::quaternion<Type> ra;
            m::quaternion<Type> rb;
            const m::value      va =
                d_tests_operand(d_tests_random_quaternion<Type>(),
                                held[pick][0],
                                ra);
            const m::value      vb =
                d_tests_operand(d_tests_random_quaternion<Type>(),
                                held[pick][1],
                                rb);

            for (int k = 0; k < 4; ++k)
            {
                const m::quaternion<Type> want =
                    d_tests_dense_apply(ops[k], ra, held[pick][0],
                                        rb, held[pick][1]);

                all = ( (all) &&
                        (d_tests_is_dense(d_tests_apply(ops[k], va, vb),
                                          want)) );
            }
        }

        return all;
    }

    // d_tests_dense_arithmetic
    //   in each type, quat.h's bits; an exact operand its nearest in the
    // other's type; float beside double in double.
    void
    d_tests_dense_arithmetic()
    {
        D_TESTS_EXPECT( (d_tests_dense_arithmetic_in<float>()),
                        "1,500 random pairs of float parts, real and complex "
                        "operands among them: each part quat.h's bit for "
                        "bit, a real operand taken as itself, a sum changing "
                        "only the parts an operand holds");
        D_TESTS_EXPECT( (d_tests_dense_arithmetic_in<double>()),
                        "and of double parts");
        D_TESTS_EXPECT( (d_tests_dense_arithmetic_in<long double>()),
                        "and of long double parts");

        const m::quaternion<double> q = d_tests_quat(0.75, -1.5, 2.0, -0.0);
        const m::quaternion<float>  f = d_tests_quat(0.5f, -0.0f, 1.25f,
                                                     3.0f);
        const m::value              vq = m::expr_quaternion_of(q);
        const m::value              vf = m::expr_quaternion_of(f);
        const m::value              exact =
            m::expr_quaternion(m::value::ratio(1, 3), m::value(2),
                               m::value(3), m::value(4));
        const double                third = 1.0 / 3.0;
        const float                 fthird = 1.0f / 3.0f;

        D_TESTS_EXPECT( (d_tests_is_dense(exact * vq,
                                          d_tests_quat(third, 2.0, 3.0,
                                                       4.0) * q)) &&
                        (d_tests_is_dense(vq / exact,
                                          q / d_tests_quat(third, 2.0, 3.0,
                                                           4.0))) &&
                        (d_tests_is_dense(m::value::ratio(1, 3) * vf,
                                          d_tests_quat(f.w * fthird,
                                                       f.x * fthird,
                                                       f.y * fthird,
                                                       f.z * fthird))) &&
                        (d_tests_is_dense(vf + m::value(0.1),
                                          d_tests_quat(0.5 + 0.1, -0.0, 1.25,
                                                       3.0))) &&
                        (d_tests_is_dense(vq - vf,
                                          d_tests_quat(0.75 - 0.5,
                                                       -1.5 - -0.0,
                                                       2.0 - 1.25,
                                                       -0.0 - 3.0))),
                        "an exact operand its nearest in the other's type, "
                        "its parts each rounded once; float beside double "
                        "computed in double, a sum keeping -0 where only "
                        "one operand holds the part");
        D_TESTS_EXPECT( (d_tests_is_dense(m::value(2.0) * vq,
                                          d_tests_quat(1.5, -3.0, 4.0,
                                                       -0.0))) &&
                        (d_tests_is_dense(vq * m::value(-1.0),
                                          d_tests_quat(-0.75, 1.5, -2.0,
                                                       0.0))) &&
                        (d_tests_is_dense(vq / m::value(4.0),
                                          d_tests_quat(0.1875, -0.375, 0.5,
                                                       -0.0))) &&
                        (d_tests_is_dense(m::expr_complex_of(
                                              m::complex<double>(1.0, -0.0))
                                              + vq,
                                          d_tests_quat(1.75, -1.5, 2.0,
                                                       -0.0))),
                        "a real operand taken as itself, each part scaled "
                        "by it, a zero's sign kept, as C99's G.5 takes one "
                        "beside a complex number; a complex operand's sum "
                        "changing w and x alone");

        return;
    }

    // d_tests_unary
    //   -q, each part negated; |q| exact where it is rational, the long
    // double nearest otherwise, quat.h's norm for dense parts.
    void
    d_tests_unary()
    {
        const m::quaternion<double> q = d_tests_quat(0.0, -0.0, 1.5, -2.0);
        const m::quaternion<float>  f = d_tests_quat(3.0f, 4.0f, 12.0f,
                                                     84.0f);
        const m::value              n = abs(m::expr_quaternion_of(q));
        const m::value              nf = abs(m::expr_quaternion_of(f));
        double                      dn = 0.0;
        float                       fn = 0.0f;

        D_TESTS_EXPECT( (-d_tests_exact(1, 2, -3, 4) ==
                             d_tests_exact(-1, -2, 3, -4)) &&
                        ((-d_tests_exact(1, 2, -3, 4)).is_exact()) &&
                        (d_tests_is_dense(-m::expr_quaternion_of(q),
                                          d_tests_quat(-0.0, 0.0, -1.5,
                                                       2.0))),
                        "-q: each part negated, exactly, and of dense parts "
                        "each sign flipped, a zero's too");
        D_TESTS_EXPECT( (abs(d_tests_exact(1, 2, 2, 4)) == m::value(5)) &&
                        (abs(d_tests_exact(1, 2, 2, 4)).kind() ==
                             D_MATH_VALUE_INTEGER) &&
                        (!abs(m::expr_quaternion(m::value::ratio(1, 2),
                                                 m::value(0),
                                                 m::value::ratio(1, 2),
                                                 m::value(0))).is_exact()) &&
                        (abs(d_tests_exact(1, 1, 1, 0)).compare(
                             m::value(std::sqrt(3.0L))) ==
                                 D_MATH_VALUE_EQUAL) &&
                        (!abs(d_tests_exact(1, 1, 1, 0)).is_exact()) &&
                        (abs(m::expr_quaternion(m::value::ratio(3, 5),
                                                m::value(0),
                                                m::value(0),
                                                m::value::ratio(4, 5))) ==
                             m::value(1)),
                        "|q| of exact parts: exact where it is rational, "
                        "|1 + 2im + 2jm + 4km| = 5, |3/5 + 4/5km| = 1; the "
                        "long double nearest otherwise, |1 + im + jm|");
        D_TESTS_EXPECT( (n.kind() == D_MATH_VALUE_DOUBLE) &&
                        (n.to(dn)) &&
                        (d_tests_same_bits(dn, norm(q))) &&
                        (nf.kind() == D_MATH_VALUE_FLOAT) &&
                        (nf.to(fn)) &&
                        (fn == 85.0f) &&
                        (d_tests_same_bits(fn, norm(f))),
                        "|q| of dense parts: quat.h's scaled norm, of their "
                        "type, |3 + 4im + 12jm + 84km| = 85 in float");

        const dj_math_imax big = std::numeric_limits<dj_math_imax>::max();
        const m::value     far = abs(d_tests_exact(big, big, 1, 0));

        D_TESTS_EXPECT( (!far.is_exact()) &&
                        (far.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        ((far.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0u),
                        "the sum of the squares past the family: G2's long "
                        "double, flagged");

        return;
    }

    // d_tests_comparison
    //   equal by the parts, a missing part 0; unordered otherwise and for a
    // NaN part.
    void
    d_tests_comparison()
    {
        const double   nan = std::numeric_limits<double>::quiet_NaN();
        const m::value q   = d_tests_exact(1, 2, 3, 4);
        const m::value dc  =
            m::expr_quaternion_of(d_tests_quat(1.0, 2.0, 0.0, -0.0));
        const m::value dr  =
            m::expr_quaternion_of(d_tests_quat(3.0, -0.0, 0.0, 0.0));
        const m::value dn  =
            m::expr_quaternion_of(d_tests_quat(nan, 0.0, 1.0, 0.0));

        D_TESTS_EXPECT( (q.compare(d_tests_exact(1, 2, 3, 4)) ==
                             D_MATH_VALUE_EQUAL) &&
                        (q == m::expr_quaternion_of(
                                  d_tests_quat(1.0, 2.0, 3.0, 4.0))) &&
                        (dc.compare(m::expr_complex(m::value(1),
                                                    m::value(2))) ==
                             D_MATH_VALUE_EQUAL) &&
                        (m::expr_complex(m::value(1), m::value(2)).compare(
                             dc) == D_MATH_VALUE_EQUAL) &&
                        (dr.compare(m::value(3)) == D_MATH_VALUE_EQUAL) &&
                        (m::value(3).compare(dr) == D_MATH_VALUE_EQUAL),
                        "equal by the parts, exact or dense, a complex or "
                        "real number's missing parts 0, from either side");
        D_TESTS_EXPECT( (q.compare(d_tests_exact(1, 2, 3, 5)) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (d_tests_exact(1, 2, 3, 5).compare(q) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (q.compare(m::value(1)) == D_MATH_VALUE_UNORDERED) &&
                        (dn.compare(dn) == D_MATH_VALUE_UNORDERED) &&
                        (q != d_tests_exact(4, 3, 2, 1)),
                        "no order otherwise, as no order of the quaternions "
                        "is a field's, nor with a NaN part");

        return;
    }

    // d_tests_conversion
    //   to a real type where the im, jm and km parts are 0; never of an
    // exact quaternion, nor to an exact kind; the rounding's direction.
    void
    d_tests_conversion()
    {
        const dj_math_boxed_kind* kind = m::internal::expr_quaternion_kind();
        const m::value            d    =
            m::expr_quaternion_of(d_tests_quat(2.5, -0.0, 0.0, -0.0));
        const m::value            t    =
            m::expr_quaternion_of(d_tests_quat(0.1, 0.0, 0.0, 0.0));
        const m::value            v    =
            m::expr_quaternion_of(d_tests_quat(2.5, 0.0, 1e-300, 0.0));
        double                    x    = 0.0;
        float                     f    = 0.0f;
        long double               l    = 0.0L;
        dj_rational               r    = { 7, 1 };
        dj_math_value             out  = d_math_value_none();
        int                       dir  = 9;

        D_TESTS_EXPECT( (d.to(x)) &&
                        (x == 2.5) &&
                        (d.to(f)) &&
                        (f == 2.5f) &&
                        (d.to(l)) &&
                        (l == 2.5L) &&
                        (!d.to(r)) &&
                        (r.num == 7) &&
                        (!v.to(x)) &&
                        (!m::expr_quaternion_of(d_tests_quat(2.5, 1.0, 0.0,
                                                             0.0)).to(x)) &&
                        (!m::expr_quaternion_of(d_tests_quat(2.5, 0.0, 0.0,
                                                             -1.0)).to(x)) &&
                        (!d_tests_exact(1, 0, 1, 0).to(x)) &&
                        (x == 2.5),
                        "the w part's nearest where the others are 0, of "
                        "either sign; none to an exact kind, none of a part "
                        "past w, none of an exact quaternion");
        D_TESTS_EXPECT( (kind->convert(&out, &dir, t.c_value(),
                                       D_MATH_VALUE_FLOAT)) &&
                        (out.kind == D_MATH_VALUE_FLOAT) &&
                        (dir == 1) &&
                        (kind->convert(&out, &dir, t.c_value(),
                                       D_MATH_VALUE_DOUBLE)) &&
                        (dir == 0),
                        "the rounding's direction found exactly: 0.1 to "
                        "float up, to double exact");

        return;
    }

    // d_tests_kind_text
    //   the value layer's text of every kind of part, and a buffer too
    // short.
    void
    d_tests_kind_text()
    {
        const double              nan   =
            std::numeric_limits<double>::quiet_NaN();
        const double              inf   =
            std::numeric_limits<double>::infinity();
        const dj_math_boxed_kind* kind  = m::internal::expr_quaternion_kind();
        const m::value            q     = d_tests_exact(1, 2, -3, 4);
        char                      buffer[5] = { 'x', 'x', 'x', 'x', 'x' };

        D_TESTS_EXPECT( (d_tests_text(q) == "1 + 2im - 3jm + 4km") &&
                        (d_tests_text(m::expr_quaternion(
                             m::value::ratio(1, 2), m::value(0),
                             m::value(0), m::value::ratio(-3, 4))) ==
                             "1/2 + 0im + 0jm - 3/4km") &&
                        (d_tests_text(m::expr_quaternion_of(d_tests_quat(
                             0.1, 0.0, -0.0, 2.5))) ==
                             "0.10000000000000001 + 0im - 0jm + 2.5km") &&
                        (d_tests_text(m::expr_quaternion_of(d_tests_quat(
                             0.5f, -0.25f, 1.0f, 0.0f))) ==
                             "0.5 - 0.25im + 1jm + 0km") &&
                        (d_tests_text(m::expr_quaternion_of(d_tests_quat(
                             nan, inf, -inf, 1.0))) ==
                             "nan + inf*im - inf*jm + 1km"),
                        "w, then each part's sign, magnitude and unit: exact "
                        "parts as integers or fractions, floating-point ones "
                        "in max_digits10 digits, -0, nan and inf by name");
        D_TESTS_EXPECT( (kind->text(buffer, 5, q.c_value()) == 19u) &&
                        (std::string(buffer) == "1 + ") &&
                        (kind->text(D_NULLPTR, 0, q.c_value()) == 19u),
                        "a buffer too short: as much as it holds, the NUL "
                        "among it, and the whole length");

        return;
    }


    //==========================================================================
    // 7.  THE PARTS, THE INVERSE AND THE SIGN
    //==========================================================================

    // d_tests_parts
    //   parts and real of every kind of number; an error passes, and what
    // is no number is a domain error.
    void
    d_tests_parts()
    {
        const m::value q = d_tests_exact(1, 2, 3, 4);
        const m::value d =
            m::expr_quaternion_of(d_tests_quat(0.5, -0.0, 1.0, -2.0));
        const m::value pole = m::value(1) / m::value(0);
        std::vector<m::value> four;

        four.push_back(m::value(1));
        four.push_back(m::value(2));
        four.push_back(m::value(3));
        four.push_back(m::value(4));

        D_TESTS_EXPECT( (d_tests_row_is(m::expr_quaternion_parts(q), 1, 4,
                                        four)) &&
                        (d_tests_row_is(m::expr_quaternion_parts(
                                            m::expr_complex(m::value(1),
                                                            m::value(2))),
                                        1, 4,
                                        { m::value(1), m::value(2),
                                          m::value(0), m::value(0) })) &&
                        (d_tests_row_is(m::expr_quaternion_parts(
                                            m::value(3)),
                                        1, 4,
                                        { m::value(3), m::value(0),
                                          m::value(0), m::value(0) })),
                        "parts: the vector [w, x, y, z], of a complex or "
                        "real number its missing parts exact 0s");
        D_TESTS_EXPECT( (d_tests_row_is(m::expr_quaternion_parts(d), 1, 4,
                                        { m::value(0.5), m::value(-0.0),
                                          m::value(1.0), m::value(-2.0) }))
                        && (d_tests_row_is(m::expr_quaternion_parts(
                                               m::value(2.5f)),
                                           1, 4,
                                           { m::value(2.5f), m::value(0.0f),
                                             m::value(0.0f),
                                             m::value(0.0f) })) &&
                        (d_tests_is_error(m::expr_quaternion_parts(pole),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(m::expr_quaternion_parts(
                                              m::value(true)),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_quaternion_parts(
                                              m::expr_quaternion_parts(q)),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "of dense parts values of their type, a float's "
                        "missing parts float 0s; an error passes; a boolean "
                        "or a matrix a domain error");
        D_TESTS_EXPECT( (m::expr_quaternion_real(q) == m::value(1)) &&
                        (m::expr_quaternion_real(q).kind() ==
                             D_MATH_VALUE_INTEGER) &&
                        (m::expr_quaternion_real(d).kind() ==
                             D_MATH_VALUE_DOUBLE) &&
                        (m::expr_quaternion_real(d) == m::value(0.5)) &&
                        (m::expr_quaternion_real(m::expr_complex(
                             m::value(7), m::value(1))) == m::value(7)) &&
                        (m::expr_quaternion_real(m::value(2.5)) ==
                             m::value(2.5)) &&
                        (d_tests_is_error(m::expr_quaternion_real(
                                              m::value()),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "real: w as it is held; a complex or real number's "
                        "real part; none a domain error");

        return;
    }

    // d_tests_conj_inverse_sign
    //   the conjugate, the inverse and the sign, exact and dense.
    void
    d_tests_conj_inverse_sign()
    {
        const m::quaternion<double> q = d_tests_quat(1.0, 0.0, -0.0, 2.0);
        const m::quaternion<double> r = d_tests_quat(0.5, -1.25, 3.0, 0.75);
        const m::value              vq = m::expr_quaternion_of(q);
        const m::value              vr = m::expr_quaternion_of(r);
        const m::value              zero =
            m::expr_quaternion_of(d_tests_quat(0.0, -0.0, 0.0, 0.0));

        D_TESTS_EXPECT( (m::expr_quaternion_conj(d_tests_exact(1, 2, 3, 4))
                             == d_tests_exact(1, -2, -3, -4)) &&
                        (d_tests_is_dense(m::expr_quaternion_conj(vq),
                                          d_tests_quat(1.0, -0.0, 0.0,
                                                       -2.0))) &&
                        (m::expr_quaternion_conj(m::expr_complex(
                             m::value(1), m::value(2))) ==
                             m::expr_complex(m::value(1), m::value(-2))) &&
                        (m::expr_quaternion_conj(m::value(3)) ==
                             m::value(3)),
                        "conj: each part past w negated, exactly, and of "
                        "dense parts each sign flipped; a complex or real "
                        "number's conjugate");
        D_TESTS_EXPECT( (m::expr_quaternion_inverse(d_tests_exact(1, 0, 1,
                                                                  0)) ==
                             m::expr_quaternion(m::value::ratio(1, 2),
                                                m::value(0),
                                                m::value::ratio(-1, 2),
                                                m::value(0))) &&
                        (m::expr_quaternion_inverse(d_tests_exact(1, 2, 3,
                                                                  4)) *
                             d_tests_exact(1, 2, 3, 4) == m::value(1)) &&
                        (d_tests_is_dense(m::expr_quaternion_inverse(vr),
                                          inverse(r))) &&
                        (m::expr_quaternion_inverse(m::value(2)) ==
                             m::value::ratio(1, 2)) &&
                        (m::expr_quaternion_inverse(m::expr_complex(
                             m::value(0), m::value(1))) ==
                             m::expr_complex(m::value(0), m::value(-1))) &&
                        (d_tests_is_error(m::expr_quaternion_inverse(
                                              m::value(0)),
                                          D_MATH_VALUE_ERROR_POLE)),
                        "the inverse: q* / |q|^2 exactly, inv(q) q = 1; of "
                        "dense parts quat.h's (S359); a real or complex "
                        "number's 1 / x, an exact 0's a pole");
        D_TESTS_EXPECT( (m::expr_quaternion_sign(d_tests_exact(1, 2, 2, 4))
                             == m::expr_quaternion(m::value::ratio(1, 5),
                                                   m::value::ratio(2, 5),
                                                   m::value::ratio(2, 5),
                                                   m::value::ratio(4, 5)))
                        && (d_tests_is_dense(m::expr_quaternion_sign(
                                                 d_tests_exact(1, 0, 1, 0)),
                                             normalize(d_tests_quat(
                                                 1.0L, 0.0L, 1.0L, 0.0L))))
                        && (d_tests_is_dense(m::expr_quaternion_sign(vr),
                                             normalize(r))) &&
                        (d_tests_is_dense(m::expr_quaternion_sign(zero),
                                          d_tests_quat(0.0, -0.0, 0.0,
                                                       0.0))) &&
                        (d_tests_is_error(m::expr_quaternion_sign(
                                              m::value(2)),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "the sign: q / |q| exactly where |q| is rational; "
                        "otherwise quat.h's scaled normalization, a long "
                        "double for exact parts (S361); a 0 itself; no "
                        "quaternion a domain error, its sign function.hpp's");

        return;
    }

    // d_tests_dense_entries
    //   a dense quaternion of Type: -q, |q|, conj, inv and sign, each
    // quat.h's bits through math::quaternion<Type>, in Type.
    template<typename Type>
    bool
    d_tests_dense_entries()
    {
        const m::quaternion<Type> q =
            d_tests_quat(Type(1.5), Type(-0.0), Type(2.25), Type(-3));
        const m::value            v    = m::expr_quaternion_of(q);
        const m::value            n    = abs(v);
        Type                      held = Type(0);

        return ( (d_tests_is_dense(-v,
                                   d_tests_quat(Type(-1.5), Type(0),
                                                Type(-2.25), Type(3)))) &&
                 (!n.is_exact()) &&
                 (n.to(held)) &&
                 (d_tests_same_bits(held, norm(q))) &&
                 (d_tests_is_dense(m::expr_quaternion_conj(v),
                                   conjugate(q))) &&
                 (d_tests_is_dense(m::expr_quaternion_inverse(v),
                                   inverse(q))) &&
                 (d_tests_is_dense(m::expr_quaternion_sign(v),
                                   normalize(q))) );
    }

    // d_tests_every_type
    //   the entries of a dense quaternion in each type; the parts of a
    // complex number of long doubles, its missing parts long double 0s.
    void
    d_tests_every_type()
    {
        D_TESTS_EXPECT( (d_tests_dense_entries<float>()) &&
                        (d_tests_dense_entries<double>()) &&
                        (d_tests_dense_entries<long double>()),
                        "-q, |q|, conj, inv and sign of a dense quaternion "
                        "in float, double and long double, each in its "
                        "type, quat.h's bits");
        D_TESTS_EXPECT( d_tests_row_is(m::expr_quaternion_parts(
                                           m::expr_complex(m::value(1.5L),
                                                           m::value(-2.5L))),
                                       1, 4,
                                       { m::value(1.5L), m::value(-2.5L),
                                         m::value(0.0L), m::value(0.0L) }),
                        "parts of a complex number of long doubles: its "
                        "missing parts long double 0s");

        return;
    }

    // d_tests_of_and_to
    //   expr_quaternion_of and expr_quaternion_to, in each type.
    void
    d_tests_of_and_to()
    {
        const m::quaternion<float>       f = d_tests_quat(0.5f, -0.0f, 1.5f,
                                                          -2.0f);
        const m::quaternion<long double> l = d_tests_quat(0.1L, 0.2L, -0.3L,
                                                          0.4L);
        m::quaternion<double>            d(1.0, 1.0, 1.0, 1.0);
        m::quaternion<float>             back(9.0f, 9.0f, 9.0f, 9.0f);

        D_TESTS_EXPECT( (d_tests_is_dense(m::expr_quaternion_of(f), f)) &&
                        (d_tests_storage(m::expr_quaternion_of(f)) ==
                             d_tests_storage_kind::floats) &&
                        (d_tests_is_dense(m::expr_quaternion_of(l), l)) &&
                        (m::expr_quaternion_to(m::expr_quaternion_of(f), d))
                        && (d_tests_same_quaternion(
                                d, d_tests_quat(0.5, -0.0, 1.5, -2.0))),
                        "of: dense in T, as given; to: each part its "
                        "nearest, a float's widened exactly");
        D_TESTS_EXPECT( (m::expr_quaternion_to(m::expr_complex(m::value(3),
                                                               m::value(4)),
                                               back)) &&
                        (d_tests_same_quaternion(
                             back, d_tests_quat(3.0f, 4.0f, 0.0f, 0.0f))) &&
                        (m::expr_quaternion_to(m::value(-2), back)) &&
                        (d_tests_same_quaternion(
                             back, d_tests_quat(-2.0f, 0.0f, 0.0f, 0.0f))) &&
                        (!m::expr_quaternion_to(m::value(), back)) &&
                        (d_tests_same_quaternion(
                             back, d_tests_quat(-2.0f, 0.0f, 0.0f, 0.0f))),
                        "to of a complex number its two parts, of a real "
                        "one its w, the others +0; of none nothing");

        return;
    }


    //==========================================================================
    // 8.  FUNCTIONS
    //==========================================================================

    // d_tests_functions_in
    //   exp, log, sqrt and a real power of random quaternions of Type,
    // through expr_quaternion_apply and the language, quat.h's bit for bit.
    template<typename Type>
    bool
    d_tests_functions_in()
    {
        typedef m::internal::expr_quaternion_fn fn;

        bool all = true;

        // random quaternions and exponents
        for (int trial = 0; trial < 300; ++trial)
        {
            const m::quaternion<Type> q = d_tests_random_quaternion<Type>();
            const Type                p = d_tests_random_real<Type>() /
                                          Type(3);
            const m::value            v = m::expr_quaternion_of(q);
            const m::value            e = m::value(p);

            all = ( (all) &&
                    (d_tests_is_dense(m::expr_quaternion_apply(fn::exp, v,
                                                               e),
                                      exp(q))) &&
                    (d_tests_is_dense(m::expr_quaternion_apply(fn::log, v,
                                                               e),
                                      log(q))) &&
                    (d_tests_is_dense(m::expr_quaternion_apply(fn::sqrt, v,
                                                               e),
                                      sqrt(q))) &&
                    (d_tests_is_dense(m::expr_quaternion_apply(fn::pow, v,
                                                               e),
                                      pow(q, p))) &&
                    (d_tests_is_dense(d_tests_value_of("exp(x)", v),
                                      exp(q))) &&
                    (d_tests_is_dense(d_tests_value_of("log(x)", v),
                                      log(q))) &&
                    (d_tests_is_dense(d_tests_value_of("sqrt(x)", v),
                                      sqrt(q))) &&
                    (d_tests_is_dense(d_tests_value_of("x ^ 2", v),
                                      pow(q, Type(2)))) );
        }

        return all;
    }

    // d_tests_functions
    //   in each type quat.h's; of exact parts a long double, but exact where
    // a root or a power is rational.
    void
    d_tests_functions()
    {
        typedef m::internal::expr_quaternion_fn fn;

        const m::quaternion<long double> one_jm = d_tests_quat(1.0L, 0.0L,
                                                               1.0L, 0.0L);
        m::value                         root;

        D_TESTS_EXPECT( (d_tests_functions_in<float>()),
                        "300 random quaternions of float parts: exp, log, "
                        "sqrt and a real power, quat.h's _c forms bit for "
                        "bit, through expr_quaternion_apply and the "
                        "language, x ^ 2 among them");
        D_TESTS_EXPECT( (d_tests_functions_in<double>()),
                        "and of double parts");
        D_TESTS_EXPECT( (d_tests_functions_in<long double>()),
                        "and of long double parts");
        D_TESTS_EXPECT( (d_tests_is_dense(d_tests_value_of("exp(1 + jm)"),
                                          exp(one_jm))) &&
                        (d_tests_is_dense(d_tests_value_of("log(1 + jm)"),
                                          log(one_jm))) &&
                        (d_tests_is_dense(d_tests_value_of("sqrt(1 + jm)"),
                                          sqrt(one_jm))) &&
                        (d_tests_is_dense(d_tests_value_of("(1 + jm) ^ 0.5"),
                                          pow(one_jm, 0.5L))) &&
                        (d_tests_is_dense(m::expr_quaternion_apply(
                                              fn::pow,
                                              d_tests_exact(1, 0, 1, 0),
                                              m::value(0.5f)),
                                          pow(d_tests_quat(1.0f, 0.0f, 1.0f,
                                                           0.0f),
                                              0.5f))),
                        "exp, log, sqrt and a power not rational of exact "
                        "parts in long double, dense; to a float exponent in "
                        "float, the widest type of the operands");
        D_TESTS_EXPECT( (d_tests_is("sqrt(-2 + 2im + 2jm + 2km)",
                                    d_tests_exact(1, 1, 1, 1))) &&
                        (d_tests_is("sqrt(-3 + 4jm)",
                                    d_tests_exact(1, 0, 2, 0))) &&
                        (d_tests_is("(-46 + 3im + 6jm + 6km) ^ (1/3)",
                                    d_tests_exact(2, 1, 2, 2))) &&
                        (d_tests_is("(-2 + 2im + 2jm + 2km) ^ (3/2)",
                                    m::value(-8))) &&
                        (d_tests_is("(-3 + 4jm) ^ (-1/2)",
                                    m::expr_quaternion(
                                        m::value::ratio(1, 5),
                                        m::value(0),
                                        m::value::ratio(-2, 5),
                                        m::value(0)))) &&
                        (m::expr_quaternion_root(d_tests_exact(-3, 0, 4, 0),
                                                 root)) &&
                        (root == d_tests_exact(1, 0, 2, 0)) &&
                        (!m::expr_quaternion_root(d_tests_exact(1, 0, 1, 0),
                                                  root)) &&
                        (!m::expr_quaternion_root(m::value(4), root)),
                        "a root exact where it is a quaternion of rationals: "
                        "a square root, a cube root, a root to a power, and "
                        "to a negative one; none where |q| is irrational, "
                        "none of what is no quaternion");
        D_TESTS_EXPECT( (d_tests_is("(1 + jm) ^ 3", d_tests_exact(-2, 0, 2,
                                                                  0))) &&
                        (d_tests_is("(1 + jm) ^ -2",
                                    m::expr_quaternion(
                                        m::value(0), m::value(0),
                                        m::value::ratio(-1, 2),
                                        m::value(0)))) &&
                        (d_tests_is("(1 + jm) ^ 0", m::value(1))) &&
                        (d_tests_is("jm ^ 2", m::value(-1))) &&
                        (d_tests_is("pow(km, 3)", d_tests_exact(0, 0, 0,
                                                                -1))),
                        "whole powers exactly, by repeated squaring, a "
                        "negative one the inverse's; q ^ 0 = 1; pow as ^");

        return;
    }

    // d_tests_function_domains
    //   every function a quaternion has no value of, and every exponent
    // that is no real number: a domain error.
    void
    d_tests_function_domains()
    {
        const char* const none[] = {
            "sin(jm)", "cos(jm)", "tan(jm)", "asin(jm)", "acos(jm)",
            "atan(jm)", "sinh(km)", "cosh(km)", "tanh(km)", "asinh(km)",
            "acosh(km)", "atanh(km)", "cbrt(jm)", "exp2(jm)", "log2(jm)",
            "log10(jm)", "log1p(jm)", "expm1(jm)", "floor(jm)", "ceil(jm)",
            "round(jm)", "trunc(jm)", "jm mod 2", "jm!", "factorial(jm)",
            "fmod(jm, 2)", "remainder(jm, 2)", "atan2(jm, 1)",
            "hypot(jm, 1)", "min(jm, 1)", "max(1, km)", "imag(jm)",
            "arg(jm)", "complex(jm, 1)", "jm ^ im", "jm ^ km", "2 ^ jm",
            "im ^ jm", "jm < 1", "jm <= km", "1 > km", "jm >= jm" };
        int         failed = 0;

        // each a domain error
        for (std::size_t i = 0; i < (sizeof(none) / sizeof(none[0])); ++i)
        {
            failed += ( d_tests_domain(none[i]) ? 0 : 1 );
        }

        D_TESTS_EXPECT( (failed == 0),
                        "the trigonometric and hyperbolic functions and "
                        "their inverses, the real line's order, imag, arg "
                        "and complex of a quaternion, a complex or "
                        "quaternion exponent, and the order relations: a "
                        "domain error each (S275)");

        return;
    }


    //==========================================================================
    // 9.  THE ROTATION
    //==========================================================================

    // d_tests_rotated
    //   rotate(q, v) of a vector text, the test's value.
    m::value
    d_tests_rotated(
        const char* _text
    )
    {
        return d_tests_value_of(_text);
    }

    // d_tests_rotation_in
    //   random q and v of Type, rotated by the language: quat.h's accurate
    // rotation, each element bit for bit.
    template<typename Type>
    bool
    d_tests_rotation_in()
    {
        bool all = true;

        // random rotations
        for (int trial = 0; trial < 300; ++trial)
        {
            const m::quaternion<Type> q = d_tests_random_quaternion<Type>();
            m::linalg::vector<Type, 3> v;
            std::vector<m::value>      elements;

            for (std::size_t i = 0; i < 3; ++i)
            {
                v[i] = d_tests_random_real<Type>();
                elements.push_back(m::value(v[i]));
            }

            const m::linalg::vector<Type, 3> want = rotate(q, v);
            const m::expr_result<m::value>   got  =
                d_tests_folded("rotate(q, v)",
                               m::at("q", m::expr_quaternion_of(q))
                                   .at("v",
                                       m::expr_vector(elements).value()));
            std::vector<m::value>            expected;

            for (std::size_t i = 0; i < 3; ++i)
            {
                expected.push_back(m::value(want[i]));
            }

            all = ( (all) &&
                    (got.is_ok()) &&
                    (d_tests_row_is(got.value(), 1, 3, expected)) );

            // the bits, a zero's sign among them
            for (std::size_t i = 0; ( (all) && (i < 3) ); ++i)
            {
                Type element = Type(0);

                all = ( (m::expr_element(got.value(), 0, i).value().to(
                             element)) &&
                        (d_tests_same_bits(element, want[i])) );
            }
        }

        return all;
    }

    // d_tests_rotation_exact
    //   300 random exact rotations, q of whole parts below 4 and not 0, v of
    // rationals of terms below 10 and denominators to 3, rotated by the
    // language: each element exactly the test's own q v q^-1, Hamilton's
    // product and the quotient on the right in rationals, so that every
    // entry of q's matrix is at work.
    bool
    d_tests_rotation_exact()
    {
        const d_tests_q zero = { 0, 1 };
        bool            all  = true;

        for (int trial = 0; ( (all) && (trial < 300) ); ++trial)
        {
            d_tests_h q;
            d_tests_h v;
            d_tests_h want;

            q.w = d_tests_q_of(
                static_cast<long long>(d_tests_random() % 7u) - 3, 1);
            q.x = d_tests_q_of(
                static_cast<long long>(d_tests_random() % 7u) - 3, 1);
            q.y = d_tests_q_of(
                static_cast<long long>(d_tests_random() % 7u) - 3, 1);
            q.z = d_tests_q_of(
                static_cast<long long>(d_tests_random() % 7u) - 3, 1);

            // no rotation by 0
            if ( (q.w.n == 0) &&
                 (q.x.n == 0) &&
                 (q.y.n == 0) &&
                 (q.z.n == 0) )
            {
                q.w = d_tests_q_of(1, 1);
            }

            v.w = zero;

            // each of the vector's three, a rational
            for (d_tests_q* part : { &v.x, &v.y, &v.z })
            {
                *part = d_tests_q_of(
                    static_cast<long long>(d_tests_random() % 19u) - 9,
                    static_cast<long long>(d_tests_random() % 3u) + 1);
            }

            all = d_tests_h_op(D_MATH_VALUE_OP_DIV,
                               d_tests_h_product(q, v), q, want);

            const std::vector<m::value> elements =
                { d_tests_q_value(v.x), d_tests_q_value(v.y),
                  d_tests_q_value(v.z) };
            const m::expr_result<m::value> got =
                d_tests_folded("rotate(q, v)",
                               m::at("q", d_tests_h_value(q))
                                   .at("v",
                                       m::expr_vector(elements).value()));

            all = ( (all) &&
                    (want.w.n == 0) &&
                    (got.is_ok()) &&
                    (d_tests_row_is(got.value(), 1, 3,
                                    { d_tests_q_value(want.x),
                                      d_tests_q_value(want.y),
                                      d_tests_q_value(want.z) })) );
        }

        return all;
    }

    // d_tests_rotation
    //   exact rotations, of rows and columns; dense ones; the errors.
    void
    d_tests_rotation()
    {
        D_TESTS_EXPECT( (d_tests_row_is(d_tests_rotated(
                                            "rotate(1 + km, [1, 0, 0])"),
                                        1, 3,
                                        { m::value(0), m::value(1),
                                          m::value(0) })) &&
                        (d_tests_row_is(d_tests_rotated(
                                            "rotate(1 + km, [1; 0; 0])"),
                                        3, 1,
                                        { m::value(0), m::value(1),
                                          m::value(0) })) &&
                        (d_tests_row_is(d_tests_rotated(
                                            "rotate(im, [0, 1, 0])"),
                                        1, 3,
                                        { m::value(0), m::value(-1),
                                          m::value(0) })) &&
                        (d_tests_row_is(d_tests_rotated(
                                            "rotate(1 + im, [0, 1, 0])"),
                                        1, 3,
                                        { m::value(0), m::value(0),
                                          m::value(1) })) &&
                        (d_tests_row_is(d_tests_rotated(
                                     "rotate(1 + im + jm + km, [1, 0, 0])"),
                                        1, 3,
                                        { m::value(0), m::value(1),
                                          m::value(0) })) &&
                        (d_tests_row_is(d_tests_rotated(
                                     "rotate(1 + im + jm + km, [1, 2, 3])"),
                                        1, 3,
                                        { m::value(3), m::value(1),
                                          m::value(2) })) &&
                        (d_tests_row_is(d_tests_rotated(
                                            "rotate(1 + km, [0, 1, 0])"),
                                        1, 3,
                                        { m::value(-1), m::value(0),
                                          m::value(0) })) &&
                        (d_tests_row_is(d_tests_rotated(
                                            "rotate(2 + 2km, [1/2, 0, 3])"),
                                        1, 3,
                                        { m::value(0), m::value::ratio(1, 2),
                                          m::value(3) })) &&
                        (d_tests_row_is(d_tests_rotated(
                                            "rotate(2, [1, 2, 3])"),
                                        1, 3,
                                        { m::value(1), m::value(2),
                                          m::value(3) })),
                        "exact rotations, q v q^-1, of a row and a column, "
                        "of its shape: about km, im and a complex number's "
                        "axis, 120 degrees about (1, 1, 1), of any q not 0, "
                        "a real one rotating nothing");
        D_TESTS_EXPECT(d_tests_rotation_exact(),
                       "300 random exact rotations: each element the "
                       "test's own q v q^-1 in rationals");
        D_TESTS_EXPECT( (d_tests_rotation_in<float>()) &&
                        (d_tests_rotation_in<double>()) &&
                        (d_tests_rotation_in<long double>()),
                        "300 random rotations in each type: quat.h's "
                        "accurate rotation (S360), each element bit for bit");
        D_TESTS_EXPECT( (d_tests_domain("rotate(0, [1, 2, 3])")) &&
                        (d_tests_domain("rotate(jm, [1, 2])")) &&
                        (d_tests_domain("rotate(jm, [1, im, 2])")) &&
                        (d_tests_domain("rotate(jm, [1, 2, 3; 4, 5, 6])")) &&
                        (d_tests_domain("rotate([1, 2, 3], [1, 2, 3])")) &&
                        (d_tests_domain("rotate(true, [1, 2, 3])")) &&
                        (d_tests_domain("rotate(jm, 1)")) &&
                        (d_tests_is_error(d_tests_value_of(
                                              "rotate(1 / 0, [1, 2, 3])"),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(d_tests_value_of(
                                              "rotate(jm, [1, 2, 1 / 0])"),
                                          D_MATH_VALUE_ERROR_POLE)),
                        "an exact 0 names no rotation; a vector of other "
                        "than three real numbers, a matrix or a boolean for "
                        "q, a domain error; an error passes");

        return;
    }


    //==========================================================================
    // 10. THE LANGUAGE
    //==========================================================================

    // d_tests_language_units
    //   jm and km, their products in order, quotients on the right.
    void
    d_tests_language_units()
    {
        const m::value i = m::expr_complex(m::value(0), m::value(1));

        D_TESTS_EXPECT( (d_tests_is("jm", d_tests_exact(0, 0, 1, 0))) &&
                        (d_tests_is("km", d_tests_exact(0, 0, 0, 1))) &&
                        (d_tests_is("im * jm", d_tests_exact(0, 0, 0, 1))) &&
                        (d_tests_is("jm * im", d_tests_exact(0, 0, 0, -1))) &&
                        (d_tests_is("jm km", i)) &&
                        (m::expr_is_complex(d_tests_value_of("jm km"))) &&
                        (d_tests_is("km jm", -i)) &&
                        (d_tests_is("km im", d_tests_exact(0, 0, 1, 0))) &&
                        (d_tests_is("im km", d_tests_exact(0, 0, -1, 0))) &&
                        (d_tests_is("im jm km", m::value(-1))) &&
                        (d_tests_is("km * km", m::value(-1))),
                        "jm and km the exact units; Hamilton's products in "
                        "their order, jm km im, a complex number, and km jm "
                        "-im; im jm km = -1 (S275, S277)");
        D_TESTS_EXPECT( (d_tests_is("(1 + 2jm)(3 - km)",
                                    d_tests_exact(3, -2, 6, -1))) &&
                        (d_tests_is("(1 + im) / jm",
                                    d_tests_exact(0, 0, -1, -1))) &&
                        (d_tests_is("inv(jm) (1 + im)",
                                    d_tests_exact(0, 0, -1, 1))) &&
                        (d_tests_is("jm / km", -i)) &&
                        (d_tests_is("(3 + 4jm) / (3 + 4jm)", m::value(1))) &&
                        (d_tests_is("2jm - jm - jm", m::value(0))) &&
                        (d_tests_is("1/2 + 3/4km",
                                    m::expr_quaternion(
                                        m::value::ratio(1, 2), m::value(0),
                                        m::value(0),
                                        m::value::ratio(3, 4)))),
                        "(1 + 2jm)(3 - km) = 3 - 2im + 6jm - km; p / q = p "
                        "inv(q), the quotient on the right, and inv(q) p "
                        "written so (S277); a quaternion coming to a real "
                        "number real");

        return;
    }

    // d_tests_language_functions
    //   quat, parts, real, conj, the norm, the inverse, the sign; the
    // relations; the arities.
    void
    d_tests_language_functions()
    {
        const m::value half = m::value::ratio(1, 2);
        std::vector<m::value> four;

        four.push_back(m::value(1));
        four.push_back(m::value(-2));
        four.push_back(m::value(3));
        four.push_back(m::value(4));

        D_TESTS_EXPECT( (d_tests_is("quat(1, -2, 3, 4)",
                                    d_tests_exact(1, -2, 3, 4))) &&
                        (d_tests_is("quat(1, 2, 0, 0)",
                                    m::expr_complex(m::value(1),
                                                    m::value(2)))) &&
                        (d_tests_is("quat(5, 0, 0, 0)", m::value(5))) &&
                        (d_tests_is_dense(d_tests_value_of(
                                              "quat(x, 0, 0, 0)",
                                              m::value(1.5)),
                                          d_tests_quat(1.5, 0.0, 0.0, 0.0)))
                        && (d_tests_domain("quat(im, 0, 0, 0)")) &&
                        (d_tests_domain("quat(true, 0, 1, 0)")) &&
                        (d_tests_is_error(d_tests_value_of(
                                              "quat(1 / 0, 0, 1, 0)"),
                                          D_MATH_VALUE_ERROR_POLE)),
                        "quat(w, x, y, z): exact, complex or real where the "
                        "parts come to it; of a double kept whole; a complex "
                        "part or a boolean a domain error, an error passing");
        D_TESTS_EXPECT( (d_tests_row_is(d_tests_value_of(
                                            "parts(quat(1, -2, 3, 4))"),
                                        1, 4, four)) &&
                        (d_tests_row_is(d_tests_value_of("parts(3)"), 1, 4,
                                        { m::value(3), m::value(0),
                                          m::value(0), m::value(0) })) &&
                        (d_tests_is("real(1 + 2jm)", m::value(1))) &&
                        (d_tests_is("conj(1 + 2jm)",
                                    d_tests_exact(1, 0, -2, 0))) &&
                        (d_tests_is("(1 + 2im + 3jm)'",
                                    d_tests_exact(1, -2, -3, 0))) &&
                        (d_tests_is("|1 + 2im + 2jm + 4km|", m::value(5))) &&
                        (d_tests_is("abs(2km)", m::value(2))) &&
                        (d_tests_is("inv(1 + jm)",
                                    m::expr_quaternion(half, m::value(0),
                                                       -half,
                                                       m::value(0)))) &&
                        (d_tests_is("sign(1 + 2im + 2jm + 4km)",
                                    m::expr_quaternion(
                                        m::value::ratio(1, 5),
                                        m::value::ratio(2, 5),
                                        m::value::ratio(2, 5),
                                        m::value::ratio(4, 5)))),
                        "parts, real, conj and q' the conjugate, the norm by "
                        "abs and the bars, inv and sign, exact");

        const m::value yes(true);
        const m::value no(false);

        D_TESTS_EXPECT( (d_tests_value_of("jm = jm") == yes) &&
                        (d_tests_value_of("jm != km") == yes) &&
                        (d_tests_value_of("jm = 1") == no) &&
                        (d_tests_value_of("quat(1, 2, 3, 4) = "
                                          "1 + 2im + 3jm + 4km") == yes) &&
                        (d_tests_value_of("jm km = im") == yes) &&
                        (d_tests_fails("quat(1, 2, 3)",
                                       m::expr_status::wrong_arity)) &&
                        (d_tests_fails("parts(1, 2)",
                                       m::expr_status::wrong_arity)) &&
                        (d_tests_fails("rotate(jm)",
                                       m::expr_status::wrong_arity)),
                        "= and != by the parts; quat of four arguments, "
                        "parts of one and rotate of two, wrong_arity "
                        "otherwise");

        return;
    }

    // d_tests_real_results
    //   the context's real_results: quat apart, as complex is, and every
    // quaternion operand's answer its own.
    void
    d_tests_real_results()
    {
        const m::expr_context real(true);
        const m::expr_bindings none;
        const m::expr_bindings at =
            m::at("x", d_tests_exact(1, 0, 1, 0));

        const m::expr_result<m::value> made =
            d_tests_folded("quat(1, 2, 3, 4)", none, real);
        const m::expr_result<m::value> unit =
            d_tests_folded("jm + 1", none, real);
        const m::expr_result<m::value> root =
            d_tests_folded("sqrt(x)", at, real);
        const m::expr_result<m::value> neg  =
            d_tests_folded("sqrt(-4)", none, real);

        D_TESTS_EXPECT( (made.is_ok()) &&
                        (made.value() == d_tests_exact(1, 2, 3, 4)) &&
                        (unit.is_ok()) &&
                        (unit.value() == d_tests_exact(1, 0, 1, 0)) &&
                        (root.is_ok()) &&
                        (m::expr_is_quaternion(root.value())) &&
                        (neg.is_ok()) &&
                        (d_tests_is_error(neg.value(),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "real_results: quat(w, x, y, z) apart, as "
                        "complex(x, y) is, and a quaternion operand's "
                        "answer its own; sqrt(-4) still a domain error");

        return;
    }

    // d_tests_matrix_nodes
    //   what step 24's third part made values (S369): linear algebra's
    // functions of a quaternion (S390), a literal holding one, a product or
    // a quotient of a matrix and one, in its order; what stays a domain
    // error. t_expr_quaternion_matrix_face tests matrices of them whole.
    void
    d_tests_matrix_nodes()
    {
        const m::value j  = d_tests_exact(0, 0, 1, 0);
        const m::value k  = d_tests_exact(0, 0, 0, 1);
        const m::value mj = d_tests_exact(0, 0, -1, 0);

        D_TESTS_EXPECT( (d_tests_domain("det(jm)")) &&
                        (d_tests_is("trace(jm)", j)) &&
                        (d_tests_is("norm(jm)", m::value(1))) &&
                        (d_tests_is("eig(jm)", d_tests_value_of("im"))) &&
                        (d_tests_is("transpose(jm)", j)) &&
                        (d_tests_is("dot(jm, 1)", mj)) &&
                        (d_tests_domain("cross(1, km)")) &&
                        (d_tests_is("solve(jm, 2)",
                                    d_tests_exact(0, 0, -2, 0))) &&
                        (d_tests_is("solve(jm, km)",
                                    d_tests_value_of("-im"))),
                        "det of a quaternion a domain error (S277); trace "
                        "q, norm |q|, eig the standard eigenvalue, "
                        "transpose q, dot conj(p) q, solve q^-1 B, on the "
                        "left (S390); cross of numbers none");
        D_TESTS_EXPECT( (m::expr_is_matrix(d_tests_value_of("[1, jm]"))) &&
                        (m::expr_is_matrix(d_tests_value_of("[1; km]"))) &&
                        (d_tests_value_of("[1, 2] * jm") ==
                             d_tests_value_of("[jm, 2jm]")) &&
                        (d_tests_value_of("jm * [1, im]") ==
                             d_tests_value_of("[jm, -km]")) &&
                        (d_tests_value_of("[1, im] * jm") ==
                             d_tests_value_of("[jm, km]")) &&
                        (d_tests_value_of("[1, 2] / jm") ==
                             d_tests_value_of("[-jm, -2jm]")) &&
                        (d_tests_domain("[1, 2] + jm")) &&
                        (d_tests_domain("jm - [1, 2]")) &&
                        (d_tests_is("inv(jm)", mj)),
                        "a literal holding a quaternion a matrix (S386); a "
                        "product of a matrix and one in its order, A / q "
                        "A inv(q); a sum a domain error, as a matrix plus a "
                        "number is (S246); inv the inverse");

        // the matrix kind and expr_matrix, asked directly
        const m::value vector = m::expr_vector(std::vector<m::value>(
                                    2, m::value(1))).value();
        const m::value zeros  = m::expr_vector(std::vector<m::value>(
                                    2, m::value(0))).value();
        std::vector<m::value> elements(2, m::value(1));

        elements[1] = j;

        const m::expr_result<m::value> made = m::expr_matrix(1, 2, elements);

        D_TESTS_EXPECT( ((vector * j) == m::expr_vector({ j, j }).value()) &&
                        ((j * vector) == m::expr_vector({ j, j }).value()) &&
                        ((zeros * k) == zeros) &&
                        ((zeros / k) == zeros) &&
                        (made.is_ok()) &&
                        (m::expr_element(made.value(), 0, 1).value() == j),
                        "the matrix kind's arithmetic takes a quaternion, "
                        "and expr_matrix an element that is one");

        return;
    }


    //==========================================================================
    // 11. THE FOLDS IN A TYPE
    //==========================================================================

    // d_tests_is_nan
    //   whether a number of a type is a NaN.
    template<typename Type>
    bool
    d_tests_is_nan(
        Type _x
    )
    {
        return (_x != _x);
    }

    // d_tests_in_real_types
    //   jm and km NaNs; quat w where the other parts are 0; parts and
    // rotate NaNs; a bound quaternion its w where it is real.
    void
    d_tests_in_real_types()
    {
        const m::value real_q =
            m::expr_quaternion_of(d_tests_quat(2.5, -0.0, 0.0, 0.0));
        const m::value full_q =
            m::expr_quaternion_of(d_tests_quat(2.5, 0.0, 1.0, 0.0));

        D_TESTS_EXPECT( (d_tests_is_nan(d_tests_in<double>("jm"))) &&
                        (d_tests_is_nan(d_tests_in<float>("2 * km"))) &&
                        (d_tests_is_nan(d_tests_in<long double>("jm * km")))
                        && (d_tests_in<double>("quat(2, 0, 0, 0)") == 2.0) &&
                        (d_tests_in<float>("quat(2, -0, 0, 0)") == 2.0f) &&
                        (d_tests_is_nan(d_tests_in<double>(
                             "quat(2, 1, 0, 0)"))) &&
                        (d_tests_is_nan(d_tests_in<long double>(
                             "quat(2, 0, 0, 1)"))) &&
                        (d_tests_is_nan(d_tests_in<double>("parts(2)"))) &&
                        (d_tests_is_nan(d_tests_in<double>("rotate(1, 2)"))),
                        "in float, double and long double: jm and km NaNs; "
                        "quat w where x, y and z are 0, a NaN otherwise; "
                        "parts and rotate NaNs, as a type holds no vector");
        D_TESTS_EXPECT( (d_tests_in<double>("x + 1", real_q) == 3.5) &&
                        (d_tests_is_nan(d_tests_in<double>("x + 1",
                                                           full_q))),
                        "a bound quaternion its w's nearest where its other "
                        "parts are 0, a NaN otherwise");

        return;
    }

    // d_tests_in_complex_types
    //   in math::complex<T>: jm and km NaNs in both parts; quat w + xi
    // where y and z are 0; a bound quaternion's nearest complex number.
    void
    d_tests_in_complex_types()
    {
        typedef m::complex<double> cd;

        const m::value plane_q =
            m::expr_quaternion_of(d_tests_quat(1.5, 2.5, 0.0, -0.0));
        const m::value full_q =
            m::expr_quaternion_of(d_tests_quat(1.5, 2.5, 1.0, 0.0));
        const cd       jm  = d_tests_in_complex<double>("jm");
        const cd       q4  = d_tests_in_complex<double>("quat(1, 2, 3, 0)");
        const cd       qi  = d_tests_in_complex<double>("quat(im, 0, 0, 0)");
        const cd       q2  = d_tests_in_complex<double>("quat(1, 2, 0, 0)");
        const cd       bp  = d_tests_in_complex<double>("x", plane_q);
        const cd       bf  = d_tests_in_complex<double>("x", full_q);
        const m::complex<float> kf = d_tests_in_complex<float>("km");

        D_TESTS_EXPECT( (d_tests_is_nan(jm.real())) &&
                        (d_tests_is_nan(jm.imag())) &&
                        (d_tests_is_nan(kf.real())) &&
                        (d_tests_is_nan(kf.imag())) &&
                        (q2.real() == 1.0) &&
                        (q2.imag() == 2.0) &&
                        (d_tests_is_nan(q4.real())) &&
                        (d_tests_is_nan(q4.imag())) &&
                        (d_tests_is_nan(qi.real())) &&
                        (d_tests_is_nan(qi.imag())),
                        "in math::complex<T>: jm and km NaNs in both parts; "
                        "quat of real arguments w + xi where y and z are 0, "
                        "and a NaN in both parts otherwise");
        D_TESTS_EXPECT( (bp.real() == 1.5) &&
                        (bp.imag() == 2.5) &&
                        (d_tests_is_nan(bf.real())) &&
                        (d_tests_is_nan(bf.imag())),
                        "a bound quaternion the complex number of its w and "
                        "x where its jm and km parts are 0, a NaN otherwise");

        return;
    }


    //==========================================================================
    // 12. THE RENDERER
    //==========================================================================

    // d_tests_rendering
    //   a quaternion as a sum, in brackets; read back.
    void
    d_tests_rendering()
    {
        const dj_math_imax least = std::numeric_limits<dj_math_imax>::min();
        const double       nan   = std::numeric_limits<double>::quiet_NaN();
        const double       inf   = std::numeric_limits<double>::infinity();
        const std::string  min_text = m::internal::expr_signed_text(least);
        const m::value     low   = m::expr_quaternion(m::value(3),
                                                      m::value(least),
                                                      m::value(1),
                                                      m::value::ratio(least,
                                                                      3));

        D_TESTS_EXPECT( (d_tests_rendered(d_tests_exact(1, 2, -3, 4)) ==
                             "(1 + 2 * im - 3 * jm + 4 * km)") &&
                        (d_tests_rendered(m::expr_quaternion(
                             m::value::ratio(1, 2), m::value(0),
                             m::value(0), m::value::ratio(-3, 4))) ==
                             "(1/2 + 0 * im + 0 * jm - 3/4 * km)") &&
                        (d_tests_rendered(m::expr_quaternion_of(
                             d_tests_quat(1.5, -0.0, 0.1, -2.5))) ==
                             "(1.5 - 0 * im + 0.1 * jm - 2.5 * km)") &&
                        (d_tests_rendered(m::expr_quaternion_of(
                             d_tests_quat(nan, -inf, inf, 0.0))) ==
                             "(nan - inf * im + inf * jm + 0 * km)") &&
                        (d_tests_rendered(low) ==
                             "(3 + (" + min_text + ") * im + 1 * jm + (" +
                             min_text + ")/3 * km)"),
                        "a sum in brackets: exact parts, floating-point ones "
                        "shortest, -0, nan and inf, the least integer added "
                        "as it is");

        int agreed = 0;

        // random exact ones
        for (int trial = 0; trial < 500; ++trial)
        {
            agreed += ( d_tests_reads_back(d_tests_h_value(
                            d_tests_random_h())) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (d_tests_reads_back(d_tests_exact(1, 2, -3, 4))) &&
                        (d_tests_reads_back(low)) &&
                        (agreed == 500),
                        "each exact one read back, implied multiplication "
                        "off, as itself; 500 random ones, real and complex "
                        "ones among them");

        // in a term: beside other operands
        const m::expr_term product =
            m::expr_application(m::expr_op_kind::mul,
                                m::expr_number(d_tests_exact(0, 0, 1, 1)),
                                m::expr_variable("x"));
        const m::expr_term power =
            m::expr_application(m::expr_op_kind::pow,
                                m::expr_number(d_tests_exact(1, 0, 1, 0)),
                                m::expr_number(m::value(2)));

        D_TESTS_EXPECT( (m::expr_render(product) ==
                             "(0 + 0 * im + 1 * jm + 1 * km) * x") &&
                        (m::expr_render(power) ==
                             "(1 + 0 * im + 1 * jm + 0 * km) ^ 2") &&
                        (m::expr_evaluate(d_tests_parsed(
                                              m::expr_render(power).c_str(),
                                              false),
                                          m::expr_bindings(),
                                          m::expr_context()).value() ==
                             d_tests_exact(0, 0, 2, 0)) &&
                        (m::expr_evaluate(m::expr_simplify(power),
                                          m::expr_bindings(),
                                          m::expr_context()).value() ==
                             d_tests_exact(0, 0, 2, 0)),
                        "in a term: bracketed, read back to its value, and "
                        "simplified to it");

        return;
    }


    //==========================================================================
    // 13. SIMPLIFICATION
    //==========================================================================

    // d_tests_simplified
    //   a text simplified and rendered.
    std::string
    d_tests_simplified(
        const char* _text
    )
    {
        return m::expr_render(m::expr_simplify(d_tests_parsed(_text)));
    }

    // d_tests_keeps_value
    //   whether a text and its simplification have one value, exactly, with
    // x bound to _x.
    bool
    d_tests_keeps_value(
        const char*     _text,
        const m::value& _x
    )
    {
        const m::expr_bindings         at  = m::at("x", _x);
        const m::expr_result<m::value> was =
            m::expr_evaluate(d_tests_parsed(_text), at, m::expr_context());
        const m::expr_result<m::value> now =
            m::expr_evaluate(m::expr_simplify(d_tests_parsed(_text)),
                             at,
                             m::expr_context());

        return ( (was.is_ok()) &&
                 (now.is_ok()) &&
                 (was.value().compare(now.value()) == D_MATH_VALUE_EQUAL) );
    }

    // d_tests_simplification
    //   a quaternion's factors in their order, a complex number's beside
    // one; real numbers, and variables, commuting (S251, S277).
    void
    d_tests_simplification()
    {
        const m::expr_term j  = m::expr_number(d_tests_exact(0, 0, 1, 0));
        const m::expr_term k  = m::expr_number(d_tests_exact(0, 0, 0, 1));
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term jx =
            m::expr_application(m::expr_op_kind::mul,
                m::expr_application(m::expr_op_kind::mul, j, x), k);
        const m::expr_term kx =
            m::expr_application(m::expr_op_kind::mul,
                m::expr_application(m::expr_op_kind::mul, k, x), j);
        const m::expr_term both =
            m::expr_application(m::expr_op_kind::add, jx, kx);
        const m::expr_term sum = m::expr_simplify(both);

        D_TESTS_EXPECT( (d_tests_simplified("jm * km + km * jm") ==
                             "jm * km + km * jm") &&
                        (d_tests_simplified("im * jm - jm * im") ==
                             "im * jm - jm * im") &&
                        (d_tests_simplified("im * x * jm + jm * x * im") ==
                             "im * x * jm + jm * x * im") &&
                        (d_tests_simplified("jm * km + jm * km") ==
                             "2 * jm * km") &&
                        (d_tests_simplified("km * x * jm + 2 * km * x * jm")
                             == "3 * km * x * jm") &&
                        (d_tests_simplified("3 * jm - jm * 3") == "0") &&
                        (d_tests_simplified("2 * jm * 3") == "6 * jm") &&
                        (d_tests_simplified("im * x + x * im") ==
                             "2 * im * x") &&
                        (d_tests_simplified("x * jm + jm * x") ==
                             "2 * x * jm"),
                        "jm km + km jm stays, and im jm - jm im, im beside "
                        "jm keeping its place; like terms in one order "
                        "collected; real numbers commute, and im where no "
                        "quaternion stands; a variable taken for a real "
                        "number (S251, S277)");
        D_TESTS_EXPECT( (m::expr_render(sum) == m::expr_render(both)) &&
                        (m::expr_render(both) ==
                             "(0 + 0 * im + 1 * jm + 0 * km) * x * (0 + 0 * "
                             "im + 0 * jm + 1 * km) + (0 + 0 * im + 0 * jm + "
                             "1 * km) * x * (0 + 0 * im + 1 * jm + 0 * km)") &&
                        (d_tests_keeps_value("jm * km + km * jm",
                                             m::value(2))) &&
                        (d_tests_keeps_value("(1 + jm) * x * (1 - km) + "
                                             "2 * (1 + jm) * x * (1 - km)",
                                             m::value::ratio(1, 3))) &&
                        (d_tests_keeps_value("im * x * jm + jm * x * im",
                                             m::value(5))),
                        "quaternions a fold made keep their places too, "
                        "never taken into a coefficient; at real bindings "
                        "each value kept");

        return;
    }


    //==========================================================================
    // 14. EVERY STORAGE AND EVERY REFUSAL
    //==========================================================================

    // the test's boxed kind "lifted": a long double of rank 600, a number
    // not exact, as bignum's floating-point kind will be; each value's box
    // the long double it stands for, held by the test. Its operations give
    // inline long doubles, and its arithmetic on g_failing is a range error.
    const long double g_failing = 99.5L;

    long double
    d_tests_lifted_in(
        dj_math_value _x
    )
    {
        long double x = 0.0L;

        // the kind's own, or another number's nearest
        if ( (_x.kind == D_MATH_VALUE_BOXED) &&
             (_x.type->rank == 600) )
        {
            return *static_cast<const long double*>(_x.box);
        }

        static_cast<void>(d_math_value_to_ld(&x, _x));

        return x;
    }

    void
    d_tests_lifted_hold(
        void* _box
    )
    {
        static_cast<void>(_box);

        return;
    }

    dj_math_value
    d_tests_lifted_unary(
        dj_math_value_op _op,
        dj_math_value    _x
    )
    {
        const long double x = d_tests_lifted_in(_x);

        return d_math_value_from_ld( (_op == D_MATH_VALUE_OP_NEG)
                                         ? -x
                                         : ( (x < 0.0L) ? -x : x ) );
    }

    dj_math_value
    d_tests_lifted_binary(
        dj_math_value_op _op,
        dj_math_value    _a,
        dj_math_value    _b
    )
    {
        const long double a = d_tests_lifted_in(_a);
        const long double b = d_tests_lifted_in(_b);

        // the failing value: a range error
        if ( (a == g_failing) ||
             (b == g_failing) )
        {
            return d_math_value_from_error(D_MATH_VALUE_ERROR_RANGE);
        }

        return d_math_value_from_ld( (_op == D_MATH_VALUE_OP_ADD) ? (a + b)
                                   : (_op == D_MATH_VALUE_OP_SUB) ? (a - b)
                                   : (_op == D_MATH_VALUE_OP_MUL) ? (a * b)
                                                                  : (a / b) );
    }

    dj_math_value_order
    d_tests_lifted_cmp(
        dj_math_value _a,
        dj_math_value _b
    )
    {
        const long double a = d_tests_lifted_in(_a);
        const long double b = d_tests_lifted_in(_b);

        return ( (a < b) ? D_MATH_VALUE_LESS
               : (a > b) ? D_MATH_VALUE_GREATER
               : (a == b) ? D_MATH_VALUE_EQUAL
                          : D_MATH_VALUE_UNORDERED );
    }

    bool
    d_tests_lifted_convert(
        dj_math_value*     _out,
        int*               _direction,
        dj_math_value      _x,
        dj_math_value_kind _to
    )
    {
        const long double x = d_tests_lifted_in(_x);

        // the exact kinds: none
        if ( (_to != D_MATH_VALUE_FLOAT) &&
             (_to != D_MATH_VALUE_DOUBLE) &&
             (_to != D_MATH_VALUE_LONG_DOUBLE) )
        {
            return false;
        }

        *_out       = ( (_to == D_MATH_VALUE_FLOAT)
                            ? d_math_value_from_f(static_cast<float>(x))
                        : (_to == D_MATH_VALUE_DOUBLE)
                            ? d_math_value_from_d(static_cast<double>(x))
                            : d_math_value_from_ld(x) );
        *_direction = 0;

        return true;
    }

    std::size_t
    d_tests_lifted_text(
        char*         _buffer,
        std::size_t   _size,
        dj_math_value _x
    )
    {
        return static_cast<std::size_t>(
            std::snprintf(_buffer,
                          _size,
                          "lifted %.2Lf",
                          d_tests_lifted_in(_x)));
    }

    const dj_math_boxed_kind g_lifted_kind = { "lifted",
                                               600,
                                               D_MATH_BOXED_NUMBER,
                                               &d_tests_lifted_hold,
                                               &d_tests_lifted_hold,
                                               &d_tests_lifted_unary,
                                               &d_tests_lifted_binary,
                                               &d_tests_lifted_cmp,
                                               &d_tests_lifted_convert,
                                               &d_tests_lifted_text,
                                               D_NULLPTR };

    // d_tests_lifted
    //   a value of the kind, standing for *_x.
    m::value
    d_tests_lifted(
        const long double* _x
    )
    {
        dj_math_value boxed = d_math_value_none();

        boxed.kind = D_MATH_VALUE_BOXED;
        boxed.type = &g_lifted_kind;
        boxed.box  = const_cast<long double*>(_x);

        return m::value::adopt(boxed);
    }

    // d_tests_storages
    //   parts of a boxed kind that is not exact; storage refused; and the
    // entries asked what the protocol never asks them.
    void
    d_tests_storages()
    {
        static const long double one_half = 1.5L;
        const m::value lifted = m::expr_quaternion(d_tests_lifted(&one_half),
                                                   m::value(0),
                                                   m::value(1),
                                                   m::value(0));
        const m::value failing =
            m::expr_quaternion(d_tests_lifted(&g_failing), m::value(0),
                               m::value(1), m::value(0));

        D_TESTS_EXPECT( (m::expr_is_quaternion(lifted)) &&
                        (d_tests_storage(lifted) ==
                             d_tests_storage_kind::values) &&
                        (!lifted.is_exact()) &&
                        (d_tests_text(lifted).find("lifted 1.50") == 0u) &&
                        (d_tests_is_dense(lifted * m::value(2),
                                          d_tests_quat(3.0L, 0.0L, 2.0L,
                                                       0.0L))) &&
                        (d_tests_is_error(failing * m::value(2),
                                          D_MATH_VALUE_ERROR_RANGE)),
                        "parts of a boxed kind that is not exact kept as "
                        "values, inexact; their arithmetic the kind's, a "
                        "part's error passing");

        const m::value made = m::expr_quaternion_of(d_tests_quat(1.0, 2.0,
                                                                 3.0, 4.0));

        g_refuse_storage = true;

        const m::value exact = d_tests_exact(1, 2, 3, 4);
        const m::value dense = m::expr_quaternion_of(d_tests_quat(1.0, 2.0,
                                                                  3.0, 4.0));
        const m::value prod  = made * made;

        g_refuse_storage = false;

        D_TESTS_EXPECT( (d_tests_is_error(exact,
                                          D_MATH_VALUE_ERROR_MEMORY)) &&
                        (d_tests_is_error(dense,
                                          D_MATH_VALUE_ERROR_MEMORY)) &&
                        (d_tests_is_error(prod,
                                          D_MATH_VALUE_ERROR_MEMORY)),
                        "storage refused: a memory error, made of exact or "
                        "dense parts, or by arithmetic");

        const dj_math_boxed_kind* kind = m::internal::expr_quaternion_kind();
        const m::value            q    = d_tests_exact(1, 2, 3, 4);
        dj_math_value             out  = d_math_value_none();
        int                       dir  = 9;
        const dj_math_value       sum  =
            kind->binary(D_MATH_VALUE_OP_ADD, q.c_value(),
                         m::value(true).c_value());

        D_TESTS_EXPECT( (sum.kind == D_MATH_VALUE_ERROR) &&
                        (kind->cmp(q.c_value(), m::value(true).c_value()) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (!kind->convert(&out, &dir, q.c_value(),
                                        D_MATH_VALUE_RATIONAL)) &&
                        (!kind->convert(&out, &dir, q.c_value(),
                                        D_MATH_VALUE_INTEGER)) &&
                        (dir == 9),
                        "the entries: a boolean operand a domain error and "
                        "unordered; no exact conversion");

        return;
    }

    // d_tests_every_path
    //   what no other check reaches: the conjugate, the inverse and the
    // sign of an error and of no number; an exact quotient whose numerator
    // fails; a dense real quaternion asked for an exact kind; exact roots
    // refused where |v| or s is irrational; a quaternion transposed, or
    // among a literal's blocks, below the fold; and simplification beside a
    // complex factor that is a term.
    void
    d_tests_every_path()
    {
        const m::value pole = m::value(D_MATH_VALUE_ERROR_POLE);
        const m::value no   = m::value(true);

        D_TESTS_EXPECT( (d_tests_is_error(m::expr_quaternion_conj(pole),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(m::expr_quaternion_inverse(pole),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(m::expr_quaternion_sign(pole),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(m::expr_quaternion_conj(no),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_quaternion_inverse(no),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "conj, inv and sign: an error passes; no number a "
                        "domain error");

        const m::value failing =
            m::expr_quaternion(d_tests_lifted(&g_failing), m::value(0),
                               m::value(1), m::value(0));
        const dj_math_boxed_kind* kind = m::internal::expr_quaternion_kind();
        const m::value            real =
            m::expr_quaternion_of(d_tests_quat(2.5, 0.0, 0.0, 0.0));
        dj_math_value             out  = d_math_value_none();
        int                       dir  = 9;

        D_TESTS_EXPECT( (d_tests_is_error(failing / d_tests_exact(1, 0, 1,
                                                                  0),
                                          D_MATH_VALUE_ERROR_RANGE)) &&
                        (!kind->convert(&out, &dir, real.c_value(),
                                        D_MATH_VALUE_RATIONAL)) &&
                        (dir == 9),
                        "a quotient whose numerator fails, the error; a "
                        "dense real quaternion to no exact kind");
        D_TESTS_EXPECT( (d_tests_is_dense(d_tests_value_of(
                                              "sqrt(1 + 2jm + 2km)"),
                                          sqrt(d_tests_quat(1.0L, 0.0L,
                                                            2.0L, 2.0L))))
                        && (d_tests_is_dense(d_tests_value_of(
                                                 "(1 + jm + km) ^ (1/3)"),
                                             pow(d_tests_quat(1.0L, 0.0L,
                                                              1.0L, 1.0L),
                                                 1.0L / 3.0L))),
                        "no exact root where s, sqrt((|q| + w) / 2), or |v| "
                        "is irrational: quat.h's, in long double");

        const m::value jm = d_tests_exact(0, 0, 1, 0);

        D_TESTS_EXPECT( (m::internal::expr_transpose_value(jm, false) == jm) &&
                        (m::internal::expr_transpose_value(jm, true) ==
                             d_tests_exact(0, 0, -1, 0)) &&
                        (m::expr_is_matrix(m::internal::expr_matrix_beside(
                             { m::value(1), jm }))),
                        "below the fold, a quaternion transposed itself, "
                        "its conjugate for ', and a literal's block (S386)");
        D_TESTS_EXPECT( (d_tests_simplified("(1 + im) * jm + jm * (1 + im)")
                             == "(1 + im) * jm + jm * (1 + im)") &&
                        (d_tests_keeps_value("(1 + im) * jm + jm * (1 + im)",
                                             m::value(1))) &&
                        (d_tests_simplified("complex(x, 2) * jm - jm * "
                                            "complex(x, 2)") ==
                             "complex(x, 2) * jm - jm * complex(x, 2)") &&
                        (d_tests_keeps_value("complex(x, 2) * jm - jm * "
                                             "complex(x, 2)",
                                             m::value(1))),
                        "a complex factor that is a term, 1 + im or "
                        "complex(x, 2), keeps its place beside a "
                        "quaternion");

        return;
    }


    //==========================================================================
    // 15. THE MODULE
    //==========================================================================

    // d_tests_module
    //   every part, from the module's floor.
    void
    d_tests_module()
    {
        d_tests_kind();
        d_tests_making();
        d_tests_exact_arithmetic();
        d_tests_exact_cases();
        d_tests_dense_arithmetic();
        d_tests_unary();
        d_tests_comparison();
        d_tests_conversion();
        d_tests_kind_text();
        d_tests_parts();
        d_tests_conj_inverse_sign();
        d_tests_every_type();
        d_tests_of_and_to();
        d_tests_functions();
        d_tests_function_domains();
        d_tests_rotation();
        d_tests_language_units();
        d_tests_language_functions();
        d_tests_real_results();
        d_tests_matrix_nodes();
        d_tests_in_real_types();
        d_tests_in_complex_types();
        d_tests_rendering();
        d_tests_simplification();
        d_tests_storages();
        d_tests_every_path();

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
    //   below the module's floor: the header read, and nothing more to
    // test, as it is empty here.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_HEADER_READ == 1),
                        "the header read");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    d_tests_module();

    std::printf("t_expr_quaternion_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
