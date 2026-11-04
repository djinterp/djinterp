/*******************************************************************************
* djinterp [cpp/math]                                             t_svd_face.cpp
*
*   Tests for linear_algebra/svd.hpp, at every level from C++98, per family:
* the SVD of a known matrix -- its singular values, U S V^T = A, V
* orthogonal, rank and condition number -- and of a rank-deficient one;
* the pseudoinverse as the inverse of a square nonsingular matrix, as the
* least-squares solver of a tall one, and of a wide one through the
* transpose.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/linear_algebra/t_svd_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/linear_algebra/t_svd_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.07
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/linear_algebra/svd.hpp"  // tested
// std
#include <cstdio>   // std::printf


namespace
{
    namespace la = djinterp::math::linalg;

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
    d_tests_near(Float _a, Float _b, Float _tol)
    {
        const Float d = _a - _b;

        return ( (d <= _tol) &&
                 (d >= -_tol) );
    }

    template<typename T, std::size_t R, std::size_t C>
    bool
    d_tests_near_m(const la::matrix<T, R, C>& _a,
                   const la::matrix<T, R, C>& _b,
                   T                          _tol)
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
    void
    d_tests_family(T _tol)
    {
        // [[3, 0], [4, 5], [0, 0]]: singular values sqrt(45) and sqrt(5)
        typedef la::matrix<T, 3, 2> m32;

        const T   ae[6] = { 3, 0, 4, 5, 0, 0 };
        const m32 a(ae);
        const la::svd_decomposition<T, 3, 2> d = la::svd(a);

        D_TESTS_EXPECT( (d_tests_near(d.singular_values()[0],
                                      T(6.7082039324993690892L), _tol * 16)) &&
                        (d_tests_near(d.singular_values()[1],
                                      T(2.2360679774997896964L), _tol * 16)) &&
                        (d_tests_near_m(d.u().times(d.sigma())
                                            .times(d.v().transposed()),
                                        a, _tol * 32)) &&
                        (d_tests_near_m(d.v().transposed().times(d.v()),
                                        la::matrix<T, 2, 2>::identity(),
                                        _tol * 16)) &&
                        (d.rank() == 2) &&
                        (d_tests_near(d.condition_number(), T(3), _tol * 16)),
                        "the SVD of a known matrix");

        // a rank-1 matrix: one non-zero singular value
        const T   be[6] = { 1, 2, 2, 4, 3, 6 };
        const la::svd_decomposition<T, 3, 2> e = la::svd(m32(be));

        D_TESTS_EXPECT( (e.rank() == 1) &&
                        (d_tests_near(e.singular_values()[0],
                                      T(8.3666002653407554798L), _tol * 16)),
                        "a rank-deficient matrix");

        // the pseudoinverse: the inverse of a square nonsingular matrix, and
        // the least-squares solver of a tall one
        typedef la::matrix<T, 2, 2> m22;

        const T   ce[4] = { 4, 7, 2, 6 };
        const m22 c(ce);
        const T   ie[4] = { T(0.6), T(-0.7), T(-0.2), T(0.4) };

        D_TESTS_EXPECT( (d_tests_near_m(la::pinv(c), m22(ie), _tol * 16)) &&
                        (d_tests_near_m(la::pinv(a).times(a),
                                        la::matrix<T, 2, 2>::identity(),
                                        _tol * 16)),
                        "the pseudoinverse: an inverse, a left inverse");

        // a wide matrix, through the transpose
        typedef la::matrix<T, 2, 3> m23;

        const m23 w = a.transposed();

        D_TESTS_EXPECT(d_tests_near_m(w.times(la::pseudoinverse(w)),
                                      la::matrix<T, 2, 2>::identity(),
                                      _tol * 16),
                       "the pseudoinverse of a wide matrix: a right inverse");

        return;
    }
}  // namespace


int
main()
{
    d_tests_family<float>(1e-6f);
    d_tests_family<double>(1e-15);
    d_tests_family<long double>(1e-15L);

    std::printf("t_svd_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
