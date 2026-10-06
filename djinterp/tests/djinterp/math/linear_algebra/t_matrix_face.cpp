/*******************************************************************************
* djinterp [test]                                              t_matrix_face.cpp
*
*   Tests for linear_algebra/matrix.hpp, at every level from C++98. The
* compile-time half -- the traits, and from C++14 the operations as constant
* expressions -- runs wherever the unit compiles. The runtime half checks
* what the face adds to the C core:
*
*     - the generic path agrees with the C families, bit for bit: matrices of
*       t_real, a user's type wrapping a double, against the same matrices
*       of double, every operation, square and not;
*     - float, double and long double matrices against the identities: the
*       transpose of the transpose, A * I = A, A + A^T symmetric, A^0 = I,
*       (AB)^T = B^T A^T, the product with a vector against row dot products;
*     - integer matrices (the generic path), the factories, the operators and
*       procedural spellings, and from C++11 std::array and the aliases.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/linear_algebra/t_matrix_face.cpp -o t_matrix -lm
*
*
* path:      /tests/djinterp/math/linear_algebra/t_matrix_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "../../../../inc/djinterp/math/linear_algebra/matrix.hpp"  // tested
// std
#include <cstdio>   // std::printf


namespace
{
    using djinterp::math::linalg::matrix;
    using djinterp::math::linalg::vector;

    // t_real
    //   a user's numeric type, wrapping a double: not arithmetic, so the
    // generic path serves it.
    struct t_real
    {
        double v;

        t_real() : v(0.0) {}
        t_real(double _v) : v(_v) {}

        operator double() const { return v; }

        t_real& operator+=(const t_real& _o) { v += _o.v; return *this; }
        t_real& operator*=(const t_real& _o) { v *= _o.v; return *this; }
    };

    t_real operator+(t_real _a, t_real _b) { return t_real(_a.v + _b.v); }
    t_real operator-(t_real _a, t_real _b) { return t_real(_a.v - _b.v); }
    t_real operator*(t_real _a, t_real _b) { return t_real(_a.v * _b.v); }
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

    // the same once stored: x87 can keep excess precision in a register
    template<typename Float>
    bool
    d_tests_same(Float _a, Float _b)
    {
        volatile Float a = _a;
        volatile Float b = _b;

        return (a == b);
    }

    unsigned long g_state = 362436069u;

    double
    d_tests_random()
    {
        g_state = (g_state * 1103515245u) + 12345u;

        return (static_cast<double>((g_state >> 4) % 2000003u) / 250000.0) -
               4.0;
    }

    template<std::size_t R, std::size_t C>
    bool
    d_tests_parity(
        const matrix<double, R, C>& _d,
        const matrix<t_real, R, C>& _r
    )
    {
        for (std::size_t i = 0; i < R; ++i)
        {
            for (std::size_t j = 0; j < C; ++j)
            {
                if (!d_tests_same(_d(i, j), _r(i, j).v))
                {
                    return false;
                }
            }
        }

        return true;
    }

    // d_tests_generic_parity
    //   t_real matrices (the generic path) against double ones (the C core),
    // every operation, bit for bit.
    void
    d_tests_generic_parity()
    {
        for (int trial = 0; trial < 5000; ++trial)
        {
            double a_d[6];
            double b_d[12];
            double s_d[9];
            double v_d[3];
            t_real a_r[6];
            t_real b_r[12];
            t_real s_r[9];
            t_real v_r[3];

            for (int i = 0; i < 12; ++i)
            {
                b_d[i] = d_tests_random();
                b_r[i] = t_real(b_d[i]);
            }

            for (int i = 0; i < 9; ++i)
            {
                s_d[i] = d_tests_random() / 4.0;
                s_r[i] = t_real(s_d[i]);
            }

            for (int i = 0; i < 6; ++i)
            {
                a_d[i] = d_tests_random();
                a_r[i] = t_real(a_d[i]);
            }

            for (int i = 0; i < 3; ++i)
            {
                v_d[i] = d_tests_random();
                v_r[i] = t_real(v_d[i]);
            }

            const matrix<double, 2, 3> a(a_d);
            const matrix<t_real, 2, 3> ar(a_r);
            const matrix<double, 3, 4> b(b_d);
            const matrix<t_real, 3, 4> br(b_r);
            const matrix<double, 3, 3> sq(s_d);
            const matrix<t_real, 3, 3> sqr(s_r);
            const vector<double, 3>    v(v_d);
            const vector<t_real, 3>    vr(v_r);
            const double               s = d_tests_random();

            D_TESTS_EXPECT(d_tests_parity(a.times(b), ar.times(br)),
                           "a product, generic and C");
            D_TESTS_EXPECT(d_tests_parity(a.transposed(), ar.transposed()),
                           "a transpose, generic and C");
            D_TESTS_EXPECT(d_tests_parity(a.plus(a.scaled(s)),
                                          ar.plus(ar.scaled(s))),
                           "sums and scaling, generic and C");
            D_TESTS_EXPECT(d_tests_parity(a.minus(a.hadamard(a)).negated(),
                                          ar.minus(ar.hadamard(ar))
                                              .negated()),
                           "differences, generic and C");
            D_TESTS_EXPECT(d_tests_parity(sq.power(4), sqr.power(4)),
                           "a power, generic and C");
            D_TESTS_EXPECT(d_tests_same(sq.trace(), sqr.trace().v),
                           "trace, generic and C");
            D_TESTS_EXPECT(d_tests_same(sq.norm(), sqr.norm().v),
                           "the norm, generic and C");
            D_TESTS_EXPECT(sq.is_symmetric() == sqr.is_symmetric(),
                           "symmetry, generic and C");
            D_TESTS_EXPECT( (ar == ar) &&
                            (sqr.equals(sqr)),
                            "equality, generic");

            const vector<double, 3> mv  = sq.times(v);
            const vector<t_real, 3> mvr = sqr.times(vr);

            D_TESTS_EXPECT( (d_tests_same(mv[0], mvr[0].v)) &&
                            (d_tests_same(mv[1], mvr[1].v)) &&
                            (d_tests_same(mv[2], mvr[2].v)),
                            "a matrix-vector product, generic and C");
        }

        return;
    }

    // d_tests_family
    //   one C family against the identities.
    template<typename Type>
    void
    d_tests_family()
    {
        for (int trial = 0; trial < 2000; ++trial)
        {
            Type a_e[9];
            Type b_e[9];

            for (int i = 0; i < 9; ++i)
            {
                a_e[i] = static_cast<Type>(d_tests_random());
                b_e[i] = static_cast<Type>(d_tests_random());
            }

            const matrix<Type, 3, 3> a(a_e);
            const matrix<Type, 3, 3> b(b_e);
            const matrix<Type, 3, 3> id = matrix<Type, 3, 3>::identity();

            D_TESTS_EXPECT(a.transposed().transposed() == a,
                           "the transpose of the transpose");
            D_TESTS_EXPECT( (a.times(id) == a) &&
                            (id.times(a) == a),
                            "the identity is the product's unit");
            D_TESTS_EXPECT(a.plus(a.transposed()).is_symmetric(
                               static_cast<Type>(0)),
                           "A + A^T is symmetric");
            D_TESTS_EXPECT( (a.power(0) == id) &&
                            (a.power(1) == a.times(id)) &&
                            (a.power(2) == a.times(a.times(id))),
                            "powers");
            D_TESTS_EXPECT(a.times(b).transposed().equals(
                               b.transposed().times(a.transposed()),
                               static_cast<Type>(1e-3)),
                           "(AB)^T = B^T A^T, to rounding");
            D_TESTS_EXPECT(id.is_identity(static_cast<Type>(0)),
                           "the identity is the identity");

            const vector<Type, 3> v(a_e[0], a_e[1], a_e[2]);
            const vector<Type, 3> av = a.times(v);

            D_TESTS_EXPECT( (av[0] == a.row(0).dot(v)) &&
                            (av[1] == a.row(1).dot(v)) &&
                            (av[2] == a.row(2).dot(v)) &&
                            (a(v) == av),
                            "a matrix-vector product is row dot products");
        }

        // shapes that are not square
        const Type               e23[6]  = { 1, 2, 3, 4, 5, 6 };
        const Type               e32[6]  = { 7, 8, 9, 10, 11, 12 };
        const matrix<Type, 2, 3> m23(e23);
        const matrix<Type, 3, 2> m32(e32);
        const matrix<Type, 2, 2> p = m23.times(m32);

        D_TESTS_EXPECT( (p(0, 0) == static_cast<Type>(58)) &&
                        (p(0, 1) == static_cast<Type>(64)) &&
                        (p(1, 0) == static_cast<Type>(139)) &&
                        (p(1, 1) == static_cast<Type>(154)),
                        "a 2x3 by 3x2 product");
        D_TESTS_EXPECT( (m23.transposed()(2, 1) == static_cast<Type>(6)) &&
                        (m23.col(2)[1] == static_cast<Type>(6)) &&
                        (m23.row(1)[0] == static_cast<Type>(4)),
                        "rows, columns and the transpose of a 2x3");
        D_TESTS_EXPECT( (m23 * static_cast<Type>(2) == m23.scaled(2)) &&
                        (static_cast<Type>(2) * m23 == m23.scaled(2)) &&
                        (m23 + m23 == m23.scaled(2)) &&
                        (m23 - m23 == matrix<Type, 2, 3>::zeros()) &&
                        (-m23 == m23.negated()) &&
                        (m23 * m32 == p) &&
                        (m23 != m23.with(0, 0, static_cast<Type>(9))),
                        "the operators");
        D_TESTS_EXPECT( (multiply(m23, m32) == p) &&
                        (transpose(m23) == m23.transposed()) &&
                        (add(m23, m23) == m23.scaled(2)) &&
                        (trace(p) == static_cast<Type>(212)) &&
                        (d_tests_same(frobenius_norm(p), p.norm())) &&
                        (approx_equal(p, p)),
                        "the procedural spellings");
        D_TESTS_EXPECT( (djinterp::math::linalg::identity<Type, 2>() ==
                         matrix<Type, 2, 2>::identity()) &&
                        (matrix<Type, 2, 2>::diagonal(
                             vector<Type, 2>(3, 4))(1, 1) ==
                         static_cast<Type>(4)) &&
                        (matrix<Type, 2, 3>::filled(static_cast<Type>(1))
                             .norm_squared() == static_cast<Type>(6)),
                        "the factories");

        return;
    }

    struct t_double
    {
        double operator()(double _x) const { return _x * 2.0; }
    };

    void
    d_tests_integers()
    {
        const int            e[4] = { 1, 2, 3, 4 };
        const matrix<int, 2, 2> m(e);

        D_TESTS_EXPECT( (m.power(2)(0, 0) == 7) &&
                        (m.power(2)(1, 1) == 22) &&
                        (m.trace() == 5) &&
                        (m.transposed()(0, 1) == 3),
                        "an int matrix");
        D_TESTS_EXPECT(m.norm() == 5, "an int norm: through double, truncated");

        const double            d[4] = { 1, 2, 3, 4 };
        const matrix<double, 2, 2> md(d);

        D_TESTS_EXPECT(md.map(t_double()).trace() == 10.0, "map");

        return;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    void
    d_tests_cpp11()
    {
        const std::array<double, 4> parts = { { 1.0, 2.0, 3.0, 4.0 } };
        const djinterp::math::linalg::matrix2<> m2(parts);

        D_TESTS_EXPECT( (m2.trace() == 5.0) &&
                        (djinterp::math::linalg::mat2d::from_row_major(parts)
                             == m2),
                        "std::array interop and the aliases");

        return;
    }
#endif
}  // namespace

D_STATIC_ASSERT((djinterp::math::linalg::is_matrix<
                     matrix<double, 2, 3> >::value),
                "is_matrix");
D_STATIC_ASSERT((djinterp::math::linalg::is_square_matrix<
                     matrix<double, 3, 3> >::value),
                "is_square_matrix");
D_STATIC_ASSERT((!djinterp::math::linalg::is_square_matrix<
                     matrix<double, 2, 3> >::value),
                "a 2x3 is not square");
D_STATIC_ASSERT((sizeof(matrix<float, 3, 3>) == 9 * sizeof(float)),
                "a matrix is its entries and nothing more");

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
static_assert(matrix<double, 3, 3>::identity().power(5).trace() == 3.0,
              "constexpr power and trace");
// (the literal cast to double: an x87 compiler reads a double literal at 64
// bits)
static_assert(matrix<double, 2, 2>::identity().scaled(2).norm() ==
                  static_cast<double>(2.8284271247461903),
              "constexpr Frobenius norm, correctly rounded");
#endif


int
main()
{
    d_tests_generic_parity();
    d_tests_family<float>();
    d_tests_family<double>();
    d_tests_family<long double>();
    d_tests_integers();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_cpp11();
#endif

    std::printf("t_matrix_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
