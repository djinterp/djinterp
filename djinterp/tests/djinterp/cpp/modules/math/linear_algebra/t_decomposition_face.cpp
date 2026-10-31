/*******************************************************************************
* djinterp [test]                                       t_decomposition_face.cpp
*
*   Tests for linear_algebra/decomposition.hpp, at every level from C++98,
* per family: LU of a known matrix -- its factors, P A = L U, determinant,
* solve and inverse -- and of a singular one; QR of a tall matrix -- Q
* orthogonal, R upper, Q R = A; Cholesky of a known SPD matrix -- its
* factor, determinant, solve -- and of an indefinite one; from C++14 an LU
* determinant as a constant expression.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/cpp/modules/math/linear_algebra/t_decomposition_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/linear_algebra/t_decomposition_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.07
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/linear_algebra/decomposition.hpp"
// std
#include <cstdio>   // std::printf


namespace
{
    namespace la = djinterp::math::linalg;

    long g_checks   = 0;
    long g_failures = 0;

    void d_tests_expect(
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
    bool d_tests_near(
        Float _a,
        Float _b,
        Float _tol
    )
    {
        const Float d = _a - _b;

        return ( (d <= _tol) &&
                 (d >= -_tol) );
    }

    template<typename    T,
             std::size_t R,
             std::size_t C>
    bool d_tests_near_m(
        const la::matrix<T, R, C>& _a,
        const la::matrix<T, R, C>& _b,
        T                          _tol
    )
    {
        for (std::size_t i = 0; i < R; ++i)
        {
            for (std::size_t j = 0; j < C; ++j)
            {
                if (!d_tests_near(_a(i, j), _b(i, j), _tol))
                {
                    return false;
                }
            }
        }

        return true;
    }

    template<typename T>
    void d_tests_family(
        T _tol
    )
    {
        typedef la::matrix<T, 3, 3> m3;
        typedef la::vector<T, 3>    v3;

        // A = [[2, 1, 1], [4, -6, 0], [-2, 7, 2]], det -16
        const T  ae[9] = { 2, 1, 1, 4, -6, 0, -2, 7, 2 };
        const m3 a(ae);
        const la::lu_decomposition<T, 3> f = la::lu(a);

        D_TESTS_EXPECT( (!f.is_singular()) &&
                        (d_tests_near(f.determinant(), T(-16), _tol * 16)) &&
                        (d_tests_near_m(f.p().times(a), f.l().times(f.u()),
                                        _tol * 16)),
                        "LU: P A = L U, the determinant");

        const v3 x = f.solve(v3(5, -2, 9));

        D_TESTS_EXPECT( (d_tests_near(x[0], T(1), _tol * 16)) &&
                        (d_tests_near(x[1], T(1), _tol * 16)) &&
                        (d_tests_near(x[2], T(2), _tol * 16)) &&
                        (d_tests_near_m(a.times(f.inverse()), m3::identity(),
                                        _tol * 16)),
                        "LU: solve and inverse");

        const T se[9] = { 1, 2, 3, 2, 4, 6, 0, 1, 1 };

        D_TESTS_EXPECT( (la::lu(m3(se)).is_singular()) &&
                        (la::lu(m3(se)).determinant() == T(0)),
                        "LU of a singular matrix");

        // QR of a 4-by-2 matrix
        typedef la::matrix<T, 4, 2> m42;

        const T   te[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        const m42 tall(te);
        const la::qr_decomposition<T, 4, 2> g = la::qr(tall);
        const la::matrix<T, 4, 4>             q = g.q();

        D_TESTS_EXPECT( (d_tests_near_m(q.transposed().times(q),
                                        la::matrix<T, 4, 4>::identity(),
                                        _tol * 16)) &&
                        (d_tests_near_m(q.times(g.r()), tall, _tol * 64)) &&
                        (d_tests_near(g.r()(1, 0), T(0), _tol * 16)),
                        "QR: Q orthogonal, R upper, Q R = A");

        // Cholesky of [[4, 2, 2], [2, 5, 3], [2, 3, 6]]: L = [[2], [1, 2],
        // [1, 1, 2]], det 64
        const T  ce[9] = { 4, 2, 2, 2, 5, 3, 2, 3, 6 };
        const m3 spd(ce);
        const la::cholesky_decomposition<T, 3> h = la::cholesky(spd);
        const T  le[9] = { 2, 0, 0, 1, 2, 0, 1, 1, 2 };

        D_TESTS_EXPECT( (h.is_spd()) &&
                        (h.l() == m3(le)) &&
                        (h.determinant() == T(64)) &&
                        (d_tests_near_m(m3(le).times(m3(le).transposed()),
                                        spd, _tol)),
                        "Cholesky: the factor and the determinant");

        const v3 y = h.solve(v3(8, 10, 11));

        D_TESTS_EXPECT( (d_tests_near(y[0], T(1), _tol * 16)) &&
                        (d_tests_near(y[1], T(1), _tol * 16)) &&
                        (d_tests_near(y[2], T(1), _tol * 16)),
                        "Cholesky: solve");

        const T ie[9] = { 1, 2, 0, 2, 1, 0, 0, 0, 1 };

        D_TESTS_EXPECT(!la::cholesky(m3(ie)).success(),
                       "Cholesky of an indefinite matrix");

        return;
    }
}  // namespace

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
constexpr
double d_tests_constexpr_det()
{
    const double e[4] = { 3, 1, 2, 4 };

    return la::lu(la::matrix<double, 2, 2>(e)).determinant();
}

static_assert(d_tests_constexpr_det() == 10.0,
              "an LU determinant as a constant expression, from C++14");
#endif


int main()
{
    d_tests_family<float>(1e-6f);
    d_tests_family<double>(1e-15);
    d_tests_family<long double>(1e-15L);

    std::printf("t_decomposition_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
