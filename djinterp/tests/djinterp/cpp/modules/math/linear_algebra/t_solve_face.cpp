/*******************************************************************************
* djinterp [cpp/math]                                           t_solve_face.cpp
*
*   Tests for linear_algebra/solve.hpp, at every level from C++98, per
* family: forward and back substitution on known triangular systems; solve
* with a vector and with a matrix right-hand side; solve_spd; least squares
* fitting a line through collinear points exactly, and through scattered
* ones to the normal equations' solution; from C++14 a solve as a constant
* expression.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/cpp/modules/math/linear_algebra/t_solve_face.cpp      \
*         -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/linear_algebra/t_solve_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.07
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/linear_algebra/solve.hpp"  // tested
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

    template<typename T, std::size_t N>
    bool
    d_tests_near_v(const la::vector<T, N>& _a,
                   const la::vector<T, N>& _b,
                   T                       _tol)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            const T d = _a[i] - _b[i];

            if ( (d > _tol) ||
                 (d < -_tol) )
            {
                return false;
            }
        }

        return true;
    }

    template<typename T>
    void
    d_tests_family(T _tol)
    {
        typedef la::matrix<T, 3, 3> m3;
        typedef la::vector<T, 3>    v3;

        // L = [[2], [1, 3], [4, 5, 6]]: L (1, 2, 3) = (2, 7, 32)
        const T le[9] = { 2, 0, 0, 1, 3, 0, 4, 5, 6 };
        const T ue[9] = { 2, 1, 4, 0, 3, 5, 0, 0, 6 };

        D_TESTS_EXPECT( (la::forward_substitution(m3(le), v3(2, 7, 32)) ==
                         v3(1, 2, 3)) &&
                        (la::back_substitution(m3(ue), v3(16, 21, 18)) ==
                         v3(1, 2, 3)),
                        "forward and back substitution");

        // A = [[2, 1, 1], [4, -6, 0], [-2, 7, 2]]: A (1, 1, 2) = (5, -2, 9)
        const T  ae[9] = { 2, 1, 1, 4, -6, 0, -2, 7, 2 };
        const m3 a(ae);
        la::matrix<T, 3, 2> rhs;

        rhs(0, 0) = 5;
        rhs(1, 0) = -2;
        rhs(2, 0) = 9;
        rhs(0, 1) = 10;
        rhs(1, 1) = -4;
        rhs(2, 1) = 18;

        const la::matrix<T, 3, 2> xs = la::solve(a, rhs);

        D_TESTS_EXPECT( (d_tests_near_v(la::solve(a, v3(5, -2, 9)),
                                        v3(1, 1, 2), _tol * 16)) &&
                        (d_tests_near_v(xs.col(1), v3(2, 2, 4), _tol * 32)),
                        "solve, a vector and a matrix right-hand side");

        // SPD [[4, 2, 2], [2, 5, 3], [2, 3, 6]]: S (1, 1, 1) = (8, 10, 11)
        const T se[9] = { 4, 2, 2, 2, 5, 3, 2, 3, 6 };

        D_TESTS_EXPECT(d_tests_near_v(la::solve_spd(m3(se), v3(8, 10, 11)),
                                      v3(1, 1, 1), _tol * 16),
                       "solve_spd");

        // a line through (0, 1), (1, 3), (2, 5), (3, 7): y = 1 + 2 x
        typedef la::matrix<T, 4, 2> m42;
        typedef la::vector<T, 4>    v4;

        const T   de[8] = { 1, 0, 1, 1, 1, 2, 1, 3 };
        const m42 design(de);

        D_TESTS_EXPECT(d_tests_near_v(la::least_squares(design,
                                                        v4(1, 3, 5, 7)),
                                      la::vector<T, 2>(1, 2), _tol * 64),
                       "least squares: collinear points, exactly");

        // scattered: (0, 1), (1, 2), (2, 2), (3, 4) -> y = 0.9 + 0.9 x
        D_TESTS_EXPECT(d_tests_near_v(la::least_squares(design,
                                                        v4(1, 2, 2, 4)),
                                      la::vector<T, 2>(T(0.9), T(0.9)),
                                      _tol * 64),
                       "least squares: the normal equations' fit");

        return;
    }
}  // namespace

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
constexpr double
d_tests_constexpr_solve()
{
    const double e[4] = { 2, 1, 1, 3 };

    return la::solve(la::matrix<double, 2, 2>(e),
                     la::vector<double, 2>(3.0, 5.0))[1];
}

// (near, not equal: g++ for an x87 target evaluates the constant at long
// double's precision)
static_assert( (d_tests_constexpr_solve() > 1.4 - 1e-12) &&
               (d_tests_constexpr_solve() < 1.4 + 1e-12),
              "a solve as a constant expression, from C++14");
#endif


int
main()
{
    d_tests_family<float>(1e-6f);
    d_tests_family<double>(1e-15);
    d_tests_family<long double>(1e-15L);

    std::printf("t_solve_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
