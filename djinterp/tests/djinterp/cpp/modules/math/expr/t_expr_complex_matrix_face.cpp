/*******************************************************************************
* djinterp [cpp/math]                             t_expr_complex_matrix_face.cpp
*
* Tests for what step 23's third part, chunk 43, made of matrices of complex
* numbers in the language (math/expr/matrix.hpp, function.hpp,
* evaluate.hpp).
*   - storage (S348): exact and boxed elements as values; dense where each
*     is a float, a double, a long double or a complex number of such
*     parts, the widest type's complex numbers where any is, a real element
*     x then x + 0i;
*   - arithmetic (S349): dense sums, products and scalings in the widest
*     type, a real operand taken as itself (G.5), a complex product the
*     kernels'; exact ones exactly;
*   - A' the conjugate transpose and transpose(A) the transpose, real,
*     imag, conj and arg of a matrix and complex(A, B) element by element
*     (S350); indexing and blocks;
*   - linear algebra (S351): a dense matrix's det, inv and solve cmat.h's
*     bit for bit, trace the kernels', norm of the moduli, the face's
*     frobenius_norm and norm, dot conjugating its first operand, the
*     face's vdot; an exact matrix's exactly; the face's norms of values
*     holding complex numbers the language's, exactly (S355);
*   - eig (S352): a real matrix's complex pairs, a Hermitian matrix's real
*     eigenvalues, any other complex matrix's, decomp.h's and cmat.h's bit
*     for bit where dense, against mpmath's where exact, ordered by real
*     part, then by imaginary part; an exact matrix's Gaussian rational
*     eigenvalues exact; real results refusing a complex spectrum of real
*     operands.
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/expr/t_expr_complex_matrix_face.cpp  \
*         -o t_cm
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_complex_matrix_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.02
*                                                            revised: 2026.11.07
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
#include <cmath>    // std::signbit, std::fabs
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../../inc/djinterp/c/modules/math/cmat.h"             // d_cmat_*
#include "../../../../../../inc/djinterp/c/modules/math/decomp.h"           // d_decomp_eigen
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/matrix.hpp"  // the kind
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/parse.hpp"   // expr_parse
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/token.hpp"   // the decimals
#include "../../../../../../inc/djinterp/cpp/modules/math/linear_algebra/matrix.hpp"
                                                            // the face
#include "../../../../../../inc/djinterp/cpp/modules/math/linear_algebra/vector.hpp"
                                                            // the face
#include "./math_tests_expr_cmatrix_refs.h"                 // the references

namespace m  = djinterp::math;
namespace la = djinterp::math::linalg;

namespace
{
    typedef m::internal::expr_matrix_storage storage;

    //==========================================================================
    // 2.  HELPERS
    //==========================================================================

    // d_tests_same_bits, d_tests_same_complex
    //   whether two numbers of a type are the same, equal with one sign or
    // both NaNs; and two math::complex<T>, part by part.
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
    d_tests_same_complex(
        const m::complex<Type>& _a,
        const m::complex<Type>& _b
    )
    {
        return ( (d_tests_same_bits(_a.real(), _b.real())) &&
                 (d_tests_same_bits(_a.imag(), _b.imag())) );
    }

    // d_tests_value_of
    //   a text's value, implied multiplication on, x bound to _x, in the
    // context given; an empty value where the fold fails.
    m::value
    d_tests_value_of(
        const char*            _text,
        const m::value&        _x       = m::value(0),
        const m::expr_context& _context = m::expr_context()
    )
    {
        m::expr_syntax syntax;

        syntax.implied_multiplication = true;

        const m::expr_result<m::expr_term> parsed = m::expr_parse(_text,
                                                                  syntax);

        if (parsed.is_err())
        {
            return m::value();
        }

        const m::expr_result<m::value> folded =
            m::expr_evaluate(parsed.value(),
                             m::at("x", _x),
                             _context);

        return ( folded.is_ok() ? folded.value() : m::value() );
    }

    // d_tests_storage_of
    //   a matrix's storage, values where it is no matrix.
    storage
    d_tests_storage_of(
        const m::value& _x
    )
    {
        return ( m::expr_is_matrix(_x)
                     ? m::internal::expr_matrix_box_of(_x.c_value()).storage
                     : storage::values );
    }

    // d_tests_is_error
    //   whether a value is the error given.
    bool
    d_tests_is_error(
        const m::value&     _x,
        dj_math_value_error _error
    )
    {
        dj_math_value_error code = D_MATH_VALUE_ERROR_MEMORY;

        return ( (_x.kind() == D_MATH_VALUE_ERROR) &&
                 (_x.to(code)) &&
                 (code == _error) );
    }

    // d_tests_complex_matrix, d_tests_real_matrix
    //   a matrix of the shape given, of math::complex<T> elements, each the
    // value of its own, or of T elements.
    template<typename Type>
    m::value
    d_tests_complex_matrix(
        std::size_t                          _rows,
        std::size_t                          _columns,
        const std::vector<m::complex<Type> >& _z
    )
    {
        std::vector<m::value> elements;

        for (std::size_t i = 0; i < _z.size(); ++i)
        {
            elements.push_back(m::expr_complex_of(_z[i]));
        }

        return m::expr_matrix(_rows, _columns, elements).value();
    }

    template<typename Type>
    m::value
    d_tests_real_matrix(
        std::size_t              _rows,
        std::size_t              _columns,
        const std::vector<Type>& _x
    )
    {
        std::vector<m::value> elements;

        for (std::size_t i = 0; i < _x.size(); ++i)
        {
            elements.push_back(m::value(_x[i]));
        }

        return m::expr_matrix(_rows, _columns, elements).value();
    }

    // d_tests_elements
    //   a matrix's elements, row by row; none where it is no matrix.
    std::vector<m::value>
    d_tests_elements(
        const m::value& _x
    )
    {
        std::vector<m::value> out;

        if (!m::expr_is_matrix(_x))
        {
            return out;
        }

        const m::expr_shape shape = m::expr_shape_of(_x).value();

        for (std::size_t r = 0; r < shape.rows; ++r)
        {
            for (std::size_t c = 0; c < shape.columns; ++c)
            {
                out.push_back(m::expr_element(_x, r, c).value());
            }
        }

        return out;
    }

    // d_tests_holds
    //   whether a value is a matrix of _rows by _columns of complex storage
    // of Type whose elements are _want's, bit for bit.
    template<typename Type>
    bool
    d_tests_holds(
        const m::value&                       _x,
        std::size_t                           _rows,
        std::size_t                           _columns,
        const std::vector<m::complex<Type> >& _want
    )
    {
        const storage               want_storage =
            m::internal::expr_matrix_storage_for<m::complex<Type> >();
        const std::vector<m::value> got = d_tests_elements(_x);
        bool                        ok  = ( (m::expr_is_matrix(_x)) &&
                                            (m::expr_shape_of(_x).value() ==
                                                 m::expr_shape(_rows,
                                                               _columns)) &&
                                            (got.size() == _want.size()) &&
                                            (d_tests_storage_of(_x) ==
                                                 want_storage) );

        for (std::size_t i = 0; ( (ok) && (i < got.size()) ); ++i)
        {
            m::complex<Type> z;

            ok = ( (m::expr_is_complex(got[i])) &&
                   (m::expr_complex_to(got[i], z)) &&
                   (d_tests_same_complex(z, _want[i])) );
        }

        return ok;
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

        // a whole number in [-_k, _k]
        long
        small(
            long _k
        )
        {
            return ( static_cast<long>(next() %
                                       static_cast<unsigned long long>(
                                           (2 * _k) + 1)) - _k );
        }

        // a number in [-1, 1), of 20 bits
        double
        unit()
        {
            return ( static_cast<double>(static_cast<long>(next() >> 44) -
                                         (1L << 19)) /
                     static_cast<double>(1L << 19) );
        }
    };

    template<typename Type>
    std::vector<m::complex<Type> >
    d_tests_random_complex(
        d_tests_rng& _rng,
        std::size_t  _count
    )
    {
        std::vector<m::complex<Type> > out;

        for (std::size_t i = 0; i < _count; ++i)
        {
            out.push_back(m::complex<Type>(static_cast<Type>(_rng.unit()),
                                           static_cast<Type>(_rng.unit())));
        }

        return out;
    }

    template<typename Type>
    std::vector<Type>
    d_tests_random_real(
        d_tests_rng& _rng,
        std::size_t  _count
    )
    {
        std::vector<Type> out;

        for (std::size_t i = 0; i < _count; ++i)
        {
            out.push_back(static_cast<Type>(_rng.unit()));
        }

        return out;
    }


    //==========================================================================
    // 3.  STORAGE
    //==========================================================================

    // d_tests_storage
    //   S348: where each element is dense, real or complex, the widest
    // type's dense storage, complex where any is; values otherwise.
    void
    d_tests_storage()
    {
        const m::value cd = m::expr_complex_of(m::complex<double>(1.5, -2.0));
        const m::value cf = m::expr_complex_of(m::complex<float>(0.5f, 1.0f));
        const m::value made = m::expr_matrix(1, 3, { m::value(2.0f), cd,
                                                     cf }).value();
        const std::vector<m::value> e = d_tests_elements(made);
        m::complex<double>          first;

        D_TESTS_EXPECT( (d_tests_storage_of(made) ==
                             storage::complex_doubles) &&
                        (e.size() == 3) &&
                        (m::expr_is_complex(e[0])) &&
                        (m::expr_complex_to(e[0], first)) &&
                        (d_tests_same_complex(first,
                                              m::complex<double>(2.0, 0.0))) &&
                        (e[1] == cd) &&
                        (e[2] == cf),
                        "a float, a complex double and a complex float: "
                        "complex doubles, the float 2 + 0i (S348)");
        D_TESTS_EXPECT( (d_tests_storage_of(d_tests_value_of("[1, im]")) ==
                             storage::values) &&
                        (d_tests_storage_of(m::expr_matrix(
                             1, 2, { m::value(1), cd }).value()) ==
                             storage::values) &&
                        (d_tests_storage_of(m::expr_matrix(
                             1, 2, { m::value(1.0L), cf }).value()) ==
                             storage::complex_long_doubles) &&
                        (d_tests_storage_of(m::expr_matrix(
                             1, 2, { cf, cf }).value()) ==
                             storage::complex_floats),
                        "exact elements values; a long double beside a "
                        "complex float complex long doubles; complex floats "
                        "their own");

        const m::value text = d_tests_value_of("[1, 2im; 3, 1/2 - im]");

        D_TESTS_EXPECT( (m::expr_is_matrix(text)) &&
                        (m::expr_element(text, 0, 1).value() ==
                             d_tests_value_of("2im")) &&
                        (m::expr_element(text, 1, 1).value() ==
                             d_tests_value_of("1/2 - im")) &&
                        (d_tests_value_of("[1, 2im; 3, 1/2 - im]") ==
                             d_tests_value_of("[1, 0 + 2im; 3, 1/2 - im]")) &&
                        (d_tests_value_of("[1, 2im] = [1, 2im]") ==
                             m::value(true)) &&
                        (d_tests_value_of("[1, 2im] = [1, -2im]") ==
                             m::value(false)),
                        "a literal of complex elements a matrix, each "
                        "element itself; = by the elements (S242)");

        return;
    }


    //==========================================================================
    // 4.  ARITHMETIC
    //==========================================================================

    // d_tests_dense_arithmetic
    //   S349, in Type: dense sums, products and scalings, a real operand
    // taken as itself, each element math::complex's operators', and a
    // product of complex matrices the kernels' product.
    template<typename Type>
    bool
    d_tests_dense_arithmetic(
        d_tests_rng& _rng
    )
    {
        typedef m::complex<Type> complex_type;
        typedef la::internal::linalg_kernel<complex_type> kernel;

        bool ok = true;

        for (int round = 0; round < 50; ++round)
        {
            const std::vector<complex_type> a = d_tests_random_complex<Type>(
                _rng, 6);
            const std::vector<complex_type> b = d_tests_random_complex<Type>(
                _rng, 6);
            const std::vector<complex_type> c = d_tests_random_complex<Type>(
                _rng, 6);
            const std::vector<Type>         r = d_tests_random_real<Type>(
                _rng, 6);
            const complex_type              z(static_cast<Type>(_rng.unit()),
                                              static_cast<Type>(_rng.unit()));
            const Type                      s = static_cast<Type>(
                _rng.unit());
            const m::value A = d_tests_complex_matrix<Type>(2, 3, a);
            const m::value B = d_tests_complex_matrix<Type>(2, 3, b);
            const m::value C = d_tests_complex_matrix<Type>(3, 2, c);
            const m::value R = d_tests_real_matrix<Type>(2, 3, r);
            const m::value Z = m::expr_complex_of(z);
            const m::value S(s);
            std::vector<complex_type> sum(6), difference(6), plus_real(6),
                                      real_less(6), negated(6), by_z(6),
                                      over_z(6), by_s(6), over_s(6),
                                      real_by_z(6);

            for (std::size_t i = 0; i < 6; ++i)
            {
                sum[i]        = a[i] + b[i];
                difference[i] = a[i] - b[i];
                plus_real[i]  = a[i] + r[i];
                real_less[i]  = r[i] - a[i];
                negated[i]    = -a[i];
                by_z[i]       = a[i] * z;
                over_z[i]     = a[i] / z;
                by_s[i]       = a[i] * s;
                over_s[i]     = a[i] / s;
                real_by_z[i]  = r[i] * z;
            }

            std::vector<complex_type> product(4);
            std::vector<complex_type> real_product(4);
            std::vector<complex_type> adjoint(6);
            std::vector<complex_type> transposed(6);

            kernel::multiply(product.data(), a.data(), c.data(), 2, 3, 2);

            // A' and transpose(A): each element to its place, conjugated
            // for A'
            for (std::size_t i = 0; i < 6; ++i)
            {
                adjoint[((i % 3) * 2) + (i / 3)]    = conj(a[i]);
                transposed[((i % 3) * 2) + (i / 3)] = a[i];
            }

            // the real operand's elements as themselves, summed from 0
            for (std::size_t i = 0; i < 2; ++i)
            {
                for (std::size_t j = 0; j < 2; ++j)
                {
                    complex_type acc;

                    for (std::size_t k = 0; k < 3; ++k)
                    {
                        acc += (r[(i * 3) + k] * c[(k * 2) + j]);
                    }

                    real_product[(i * 2) + j] = acc;
                }
            }

            ok = ( (ok) &&
                   (d_tests_holds<Type>(A + B, 2, 3, sum)) &&
                   (d_tests_holds<Type>(A - B, 2, 3, difference)) &&
                   (d_tests_holds<Type>(A + R, 2, 3, plus_real)) &&
                   (d_tests_holds<Type>(R - A, 2, 3, real_less)) &&
                   (d_tests_holds<Type>(-A, 2, 3, negated)) &&
                   (d_tests_holds<Type>(A * Z, 2, 3, by_z)) &&
                   (d_tests_holds<Type>(Z * A, 2, 3, by_z)) &&
                   (d_tests_holds<Type>(A / Z, 2, 3, over_z)) &&
                   (d_tests_holds<Type>(A * S, 2, 3, by_s)) &&
                   (d_tests_holds<Type>(A / S, 2, 3, over_s)) &&
                   (d_tests_holds<Type>(R * Z, 2, 3, real_by_z)) &&
                   (d_tests_holds<Type>(A * C, 2, 2, product)) &&
                   (d_tests_holds<Type>(R * C, 2, 2, real_product)) &&
                   (d_tests_holds<Type>(m::expr_transpose(A).value(), 3, 2,
                                        adjoint)) &&
                   (d_tests_holds<Type>(m::expr_transpose(A, false).value(),
                                        3, 2, transposed)) &&
                   (d_tests_is_error(A + Z, D_MATH_VALUE_ERROR_DOMAIN)) &&
                   (d_tests_is_error(A * B, D_MATH_VALUE_ERROR_DOMAIN)) );
        }

        return ok;
    }

    // d_tests_arithmetic
    //   S349 in each type; an exact matrix's arithmetic exactly; mixed
    // types in the wider.
    void
    d_tests_arithmetic()
    {
        d_tests_rng rng = { 0x43c0ffee43ULL };

        D_TESTS_EXPECT( (d_tests_dense_arithmetic<float>(rng)) &&
                        (d_tests_dense_arithmetic<double>(rng)) &&
                        (d_tests_dense_arithmetic<long double>(rng)),
                        "dense arithmetic in each type, 50 random rounds: "
                        "A + B, A - B, A + R, R - A, -A, A z, z A, A / z, "
                        "A s, A / s, R z, a real operand taken as itself; "
                        "A C the kernels' product, R C each real element as "
                        "itself; A' conjugating and transpose(A) not; A + z "
                        "and A B of no fitting shape a domain error (S246, "
                        "S349, S350)");

        const m::value cf = m::expr_complex_of(m::complex<float>(0.5f, 1.0f));
        const m::value mixed = ( m::expr_matrix(1, 1, { cf }).value() +
                                 m::expr_matrix(1, 1, { m::value(0.25) })
                                     .value() );
        m::complex<double> got;

        D_TESTS_EXPECT( (d_tests_storage_of(mixed) ==
                             storage::complex_doubles) &&
                        (m::expr_complex_to(m::expr_element(mixed, 0, 0)
                                                .value(),
                                            got)) &&
                        (d_tests_same_complex(got,
                                              m::complex<double>(0.75, 1.0))),
                        "a complex float matrix plus a double one: complex "
                        "doubles, the wider type's");

        // a real dense A by a complex number, exact or dense, and a
        // complex A times a real B: complex long doubles, each element by
        // math::complex's operators, a real operand taken as itself (G.5)
        typedef m::complex<long double> cld;

        const m::value R = d_tests_value_of("[sqrt(2), sqrt(3)]");
        const m::value S = d_tests_value_of("[sqrt(5); sqrt(7)]");
        cld            e;
        long double    x[2] = { 0.0L, 0.0L };
        long double    y[2] = { 0.0L, 0.0L };

        // the elements as the language computed them
        for (std::size_t i = 0; i < 2; ++i)
        {
            static_cast<void>(m::expr_element(R, 0, i).value().to(x[i]));
            static_cast<void>(m::expr_element(S, i, 0).value().to(y[i]));
        }

        static_cast<void>(m::expr_complex_to(d_tests_value_of("exp(im)"),
                                             e));

        const cld i_unit(0.0L, 1.0L);
        const cld one_i(1.0L, 1.0L);
        cld       sum;

        sum += (e * y[0]);
        sum += (cld(x[0]) * y[1]);

        D_TESTS_EXPECT( (d_tests_storage_of(R) == storage::long_doubles) &&
                        (d_tests_holds<long double>(
                             d_tests_value_of("[sqrt(2), sqrt(3)] * im"),
                             1, 2,
                             std::vector<cld>({ x[0] * i_unit,
                                                x[1] * i_unit }))) &&
                        (d_tests_holds<long double>(
                             d_tests_value_of("[sqrt(2), sqrt(3)] / "
                                              "(1 + im)"),
                             1, 2,
                             std::vector<cld>({ x[0] / one_i,
                                                x[1] / one_i }))) &&
                        (d_tests_holds<long double>(
                             d_tests_value_of("[exp(im), sqrt(2)] * "
                                              "[sqrt(5); sqrt(7)]"),
                             1, 1,
                             std::vector<cld>({ sum }))),
                        "a dense real A times and over an exact complex "
                        "number, and a complex A times a real B: complex "
                        "long doubles, each element by math::complex's "
                        "operators, the real operand as itself (S349)");
        D_TESTS_EXPECT( (d_tests_value_of("[1, im] + [im, 1]") ==
                             d_tests_value_of("[1 + im, 1 + im]")) &&
                        (d_tests_value_of("[1, im] * [1; im]") ==
                             d_tests_value_of("[0]")) &&
                        (d_tests_value_of("[1, im; 2, 3] * [im; 1/2]") ==
                             d_tests_value_of("[3/2im; 3/2 + 2im]")) &&
                        (d_tests_value_of("(1 + im) * [2, im]") ==
                             d_tests_value_of("[2 + 2im, -1 + im]")) &&
                        (d_tests_value_of("[2, 2im] / (1 + im)") ==
                             d_tests_value_of("[1 - im, 1 + im]")) &&
                        (d_tests_value_of("[1, im; -im, 1] ^ 2") ==
                             d_tests_value_of("[2, 2im; -2im, 2]")) &&
                        (d_tests_value_of("[1, im; 0, 1] ^ -1") ==
                             d_tests_value_of("[1, -im; 0, 1]")) &&
                        (d_tests_value_of("-[im, 1]") ==
                             d_tests_value_of("[-im, -1]")),
                        "exact arithmetic of Gaussian rationals exactly: "
                        "sums, products, scalings, quotients, powers and a "
                        "negative power, the inverse's");

        return;
    }


    //==========================================================================
    // 5.  TRANSPOSES, PARTS, INDEXING AND BLOCKS
    //==========================================================================

    // d_tests_shapes
    //   S350: A' conjugates, transpose(A) does not; the parts element by
    // element; indexing and blocks.
    void
    d_tests_shapes()
    {
        const std::vector<m::complex<double> > z = {
            m::complex<double>(1.0, 2.0), m::complex<double>(-0.5, 0.0),
            m::complex<double>(0.0, -3.0), m::complex<double>(4.0, -0.0) };
        const m::value A = d_tests_complex_matrix<double>(2, 2, z);
        const std::vector<m::complex<double> > adjoint = {
            conj(z[0]), conj(z[2]), conj(z[1]), conj(z[3]) };
        const std::vector<m::complex<double> > plain = {
            z[0], z[2], z[1], z[3] };

        D_TESTS_EXPECT( (d_tests_holds<double>(m::expr_transpose(A).value(),
                                               2, 2, adjoint)) &&
                        (d_tests_holds<double>(
                             m::expr_transpose(A, false).value(),
                             2, 2, plain)) &&
                        (d_tests_value_of("[1, 2im; im, 3]'") ==
                             d_tests_value_of("[1, -im; -2im, 3]")) &&
                        (d_tests_value_of("transpose([1, 2im; im, 3])") ==
                             d_tests_value_of("[1, im; 2im, 3]")) &&
                        (d_tests_value_of("transpose(1 + im)") ==
                             d_tests_value_of("1 + im")) &&
                        (d_tests_value_of("(1 + im)'") ==
                             d_tests_value_of("1 - im")) &&
                        (d_tests_value_of("[1, 2; 3, 4]'") ==
                             d_tests_value_of("transpose([1, 2; 3, 4])")),
                        "A' the conjugate transpose, each complex element "
                        "conjugated, a dense one by cplx.h's conj; "
                        "transpose(A) the transpose; of a real matrix the "
                        "same (S273, S350)");

        const m::value re = d_tests_value_of("real(x)", A);
        const m::value im = d_tests_value_of("imag(x)", A);
        const m::value cj = d_tests_value_of("conj(x)", A);
        const m::value ag = d_tests_value_of("arg(x)", A);
        const std::vector<m::value> res = d_tests_elements(re);
        const std::vector<m::value> ims = d_tests_elements(im);
        const std::vector<m::value> ags = d_tests_elements(ag);
        bool                        each = ( (res.size() == 4) &&
                                             (ims.size() == 4) &&
                                             (ags.size() == 4) );

        for (std::size_t i = 0; ( (each) && (i < 4) ); ++i)
        {
            double x = 0.0;
            double y = 0.0;
            double t = 0.0;

            each = ( (res[i].kind() == D_MATH_VALUE_DOUBLE) &&
                     (res[i].to(x)) &&
                     (ims[i].to(y)) &&
                     (ags[i].to(t)) &&
                     (d_tests_same_bits(x, z[i].real())) &&
                     (d_tests_same_bits(y, z[i].imag())) &&
                     (d_tests_same_bits(t, arg(z[i]))) );
        }

        D_TESTS_EXPECT( (each) &&
                        (d_tests_storage_of(re) == storage::doubles) &&
                        (d_tests_holds<double>(cj, 2, 2, std::vector<
                             m::complex<double> >({ conj(z[0]), conj(z[1]),
                                                    conj(z[2]),
                                                    conj(z[3]) }))) &&
                        (d_tests_value_of("complex([1, 2], [3, 1/2])") ==
                             d_tests_value_of("[1 + 3im, 2 + 1/2im]")) &&
                        (d_tests_value_of("complex([1, 0], [0, 0])") ==
                             d_tests_value_of("[1, 0]")) &&
                        (d_tests_is_error(
                             d_tests_value_of("complex([1, 2], [1])"),
                             D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(
                             d_tests_value_of("complex([1, 2], [1; 2])"),
                             D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_is_error(
                             d_tests_value_of("complex([1, im], [1, 1])"),
                             D_MATH_VALUE_ERROR_DOMAIN)),
                        "real, imag, conj and arg of a matrix element by "
                        "element, each the face's, real ones dense in the "
                        "parts' type; complex(A, B) of one shape pair by "
                        "pair, of two shapes or a complex part a domain "
                        "error (S350)");

        const m::value two = d_tests_value_of("[1, 2]");

        D_TESTS_EXPECT( (d_tests_is_error(
                             m::internal::expr_matrix_complex_of(
                                 m::value(D_MATH_VALUE_ERROR_RANGE), two),
                             D_MATH_VALUE_ERROR_RANGE)) &&
                        (d_tests_is_error(
                             m::internal::expr_matrix_complex_of(
                                 two, m::value(D_MATH_VALUE_ERROR_POLE)),
                             D_MATH_VALUE_ERROR_POLE)),
                        "complex(A, B) made in C++ of an error: the error, "
                        "the first, as the language's fold passes it first "
                        "(S350)");
        D_TESTS_EXPECT( (d_tests_value_of("[1, 2im; 3, 4][1, 2]") ==
                             d_tests_value_of("2im")) &&
                        (d_tests_value_of("[1, 2im; 3, 4][1]") ==
                             d_tests_value_of("[1, 2im]")) &&
                        (d_tests_value_of("[im, 2, 3][3]") ==
                             d_tests_value_of("3")) &&
                        (d_tests_value_of("[[1, im], 2]") ==
                             d_tests_value_of("[1, im, 2]")) &&
                        (d_tests_value_of("[[1, im]; [im, 1]]") ==
                             d_tests_value_of("[1, im; im, 1]")) &&
                        (d_tests_value_of("[[1; im], [2; 3]]") ==
                             d_tests_value_of("[1, 2; im, 3]")),
                        "indexing a complex element, a row of them; blocks "
                        "of complex numbers beside and above (S249, S261)");

        return;
    }


    //==========================================================================
    // 6.  LINEAR ALGEBRA
    //==========================================================================

    // d_tests_dense_algebra
    //   S351 in Type: det, inv and solve cmat.h's bit for bit, trace the
    // kernels', norm the face's frobenius_norm and norm, dot the face's
    // vdot, cross by its formula, a singular matrix's a pole.
    template<typename Type>
    bool
    d_tests_dense_algebra(
        d_tests_rng& _rng
    )
    {
        typedef m::complex<Type>                       complex_type;
        typedef typename complex_type::core_type       core_type;
        typedef la::internal::linalg_kernel<complex_type> kernel;

        bool ok = true;

        for (int round = 0; round < 30; ++round)
        {
            const std::vector<complex_type> a = d_tests_random_complex<Type>(
                _rng, 9);
            const std::vector<complex_type> b = d_tests_random_complex<Type>(
                _rng, 6);
            const std::vector<Type>         r = d_tests_random_real<Type>(
                _rng, 9);
            const m::value A = d_tests_complex_matrix<Type>(3, 3, a);
            const m::value B = d_tests_complex_matrix<Type>(3, 2, b);
            const m::value R = d_tests_real_matrix<Type>(3, 3, r);
            std::vector<core_type> ca, cb, cr, scratch(9), inverse(9),
                                   solution(6), real_solution(6);

            for (std::size_t i = 0; i < 9; ++i)
            {
                ca.push_back(a[i].c_value());
                cr.push_back(complex_type(r[i], Type(0)).c_value());
            }

            for (std::size_t i = 0; i < 6; ++i)
            {
                cb.push_back(b[i].c_value());
            }

            const complex_type det(d_cmat_determinant(ca.data(),
                                                      scratch.data(),
                                                      3));
            const bool         inverted = d_cmat_inverse(inverse.data(),
                                                         scratch.data(),
                                                         ca.data(),
                                                         3);
            const bool         solved   = d_cmat_solve(solution.data(),
                                                       scratch.data(),
                                                       ca.data(),
                                                       cb.data(),
                                                       3,
                                                       2);
            const bool         real_ok  = d_cmat_solve(real_solution.data(),
                                                       scratch.data(),
                                                       cr.data(),
                                                       cb.data(),
                                                       3,
                                                       2);
            std::vector<complex_type> inv(9), x(6), rx(6);

            for (std::size_t i = 0; i < 9; ++i)
            {
                inv[i] = complex_type(inverse[i]);
            }

            for (std::size_t i = 0; i < 6; ++i)
            {
                x[i]  = complex_type(solution[i]);
                rx[i] = complex_type(real_solution[i]);
            }

            la::matrix<complex_type, 3, 3> face;
            la::vector<complex_type, 3>    u;
            la::vector<complex_type, 3>    v;

            for (std::size_t i = 0; i < 9; ++i)
            {
                face(i / 3, i % 3) = a[i];
            }

            for (std::size_t i = 0; i < 3; ++i)
            {
                u[i] = a[i];
                v[i] = a[3 + i];
            }

            const m::value U = d_tests_complex_matrix<Type>(
                1, 3, std::vector<complex_type>(a.begin(), a.begin() + 3));
            const m::value V = d_tests_complex_matrix<Type>(
                1, 3, std::vector<complex_type>(a.begin() + 3,
                                                a.begin() + 6));
            const std::vector<complex_type> crossed = {
                (u[1] * v[2]) - (u[2] * v[1]),
                (u[2] * v[0]) - (u[0] * v[2]),
                (u[0] * v[1]) - (u[1] * v[0]) };
            const m::value W = d_tests_real_matrix<Type>(
                1, 3, std::vector<Type>(r.begin(), r.begin() + 3));
            const std::vector<complex_type> real_crossed = {
                (r[1] * v[2]) - (r[2] * v[1]),
                (r[2] * v[0]) - (r[0] * v[2]),
                (r[0] * v[1]) - (r[1] * v[0]) };
            const std::vector<complex_type> crossed_real = {
                (u[1] * r[2]) - (u[2] * r[1]),
                (u[2] * r[0]) - (u[0] * r[2]),
                (u[0] * r[1]) - (u[1] * r[0]) };
            complex_type real_dot;
            complex_type dot_real;

            // a real operand's elements as themselves, conj(r) = r
            for (std::size_t i = 0; i < 3; ++i)
            {
                real_dot += (r[i] * v[i]);
                dot_real += (conj(u[i]) * r[i]);
            }
            complex_type got_det;
            complex_type got_trace;
            complex_type got_dot;
            complex_type got_real_dot;
            complex_type got_dot_real;
            Type         got_norm   = Type(0);
            Type         got_vnorm  = Type(0);

            ok = ( (ok) &&
                   (inverted) &&
                   (solved) &&
                   (real_ok) &&
                   (m::expr_complex_to(m::expr_det(A).value(), got_det)) &&
                   (d_tests_same_complex(got_det, det)) &&
                   (d_tests_holds<Type>(m::expr_inv(A).value(), 3, 3, inv)) &&
                   (d_tests_holds<Type>(m::expr_solve(A, B).value(), 3, 2,
                                        x)) &&
                   (d_tests_holds<Type>(m::expr_solve(R, B).value(), 3, 2,
                                        rx)) &&
                   (m::expr_complex_to(m::expr_trace(A).value(), got_trace)) &&
                   (d_tests_same_complex(got_trace,
                                         kernel::trace(a.data(), 3))) &&
                   (m::expr_norm(A).value().to(got_norm)) &&
                   (d_tests_same_bits(got_norm, la::frobenius_norm(face))) &&
                   (m::expr_norm(U).value().to(got_vnorm)) &&
                   (d_tests_same_bits(got_vnorm, la::norm(u))) &&
                   (m::expr_complex_to(m::expr_dot(U, V).value(), got_dot)) &&
                   (d_tests_same_complex(got_dot, la::vdot(u, v))) &&
                   (d_tests_holds<Type>(m::expr_cross(U, V).value(), 1, 3,
                                        crossed)) &&
                   (m::expr_complex_to(m::expr_dot(W, V).value(),
                                       got_real_dot)) &&
                   (d_tests_same_complex(got_real_dot, real_dot)) &&
                   (m::expr_complex_to(m::expr_dot(U, W).value(),
                                       got_dot_real)) &&
                   (d_tests_same_complex(got_dot_real, dot_real)) &&
                   (d_tests_holds<Type>(m::expr_cross(W, V).value(), 1, 3,
                                        real_crossed)) &&
                   (d_tests_holds<Type>(m::expr_cross(U, W).value(), 1, 3,
                                        crossed_real)) );
        }

        // singular: a pole
        const std::vector<complex_type> twice = {
            complex_type(Type(1), Type(1)), complex_type(Type(2), Type(2)),
            complex_type(Type(2), Type(2)), complex_type(Type(4), Type(4)) };
        const m::value singular = d_tests_complex_matrix<Type>(2, 2, twice);

        return ( (ok) &&
                 (d_tests_is_error(m::expr_inv(singular).value(),
                                   D_MATH_VALUE_ERROR_POLE)) &&
                 (d_tests_is_error(m::expr_solve(singular,
                                                 singular).value(),
                                   D_MATH_VALUE_ERROR_POLE)) );
    }

    // d_tests_cofactors
    //   the test's own determinant of an exact square matrix: the cofactor
    // expansion along the first row.
    m::value
    d_tests_cofactors(
        const std::vector<m::value>& _a,
        std::size_t                  _n
    )
    {
        if (_n == 1)
        {
            return _a[0];
        }

        m::value sum(0);

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

            const m::value term = _a[j] * d_tests_cofactors(minor, _n - 1);

            sum = ( ((j % 2) == 0) ? (sum + term) : (sum - term) );
        }

        return sum;
    }

    // d_tests_gaussian
    //   a random Gaussian rational, of parts whole in [-3, 3] over 1 to 3.
    m::value
    d_tests_gaussian(
        d_tests_rng& _rng
    )
    {
        const long den = 1 + static_cast<long>(_rng.next() % 3);

        return m::expr_complex(m::value::ratio(_rng.small(3), den),
                               m::value::ratio(_rng.small(3), den));
    }

    // d_tests_face_moduli
    //   the C++ face's generic norms of values holding complex numbers,
    // against the language's: in a random round, a vector of three exact
    // Gaussian rationals or rationals, its norm_squared dot(u, u), its norm
    // and its distance to another the language's norm, and a 3 by 2
    // matrix's frobenius_norm too, exactly (S355); and a real vector's
    // norm_squared the face's own dot, as before. The rounds whose every
    // check holds.
    std::size_t
    d_tests_face_moduli(
        d_tests_rng& _rng,
        int          _count
    )
    {
        std::size_t held = 0;

        for (int round = 0; round < _count; ++round)
        {
            std::vector<m::value> ue;
            std::vector<m::value> ve;
            std::vector<m::value> we;
            std::vector<m::value> re;
            la::matrix<m::value, 3, 2> w;

            for (int i = 0; i < 3; ++i)
            {
                ue.push_back( ((_rng.next() % 3) == 0)
                                  ? m::value::ratio(_rng.small(4), 1 + i)
                                  : d_tests_gaussian(_rng) );
                ve.push_back(d_tests_gaussian(_rng));
                re.push_back( ((_rng.next() % 2) == 0)
                                  ? m::value(static_cast<double>(
                                        _rng.small(9)) / 7.0)
                                  : m::value::ratio(_rng.small(9), 5) );

                // the matrix's row i
                for (int j = 0; j < 2; ++j)
                {
                    we.push_back(d_tests_gaussian(_rng));
                    w(static_cast<std::size_t>(i),
                      static_cast<std::size_t>(j)) = we.back();
                }
            }

            const la::vector<m::value, 3> u(ue[0], ue[1], ue[2]);
            const la::vector<m::value, 3> v(ve[0], ve[1], ve[2]);
            const la::vector<m::value, 3> r(re[0], re[1], re[2]);
            const m::value U = m::expr_matrix(3, 1, ue).value();
            const m::value V = m::expr_matrix(3, 1, ve).value();
            const m::value W = m::expr_matrix(3, 2, we).value();

            held += ( ( (la::norm_squared(u) ==
                             m::expr_dot(U, U).value()) &&
                        (la::norm(u) == m::expr_norm(U).value()) &&
                        (la::distance(u, v) ==
                             m::expr_norm(U - V).value()) &&
                        (la::frobenius_norm(w) ==
                             m::expr_norm(W).value()) &&
                        (la::norm_squared(r) == la::dot(r, r)) )
                          ? 1 : 0 );
        }

        return held;
    }

    // d_tests_algebra
    //   S351 in each type; and an exact matrix's exactly.
    void
    d_tests_algebra()
    {
        d_tests_rng rng = { 0x5351a1eb7aULL };

        D_TESTS_EXPECT( (d_tests_dense_algebra<float>(rng)) &&
                        (d_tests_dense_algebra<double>(rng)) &&
                        (d_tests_dense_algebra<long double>(rng)),
                        "each dense type's algebra, 30 random rounds: det, "
                        "inv and solve cmat.h's bit for bit, a real A's "
                        "solve of complex B in complex numbers; trace the "
                        "kernels'; norm the face's frobenius_norm and norm; "
                        "dot the face's vdot, conjugating its first; cross "
                        "unconjugated; dot and cross of a real and a complex "
                        "vector, the real one's elements as themselves; a "
                        "singular matrix's a pole (S351, S354)");

        std::size_t determined = 0;
        std::size_t inverted   = 0;
        std::size_t singular   = 0;
        std::size_t solved     = 0;
        const int   count      = 200;
        const m::value eye = d_tests_value_of("[1, 0, 0; 0, 1, 0; 0, 0, 1]");

        for (int round = 0; round < count; ++round)
        {
            std::vector<m::value> a;
            std::vector<m::value> b;

            for (int i = 0; i < 9; ++i)
            {
                a.push_back( ((rng.next() % 3) == 0)
                                 ? m::value::ratio(rng.small(4), 1)
                                 : d_tests_gaussian(rng) );
            }

            for (int i = 0; i < 3; ++i)
            {
                b.push_back(d_tests_gaussian(rng));
            }

            const m::value A   = m::expr_matrix(3, 3, a).value();
            const m::value B   = m::expr_matrix(3, 1, b).value();
            const m::value det = d_tests_cofactors(a, 3);
            const m::value inv = m::expr_inv(A).value();
            const m::value x   = m::expr_solve(A, B).value();

            determined += ( (m::expr_det(A).value() == det) ? 1 : 0 );

            if (det == m::value(0))
            {
                singular += ( ( (d_tests_is_error(inv,
                                                  D_MATH_VALUE_ERROR_POLE)) &&
                                (d_tests_is_error(x,
                                                  D_MATH_VALUE_ERROR_POLE)) )
                                  ? 1 : 0 );
            }
            else
            {
                inverted += ( ( (inv * A == eye) &&
                                (d_tests_storage_of(inv) ==
                                     storage::values) ) ? 1 : 0 );
                solved   += ( (A * x == B) ? 1 : 0 );
            }
        }

        D_TESTS_EXPECT( (determined == static_cast<std::size_t>(count)) &&
                        (inverted + singular ==
                             static_cast<std::size_t>(count)) &&
                        (solved == inverted) &&
                        (inverted > 0),
                        "200 random exact matrices of Gaussian rationals: "
                        "det the test's own cofactor expansion, exactly; inv "
                        "and solve exact, inv(A) A = I and A x = b; a "
                        "singular one's a pole (S351)");
        D_TESTS_EXPECT( (d_tests_value_of("norm([3im, 4])") ==
                             m::value(5)) &&
                        (d_tests_value_of("norm([3im, 4])").is_exact()) &&
                        (d_tests_value_of("norm([1, im])").kind() ==
                             D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_value_of("dot([im, 1], [im, 2])") ==
                             m::value(3)) &&
                        (d_tests_value_of("dot([1, 1], [im, 2])") ==
                             d_tests_value_of("2 + im")) &&
                        (d_tests_value_of("trace([im, 1; 2, 3 - im])") ==
                             m::value(3)) &&
                        (d_tests_value_of("cross([im, 0, 0], [0, im, 0])") ==
                             d_tests_value_of("[0, 0, -1]")),
                        "exact norms, the root exact where it is rational; "
                        "dot conjugating its first; trace; cross "
                        "unconjugated (S351)");

        // a values matrix of an inexact real element and an exact complex
        // one in complex long double; a dense real A's solve of an exact
        // complex B in complex long double too; a complex norm past
        // double's range, of values, rescued by the scaled norm of the
        // parts (S351)
        typedef m::complex<long double>    cld;
        typedef cld::core_type             core_ld;

        long double root2 = 0.0L;

        static_cast<void>(d_tests_value_of("sqrt(2)").to(root2));

        const core_ld          mixed_cores[4] = {
            cld(root2).c_value(), cld(0.0L, 1.0L).c_value(),
            cld(1.0L).c_value(), cld(2.0L).c_value() };
        const core_ld          system_cores[4] = {
            cld(root2).c_value(), cld(1.0L).c_value(),
            cld(0.0L).c_value(), cld(2.0L).c_value() };
        const core_ld          right_cores[2] = {
            cld(0.0L, 2.0L).c_value(), cld(4.0L).c_value() };
        core_ld                scratch_ld[4];
        core_ld                solved_ld[2];
        std::vector<core_ld>   mixed_copy(mixed_cores, mixed_cores + 4);
        std::vector<core_ld>   system_copy(system_cores, system_cores + 4);
        const cld              mixed_det(d_cmat_determinant(
                                             mixed_copy.data(),
                                             scratch_ld,
                                             2));
        const bool             solved_ok = d_cmat_solve(solved_ld,
                                                        scratch_ld,
                                                        system_copy.data(),
                                                        right_cores,
                                                        2,
                                                        1);
        const m::value         far = m::expr_matrix(
            1, 2, { m::expr_complex(m::value(1e200), m::value(1e200)),
                    m::value(1) }).value();
        const la::vector<m::complex<double>, 2> far_face(
            m::complex<double>(1e200, 1e200), m::complex<double>(1.0, 0.0));
        cld                    got_det;
        double                 got_far = 0.0;

        D_TESTS_EXPECT( (solved_ok) &&
                        (m::expr_complex_to(
                             d_tests_value_of("det([sqrt(2), im; 1, 2])"),
                             got_det)) &&
                        (d_tests_same_complex(got_det, mixed_det)) &&
                        (d_tests_holds<long double>(
                             d_tests_value_of("solve([sqrt(2), 1; 0, 2], "
                                              "[2im; 4])"),
                             2, 1,
                             std::vector<cld>({ cld(solved_ld[0]),
                                                cld(solved_ld[1]) }))) &&
                        (d_tests_storage_of(far) == storage::values) &&
                        (m::expr_norm(far).value().to(got_far)) &&
                        (got_far <= std::numeric_limits<double>::max()) &&
                        (d_tests_same_bits(got_far, la::norm(far_face))),
                        "a matrix of values with an inexact element and an "
                        "exact complex one computed in complex long double, "
                        "cmat.h's; a dense real A's solve of an exact "
                        "complex B too; a norm past double's range of a "
                        "complex element rescued, the face's (S351)");

        const la::vector<m::value, 2> pair(d_tests_value_of("1 + im"),
                                           d_tests_value_of("1 - im"));

        D_TESTS_EXPECT( (d_tests_face_moduli(rng, 100) == 100u) &&
                        (la::norm(pair) == m::value(2)) &&
                        (la::norm(pair).is_exact()),
                        "the face's norm, norm_squared, distance and "
                        "frobenius_norm of values holding complex numbers, "
                        "100 random rounds: the language's norms, exactly; "
                        "norm([1 + im, 1 - im]) 2, where the squares' sum "
                        "is 0; a real vector's norm_squared its dot (S355)");

        return;
    }


    //==========================================================================
    // 7.  EIGENVALUES
    //==========================================================================

    // d_tests_part_of
    //   a reference part's long double.
    long double
    d_tests_part_of(
        const char* _text
    )
    {
        long double x = 0.0L;

        static_cast<void>(m::internal::expr_read_decimal(x,
                                                         _text));

        return x;
    }

    // d_tests_references
    //   each reference matrix's eig against mpmath's: as many, each within
    // 2^-56 of 1 + |A| of its own, in their order; a Hermitian one's real.
    void
    d_tests_references()
    {
        const std::size_t count = ( sizeof(g_expr_cmatrix_refs) /
                                    sizeof(g_expr_cmatrix_refs[0]) );
        std::size_t       near  = 0;
        std::size_t       real  = 0;

        for (std::size_t k = 0; k < count; ++k)
        {
            const dj_tests_cmatrix_ref& ref    = g_expr_cmatrix_refs[k];
            const m::value              eig    = d_tests_value_of(
                ("eig(" + std::string(ref.text) + ")").c_str());
            const std::vector<m::value> values = d_tests_elements(eig);
            const m::value              norm   = d_tests_value_of(
                ("norm(" + std::string(ref.text) + ")").c_str());
            long double                 size   = 0.0L;
            bool                        ok     = ( (values.size() == ref.n) &&
                                                   (norm.to(size)) );
            bool                        all_real = true;

            for (std::size_t i = 0; ( (ok) && (i < ref.n) ); ++i)
            {
                m::complex<long double> z;
                const long double       re = d_tests_part_of(ref.re[i]);
                const long double       im = d_tests_part_of(ref.im[i]);

                all_real = ( (all_real) &&
                             (!m::expr_is_complex(values[i])) );
                ok = ( (m::expr_complex_to(values[i], z)) &&
                       (std::fabs(z.real() - re) <=
                            (std::ldexp(1.0L, -56) * (1.0L + size))) &&
                       (std::fabs(z.imag() - im) <=
                            (std::ldexp(1.0L, -56) * (1.0L + size))) );
            }

            // a Hermitian matrix's: real, each imaginary part 0
            const bool hermitian = ( (std::string(ref.im[0]) == "0") &&
                                     (std::string(ref.text).find("im") !=
                                          std::string::npos) );

            near += ( ok ? 1 : 0 );
            real += ( ( (!hermitian) || (all_real) ) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (near == count) &&
                        (real == count) &&
                        (count >= 14),
                        "14 exact matrices' eigenvalues against mpmath's at "
                        "400 bits, each within 2^-56 of 1 + |A|, by real "
                        "part, then by imaginary part; a Hermitian one's "
                        "real (math_tests_expr_cmatrix_refs.h, S352)");

        return;
    }

    // d_tests_dense_eig
    //   S352 in Type: a real matrix with a complex pair decomp.h's
    // d_decomp_eigen's, a general complex one cmat.h's d_cmat_eigen's, bit
    // for bit; a Hermitian one real and ascending; NaNs of a NaN.
    template<typename Type>
    bool
    d_tests_dense_eig(
        d_tests_rng& _rng
    )
    {
        typedef m::complex<Type>                 complex_type;
        typedef typename complex_type::core_type core_type;

        bool ok = true;

        for (int round = 0; round < 20; ++round)
        {
            std::vector<Type>         r = d_tests_random_real<Type>(_rng, 9);
            const std::vector<complex_type> a =
                d_tests_random_complex<Type>(_rng, 9);
            std::vector<core_type>    found(3);
            std::vector<core_type>    general(3);
            std::vector<Type>         work(4);
            std::vector<Type>         rwork(7);
            std::vector<core_type>    ca;

            // a turn in the leading block, so that a pair is complex
            r[0] = Type(0);
            r[1] = Type(-1);
            r[3] = Type(1);
            r[4] = Type(0);

            const m::value R = d_tests_real_matrix<Type>(3, 3, r);
            const m::value A = d_tests_complex_matrix<Type>(3, 3, a);

            for (std::size_t i = 0; i < 9; ++i)
            {
                ca.push_back(a[i].c_value());
            }

            std::vector<Type> rr = r;

            static_cast<void>(d_decomp_eigen(rr.data(), 3, found.data(),
                                             D_NULLPTR, work.data(),
                                             D_NULLPTR, 1000, Type(0)));
            static_cast<void>(d_cmat_eigen(ca.data(), 3, general.data(),
                                           D_NULLPTR, D_NULLPTR,
                                           rwork.data(), D_NULLPTR, 1000,
                                           Type(0)));

            std::vector<complex_type> want_real(3);
            std::vector<complex_type> want_general(3);

            for (std::size_t i = 0; i < 3; ++i)
            {
                want_real[i]    = complex_type(found[i]);
                want_general[i] = complex_type(general[i]);
            }

            ok = ( (ok) &&
                   (d_tests_holds<Type>(m::expr_eig(R).value(), 1, 3,
                                        want_real)) &&
                   (d_tests_holds<Type>(m::expr_eig(A).value(), 1, 3,
                                        want_general)) );

            // Hermitian: A + A^H
            std::vector<complex_type> h(9);

            for (std::size_t i = 0; i < 3; ++i)
            {
                for (std::size_t j = 0; j < 3; ++j)
                {
                    h[(i * 3) + j] = ( (i == j)
                                           ? complex_type(a[(i * 3) + j]
                                                              .real())
                                           : ( (i < j)
                                                   ? a[(i * 3) + j]
                                                   : conj(a[(j * 3) + i]) ) );
                }
            }

            const m::value              H  = d_tests_complex_matrix<Type>(
                3, 3, h);
            const m::value              eh = m::expr_eig(H).value();
            const std::vector<m::value> ev = d_tests_elements(eh);
            Type                        sum = Type(0);
            Type                        previous = -std::numeric_limits<
                                                       Type>::infinity();
            bool                        rising = (ev.size() == 3);

            for (std::size_t i = 0; ( (rising) && (i < 3) ); ++i)
            {
                Type x = Type(0);

                rising = ( (!m::expr_is_complex(ev[i])) &&
                           (ev[i].to(x)) &&
                           (x >= previous) );
                previous = x;
                sum     += x;
            }

            const Type trace = ( h[0].real() + h[4].real() + h[8].real() );

            ok = ( (ok) &&
                   (rising) &&
                   (d_tests_storage_of(eh) ==
                        m::internal::expr_matrix_storage_for<Type>()) &&
                   (std::fabs(sum - trace) <=
                        (Type(64) * std::numeric_limits<Type>::epsilon() *
                         (Type(1) + std::fabs(trace)))) );
        }

        const Type nan = std::numeric_limits<Type>::quiet_NaN();
        const std::vector<complex_type> with_nan = {
            complex_type(nan, Type(0)), complex_type(Type(1), Type(1)),
            complex_type(Type(0), Type(1)), complex_type(Type(2), Type(0)) };
        const std::vector<m::value> nans = d_tests_elements(m::expr_eig(
            d_tests_complex_matrix<Type>(2, 2, with_nan)).value());
        m::complex<Type> first;

        return ( (ok) &&
                 (nans.size() == 2) &&
                 (m::expr_complex_to(nans[0], first)) &&
                 (first.real() != first.real()) );
    }

    // d_tests_eig
    //   S352: the references; each dense type; exact Gaussian eigenvalues;
    // real results.
    void
    d_tests_eig()
    {
        d_tests_rng rng = { 0x5352e16e00ULL };

        d_tests_references();

        D_TESTS_EXPECT( (d_tests_dense_eig<float>(rng)) &&
                        (d_tests_dense_eig<double>(rng)) &&
                        (d_tests_dense_eig<long double>(rng)),
                        "each dense type, 20 random rounds: a real matrix "
                        "with a complex pair d_decomp_eigen's, a general "
                        "complex one d_cmat_eigen's, bit for bit, complex "
                        "throughout; a Hermitian one real, ascending, its "
                        "sum the trace; a NaN's NaNs (S352)");
        const m::value zero = d_tests_complex_matrix<double>(
            2, 2, std::vector<m::complex<double> >(4));
        const m::value zero_eig = m::expr_eig(zero).value();

        D_TESTS_EXPECT( (d_tests_storage_of(zero_eig) == storage::doubles) &&
                        (zero_eig == d_tests_value_of("[0, 0]")),
                        "a dense complex zero matrix, Hermitian: its "
                        "eigenvalues real zeros (S352)");
        D_TESTS_EXPECT( (m::internal::expr_eigen_budget(2) == 1000u) &&
                        (m::internal::expr_eigen_budget(33) == 1000u) &&
                        (m::internal::expr_eigen_budget(34) == 1020u) &&
                        (m::internal::expr_eigen_budget(700) == 21000u),
                        "the solvers' steps: 30 a row, at least 1000, so "
                        "that a matrix of 700 rows has its eigenvalues "
                        "(S352)");
        D_TESTS_EXPECT( (d_tests_value_of("eig([0, -1; 1, 0])") ==
                             d_tests_value_of("[-im, im]")) &&
                        (d_tests_value_of("eig([1, 2im; 0, 1 + im])") ==
                             d_tests_value_of("[1, 1 + im]")) &&
                        (d_tests_value_of("eig([2, im; -im, 2])") ==
                             d_tests_value_of("[1, 3]")) &&
                        (d_tests_value_of("eig([0, -1, 0, 0; 1, 0, 0, 0; "
                                          "0, 0, 1, -2; 0, 0, 2, 1])") ==
                             d_tests_value_of("[-im, im, 1 - 2im, "
                                              "1 + 2im]")) &&
                        (d_tests_value_of("eig(1 + im)") ==
                             d_tests_value_of("1 + im")),
                        "an exact matrix's Gaussian rational eigenvalues "
                        "exact, by real part, then by imaginary part; a "
                        "number's itself (S352)");

        std::size_t exact = 0;
        const int   count = 100;

        // P D P^-1, P = [1, p; q, 1 + p q], unimodular, of Gaussian
        // integers, so that A is full and its eigenvalues are found
        // inexactly; D Gaussian, and every fourth round a Jordan block
        // [d, 1; 0, d], whose eigenvalue is repeated and defective
        for (int round = 0; round < count; ++round)
        {
            const bool     jordan = ( (round % 4) == 3 );
            const m::value d1 = d_tests_gaussian(rng);
            const m::value d2 = ( jordan ? d1 : d_tests_gaussian(rng) );
            const m::value p  = m::expr_complex(m::value(rng.small(1)),
                                                m::value(rng.small(1)));
            const m::value q  = m::expr_complex(m::value(rng.small(1)),
                                                m::value(rng.small(1)));
            const m::value P  = m::expr_matrix(2, 2, { m::value(1), p,
                                                       q,
                                                       m::value(1) +
                                                           (p * q) })
                                    .value();
            const m::value D  = m::expr_matrix(2, 2, { d1,
                                                       ( jordan
                                                             ? m::value(1)
                                                             : m::value(0) ),
                                                       m::value(0), d2 })
                                    .value();
            const m::value A  = (P * D) * m::expr_inv(P).value();
            const m::value e  = m::expr_eig(A).value();
            const std::vector<m::value> got = d_tests_elements(e);
            const bool     first = m::internal::expr_eigenvalue_before(d2,
                                                                       d1);
            const m::value low   = ( first ? d2 : d1 );
            const m::value high  = ( first ? d1 : d2 );

            exact += ( ( (got.size() == 2) &&
                         (got[0] == low) &&
                         (got[1] == high) &&
                         (got[0].is_exact()) &&
                         (got[1].is_exact()) ) ? 1 : 0 );
        }

        // three P J P^-1 of a 3 by 3 Jordan block [d, 1000, 0; 0, d, 1000;
        // 0, 0, d], d = k/1009, whose found copies of d lie some 10^-3 from
        // it, one of them nearer another multiple of 1/1009: d divided out
        // as often as it divides, from the copies that round to it
        D_TESTS_EXPECT( (d_tests_value_of("eig([-1008999/1009, 0, -1000; "
                                          "-1000, 1009001/1009, 0; "
                                          "1000, -1000, 1/1009])") ==
                             d_tests_value_of("[1/1009, 1/1009, 1/1009]")) &&
                        (d_tests_value_of("eig([1009002/1009, 0, -1000; "
                                          "-1000, 1009002/1009, 2000; "
                                          "1000, -1000, -2017998/1009])") ==
                             d_tests_value_of("[2/1009, 2/1009, 2/1009]")) &&
                        (d_tests_value_of("eig([-1008995/1009, 2000, -1000; "
                                          "1000, -1008995/1009, 1000; "
                                          "2000, -3000, 2018005/1009])") ==
                             d_tests_value_of("[5/1009, 5/1009, 5/1009]")),
                        "a defective triple eigenvalue exact thrice where "
                        "one of its found copies rounds elsewhere (S352)");
        D_TESTS_EXPECT( (exact == static_cast<std::size_t>(count)),
                        "100 random P D P^-1 of Gaussian rationals, P full "
                        "and unimodular, a quarter of them a Jordan block's: "
                        "eig D's diagonal, exactly, in order, a repeated "
                        "eigenvalue as often as it is repeated (S352)");

        const m::expr_context real_results(true);

        D_TESTS_EXPECT( (d_tests_is_error(
                             d_tests_value_of("eig([0, -1; 1, 0])",
                                              m::value(0),
                                              real_results),
                             D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_value_of("eig([2, 1; 1, 2])",
                                          m::value(0),
                                          real_results) ==
                             d_tests_value_of("[1, 3]")) &&
                        (d_tests_value_of("eig([1, 2im; 0, 1 + im])",
                                          m::value(0),
                                          real_results) ==
                             d_tests_value_of("[1, 1 + im]")),
                        "real results: a complex spectrum of a real matrix "
                        "a domain error, a real one's and a complex "
                        "matrix's their own (S340, S352)");

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

        d_tests_storage();
        d_tests_arithmetic();
        d_tests_shapes();
        d_tests_algebra();
        d_tests_eig();

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

    std::printf("t_expr_complex_matrix_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return ( (g_failures == 0) ? 0 : 1 );
}
