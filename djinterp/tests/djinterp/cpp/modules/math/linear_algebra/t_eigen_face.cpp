/*******************************************************************************
* djinterp [cpp/math]                                           t_eigen_face.cpp
*
*   Tests for linear_algebra/eigen.hpp, at every level from C++98, per
* family: the symmetric eigendecomposition of a known matrix -- its
* eigenvalues ascending, A V = V L, V orthogonal; power iteration's
* dominant eigenpair; the general eigenvalues (chunk 38: balancing, the
* Hessenberg form and Francis's double-shift QR) of a 2-by-2, an
* upper-triangular 3-by-3, a
* non-symmetric 3-by-3 with eigenvalues 1, 2, 3, and random symmetric
* 5-by-5s against the Jacobi solver's, converged() set; a cyclic shift's
* (a complex pair) and a NaN's, converged() clear; 2^20 A's, 2^20 times A's
* exactly; and the general solver's eigenpairs as math::complex<T> (chunk
* 39), in float too: each eigenvalue's real part eigenvalues_qr's bit for
* bit, A x = l x, x unit with an entry of the largest modulus real and
* positive, a pair exactly conjugate, its vectors too -- of [[4 1] [2 3]],
* the non-symmetric 3-by-3, the cyclic shift's pair, -i and i of a right
* angle's rotation, and 50 random 5-by-5s, most with pairs; a NaN's every
* eigenvalue and vector NaNs, converged() clear; e_k for a diagonal, and
* +-(1, -1)/sqrt 2 for [[2 1] [1 2]], its larger entry positive. From
* C++14, the 3-by-3's eigenvalues as a constant expression, equal to the
* run time's bit for bit, and the 2-by-2's eigenvector as one.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/linear_algebra/t_eigen_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/linear_algebra/t_eigen_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.30
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/linear_algebra/eigen.hpp"  // tested
// std
#include <cstdio>   // std::printf
#include <limits>   // std::numeric_limits


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

    // a value in [-1, 1) from a 32-bit linear congruential generator
    unsigned long g_state = 12345UL;

    double
    d_tests_random()
    {
        g_state = ((g_state * 1103515245UL) + 12345UL) & 0xFFFFFFFFUL;

        return (static_cast<double>(g_state >> 8) / 8388608.0) - 1.0;
    }

    // the general solver's eigenpairs of _a (S308): every eigenvalue
    // found, its real part eigenvalues_qr's bit for bit; a pair's two side
    // by side, the negative first, exactly conjugate, their vectors too;
    // each eigenvector of unit length, an entry of the largest modulus real
    // and positive, and A x within _bound of l x, entry by entry
    template<typename    T,
             std::size_t N>
    bool
    d_tests_general(
        const la::matrix<T, N, N>& _a,
        T                          _bound
    )
    {
        typedef djinterp::math::complex<T> C;

        const T                        eps = std::numeric_limits<T>::epsilon();
        const la::general_eigen<T, N>  g   = la::eigen_general(_a);
        const la::eigenvalue_set<T, N> q   = la::eigenvalues_qr(_a);
        la::matrix<C, N, N>            ac;
        bool                           ok  = g.converged();

        for (std::size_t k = 0; k < (N * N); ++k)
        {
            ac(k / N, k % N) = C(_a(k / N, k % N));
        }

        for (std::size_t k = 0; k < N; ++k)
        {
            const C                l    = g.eigenvalue(k);
            const la::vector<C, N> x    = g.eigenvector(k);
            const la::vector<C, N> ax   = ac.times(x);
            T                      unit = T(0);
            T                      big  = T(0);
            bool                   lead = false;

            ok = ( (ok) &&
                   (l.real() == q.values()[k]) );

            // a pair: its conjugate next, and the conjugate vector
            if (l.imag() < T(0))
            {
                ok = ( (ok) &&
                       (k + 1 < N) &&
                       (g.eigenvalue(k + 1) == conj(l)) );

                for (std::size_t i = 0; ( (ok) && (i < N) ); ++i)
                {
                    ok = (g.eigenvector(k + 1)[i] == conj(x[i]));
                }
            }

            for (std::size_t i = 0; i < N; ++i)
            {
                const T m = abs(x[i]);

                ok    = ( (ok) &&
                          (abs(ax[i] - (l * x[i])) <= _bound) );
                unit += m * m;
                big   = (m > big) ? m : big;
            }

            // an entry of the largest modulus, to rounding, real and positive
            for (std::size_t i = 0; i < N; ++i)
            {
                lead = ( (lead) ||
                         ( (x[i].imag() == T(0)) &&
                           (x[i].real() > T(0)) &&
                           (abs(x[i]) >= big * (T(1) - (T(4) * eps))) ) );
            }

            ok = ( (ok) &&
                   (lead) &&
                   (d_tests_near(unit, T(1), T(8) * eps)) );
        }

        return ok;
    }

    template<typename T>
    void
    d_tests_family(T _tol)
    {
        typedef la::matrix<T, 3, 3> m3;

        // [[2, 1, 0], [1, 2, 1], [0, 1, 2]]: 2 - sqrt 2, 2, 2 + sqrt 2
        const T  se[9] = { 2, 1, 0, 1, 2, 1, 0, 1, 2 };
        const m3 s(se);
        const la::symmetric_eigen<T, 3> e = la::eigen_symmetric(s);
        const T  r2 = T(1.4142135623730950488L);
        bool     av = true;

        for (std::size_t j = 0; j < 3; ++j)
        {
            const la::vector<T, 3> lhs = s.times(e.eigenvector(j));
            const la::vector<T, 3> rhs = e.eigenvector(j).scaled(
                                             e.eigenvalue(j));

            // (to the Jacobi threshold's order: 1e-12 of the norm by default)
            for (std::size_t i = 0; i < 3; ++i)
            {
                av = av && d_tests_near(lhs[i], rhs[i], T(1e-10) + _tol);
            }
        }

        D_TESTS_EXPECT( (d_tests_near(e.eigenvalue(0), 2 - r2, _tol * 16)) &&
                        (d_tests_near(e.eigenvalue(1), T(2), _tol * 16)) &&
                        (d_tests_near(e.eigenvalue(2), 2 + r2, _tol * 16)) &&
                        (av),
                        "the symmetric eigendecomposition: L, A V = V L");

        const la::matrix<T, 3, 3> v = e.eigenvectors();
        const la::matrix<T, 3, 3> vtv = v.transposed().times(v);

        D_TESTS_EXPECT( (d_tests_near(vtv(0, 0), T(1), _tol * 16)) &&
                        (d_tests_near(vtv(0, 1), T(0), _tol * 16)) &&
                        (d_tests_near(vtv(1, 2), T(0), _tol * 16)),
                        "the eigenvectors are orthonormal");

        // the dominant eigenpair: 2 + sqrt 2, along (1, sqrt 2, 1) / 2
        const la::eigenpair<T, 3> p = la::power_iteration(s, 2000,
                                                          _tol * 16);

        D_TESTS_EXPECT( (d_tests_near(p.value(), 2 + r2, _tol * 1024)) &&
                        (d_tests_near(p.eigenvector()[1], r2 / 2,
                                      T(1e-3))),
                        "power iteration");

        // the shifted QR on a 2-by-2 and on an upper-triangular 3-by-3
        typedef la::matrix<T, 2, 2> m2;

        const T  te[4] = { 4, 1, 2, 3 };
        const la::eigenvalue_set<T, 2> q2 = la::eigenvalues_qr(m2(te));
        const T  ue[9] = { 1, 5, 7, 0, 3, 4, 0, 0, 2 };
        const la::eigenvalue_set<T, 3> q3 = la::eigenvalues_qr(m3(ue));

        D_TESTS_EXPECT( (q2.converged()) &&
                        (d_tests_near(q2.values()[0], T(2), _tol * 64)) &&
                        (d_tests_near(q2.values()[1], T(5), _tol * 64)) &&
                        (q3.converged()) &&
                        (q3.values()[0] == T(1)) &&
                        (q3.values()[2] == T(3)),
                        "the shifted QR algorithm");

        const T eps = std::numeric_limits<T>::epsilon();

        // a non-symmetric 3-by-3 with eigenvalues 1, 2, 3: L T L^-1, L
        // [[1 0 0] [1 1 0] [0 1 1]], T [[1 1 0] [0 2 1] [0 0 3]]
        const T  ne[9] = { 0, 1, 0, -1, 2, 1, 2, -2, 4 };
        const m3 n(ne);
        const la::eigenvalue_set<T, 3> qn = la::eigenvalues_qr(n);
        const T  nb = T(64) * eps * n.norm();

        D_TESTS_EXPECT( (qn.converged()) &&
                        (d_tests_near(qn.values()[0], T(1), nb)) &&
                        (d_tests_near(qn.values()[1], T(2), nb)) &&
                        (d_tests_near(qn.values()[2], T(3), nb)),
                        "a non-symmetric 3-by-3's eigenvalues: 1, 2, 3");

        // 2^20 A: 2^20 times A's eigenvalues, exactly
        const T  big = T(1048576);
        const la::eigenvalue_set<T, 3> qs = la::eigenvalues_qr(n.scaled(big));

        D_TESTS_EXPECT( (qs.converged()) &&
                        (qs.values()[0] == qn.values()[0] * big) &&
                        (qs.values()[1] == qn.values()[1] * big) &&
                        (qs.values()[2] == qn.values()[2] * big),
                        "2^20 A's eigenvalues: 2^20 times A's, exactly");

        // random symmetric 5-by-5s: the Jacobi solver's eigenvalues
        typedef la::matrix<T, 5, 5> m5;

        bool sym_ok = true;

        for (int trial = 0; trial < 50; ++trial)
        {
            m5 r;

            for (std::size_t i = 0; i < 5; ++i)
            {
                for (std::size_t j = i; j < 5; ++j)
                {
                    r(i, j) = static_cast<T>(d_tests_random());
                    r(j, i) = r(i, j);
                }
            }

            const la::eigenvalue_set<T, 5> qr5 = la::eigenvalues_qr(r);
            const la::vector<T, 5> jac = la::eigen_symmetric(r).eigenvalues();
            const T rb = T(64) * eps * r.norm();

            sym_ok = sym_ok && qr5.converged();

            for (std::size_t i = 0; i < 5; ++i)
            {
                sym_ok = sym_ok && d_tests_near(qr5.values()[i], jac[i], rb);
            }
        }

        D_TESTS_EXPECT(sym_ok,
                       "random symmetric 5-by-5s: the Jacobi solver's");

        // a cyclic shift: 1 and the complex pair -1/2 +- i sqrt(3)/2
        const T  ce[9] = { 0, 1, 0, 0, 0, 1, 1, 0, 0 };
        const la::eigenvalue_set<T, 3> qc = la::eigenvalues_qr(m3(ce));

        D_TESTS_EXPECT( (!qc.converged()) &&
                        (d_tests_near(qc.values()[0], T(-0.5), 8 * eps)) &&
                        (d_tests_near(qc.values()[1], T(-0.5), 8 * eps)) &&
                        (d_tests_near(qc.values()[2], T(1), 8 * eps)),
                        "a complex pair: its real part twice, flagged");

        // a NaN: every eigenvalue NaN, flagged
        T        be[9] = { 3, 0, 0, 0, 1, 0, 0, 0, 2 };

        be[5] = std::numeric_limits<T>::quiet_NaN();

        const la::eigenvalue_set<T, 3> qb = la::eigenvalues_qr(m3(be));

        D_TESTS_EXPECT( (!qb.converged()) &&
                        (qb.values()[0] != qb.values()[0]) &&
                        (qb.values()[2] != qb.values()[2]),
                        "a NaN: no eigenvalues, flagged");

        // the general solver, every family (S308): [[4 1] [2 3]], the
        // non-symmetric 3-by-3, the cyclic shift's complex pair, a rotation
        // by a right angle's, and random 5-by-5s -- each eigenvalue's real
        // part eigenvalues_qr's, A x = l x, x of unit length
        typedef djinterp::math::complex<T> C;

        const m2 t2(te);

        D_TESTS_EXPECT( (d_tests_general(t2, T(32) * eps * t2.norm())) &&
                        (la::eigen_general(t2).is_real()) &&
                        (d_tests_general(n, T(32) * eps * n.norm())) &&
                        (la::eigen_general(n).is_real()),
                        "the general solver's real eigenpairs: A x = l x");

        // the cyclic shift: -1/2 - i sqrt(3)/2, its conjugate, then 1
        const m3                      cs(ce);
        const la::general_eigen<T, 3> gc = la::eigen_general(cs);
        const T                       r3 = T(0.86602540378443864676L);

        D_TESTS_EXPECT( (d_tests_general(cs, T(32) * eps * cs.norm())) &&
                        (!gc.is_real()) &&
                        (d_tests_near(gc.eigenvalue(0).real(), T(-0.5),
                                      8 * eps)) &&
                        (d_tests_near(gc.eigenvalue(0).imag(), -r3,
                                      8 * eps)) &&
                        (gc.eigenvalue(1) == conj(gc.eigenvalue(0))) &&
                        (d_tests_near(gc.eigenvalue(2).real(), T(1),
                                      8 * eps)) &&
                        (gc.eigenvalue(2).imag() == T(0)),
                        "a complex pair: -1/2 -+ i sqrt(3)/2, conjugate, "
                        "A x = l x");

        // [[0 -1] [1 0]]: -i, then i, their vectors (1, +-i) / sqrt 2
        const T                       qe[4] = { 0, -1, 1, 0 };
        const la::general_eigen<T, 2> gq    = la::eigen_general(m2(qe));
        const T                       h     = T(0.70710678118654752440L);
        const C                       x0    = gq.eigenvector(0)[0];
        const C                       x1    = gq.eigenvector(0)[1];

        D_TESTS_EXPECT( (d_tests_general(m2(qe), T(32) * eps * T(2))) &&
                        (gq.eigenvalue(0) == C(T(0), T(-1))) &&
                        (gq.eigenvalue(1) == C(T(0), T(1))) &&
                        (d_tests_near(abs(x0), h, T(4) * eps)) &&
                        (d_tests_near(abs(x1), h, T(4) * eps)) &&
                        (d_tests_near(abs(x1 - (C(T(0), T(1)) * x0)), T(0),
                                      T(4) * eps)),
                        "a rotation by a right angle: -+i, (1, +-i)/sqrt 2");

        // random 5-by-5s, most with complex pairs
        bool rand_ok = true;
        int  pairs   = 0;

        for (int trial = 0; trial < 50; ++trial)
        {
            m5 r;

            for (std::size_t k = 0; k < 25; ++k)
            {
                r(k / 5, k % 5) = static_cast<T>(d_tests_random());
            }

            rand_ok = rand_ok &&
                      d_tests_general(r, T(64) * eps * r.norm());
            pairs  += la::eigen_general(r).is_real() ? 0 : 1;
        }

        D_TESTS_EXPECT( (rand_ok) &&
                        (pairs > 25),
                        "random 5-by-5s: A x = l x, pairs conjugate");

        // a NaN: not every eigenvalue found, every eigenvalue and every
        // vector's every entry a NaN, as the driver gives them
        const la::general_eigen<T, 3> gb      = la::eigen_general(m3(be));
        bool                          nan_ok  = !gb.converged();

        for (std::size_t k = 0; k < 9; ++k)
        {
            const C x = gb.eigenvectors()(k / 3, k % 3);
            const C l = gb.eigenvalue(k % 3);

            nan_ok = ( (nan_ok) &&
                       (!(x.real() == x.real())) &&
                       (!(x.imag() == x.imag())) &&
                       (!(l.real() == l.real())) );
        }

        D_TESTS_EXPECT(nan_ok, "a NaN: not converged, its values and vectors "
                               "NaNs");

        // diag(1, 2, 3): e1, e2, e3, in float too
        const T  de[9] = { 1, 0, 0, 0, 2, 0, 0, 0, 3 };
        const la::general_eigen<T, 3> gd = la::eigen_general(m3(de));
        bool                          diag_ok = gd.converged();

        for (std::size_t k = 0; k < 3; ++k)
        {
            for (std::size_t i = 0; i < 3; ++i)
            {
                const T want = (i == k) ? T(1) : T(0);

                diag_ok = diag_ok &&
                          d_tests_near(gd.eigenvector(k)[i].real(), want,
                                       eps) &&
                          (gd.eigenvector(k)[i].imag() == T(0));
            }
        }

        D_TESTS_EXPECT(diag_ok, "diag(1, 2, 3)'s eigenvectors: e1, e2, e3");

        // [[2 1] [1 2]], for 1: +-(1, -1) / sqrt 2, orthogonal to the
        // all-ones vector the old iteration started from; the entry of the
        // larger magnitude, as rounding leaves them, positive
        const T  se2[4] = { 2, 1, 1, 2 };
        const la::general_eigen<T, 2> g2 = la::eigen_general(m2(se2));
        const T  v0 = g2.eigenvector(0)[0].real();
        const T  v1 = g2.eigenvector(0)[1].real();
        const T  a0 = (v0 < T(0)) ? -v0 : v0;
        const T  a1 = (v1 < T(0)) ? -v1 : v1;

        D_TESTS_EXPECT( (g2.converged()) &&
                        (g2.is_real()) &&
                        (d_tests_near(g2.eigenvalue(0).real(), T(1),
                                      T(8) * eps)) &&
                        (g2.eigenvector(0)[0].imag() == T(0)) &&
                        (g2.eigenvector(0)[1].imag() == T(0)) &&
                        (d_tests_near(a0, h, T(4) * eps)) &&
                        (d_tests_near(a1, h, T(4) * eps)) &&
                        (d_tests_near(v0, -v1, T(4) * eps)) &&
                        ( ((a1 > a0) && (v1 > T(0))) ||
                          ((!(a1 > a0)) && (v0 > T(0))) ),
                        "[[2 1] [1 2]]'s eigenvector for 1: +-(1, -1)/sqrt 2, "
                        "its larger entry positive");

        return;
    }
}  // namespace

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
namespace
{
    // the non-symmetric 3-by-3's eigenvalues, 1, 2, 3, as a constant
    // expression: the _c forms, from C++14
    constexpr la::vector<double, 3>
    d_tests_constexpr_eigenvalues()
    {
        const double e[9] = { 0, 1, 0, -1, 2, 1, 2, -2, 4 };

        return la::eigenvalues_qr(la::matrix<double, 3, 3>(e)).values();
    }

    constexpr la::vector<double, 3> g_constexpr_eigenvalues =
        d_tests_constexpr_eigenvalues();

    static_assert( (g_constexpr_eigenvalues[0] > 1.0 - 1e-12) &&
                   (g_constexpr_eigenvalues[0] < 1.0 + 1e-12) &&
                   (g_constexpr_eigenvalues[2] > 3.0 - 1e-12) &&
                   (g_constexpr_eigenvalues[2] < 3.0 + 1e-12),
                   "the general eigenvalues as a constant expression, from "
                   "C++14");

    // [[2 1] [1 2]]'s eigenvector for 1, +-(1, -1) / sqrt 2, likewise: its
    // first entry's magnitude
    constexpr double
    d_tests_constexpr_eigenvector()
    {
        const double e[4] = { 2, 1, 1, 2 };
        const double x    = la::eigen_general(la::matrix<double, 2, 2>(e))
                                .eigenvector(0)[0].real();

        return (x < 0.0) ? -x : x;
    }

    static_assert( (d_tests_constexpr_eigenvector() > 0.7071067811865) &&
                   (d_tests_constexpr_eigenvector() < 0.7071067811866),
                   "a general eigenvector as a constant expression, from "
                   "C++14");

    // the constant's eigenvalues are the run time's, bit for bit
    void
    d_tests_constant_evaluation()
    {
        const double e[9] = { 0, 1, 0, -1, 2, 1, 2, -2, 4 };
        const la::vector<double, 3> run =
            la::eigenvalues_qr(la::matrix<double, 3, 3>(e)).values();

        D_TESTS_EXPECT( (run[0] == g_constexpr_eigenvalues[0]) &&
                        (run[1] == g_constexpr_eigenvalues[1]) &&
                        (run[2] == g_constexpr_eigenvalues[2]),
                        "the constant's eigenvalues are the run time's");

        return;
    }
}  // namespace
#endif


int
main()
{
    d_tests_family<float>(1e-6f);
    d_tests_family<double>(1e-14);
    d_tests_family<long double>(1e-14L);

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    d_tests_constant_evaluation();
#endif

    std::printf("t_eigen_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
