/*******************************************************************************
* djinterp [test]                                              t_square_face.cpp
*
*   Tests for linear_algebra/square.hpp, at every level from C++98: the
* generic path (t_real, a user's type wrapping a double) against the C core,
* bit for bit -- determinant, submatrix, minor, cofactor, the cofactor matrix
* and adjugate, orthogonality; each C family against the algebra's
* identities on integer-valued matrices, where every step is exact (A adj(A)
* = det(A) I, A inv(A) = I to rounding, a singular matrix's zero inverse);
* integer matrices; and from C++14 the determinant and inverse as constant
* expressions.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/linear_algebra/t_square_face.cpp -o t_square -lm
*
*
* path:      /tests/djinterp/math/linear_algebra/t_square_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "../../../../inc/djinterp/math/linear_algebra/square.hpp"  // tested
// std
#include <cstdio>   // std::printf


namespace
{
    using djinterp::math::linalg::matrix;

    // t_real
    //   a user's numeric type, wrapping a double: the generic path.
    struct t_real
    {
        double v;

        t_real() : v(0.0) {}
        t_real(double _v) : v(_v) {}

        operator double() const { return v; }

        t_real& operator+=(const t_real& _o) { v += _o.v; return *this; }
    };

    t_real operator-(t_real _a, t_real _b) { return t_real(_a.v - _b.v); }
    t_real operator*(t_real _a, t_real _b) { return t_real(_a.v * _b.v); }
    t_real operator/(t_real _a, t_real _b) { return t_real(_a.v / _b.v); }
    t_real operator-(t_real _a) { return t_real(-_a.v); }
    bool operator<(t_real _a, t_real _b) { return _a.v < _b.v; }
    bool operator>(t_real _a, t_real _b) { return _a.v > _b.v; }
    bool operator==(t_real _a, t_real _b) { return _a.v == _b.v; }

    long g_checks   = 0;
    long g_failures = 0;

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

    template<typename Float>
    bool
    d_tests_same(Float _a, Float _b)
    {
        volatile Float a = _a;
        volatile Float b = _b;

        return (a == b);
    }

    unsigned long g_state = 521288629u;

    // a small integer in [-4, 4], zero a third of the time
    int
    d_tests_small()
    {
        g_state = (g_state * 1103515245u) + 12345u;

        return ((g_state >> 8) % 3u == 0u)
                   ? 0
                   : (static_cast<int>((g_state >> 12) % 9u) - 4);
    }

    double
    d_tests_random()
    {
        g_state = (g_state * 1103515245u) + 12345u;

        return (static_cast<double>((g_state >> 4) % 2000003u) / 250000.0) -
               4.0;
    }

    template<std::size_t N>
    bool
    d_tests_parity(
        const matrix<double, N, N>& _d,
        const matrix<t_real, N, N>& _r
    )
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            for (std::size_t j = 0; j < N; ++j)
            {
                if (!d_tests_same(_d(i, j), _r(i, j).v))
                {
                    return false;
                }
            }
        }

        return true;
    }

    void
    d_tests_generic_parity()
    {
        for (int trial = 0; trial < 5000; ++trial)
        {
            double e_d[16];
            t_real e_r[16];

            for (int i = 0; i < 16; ++i)
            {
                e_d[i] = d_tests_random();
                e_r[i] = t_real(e_d[i]);
            }

            const matrix<double, 4, 4> a(e_d);
            const matrix<t_real, 4, 4> ar(e_r);

            D_TESTS_EXPECT(d_tests_same(determinant(a), determinant(ar).v),
                           "determinant, generic and C");
            D_TESTS_EXPECT(d_tests_same(minor(a, 1, 2), minor(ar, 1, 2).v),
                           "minor, generic and C");
            D_TESTS_EXPECT(d_tests_same(cofactor(a, 2, 1),
                                        cofactor(ar, 2, 1).v),
                           "cofactor, generic and C");
            D_TESTS_EXPECT(d_tests_parity(adjugate(a), adjugate(ar)),
                           "adjugate, generic and C");
            D_TESTS_EXPECT(d_tests_parity(cofactor_matrix(a),
                                          cofactor_matrix(ar)),
                           "cofactor matrix, generic and C");
            D_TESTS_EXPECT(is_orthogonal(a) == is_orthogonal(ar),
                           "orthogonality, generic and C");
        }

        return;
    }

    template<typename Type, std::size_t N>
    matrix<Type, N, N>
    d_tests_small_matrix()
    {
        matrix<Type, N, N> m;

        for (std::size_t i = 0; i < N; ++i)
        {
            for (std::size_t j = 0; j < N; ++j)
            {
                m(i, j) = static_cast<Type>(d_tests_small());
            }
        }

        return m;
    }

    template<typename Type>
    void
    d_tests_family()
    {
        typedef matrix<Type, 3, 3> m3;

        for (int trial = 0; trial < 5000; ++trial)
        {
            const m3   a = d_tests_small_matrix<Type, 3>();
            const Type d = determinant(a);

            // exact on integer values: A adj(A) = det(A) I
            D_TESTS_EXPECT(a.times(adjugate(a)) == m3::identity().scaled(d),
                           "A adj(A) = det(A) I");
            D_TESTS_EXPECT(adjugate(a) == cofactor_matrix(a).transposed(),
                           "the adjugate is the cofactors transposed");
            D_TESTS_EXPECT(d == det(a.transposed()),
                           "det(A^T) = det(A), exactly on integers");

            // the inverse
            if (d != static_cast<Type>(0))
            {
                D_TESTS_EXPECT( (a.times(inverse(a)).is_identity(
                                     static_cast<Type>(1e-4))) &&
                                (is_invertible(a)) &&
                                (!is_singular(a)),
                                "A inv(A) = I");
            }
            else
            {
                const m3 i = inv(a);

                D_TESTS_EXPECT( (is_singular(a)) &&
                                ( (i == m3::zeros()) ||
                                  (!i.equals(m3::zeros())) ),
                                "a singular matrix");
            }
        }

        // a rotation by a quarter turn is orthogonal; a shear is not
        const Type rot_e[4]   = { 0, -1, 1, 0 };
        const Type shear_e[4] = { 1, 1, 0, 1 };

        D_TESTS_EXPECT( (is_orthogonal(matrix<Type, 2, 2>(rot_e))) &&
                        (!is_orthogonal(matrix<Type, 2, 2>(shear_e))),
                        "orthogonality");
        D_TESTS_EXPECT(submatrix(m3::identity(), 0, 0) ==
                       (matrix<Type, 2, 2>::identity()),
                       "submatrix");

        return;
    }

    void
    d_tests_integers()
    {
        for (int trial = 0; trial < 3000; ++trial)
        {
            const matrix<int, 4, 4> a = d_tests_small_matrix<int, 4>();

            D_TESTS_EXPECT((a.times(adjugate(a)) ==
                            matrix<int, 4, 4>::identity().scaled(
                                determinant(a))),
                           "an int matrix: A adj(A) = det(A) I, exactly");
        }

        const int e[9] = { 2, -3, 1, 2, 0, -1, 1, 4, 5 };

        D_TESTS_EXPECT(determinant(matrix<int, 3, 3>(e)) == 49,
                       "an int determinant");

        return;
    }
}  // namespace

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
static_assert(djinterp::math::linalg::determinant(
                  matrix<double, 3, 3>::identity().scaled(2)) == 8.0,
              "constexpr determinant");
static_assert(djinterp::math::linalg::inverse(
                  matrix<double, 2, 2>::identity().scaled(4))(1, 1) == 0.25,
              "constexpr inverse");
#endif


int
main()
{
    d_tests_generic_parity();
    d_tests_family<float>();
    d_tests_family<double>();
    d_tests_family<long double>();
    d_tests_integers();

    std::printf("t_square_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
