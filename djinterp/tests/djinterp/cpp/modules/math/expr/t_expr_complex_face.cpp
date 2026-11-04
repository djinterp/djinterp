/*******************************************************************************
* djinterp [cpp/math]                                    t_expr_complex_face.cpp
*
* Tests for math/expr/complex.hpp, the language's complex numbers, and for
* what step 23's first part (3e) made of them in the language: the kind and
* the folds, the functions, the relations and the renderer.
*   - the kind: its table, its rank, a number exact or not value by value,
*     and its holders; the value layer's exactness asked of each value;
*   - making one: exact parts kept as values, a real number where they are
*     exact and the imaginary part is 0; floating-point parts dense in the
*     wider type, each its nearest there; a wide integer's parts; an error
*     passed, and what is no real part refused;
*   - its arithmetic: exact, against the test's own Gaussian rationals, on
*     random operands, real ones among them, and a quotient by an exact 0 a
*     pole; past the family G2's long double, flagged; dense, every part
*     cplx.h's bit for bit, through math::complex<T>, in float, double and
*     long double, a real operand taken as itself, so that a zero's sign
*     survives, and an exact operand its nearest in the other's type;
*   - -z and |z|: exact where the modulus is rational, the long double
*     nearest otherwise, and cplx.h's abs_c for dense parts;
*   - comparison by the parts, a real number's imaginary part 0, unordered
*     otherwise and for a NaN part; conversion where the imaginary part is
*     0, never to an exact number;
*   - the kind's text, and a buffer too short;
*   - expr_real, expr_imag, expr_conj and expr_arg of every kind of number,
*     the argument's special values; expr_complex_of and expr_complex_to;
*   - number_traits<value>'s root refusing a complex number; the matrix
*     kind refusing one until step 23's third part;
*   - the language: im the unit as a value and a NaN in each type; 3 + 4im
*     by implied multiplication and 3 + 4 * im without; complex, real, imag,
*     conj and arg as values and in each type; the modulus and the bars; z'
*     the conjugate; = and != by the parts and < <= > >= a domain error;
*     exact powers to a whole exponent; the functions of the real line's
*     order a domain error; and not_yet, with its message, for each node
*     step 23's third part computes, after an error and wrong_arity, and no
*     longer for what chunks 41 and 42 compute, exp2, log2, log10, log1p and
*     expm1 among them (t_expr_complex_function_face.cpp and
*     t_expr_complex_five_face.cpp test those values);
*   - the renderer: a complex number as a sum, in brackets, every kind of
*     part, the least integer among them, read back as the same number, on
*     random exact ones too, and in a term beside other operands;
*   - every storage through -z, |z|, conj, the text and the renderer; parts
*     of a boxed kind that is not exact, the test's own, kept as values, a
*     part's error passing; storage refused, by the test's own nothrow
*     operator new, a memory error; and the entries asked what the protocol
*     never asks them.
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/math/expr/t_expr_complex_face.cpp -o t_cplx
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_complex_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.31
*                                                            revised: 2026.11.02
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/complex.hpp"
// std
#include <cstdio>  // std::printf


// D_TESTS_HEADER_READ
//   1 where the header was read: at every level, as it defines its guard
// before its floor.
#if defined(DJINTERP_MATH_EXPR_COMPLEX_HPP)
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
#include <cmath>    // std::signbit, std::sqrt
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <new>      // std::nothrow_t, operator new
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../../inc/djinterp/c/modules/math/value_wide.h"         // a boxed kind
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/evaluate.hpp"  // the folds
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/function.hpp"  // the tables
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/matrix.hpp"    // matrices
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/parse.hpp"     // expr_parse
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/render.hpp"    // expr_render
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/simplify.hpp"  // expr_simplify
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/substitute.hpp"


namespace
{
    // g_refuse_storage
    //   whether the nothrow operator new below refuses, as a host's arena
    // may: the complex kind's boxes are taken by it.
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

    // d_tests_same_complex
    //   whether two math::complex<T> are the same, part by part.
    template<typename Type>
    bool
    d_tests_same_complex(
        const m::complex<Type>& _a,
        const m::complex<Type>& _b
    )
    {
        return ( (d_tests_same_bits(_a.real(), _b.real())) &&
                 (d_tests_same_bits(_a.imag(), _b.imag())) );
    }

    // d_tests_storage
    //   a complex value's storage, as the kind's box holds it.
    m::internal::expr_complex_storage
    d_tests_storage(
        const m::value& _z
    )
    {
        return m::internal::expr_complex_box_of(_z.c_value()).storage;
    }

    // d_tests_is_dense
    //   whether _z is a complex number of dense parts of Type, the same as
    // _expected, part by part.
    template<typename Type>
    bool
    d_tests_is_dense(
        const m::value&         _z,
        const m::complex<Type>& _expected
    )
    {
        m::complex<Type> held;

        return ( (m::expr_is_complex(_z)) &&
                 (d_tests_storage(_z) ==
                      m::internal::expr_complex_storage_for<Type>()) &&
                 (!_z.is_exact()) &&
                 (m::expr_complex_to(_z, held)) &&
                 (d_tests_same_complex(held, _expected)) );
    }

    // d_tests_text
    //   a value's text: a boxed one's by its kind's text entry.
    std::string
    d_tests_text(
        const m::value& _x
    )
    {
        const d_math_value& held = _x.c_value();

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
        const m::value&    _x,
        d_math_value_error _code
    )
    {
        d_math_value_error code = D_MATH_VALUE_ERROR_MEMORY;

        return ( (_x.kind() == D_MATH_VALUE_ERROR) &&
                 (_x.to(code)) &&
                 (code == _code) );
    }

    // d_tests_complex
    //   the exact complex number _a/_b + _c/_d im.
    m::value
    d_tests_complex(
        d_math_imax _a,
        d_math_imax _b,
        d_math_imax _c,
        d_math_imax _d
    )
    {
        return m::expr_complex(m::value::ratio(_a, _b),
                               m::value::ratio(_c, _d));
    }

    // d_tests_parsed
    //   the term of a text, by the syntax given, implied multiplication and
    // the bars on by default; a term made by default where it fails.
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

    // d_tests_value_of
    //   a text's value, with x bound to _x; none where the fold fails.
    m::value
    d_tests_value_of(
        const char*     _text,
        const m::value& _x = m::value(0)
    )
    {
        const m::expr_result<m::value> folded =
            m::expr_evaluate(d_tests_parsed(_text),
                             m::at("x", _x),
                             m::expr_context());

        return ( folded.is_ok() ? folded.value() : m::value() );
    }

    // d_tests_in
    //   a text's value in Type, x bound to _x; a quiet NaN of a marked sign
    // where the fold fails.
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

    // the test's own random numbers: a 32-bit linear congruential generator,
    // the same in every build
    unsigned long g_state = 31415926UL;

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


    //==========================================================================
    // 3.  THE TEST'S GAUSSIAN RATIONALS
    //==========================================================================
    // Exact complex numbers of small terms, computed in long long: the
    // reference for the kind's exact arithmetic.

    struct d_tests_q
    {
        long long n;
        long long d;
    };

    struct d_tests_gauss
    {
        d_tests_q re;
        d_tests_q im;
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
    d_tests_q_neg(
        d_tests_q _a
    )
    {
        return d_tests_q_of(-_a.n, _a.d);
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

    // d_tests_gauss_value
    //   the value of a Gaussian rational, made as the kind makes it.
    m::value
    d_tests_gauss_value(
        const d_tests_gauss& _z
    )
    {
        return m::expr_complex(
            m::value::ratio(static_cast<d_math_imax>(_z.re.n),
                            static_cast<d_math_imax>(_z.re.d)),
            m::value::ratio(static_cast<d_math_imax>(_z.im.n),
                            static_cast<d_math_imax>(_z.im.d)));
    }

    // d_tests_gauss_op
    //   _a _op _b, by the textbook formulas, exactly; false where _b is 0
    // for a quotient.
    bool
    d_tests_gauss_op(
        d_math_value_op      _op,
        const d_tests_gauss& _a,
        const d_tests_gauss& _b,
        d_tests_gauss&       _out
    )
    {
        const d_tests_q zero = { 0, 1 };

        // by operation
        switch (_op)
        {
            case D_MATH_VALUE_OP_ADD:
                _out.re = d_tests_q_add(_a.re, _b.re);
                _out.im = d_tests_q_add(_a.im, _b.im);

                return true;

            case D_MATH_VALUE_OP_SUB:
                _out.re = d_tests_q_add(_a.re, d_tests_q_neg(_b.re));
                _out.im = d_tests_q_add(_a.im, d_tests_q_neg(_b.im));

                return true;

            case D_MATH_VALUE_OP_MUL:
                _out.re = d_tests_q_add(d_tests_q_mul(_a.re, _b.re),
                                        d_tests_q_neg(d_tests_q_mul(_a.im,
                                                                    _b.im)));
                _out.im = d_tests_q_add(d_tests_q_mul(_a.re, _b.im),
                                        d_tests_q_mul(_a.im, _b.re));

                return true;

            case D_MATH_VALUE_OP_DIV:
            default:
                break;
        }

        const d_tests_q s = d_tests_q_add(d_tests_q_mul(_b.re, _b.re),
                                          d_tests_q_mul(_b.im, _b.im));

        // no quotient by 0
        if (s.n == 0)
        {
            return false;
        }

        _out.re = d_tests_q_div(d_tests_q_add(d_tests_q_mul(_a.re, _b.re),
                                              d_tests_q_mul(_a.im, _b.im)),
                                s);
        _out.im = d_tests_q_div(d_tests_q_add(d_tests_q_mul(_a.im, _b.re),
                                              d_tests_q_neg(
                                                  d_tests_q_mul(_a.re,
                                                                _b.im))),
                                s);
        static_cast<void>(zero);

        return true;
    }

    // d_tests_random_gauss
    //   a random Gaussian rational of terms below 30, its imaginary part 0
    // one time in four, so that real operands come too.
    d_tests_gauss
    d_tests_random_gauss()
    {
        d_tests_gauss z;

        z.re = d_tests_q_of(static_cast<long long>(d_tests_random() % 59u) -
                                29,
                            static_cast<long long>(d_tests_random() % 29u) +
                                1);
        z.im = d_tests_q_of(static_cast<long long>(d_tests_random() % 59u) -
                                29,
                            static_cast<long long>(d_tests_random() % 29u) +
                                1);

        // a real number, now and then
        if ((d_tests_random() % 4u) == 0u)
        {
            z.im = d_tests_q_of(0, 1);
        }

        return z;
    }

    // d_tests_apply
    //   _a _op _b, as values.
    m::value
    d_tests_apply(
        d_math_value_op _op,
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

    // d_tests_dense_apply
    //   _a _op _b, as math::complex<T> computes it, a real operand as T
    // where _real_a or _real_b says.
    template<typename Type>
    m::complex<Type>
    d_tests_dense_apply(
        d_math_value_op         _op,
        const m::complex<Type>& _a,
        bool                    _real_a,
        const m::complex<Type>& _b,
        bool                    _real_b
    )
    {
        const Type x = ( _real_a ? _a.real() : _b.real() );

        // by operation, and which operand is real
        switch (_op)
        {
            case D_MATH_VALUE_OP_ADD:
                return ( _real_a ? (x + _b) : _real_b ? (_a + x) : (_a + _b) );

            case D_MATH_VALUE_OP_SUB:
                return ( _real_a ? (x - _b) : _real_b ? (_a - x) : (_a - _b) );

            case D_MATH_VALUE_OP_MUL:
                return ( _real_a ? (x * _b) : _real_b ? (_a * x) : (_a * _b) );

            case D_MATH_VALUE_OP_DIV:
            default:
                break;
        }

        return ( _real_a ? (x / _b) : _real_b ? (_a / x) : (_a / _b) );
    }


    // the test's boxed kind "lifted": a long double of rank 600, a number
    // not exact, as bignum's floating-point kind will be; each value's box
    // the long double it stands for, held by the test. Its operations give
    // inline long doubles; it refuses to convert g_opaque, and its
    // arithmetic on g_failing is a range error.
    const long double g_opaque  = 42.5L;
    const long double g_failing = 99.5L;

    long double
    d_tests_lifted_in(
        d_math_value _x
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

    d_math_value
    d_tests_lifted_unary(
        d_math_value_op _op,
        d_math_value    _x
    )
    {
        const long double x = d_tests_lifted_in(_x);

        return d_math_value_from_ld( (_op == D_MATH_VALUE_OP_NEG)
                                         ? -x
                                         : ( (x < 0.0L) ? -x : x ) );
    }

    d_math_value
    d_tests_lifted_binary(
        d_math_value_op _op,
        d_math_value    _a,
        d_math_value    _b
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

    d_math_value_order
    d_tests_lifted_cmp(
        d_math_value _a,
        d_math_value _b
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
        d_math_value*     _out,
        int*              _direction,
        d_math_value      _x,
        d_math_value_kind _to
    )
    {
        const long double x = d_tests_lifted_in(_x);

        // the opaque value, and the exact kinds: none
        if ( (x == g_opaque) ||
             ( (_to != D_MATH_VALUE_FLOAT) &&
               (_to != D_MATH_VALUE_DOUBLE) &&
               (_to != D_MATH_VALUE_LONG_DOUBLE) ) )
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
        char*        _buffer,
        std::size_t  _size,
        d_math_value _x
    )
    {
        return static_cast<std::size_t>(
            std::snprintf(_buffer,
                          _size,
                          "lifted %.2Lf",
                          d_tests_lifted_in(_x)));
    }

    const d_math_boxed_kind g_lifted_kind = { "lifted",
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
        d_math_value boxed = d_math_value_none();

        boxed.kind = D_MATH_VALUE_BOXED;
        boxed.type = &g_lifted_kind;
        boxed.box  = const_cast<long double*>(_x);

        return m::value::adopt(boxed);
    }


    //==========================================================================
    // 4.  THE KIND
    //==========================================================================

    // d_tests_kind
    //   the table, the rank, the traits; the exactness entry; holders.
    void
    d_tests_kind()
    {
        const d_math_boxed_kind* kind = m::internal::expr_complex_kind();
        const m::value           z    = d_tests_complex(1, 2, -3, 4);
        const m::value           w    = m::expr_complex(m::value(0.5),
                                                        m::value(-0.25));

        D_TESTS_EXPECT( (kind != D_NULLPTR) &&
                        (std::string(kind->name) == "complex") &&
                        (kind->rank == 900) &&
                        (m::expr_complex_rank == 900) &&
                        (kind->traits == D_MATH_BOXED_NUMBER) &&
                        (kind->retain != D_NULLPTR) &&
                        (kind->release != D_NULLPTR) &&
                        (kind->unary != D_NULLPTR) &&
                        (kind->binary != D_NULLPTR) &&
                        (kind->cmp != D_NULLPTR) &&
                        (kind->convert != D_NULLPTR) &&
                        (kind->text != D_NULLPTR) &&
                        (kind->exact != D_NULLPTR) &&
                        (kind == m::internal::expr_complex_kind()),
                        "the table: complex, rank 900, a number, every entry "
                        "and the exact entry set, one for the program");
        D_TESTS_EXPECT( (m::expr_is_complex(z)) &&
                        (m::expr_is_complex(w)) &&
                        (z.is_number()) &&
                        (z.is_exact()) &&
                        (d_math_value_is_exact(z.c_value())) &&
                        (w.is_number()) &&
                        (!w.is_exact()) &&
                        (!d_math_value_is_exact(w.c_value())) &&
                        (d_math_value_rank(z.c_value()) == 900) &&
                        (std::string(d_math_value_kind_name(z.c_value())) ==
                             "complex"),
                        "exact or not value by value, as the exact entry "
                        "says (S325)");
        D_TESTS_EXPECT( (m::internal::expr_matrix_kind()->exact ==
                             D_NULLPTR) &&
                        (d_math_value_wide_kind()->exact == D_NULLPTR) &&
                        (!m::expr_is_complex(m::value(1))) &&
                        (!m::expr_is_complex(m::value(1.5))) &&
                        (!m::expr_is_complex(m::value(true))) &&
                        (!m::expr_is_complex(m::value())),
                        "no other kind asks: the matrix's and the wide "
                        "integer's tables hold no exact entry");

        // holders: a copy one more, its release one fewer; atomic here
        const std::size_t before =
            m::internal::expr_complex_box_of(z.c_value()).holders;
        std::size_t       during = 0;
        {
            const m::value copy = z;

            during = m::internal::expr_complex_box_of(copy.c_value())
                         .holders;
        }
        const std::size_t after =
            m::internal::expr_complex_box_of(z.c_value()).holders;

        D_TESTS_EXPECT( (before == 1) &&
                        (during == 2) &&
                        (after == 1),
                        "holders: a copy holds the box, its release lets "
                        "it go");
        D_TESTS_EXPECT( (D_MATH_BOXED_HOLDERS_ATOMIC ==
                         ( (D_INTERNAL_MATH_VALUE_HOLDERS != 0) ? 1 : 0 )),
                        "holders counted atomically where cfg_value.h "
                        "resolves it (S134, S326)");

        // im, one box, which every fold that reads it holds
        const m::expr_named_constant* im =
            m::expr_standard_constants().find("im");
        const std::size_t             im_before =
            m::internal::expr_complex_box_of(im->as_value.c_value()).holders;
        const m::value                read = d_tests_value_of("im");
        const std::size_t             im_during =
            m::internal::expr_complex_box_of(im->as_value.c_value()).holders;

        D_TESTS_EXPECT( (read.c_value().box == im->as_value.c_value().box) &&
                        (im_during == (im_before + 1)),
                        "im: one box, held by the value a fold gives");

        return;
    }


    //==========================================================================
    // 5.  MAKING ONE
    //==========================================================================

    // d_tests_making
    //   expr_complex: storage, exactness, the real part where the
    // imaginary part is exactly 0, the wider type, errors and refusals.
    void
    d_tests_making()
    {
        typedef m::internal::expr_complex_storage storage;

        const m::value exact = d_tests_complex(1, 3, 2, 1);
        const m::value real  = m::expr_complex(m::value::ratio(1, 3),
                                               m::value(0));
        const m::value third = m::expr_complex(m::value::ratio(1, 3),
                                               m::value(0.5));
        const m::value mixed = m::expr_complex(m::value(1.5f),
                                               m::value(0.1));
        const m::value lng   = m::expr_complex(m::value(2),
                                               m::value(0.1L));
        const m::value flt   = m::expr_complex(m::value(0.1f),
                                               m::value(-0.0f));
        const m::value zero  = m::expr_complex(m::value(1.0),
                                               m::value(0.0));

        D_TESTS_EXPECT( (d_tests_storage(exact) == storage::values) &&
                        (exact.is_exact()) &&
                        (exact == m::expr_complex(m::value::ratio(2, 6),
                                                  m::value(2))),
                        "exact parts: kept as values, exact");
        D_TESTS_EXPECT( (!m::expr_is_complex(real)) &&
                        (real.kind() == D_MATH_VALUE_RATIONAL) &&
                        (real == m::value::ratio(1, 3)),
                        "exact, its imaginary part 0: the real part, a "
                        "rational");
        D_TESTS_EXPECT( (d_tests_is_dense(third,
                                          m::complex<double>(1.0 / 3.0,
                                                             0.5))) &&
                        (d_tests_is_dense(mixed,
                                          m::complex<double>(1.5, 0.1))) &&
                        (d_tests_is_dense(lng,
                                          m::complex<long double>(2.0L,
                                                                  0.1L))) &&
                        (d_tests_is_dense(flt,
                                          m::complex<float>(0.1f, -0.0f))) &&
                        (d_tests_is_dense(zero,
                                          m::complex<double>(1.0, 0.0))),
                        "floating-point parts: dense in the wider type, an "
                        "exact part its nearest, a float widened exactly, "
                        "-0 and +0 kept, never made real");

        // a wide integer's part: exact, as values
        const char     digits[] = "1208925819614629174706176";
        d_math_wide    two_80   = d_math_wide_from_imax(0);

        static_cast<void>(d_math_wide_from_text(&two_80,
                                                digits,
                                                sizeof(digits) - 1));

        const m::value big  = m::value::adopt(d_math_value_from_wide(
                                  two_80));
        const m::value wide = m::expr_complex(big, m::value(-1));

        D_TESTS_EXPECT( (m::expr_is_complex(wide)) &&
                        (wide.is_exact()) &&
                        (d_tests_storage(wide) == storage::values) &&
                        (m::expr_real(wide) == big) &&
                        (d_tests_text(wide) ==
                             "1208925819614629174706176 - 1im"),
                        "a wide integer's part: exact, kept as a value");

        // an error passes, the first; what is no real part is refused
        const m::value pole  = m::value(D_MATH_VALUE_ERROR_POLE);
        const m::value range = m::value(D_MATH_VALUE_ERROR_RANGE);
        const m::value a_matrix =
            m::expr_vector({ m::value(1), m::value(2) }).value();

        D_TESTS_EXPECT( (d_tests_is_error(m::expr_complex(pole, range),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(m::expr_complex(m::value(true),
                                                          pole),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(m::expr_complex(m::value(true),
                                                          m::value(1)),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_complex(m::value(1),
                                                          m::value()),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_complex(a_matrix,
                                                          m::value(1)),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_complex(exact,
                                                          m::value(1)),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "an error passes, the first; a boolean, none, a "
                        "matrix or a complex part: a domain error");

        // flags: a part's, carried
        d_math_value flagged = d_math_value_from_int(3);

        flagged.flags = D_MATH_VALUE_FLAG_INEXACT;

        D_TESTS_EXPECT( (m::expr_complex(m::value::adopt(flagged),
                                         m::value(1)).flags() ==
                             D_MATH_VALUE_FLAG_INEXACT) &&
                        (m::expr_complex(m::value(1), m::value(1)).flags() ==
                             0u),
                        "a part's flags carried");

        // a math::complex<T>, and back
        const m::complex<float> made(1.5f, -0.0f);
        m::complex<double>      back;
        const m::value          as_value = m::expr_complex_of(made);

        D_TESTS_EXPECT( (d_tests_is_dense(as_value, made)) &&
                        (m::expr_complex_to(as_value, back)) &&
                        (d_tests_same_complex(back,
                                              m::complex<double>(1.5,
                                                                 -0.0))) &&
                        (d_tests_is_dense(m::expr_complex_of(
                                              m::complex<double>(2.0, 0.0)),
                                          m::complex<double>(2.0, 0.0))) &&
                        (m::expr_complex_to(m::value::ratio(1, 4), back)) &&
                        (d_tests_same_complex(back,
                                              m::complex<double>(0.25,
                                                                 0.0))) &&
                        (m::expr_complex_to(exact, back)) &&
                        (d_tests_same_complex(back,
                                              m::complex<double>(1.0 / 3.0,
                                                                 2.0))) &&
                        (!m::expr_complex_to(m::value(true), back)) &&
                        (d_tests_same_complex(back,
                                              m::complex<double>(1.0 / 3.0,
                                                                 2.0))),
                        "expr_complex_of keeps its parts, never real; "
                        "expr_complex_to gives each part's nearest, a real "
                        "number's +0, and refuses what is no number");

        return;
    }


    //==========================================================================
    // 6.  ARITHMETIC
    //==========================================================================

    // d_tests_exact_arithmetic
    //   random Gaussian rationals, real ones among them, each operation
    // against the test's own; the textbook cases.
    void
    d_tests_exact_arithmetic()
    {
        const d_math_value_op ops[4] = { D_MATH_VALUE_OP_ADD,
                                         D_MATH_VALUE_OP_SUB,
                                         D_MATH_VALUE_OP_MUL,
                                         D_MATH_VALUE_OP_DIV };
        long                  agreed = 0;
        long                  tried  = 0;
        long                  real   = 0;

        // pairs of random operands, each operation
        for (int trial = 0; trial < 3000; ++trial)
        {
            const d_tests_gauss a  = d_tests_random_gauss();
            const d_tests_gauss b  = d_tests_random_gauss();
            const m::value      va = d_tests_gauss_value(a);
            const m::value      vb = d_tests_gauss_value(b);

            for (int k = 0; k < 4; ++k)
            {
                d_tests_gauss  expected;
                const bool     defined = d_tests_gauss_op(ops[k],
                                                          a,
                                                          b,
                                                          expected);
                const m::value got = d_tests_apply(ops[k], va, vb);

                // only operations with a complex operand are the kind's
                if ( (!m::expr_is_complex(va)) &&
                     (!m::expr_is_complex(vb)) )
                {
                    continue;
                }

                ++tried;

                if (!defined)
                {
                    agreed += ( d_tests_is_error(got,
                                                 D_MATH_VALUE_ERROR_POLE)
                                    ? 1 : 0 );
                    continue;
                }

                const m::value want = d_tests_gauss_value(expected);

                real   += ( (!m::expr_is_complex(got)) ? 1 : 0 );
                agreed += ( ( (got.is_exact()) &&
                              (got.flags() == 0u) &&
                              (m::expr_is_complex(got) ==
                                   (expected.im.n != 0)) &&
                              (got.compare(want) == D_MATH_VALUE_EQUAL) )
                                ? 1 : 0 );
            }
        }

        D_TESTS_EXPECT( (agreed == tried) &&
                        (tried > 9000) &&
                        (real > 0),
                        "random Gaussian rationals, a real operand among "
                        "them: each sum, difference, product and quotient "
                        "exact, a real number where its imaginary part "
                        "comes to 0, a quotient by an exact 0 a pole");

        // the textbook cases
        const m::value i = d_tests_complex(0, 1, 1, 1);

        D_TESTS_EXPECT( (d_tests_complex(1, 1, 2, 1) *
                             d_tests_complex(3, 1, -1, 1) ==
                         d_tests_complex(5, 1, 5, 1)) &&
                        (i * i == m::value(-1)) &&
                        ((i * i).kind() == D_MATH_VALUE_INTEGER) &&
                        (d_tests_complex(3, 1, 4, 1) *
                             d_tests_complex(3, 1, -4, 1) ==
                         m::value(25)) &&
                        (m::value(1) / i == d_tests_complex(0, 1, -1, 1)) &&
                        (d_tests_complex(1, 1, 2, 1) /
                             d_tests_complex(3, 1, -1, 1) ==
                         d_tests_complex(1, 10, 7, 10)) &&
                        (m::value(2) - i == d_tests_complex(2, 1, -1, 1)) &&
                        (d_tests_is_error(i / m::value(0),
                                          D_MATH_VALUE_ERROR_POLE)),
                        "(1 + 2im)(3 - im) = 5 + 5im, im^2 = -1, (3 + 4im)"
                        "(3 - 4im) = 25, 1/im = -im, a quotient, 2 - im, "
                        "im / 0 a pole");

        // past the family: G2's long double, flagged, the number of long
        // double parts
        const d_math_imax big   = std::numeric_limits<d_math_imax>::max();
        const m::value    wide  = m::expr_complex(m::value(big),
                                                  m::value(1));
        const m::value    prod  = wide * wide;

        D_TESTS_EXPECT( (m::expr_is_complex(prod)) &&
                        (!prod.is_exact()) &&
                        (d_tests_storage(prod) ==
                             m::internal::expr_complex_storage::
                                 long_doubles) &&
                        ((prod.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0u),
                        "a part past the family: G2's long double, flagged, "
                        "the number then of long double parts");

        return;
    }

    // d_tests_dense_arithmetic_in
    //   random operands of Type, complex and real, each operation the bits
    // of math::complex<T>'s, cplx.h's.
    template<typename Type>
    bool
    d_tests_dense_arithmetic_in()
    {
        const d_math_value_op ops[4] = { D_MATH_VALUE_OP_ADD,
                                         D_MATH_VALUE_OP_SUB,
                                         D_MATH_VALUE_OP_MUL,
                                         D_MATH_VALUE_OP_DIV };
        bool                  all    = true;

        // pairs of random operands, a real one now and then
        for (int trial = 0; trial < 1500; ++trial)
        {
            const m::complex<Type> a(d_tests_random_real<Type>(),
                                     d_tests_random_real<Type>());
            const m::complex<Type> b(d_tests_random_real<Type>(),
                                     d_tests_random_real<Type>());
            const unsigned long    pick   = (d_tests_random() % 3u);
            const bool             real_a = (pick == 1u);
            const bool             real_b = (pick == 2u);
            const m::value         va     = ( real_a ? m::value(a.real())
                                                     : m::expr_complex_of(a) );
            const m::value         vb     = ( real_b ? m::value(b.real())
                                                     : m::expr_complex_of(b) );

            for (int k = 0; k < 4; ++k)
            {
                all = ( (all) &&
                        (d_tests_is_dense(d_tests_apply(ops[k], va, vb),
                                          d_tests_dense_apply(ops[k],
                                                              a,
                                                              real_a,
                                                              b,
                                                              real_b))) );
            }
        }

        return all;
    }

    // d_tests_dense_arithmetic
    //   the three types; a real operand taken as itself; the wider type.
    void
    d_tests_dense_arithmetic()
    {
        D_TESTS_EXPECT( d_tests_dense_arithmetic_in<float>(),
                        "float: every sum, difference, product and quotient "
                        "cplx.h's, bit for bit, a real operand its _real "
                        "form's");
        D_TESTS_EXPECT( d_tests_dense_arithmetic_in<double>(),
                        "double: the same");
        D_TESTS_EXPECT( d_tests_dense_arithmetic_in<long double>(),
                        "long double: the same");

        const m::value one = m::expr_complex(m::value(1.0), m::value(0.0));

        D_TESTS_EXPECT( (d_tests_is_dense(one * m::value(-1.0),
                                          m::complex<double>(-1.0, -0.0))) &&
                        (d_tests_is_dense(one * m::expr_complex(m::value(-1.0),
                                                                m::value(0.0)),
                                          m::complex<double>(-1.0, 0.0))) &&
                        (d_tests_is_dense(m::value(1.0) - one,
                                          m::complex<double>(0.0, -0.0))) &&
                        (d_tests_is_dense(one * m::value(-1),
                                          m::complex<double>(-1.0, -0.0))),
                        "a real operand as itself (G.5): (1 + 0im)(-1) is "
                        "-1 - 0im, (1 + 0im)(-1 + 0im) -1 + 0im, 1 - (1 + "
                        "0im) 0 - 0im, an exact -1 too");

        // the wider type; an exact operand its nearest there
        const m::value third = d_tests_complex(1, 3, 2, 3);
        const m::value half  = m::expr_complex(m::value(0.5f),
                                               m::value(0.25f));

        D_TESTS_EXPECT( (d_tests_is_dense(third + m::value(0.5),
                                          m::complex<double>(1.0 / 3.0,
                                                             2.0 / 3.0) +
                                              0.5)) &&
                        (d_tests_is_dense(half * m::value(3.0),
                                          m::complex<double>(0.5, 0.25) *
                                              3.0)) &&
                        (d_tests_is_dense(half + m::value(0.1L),
                                          m::complex<long double>(0.5L,
                                                                  0.25L) +
                                              0.1L)) &&
                        (d_tests_is_dense(half * third,
                                          m::complex<float>(0.5f, 0.25f) *
                                              m::complex<float>(
                                                  1.0f / 3.0f,
                                                  2.0f / 3.0f))),
                        "the widest floating-point type of the operands, "
                        "each exact part its nearest there");

        return;
    }

    // d_tests_unary
    //   -z and |z|.
    void
    d_tests_unary()
    {
        const m::value z = d_tests_complex(3, 1, -4, 1);
        const m::value d = m::expr_complex(m::value(0.0), m::value(-1.5));

        D_TESTS_EXPECT( (-z == d_tests_complex(-3, 1, 4, 1)) &&
                        ((-z).is_exact()) &&
                        (d_tests_is_dense(-d,
                                          m::complex<double>(-0.0, 1.5))),
                        "-z: both parts negated, a zero's sign too");
        D_TESTS_EXPECT( (abs(z) == m::value(5)) &&
                        (abs(z).kind() == D_MATH_VALUE_INTEGER) &&
                        (abs(d_tests_complex(3, 5, 4, 5)) ==
                             m::value::ratio(1, 1)) &&
                        (abs(d_tests_complex(5, 2, 6, 1)) ==
                             m::value::ratio(13, 2)),
                        "|z| exact where it is rational: |3 - 4im| = 5, "
                        "|3/5 + 4/5im| = 1, |5/2 + 6im| = 13/2");

        long double root2 = 0.0L;

        D_TESTS_EXPECT( (abs(d_tests_complex(1, 1, 1, 1)).to(root2)) &&
                        (abs(d_tests_complex(1, 1, 1, 1)).kind() ==
                             D_MATH_VALUE_LONG_DOUBLE) &&
                        (root2 == std::sqrt(2.0L)),
                        "|1 + im|: the long double nearest sqrt(2)");

        bool all = true;

        // dense: abs_c's bits
        for (int trial = 0; trial < 500; ++trial)
        {
            const m::complex<double> w(d_tests_random_real<double>(),
                                       d_tests_random_real<double>());
            double                   got = 0.0;

            all = ( (all) &&
                    (abs(m::expr_complex_of(w)).kind() ==
                         D_MATH_VALUE_DOUBLE) &&
                    (abs(m::expr_complex_of(w)).to(got)) &&
                    (d_tests_same_bits(got, abs(w))) );
        }

        D_TESTS_EXPECT( all,
                        "|z| of dense parts: cplx.h's abs_c, bit for bit");

        // the sum of the squares past the family: G2's, flagged
        const d_math_imax most = std::numeric_limits<d_math_imax>::max();
        const m::value    far  = abs(m::expr_complex(m::value(most),
                                                     m::value(1)));

        D_TESTS_EXPECT( (far.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        ((far.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0u) &&
                        (far.to(root2)) &&
                        (root2 == static_cast<long double>(most)),
                        "|z| whose squares pass the family: the long double "
                        "nearest, flagged inexact");

        return;
    }


    //==========================================================================
    // 7.  COMPARISON, CONVERSION AND TEXT
    //==========================================================================

    // d_tests_comparison
    //   equal by the parts; unordered otherwise.
    void
    d_tests_comparison()
    {
        const double   nan = std::numeric_limits<double>::quiet_NaN();
        const m::value z   = d_tests_complex(1, 2, 1, 1);
        const m::value w   = m::expr_complex(m::value(0.5), m::value(1.0));
        const m::value r   = m::expr_complex(m::value(3.0), m::value(-0.0));
        const m::value n   = m::expr_complex(m::value(nan), m::value(1.0));

        D_TESTS_EXPECT( (z.compare(w) == D_MATH_VALUE_EQUAL) &&
                        (w.compare(z) == D_MATH_VALUE_EQUAL) &&
                        (z == w) &&
                        (r == m::value(3)) &&
                        (m::value(3) == r) &&
                        (r.compare(m::value(3.0f)) == D_MATH_VALUE_EQUAL),
                        "equal by the parts, in the exact order: 1/2 + im "
                        "and 0.5 + 1.0im, and 3 - 0.0im and 3");
        D_TESTS_EXPECT( (z.compare(d_tests_complex(1, 2, -1, 1)) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (z.compare(m::value(1)) == D_MATH_VALUE_UNORDERED) &&
                        (m::value(1).compare(z) == D_MATH_VALUE_UNORDERED) &&
                        (n.compare(n) == D_MATH_VALUE_UNORDERED) &&
                        (!(z < w)) &&
                        (!(z > w)) &&
                        (z <= w) &&
                        (z != d_tests_complex(1, 2, -1, 1)) &&
                        (!(z < m::value(100))) &&
                        (z.compare(m::value(true)) == D_MATH_VALUE_UNORDERED),
                        "unordered otherwise, a NaN part with itself too, "
                        "and with what is no number");

        return;
    }

    // d_tests_conversion
    //   to a real type where the imaginary part is 0; never exact.
    void
    d_tests_conversion()
    {
        // the tenths stored, so that the x87 holds no more of them than
        // their types do
        volatile double tenth   = 0.1;
        volatile float  tenth_f = 0.1f;
        const m::value  r = m::expr_complex(m::value(0.1), m::value(-0.0));
        float           f = 7.0f;
        double          d = 7.0;
        long double     l = 7.0L;
        d_rational      q = { 7, 1 };
        int             direction = 9;
        d_math_value    out = d_math_value_none();

        D_TESTS_EXPECT( (r.to(d)) &&
                        (d == tenth) &&
                        (r.to(f)) &&
                        (f == tenth_f) &&
                        (r.to(l)) &&
                        (l == static_cast<long double>(tenth)) &&
                        (!r.to(q)) &&
                        (q.num == 7),
                        "an imaginary part of 0: the real part's nearest; "
                        "no rational");
        const d_math_boxed_kind* kind = m::internal::expr_complex_kind();
        const m::value           down = m::expr_complex(m::value(0.7),
                                                        m::value(0.0));

        D_TESTS_EXPECT( (m::expr_is_complex(r)) &&
                        (kind->convert(&out,
                                       &direction,
                                       r.c_value(),
                                       D_MATH_VALUE_FLOAT)) &&
                        (out.kind == D_MATH_VALUE_FLOAT) &&
                        (direction == 1) &&
                        (kind->convert(&out,
                                       &direction,
                                       r.c_value(),
                                       D_MATH_VALUE_DOUBLE)) &&
                        (direction == 0) &&
                        (m::expr_is_complex(down)) &&
                        (kind->convert(&out,
                                       &direction,
                                       down.c_value(),
                                       D_MATH_VALUE_FLOAT)) &&
                        (direction == -1),
                        "the rounding's direction, exactly: 0.1 as a float "
                        "above the double, as a double equal, and 0.7 as a "
                        "float below");

        d = 7.0;

        D_TESTS_EXPECT( (!d_tests_complex(1, 1, 1, 1).to(d)) &&
                        (!m::expr_complex(m::value(1.0),
                                          m::value(1e-300)).to(d)) &&
                        (!m::expr_complex(m::value(1.0),
                                          m::value(std::numeric_limits<
                                              double>::quiet_NaN())).to(d)) &&
                        (d == 7.0) &&
                        (!d_tests_complex(1, 1, 1, 1).to(q)),
                        "an imaginary part, or a NaN for one: none, out as "
                        "it was");

        return;
    }

    // d_tests_kind_text
    //   the value layer's text.
    void
    d_tests_kind_text()
    {
        const double   inf = std::numeric_limits<double>::infinity();
        const double   nan = std::numeric_limits<double>::quiet_NaN();
        const m::value z   = d_tests_complex(1, 2, -3, 4);
        char           room[8];

        D_TESTS_EXPECT( (d_tests_text(d_tests_complex(0, 1, 1, 1)) ==
                             "0 + 1im") &&
                        (d_tests_text(d_tests_complex(5, 1, 5, 1)) ==
                             "5 + 5im") &&
                        (d_tests_text(z) == "1/2 - 3/4im") &&
                        (d_tests_text(d_tests_complex(-7, 1, 1, 3)) ==
                             "-7 + 1/3im") &&
                        (d_tests_text(m::expr_complex(m::value(1.5),
                                                      m::value(0.0))) ==
                             "1.5 + 0im") &&
                        (d_tests_text(m::expr_complex(m::value(-0.25),
                                                      m::value(-0.0))) ==
                             "-0.25 - 0im") &&
                        (d_tests_text(m::expr_complex(m::value(0.1f),
                                                      m::value(1.0f))) ==
                             "0.100000001 + 1im") &&
                        (d_tests_text(m::expr_complex(m::value(nan),
                                                      m::value(nan))) ==
                             "nan + nan*im") &&
                        (d_tests_text(m::expr_complex(m::value(-inf),
                                                      m::value(-inf))) ==
                             "-inf - inf*im"),
                        "text: the real part, + or -, the imaginary part's "
                        "magnitude and im; a name times im");

        room[0] = 'x';
        room[6] = 'y';

        const d_math_boxed_kind* kind   = m::internal::expr_complex_kind();
        const std::size_t        length = kind->text(room, 6u, z.c_value());

        D_TESTS_EXPECT( (length == 11u) &&
                        (std::string(room) == "1/2 -") &&
                        (room[6] == 'y') &&
                        (kind->text(D_NULLPTR, 0u, z.c_value()) == 11u),
                        "text: a buffer too short, the whole length "
                        "returned");

        return;
    }


    //==========================================================================
    // 8.  THE PARTS
    //==========================================================================

    // d_tests_parts
    //   expr_real, expr_imag, expr_conj and expr_arg.
    void
    d_tests_parts()
    {
        const long double pi  = 3.141592653589793238462643383279502884197L;
        const double      nan = std::numeric_limits<double>::quiet_NaN();
        const m::value    z   = d_tests_complex(1, 2, -3, 4);
        const m::value    d   = m::expr_complex(m::value(-1.0),
                                                m::value(-0.0));
        long double       got = 0.0L;
        double            dgot = 0.0;
        float             fgot = 0.0f;

        D_TESTS_EXPECT( (m::expr_real(z) == m::value::ratio(1, 2)) &&
                        (m::expr_imag(z) == m::value::ratio(-3, 4)) &&
                        (m::expr_conj(z) == d_tests_complex(1, 2, 3, 4)) &&
                        (m::expr_real(m::value(7)) == m::value(7)) &&
                        (m::expr_imag(m::value(7)) == m::value(0)) &&
                        (m::expr_imag(m::value(7)).is_exact()) &&
                        (m::expr_imag(m::value(2.5f)).kind() ==
                             D_MATH_VALUE_FLOAT) &&
                        (m::expr_imag(m::value(-2.5)).to(dgot)) &&
                        (d_tests_same_bits(dgot, 0.0)) &&
                        (m::expr_conj(m::value(1.5)) == m::value(1.5)),
                        "real, imag and conj; a real number's imaginary part "
                        "a +0 of its kind");
        D_TESTS_EXPECT( (d_tests_is_dense(m::expr_conj(d),
                                          m::complex<double>(-1.0, 0.0))) &&
                        (m::expr_imag(d).to(dgot)) &&
                        (d_tests_same_bits(dgot, -0.0)),
                        "a dense conjugate flips the sign of 0 too");
        D_TESTS_EXPECT( (m::expr_arg(m::value(3)) == m::value(0)) &&
                        (m::expr_arg(m::value(3)).is_exact()) &&
                        (m::expr_arg(m::value(0)) == m::value(0)) &&
                        (m::expr_arg(m::value(0)).is_exact()) &&
                        (m::expr_arg(m::value(-2)).to(got)) &&
                        (got == pi) &&
                        (m::expr_arg(m::value(-2.0)).to(dgot)) &&
                        (dgot == static_cast<double>(pi)) &&
                        (m::expr_arg(m::value(-0.0f)).kind() ==
                             D_MATH_VALUE_FLOAT) &&
                        (m::expr_arg(m::value(-0.0f)).to(fgot)) &&
                        (fgot == static_cast<float>(pi)) &&
                        (m::expr_arg(m::value(0.0)).to(dgot)) &&
                        (d_tests_same_bits(dgot, 0.0)) &&
                        (m::expr_arg(m::value(nan)).to(dgot)) &&
                        (dgot != dgot),
                        "arg of a real number: 0 exactly where it is exact "
                        "and not negative, 0 among them; pi, the constant's, "
                        "where negative, -0 among them; a NaN's a NaN");
        D_TESTS_EXPECT( (m::expr_arg(d).to(dgot)) &&
                        (d_tests_same_bits(dgot,
                                           arg(m::complex<double>(-1.0,
                                                                  -0.0)))) &&
                        (dgot == -static_cast<double>(pi)) &&
                        (m::expr_arg(d_tests_complex(0, 1, 1, 1)).to(got)) &&
                        (got == arg(m::complex<long double>(0.0L, 1.0L))) &&
                        (m::expr_arg(z).kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (m::expr_arg(z).to(got)) &&
                        (got == arg(m::complex<long double>(0.5L, -0.75L))),
                        "arg of a complex number: arg_c of its parts in "
                        "their type, an exact one's long doubles");
        D_TESTS_EXPECT( (d_tests_is_error(m::expr_real(m::value(true)),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_imag(m::value()),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(m::expr_conj(
                                              m::value(
                                                  D_MATH_VALUE_ERROR_RANGE)),
                                          D_MATH_VALUE_ERROR_RANGE)) &&
                        (d_tests_is_error(m::expr_arg(
                                              m::expr_vector({ m::value(1) })
                                                  .value()),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "an error passes; what is no number is a domain "
                        "error");

        // number_traits<value>: no real root of a complex number
        D_TESTS_EXPECT( (d_tests_is_error(
                            m::number_traits<m::value>::sqrt(z),
                            D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(
                            m::number_traits<m::value>::sqrt(
                                m::expr_complex(m::value(-1.0),
                                                m::value(0.5))),
                            D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (m::number_traits<m::value>::sqrt(d).to(got)) &&
                        (got != got) &&
                        (m::number_traits<m::value>::sqrt(
                             m::expr_complex(m::value(4.0),
                                             m::value(0.0))) ==
                             m::value(2.0L)),
                        "number_traits<value>::sqrt: a domain error of a "
                        "number of no real value; where the imaginary part "
                        "is 0, the real part's root, a NaN for -1 - 0im as "
                        "for -1.0");

        return;
    }

    // d_tests_matrices_made
    //   matrices of complex numbers, from step 23's third part (S348): made,
    // their arithmetic and blocks the kind's, linear algebra's functions of
    // a complex number computed, as scalars.
    void
    d_tests_matrices_made()
    {
        const m::value z = d_tests_complex(1, 1, 1, 1);
        const m::value a = m::expr_vector({ m::value(1), m::value(2) })
                               .value();

        const m::expr_result<m::value> made =
            m::expr_matrix(1, 2, { m::value(1), z });
        const m::value twice =
            m::expr_vector({ z, z * m::value(2) }).value();

        D_TESTS_EXPECT( (made.is_ok()) &&
                        (m::expr_element(made.value(), 0, 1).value() == z) &&
                        (a * z == twice) &&
                        (z * a == twice) &&
                        (a / z == m::expr_vector({ m::value(1) / z,
                                                   m::value(2) / z })
                                      .value()) &&
                        (m::internal::expr_matrix_beside({ a, z }) ==
                             m::expr_vector({ m::value(1), m::value(2), z })
                                 .value()),
                        "a matrix of complex numbers made: expr_matrix's, "
                        "the kind's arithmetic and a block's (S348, S349)");

        const m::value args[2] = { z, m::value(2) };

        D_TESTS_EXPECT( (m::internal::expr_det_value(args, 1) == z) &&
                        (m::internal::expr_dot_value(args, 2) ==
                             (m::expr_conj(z) * m::value(2))) &&
                        (m::internal::expr_inv_value(args, 1) ==
                             (m::value(1) / z)) &&
                        (m::internal::expr_norm_value(args, 1) ==
                             abs(z)) &&
                        (m::internal::expr_transpose_value(z) ==
                             m::expr_conj(z)) &&
                        (m::internal::expr_transpose_value(z, false) == z),
                        "linear algebra's functions of one, a scalar: det "
                        "itself, dot conj(z) times the other, inv 1/z, norm "
                        "|z| (S351); z' its conjugate, transpose(z) itself "
                        "(S350)");

        return;
    }


    //==========================================================================
    // 9.  THE LANGUAGE
    //==========================================================================

    // d_tests_language
    //   im, the five functions, the modulus, the transpose, the relations
    // and the powers, as values.
    void
    d_tests_language()
    {
        const m::value i = d_tests_complex(0, 1, 1, 1);

        D_TESTS_EXPECT( (d_tests_value_of("im") == i) &&
                        (d_tests_value_of("3 + 4im") ==
                             d_tests_complex(3, 1, 4, 1)) &&
                        (m::expr_evaluate(d_tests_parsed("3 + 4 * im",
                                                         false),
                                          m::expr_bindings(),
                                          m::expr_context()).value() ==
                             d_tests_complex(3, 1, 4, 1)) &&
                        (d_tests_value_of("(1 + 2im)(3 - im)") ==
                             d_tests_complex(5, 1, 5, 1)) &&
                        (d_tests_value_of("1/2im") ==
                             d_tests_complex(0, 1, 1, 2)) &&
                        (d_tests_value_of("im im").kind() ==
                             D_MATH_VALUE_INTEGER),
                        "im the unit; 3 + 4im by implied multiplication, "
                        "3 + 4 * im without; 1/2im (1/2)im; im im -1, an "
                        "integer");
        D_TESTS_EXPECT( (d_tests_value_of("complex(3, 4)") ==
                             d_tests_complex(3, 1, 4, 1)) &&
                        (d_tests_value_of("complex(3, 0)").kind() ==
                             D_MATH_VALUE_INTEGER) &&
                        (d_tests_is_dense(d_tests_value_of("complex(x, 0)",
                                                           m::value(-0.0)),
                                          m::complex<double>(-0.0, 0.0))) &&
                        (d_tests_value_of("real(3 + 4im)") == m::value(3)) &&
                        (d_tests_value_of("imag(3 + 4im)") == m::value(4)) &&
                        (d_tests_value_of("conj(3 + 4im)") ==
                             d_tests_complex(3, 1, -4, 1)) &&
                        (d_tests_value_of("(3 + 4im)'") ==
                             d_tests_complex(3, 1, -4, 1)) &&
                        (d_tests_value_of("arg(im)") ==
                             m::expr_arg(i)) &&
                        (d_tests_value_of("abs(3 + 4im)") == m::value(5)) &&
                        (d_tests_value_of("|3 - 4im|") == m::value(5)),
                        "complex, real, imag, conj, the transpose, arg, abs "
                        "and the bars, as values");
        D_TESTS_EXPECT( (d_tests_value_of("complex(1, 2) = 1 + 2im") ==
                             m::value(true)) &&
                        (d_tests_value_of("im = 1") == m::value(false)) &&
                        (d_tests_value_of("im != 1") == m::value(true)) &&
                        (d_tests_value_of("complex(x, -0.0) = 3",
                                          m::value(3.0)) ==
                             m::value(true)) &&
                        (d_tests_is_error(d_tests_value_of("im < 1"),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(d_tests_value_of("1 <= im"),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(d_tests_value_of("im > im"),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(d_tests_value_of("x >= 0",
                                                           i),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "= and != by the parts; < <= > >= of a complex "
                        "number a domain error (S271)");
        D_TESTS_EXPECT( (d_tests_value_of("(1 + im)^2") ==
                             d_tests_complex(0, 1, 2, 1)) &&
                        (d_tests_value_of("(1 + im)^(-2)") ==
                             d_tests_complex(0, 1, -1, 2)) &&
                        (d_tests_value_of("im^4").kind() ==
                             D_MATH_VALUE_INTEGER) &&
                        (d_tests_value_of("im^4") == m::value(1)) &&
                        (d_tests_value_of("(2im)^0") == m::value(1)) &&
                        (d_tests_value_of("pow(1 + im, 3)") ==
                             d_tests_complex(-2, 1, 2, 1)) &&
                        (d_tests_value_of("(1/3 + 2/7im)^3") ==
                             d_tests_complex(-59, 1323, 74, 1029)),
                        "an exact complex number to a whole power: exact, "
                        "by repeated squaring, its reciprocal's for a "
                        "negative one");

        // the real line's order: a domain error of a complex number
        const char* const ordered[] = {
            "cbrt(im)", "floor(im)", "ceil(im)", "round(im)", "trunc(im)",
            "fmod(im, 2)", "remainder(2, im)", "im mod 2", "atan2(im, 1)",
            "hypot(1, im)", "factorial(im)", "im!" };
        bool              all = true;

        for (std::size_t k = 0; k < 12; ++k)
        {
            all = ( (all) &&
                    (d_tests_is_error(d_tests_value_of(ordered[k]),
                                      D_MATH_VALUE_ERROR_DOMAIN)) );
        }

        D_TESTS_EXPECT( all,
                        "cbrt, floor, ceil, round, trunc, fmod, remainder, "
                        "mod, atan2, hypot and the factorial of a complex "
                        "number: a domain error (S271)");

        // dense, bound
        D_TESTS_EXPECT( (d_tests_is_dense(d_tests_value_of("x + 2im",
                                                           m::value(1.5)),
                                          m::complex<double>(1.5, 2.0))) &&
                        (d_tests_is_dense(d_tests_value_of("x * im",
                                                           m::value(-1.0f)),
                                          m::complex<float>(-0.0f, -1.0f))),
                        "a bound double plus 2im: dense in double; -1.0f "
                        "times im: -0 - 1im in float, im taken as 0 + 1im "
                        "times a real operand");

        return;
    }

    // d_tests_not_yet_nodes
    //   what step 23's later parts compute: not_yet with its message; an
    // error and wrong_arity first.
    void
    d_tests_not_yet_nodes()
    {
        const char* const principal[] = {
            "sqrt", "sin", "cos", "tan", "asin", "acos", "atan", "sinh",
            "cosh", "tanh", "exp", "log", "asinh", "acosh", "atanh" };
        const char* const five[] = {
            "exp2", "log2", "log10", "log1p", "expm1" };
        const char* const algebra[] = {
            "det", "inv", "trace", "norm", "eig" };
        bool              all = true;

        for (std::size_t k = 0; k < 5; ++k)
        {
            const std::string name  = five[k];
            const m::value    value =
                d_tests_value_of((name + "(1 + im)").c_str());

            all = ( (all) &&
                    (m::expr_is_complex(value)) &&
                    (!value.is_exact()) );
        }

        D_TESTS_EXPECT( (all) &&
                        (d_tests_is_dense(d_tests_value_of("log2(-8)"),
                                          log2(m::complex<long double>(
                                              -8.0L)))) &&
                        (d_tests_is_dense(d_tests_value_of("log10(x)",
                                                           m::value(-0.5)),
                                          log10(m::complex<double>(-0.5)))) &&
                        (d_tests_is_dense(d_tests_value_of("log1p(-3/2)"),
                                          log1p(m::complex<long double>(
                                              -1.5L)))),
                        "exp2, log2, log10, log1p and expm1 of a complex "
                        "argument, and of a real one whose value is complex "
                        "alone: their principal values from chunk 42, dense, "
                        "never not_yet (S342)");

        all = true;

        for (std::size_t k = 0; k < 15; ++k)
        {
            const std::string name = principal[k];

            all = ( (all) &&
                    (m::expr_is_complex(
                         d_tests_value_of((name + "(1 + im)").c_str()))) );
        }

        D_TESTS_EXPECT( all,
                        "each other function with a principal complex "
                        "value, of a complex argument: computed, no longer "
                        "not_yet (chunk 41)");

        all = true;

        const char* const scalars[] = { "im", "-im", "im", "1", "im" };

        for (std::size_t k = 0; k < 5; ++k)
        {
            const std::string name = algebra[k];

            all = ( (all) &&
                    (d_tests_value_of((name + "(im)").c_str()) ==
                         d_tests_value_of(scalars[k])) );
        }

        D_TESTS_EXPECT( (all) &&
                        (d_tests_value_of("dot(im, 1)") ==
                             d_tests_value_of("-im")) &&
                        (d_tests_is_error(
                             d_tests_value_of("cross([1, 2, 3], x)",
                                              d_tests_complex(1, 1, 1, 1)),
                             D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_value_of("solve(2, im)") ==
                             d_tests_value_of("1/2im")),
                        "linear algebra's functions of a complex number, a "
                        "scalar: det, trace and eig itself, inv its "
                        "reciprocal, norm its modulus, dot(a, b) conj(a) b, "
                        "solve(a, b) b / a; cross of a number a domain error "
                        "(S351)");

        D_TESTS_EXPECT( (m::expr_is_complex(d_tests_value_of("im^0.5"))) &&
                        (m::expr_is_complex(d_tests_value_of("2^im"))) &&
                        (d_tests_value_of("im^im").kind() ==
                             D_MATH_VALUE_BOXED) &&
                        (m::expr_is_complex(d_tests_value_of(
                             "pow(im, x)", m::value(2.0)))) &&
                        (m::expr_is_complex(d_tests_value_of(
                             "x^2",
                             m::expr_complex(m::value(1.0),
                                             m::value(1.0))))),
                        "every other power of a complex number: computed, "
                        "no longer not_yet (chunk 41)");
        D_TESTS_EXPECT( (m::expr_is_matrix(d_tests_value_of("[1, im]"))) &&
                        (m::expr_is_matrix(d_tests_value_of("[1; im]"))) &&
                        (d_tests_value_of("[1, 2] * im") ==
                             d_tests_value_of("[im, 2im]")) &&
                        (d_tests_is_error(d_tests_value_of("im + [1, 2]"),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_value_of("real([1, 2])") ==
                             d_tests_value_of("[1, 2]")) &&
                        (d_tests_is_error(d_tests_value_of(
                                              "complex([1, 2], 1)"),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_value_of("arg([1])") ==
                             d_tests_value_of("[0]")),
                        "a literal with a complex element a matrix (S348); "
                        "a matrix times a complex number each element's "
                        "product, plus one a domain error, as for a real "
                        "one (S246); real and arg of a matrix element by "
                        "element, complex of a matrix and a number a domain "
                        "error (S350)");

        // an error first; wrong_arity first
        const m::expr_result<m::value> arity =
            m::expr_evaluate(d_tests_parsed("sqrt(im, 1)"),
                             m::expr_bindings(),
                             m::expr_context());

        D_TESTS_EXPECT( (d_tests_is_error(d_tests_value_of("sqrt(im + 1/0)"),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(d_tests_value_of("[im, 1/0]"),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (arity.is_err()) &&
                        (arity.error().status ==
                             m::expr_status::wrong_arity),
                        "an operand's error passes first; wrong_arity "
                        "first");

        // what is computed: abs, piecewise, the parts' functions of numbers
        D_TESTS_EXPECT( (d_tests_value_of("piecewise(when(x = im, 2im), "
                                          "otherwise(0))",
                                          d_tests_complex(0, 1, 1, 1)) ==
                             d_tests_complex(0, 1, 2, 1)) &&
                        (d_tests_value_of("abs(im)") == m::value(1)),
                        "piecewise's values and abs: computed");

        return;
    }

    // d_tests_in_types
    //   every number in a type is real.
    template<typename Type>
    bool
    d_tests_in_type()
    {
        const Type pi = static_cast<Type>(
            3.141592653589793238462643383279502884197L);

        return ( (d_tests_in<Type>("im") != d_tests_in<Type>("im")) &&
                 (d_tests_in<Type>("3 + 4im") !=
                      d_tests_in<Type>("3 + 4im")) &&
                 (d_tests_in<Type>("complex(2, 0)") == Type(2)) &&
                 (d_tests_in<Type>("complex(2, -0.0)") == Type(2)) &&
                 (d_tests_in<Type>("complex(2, 1)") !=
                      d_tests_in<Type>("complex(2, 1)")) &&
                 (d_tests_in<Type>("real(x)", m::value(1.5)) == Type(1.5)) &&
                 (d_tests_same_bits(d_tests_in<Type>("imag(x)",
                                                     m::value(-1.5)),
                                    Type(0))) &&
                 (d_tests_in<Type>("conj(x)", m::value(-1.5)) == Type(-1.5)) &&
                 (d_tests_in<Type>("arg(-1)") == pi) &&
                 (d_tests_in<Type>("arg(-0.0)") == pi) &&
                 (d_tests_same_bits(d_tests_in<Type>("arg(2)"), Type(0))) &&
                 (d_tests_same_bits(d_tests_in<Type>("arg(0)"), Type(0))) &&
                 (d_tests_in<Type>("arg(0/0)") !=
                      d_tests_in<Type>("arg(0/0)")) &&
                 (d_tests_in<Type>("x", d_tests_complex(1, 1, 1, 1)) !=
                      d_tests_in<Type>("x", d_tests_complex(1, 1, 1, 1))) &&
                 (d_tests_in<Type>("x",
                                   m::expr_complex(m::value(3.0),
                                                   m::value(-0.0))) ==
                      Type(3)) );
    }

    void
    d_tests_in_types()
    {
        D_TESTS_EXPECT( d_tests_in_type<float>(),
                        "in float: im a NaN, complex(x, y) x where y is 0, "
                        "real, imag, conj and arg of a real number, a "
                        "variable bound to a complex number a NaN, to one "
                        "of an imaginary part 0 its real part");
        D_TESTS_EXPECT( d_tests_in_type<double>(), "in double: the same");
        D_TESTS_EXPECT( d_tests_in_type<long double>(),
                        "in long double: the same");

        return;
    }


    //==========================================================================
    // 10. THE RENDERER
    //==========================================================================

    // d_tests_rendering
    //   a complex number as a sum, in brackets; read back.
    void
    d_tests_rendering()
    {
        const d_math_imax least = std::numeric_limits<d_math_imax>::min();
        const double      nan   = std::numeric_limits<double>::quiet_NaN();
        const double      inf   = std::numeric_limits<double>::infinity();
        const std::string min_text =
            m::internal::expr_signed_text(least);

        D_TESTS_EXPECT( (d_tests_rendered(d_tests_complex(3, 1, 4, 1)) ==
                             "(3 + 4 * im)") &&
                        (d_tests_rendered(d_tests_complex(1, 2, -3, 4)) ==
                             "(1/2 - 3/4 * im)") &&
                        (d_tests_rendered(d_tests_complex(-7, 1, 1, 3)) ==
                             "(-7 + 1/3 * im)") &&
                        (d_tests_rendered(d_tests_complex(0, 1, 1, 1)) ==
                             "(0 + 1 * im)") &&
                        (d_tests_rendered(m::expr_complex(m::value(1.5),
                                                          m::value(-0.0))) ==
                             "(1.5 - 0 * im)") &&
                        (d_tests_rendered(m::expr_complex(m::value(0.1),
                                                          m::value(-2.5))) ==
                             "(0.1 - 2.5 * im)") &&
                        (d_tests_rendered(m::expr_complex(m::value(nan),
                                                          m::value(-inf))) ==
                             "(nan - inf * im)"),
                        "a sum in brackets: exact parts, floating-point ones "
                        "shortest, -0, nan and inf");
        D_TESTS_EXPECT( (d_tests_rendered(m::expr_complex(m::value(least),
                                                          m::value(2))) ==
                             "(" + min_text + " + 2 * im)") &&
                        (d_tests_rendered(m::expr_complex(m::value(3),
                                                          m::value(least))) ==
                             "(3 + (" + min_text + ") * im)") &&
                        (d_tests_rendered(m::expr_complex(
                             m::value(1),
                             m::value::ratio(least, 3))) ==
                             "(1 + (" + min_text + ")/3 * im)"),
                        "the least integer: the next one less 1, its "
                        "imaginary part added as it is");

        D_TESTS_EXPECT( (d_tests_reads_back(d_tests_complex(3, 1, 4, 1))) &&
                        (d_tests_reads_back(d_tests_complex(1, 2, -3, 4))) &&
                        (d_tests_reads_back(m::expr_complex(m::value(least),
                                                            m::value(2)))) &&
                        (d_tests_reads_back(m::expr_complex(m::value(3),
                                                            m::value(least))))
                        && (d_tests_reads_back(m::expr_complex(
                                m::value(1),
                                m::value::ratio(least, 3)))),
                        "each exact one read back, implied multiplication "
                        "off, as itself");

        int agreed = 0;

        // random exact ones
        for (int trial = 0; trial < 500; ++trial)
        {
            const d_tests_gauss z = d_tests_random_gauss();

            agreed += ( d_tests_reads_back(d_tests_gauss_value(z)) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (agreed == 500),
                        "500 random exact numbers, real ones among them, "
                        "read back as themselves");

        // in a term: beside other operands, and substituted
        const m::expr_term product =
            m::expr_application(m::expr_op_kind::mul,
                                m::expr_number(d_tests_complex(3, 1, 4, 1)),
                                m::expr_variable("x"));
        const m::expr_term power =
            m::expr_application(m::expr_op_kind::pow,
                                m::expr_number(d_tests_complex(1, 1, 1, 1)),
                                m::expr_number(m::value(2)));
        const m::expr_term put =
            m::expr_substitute(d_tests_parsed("x + 1", false),
                               "x",
                               m::expr_number(d_tests_complex(0, 1, 2, 1)));

        D_TESTS_EXPECT( (m::expr_render(product) == "(3 + 4 * im) * x") &&
                        (m::expr_render(power) == "(1 + 1 * im) ^ 2") &&
                        (m::expr_render(put) == "(0 + 2 * im) + 1") &&
                        (m::expr_evaluate(d_tests_parsed(
                                              m::expr_render(power).c_str(),
                                              false),
                                          m::expr_bindings(),
                                          m::expr_context()).value() ==
                             d_tests_complex(0, 1, 2, 1)),
                        "in a term: bracketed, read back to its value");

        // simplification folds complex numbers as numbers
        const m::expr_term simpler = m::expr_simplify(power);

        D_TESTS_EXPECT( (!simpler.empty()) &&
                        (m::expr_evaluate(simpler,
                                          m::expr_bindings(),
                                          m::expr_context()).value() ==
                             d_tests_complex(0, 1, 2, 1)),
                        "simplification keeps a complex number's value");

        return;
    }


    //==========================================================================
    // 11. EVERY STORAGE AND EVERY REFUSAL
    //==========================================================================

    // d_tests_storages
    //   float and long double parts through -z, |z|, conj, imag, the text
    // and the renderer; parts of a boxed kind that is not exact; storage
    // refused; and the entries asked what the protocol never asks them.
    void
    d_tests_storages()
    {
        typedef m::internal::expr_complex_storage storage;

        const m::complex<float>       zf(0.5f, -0.25f);
        const m::complex<long double> zl(0.1L, -0.0L);
        const m::value                vf = m::expr_complex_of(zf);
        const m::value                vl = m::expr_complex_of(zl);
        long double                   got = 0.0L;
        float                         fgot = 0.0f;

        D_TESTS_EXPECT( (d_tests_is_dense(-vf, -zf)) &&
                        (d_tests_is_dense(-vl, -zl)) &&
                        (abs(vf).kind() == D_MATH_VALUE_FLOAT) &&
                        (abs(vf).to(fgot)) &&
                        (fgot == abs(zf)) &&
                        (abs(vl).to(got)) &&
                        (got == abs(zl)) &&
                        (d_tests_is_dense(m::expr_conj(vf), conj(zf))) &&
                        (d_tests_is_dense(m::expr_conj(vl), conj(zl))) &&
                        (m::expr_imag(m::value(2.5L)).kind() ==
                             D_MATH_VALUE_LONG_DOUBLE),
                        "float and long double parts: -z, |z|, conj; a long "
                        "double's imaginary part a long double");
        D_TESTS_EXPECT( (d_tests_text(vl) ==
                             "0.100000000000000000001 - 0im") &&
                        (d_tests_rendered(vf) == "(0.5 - 0.25 * im)") &&
                        (d_tests_rendered(vl) == "(0.1 - 0 * im)") &&
                        (d_tests_rendered(m::expr_complex(m::value(1),
                                                          m::value(-0.5f))) ==
                             "(1 - 0.5 * im)"),
                        "the text and the renderer of float and long double "
                        "parts");

        // parts of a boxed kind that is not exact: kept as values
        const m::value lifted = d_tests_lifted(&g_failing);
        static const long double half = 0.5L;
        const m::value z = m::expr_complex(d_tests_lifted(&half),
                                           m::value(1));
        const m::value w = m::expr_complex(m::value(1),
                                           d_tests_lifted(&half));

        D_TESTS_EXPECT( (m::expr_is_complex(z)) &&
                        (d_tests_storage(z) == storage::values) &&
                        (!z.is_exact()) &&
                        (d_tests_text(z) == "lifted 0.50 + 1im") &&
                        (d_tests_rendered(w) == "(1 + lifted 0.50 * im)") &&
                        (d_tests_is_dense(z + m::value(1),
                                          m::complex<long double>(1.5L,
                                                                  1.0L))) &&
                        (abs(z).kind() == D_MATH_VALUE_LONG_DOUBLE),
                        "parts of a boxed kind not exact: kept as values, "
                        "not exact; its arithmetic on them as values");
        D_TESTS_EXPECT( (d_tests_is_error(m::expr_complex(lifted,
                                                          m::value(1)) +
                                              m::value(1),
                                          D_MATH_VALUE_ERROR_RANGE)) &&
                        (d_tests_is_error(m::expr_complex(m::value(1),
                                                          lifted) *
                                              d_tests_complex(0, 1, 1, 1),
                                          D_MATH_VALUE_ERROR_RANGE)) &&
                        (d_tests_is_error(m::expr_complex(m::value(1),
                                                          lifted) +
                                              d_tests_complex(0, 1, 1, 1),
                                          D_MATH_VALUE_ERROR_RANGE)),
                        "a part's error, either part's, passes");

        // a part that converts to nothing: no conversion
        const m::value opaque = m::expr_complex(d_tests_lifted(&g_opaque),
                                                m::value(0));
        float          f = 7.0f;
        double         d = 7.0;
        long double    l = 7.0L;

        D_TESTS_EXPECT( (m::expr_is_complex(opaque)) &&
                        (!opaque.to(f)) &&
                        (!opaque.to(d)) &&
                        (!opaque.to(l)) &&
                        (f == 7.0f) &&
                        (d == 7.0) &&
                        (l == 7.0L),
                        "a real part of no conversion: none");

        // storage refused: a memory error
        g_refuse_storage = true;

        const m::value refused_exact = d_tests_complex(1, 2, 1, 1);
        const m::value refused_dense = m::expr_complex(m::value(0.5),
                                                       m::value(1.0));

        g_refuse_storage = false;

        D_TESTS_EXPECT( (d_tests_is_error(refused_exact,
                                          D_MATH_VALUE_ERROR_MEMORY)) &&
                        (d_tests_is_error(refused_dense,
                                          D_MATH_VALUE_ERROR_MEMORY)),
                        "storage refused: a memory error, exact or dense");

        // the entries asked what the protocol never asks
        const d_math_boxed_kind* kind = m::internal::expr_complex_kind();
        const m::value           i    = d_tests_complex(0, 1, 1, 1);
        const m::value           r    = m::expr_complex(m::value(2.0),
                                                        m::value(0.0));
        d_math_value             out  = d_math_value_none();
        int                      dir  = 9;
        d_math_value             no   = kind->binary(D_MATH_VALUE_OP_ADD,
                                                     i.c_value(),
                                                     m::value(true)
                                                         .c_value());

        D_TESTS_EXPECT( (no.kind == D_MATH_VALUE_ERROR) &&
                        (kind->cmp(i.c_value(),
                                   m::value(true).c_value()) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (!kind->convert(&out,
                                        &dir,
                                        r.c_value(),
                                        D_MATH_VALUE_RATIONAL)) &&
                        (!kind->convert(&out,
                                        &dir,
                                        r.c_value(),
                                        D_MATH_VALUE_INTEGER)) &&
                        (dir == 9),
                        "the entries: a boolean operand a domain error and "
                        "unordered; no exact conversion");

        // the five, asked of the evaluators: their principal values, the
        // face's in long double (S342)
        const m::value five_of[1] = { i };
        const m::complex<long double> unit(0.0L, 1.0L);

        D_TESTS_EXPECT( (d_tests_is_dense(m::internal::expr_value_fn(
                                              m::internal::expr_fn::log2,
                                              five_of,
                                              1),
                                          log2(unit))) &&
                        (d_tests_is_dense(m::internal::expr_value_fn(
                                              m::internal::expr_fn::expm1,
                                              five_of,
                                              1),
                                          expm1(unit))) &&
                        (d_tests_value_of("[1, 2] / im") ==
                             d_tests_value_of("[-im, -2im]")) &&
                        (d_tests_is_error(d_tests_value_of("[1, 2] - im"),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "the five's evaluators the principal values; a "
                        "matrix over a complex number each element's "
                        "quotient, less one a domain error (S349)");

        return;
    }


    //==========================================================================
    // 12. THE MODULE
    //==========================================================================

    // d_tests_module
    //   every part, from the module's floor.
    void
    d_tests_module()
    {
        d_tests_kind();
        d_tests_making();
        d_tests_exact_arithmetic();
        d_tests_dense_arithmetic();
        d_tests_unary();
        d_tests_comparison();
        d_tests_conversion();
        d_tests_kind_text();
        d_tests_parts();
        d_tests_matrices_made();
        d_tests_language();
        d_tests_not_yet_nodes();
        d_tests_in_types();
        d_tests_rendering();
        d_tests_storages();

        return;
    }
}  // namespace

#else

namespace
{
    //==========================================================================
    // 12. THE MODULE
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

    std::printf("t_expr_complex_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
