/*******************************************************************************
* djinterp [cpp/math]                           t_expr_complex_function_face.cpp
*
* Tests for what step 23's second part, in chunk 41, made of complex numbers
* in the language (math/expr/function.hpp, evaluate.hpp, complex.hpp):
*   - the table: log1p, expm1, asinh, acosh, atanh, sign, min and max, and
*     every function's evaluators in each complex type (S203, S339);
*   - exact values (S337): roots and rational powers where the principal
*     root is a Gaussian rational, on fixed points and on random Gaussian
*     rationals' powers, each root's power checked exactly and its argument
*     principal, and a power that is no root's never made exact; sign(z)
*     where the modulus is rational;
*   - dense values (S336): each function with a principal complex value, of
*     random complex arguments in float, double and long double, the face's
*     bits, as a value and in a complex type, the widest type taken and an
*     exact argument's long double;
*   - references: the long double values of exact arguments against mpmath,
*     within cplx.h's bounds (math_tests_expr_complex_refs.h);
*   - real arguments beyond their real domain (S272, S341): the principal
*     complex value at x + 0i, exact where it is a Gaussian rational; the
*     domain's edges, NaNs and infinities real; Annex G's values;
*   - the context's real_results (S340): a domain error for each, exact or
*     not, and nothing else changed;
*   - the eight new functions in each real type and as values: the C core's
*     plain forms, their exact points and poles; sign, min and max (S338);
*   - the fold in math::complex<T> (S339): a real term's bits those in T,
*     complex values the face's, the constants, the relations, the logic,
*     the transpose, the real line's functions, a bound value, a function of
*     no complex evaluator; exp2, log2, log10, log1p and expm1 where their
*     value is complex the face's from chunk 42 (S342), and not_yet for
*     linear algebra of a number not real.
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/math/expr/t_expr_complex_function_face.cpp -o t_cf
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_complex_function_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.01
*                                                            revised: 2026.11.02
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/evaluate.hpp"
// std
#include <cstdio>  // std::printf


// D_TESTS_HEADER_READ
//   1 where the header was read: at every level, as it defines its guard
// before its floor.
#if defined(DJINTERP_MATH_EXPR_EVALUATE_HPP)
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
#include <cmath>    // std::signbit, std::atan2
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../../inc/djinterp/c/modules/math/float_order.h"        // d_math_ld_steps
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/complex.hpp"   // the kind
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/function.hpp"  // the tables
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/parse.hpp"     // expr_parse
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/token.hpp"     // the decimals
#include "./math_tests_expr_complex_refs.h"                                   // the references

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

    // d_tests_stored
    //   _x stored in its type, as the 32-bit x87 holds a result wider until
    // it is stored: an expected value made comparable with a stored one.
    template<typename Type>
    Type
    d_tests_stored(
        Type _x
    )
    {
        volatile Type stored = _x;

        return stored;
    }

    // d_tests_kind_of
    //   the value kind of a type: float, double or long double.
    template<typename Type>
    d_math_value_kind
    d_tests_kind_of()
    {
        return ( (sizeof(Type) == sizeof(float))
                     ? D_MATH_VALUE_FLOAT
                     : ( (sizeof(Type) == sizeof(double))
                             ? D_MATH_VALUE_DOUBLE
                             : D_MATH_VALUE_LONG_DOUBLE ) );
    }

    // d_tests_dense_is
    //   whether a value is a dense complex number of Type parts, the same as
    // _z bit for bit.
    template<typename Type>
    bool
    d_tests_dense_is(
        const m::value&         _value,
        const m::complex<Type>& _z
    )
    {
        m::complex<Type> got;

        return ( (m::expr_is_complex(_value)) &&
                 (m::internal::expr_complex_type_of(_value.c_value()) ==
                      d_tests_kind_of<Type>()) &&
                 (m::expr_complex_to(_value, got)) &&
                 (d_tests_same_complex(got, _z)) );
    }

    // d_tests_parsed
    //   the term of a text, implied multiplication and the bars on; a term
    // made by default where it fails.
    m::expr_term
    d_tests_parsed(
        const char* _text
    )
    {
        m::expr_syntax syntax;

        syntax.implied_multiplication = true;
        syntax.absolute_bars          = true;

        const m::expr_result<m::expr_term> parsed =
            m::expr_parse(_text, syntax);

        return ( parsed.is_ok() ? parsed.value() : m::expr_term() );
    }

    // d_tests_value_of
    //   a text's value under the bindings and the context; none where the
    // fold fails.
    m::value
    d_tests_value_of(
        const char*            _text,
        const m::expr_bindings& _bindings = m::expr_bindings(),
        const m::expr_context&  _context  = m::expr_context()
    )
    {
        const m::expr_result<m::value> folded =
            m::expr_evaluate(d_tests_parsed(_text), _bindings, _context);

        return ( folded.is_ok() ? folded.value() : m::value() );
    }

    // d_tests_is_error
    //   whether a value is the error given.
    bool
    d_tests_is_error(
        const m::value&    _value,
        d_math_value_error _error
    )
    {
        return ( (_value.kind() == D_MATH_VALUE_ERROR) &&
                 (_value.c_value().code == static_cast<int>(_error)) );
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

    // d_tests_valued
    //   whether a text's value is the value given, by value.h's order, of
    // the same exactness.
    bool
    d_tests_valued(
        const char*            _text,
        const m::value&         _want,
        const m::expr_bindings& _bindings = m::expr_bindings()
    )
    {
        const m::value got = d_tests_value_of(_text, _bindings);

        return ( (got.is_number()) &&
                 (got.is_exact() == _want.is_exact()) &&
                 (got == _want) );
    }

    // d_tests_in
    //   a text's value in Type, real or complex, under the bindings; and
    // whether the fold answered not_yet with the message given.
    template<typename Type>
    m::expr_result<Type>
    d_tests_in(
        const char*            _text,
        const m::expr_bindings& _bindings = m::expr_bindings()
    )
    {
        return m::expr_evaluate<Type>(d_tests_parsed(_text), _bindings);
    }

    template<typename Type>
    bool
    d_tests_in_is(
        const char*             _text,
        const Type&             _want,
        const m::expr_bindings& _bindings = m::expr_bindings()
    )
    {
        const m::expr_result<Type> got = d_tests_in<Type>(_text, _bindings);

        return ( (got.is_ok()) &&
                 (d_tests_same_complex(got.value(), _want)) );
    }

    // d_tests_rng
    //   a generator of its own, seeded, so that every run draws the same
    // numbers: xorshift64*.
    struct d_tests_rng
    {
        unsigned long long state;

        unsigned long long
        next()
        {
            state ^= (state >> 12);
            state ^= (state << 25);
            state ^= (state >> 27);

            return (state * 2685821657736338717ULL);
        }

        // below
        //   a whole number in [0, _n).
        long
        below(
            long _n
        )
        {
            return static_cast<long>(next() % static_cast<unsigned long long>(
                                                  _n));
        }

        // part
        //   a part of a complex number in Type: m 2^e, |m| below 2^24 and e
        // in [-12, 12], so that no function's value overflows a float, and
        // a 0 of either sign one time in sixteen.
        template<typename Type>
        Type
        part()
        {
            const long mantissa = below(1L << 24) - (1L << 23);
            const int  exponent = static_cast<int>(below(25)) - 12 - 23;

            // a 0, of either sign
            if (below(16) == 0)
            {
                return ( (below(2) == 0) ? Type(0) : -Type(0) );
            }

            return static_cast<Type>(std::ldexp(static_cast<long double>(
                                                    mantissa),
                                                exponent));
        }
    };

    // d_tests_names, d_tests_face
    //   the fifteen functions with a principal complex value whose complex
    // form cplx.h has, by name; and one of them, by its index, of the
    // face's.
    const char* const d_tests_names[15] = {
        "sqrt", "exp", "log", "sin", "cos", "tan", "asin", "acos", "atan",
        "sinh", "cosh", "tanh", "asinh", "acosh", "atanh" };

    template<typename Type>
    m::complex<Type>
    d_tests_face(
        std::size_t             _k,
        const m::complex<Type>& _z
    )
    {
        // by index
        switch (_k)
        {
            case 0:  return sqrt(_z);
            case 1:  return exp(_z);
            case 2:  return log(_z);
            case 3:  return sin(_z);
            case 4:  return cos(_z);
            case 5:  return tan(_z);
            case 6:  return asin(_z);
            case 7:  return acos(_z);
            case 8:  return atan(_z);
            case 9:  return sinh(_z);
            case 10: return cosh(_z);
            case 11: return tanh(_z);
            case 12: return asinh(_z);
            case 13: return acosh(_z);
            default: break;
        }

        return atanh(_z);
    }

    // d_tests_steps
    //   the steps between two long doubles, either above the other.
    d_math_umax
    d_tests_steps(
        long double _a,
        long double _b
    )
    {
        return ( (_a <= _b) ? d_math_ld_steps(_a, _b)
                            : d_math_ld_steps(_b, _a) );
    }

    // d_tests_read
    //   a reference's text as long double's nearest.
    long double
    d_tests_read(
        const char* _text
    )
    {
        long double out = 0.0L;

        static_cast<void>(m::internal::expr_read_decimal(out,
                                                         std::string(_text)));

        return out;
    }


    //==========================================================================
    // 3.  THE TABLE
    //==========================================================================

    // d_tests_table
    //   the eight new functions, and every function's evaluators in each
    // complex type.
    void
    d_tests_table()
    {
        const m::expr_function_table& table = m::expr_standard_functions();
        const char* const             one[] = {
            "log1p", "expm1", "asinh", "acosh", "atanh", "sign" };
        bool                          all   = true;

        for (std::size_t k = 0; k < 6; ++k)
        {
            const m::expr_function* f = table.find(one[k]);

            all = ( (all) &&
                    (f != D_NULLPTR) &&
                    (f->min_arity == 1) &&
                    (f->max_arity == 1) );
        }

        D_TESTS_EXPECT( (all) &&
                        (table.find("min") != D_NULLPTR) &&
                        (table.find("min")->min_arity == 1) &&
                        (table.find("min")->max_arity == m::expr_variadic) &&
                        (table.find("max") != D_NULLPTR) &&
                        (table.find("max")->max_arity == m::expr_variadic) &&
                        (table.size() == 52),
                        "log1p, expm1, asinh, acosh, atanh and sign of one "
                        "argument, min and max of any number from 1 (S203, "
                        "S338), and transpose (S350): 52 functions");

        const char* const names[] = {
            "sqrt", "cbrt", "sin", "cos", "tan", "asin", "acos", "atan",
            "atan2", "sinh", "cosh", "tanh", "exp", "exp2", "log", "log2",
            "log10", "pow", "hypot", "fmod", "remainder", "floor", "ceil",
            "round", "trunc", "abs", "factorial", "complex", "real", "imag",
            "conj", "arg", "log1p", "expm1", "asinh", "acosh", "atanh",
            "sign", "min", "max", "piecewise", "all_of", "any_of", "det",
            "inv", "trace", "norm", "eig", "dot", "cross", "solve",
            "transpose" };

        all = true;

        for (std::size_t k = 0; k < 52; ++k)
        {
            const m::expr_function* f = table.find(names[k]);

            all = ( (all) &&
                    (f != D_NULLPTR) &&
                    (f->evaluate_complex_float != D_NULLPTR) &&
                    (f->evaluate_complex_double != D_NULLPTR) &&
                    (f->evaluate_complex_long_double != D_NULLPTR) );
        }

        D_TESTS_EXPECT( all,
                        "every standard function an evaluator in each "
                        "complex type (S339)");

        const m::expr_named_constant* im =
            m::expr_standard_constants().find("im");
        const m::expr_named_constant* pi =
            m::expr_standard_constants().find("pi");

        D_TESTS_EXPECT( (im != D_NULLPTR) &&
                        (pi != D_NULLPTR) &&
                        (d_tests_same_complex(im->as_complex_double,
                                              m::complex<double>(0.0, 1.0))) &&
                        (d_tests_same_complex(im->as_complex_float,
                                              m::complex<float>(0.0f,
                                                                1.0f))) &&
                        (d_tests_same_complex(
                             im->as_complex_long_double,
                             m::complex<long double>(0.0L, 1.0L))) &&
                        (d_tests_same_complex(pi->as_complex_double,
                                              m::complex<double>(
                                                  pi->as_double))),
                        "the constants' complex values: im's 0 + 1i, pi's "
                        "its real one and +0");

        return;
    }


    //==========================================================================
    // 4.  EXACT VALUES
    //==========================================================================

    // d_tests_principal
    //   whether the complex number _r of exact parts has a principal argument
    // for a q-th root: in (-pi / q, pi / q], within a long double's rounding.
    bool
    d_tests_principal(
        const m::value& _r,
        d_math_imax     _q
    )
    {
        long double re    = 0.0L;
        long double im    = 0.0L;
        const long double pi = 3.141592653589793238462643383279502884197L;

        static_cast<void>(m::expr_real(_r).to(re));
        static_cast<void>(m::expr_imag(_r).to(im));

        const long double angle = std::atan2(im, re);
        const long double edge  = pi / static_cast<long double>(_q);

        return ( (angle > -edge + 1e-12L) &&
                 (angle <= edge + 1e-12L) );
    }

    // d_tests_exact_values
    //   exact roots, rational powers and signs (S337).
    void
    d_tests_exact_values()
    {
        D_TESTS_EXPECT( (d_tests_valued("sqrt(2im)",
                                        d_tests_complex(1, 1, 1, 1))) &&
                        (d_tests_valued("sqrt(-2im)",
                                        d_tests_complex(1, 1, -1, 1))) &&
                        (d_tests_valued("sqrt(3 + 4im)",
                                        d_tests_complex(2, 1, 1, 1))) &&
                        (d_tests_valued("sqrt(-3 - 4im)",
                                        d_tests_complex(1, 1, -2, 1))) &&
                        (d_tests_valued("sqrt(-4)",
                                        d_tests_complex(0, 1, 2, 1))) &&
                        (d_tests_valued("sqrt(-1/4)",
                                        d_tests_complex(0, 1, 1, 2))) &&
                        (d_tests_valued("sqrt(-7/36 + 6/9im)",
                                        d_tests_complex(1, 2, 2, 3))) &&
                        (d_tests_valued("sqrt(0im)", m::value(0))),
                        "sqrt exact where the root is a Gaussian rational: "
                        "sqrt(2im) 1 + im, sqrt(-4) 2im");
        D_TESTS_EXPECT( (d_tests_valued("(2im) ^ (1/2)",
                                        d_tests_complex(1, 1, 1, 1))) &&
                        (d_tests_valued("(-4) ^ (3/2)",
                                        d_tests_complex(0, 1, -8, 1))) &&
                        (d_tests_valued("(-4) ^ (1/4)",
                                        d_tests_complex(1, 1, 1, 1))) &&
                        (d_tests_valued("(-2 + 2im) ^ (1/3)",
                                        d_tests_complex(1, 1, 1, 1))) &&
                        (d_tests_valued("(-2 + 2im) ^ (-2/3)",
                                        d_tests_complex(0, 1, -1, 2))) &&
                        (d_tests_valued("(-4) ^ 0.5",
                                        d_tests_complex(0, 1, 2, 1))),
                        "a rational power exact where the principal root is "
                        "a Gaussian rational, then raised exactly");

        const m::value cube = d_tests_value_of("(-27/8) ^ (2/3)");
        const m::value root = d_tests_value_of("sqrt(im)");

        D_TESTS_EXPECT( (m::expr_is_complex(cube)) &&
                        (!cube.is_exact()) &&
                        (m::expr_is_complex(root)) &&
                        (!root.is_exact()) &&
                        (m::internal::expr_complex_type_of(root.c_value()) ==
                             D_MATH_VALUE_LONG_DOUBLE),
                        "no Gaussian root: the long double's, (-27/8) ^ "
                        "(2/3) and sqrt(im) among them");
        D_TESTS_EXPECT( (d_tests_valued("sign(3 + 4im)",
                                        d_tests_complex(3, 5, 4, 5))) &&
                        (d_tests_valued("sign(-5/2 + 6im)",
                                        d_tests_complex(-5, 13, 12, 13))) &&
                        (!d_tests_value_of("sign(1 + im)").is_exact()) &&
                        (d_tests_valued("sign(-1/2)", m::value(-1))) &&
                        (d_tests_valued("sign(0)", m::value(0))) &&
                        (d_tests_valued("sign(7)", m::value(1))),
                        "sign: z / |z|, exact where the modulus is rational, "
                        "and of an exact real number -1, 0 or 1");

        // random Gaussian rationals' powers: the principal root of w^q a
        // Gaussian rational exactly where w u is principal for a unit u,
        // 1, im, -1 or -im, with u^q = 1, as each other root is w times a
        // q-th root of unity that is no Gaussian rational; then exact, that
        // root; a number past a power never made exact wrongly
        const m::value units[4] = {
            m::value(1), m::expr_complex(m::value(0), m::value(1)),
            m::value(-1), m::expr_complex(m::value(0), m::value(-1)) };
        d_tests_rng    rng      = { 20261101ULL };
        long           agree    = 0;
        long           roots    = 0;
        long           gaussian = 0;
        long           sound    = 0;
        const long     count    = 1500;

        for (long n = 0; n < count; ++n)
        {
            const d_math_imax q   = 2 + rng.below(4);
            const m::value    w   = d_tests_complex(rng.below(21) - 10,
                                                    1 + rng.below(6),
                                                    rng.below(21) - 10,
                                                    1 + rng.below(6));
            const m::value    z   = m::internal::expr_whole_power(w, q);
            const m::value    got = d_tests_value_of(
                                        "x ^ (1 / q)",
                                        m::at("x", z).at("q", q));
            const m::value    off = d_tests_value_of(
                                        "x ^ (1 / q)",
                                        m::at("x", z + m::value(1))
                                            .at("q", q));
            bool              has  = false;
            m::value          want;

            // the unit that makes w principal, where one does
            for (std::size_t k = 0; k < 4; ++k)
            {
                if ( (m::internal::expr_whole_power(units[k], q) ==
                          m::value(1)) &&
                     (d_tests_principal(w * units[k], q)) )
                {
                    has  = true;
                    want = w * units[k];
                }
            }

            gaussian += ( has ? 1 : 0 );
            agree    += ( ( has ? ( (got.is_exact()) &&
                                    (got == want) )
                                : ( (got.is_number()) &&
                                    (!got.is_exact()) ) ) ? 1 : 0 );
            roots    += ( ( (!got.is_exact()) ||
                            ( (m::internal::expr_whole_power(got, q) ==
                                   z) &&
                              (d_tests_principal(got, q)) ) ) ? 1 : 0 );
            sound    += ( ( (!off.is_exact()) ||
                            (m::internal::expr_whole_power(off, q) ==
                                 z + m::value(1)) ) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (agree == count) &&
                        (gaussian > count / 3) &&
                        (gaussian < count),
                        "1,500 random Gaussian rationals w to a power q "
                        "from 2 to 5: (w^q)^(1/q) exact exactly where its "
                        "principal root is a Gaussian rational, w times a "
                        "unit, and then that root");
        D_TESTS_EXPECT( (roots == count),
                        "each exact root's q-th power the number, its "
                        "argument principal");
        D_TESTS_EXPECT( (sound == count),
                        "and of each power plus 1, a root exact only where "
                        "its q-th power is the number");

        const m::value big  = d_tests_value_of(
            "sqrt(9223372036854775807 + 9223372036854775807im)");
        const m::value wide = d_tests_value_of(
            "sqrt(2im / 205891132094649)");

        D_TESTS_EXPECT( (m::expr_is_complex(big)) &&
                        (!big.is_exact()) &&
                        (wide.is_exact()) &&
                        (wide == d_tests_complex(1, 14348907, 1, 14348907)),
                        "no square root of a number of the family's largest "
                        "parts that is one: dense; sqrt(2im / 3^30) exactly "
                        "(1 + im) / 3^15, D w within the family");

        return;
    }


    //==========================================================================
    // 5.  DENSE VALUES
    //==========================================================================

    // d_tests_dense_in
    //   each of the fifteen functions and pow, of random complex arguments
    // of Type parts, as a value and in math::complex<Type>: the face's bits.
    template<typename Type>
    void
    d_tests_dense_in(
        d_tests_rng& _rng,
        const char*  _type
    )
    {
        const long count  = 200;
        long       valued = 0;
        long       typed  = 0;
        long       powers = 0;

        for (std::size_t k = 0; k < 15; ++k)
        {
            const std::string text = std::string(d_tests_names[k]) + "(x)";

            for (long n = 0; n < count; ++n)
            {
                const m::complex<Type> z(_rng.template part<Type>(),
                                         _rng.template part<Type>());
                const m::complex<Type> want = d_tests_face(k, z);
                const m::expr_bindings x    = m::at("x",
                                                    m::expr_complex_of(z));

                valued += ( d_tests_dense_is(d_tests_value_of(text.c_str(),
                                                              x),
                                             want) ? 1 : 0 );
                typed  += ( d_tests_in_is(text.c_str(), want, x) ? 1 : 0 );
            }
        }

        for (long n = 0; n < count; ++n)
        {
            const m::complex<Type> z(_rng.template part<Type>(),
                                     _rng.template part<Type>());
            const m::complex<Type> w(_rng.template part<Type>() /
                                         static_cast<Type>(4096),
                                     _rng.template part<Type>() /
                                         static_cast<Type>(4096));
            const m::complex<Type> want = pow(z, w);
            const m::expr_bindings xy   = m::at("x", m::expr_complex_of(z))
                                              .at("y", m::expr_complex_of(w));

            powers += ( ( (d_tests_dense_is(d_tests_value_of("x ^ y", xy),
                                            want)) &&
                          (d_tests_dense_is(d_tests_value_of("pow(x, y)", xy),
                                            want)) &&
                          (d_tests_in_is("x ^ y", want, xy)) ) ? 1 : 0 );
        }

        const std::string what = std::string("in ") + _type +
                                 ": the fifteen functions and pow of random "
                                 "complex arguments, the face's bits, as a "
                                 "value and in the complex type (S336)";

        D_TESTS_EXPECT( (valued == 15 * count) &&
                        (typed == 15 * count) &&
                        (powers == count),
                        what.c_str() );

        return;
    }

    // d_tests_dense_values
    //   each type, the widest of mixed arguments, and an exact argument's
    // long double.
    void
    d_tests_dense_values()
    {
        d_tests_rng rng = { 7ULL };

        d_tests_dense_in<float>(rng, "float");
        d_tests_dense_in<double>(rng, "double");
        d_tests_dense_in<long double>(rng, "long double");

        const m::complex<float>  zf(0.5f, -1.25f);
        const double             y = 0.375;
        const m::value           mixed =
            d_tests_value_of("pow(x, y)",
                             m::at("x", m::expr_complex_of(zf)).at("y", y));
        const m::value           exact = d_tests_value_of("sin(1 + 2im)");
        const m::value           half  = d_tests_value_of("exp(1/2 + im)");

        D_TESTS_EXPECT( (d_tests_dense_is(mixed,
                                          pow(m::complex<double>(0.5, -1.25),
                                              m::complex<double>(y)))) &&
                        (d_tests_dense_is(exact,
                                          sin(m::complex<long double>(
                                              1.0L, 2.0L)))) &&
                        (d_tests_dense_is(half,
                                          exp(m::complex<long double>(
                                              0.5L, 1.0L)))),
                        "the widest type of the arguments, a float's and a "
                        "double's double; exact arguments' long double, an "
                        "exact part its nearest there");

        const m::value flagged = d_tests_value_of("9223372036854775807 + 1");
        const m::value carried = d_tests_value_of(
            "exp(x + im)", m::at("x", flagged));

        D_TESTS_EXPECT( ((carried.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0) &&
                        (m::expr_is_complex(carried)),
                        "an argument's flags carried to the value");

        return;
    }


    //==========================================================================
    // 6.  REFERENCES
    //==========================================================================

    // d_tests_references
    //   the long double values of exact arguments, each part within its
    // function's bound of the exact value's nearest.
    void
    d_tests_references()
    {
        const std::size_t count  = sizeof(g_expr_complex_refs) /
                                   sizeof(g_expr_complex_refs[0]);
        std::size_t       within = 0;

        for (std::size_t k = 0; k < count; ++k)
        {
            const d_tests_expr_complex_ref& ref = g_expr_complex_refs[k];
            const m::value                  got = d_tests_value_of(ref.text);
            m::complex<long double>         z;
            const d_math_umax               bound =
                static_cast<d_math_umax>(ref.bound);
            const bool                      ok =
                ( (m::expr_is_complex(got)) &&
                  (m::internal::expr_complex_type_of(got.c_value()) ==
                       D_MATH_VALUE_LONG_DOUBLE) &&
                  (m::expr_complex_to(got, z)) &&
                  (d_tests_steps(z.real(), d_tests_read(ref.re)) <=
                       bound) &&
                  (d_tests_steps(z.imag(), d_tests_read(ref.im)) <=
                       bound) );

            if (!ok)
            {
                std::printf("  reference %s: %.21Lg %+.21Lg\n",
                            ref.text,
                            z.real(),
                            z.imag());
            }

            within += ( ok ? 1u : 0u );
        }

        D_TESTS_EXPECT( (within == count) &&
                        (count == 40),
                        "40 values of exact arguments, long double's, each "
                        "part within cplx.h's bound of the exact value "
                        "mpmath computes");

        return;
    }


    //==========================================================================
    // 7.  BEYOND THE REAL DOMAIN
    //==========================================================================

    // d_tests_beyond_in
    //   a function of a real argument beyond its real domain, of Type: the
    // face's at x + 0i, as a value and in the complex type; a NaN in Type.
    template<typename Type>
    bool
    d_tests_beyond_in(
        const char*             _text,
        Type                    _x,
        const m::complex<Type>& _want
    )
    {
        const m::expr_bindings  x    = m::at("x", m::value(_x));
        const m::expr_result<Type> real = d_tests_in<Type>(_text, x);

        return ( (d_tests_dense_is(d_tests_value_of(_text, x), _want)) &&
                 (d_tests_in_is(_text, _want, x)) &&
                 (real.is_ok()) &&
                 (real.value() != real.value()) );
    }

    // d_tests_beyond_real
    //   S272's values, the domain's edges and S341's side of the cut.
    void
    d_tests_beyond_real()
    {
        typedef m::complex<double> cd;

        d_tests_rng rng = { 99ULL };
        long        good = 0;

        for (long n = 0; n < 200; ++n)
        {
            const double x  = -std::fabs(rng.part<double>()) - 1.0e-300;
            const double y  = std::fabs(rng.part<double>()) + 1.25;
            const double u  = 0.75 - std::fabs(rng.part<double>());

            good += ( ( (d_tests_beyond_in("sqrt(x)", x, sqrt(cd(x)))) &&
                        (d_tests_beyond_in("log(x)", x, log(cd(x)))) &&
                        (d_tests_beyond_in("asin(x)", y, asin(cd(y)))) &&
                        (d_tests_beyond_in("acos(x)", -y, acos(cd(-y)))) &&
                        (d_tests_beyond_in("atanh(x)", y, atanh(cd(y)))) &&
                        (d_tests_beyond_in("acosh(x)", u, acosh(cd(u)))) &&
                        (d_tests_beyond_in("x ^ 0.25", x,
                                           pow(cd(x), cd(0.25)))) )
                          ? 1 : 0 );
        }

        D_TESTS_EXPECT( (good == 200),
                        "of 200 random doubles beyond the real domain, sqrt, "
                        "log, asin, acos, atanh, acosh and a power not "
                        "whole: the face's at x + 0i, as a value and in "
                        "math::complex<double>; a NaN in double (S272)");
        D_TESTS_EXPECT( (d_tests_valued("sqrt(-9/4)",
                                        d_tests_complex(0, 1, 3, 2))) &&
                        (d_tests_dense_is(d_tests_value_of("sqrt(-2)"),
                                          sqrt(m::complex<long double>(
                                              -2.0L)))) &&
                        (d_tests_dense_is(d_tests_value_of("log(-1)"),
                                          log(m::complex<long double>(
                                              -1.0L)))) &&
                        (d_tests_dense_is(d_tests_value_of("(-8) ^ (1/3)"),
                                          pow(m::complex<long double>(-8.0L),
                                              m::complex<long double>(
                                                  1.0L / 3.0L)))) &&
                        (d_tests_valued("cbrt(-8)", m::value(-2))) &&
                        (d_tests_valued("(-8) ^ 2", m::value(64))),
                        "of exact arguments: exact where the value is a "
                        "Gaussian rational, otherwise long double's; cbrt(-8) "
                        "-2, the real root, and (-8) ^ 2 real");

        const double   inf  = std::numeric_limits<double>::infinity();
        const double   nan  = std::numeric_limits<double>::quiet_NaN();
        const m::value root = d_tests_value_of("sqrt(x)", m::at("x", -0.0));
        const m::value logz = d_tests_value_of("log(x)", m::at("x", -0.0));
        double         zero = 1.0;

        static_cast<void>(root.to(zero));

        D_TESTS_EXPECT( (root.kind() == D_MATH_VALUE_DOUBLE) &&
                        (zero == 0.0) &&
                        (std::signbit(zero)) &&
                        (logz.kind() == D_MATH_VALUE_DOUBLE) &&
                        (logz == m::value(-inf)) &&
                        (d_tests_is_error(d_tests_value_of("log(0)"),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(d_tests_value_of("atanh(1)"),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(d_tests_value_of("atanh(-1)"),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_valued("acosh(1)", m::value(0))) &&
                        (d_tests_value_of("asin(x)", m::at("x", 1.0)) ==
                             m::value(d_tests_stored(d_math_asin(1.0)))),
                        "the domain's edges real: sqrt(-0.0) -0.0, log(-0.0) "
                        "-inf, log(0) and atanh(1) and atanh(-1) poles, "
                        "acosh(1) 0, asin(1.0) the C core's");

        const m::value nan_power = d_tests_value_of("(-2) ^ x",
                                                    m::at("x", nan));
        const m::value inf_power = d_tests_value_of("(-2) ^ x",
                                                    m::at("x", inf));
        const m::value whole     = d_tests_value_of("x ^ 3",
                                                    m::at("x", -2.0));
        const m::value cube      = d_tests_value_of("(-2) ^ x",
                                                    m::at("x", 3.0));

        D_TESTS_EXPECT( (nan_power.kind() == D_MATH_VALUE_DOUBLE) &&
                        (inf_power.kind() == D_MATH_VALUE_DOUBLE) &&
                        (inf_power == m::value(inf)) &&
                        (whole == m::value(-8.0)) &&
                        (whole.kind() == D_MATH_VALUE_DOUBLE) &&
                        (cube == m::value(-8.0)) &&
                        (cube.kind() == D_MATH_VALUE_DOUBLE),
                        "a negative base to a NaN, an infinite or a whole "
                        "power, exact or not: real");
        D_TESTS_EXPECT( (d_tests_dense_is(d_tests_value_of("sqrt(x)",
                                                           m::at("x", -inf)),
                                          m::complex<double>(0.0, inf))) &&
                        (d_tests_dense_is(d_tests_value_of("log(x)",
                                                           m::at("x", -inf)),
                                          log(m::complex<double>(-inf)))) &&
                        (std::isinf(log(m::complex<double>(-inf)).real())),
                        "Annex G's values: sqrt(-inf) +0 + inf i, log(-inf) "
                        "inf + pi i");

        const m::value as = d_tests_value_of("asin(2)");
        const m::value ac = d_tests_value_of("acos(2)");
        long double    as_im = 0.0L;
        long double    ac_im = 0.0L;

        static_cast<void>(m::expr_imag(as).to(as_im));
        static_cast<void>(m::expr_imag(ac).to(ac_im));

        D_TESTS_EXPECT( (as_im > 1.3L) &&
                        (ac_im < -1.3L),
                        "a real argument on a cut taken as x + 0i, the upper "
                        "side's value: asin(2) pi/2 + 1.317i, acos(2) "
                        "-1.317i, as C and Python give them (S341)");

        return;
    }


    //==========================================================================
    // 8.  REAL RESULTS
    //==========================================================================

    // d_tests_real_results
    //   the context's option (S340).
    void
    d_tests_real_results()
    {
        const m::expr_context real(true);
        const char* const     strays[] = {
            "sqrt(-4)", "log(-1)", "asin(2)", "acos(-3/2)", "atanh(2)",
            "acosh(1/2)", "(-8) ^ (1/3)", "pow(-2, 0.5)", "log2(-8)",
            "log10(-1)", "log1p(-2)" };
        bool                  all = true;

        for (std::size_t k = 0; k < 11; ++k)
        {
            all = ( (all) &&
                    (d_tests_is_error(d_tests_value_of(strays[k],
                                                       m::expr_bindings(),
                                                       real),
                                      D_MATH_VALUE_ERROR_DOMAIN)) );
        }

        D_TESTS_EXPECT( (all) &&
                        (d_tests_is_error(d_tests_value_of("sqrt(x)",
                                                           m::at("x", -4.0),
                                                           real),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(d_tests_value_of("log2(x)",
                                                           m::at("x", -8.0),
                                                           real),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "real_results: each value complex alone of real "
                        "operands a domain error, exact or not, log2, "
                        "log10 and log1p among them");
        D_TESTS_EXPECT( (m::expr_is_complex(d_tests_value_of(
                             "complex(1, 2)", m::expr_bindings(), real))) &&
                        (m::expr_is_complex(d_tests_value_of(
                             "sqrt(im)", m::expr_bindings(), real))) &&
                        (m::expr_is_complex(d_tests_value_of(
                             "3 + 4im", m::expr_bindings(), real))) &&
                        (d_tests_value_of("sqrt(4)", m::expr_bindings(),
                                          real) == m::value(2)) &&
                        (!m::expr_context().real_results),
                        "and nothing else: complex(x, y), a complex operand "
                        "and a real value as they were; off by default");

        const m::expr_result<double> in_double =
            d_tests_in<double>("sqrt(-4)");

        D_TESTS_EXPECT( (in_double.is_ok()) &&
                        (in_double.value() != in_double.value()) &&
                        (d_tests_in_is("sqrt(-4)",
                                       m::complex<double>(0.0, 2.0))),
                        "the typed folds, which have no context: a NaN in "
                        "double, 2i in math::complex<double>");

        return;
    }


    //==========================================================================
    // 9.  THE NEW FUNCTIONS
    //==========================================================================

    // d_tests_in_real
    //   a text's value in Type, x bound to _x; a NaN where the fold fails.
    template<typename Type>
    Type
    d_tests_in_real(
        const char* _text,
        Type        _x
    )
    {
        const m::expr_result<Type> got =
            d_tests_in<Type>(_text, m::at("x", m::value(_x)));

        return ( got.is_ok() ? got.value()
                             : std::numeric_limits<Type>::quiet_NaN() );
    }

    // d_tests_new_functions
    //   log1p, expm1, asinh, acosh, atanh, sign, min and max (S203, S338).
    void
    d_tests_new_functions()
    {
        d_tests_rng rng  = { 5ULL };
        long        same = 0;

        for (long n = 0; n < 300; ++n)
        {
            const double x = rng.part<double>() / 4096.0;
            const double y = std::fabs(x) + 1.0;
            const double t = x / (1.0 + std::fabs(x));

            same += ( ( (d_tests_same_bits(d_tests_in_real("log1p(x)",
                                                            std::fabs(x)),
                                           d_tests_stored(d_math_log1p(
                                               std::fabs(x))))) &&
                        (d_tests_same_bits(d_tests_in_real("expm1(x)", x),
                                           d_tests_stored(d_math_expm1(x)))) &&
                        (d_tests_same_bits(d_tests_in_real("asinh(x)", x),
                                           d_tests_stored(d_math_asinh(x)))) &&
                        (d_tests_same_bits(d_tests_in_real("acosh(x)", y),
                                           d_tests_stored(d_math_acosh(y)))) &&
                        (d_tests_same_bits(d_tests_in_real("atanh(x)", t),
                                           d_tests_stored(d_math_atanh(t)))) &&
                        (d_tests_value_of("expm1(x)", m::at("x", x)) ==
                             m::value(d_tests_stored(d_math_expm1(x)))) &&
                        (d_tests_value_of("atanh(x)", m::at("x", t)) ==
                             m::value(d_tests_stored(d_math_atanh(t)))) )
                          ? 1 : 0 );
        }

        D_TESTS_EXPECT( (same == 300),
                        "log1p, expm1, asinh, acosh and atanh of 300 random "
                        "doubles: the C core's plain forms, in double and "
                        "as values");
        D_TESTS_EXPECT( (d_tests_valued("log1p(0)", m::value(0))) &&
                        (d_tests_valued("expm1(0)", m::value(0))) &&
                        (d_tests_valued("asinh(0)", m::value(0))) &&
                        (d_tests_valued("acosh(1)", m::value(0))) &&
                        (d_tests_valued("atanh(0)", m::value(0))) &&
                        (d_tests_is_error(d_tests_value_of("log1p(-1)"),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (!d_tests_value_of("log1p(1)").is_exact()) &&
                        (m::expr_is_complex(d_tests_value_of("acosh(0)"))),
                        "exact at their one rational point each; log1p(-1) "
                        "a pole; elsewhere long double, acosh below 1 "
                        "complex");

        const float  nf  = std::numeric_limits<float>::quiet_NaN();
        const double nd  = std::numeric_limits<double>::quiet_NaN();

        D_TESTS_EXPECT( (d_tests_same_bits(d_tests_in_real("sign(x)", -0.0),
                                           -0.0)) &&
                        (d_tests_same_bits(d_tests_in_real("sign(x)", 0.0f),
                                           0.0f)) &&
                        (d_tests_same_bits(d_tests_in_real("sign(x)", -3.5L),
                                           -1.0L)) &&
                        (d_tests_same_bits(d_tests_in_real("sign(x)", nf),
                                           nf)) &&
                        (d_tests_value_of("sign(x)", m::at("x", 2.5f)) ==
                             m::value(1.0f)) &&
                        (d_tests_value_of("sign(x)", m::at("x", 2.5f))
                             .kind() == D_MATH_VALUE_FLOAT),
                        "sign: 1 or -1, a 0 or a NaN itself, in its type");
        D_TESTS_EXPECT( (d_tests_valued("min(3, 1/2, 2)",
                                        m::value::ratio(1, 2))) &&
                        (d_tests_valued("max(1, 0.5)", m::value(1))) &&
                        (d_tests_valued("min(5)", m::value(5))) &&
                        (d_tests_valued("min(x, 1/3)", m::value::ratio(1, 3),
                                        m::at("x", 0.5))) &&
                        (d_tests_value_of("max(x, 1/3)", m::at("x", 0.5)) ==
                             m::value(0.5)) &&
                        (d_tests_value_of("max(x, 1/3)", m::at("x", 0.5))
                             .kind() == D_MATH_VALUE_DOUBLE),
                        "min and max as values: the argument itself, exact "
                        "kept exact, of any number of arguments (S338)");

        const m::value low  = d_tests_value_of("min(0, x)",
                                               m::at("x", -0.0));
        const m::value high = d_tests_value_of("max(x, 0)",
                                               m::at("x", -0.0));
        const m::value not_a = d_tests_value_of("min(1, x, 2)",
                                                m::at("x", nd));
        const m::value flagged = d_tests_value_of("9223372036854775807 + 1");
        const m::value flags   = d_tests_value_of("min(x, 1)",
                                                  m::at("x", flagged));
        double         zero    = 1.0;

        static_cast<void>(low.to(zero));

        D_TESTS_EXPECT( (low.kind() == D_MATH_VALUE_DOUBLE) &&
                        (std::signbit(zero)) &&
                        (high.is_exact()) &&
                        (high == m::value(0)) &&
                        (not_a.kind() == D_MATH_VALUE_DOUBLE) &&
                        (!(not_a == not_a)) &&
                        ((flags.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0),
                        "-0 below 0, a NaN the first where any is one, "
                        "every argument's flags kept");
        D_TESTS_EXPECT( (d_tests_is_error(d_tests_value_of("min(im, 1)"),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(d_tests_value_of("max(true, 1)"),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(d_tests_value_of("max(1, 1/0, 2)"),
                                          D_MATH_VALUE_ERROR_POLE)),
                        "a complex number or a boolean a domain error; an "
                        "argument's error passes");

        const double six[4] = { 2.0, -0.0, 0.0, -1.5 };

        D_TESTS_EXPECT( (d_tests_same_bits(m::internal::expr_min_in(six, 3),
                                           -0.0)) &&
                        (d_tests_same_bits(m::internal::expr_max_in(six + 1, 2),
                                           0.0)) &&
                        (d_tests_same_bits(m::internal::expr_min_in(six, 4),
                                           -1.5)) &&
                        (d_tests_same_bits(d_tests_in_real("min(1, x)", nd),
                                           nd)) &&
                        (d_tests_same_bits(d_tests_in_real("max(x, 1, 7)",
                                                           3.0),
                                           7.0)) &&
                        (d_tests_same_bits(d_tests_in_real("min(x, 1)", 3.0),
                                           1.0)) &&
                        (d_tests_same_bits(d_tests_in_real("min(x, 1)",
                                                           3.0f),
                                           1.0f)) &&
                        (d_tests_same_bits(d_tests_in_real("max(x, 7)",
                                                           3.0L),
                                           7.0L)),
                        "in a type: IEEE 754-2019's minimum and maximum, -0 "
                        "below +0, a NaN where any argument is one");

        return;
    }


    //==========================================================================
    // 10. THE FOLD IN A COMPLEX TYPE
    //==========================================================================

    // d_tests_real_terms
    //   whether a real term's value in math::complex<Type> is its bits in
    // Type, plus +0i, x bound to _x.
    template<typename Type>
    bool
    d_tests_real_terms(
        Type _x
    )
    {
        const char* const terms[] = {
            "x^2 + 3*x - 1", "sin(x) / (1 + x^2)", "exp(-x) * cos(x)",
            "sqrt(abs(x)) + log(1 + x^2)", "x mod 3", "floor(x) + ceil(x)",
            "atan2(x, 2)", "hypot(x, 3)", "min(x, 1, -x)", "sign(x)",
            "(x > 0 and x < 10) or not (x = 1)",
            "piecewise(when(x > 0, x), otherwise(-x))", "pi * x",
            "e ^ x", "log1p(abs(x))", "x'", "factorial(3) + cbrt(x)",
            "real(x) + imag(x) + arg(x)", "max(x, 2) - asinh(x)" };
        const m::expr_bindings x = m::at("x", m::value(_x));
        bool                   all = true;

        for (std::size_t k = 0; k < 19; ++k)
        {
            const m::expr_result<Type> real = d_tests_in<Type>(terms[k], x);

            all = ( (all) &&
                    (real.is_ok()) &&
                    (d_tests_in_is(terms[k],
                                   m::complex<Type>(real.value()),
                                   x)) );
        }

        return all;
    }

    // d_tests_complex_fold
    //   expr_evaluate<math::complex<T> > (S339).
    void
    d_tests_complex_fold()
    {
        typedef m::complex<double> cd;

        d_tests_rng rng  = { 11ULL };
        long        real = 0;

        for (long n = 0; n < 100; ++n)
        {
            const double x = rng.part<double>();

            real += ( ( (d_tests_real_terms<double>(x)) &&
                        (d_tests_real_terms<float>(static_cast<float>(x))) &&
                        (d_tests_real_terms<long double>(
                             static_cast<long double>(x))) ) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (real == 100),
                        "19 real terms at 100 random points: in each "
                        "complex type the bits they have in its real one, "
                        "plus +0i");

        const double nan = std::numeric_limits<double>::quiet_NaN();

        D_TESTS_EXPECT( (d_tests_in_is("im", cd(0.0, 1.0))) &&
                        (d_tests_in_is("3 + 4im", cd(3.0, 4.0))) &&
                        (d_tests_in_is("(1 + 2im)(3 - im)",
                                       cd(1.0, 2.0) * cd(3.0, -1.0))) &&
                        (d_tests_in_is("2 * (1 + 0 * im)",
                                       2.0 * cd(1.0, 0.0))) &&
                        (d_tests_in_is("-(4 + 0im)", -cd(4.0, 0.0))) &&
                        (d_tests_in_is("sqrt(-4)", cd(0.0, 2.0))) &&
                        (d_tests_in_is("log(-1)", log(cd(-1.0)))) &&
                        (d_tests_in_is("(-8) ^ (1/3)",
                                       pow(cd(-8.0), cd(1.0 / 3.0)))) &&
                        (d_tests_in_is("im ^ 2", pow(cd(0.0, 1.0),
                                                     cd(2.0)))),
                        "im 0 + 1i; complex operands the face's, a real one "
                        "taken as itself; a real number's value beyond its "
                        "domain the face's at x + 0i, sqrt(-4) 2i");
        D_TESTS_EXPECT( (d_tests_in_is("im < 1", cd(nan))) &&
                        (d_tests_in_is("3 < 4", cd(1.0))) &&
                        (d_tests_in_is("im = im", cd(1.0))) &&
                        (d_tests_in_is("im != 1", cd(1.0))) &&
                        (d_tests_in_is("im and false", cd(0.0))) &&
                        (d_tests_in_is("im or false", cd(1.0))) &&
                        (d_tests_in_is("not (0im)", cd(1.0))) &&
                        (d_tests_in_is("(1 + im)'", cd(1.0, -1.0))) &&
                        (d_tests_in_is("+(1 + im)", cd(1.0, 1.0))) &&
                        (d_tests_in_is("im mod 2", cd(nan, nan))) &&
                        (d_tests_in_is("im!", cd(nan, nan))) &&
                        (d_tests_in_is("floor(im)", cd(nan, nan))) &&
                        (d_tests_in_is("floor(2.5)", cd(2.0))),
                        "relations by the parts and no order of a complex "
                        "number; logic; the conjugate; mod, ! and the real "
                        "line's functions of one a NaN");
        D_TESTS_EXPECT( (d_tests_in_is("abs(3 + 4im)", cd(5.0))) &&
                        (d_tests_in_is("real(3 + 4im)", cd(3.0))) &&
                        (d_tests_in_is("imag(3 + 4im)", cd(4.0))) &&
                        (d_tests_in_is("conj(3 + 4im)", cd(3.0, -4.0))) &&
                        (d_tests_in_is("arg(im)", cd(arg(cd(0.0, 1.0))))) &&
                        (d_tests_in_is("complex(1, 2)", cd(1.0, 2.0))) &&
                        (d_tests_in_is("complex(im, 2)", cd(nan, nan))) &&
                        (d_tests_in_is("sign(3 + 4im)",
                                       cd(3.0, 4.0) / 5.0)) &&
                        (d_tests_in_is("sign(0im)", cd(0.0, 0.0))) &&
                        (d_tests_in_is("min(im, 1)", cd(nan, nan))) &&
                        (d_tests_in_is("det(3)", cd(3.0))) &&
                        (d_tests_in_is("dot(2, 3)", cd(6.0))),
                        "the parts, the modulus, complex(x, y), sign, and "
                        "linear algebra of real numbers");

        const m::expr_bindings bound =
            m::at("z", m::expr_complex(m::value(1), m::value(2)))
                .at("t", true)
                .at("v", m::value())
                .at("w", m::expr_complex_of(m::complex<float>(0.5f, 0.0f)));

        D_TESTS_EXPECT( (d_tests_in_is("z * z", cd(-3.0, 4.0), bound)) &&
                        (d_tests_in_is("t + 1", cd(2.0), bound)) &&
                        (d_tests_in_is("v", cd(nan), bound)) &&
                        (d_tests_in_is("w", cd(0.5, 0.0), bound)) &&
                        (d_tests_in_is("z[1]", cd(nan), bound)) &&
                        (d_tests_in<cd>("[1, 2]").is_err()) &&
                        (d_tests_in<cd>("jm").is_err()),
                        "a bound value its nearest, a boolean 1, none a NaN, "
                        "a complex number of imaginary part 0 complex; "
                        "indexing a NaN, a literal bad_shape, jm not_yet");
        D_TESTS_EXPECT( (d_tests_in_is("im * im",
                                       m::complex<float>(0.0f, 1.0f) *
                                           m::complex<float>(0.0f, 1.0f))) &&
                        (d_tests_in_is("exp(1 + im)",
                                       exp(m::complex<long double>(1.0L,
                                                                   1.0L)))),
                        "and so in float and long double");

        const m::expr_bindings odd =
            m::at("n", nan)
                .at("u", m::expr_complex(m::value(nan), m::value(1.0)))
                .at("v", m::expr_complex(m::value(1.0), m::value(nan)));

        D_TESTS_EXPECT( (d_tests_in_is("im + 2", cd(0.0, 1.0) + 2.0)) &&
                        (d_tests_in_is("im - 2", cd(0.0, 1.0) - 2.0)) &&
                        (d_tests_in_is("im / 2", cd(0.0, 1.0) / 2.0)) &&
                        (d_tests_in_is("2 / im", 2.0 / cd(0.0, 1.0))) &&
                        (d_tests_in_is("2 - im", 2.0 - cd(0.0, 1.0))) &&
                        (d_tests_in_is("norm(-3)", cd(3.0))) &&
                        (d_tests_in_is("sqrt(n)", cd(nan), odd)) &&
                        (d_tests_in_is("u and true", cd(nan), odd)) &&
                        (d_tests_in_is("v and true", cd(nan), odd)) &&
                        (d_tests_in_is("(1 + 0im) * -1",
                                       cd(1.0, 0.0) * -1.0)),
                        "a real operand on either side taken as itself; "
                        "norm of a real number; a NaN given stays real; a "
                        "NaN part's truth unknown");
        D_TESTS_EXPECT( (m::expr_evaluate<cd>(m::expr_term()).is_err()) &&
                        (d_tests_in<cd>("y").is_err()) &&
                        (d_tests_in<cd>("f(1)").is_err()) &&
                        (d_tests_in<cd>("sqrt(1, 2)").is_err()) &&
                        (d_tests_in<cd>("1 + y").is_err()),
                        "the fold's errors in a complex type: no term, an "
                        "unbound variable, an unknown name, wrong_arity, an "
                        "operand's error passing");

        // a function with no complex evaluator: its real one, of real
        // arguments, and a NaN otherwise
        typedef m::internal::expr_typed<cd> typed;

        m::expr_function floor = *m::expr_standard_functions().find("floor");
        std::vector<typed::carrier> one(1);

        floor.evaluate_complex_double = D_NULLPTR;
        one[0] = typed::of_real(2.5);

        const typed::carrier of_real = typed::call(floor, one);

        one[0] = typed::of_complex(cd(2.5, 1.0));

        const typed::carrier of_complex = typed::call(floor, one);

        D_TESTS_EXPECT( (of_real.real) &&
                        (of_real.z.real() == 2.0) &&
                        (!of_complex.real) &&
                        (d_tests_same_complex(of_complex.z, cd(nan, nan))),
                        "a function of no complex evaluator: its real one of "
                        "real arguments, a NaN of a complex one");

        return;
    }

    // d_tests_evaluators
    //   the evaluators asked directly, as a caller of the table may: a
    // complex type's of real arguments the real ones', plus +0i; min and
    // max as values through expr_value_fn; a Gaussian root whose least
    // common denominator passes the family.
    void
    d_tests_evaluators()
    {
        typedef m::complex<double> cd;

        const m::expr_function_table& table = m::expr_standard_functions();
        const double                  nan   =
            std::numeric_limits<double>::quiet_NaN();
        const cd                      half[2] = { cd(2.5), cd(-1.5) };
        const cd                      mixed[2] = { cd(2.5), cd(1.0, 1.0) };
        const cd                      pair[2] = { cd(1.0), cd(1.0) };
        const cd                      norm[1] = { cd(3.0, 4.0) };

        D_TESTS_EXPECT( (d_tests_same_complex(
                             table.find("floor")->evaluate_complex_double(
                                 half, 1),
                             cd(2.0))) &&
                        (d_tests_same_complex(
                             table.find("atan2")->evaluate_complex_double(
                                 pair, 2),
                             cd(d_tests_stored(d_math_atan2(1.0, 1.0))))) &&
                        (d_tests_same_complex(
                             table.find("min")->evaluate_complex_double(
                                 half, 2),
                             cd(-1.5))) &&
                        (d_tests_same_complex(
                             table.find("max")->evaluate_complex_double(
                                 half, 2),
                             cd(2.5))) &&
                        (d_tests_same_complex(
                             table.find("max")->evaluate_complex_double(
                                 mixed, 2),
                             cd(nan, nan))) &&
                        (d_tests_same_complex(
                             table.find("norm")->evaluate_complex_double(
                                 norm, 1),
                             cd(5.0))),
                        "a complex type's evaluators of real arguments: the "
                        "real ones', plus +0i; min and max of a complex "
                        "number a NaN; norm the modulus");

        const m::value three[3] = { m::value(3), m::value::ratio(1, 2),
                                    m::value(2) };

        D_TESTS_EXPECT( (m::internal::expr_value_fn(m::internal::expr_fn::min,
                                                    three,
                                                    3) ==
                             m::value::ratio(1, 2)) &&
                        (m::internal::expr_value_fn(m::internal::expr_fn::max,
                                                    three,
                                                    3) == m::value(3)),
                        "min and max through expr_value_fn");

        const m::value lcm = d_tests_value_of(
            "sqrt(1/4294967291 + 1/4294967279im)");

        D_TESTS_EXPECT( (m::expr_is_complex(lcm)) &&
                        (!lcm.is_exact()),
                        "a root of parts whose least common denominator "
                        "passes the family: dense");

        return;
    }

    // d_tests_not_yet
    //   what chunks 42 and 43 computed where chunk 41 answered not_yet, in
    // a complex type.
    void
    d_tests_not_yet()
    {
        typedef m::complex<double> cd;

        const cd unit(0.0, 1.0);

        // the five where their value is complex: no longer not_yet, but
        // their principal values from chunk 42, the face's (S342)
        D_TESTS_EXPECT( (d_tests_in_is("exp2(im)", exp2(unit))) &&
                        (d_tests_in_is("log2(im)", log2(unit))) &&
                        (d_tests_in_is("log10(im)", log10(unit))) &&
                        (d_tests_in_is("log1p(im)", log1p(unit))) &&
                        (d_tests_in_is("expm1(im)", expm1(unit))) &&
                        (d_tests_in_is("log2(-8)", log2(cd(-8.0)))) &&
                        (d_tests_in_is("log2(8)", cd(3.0))) &&
                        (d_tests_in_is("expm1(0)", cd(0.0))),
                        "exp2, log2, log10, log1p and expm1 where their "
                        "value is complex: the face's, from chunk 42 "
                        "(S342); their real values real");
        D_TESTS_EXPECT( (d_tests_in_is("det(im)", unit)) &&
                        (d_tests_in_is("norm(3 + 4im)", cd(5.0))) &&
                        (d_tests_in_is("solve(2, im)", cd(0.0, 0.5))) &&
                        (d_tests_in_is("inv(2im)", 1.0 / cd(0.0, 2.0))) &&
                        (d_tests_in_is("dot(im, 2)", cd(0.0, -2.0))) &&
                        (d_tests_in_is("transpose(1 + im)", cd(1.0, 1.0))) &&
                        (d_tests_in_is("eig(im)", unit)),
                        "linear algebra of a number not real, in a complex "
                        "type: a scalar's, det, eig and transpose itself, "
                        "norm its modulus, inv its reciprocal, its 1 as "
                        "itself, dot(a, b) conj(a) b, solve(a, b) b / a "
                        "(S351)");

        return;
    }


    //==========================================================================
    // 11. THE MODULE
    //==========================================================================

    // d_tests_module
    //   every part, from the module's floor.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_HEADER_READ == 1),
                        "the module's headers read");

        d_tests_table();
        d_tests_exact_values();
        d_tests_dense_values();
        d_tests_references();
        d_tests_beyond_real();
        d_tests_real_results();
        d_tests_new_functions();
        d_tests_complex_fold();
        d_tests_evaluators();
        d_tests_not_yet();

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

    std::printf("t_expr_complex_function_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return ( (g_failures == 0) ? 0 : 1 );
}
