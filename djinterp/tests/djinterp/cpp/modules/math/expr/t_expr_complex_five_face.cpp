/*******************************************************************************
* djinterp [cpp/math]                               t_expr_complex_five_face.cpp
*
* Tests for what step 23's second part, in its second chunk, chunk 42, made
* of complex numbers in the language (math/expr/function.hpp,
* evaluate.hpp): exp2, log2, log10, log1p and expm1 of a complex number,
* cplx.h's own from chunk 42 (S342).
*   - dense values: each of the five, of random complex arguments in float,
*     double and long double, the face's bits, as a value and in a complex
*     type; an exact argument's long double;
*   - references: the long double values of exact arguments against mpmath,
*     within a step (math_tests_expr_five_refs.h);
*   - real arguments beyond their real domain (S272, S341): log2 and log10
*     of a negative number and log1p of one below -1, the principal complex
*     value at x + 0i, a power of the base's exponent exactly in the real
*     part; the domain's edges, NaNs and infinities real, Annex G's values;
*   - the context's real_results (S340): a domain error for each, exact or
*     not, and their real values unchanged;
*   - the fold in math::complex<T> (S339): a real term's bits those in T,
*     real ones beyond the domain the face's at x + 0i, complex ones the
*     face's;
*   - none of them exact of a complex argument, no value of theirs being a
*     Gaussian rational, and their exact real values kept; and none of them
*     answering not_yet, in either fold.
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/math/expr/t_expr_complex_five_face.cpp -o t_c5
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_complex_five_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.02
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
#include <cmath>    // std::signbit, std::ldexp, std::fabs
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <string>   // std::string
// djinterp
#include "../../../../../../inc/djinterp/c/modules/math/float_order.h"        // d_math_ld_steps
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/complex.hpp"   // the kind
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/function.hpp"  // the tables
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/parse.hpp"     // expr_parse
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/token.hpp"     // the decimals
#include "./math_tests_expr_five_refs.h"                                      // the references

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

    // d_tests_folded
    //   a text's fold to a value under the bindings and the context.
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
    //   a text's value under the bindings and the context; none where the
    // fold fails.
    m::value
    d_tests_value_of(
        const char*             _text,
        const m::expr_bindings& _bindings = m::expr_bindings(),
        const m::expr_context&  _context  = m::expr_context()
    )
    {
        const m::expr_result<m::value> folded =
            d_tests_folded(_text, _bindings, _context);

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

    // d_tests_valued
    //   whether a text's value is the value given, by value.h's order, of
    // the same exactness.
    bool
    d_tests_valued(
        const char*             _text,
        const m::value&         _want,
        const m::expr_bindings& _bindings = m::expr_bindings()
    )
    {
        const m::value got = d_tests_value_of(_text, _bindings);

        return ( (got.is_number()) &&
                 (got.is_exact() == _want.is_exact()) &&
                 (got == _want) );
    }

    // d_tests_in, d_tests_in_is
    //   a text's value in Type, real or complex, under the bindings; and
    // whether it is _want, bit for bit.
    template<typename Type>
    m::expr_result<Type>
    d_tests_in(
        const char*             _text,
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
        // in [-12, 12], so that no value of the five overflows a float, and
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
    //   the five, by name; and one of them, by its index, of the face's.
    const char* const d_tests_names[5] = {
        "exp2", "log2", "log10", "log1p", "expm1" };

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
            case 0:  return exp2(_z);
            case 1:  return log2(_z);
            case 2:  return log10(_z);
            case 3:  return log1p(_z);
            default: break;
        }

        return expm1(_z);
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
    // 3.  DENSE VALUES
    //==========================================================================

    // d_tests_dense_in
    //   each of the five, of random complex arguments of Type parts, as a
    // value and in math::complex<Type>: the face's bits.
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

        for (std::size_t k = 0; k < 5; ++k)
        {
            const std::string text = std::string(d_tests_names[k]) + "(x)";

            for (long n = 0; n < count; ++n)
            {
                const m::complex<Type> z(_rng.template part<Type>(),
                                         _rng.template part<Type>() +
                                             static_cast<Type>(0.5));
                const m::complex<Type> want = d_tests_face(k, z);
                const m::expr_bindings x    = m::at("x",
                                                    m::expr_complex_of(z));

                valued += ( d_tests_dense_is(d_tests_value_of(text.c_str(),
                                                              x),
                                             want) ? 1 : 0 );
                typed  += ( d_tests_in_is(text.c_str(), want, x) ? 1 : 0 );
            }
        }

        const std::string what = std::string("in ") + _type +
                                 ": exp2, log2, log10, log1p and expm1 of "
                                 "random complex arguments, the face's bits, "
                                 "as a value and in the complex type (S342)";

        D_TESTS_EXPECT( (valued == 5 * count) &&
                        (typed == 5 * count),
                        what.c_str() );

        return;
    }

    // d_tests_dense_values
    //   each type, and an exact argument's long double.
    void
    d_tests_dense_values()
    {
        typedef m::complex<long double> cl;

        d_tests_rng rng = { 4242ULL };

        d_tests_dense_in<float>(rng, "float");
        d_tests_dense_in<double>(rng, "double");
        d_tests_dense_in<long double>(rng, "long double");

        D_TESTS_EXPECT( (d_tests_dense_is(d_tests_value_of("exp2(1/3 + im)"),
                                          exp2(cl(1.0L / 3.0L, 1.0L)))) &&
                        (d_tests_dense_is(d_tests_value_of("log10(3 - 4im)"),
                                          log10(cl(3.0L, -4.0L)))) &&
                        (d_tests_dense_is(d_tests_value_of("expm1(x + im)",
                                                           m::at("x",
                                                                 0.25f)),
                                          expm1(m::complex<float>(0.25f,
                                                                  1.0f)))),
                        "an exact argument's value long double's, a float's "
                        "float's (S336)");

        return;
    }


    //==========================================================================
    // 4.  REFERENCES
    //==========================================================================

    // d_tests_references
    //   the long double values of exact arguments, each part within its
    // bound of mpmath's.
    void
    d_tests_references()
    {
        const std::size_t count = sizeof(g_expr_five_refs) /
                                  sizeof(g_expr_five_refs[0]);
        std::size_t       within = 0u;

        for (std::size_t i = 0u; i < count; ++i)
        {
            const d_tests_expr_complex_ref& ref = g_expr_five_refs[i];
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
                        (count == 26),
                        "26 values of exact arguments, long double's, each "
                        "part within a step of the exact value mpmath "
                        "computes");

        return;
    }


    //==========================================================================
    // 5.  BEYOND THE REAL DOMAIN
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
        const m::expr_bindings     x    = m::at("x", m::value(_x));
        const m::expr_result<Type> real = d_tests_in<Type>(_text, x);

        return ( (d_tests_dense_is(d_tests_value_of(_text, x), _want)) &&
                 (d_tests_in_is(_text, _want, x)) &&
                 (real.is_ok()) &&
                 (real.value() != real.value()) );
    }

    // d_tests_beyond_real
    //   S272's values for the three of the five that have them, the
    // domain's edges and S341's side of the cut.
    void
    d_tests_beyond_real()
    {
        typedef m::complex<double>      cd;
        typedef m::complex<long double> cl;

        d_tests_rng rng = { 77ULL };
        long        good = 0;

        for (long n = 0; n < 200; ++n)
        {
            const double x = -std::fabs(rng.part<double>()) - 1.0e-300;
            const double w = -std::fabs(rng.part<double>()) - 1.0 - 1.0e-9;

            good += ( ( (d_tests_beyond_in("log2(x)", x, log2(cd(x)))) &&
                        (d_tests_beyond_in("log10(x)", x, log10(cd(x)))) &&
                        (d_tests_beyond_in("log1p(x)", w, log1p(cd(w)))) )
                          ? 1 : 0 );
        }

        D_TESTS_EXPECT( (good == 200),
                        "of 200 random doubles beyond the real domain, log2 "
                        "and log10 below 0 and log1p below -1: the face's at "
                        "x + 0i, as a value and in math::complex<double>; a "
                        "NaN in double (S272)");

        const m::value l2 = d_tests_value_of("log2(-8)");
        const m::value lt = d_tests_value_of("log10(-1000)");
        long double    l2_re = 1.0L;
        long double    lt_re = 1.0L;

        static_cast<void>(m::expr_real(l2).to(l2_re));
        static_cast<void>(m::expr_real(lt).to(lt_re));

        D_TESTS_EXPECT( (d_tests_dense_is(l2, log2(cl(-8.0L)))) &&
                        (l2_re == 3.0L) &&
                        (d_tests_dense_is(lt, log10(cl(-1000.0L)))) &&
                        (lt_re == 3.0L) &&
                        (d_tests_dense_is(d_tests_value_of("log1p(-3)"),
                                          log1p(cl(-3.0L)))) &&
                        (d_tests_dense_is(d_tests_value_of("log10(-1/3)"),
                                          log10(cl(-1.0L / 3.0L)))),
                        "of exact arguments, long double's: log2(-8) and "
                        "log10(-1000) a real part of 3 exactly, the real "
                        "function's on the axis (S343)");

        const double   inf = std::numeric_limits<double>::infinity();
        const double   nan = std::numeric_limits<double>::quiet_NaN();
        const m::value at0 = d_tests_value_of("log2(x)", m::at("x", -0.0));
        const m::value at1 = d_tests_value_of("log1p(x)", m::at("x", -1.0));
        const m::value atn = d_tests_value_of("log10(x)", m::at("x", nan));

        D_TESTS_EXPECT( (at0.kind() == D_MATH_VALUE_DOUBLE) &&
                        (at0 == m::value(-inf)) &&
                        (at1.kind() == D_MATH_VALUE_DOUBLE) &&
                        (at1 == m::value(-inf)) &&
                        (atn.kind() == D_MATH_VALUE_DOUBLE) &&
                        (d_tests_is_error(d_tests_value_of("log2(0)"),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(d_tests_value_of("log10(0)"),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(d_tests_value_of("log1p(-1)"),
                                          D_MATH_VALUE_ERROR_POLE)),
                        "the domain's edges real: log2(-0.0) -inf, log1p(-1.0) "
                        "-inf, log10 of a NaN a NaN; log2(0), log10(0) and "
                        "log1p(-1) poles");
        D_TESTS_EXPECT( (d_tests_dense_is(d_tests_value_of("log2(x)",
                                                           m::at("x", -inf)),
                                          log2(cd(-inf)))) &&
                        (std::isinf(log2(cd(-inf)).real())) &&
                        (d_tests_dense_is(d_tests_value_of("log1p(x)",
                                                           m::at("x", -inf)),
                                          log1p(cd(-inf)))),
                        "Annex G's values: log2(-inf) inf + i pi/ln 2, "
                        "log1p(-inf) inf + pi i");

        long double up = 0.0L;

        static_cast<void>(m::expr_imag(l2).to(up));

        D_TESTS_EXPECT( (up > 4.53L) &&
                        (up < 4.54L),
                        "a real argument on the cut taken as x + 0i, the upper "
                        "side's value: log2(-8) 3 + 4.532i (S341)");

        return;
    }


    //==========================================================================
    // 6.  REAL RESULTS
    //==========================================================================

    // d_tests_real_results
    //   the context's option (S340).
    void
    d_tests_real_results()
    {
        const m::expr_context real(true);
        const char* const     strays[] = {
            "log2(-8)", "log2(-1/3)", "log10(-1)", "log10(-7/2)",
            "log1p(-2)", "log1p(-5/2)" };
        bool                  all = true;

        for (std::size_t k = 0; k < 6; ++k)
        {
            all = ( (all) &&
                    (d_tests_is_error(d_tests_value_of(strays[k],
                                                       m::expr_bindings(),
                                                       real),
                                      D_MATH_VALUE_ERROR_DOMAIN)) );
        }

        D_TESTS_EXPECT( (all) &&
                        (d_tests_is_error(d_tests_value_of("log2(x)",
                                                           m::at("x", -0.5),
                                                           real),
                                          D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(d_tests_value_of("log1p(x)",
                                                           m::at("x", -3.0f),
                                                           real),
                                          D_MATH_VALUE_ERROR_DOMAIN)),
                        "real results asked: log2 and log10 of a negative "
                        "number and log1p of one below -1 a domain error, "
                        "exact or not (S340)");

        const m::value two = d_tests_value_of("log2(8)", m::expr_bindings(),
                                              real);
        const m::value one = d_tests_value_of("log1p(-1/2)",
                                              m::expr_bindings(), real);
        const m::value its = d_tests_value_of("log1p(-1/2)");

        D_TESTS_EXPECT( (two.is_exact()) &&
                        (two == m::value(3)) &&
                        (one.is_number()) &&
                        (!m::expr_is_complex(one)) &&
                        (one == its) &&
                        (m::expr_is_complex(d_tests_value_of("exp2(im)",
                                                             m::expr_bindings(),
                                                             real))),
                        "their real values unchanged, log2(8) exactly 3; a "
                        "complex argument's value its own");

        return;
    }


    //==========================================================================
    // 7.  THE FOLD IN A COMPLEX TYPE
    //==========================================================================

    // d_tests_complex_fold
    //   expr_evaluate<math::complex<T> > (S339): a real term real, one
    // beyond the domain the face's at x + 0i.
    void
    d_tests_complex_fold()
    {
        typedef m::complex<float>       cf;
        typedef m::complex<double>      cd;
        typedef m::complex<long double> cl;

        D_TESTS_EXPECT( (d_tests_in_is("log2(8)", cd(3.0))) &&
                        (d_tests_in_is("exp2(x)",
                                       cf(d_tests_stored(d_math_exp2(0.5f))),
                                       m::at("x", 0.5))) &&
                        (d_tests_in_is("log1p(x)",
                                       cl(d_tests_stored(
                                           d_math_log1p(-0.25L))),
                                       m::at("x", -0.25))) &&
                        (d_tests_in_is("expm1(-0.0)", cd(-0.0))),
                        "a real term's value its real type's bits, plus "
                        "+0i, -0 kept");
        D_TESTS_EXPECT( (d_tests_in_is("log2(-8)", log2(cd(-8.0)))) &&
                        (d_tests_in_is("log10(x)", log10(cf(-0.5f)),
                                       m::at("x", -0.5))) &&
                        (d_tests_in_is("log1p(-2)", log1p(cl(-2.0L)))),
                        "a real term beyond the domain: the face's at x + 0i, "
                        "its NaN of a real argument not taken");
        D_TESTS_EXPECT( (d_tests_in_is("exp2(3 + 4im)", exp2(cd(3.0, 4.0)))) &&
                        (d_tests_in_is("log2(2im)", log2(cf(0.0f, 2.0f)))) &&
                        (d_tests_in_is("expm1(im) - log1p(-im)",
                                       expm1(cl(0.0L, 1.0L)) -
                                           log1p(cl(0.0L, -1.0L)))),
                        "complex values the face's, in each type");

        return;
    }


    //==========================================================================
    // 8.  EXACTNESS AND NOT_YET
    //==========================================================================

    // d_tests_exactness
    //   no value of the five of a complex argument is a Gaussian rational:
    // dense; their exact real values kept.
    void
    d_tests_exactness()
    {
        const char* const texts[] = {
            "exp2(im)", "log2(2im)", "log10(10im)", "log1p(-1 + im)",
            "expm1(im)", "log2(1 + im)", "log1p(-2)", "log10(-100)" };
        bool              dense = true;

        for (std::size_t k = 0; k < 8; ++k)
        {
            const m::value got = d_tests_value_of(texts[k]);

            dense = ( (dense) &&
                      (m::expr_is_complex(got)) &&
                      (!got.is_exact()) );
        }

        D_TESTS_EXPECT( (dense) &&
                        (d_tests_valued("log2(8)", m::value(3))) &&
                        (d_tests_valued("log10(100)", m::value(2))) &&
                        (d_tests_valued("exp2(3)", m::value(8))) &&
                        (d_tests_valued("log1p(0)", m::value(0))) &&
                        (d_tests_valued("expm1(0)", m::value(0))),
                        "the five of a complex argument dense, none a "
                        "Gaussian rational (S337); their exact real values "
                        "exact");

        return;
    }

    // d_tests_no_not_yet
    //   none of the five answers not_yet, in either fold.
    void
    d_tests_no_not_yet()
    {
        typedef m::complex<double> cd;

        const char* const texts[] = {
            "exp2(im)", "log2(-8)", "log10(1 + im)", "log1p(-2)",
            "expm1(3im)", "log2(x)", "log1p(x - 2)", "exp2(x') + 1" };
        const m::expr_bindings x = m::at("x", m::expr_complex(m::value(1),
                                                              m::value(2)));
        bool                   none = true;

        for (std::size_t k = 0; k < 8; ++k)
        {
            const m::expr_result<m::value> folded =
                d_tests_folded(texts[k], x);
            const m::expr_result<cd>       typed  =
                d_tests_in<cd>(texts[k], x);

            none = ( (none) &&
                     (folded.is_ok()) &&
                     (typed.is_ok()) );
        }

        D_TESTS_EXPECT( none,
                        "exp2, log2, log10, log1p and expm1 never not_yet, as "
                        "a value or in math::complex<double>: chunk 42's");

        return;
    }


    //==========================================================================
    // 9.  THE MODULE
    //==========================================================================

    // d_tests_module
    //   every part, from the module's floor.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_HEADER_READ == 1),
                        "the module's headers read");

        d_tests_dense_values();
        d_tests_references();
        d_tests_beyond_real();
        d_tests_real_results();
        d_tests_complex_fold();
        d_tests_exactness();
        d_tests_no_not_yet();

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

    std::printf("t_expr_complex_five_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return ( (g_failures == 0) ? 0 : 1 );
}
