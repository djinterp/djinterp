/*******************************************************************************
* djinterp [cpp/math]                          t_expr_quaternion_matrix_face.cpp
*
* Tests for what step 24's third part, chunk 46, made of matrices of
* quaternions in the language (math/expr/matrix.hpp, rotation.hpp,
* quaternion.hpp, function.hpp, evaluate.hpp).
*   - storage (S386): exact and boxed elements as values; dense where each
*     is a float, a double, a long double or a complex number or a
*     quaternion of such parts, the widest type's quaternions where any is
*     one, a real element x then x + 0im + 0jm + 0km and a complex one
*     x + yi x + yim + 0jm + 0km, each part read back bit for bit;
*   - arithmetic (S387): dense sums, products and scalings in the widest
*     type, each element the quaternion kind's own operation on its two
*     elements, bit for bit, a real or complex operand taken as itself, a
*     product's each element summed from 0 in order; a number's product
*     with a matrix in its order, A / q being A inv(q); exact ones exactly;
*     powers, A ^ 0 the identity of A's storage, a complex one's too;
*   - A' conjugating each element, transpose(A) moving it, -A, real and
*     conj of a matrix, imag, arg and complex of one a domain error;
*     indexing and blocks;
*   - linear algebra (S388, S390): det a domain error; inv and solve the
*     generic kernels' on the kind's operands, bit for bit, exact for exact
*     matrices; a singular one a pole; trace, norm of the moduli, dot
*     conjugating its first operand, cross in order; solve(q, B) q^-1 B on
*     the left, quat.h's left quotient bit for bit;
*   - eig (S389): a quaternion's standard eigenvalue, w + |v|i; a
*     triangular matrix's its diagonal's; Zhang's standard right
*     eigenvalues against mpmath's, exact and dense, a Hermitian matrix's
*     real; one of each pair of the language's eig of the complex adjoint,
*     bit for bit; invariant under similarity, as they should be;
*   - rotations (S391): rotmat exact where q is and quat.h's to_matrix3
*     bit for bit where dense; rotquat exact where the root is rational and
*     quat.h's from_matrix3 bit for bit otherwise, of a 4 by 4 its upper
*     left; slerp quat.h's on the normalized operands, bit for bit; their
*     errors; NaNs in a type.
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*     tests/djinterp/cpp/modules/math/expr/t_expr_quaternion_matrix_face.cpp \
*         -o t_qm
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_quaternion_matrix_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.08
*                                                            revised: 2026.11.13
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
#include <cmath>    // std::signbit, std::fabs, std::ldexp
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/function.hpp"  // the tables
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/matrix.hpp"    // the kind
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/parse.hpp"     // expr_parse
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/rotation.hpp"  // rotations
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/token.hpp"     // the decimals
#include "../../../../../../inc/djinterp/cpp/modules/math/quaternion.hpp"     // the face
#include "./math_tests_expr_qmatrix_refs.h"                                   // the references

namespace m  = djinterp::math;
namespace la = djinterp::math::linalg;

namespace
{
    typedef m::internal::expr_matrix_storage storage;

    //==========================================================================
    // 2.  HELPERS
    //==========================================================================

    // d_tests_same_bits, d_tests_same_quaternion
    //   whether two numbers of a type are the same, equal with one sign or
    // both NaNs; and two math::quaternion<T>, part by part.
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

    // d_tests_q
    //   the math::quaternion<T> w + x im + y jm + z km.
    template<typename Type>
    m::quaternion<Type>
    d_tests_q(
        Type _w,
        Type _x,
        Type _y,
        Type _z
    )
    {
        return m::quaternion<Type>(_x, _y, _z, _w);
    }

    // d_tests_sum
    //   _a + _b, or _a - _b where _minus, part by part, as the kind adds
    // two quaternions.
    template<typename Type>
    m::quaternion<Type>
    d_tests_sum(
        const m::quaternion<Type>& _a,
        const m::quaternion<Type>& _b,
        bool                       _minus
    )
    {
        return ( _minus ? m::quaternion<Type>(_a.x - _b.x, _a.y - _b.y,
                                              _a.z - _b.z, _a.w - _b.w)
                        : m::quaternion<Type>(_a.x + _b.x, _a.y + _b.y,
                                              _a.z + _b.z, _a.w + _b.w) );
    }

    // d_tests_exact
    //   the exact number w + x im + y jm + z km, of whole parts.
    m::value
    d_tests_exact(
        long _w,
        long _x,
        long _y,
        long _z
    )
    {
        return m::expr_quaternion(m::value(_w),
                                  m::value(_x),
                                  m::value(_y),
                                  m::value(_z));
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

    // d_tests_same_value
    //   whether two texts' values are equal, a matrix element by element,
    // and exact or not alike.
    bool
    d_tests_same_value(
        const char* _a,
        const char* _b
    )
    {
        const m::value a = d_tests_value_of(_a);
        const m::value b = d_tests_value_of(_b);

        return ( (a.kind() != D_MATH_VALUE_NONE) &&
                 (a == b) );
    }

    // d_tests_matrix_of
    //   a matrix of the shape given, of math::quaternion<T> elements, each
    // the value of its own.
    template<typename Type>
    m::value
    d_tests_matrix_of(
        std::size_t                              _rows,
        std::size_t                              _columns,
        const std::vector<m::quaternion<Type> >& _q
    )
    {
        std::vector<m::value> elements;

        for (std::size_t i = 0; i < _q.size(); ++i)
        {
            elements.push_back(m::expr_quaternion_of(_q[i]));
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

    // d_tests_dense_q
    //   a value's nearest math::quaternion<T>, NaNs where it has none.
    template<typename Type>
    m::quaternion<Type>
    d_tests_dense_q(
        const m::value& _x
    )
    {
        const Type          nan = std::numeric_limits<Type>::quiet_NaN();
        m::quaternion<Type> q(nan, nan, nan, nan);

        if (!m::expr_quaternion_to(_x, q))
        {
            return m::quaternion<Type>(nan, nan, nan, nan);
        }

        return q;
    }

    // d_tests_holds
    //   whether a value is a matrix of _rows by _columns of the quaternion
    // storage of Type whose elements are _want's, bit for bit.
    template<typename Type>
    bool
    d_tests_holds(
        const m::value&                          _x,
        std::size_t                              _rows,
        std::size_t                              _columns,
        const std::vector<m::quaternion<Type> >& _want
    )
    {
        const storage               want_storage =
            m::internal::expr_matrix_storage_for<m::quaternion<Type> >();
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
            ok = ( (m::expr_is_quaternion(got[i])) &&
                   (d_tests_same_quaternion(d_tests_dense_q<Type>(got[i]),
                                            _want[i])) );
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

    // d_tests_random_q, d_tests_random_exact
    //   _count quaternions of Type, each part of 20 bits in [-1, 1); and an
    // exact matrix of _n by _n quaternions of whole parts in [-3, 3], its
    // diagonal's real parts 20 more, so that it is invertible.
    template<typename Type>
    std::vector<m::quaternion<Type> >
    d_tests_random_q(
        d_tests_rng& _rng,
        std::size_t  _count
    )
    {
        std::vector<m::quaternion<Type> > out;

        for (std::size_t i = 0; i < _count; ++i)
        {
            const Type w = static_cast<Type>(_rng.unit());
            const Type x = static_cast<Type>(_rng.unit());
            const Type y = static_cast<Type>(_rng.unit());
            const Type z = static_cast<Type>(_rng.unit());

            out.push_back(d_tests_q(w, x, y, z));
        }

        return out;
    }

    m::value
    d_tests_random_exact(
        d_tests_rng& _rng,
        std::size_t  _n
    )
    {
        std::vector<m::value> elements;

        for (std::size_t i = 0; i < (_n * _n); ++i)
        {
            const long w = ( _rng.small(3) +
                             ( ((i % (_n + 1)) == 0) ? 20 : 0 ) );
            const long x = _rng.small(3);
            const long y = _rng.small(3);
            const long z = _rng.small(3);

            elements.push_back(d_tests_exact(w, x, y, z));
        }

        return m::expr_matrix(_n, _n, elements).value();
    }


    //==========================================================================
    // 3.  STORAGE
    //==========================================================================

    // d_tests_storage
    //   S386: where each element is dense, real, complex or a quaternion,
    // the widest type's quaternions where any is one; values otherwise.
    void
    d_tests_storage()
    {
        const m::value qd   = m::expr_quaternion_of(
            d_tests_q(1.5, -2.0, 0.25, -0.0));
        const m::value qf   = m::expr_quaternion_of(
            d_tests_q(0.5f, 1.0f, -1.0f, 2.0f));
        const m::value cd   = m::expr_complex_of(m::complex<double>(3.0, -0.0));
        const m::value made = m::expr_matrix(1, 4, { m::value(2.0f), cd, qd,
                                                     qf }).value();
        const std::vector<m::value> e = d_tests_elements(made);

        D_TESTS_EXPECT( (d_tests_storage_of(made) ==
                             storage::quaternion_doubles) &&
                        (e.size() == 4) &&
                        (d_tests_same_quaternion(d_tests_dense_q<double>(e[0]),
                                                 d_tests_q(2.0, 0.0, 0.0,
                                                           0.0))) &&
                        (d_tests_same_quaternion(d_tests_dense_q<double>(e[1]),
                                                 d_tests_q(3.0, -0.0, 0.0,
                                                           0.0))) &&
                        (d_tests_same_quaternion(d_tests_dense_q<double>(e[2]),
                                                 d_tests_q(1.5, -2.0, 0.25,
                                                           -0.0))) &&
                        (e[3] == qf),
                        "a float, a complex double, a quaternion double and "
                        "a quaternion float: quaternions of doubles, the "
                        "float 2 + 0im + 0jm + 0km, the complex 3 - 0im + "
                        "0jm + 0km, each zero's sign kept (S386)");
        D_TESTS_EXPECT( (d_tests_storage_of(d_tests_value_of("[1, jm]")) ==
                             storage::values) &&
                        (d_tests_storage_of(m::expr_matrix(
                             1, 2, { m::value(1), qd }).value()) ==
                             storage::values) &&
                        (d_tests_storage_of(m::expr_matrix(
                             1, 2, { m::value(1.0L), qf }).value()) ==
                             storage::quaternion_long_doubles) &&
                        (d_tests_storage_of(m::expr_matrix(
                             1, 2, { qf, qf }).value()) ==
                             storage::quaternion_floats) &&
                        (d_tests_storage_of(m::expr_matrix(
                             1, 2, { cd, qf }).value()) ==
                             storage::quaternion_doubles),
                        "exact elements values; a long double beside a "
                        "quaternion float quaternions of long doubles; of "
                        "floats their own; a complex double beside one "
                        "quaternions of doubles");

        const m::value text = d_tests_value_of("[1, 2jm; 3, 1/2 - km]");

        D_TESTS_EXPECT( (m::expr_is_matrix(text)) &&
                        (m::expr_element(text, 0, 1).value() ==
                             d_tests_exact(0, 0, 2, 0)) &&
                        (m::expr_element(text, 1, 1).value() ==
                             d_tests_value_of("1/2 - km")) &&
                        (d_tests_value_of("[1, 2jm] = [1, 2jm]") ==
                             m::value(true)) &&
                        (d_tests_value_of("[1, 2jm] = [1, 2km]") ==
                             m::value(false)),
                        "a literal of quaternions a matrix, each element "
                        "itself; = by the elements (S242)");

        return;
    }


    //==========================================================================
    // 4.  ARITHMETIC
    //==========================================================================

    // d_tests_each_is
    //   whether a value is a matrix of the quaternion storage of Type whose
    // elements are _want's, bit for bit, each as a quaternion of Type.
    template<typename Type>
    bool
    d_tests_each_is(
        const m::value&              _x,
        const std::vector<m::value>& _want
    )
    {
        typedef m::quaternion<Type> q_type;

        const std::vector<m::value> got = d_tests_elements(_x);
        bool                        ok  =
            ( (got.size() == _want.size()) &&
              (d_tests_storage_of(_x) ==
                   m::internal::expr_matrix_storage_for<q_type>()) );

        for (std::size_t i = 0; ( (ok) && (i < got.size()) ); ++i)
        {
            ok = d_tests_same_quaternion(d_tests_dense_q<Type>(got[i]),
                                         d_tests_dense_q<Type>(_want[i]));
        }

        return ok;
    }

    // d_tests_kind_product
    //   row _i of _a times column _j of _b, _inner by _columns, as values:
    // the kind's products a_ik b_kj, each in that order, summed from _zero
    // in order.
    m::value
    d_tests_kind_product(
        const std::vector<m::value>& _a,
        const std::vector<m::value>& _b,
        std::size_t                  _i,
        std::size_t                  _j,
        std::size_t                  _inner,
        std::size_t                  _columns,
        const m::value&              _zero
    )
    {
        m::value sum = _zero;

        for (std::size_t k = 0; k < _inner; ++k)
        {
            sum = (sum + (_a[(_i * _inner) + k] * _b[(k * _columns) + _j]));
        }

        return sum;
    }

    // d_tests_dense_sides
    //   a random real and complex matrix of 2 by 3 in Type.
    template<typename Type>
    void
    d_tests_dense_sides(
        d_tests_rng& _rng,
        m::value&    _real,
        m::value&    _complex
    )
    {
        std::vector<m::value> r;
        std::vector<m::value> c;

        for (std::size_t i = 0; i < 6; ++i)
        {
            r.push_back(m::value(static_cast<Type>(_rng.unit())));
            c.push_back(m::expr_complex_of(m::complex<Type>(
                static_cast<Type>(_rng.unit()),
                static_cast<Type>(_rng.unit()))));
        }

        _real    = m::expr_matrix(2, 3, r).value();
        _complex = m::expr_matrix(2, 3, c).value();

        return;
    }

    // d_tests_dense_scalings
    //   S387 in Type: A * q, q * A, A / q, z * A and A * r, each element the
    // kind's own product or quotient of its two, in its order.
    template<typename Type>
    bool
    d_tests_dense_scalings(
        d_tests_rng& _rng
    )
    {
        const m::value A = d_tests_matrix_of<Type>(
            2, 3, d_tests_random_q<Type>(_rng, 6));
        const m::value q = m::expr_quaternion_of(
            d_tests_random_q<Type>(_rng, 1)[0]);
        const m::value z = m::expr_complex_of(m::complex<Type>(
            static_cast<Type>(_rng.unit()), static_cast<Type>(_rng.unit())));
        const m::value r = m::value(static_cast<Type>(_rng.unit()));
        const std::vector<m::value> a = d_tests_elements(A);
        const m::quaternion<Type>   a0 = d_tests_dense_q<Type>(a[0]);
        const m::quaternion<Type>   q0 = d_tests_dense_q<Type>(q);
        std::vector<m::value>       right, left, over, by_z, by_r;

        for (std::size_t i = 0; i < 6; ++i)
        {
            right.push_back(a[i] * q);
            left.push_back(q * a[i]);
            over.push_back(a[i] / q);
            by_z.push_back(z * a[i]);
            by_r.push_back(a[i] * r);
        }

        return ( (d_tests_each_is<Type>(A * q, right)) &&
                 (d_tests_each_is<Type>(q * A, left)) &&
                 (d_tests_each_is<Type>(A / q, over)) &&
                 (d_tests_each_is<Type>(z * A, by_z)) &&
                 (d_tests_each_is<Type>(A * r, by_r)) &&
                 (d_tests_same_quaternion(
                      d_tests_dense_q<Type>(d_tests_elements(A * q)[0]),
                      a0 * q0)) &&
                 (d_tests_same_quaternion(
                      d_tests_dense_q<Type>(d_tests_elements(q * A)[0]),
                      q0 * a0)) &&
                 (d_tests_same_quaternion(
                      d_tests_dense_q<Type>(d_tests_elements(A / q)[0]),
                      a0 / q0)) );
    }

    // d_tests_dense_arithmetic
    //   S387 in Type: dense sums and products, each element the kind's own
    // operation on its two, bit for bit, a real or complex operand taken as
    // itself, a product's each element summed from 0 in order.
    template<typename Type>
    bool
    d_tests_dense_arithmetic(
        d_tests_rng& _rng
    )
    {
        typedef m::quaternion<Type> q_type;

        const m::value zero = m::expr_quaternion_of(q_type(Type(0), Type(0),
                                                           Type(0), Type(0)));
        bool           ok   = true;

        for (int round = 0; round < 10; ++round)
        {
            const m::value A  = d_tests_matrix_of<Type>(
                2, 3, d_tests_random_q<Type>(_rng, 6));
            const m::value B  = d_tests_matrix_of<Type>(
                2, 3, d_tests_random_q<Type>(_rng, 6));
            const m::value Bt = m::expr_transpose(B, false).value();
            m::value       R;
            m::value       C;

            d_tests_dense_sides<Type>(_rng, R, C);

            const std::vector<m::value> a  = d_tests_elements(A);
            const std::vector<m::value> b  = d_tests_elements(B);
            const std::vector<m::value> bt = d_tests_elements(Bt);
            const std::vector<m::value> r  = d_tests_elements(R);
            const std::vector<m::value> c  = d_tests_elements(C);
            std::vector<m::value>       sum, dif, rsum, cdif, ab, rb, cb;

            for (std::size_t i = 0; i < 6; ++i)
            {
                sum.push_back(a[i] + b[i]);
                dif.push_back(a[i] - b[i]);
                rsum.push_back(r[i] + a[i]);
                cdif.push_back(a[i] - c[i]);
            }

            for (std::size_t i = 0; i < 2; ++i)
            {
                for (std::size_t j = 0; j < 2; ++j)
                {
                    ab.push_back(d_tests_kind_product(a, bt, i, j, 3, 2, zero));
                    rb.push_back(d_tests_kind_product(r, bt, i, j, 3, 2, zero));
                    cb.push_back(d_tests_kind_product(c, bt, i, j, 3, 2, zero));
                }
            }

            ok = ( (ok) &&
                   (d_tests_each_is<Type>(A + B, sum)) &&
                   (d_tests_each_is<Type>(A - B, dif)) &&
                   (d_tests_each_is<Type>(R + A, rsum)) &&
                   (d_tests_each_is<Type>(A - C, cdif)) &&
                   (d_tests_each_is<Type>(A * Bt, ab)) &&
                   (d_tests_each_is<Type>(R * Bt, rb)) &&
                   (d_tests_each_is<Type>(C * Bt, cb)) &&
                   (d_tests_dense_scalings<Type>(_rng)) );
        }

        return ok;
    }

    // d_tests_exact_arithmetic
    //   S387: exact sums, products and scalings, each element the kind's
    // exact operation, on random matrices of whole parts.
    bool
    d_tests_exact_arithmetic(
        d_tests_rng& _rng
    )
    {
        bool ok = true;

        for (int round = 0; round < 10; ++round)
        {
            const m::value A = d_tests_random_exact(_rng, 3);
            const m::value B = d_tests_random_exact(_rng, 3);
            const m::value q = d_tests_exact(_rng.small(3), _rng.small(3),
                                             2 + _rng.small(1), _rng.small(3));
            const std::vector<m::value> a = d_tests_elements(A);
            const std::vector<m::value> b = d_tests_elements(B);
            std::vector<m::value>       sum, ab, left, over;

            for (std::size_t i = 0; i < 9; ++i)
            {
                sum.push_back(a[i] + b[i]);
                left.push_back(q * a[i]);
                over.push_back(a[i] / q);
            }

            for (std::size_t i = 0; i < 3; ++i)
            {
                for (std::size_t j = 0; j < 3; ++j)
                {
                    ab.push_back(d_tests_kind_product(a, b, i, j, 3, 3,
                                                      m::value(0)));
                }
            }

            ok = ( (ok) &&
                   ((A + B) == m::expr_matrix(3, 3, sum).value()) &&
                   ((A * B) == m::expr_matrix(3, 3, ab).value()) &&
                   ((q * A) == m::expr_matrix(3, 3, left).value()) &&
                   ((A / q) == m::expr_matrix(3, 3, over).value()) &&
                   (d_tests_storage_of(A * B) == storage::values) &&
                   (d_tests_elements(A * B)[0].is_exact()) );
        }

        return ok;
    }

    // d_tests_identities
    //   A ^ 0 of a dense matrix of complex numbers, and of one of
    // quaternions, in Type: the identity of A's own storage.
    template<typename Type>
    bool
    d_tests_identities()
    {
        typedef m::complex<Type> c_type;

        const m::value dc   = m::expr_matrix(
            1, 1, { m::expr_complex_of(c_type(Type(1), Type(2))) }).value();
        const m::value dq   = d_tests_matrix_of<Type>(
            1, 1, { d_tests_q(Type(1), Type(0), Type(2), Type(0)) });
        const m::value id_c = m::internal::expr_matrix_power(dc,
                                                             m::value(0));
        const m::value id_q = m::internal::expr_matrix_power(dq,
                                                             m::value(0));

        return ( (d_tests_storage_of(id_c) ==
                      m::internal::expr_matrix_storage_for<c_type>()) &&
                 (d_tests_elements(id_c).size() == 1) &&
                 (d_tests_elements(id_c)[0] ==
                      m::expr_complex_of(c_type(Type(1), Type(0)))) &&
                 (d_tests_each_is<Type>(id_q, { m::expr_quaternion_of(
                      d_tests_q(Type(1), Type(0), Type(0), Type(0))) })) );
    }

    // d_tests_powers
    //   A ^ n by the product and the inverse; A ^ 0 the identity of A's
    // storage, of a dense matrix of quaternions or of complex numbers too,
    // in each type.
    bool
    d_tests_powers()
    {
        const m::value dq = d_tests_matrix_of<double>(
            2, 2, { d_tests_q(1.0, 0.0, 0.0, 0.0),
                    d_tests_q(0.0, 0.0, 1.0, 0.0),
                    d_tests_q(0.0, 0.0, 0.0, 1.0),
                    d_tests_q(2.0, 0.5, 0.0, 0.0) });
        const m::value one_q  = m::expr_quaternion_of(
            d_tests_q(1.0, 0.0, 0.0, 0.0));
        const m::value zero_q = m::expr_quaternion_of(
            d_tests_q(0.0, 0.0, 0.0, 0.0));
        const m::value id_q   = m::internal::expr_matrix_power(dq,
                                                               m::value(0));

        return ( (d_tests_same_value("[1, jm; km, 2] ^ 2",
                                     "[1, jm; km, 2] * [1, jm; km, 2]")) &&
                 (d_tests_same_value("[1, jm; km, 2] ^ -2",
                                     "inv([1, jm; km, 2]) * "
                                     "inv([1, jm; km, 2])")) &&
                 (m::internal::expr_matrix_power(dq, m::value(3)) ==
                      (dq * (dq * dq))) &&
                 (d_tests_each_is<double>(id_q, { one_q, zero_q, zero_q,
                                                  one_q })) &&
                 (d_tests_identities<float>()) &&
                 (d_tests_identities<double>()) &&
                 (d_tests_identities<long double>()) );
    }

    // d_tests_operand_kinds
    //   the kind's operands in Type, as the generic kernels take them, where
    // the language reaches them through others alone (S387): two complex
    // numbers' product and quotient, and a real one over a complex one,
    // math::complex's, so cplx.h's, bit for bit, each holding two parts;
    // and an operand's value by the parts it holds: a real number, a
    // complex one, or a quaternion.
    template<typename Type>
    bool
    d_tests_operand_kinds()
    {
        typedef m::internal::expr_quaternion_operand<Type> operand;
        typedef m::complex<Type>                           c_type;

        const c_type   a(Type(0.75), Type(-1.5));
        const c_type   b(Type(-2.25), Type(0.5));
        const operand  oa       = m::internal::expr_quaternion_operand_made(
            d_tests_q(a.real(), a.imag(), Type(0), Type(0)), 2);
        const operand  ob       = m::internal::expr_quaternion_operand_made(
            d_tests_q(b.real(), b.imag(), Type(0), Type(0)), 2);
        const operand  three    = m::internal::expr_quaternion_operand_made(
            d_tests_q(Type(3), Type(0), Type(0), Type(0)), 1);
        const operand  jm       = m::internal::expr_quaternion_operand_made(
            d_tests_q(Type(0), Type(0), Type(1), Type(0)), 4);
        const operand  product  = (oa * ob);
        const operand  quotient = (oa / ob);
        const operand  over     = (three / ob);
        const c_type   p        = (a * b);
        const c_type   q        = (a / b);
        const c_type   r        = (Type(3) / b);
        const m::value real     =
            m::internal::expr_quaternion_operand_value(three, 0);
        const m::value complex  =
            m::internal::expr_quaternion_operand_value(product, 0);
        c_type         z;
        Type           x        = Type(0);

        return ( (product.held == 2) &&
                 (d_tests_same_quaternion(product.q,
                                          d_tests_q(p.real(), p.imag(),
                                                    Type(0), Type(0)))) &&
                 (quotient.held == 2) &&
                 (d_tests_same_quaternion(quotient.q,
                                          d_tests_q(q.real(), q.imag(),
                                                    Type(0), Type(0)))) &&
                 (d_tests_same_quaternion(over.q,
                                          d_tests_q(r.real(), r.imag(),
                                                    Type(0), Type(0)))) &&
                 (!m::expr_is_complex(real)) &&
                 (real.to(x)) &&
                 (d_tests_same_bits(x, Type(3))) &&
                 (m::expr_is_complex(complex)) &&
                 (m::expr_complex_to(complex, z)) &&
                 (d_tests_same_bits(z.real(), p.real())) &&
                 (d_tests_same_bits(z.imag(), p.imag())) &&
                 (m::expr_is_quaternion(
                      m::internal::expr_quaternion_operand_value((oa * jm),
                                                                 0))) );
    }

    // d_tests_arithmetic
    //   S387: each dense type; exact matrices; products in order; powers.
    void
    d_tests_arithmetic()
    {
        d_tests_rng rng = { 0x5eedf00dULL };

        D_TESTS_EXPECT( (d_tests_dense_arithmetic<float>(rng)) &&
                        (d_tests_dense_arithmetic<double>(rng)) &&
                        (d_tests_dense_arithmetic<long double>(rng)),
                        "dense sums, products and scalings in float, double "
                        "and long double: each element the kind's own "
                        "operation on its two, bit for bit, a real or "
                        "complex operand taken as itself, a product summed "
                        "from 0 in order, a number's product in its order "
                        "(S387)");
        D_TESTS_EXPECT( (d_tests_exact_arithmetic(rng)),
                        "exact sums, products and scalings, each element "
                        "the kind's exact operation (S387)");
        D_TESTS_EXPECT( (d_tests_same_value("[jm] * [km]", "[im]")) &&
                        (d_tests_same_value("[km] * [jm]", "[-im]")) &&
                        (d_tests_same_value("[1, jm] * km", "[km, im]")) &&
                        (d_tests_same_value("km * [1, jm]", "[km, -im]")) &&
                        (d_tests_same_value("[1, jm] / km", "[-km, -im]")) &&
                        (d_tests_same_value("[im, jm] * [km; 1]", "[0]")) &&
                        (d_tests_domain("[1, jm] + km")),
                        "products in their order, A / q = A inv(q) (S277); "
                        "a matrix plus a number a domain error (S246)");
        D_TESTS_EXPECT( (d_tests_powers()),
                        "powers by the product and the inverse; A ^ 0 the "
                        "identity of A's storage, of quaternions or of "
                        "complex numbers, in each type");
        D_TESTS_EXPECT( (d_tests_operand_kinds<float>()) &&
                        (d_tests_operand_kinds<double>()) &&
                        (d_tests_operand_kinds<long double>()),
                        "the kind's operands: two complex numbers' product "
                        "and quotient, and a real one over a complex one, "
                        "cplx.h's, bit for bit, holding two parts; an "
                        "operand's value a real number, a complex one or a "
                        "quaternion by the parts it holds (S387)");

        return;
    }


    //==========================================================================
    // 5.  TRANSPOSES, PARTS, INDEXING AND BLOCKS
    //==========================================================================

    // d_tests_dense_shapes
    //   in Type: A' each element conjugated by quat.h's conjugate, and
    // moved; transpose(A) moved; -A each part negated; real and conj each
    // element's, bit for bit.
    template<typename Type>
    bool
    d_tests_dense_shapes(
        d_tests_rng& _rng
    )
    {
        typedef m::quaternion<Type> q_type;

        const std::vector<q_type> q = d_tests_random_q<Type>(_rng, 6);
        const m::value            A = d_tests_matrix_of<Type>(2, 3, q);
        std::vector<q_type>       starred, moved, negated, conjugated;
        std::vector<m::value>     real;

        for (std::size_t j = 0; j < 3; ++j)
        {
            for (std::size_t i = 0; i < 2; ++i)
            {
                starred.push_back(conjugate(q[(i * 3) + j]));
                moved.push_back(q[(i * 3) + j]);
            }
        }

        for (std::size_t i = 0; i < 6; ++i)
        {
            negated.push_back(q_type(-q[i].x, -q[i].y, -q[i].z, -q[i].w));
            conjugated.push_back(conjugate(q[i]));
            real.push_back(m::value(q[i].w));
        }

        return ( (d_tests_holds<Type>(m::expr_transpose(A).value(), 3, 2,
                                      starred)) &&
                 (d_tests_holds<Type>(m::expr_transpose(A, false).value(), 3,
                                      2, moved)) &&
                 (d_tests_holds<Type>(-A, 2, 3, negated)) &&
                 (d_tests_holds<Type>(d_tests_value_of("conj(x)", A), 2, 3,
                                      conjugated)) &&
                 (d_tests_value_of("real(x)", A) ==
                      m::expr_matrix(2, 3, real).value()) &&
                 (d_tests_storage_of(d_tests_value_of("real(x)", A)) ==
                      m::internal::expr_matrix_storage_for<Type>()) );
    }

    // d_tests_shapes
    //   A' and transpose(A), -A, real, imag, conj and arg of a matrix and
    // complex(A, B); indexing; blocks.
    void
    d_tests_shapes()
    {
        d_tests_rng rng = { 0xb10c5ULL };

        D_TESTS_EXPECT( (d_tests_dense_shapes<float>(rng)) &&
                        (d_tests_dense_shapes<double>(rng)) &&
                        (d_tests_dense_shapes<long double>(rng)),
                        "in float, double and long double: A' each element "
                        "quat.h's conjugate, transpose(A) each moved, -A "
                        "each part negated, real and conj each element's, "
                        "bit for bit");
        D_TESTS_EXPECT( (d_tests_same_value("[1, 2jm; km, 3]'",
                                            "[1, -km; -2jm, 3]")) &&
                        (d_tests_same_value("transpose([1, 2jm; km, 3])",
                                            "[1, km; 2jm, 3]")) &&
                        (d_tests_same_value("-[1, jm]", "[-1, -jm]")) &&
                        (d_tests_same_value("real([1 + jm, 2km])", "[1, 0]")) &&
                        (d_tests_same_value("conj([1 + jm, 2km])",
                                            "[1 - jm, -2km]")) &&
                        (d_tests_domain("imag([1 + jm, 2km])")) &&
                        (d_tests_domain("arg([1, jm])")) &&
                        (d_tests_domain("complex([1, jm], [1, 2])")) &&
                        (d_tests_same_value("transpose(jm)", "jm")) &&
                        (d_tests_same_value("jm'", "-jm")),
                        "exactly: A' conjugating each element, transpose "
                        "moving it, -A, real and conj each element's; "
                        "imag, arg and complex of a quaternion a domain "
                        "error (S275); a quaternion its own transpose, its "
                        "conjugate for '");
        D_TESTS_EXPECT( (d_tests_same_value("[1, jm; km, 2][2]",
                                            "[km, 2]")) &&
                        (d_tests_same_value("[1, jm; km, 2][1, 2]", "jm")) &&
                        (d_tests_same_value("[[1, jm], km]",
                                            "[1, jm, km]")) &&
                        (d_tests_same_value("[[1; jm], [km; 2]]",
                                            "[1, km; jm, 2]")) &&
                        (d_tests_same_value("[jm, []]", "[jm]")) &&
                        (d_tests_storage_of(d_tests_value_of(
                             "[x, 2x]", m::expr_quaternion_of(
                                 d_tests_q(1.0, 2.0, 3.0, 4.0)))) ==
                             storage::quaternion_doubles),
                        "indexing a row and an element; blocks of "
                        "quaternions beside and above one another, a "
                        "quaternion a block of 1 by 1 (S386)");

        return;
    }


    //==========================================================================
    // 6.  LINEAR ALGEBRA
    //==========================================================================

    // d_tests_operands
    //   a matrix's elements as the quaternion kind's operands in Type, each
    // holding four parts.
    template<typename Type>
    std::vector<m::internal::expr_quaternion_operand<Type> >
    d_tests_operands(
        const std::vector<m::quaternion<Type> >& _q
    )
    {
        std::vector<m::internal::expr_quaternion_operand<Type> > out;

        for (std::size_t i = 0; i < _q.size(); ++i)
        {
            out.push_back(m::internal::expr_quaternion_operand_made(_q[i],
                                                                    4));
        }

        return out;
    }

    // d_tests_parts_of
    //   operands' quaternions.
    template<typename Type>
    std::vector<m::quaternion<Type> >
    d_tests_parts_of(
        const std::vector<m::internal::expr_quaternion_operand<Type> >& _x
    )
    {
        std::vector<m::quaternion<Type> > out;

        for (std::size_t i = 0; i < _x.size(); ++i)
        {
            out.push_back(_x[i].q);
        }

        return out;
    }

    // d_tests_near_identity
    //   whether a dense matrix of quaternions is within _tolerance of the
    // identity, part by part.
    template<typename Type>
    bool
    d_tests_near_identity(
        const m::value& _x,
        std::size_t     _n,
        Type            _tolerance
    )
    {
        const std::vector<m::value> e  = d_tests_elements(_x);
        bool                        ok = (e.size() == (_n * _n));

        for (std::size_t i = 0; ( (ok) && (i < e.size()) ); ++i)
        {
            const m::quaternion<Type> q = d_tests_dense_q<Type>(e[i]);
            const Type                w = ( ((i % (_n + 1)) == 0) ? Type(1)
                                                                  : Type(0) );

            ok = ( (std::fabs(q.w - w) <= _tolerance) &&
                   (std::fabs(q.x) <= _tolerance) &&
                   (std::fabs(q.y) <= _tolerance) &&
                   (std::fabs(q.z) <= _tolerance) );
        }

        return ok;
    }

    // d_tests_dense_algebra
    //   S388 in Type: inv and solve the generic kernels' Gauss-Jordan on the
    // kind's operands, bit for bit, A inv(A) and A X near I and B; trace
    // the kernels'; a dominant diagonal keeps A far from singular.
    template<typename Type>
    bool
    d_tests_dense_algebra(
        d_tests_rng& _rng
    )
    {
        typedef m::internal::expr_quaternion_operand<Type> operand;
        typedef la::internal::linalg_kernel<operand>       kernel;

        const Type tolerance = ( Type(512) *
                                 std::numeric_limits<Type>::epsilon() );
        bool       ok        = true;

        for (int round = 0; round < 10; ++round)
        {
            std::vector<m::quaternion<Type> > q =
                d_tests_random_q<Type>(_rng, 9);
            const std::vector<m::quaternion<Type> > b =
                d_tests_random_q<Type>(_rng, 6);

            q[0].w += Type(4);
            q[4].w += Type(4);
            q[8].w += Type(4);

            const m::value             A  = d_tests_matrix_of<Type>(3, 3, q);
            const m::value             B  = d_tests_matrix_of<Type>(3, 2, b);
            const std::vector<operand> oa = d_tests_operands<Type>(q);
            const std::vector<operand> ob = d_tests_operands<Type>(b);
            std::vector<operand>       inverse(9);
            std::vector<operand>       x(6);
            std::vector<operand>       scratch(9);

            static_cast<void>(kernel::inverse(inverse.data(), scratch.data(),
                                              oa.data(), 3));
            static_cast<void>(kernel::solve(x.data(), scratch.data(),
                                            oa.data(), ob.data(), 3, 2));

            const m::value inv = m::expr_inv(A).value();
            const m::value X   = m::expr_solve(A, B).value();
            const m::value tr  = m::expr_trace(A).value();

            ok = ( (ok) &&
                   (d_tests_holds<Type>(inv, 3, 3,
                                        d_tests_parts_of<Type>(inverse))) &&
                   (d_tests_holds<Type>(X, 3, 2,
                                        d_tests_parts_of<Type>(x))) &&
                   (d_tests_near_identity<Type>(A * inv, 3, tolerance)) &&
                   (d_tests_near_identity<Type>(inv * A, 3, tolerance)) &&
                   (d_tests_same_quaternion(
                        d_tests_dense_q<Type>(tr),
                        kernel::trace(oa.data(), 3).q)) );
        }

        return ok;
    }

    // d_tests_dense_vectors
    //   S388 in Type: dot of dense vectors of quaternions the sum from 0, in
    // order, of conj(a_i) b_i, each by quat.h's product, bit for bit, an
    // element of a complex vector conjugated as the complex number it is,
    // its jm and km parts +0; cross a_2 b_3 - a_3 b_2, a_3 b_1 - a_1 b_3,
    // a_1 b_2 - a_2 b_1, each product in that order, bit for bit; and norm
    // the root of the parts' squares' sum, within 4 of Type's epsilons.
    template<typename Type>
    bool
    d_tests_dense_vectors(
        d_tests_rng& _rng
    )
    {
        typedef m::quaternion<Type> q_type;

        const q_type zero(Type(0), Type(0), Type(0), Type(0));
        bool         ok = true;

        for (int round = 0; round < 10; ++round)
        {
            const std::vector<q_type> a = d_tests_random_q<Type>(_rng, 3);
            const std::vector<q_type> b = d_tests_random_q<Type>(_rng, 3);
            const m::value            A = d_tests_matrix_of<Type>(1, 3, a);
            const m::value            B = d_tests_matrix_of<Type>(1, 3, b);
            std::vector<m::value>     complexes;
            q_type                    dot   = zero;
            q_type                    mixed = zero;
            long double               sum   = 0.0L;
            Type                      n     = Type(0);

            // the test's own sums, each product quat.h's
            for (std::size_t i = 0; i < 3; ++i)
            {
                complexes.push_back(m::expr_complex_of(
                    m::complex<Type>(a[i].w, a[i].x)));
                sum  += ( ( (static_cast<long double>(a[i].w) * a[i].w) +
                            (static_cast<long double>(a[i].x) * a[i].x) ) +
                          ( (static_cast<long double>(a[i].y) * a[i].y) +
                            (static_cast<long double>(a[i].z) * a[i].z) ) );
                dot   = d_tests_sum(dot, (conjugate(a[i]) * b[i]), false);
                mixed = d_tests_sum(mixed,
                                    (d_tests_q(a[i].w, -a[i].x, Type(0),
                                               Type(0)) *
                                     b[i]),
                                    false);
            }

            const m::value              C     = m::expr_matrix(1, 3,
                                                               complexes)
                                                    .value();
            const m::value              cross = m::expr_cross(A, B).value();
            const std::vector<m::value> c     = d_tests_elements(cross);

            ok = ( (ok) &&
                   (d_tests_same_quaternion(
                        d_tests_dense_q<Type>(m::expr_dot(A, B).value()),
                        dot)) &&
                   (d_tests_same_quaternion(
                        d_tests_dense_q<Type>(m::expr_dot(C, B).value()),
                        mixed)) &&
                   (d_tests_storage_of(cross) ==
                        m::internal::expr_matrix_storage_for<q_type>()) &&
                   (c.size() == 3) &&
                   (m::expr_norm(A).value().to(n)) &&
                   (std::fabs(static_cast<long double>(n) -
                              std::sqrt(sum)) <=
                        (4.0L * std::numeric_limits<Type>::epsilon() *
                         std::sqrt(sum))) );

            ok = ( (ok) &&
                   (d_tests_same_quaternion(
                        d_tests_dense_q<Type>(c[0]),
                        d_tests_sum((a[1] * b[2]), (a[2] * b[1]), true))) &&
                   (d_tests_same_quaternion(
                        d_tests_dense_q<Type>(c[1]),
                        d_tests_sum((a[2] * b[0]), (a[0] * b[2]), true))) &&
                   (d_tests_same_quaternion(
                        d_tests_dense_q<Type>(c[2]),
                        d_tests_sum((a[0] * b[1]), (a[1] * b[0]), true))) );
        }

        return ok;
    }

    // d_tests_mixed_values
    //   S388, S390 for a matrix of values holding an inexact element: its
    // square algebra in that element's type's quaternions, each element its
    // nearest there, the kind's operand it is: inv the kernels' on those
    // operands, bit for bit, A inv(A) near I; trace in long double; norm
    // near sqrt(3); eig the dense matrix's; and solve(a, B) of a complex a
    // and a B holding a quaternion a^-1 b for each b, cplx.h's quotient where
    // b is real or complex, quat.h's left quotient where it is a
    // quaternion, bit for bit, in long double, and in float.
    bool
    d_tests_mixed_values()
    {
        typedef long double                              ld;
        typedef m::internal::expr_quaternion_operand<ld> operand;
        typedef la::internal::linalg_kernel<operand>     kernel;
        typedef m::quaternion<ld>                        q_type;

        const ld                    eps = std::numeric_limits<ld>::epsilon();
        const m::value              A   = d_tests_value_of(
            "[sqrt(2), jm; km, 1]");
        const std::vector<m::value> e   = d_tests_elements(A);
        std::vector<operand>        oa;
        std::vector<q_type>         qa;
        std::vector<operand>        inverse(4);
        std::vector<operand>        scratch(4);
        ld                          s   = 0.0L;
        ld                          t   = 0.0L;
        ld                          n   = 0.0L;

        static_cast<void>(m::internal::value_root_of(m::value(2)).to(s));

        // each element the operand of its own parts, in long double
        for (std::size_t i = 0; i < e.size(); ++i)
        {
            oa.push_back(m::internal::expr_quaternion_operand_of<ld>(
                m::internal::expr_quaternion_parts_of(e[i].c_value())));
            qa.push_back(oa.back().q);
        }

        static_cast<void>(kernel::inverse(inverse.data(), scratch.data(),
                                          oa.data(), 2));

        const m::value inv = m::expr_inv(A).value();
        const bool     algebra =
            ( (d_tests_storage_of(A) == storage::values) &&
              (oa.size() == 4) &&
              (d_tests_holds<ld>(inv, 2, 2,
                                 d_tests_parts_of<ld>(inverse))) &&
              (d_tests_near_identity<ld>(A * inv, 2, (64.0L * eps))) &&
              (m::expr_trace(A).value().to(t)) &&
              (d_tests_same_bits(t, (s + 1.0L))) &&
              (m::expr_norm(d_tests_value_of("[sqrt(2), jm]")).value()
                   .to(n)) &&
              (std::fabs(n - std::sqrt(3.0L)) <= (4.0L * eps)) &&
              (m::expr_eig(A).value() ==
                   m::expr_eig(d_tests_matrix_of<ld>(2, 2, qa)).value()) );

        const std::vector<m::value> x  = d_tests_elements(d_tests_value_of(
            "solve(sqrt(2) + im, [1, 1 + im, jm])"));
        const m::complex<ld>        a(s, 1.0L);
        const m::complex<ld>        x0 = (1.0L / a);
        const m::complex<ld>        x1 = (m::complex<ld>(1.0L, 1.0L) / a);
        const m::quaternion<float>  f  = d_tests_q(0.5f, -1.0f, 0.25f, 2.0f);

        return ( (algebra) &&
                 (x.size() == 3) &&
                 (d_tests_same_quaternion(
                      d_tests_dense_q<ld>(x[0]),
                      d_tests_q(x0.real(), x0.imag(), 0.0L, 0.0L))) &&
                 (d_tests_same_quaternion(
                      d_tests_dense_q<ld>(x[1]),
                      d_tests_q(x1.real(), x1.imag(), 0.0L, 0.0L))) &&
                 (d_tests_same_quaternion(
                      d_tests_dense_q<ld>(x[2]),
                      left_divide(d_tests_q(s, 1.0L, 0.0L, 0.0L),
                                  d_tests_q(0.0L, 0.0L, 1.0L, 0.0L)))) &&
                 (d_tests_same_quaternion(
                      d_tests_dense_q<float>(d_tests_value_of(
                          "solve(x, jm)", m::expr_quaternion_of(f))),
                      left_divide(f, d_tests_q(0.0f, 0.0f, 1.0f,
                                               0.0f)))) );
    }

    // d_tests_dense_edges
    //   four edges of the dense paths: a pivot counted by all four parts,
    // [1e-20, 1; km, km]'s inverse by the pivot km, D inv(D) near I, where
    // the pivot 1e-20 leaves nothing of the second row (S388); a complex
    // matrix's element in a sum with a quaternion's as
    // the complex number it is, so that the quaternion's -0 jm part is kept
    // (S387, S372); solve of an exact A holding a quaternion and a dense
    // real B in B's type's quaternions, the dense A's solution (S388); and
    // under real_results a matrix of quaternions not taken for real, its
    // complex eigenvalues given, where a real matrix's are refused (S385).
    bool
    d_tests_dense_edges()
    {
        const double   eps = std::numeric_limits<double>::epsilon();
        const m::value D   = d_tests_matrix_of<double>(
            2, 2, { d_tests_q(1.0e-20, 0.0, 0.0, 0.0),
                    d_tests_q(1.0, 0.0, 0.0, 0.0),
                    d_tests_q(0.0, 0.0, 0.0, 1.0),
                    d_tests_q(0.0, 0.0, 0.0, 1.0) });
        const m::value inv = m::expr_inv(D).value();
        const m::value C   = m::expr_matrix(
            1, 1, { m::expr_complex_of(m::complex<double>(1.0, 2.0)) })
                .value();
        const m::value Q   = d_tests_matrix_of<double>(
            1, 1, { d_tests_q(3.0, 4.0, -0.0, 5.0) });
        const m::value A   = d_tests_value_of("[1, jm; km, 2]");
        const m::value Ad  = d_tests_matrix_of<double>(
            2, 2, { d_tests_q(1.0, 0.0, 0.0, 0.0),
                    d_tests_q(0.0, 0.0, 1.0, 0.0),
                    d_tests_q(0.0, 0.0, 0.0, 1.0),
                    d_tests_q(2.0, 0.0, 0.0, 0.0) });
        const m::value B   = m::expr_matrix(
            2, 1, { m::value(0.5), m::value(0.25) }).value();
        const m::value X   = m::expr_solve(A, B).value();
        const m::expr_context real(true);

        return ( (!d_tests_is_error(inv, D_MATH_VALUE_ERROR_POLE)) &&
                 (d_tests_near_identity<double>(D * inv, 2, (8.0 * eps))) &&
                 (d_tests_same_quaternion(
                      d_tests_dense_q<double>(d_tests_elements(C + Q)[0]),
                      d_tests_q(4.0, 6.0, -0.0, 5.0))) &&
                 (d_tests_storage_of(X) == storage::quaternion_doubles) &&
                 (X == m::expr_solve(Ad, B).value()) &&
                 (m::expr_is_matrix(d_tests_value_of(
                      "eig([0, jm; km, 0])", m::value(0), real))) &&
                 (d_tests_value_of("eig([0, jm; km, 0])", m::value(0), real) ==
                      d_tests_value_of("eig([0, jm; km, 0])")) &&
                 (d_tests_is_error(d_tests_value_of("eig([0, 1; -1, 0])",
                                                    m::value(0),
                                                    real),
                                   D_MATH_VALUE_ERROR_DOMAIN)) );
    }

    // d_tests_exact_algebra
    //   S388: exact matrices' inverses and solutions exactly, A inv(A),
    // inv(A) A and A X exactly I and B, of quaternions and of a real A with
    // quaternion columns.
    bool
    d_tests_exact_algebra(
        d_tests_rng& _rng
    )
    {
        const m::value I  = d_tests_value_of("[1, 0, 0; 0, 1, 0; 0, 0, 1]");
        bool           ok = true;

        for (int round = 0; round < 6; ++round)
        {
            const m::value A   = d_tests_random_exact(_rng, 3);
            const m::value B   = d_tests_random_exact(_rng, 3);
            const m::value R   = d_tests_value_of(
                "[3, 1, 0; 1, 4, 1; 0, 1, 5]");
            const m::value inv = m::expr_inv(A).value();
            const m::value X   = m::expr_solve(A, B).value();
            const m::value Y   = m::expr_solve(R, B).value();

            ok = ( (ok) &&
                   ((A * inv) == I) &&
                   ((inv * A) == I) &&
                   ((A * X) == B) &&
                   ((R * Y) == B) &&
                   (d_tests_storage_of(X) == storage::values) &&
                   (d_tests_elements(X)[0].is_exact()) );
        }

        return ok;
    }

    // d_tests_numbers
    //   S390: linear algebra's functions of a quaternion, as a scalar.
    bool
    d_tests_numbers()
    {
        typedef m::quaternion<double> q_type;

        const q_type   a = d_tests_q(0.5, -1.0, 2.0, 0.25);
        const q_type   b = d_tests_q(-1.5, 0.75, 1.0, -2.0);
        const m::value A = m::expr_quaternion_of(a);
        const m::value B = m::expr_quaternion_of(b);

        return ( (d_tests_domain("det(jm)")) &&
                 (d_tests_same_value("inv(jm)", "-jm")) &&
                 (d_tests_same_value("trace(2km)", "2km")) &&
                 (d_tests_same_value("norm(2 + 2im + 2jm + 2km)", "4")) &&
                 (d_tests_same_value("dot(jm, km)", "-im")) &&
                 (d_tests_same_value("solve(jm, km)", "-im")) &&
                 (d_tests_same_value("solve(im, jm)", "-km")) &&
                 (d_tests_same_value("solve(2, jm)", "jm / 2")) &&
                 (d_tests_same_value("solve(im, [jm, 1])", "[-km, -im]")) &&
                 (d_tests_domain("cross(jm, km)")) &&
                 (d_tests_same_quaternion(
                      d_tests_dense_q<double>(m::expr_solve(A, B).value()),
                      left_divide(a, b))) );
    }

    // d_tests_algebra
    //   S388, S390: det; inv, solve and trace dense and exact; singular
    // matrices; norm, dot and cross; numbers as scalars.
    void
    d_tests_algebra()
    {
        d_tests_rng rng = { 0xa16eb7aULL };

        D_TESTS_EXPECT( (d_tests_domain("det([1, jm; km, 2])")) &&
                        (d_tests_is_error(m::expr_det(d_tests_matrix_of<
                             double>(1, 1, { d_tests_q(1.0, 0.0, 0.0,
                                                       1.0) })).value(),
                             D_MATH_VALUE_ERROR_DOMAIN)) &&
                        (d_tests_same_value("det([1, im; 2, 3])",
                                            "3 - 2im")),
                        "det of a matrix of quaternions a domain error, "
                        "exact or dense (S277); of a complex one still its "
                        "own");
        D_TESTS_EXPECT( (d_tests_dense_algebra<float>(rng)) &&
                        (d_tests_dense_algebra<double>(rng)) &&
                        (d_tests_dense_algebra<long double>(rng)),
                        "in float, double and long double: inv and solve "
                        "the generic kernels' on the kind's operands, bit "
                        "for bit, A inv(A), inv(A) A near I; trace the "
                        "kernels' (S269, S388)");
        D_TESTS_EXPECT( (d_tests_exact_algebra(rng)),
                        "exact inverses and solutions exactly, of "
                        "quaternions and of a real A with quaternion "
                        "columns (S388)");
        const m::value S = d_tests_matrix_of<double>(
            2, 2, { d_tests_q(1.0, 0.0, 0.0, 0.0),
                    d_tests_q(0.0, 0.0, 1.0, 0.0),
                    d_tests_q(0.0, 0.0, 1.0, 0.0),
                    d_tests_q(-1.0, 0.0, 0.0, 0.0) });
        const m::value b = d_tests_matrix_of<double>(
            2, 1, { d_tests_q(1.0, 0.0, 0.0, 0.0),
                    d_tests_q(2.0, 0.0, 0.0, 0.0) });

        D_TESTS_EXPECT( (d_tests_is_error(d_tests_value_of(
                             "inv([1, jm; jm, -1])"),
                             D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(d_tests_value_of(
                             "solve([1, jm; jm, -1], [1; 2])"),
                             D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(m::expr_inv(S).value(),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(m::expr_solve(S, b).value(),
                                          D_MATH_VALUE_ERROR_POLE)),
                        "a singular matrix, its second row jm times its "
                        "first, a pole, exact and dense, its inverse and a "
                        "solution");
        D_TESTS_EXPECT( (d_tests_same_value("norm([3jm, 4km])", "5")) &&
                        (d_tests_same_value("norm([1, jm; km, 1])", "2")) &&
                        (d_tests_same_value("dot([jm, 1], [km, 2])",
                                            "2 - im")) &&
                        (d_tests_same_value("dot([km, 2], [jm, 1])",
                                            "2 + im")) &&
                        (d_tests_same_value("cross([jm, km, 1], [1, im, jm])",
                                            "[-2im, 2, -2km]")),
                        "norm of the moduli; dot conjugating its first "
                        "operand, conj(a_i) b_i in that order; cross "
                        "nothing conjugated, each product in order (S388)");
        D_TESTS_EXPECT( (d_tests_dense_vectors<float>(rng)) &&
                        (d_tests_dense_vectors<double>(rng)) &&
                        (d_tests_dense_vectors<long double>(rng)),
                        "in float, double and long double: dot of dense "
                        "vectors the sum from 0 of conj(a_i) b_i, a complex "
                        "vector's elements conjugated as complex numbers; "
                        "cross in order; each quat.h's products, bit for bit; "
                        "norm the root of the parts' squares' sum (S388)");
        D_TESTS_EXPECT( (d_tests_mixed_values()),
                        "a matrix of values holding an inexact element: its "
                        "inverse the kernels' on its elements' operands in "
                        "long double, bit for bit, A inv(A) near I; its "
                        "trace, norm and eig in long double; solve(a, B) of "
                        "a complex a, cplx.h's quotients and quat.h's left "
                        "quotient, bit for bit, in long double and in float "
                        "(S388, S390)");
        D_TESTS_EXPECT( (d_tests_dense_edges()),
                        "a pivot counted by all four parts; a complex "
                        "element in a sum as the complex number it is, a "
                        "quaternion's -0 kept; an exact A's solve with a "
                        "dense real B the dense A's; under real_results a "
                        "matrix of quaternions' eigenvalues given, a real "
                        "matrix's complex ones refused (S385, S387, S388)");
        D_TESTS_EXPECT( (d_tests_numbers()),
                        "a quaternion as a scalar: det a domain error, inv, "
                        "trace, norm, dot conj(p) q, solve q^-1 B on the "
                        "left, quat.h's left quotient bit for bit where "
                        "dense; cross of numbers none (S390)");

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

    // d_tests_parts
    //   an eigenvalue's parts in long double, a real one's imaginary part 0.
    void
    d_tests_parts(
        const m::value& _x,
        long double&    _re,
        long double&    _im
    )
    {
        _re = 0.0L;
        _im = 0.0L;

        static_cast<void>(m::expr_real(_x).to(_re));
        static_cast<void>(m::expr_imag(_x).to(_im));

        return;
    }

    // d_tests_dense_ref
    //   a reference matrix's text as a dense matrix of quaternions of Type,
    // each part of the exact one's.
    template<typename Type>
    m::value
    d_tests_dense_ref(
        const char* _text
    )
    {
        const std::vector<m::value>       exact =
            d_tests_elements(d_tests_value_of(_text));
        std::vector<m::quaternion<Type> > q;

        for (std::size_t i = 0; i < exact.size(); ++i)
        {
            q.push_back(d_tests_dense_q<Type>(exact[i]));
        }

        return d_tests_matrix_of<Type>(
            m::expr_shape_of(d_tests_value_of(_text)).value().rows,
            m::expr_shape_of(d_tests_value_of(_text)).value().columns,
            q);
    }

    // d_tests_near_ref
    //   whether eig of A is the reference's, as many, each part within
    // 2^-_bits of 1 + |A|, in their order; and a Hermitian one's real.
    bool
    d_tests_near_ref(
        const m::value&             _a,
        const dj_tests_qmatrix_ref& _ref,
        int                         _bits,
        bool                        _hermitian
    )
    {
        const std::vector<m::value> values = d_tests_elements(
            m::expr_eig(_a).value());
        long double                 size   = 0.0L;
        bool                        ok     = ( (values.size() == _ref.n) &&
                                               (m::expr_norm(_a).value().to(
                                                    size)) );

        for (std::size_t i = 0; ( (ok) && (i < _ref.n) ); ++i)
        {
            long double re = 0.0L;
            long double im = 0.0L;

            d_tests_parts(values[i], re, im);

            ok = ( (std::fabs(re - d_tests_part_of(_ref.re[i])) <=
                        (std::ldexp(1.0L, -_bits) * (1.0L + size))) &&
                   (std::fabs(im - d_tests_part_of(_ref.im[i])) <=
                        (std::ldexp(1.0L, -_bits) * (1.0L + size))) &&
                   (im >= 0.0L) &&
                   ( (!_hermitian) ||
                     (!m::expr_is_complex(values[i])) ) );
        }

        return ok;
    }

    // d_tests_references
    //   each reference matrix's eig against mpmath's: exact, in long double,
    // within 2^-56 of 1 + |A|; dense, in double within 2^-44 and in float
    // within 2^-16; a Hermitian one's real.
    void
    d_tests_references()
    {
        const std::size_t count = ( sizeof(g_expr_qmatrix_refs) /
                                    sizeof(g_expr_qmatrix_refs[0]) );
        std::size_t       near  = 0;

        for (std::size_t k = 0; k < count; ++k)
        {
            const dj_tests_qmatrix_ref& ref = g_expr_qmatrix_refs[k];
            const m::value              A   = d_tests_value_of(ref.text);
            const bool hermitian = (d_tests_value_of(
                ("[" + std::string(ref.text) + "]' = [" +
                 std::string(ref.text) + "]").c_str()) == m::value(true));

            near += ( ( (d_tests_near_ref(A, ref, 56, hermitian)) &&
                        (d_tests_near_ref(d_tests_dense_ref<double>(ref.text),
                                          ref, 44, hermitian)) &&
                        (d_tests_near_ref(d_tests_dense_ref<float>(ref.text),
                                          ref, 16, hermitian)) )
                          ? 1 : 0 );
        }

        D_TESTS_EXPECT( (near == count) &&
                        (count >= 10),
                        "10 matrices' standard eigenvalues against mpmath's "
                        "at 400 bits: exact ones within 2^-56 of 1 + |A|, "
                        "dense doubles within 2^-44 and floats within "
                        "2^-16, each of imaginary part 0 or more, by real "
                        "part, then imaginary part; a Hermitian one's real "
                        "(math_tests_expr_qmatrix_refs.h, S389)");

        return;
    }

    // d_tests_adjoint_of
    //   the complex adjoint of a dense n by n matrix of quaternions, the
    // test's own: [A1, A2; -conj(A2), conj(A1)].
    template<typename Type>
    m::value
    d_tests_adjoint_of(
        const std::vector<m::quaternion<Type> >& _q,
        std::size_t                              _n
    )
    {
        std::vector<m::value> z(4 * _n * _n);
        const std::size_t     w = (2 * _n);

        for (std::size_t i = 0; i < _n; ++i)
        {
            for (std::size_t j = 0; j < _n; ++j)
            {
                const m::quaternion<Type>& e = _q[(i * _n) + j];

                z[(i * w) + j]             = m::expr_complex_of(
                    m::complex<Type>(e.w, e.x));
                z[(i * w) + _n + j]        = m::expr_complex_of(
                    m::complex<Type>(e.y, e.z));
                z[((_n + i) * w) + j]      = m::expr_complex_of(
                    m::complex<Type>(-e.y, e.z));
                z[((_n + i) * w) + _n + j] = m::expr_complex_of(
                    m::complex<Type>(e.w, -e.x));
            }
        }

        return m::expr_matrix(w, w, z).value();
    }

    // d_tests_one_of_each
    //   whether _standard, n eigenvalues of imaginary part above 0, are one
    // of each conjugate pair of _all, 2n: each one of _all bit for bit, the
    // pair's of the larger imaginary part, none twice.
    template<typename Type>
    bool
    d_tests_one_of_each(
        const std::vector<m::value>& _standard,
        const std::vector<m::value>& _all
    )
    {
        std::vector<bool> used(_all.size(), false);
        bool              ok = ( (2 * _standard.size()) == _all.size() );

        for (std::size_t i = 0; ( (ok) && (i < _standard.size()) ); ++i)
        {
            long double re = 0.0L;
            long double im = 0.0L;
            bool        found = false;

            d_tests_parts(_standard[i], re, im);

            for (std::size_t j = 0; ( (!found) && (j < _all.size()) ); ++j)
            {
                long double r2 = 0.0L;
                long double i2 = 0.0L;

                d_tests_parts(_all[j], r2, i2);

                found = ( (!used[j]) &&
                          (r2 == re) &&
                          (i2 == im) );
                used[j] = ( (used[j]) || (found) );
            }

            ok = ( (found) && (im >= 0.0L) );
        }

        return ok;
    }

    // d_tests_dense_eig
    //   S389 in Type: eig of a random matrix of quaternions one of each
    // pair of the language's eig of the test's own complex adjoint, bit for
    // bit; the sum of the standard eigenvalues and their conjugates the
    // adjoint's trace, 2 w's sum.
    template<typename Type>
    bool
    d_tests_dense_eig(
        d_tests_rng& _rng
    )
    {
        bool ok = true;

        for (int round = 0; round < 10; ++round)
        {
            const std::vector<m::quaternion<Type> > q =
                d_tests_random_q<Type>(_rng, 9);
            const m::value A   = d_tests_matrix_of<Type>(3, 3, q);
            const m::value eig = m::expr_eig(A).value();
            const m::value all = m::expr_eig(d_tests_adjoint_of<Type>(q, 3))
                                     .value();
            long double    sum = 0.0L;

            for (std::size_t i = 0; i < 3; ++i)
            {
                long double re = 0.0L;
                long double im = 0.0L;

                d_tests_parts(d_tests_elements(eig)[i], re, im);
                sum += (2.0L * re);
            }

            const long double trace = ( 2.0L * ( static_cast<long double>(
                                                     q[0].w) +
                                                 q[4].w + q[8].w ) );

            ok = ( (ok) &&
                   (d_tests_one_of_each<Type>(d_tests_elements(eig),
                                              d_tests_elements(all))) &&
                   (std::fabs(sum - trace) <=
                        (64.0L * std::numeric_limits<Type>::epsilon() *
                         (1.0L + std::fabs(trace)))) );
        }

        return ok;
    }

    // d_tests_similar
    //   eig of U A U^-1, U diagonal of quaternions, A's within 2^-40 of
    // 1 + |A|: Zhang's standard eigenvalues are similarity's invariants.
    bool
    d_tests_similar()
    {
        const char* const texts[] = { "[1, jm; km, 2]",
                                      "[jm, 1, 0; 0, km, 1; 1, 0, im]" };
        bool              ok      = true;

        for (std::size_t k = 0; k < 2; ++k)
        {
            const std::string a  = texts[k];
            const std::string u  = ( (k == 0) ? "[1 + jm, 0; 0, 2 - im + km]"
                                              : "[1 + jm, 0, 0; 0, 2km, 0; "
                                                "0, 0, 3 - im]" );
            const m::value    e1 = d_tests_value_of(
                ("eig(" + a + ")").c_str());
            const m::value    e2 = d_tests_value_of(
                ("eig(" + u + " * " + a + " * inv(" + u + "))").c_str());
            const std::vector<m::value> x = d_tests_elements(e1);
            const std::vector<m::value> y = d_tests_elements(e2);

            ok = ( (ok) && (x.size() == y.size()) && (!x.empty()) );

            for (std::size_t i = 0; ( (ok) && (i < x.size()) ); ++i)
            {
                long double r1 = 0.0L, i1 = 0.0L, r2 = 0.0L, i2 = 0.0L;

                d_tests_parts(x[i], r1, i1);
                d_tests_parts(y[i], r2, i2);

                ok = ( (std::fabs(r1 - r2) <= std::ldexp(1.0L, -40)) &&
                       (std::fabs(i1 - i2) <= std::ldexp(1.0L, -40)) );
            }
        }

        return ok;
    }

    // d_tests_scalar_eig
    //   S390: eig of a quaternion, its standard eigenvalue w + |v|i, exact
    // where |v| is rational; in each type quat.h's norm of v.
    template<typename Type>
    bool
    d_tests_scalar_eig_in()
    {
        const m::quaternion<Type> q = d_tests_q(Type(0.5), Type(-1.25),
                                                Type(2), Type(0.75));
        const Type                v = norm(m::quaternion<Type>(q.x, q.y,
                                                                q.z, Type(0)));
        m::complex<Type>          z;

        return ( (m::expr_complex_to(m::expr_eig(m::expr_quaternion_of(q))
                                         .value(),
                                     z)) &&
                 (d_tests_same_bits(z.real(), q.w)) &&
                 (d_tests_same_bits(z.imag(), v)) );
    }

    bool
    d_tests_scalar_eig()
    {
        long double root = 0.0L;

        static_cast<void>(m::internal::value_root_of(m::value(2)).to(root));

        return ( (d_tests_same_value("eig(1 + 3im + 4jm)", "1 + 5im")) &&
                 (d_tests_same_value("eig(-2 + 2im + 2jm + km)",
                                     "-2 + 3im")) &&
                 (d_tests_value_of("eig(1 + jm + km)") ==
                      m::expr_complex(m::value(1), m::value(root))) &&
                 (d_tests_same_value("eig(5)", "5")) &&
                 (d_tests_same_value("eig(2 - im)", "2 - im")) &&
                 (d_tests_scalar_eig_in<float>()) &&
                 (d_tests_scalar_eig_in<double>()) &&
                 (d_tests_scalar_eig_in<long double>()) );
    }

    // d_tests_dense_triangular
    //   S389 in Type: a dense triangular matrix's eig, upper or lower, its
    // diagonal's standard eigenvalues, w + |v|i, |v| quat.h's norm of v, bit
    // for bit, by real part.
    template<typename Type>
    bool
    d_tests_dense_triangular()
    {
        typedef m::quaternion<Type> q_type;

        const q_type                a = d_tests_q(Type(-1), Type(0.5),
                                                  Type(-2), Type(1));
        const q_type                b = d_tests_q(Type(3), Type(-0.25),
                                                  Type(1.5), Type(2));
        const m::value              U = d_tests_matrix_of<Type>(
            2, 2, { a,
                    d_tests_q(Type(1), Type(2), Type(3), Type(4)),
                    d_tests_q(Type(0), Type(0), Type(0), Type(0)),
                    b });
        const m::value              L = d_tests_matrix_of<Type>(
            2, 2, { a,
                    d_tests_q(Type(0), Type(0), Type(0), Type(0)),
                    d_tests_q(Type(1), Type(2), Type(3), Type(4)),
                    b });
        const std::vector<m::value> e = d_tests_elements(
            m::expr_eig(U).value());
        m::complex<Type>            z0;
        m::complex<Type>            z1;

        return ( (e.size() == 2) &&
                 (m::expr_eig(L).value() == m::expr_eig(U).value()) &&
                 (m::expr_complex_to(e[0], z0)) &&
                 (m::expr_complex_to(e[1], z1)) &&
                 (d_tests_same_bits(z0.real(), a.w)) &&
                 (d_tests_same_bits(z0.imag(),
                                    norm(q_type(a.x, a.y, a.z, Type(0))))) &&
                 (d_tests_same_bits(z1.real(), b.w)) &&
                 (d_tests_same_bits(z1.imag(),
                                    norm(q_type(b.x, b.y, b.z, Type(0))))) );
    }

    // d_tests_eig
    //   S389: a quaternion's; a triangular matrix's; the references; each
    // dense type, one of each pair; similarity; a NaN.
    void
    d_tests_eig()
    {
        d_tests_rng rng = { 0xe16e2ULL };

        D_TESTS_EXPECT( (d_tests_scalar_eig()),
                        "eig of a quaternion its standard eigenvalue, w + "
                        "|v|i: exact where |v| is rational, the long double "
                        "nearest otherwise, and in each type quat.h's norm "
                        "of v, bit for bit; of a real or complex number "
                        "itself (S390)");
        D_TESTS_EXPECT( (d_tests_same_value("eig([1 + 3jm, 5; 0, 2 - 4km])",
                                            "[1 + 3im, 2 + 4im]")) &&
                        (d_tests_same_value("eig([2, 0; jm, -1])",
                                            "[-1, 2]")) &&
                        (d_tests_same_value("eig([3km])", "[3im]")),
                        "a triangular matrix's its diagonal's standard "
                        "eigenvalues, exactly, a 1 by 1 among them (S389)");
        D_TESTS_EXPECT( (d_tests_dense_triangular<float>()) &&
                        (d_tests_dense_triangular<double>()) &&
                        (d_tests_dense_triangular<long double>()) &&
                        (d_tests_is_error(
                             m::internal::expr_matrix_eig_standard(
                                 m::value(D_MATH_VALUE_ERROR_DOMAIN)),
                             D_MATH_VALUE_ERROR_DOMAIN)),
                        "a dense triangular matrix's, in each type, its "
                        "diagonal's standard eigenvalues, quat.h's norm of "
                        "each v, bit for bit; the adjoint's error, where its "
                        "eigenvalues are not each found, passing (S389)");
        d_tests_references();
        D_TESTS_EXPECT( (d_tests_dense_eig<float>(rng)) &&
                        (d_tests_dense_eig<double>(rng)) &&
                        (d_tests_dense_eig<long double>(rng)),
                        "in float, double and long double: eig of a matrix "
                        "of quaternions one of each conjugate pair of the "
                        "language's eig of the test's own complex adjoint, "
                        "bit for bit, of imaginary part 0 or more; twice "
                        "their real parts' sum the adjoint's trace (S389)");
        D_TESTS_EXPECT( (d_tests_similar()),
                        "eig of U A U^-1, U diagonal of quaternions, A's: "
                        "the standard eigenvalues similarity's invariants");

        const double   nan = std::numeric_limits<double>::quiet_NaN();
        const m::value N   = d_tests_matrix_of<double>(
            2, 2, { d_tests_q(nan, 0.0, 0.0, 0.0),
                    d_tests_q(0.0, 0.0, 1.0, 0.0),
                    d_tests_q(0.0, 0.0, 0.0, 1.0),
                    d_tests_q(2.0, 0.0, 0.0, 0.0) });
        const std::vector<m::value> nans = d_tests_elements(
            m::expr_eig(N).value());
        long double re = 0.0L;
        long double im = 0.0L;

        if (!nans.empty())
        {
            d_tests_parts(nans[0], re, im);
        }

        D_TESTS_EXPECT( (nans.size() == 2) &&
                        (re != re),
                        "a matrix holding a NaN: NaNs");

        return;
    }


    //==========================================================================
    // 8.  ROTATIONS
    //==========================================================================

    // d_tests_rotmat_exact
    //   the test's own rotation matrix of the exact w + x im + y jm + z km,
    // R |q|^2 over |q|^2, as a text of the language.
    m::value
    d_tests_rotmat_exact(
        long _w,
        long _x,
        long _y,
        long _z
    )
    {
        const long n    = ( (_w * _w) + (_x * _x) + (_y * _y) + (_z * _z) );
        const long r[9] = { (_w * _w) + (_x * _x) - (_y * _y) - (_z * _z),
                            2 * ( (_x * _y) - (_w * _z) ),
                            2 * ( (_x * _z) + (_w * _y) ),
                            2 * ( (_x * _y) + (_w * _z) ),
                            (_w * _w) - (_x * _x) + (_y * _y) - (_z * _z),
                            2 * ( (_y * _z) - (_w * _x) ),
                            2 * ( (_x * _z) - (_w * _y) ),
                            2 * ( (_y * _z) + (_w * _x) ),
                            (_w * _w) - (_x * _x) - (_y * _y) + (_z * _z) };
        std::vector<m::value> e;

        for (std::size_t i = 0; i < 9; ++i)
        {
            e.push_back(m::value::ratio(r[i], n));
        }

        return m::expr_matrix(3, 3, e).value();
    }

    // d_tests_dense_rotations
    //   S391 in Type: rotmat quat.h's to_matrix3, rotquat its from_matrix3,
    // a quaternion of Type,
    // slerp its slerp of the normalized operands, each bit for bit.
    template<typename Type>
    bool
    d_tests_dense_rotations(
        d_tests_rng& _rng
    )
    {
        bool ok = true;

        for (int round = 0; round < 10; ++round)
        {
            const std::vector<m::quaternion<Type> > q =
                d_tests_random_q<Type>(_rng, 2);
            const Type                 t = static_cast<Type>(_rng.unit());
            const m::value             p = m::expr_quaternion_of(q[0]);
            const m::value             s = m::expr_quaternion_of(q[1]);
            const la::matrix<Type, 3, 3> R = to_matrix(q[0]);
            const m::value             rotm = m::expr_quaternion_matrix(p);
            const m::value             back =
                m::expr_quaternion_of_rotation(rotm);
            const std::vector<m::value> r   = d_tests_elements(rotm);

            ok = ( (ok) &&
                   (r.size() == 9) &&
                   (d_tests_storage_of(rotm) ==
                        m::internal::expr_matrix_storage_for<Type>()) );

            for (std::size_t i = 0; ( (ok) && (i < 9) ); ++i)
            {
                Type x = Type(0);

                ok = ( (r[i].to(x)) &&
                       (d_tests_same_bits(x, R((i / 3), (i % 3)))) );
            }

            ok = ( (ok) &&
                   (m::internal::expr_quaternion_type_of(back.c_value()) ==
                        m::value(Type(1)).kind()) &&
                   (d_tests_same_quaternion(d_tests_dense_q<Type>(back),
                                            m::from_matrix(R))) &&
                   (d_tests_same_quaternion(
                        d_tests_dense_q<Type>(m::expr_quaternion_slerp(
                            p, s, m::value(t))),
                        slerp(normalize(q[0]), normalize(q[1]), t))) );
        }

        return ok;
    }

    // d_tests_long_double_rotations
    //   rotquat of an exact matrix whose root is irrational, and slerp of
    // exact operands, quat.h's in long double, bit for bit.
    bool
    d_tests_long_double_rotations()
    {
        const long double        e[9] = { 1.0L, 0.0L, 0.0L, 0.0L, 0.0L,
                                          -1.0L, 0.0L, 1.0L, 0.0L };
        const m::quaternion<long double> turn =
            m::from_matrix(la::matrix<long double, 3, 3>(e));
        const m::quaternion<long double> half =
            slerp(normalize(d_tests_q(2.0L, 0.0L, 0.0L, 0.0L)),
                  normalize(d_tests_q(0.0L, 3.0L, 0.0L, 4.0L)),
                  1.0L / 3.0L);

        return ( (d_tests_same_quaternion(
                      d_tests_dense_q<long double>(d_tests_value_of(
                          "rotquat([1, 0, 0; 0, 0, -1; 0, 1, 0])")),
                      turn)) &&
                 (d_tests_same_quaternion(
                      d_tests_dense_q<long double>(d_tests_value_of(
                          "slerp(2, 3im + 4km, 1/3)")),
                      half)) );
    }

    // d_tests_rotations
    //   S391: rotmat, rotquat and slerp, exact and dense; their errors.
    void
    d_tests_rotations()
    {
        d_tests_rng rng = { 0x2077ULL };

        D_TESTS_EXPECT( (d_tests_value_of("rotmat(1 + 2im + 3jm + 4km)") ==
                             d_tests_rotmat_exact(1, 2, 3, 4)) &&
                        (d_tests_value_of("rotmat(2 - jm + 2km)") ==
                             d_tests_rotmat_exact(2, 0, -1, 2)) &&
                        (d_tests_same_value("rotmat(1 + im)",
                                            "[1, 0, 0; 0, 0, -1; 0, 1, 0]")) &&
                        (d_tests_same_value("rotmat(3)",
                                            "[1, 0, 0; 0, 1, 0; 0, 0, 1]")) &&
                        (d_tests_same_value("rotmat(jm) * [1; 0; 0]",
                                            "rotate(jm, [1; 0; 0])")),
                        "rotmat exact where q is, R |q|^2 over |q|^2, the "
                        "test's own; of a real number the identity; R v "
                        "rotate's q v q^-1 (S391)");
        D_TESTS_EXPECT( (d_tests_same_value(
                             "rotquat(rotmat(1 + im + jm + km))",
                             "1/2 + im/2 + jm/2 + km/2")) &&
                        (d_tests_same_value("rotquat([1, 0, 0; 0, 1, 0; "
                                            "0, 0, 1])", "1")) &&
                        (d_tests_same_value("rotquat([-1, 0, 0; 0, 1, 0; "
                                            "0, 0, -1])", "jm")) &&
                        (d_tests_same_value("rotquat([1, 0, 0; 0, -1, 0; "
                                            "0, 0, -1])", "im")) &&
                        (d_tests_same_value("rotquat([-1, 0, 0; 0, -1, 0; "
                                            "0, 0, 1])", "km")) &&
                        (d_tests_same_value(
                             "rotquat(rotmat(-2 + 3im + 6km))",
                             "2/7 - 3im/7 - 6km/7")) &&
                        (d_tests_same_value(
                             "rotquat([1, 0, 0, 5; 0, 0, -1, 6; 0, 1, 0, 7; "
                             "0, 0, 0, 1])",
                             "rotquat([1, 0, 0; 0, 0, -1; 0, 1, 0])")) &&
                        (d_tests_long_double_rotations()),
                        "rotquat exact where the root is rational, w >= 0, "
                        "rotquat of the identity 1; quat.h's in long double "
                        "otherwise, and slerp of exact operands; of a 4 by "
                        "4 its upper left (S391)");
        D_TESTS_EXPECT( (d_tests_dense_rotations<float>(rng)) &&
                        (d_tests_dense_rotations<double>(rng)) &&
                        (d_tests_dense_rotations<long double>(rng)),
                        "in float, double and long double: rotmat quat.h's "
                        "to_matrix3, rotquat its from_matrix3, slerp its "
                        "slerp of the normalized operands, bit for bit "
                        "(S391)");
        D_TESTS_EXPECT( (d_tests_domain("rotmat(0)")) &&
                        (d_tests_domain("rotmat([1, 2])")) &&
                        (d_tests_domain("rotquat([1, 0; 0, 1])")) &&
                        (d_tests_domain("rotquat([1, 0, 0; 0, im, 0; 0, 0, "
                                        "1])")) &&
                        (d_tests_domain("rotquat(jm)")) &&
                        (d_tests_domain("slerp(0, jm, 1/2)")) &&
                        (d_tests_domain("slerp(1, jm, im)")) &&
                        (d_tests_is_error(d_tests_value_of("slerp(1/0, jm, "
                                                           "[1])"),
                                          D_MATH_VALUE_ERROR_POLE)) &&
                        (d_tests_is_error(m::expr_quaternion_of_rotation(
                                              m::value(
                                                  D_MATH_VALUE_ERROR_POLE)),
                                          D_MATH_VALUE_ERROR_POLE)),
                        "an exact 0 names no rotation; rotmat of a matrix, "
                        "rotquat of no 3 by 3 or 4 by 4 matrix of real "
                        "numbers, slerp of no real t a domain error; an "
                        "error passes, the first (S391)");

        const m::quaternion<double> nans = d_tests_dense_q<double>(
            d_tests_value_of("slerp(x, 1, 1/2)",
                             m::expr_quaternion_of(
                                 d_tests_q(0.0, 0.0, 0.0, 0.0))));

        D_TESTS_EXPECT( (nans.w != nans.w) &&
                        (nans.x != nans.x) &&
                        (nans.y != nans.y) &&
                        (nans.z != nans.z),
                        "slerp from a dense 0, which names no rotation: "
                        "NaNs in its type (S391)");

        return;
    }


    //==========================================================================
    // 9.  THE FOLDS IN A TYPE
    //==========================================================================

    // d_tests_typed
    //   a text's value in Type, minus an infinity where the fold fails.
    template<typename Type>
    Type
    d_tests_typed(
        const char* _text
    )
    {
        m::expr_syntax syntax;

        syntax.implied_multiplication = true;

        const m::expr_result<Type> folded = m::expr_evaluate<Type>(
            m::expr_parse(_text, syntax).value(),
            m::at("x", m::value(0)));

        return ( folded.is_ok() ? folded.value()
                                : -std::numeric_limits<Type>::infinity() );
    }

    // d_tests_folds
    //   rotmat, rotquat and slerp NaNs in a type, as parts is (S391); a
    // literal of quaternions too.
    void
    d_tests_folds()
    {
        const double rotm = d_tests_typed<double>("rotmat(2)");
        const double rotq = d_tests_typed<double>("rotquat(2)");
        const double sl   = d_tests_typed<double>("slerp(1, 2, 0.5)");
        const m::complex<double> cz =
            d_tests_typed<m::complex<double> >("slerp(1, 2, 0.5)");

        D_TESTS_EXPECT( (rotm != rotm) &&
                        (rotq != rotq) &&
                        (sl != sl) &&
                        (cz.real() != cz.real()),
                        "rotmat, rotquat and slerp NaNs in a type, real or "
                        "complex, as parts is (S391)");

        return;
    }


    //==========================================================================
    // 10. THE MODULE
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
        d_tests_rotations();
        d_tests_folds();

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

    std::printf("t_expr_quaternion_matrix_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return ( (g_failures == 0) ? 0 : 1 );
}
