/******************************************************************************
* djinterp [math]                                                    solve.hpp
*
* Linear-system solvers for the linear-algebra subframework.
*   High-level free function templates that turn the factorizations in
* decomposition.hpp into the operations one usually reaches for: solving square
* systems, the triangular substitution primitives, solving symmetric
* positive-definite systems, and least-squares for overdetermined systems.
*
* PROVIDED FUNCTIONS:
*   forward_substitution(L, b)      - solve L x = b, L lower triangular
*   back_substitution(U, b)         - solve U x = b, U upper triangular
*   solve(A, b)                     - solve a square system A x = b   (LU)
*   solve(A, B)                     - solve A X = B for a matrix RHS   (LU)
*   solve_spd(A, b)                 - solve an SPD system A x = b   (Cholesky)
*   least_squares(A, b)             - minimize ||A x - b|| for M x N, M >= N (QR)
*
* DESIGN NOTES:
*   - solve performs partial-pivot Gaussian elimination by way of the LU
*     factorization; for a matrix right-hand side it factors A once and
*     substitutes for every column.
*   - forward_substitution / back_substitution read only the relevant triangle
*     and divide by the diagonal (general, not unit-diagonal, triangular).
*   - solve_spd uses Cholesky and assumes A is symmetric positive-definite; for
*     a general system use solve, and test cholesky(A).is_spd() if unsure.
*   - least_squares uses Q R: with A = Q R it solves R1 x = (Q^T b)[0:N], the
*     normal-equation solution for a full-column-rank A, without forming A^T A.
*   - All routines require a floating-point element type. They are free
*     functions (like square.hpp) and their results chain with the core fluent
*     members; the factor-once objects in decomposition.hpp remain available
*     when the factorization itself is wanted, e.g. lu(A).inverse().
*
* path:      /inc/djinterp/math/linear_algebra/solve.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_SOLVE_
#define DJINTERP_MATH_LINALG_SOLVE_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "./linalg_common.hpp"
#include "./vector.hpp"
#include "./matrix.hpp"
#include "./decomposition.hpp"


NS_DJINTERP
NS_MATH

namespace linalg
{

// ===========================================================================
// I.   TRIANGULAR SUBSTITUTION
// ===========================================================================

// forward_substitution
//   solve L x = b where L is lower triangular (only the lower triangle and the
// diagonal are read).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
forward_substitution(
    const matrix<_T, _N, _N>& _l,
    const vector<_T, _N>&     _b
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "forward_substitution: requires a floating-point element "
                  "type.");

    vector<_T, _N> x;

    for (std::size_t i = 0; i < _N; ++i)
    {
        _T s = _b[i];

        // subtract the already-known entries in this row.
        for (std::size_t k = 0; k < i; ++k)
        {
            s = s - _l(i, k) * x[k];
        }

        x[i] = s / _l(i, i);
    }

    return x;
}

// back_substitution
//   solve U x = b where U is upper triangular (only the upper triangle and the
// diagonal are read).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
back_substitution(
    const matrix<_T, _N, _N>& _u,
    const vector<_T, _N>&     _b
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "back_substitution: requires a floating-point element "
                  "type.");

    vector<_T, _N> x;

    for (std::size_t ii = _N; ii-- > 0; )
    {
        const std::size_t i = ii;

        _T s = _b[i];

        // subtract the already-known entries in this row.
        for (std::size_t k = i + 1; k < _N; ++k)
        {
            s = s - _u(i, k) * x[k];
        }

        x[i] = s / _u(i, i);
    }

    return x;
}


// ===========================================================================
// II.  SQUARE SYSTEMS  (partial-pivot LU)
// ===========================================================================

// solve
//   solve the square system A x = b by partial-pivot LU.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
solve(
    const matrix<_T, _N, _N>& _a,
    const vector<_T, _N>&     _b
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "solve: requires a floating-point element type.");

    return lu(_a).solve(_b);
}

// solve
//   solve A X = B for a matrix right-hand side, factoring A once.
template<typename    _T,
         std::size_t _N,
         std::size_t _K>
D_CONSTEXPR matrix<_T, _N, _K>
solve(
    const matrix<_T, _N, _N>& _a,
    const matrix<_T, _N, _K>& _b
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "solve: requires a floating-point element type.");

    const lu_decomposition<_T, _N> f = lu(_a);

    matrix<_T, _N, _K> x;

    // solve column by column, reusing the single factorization.
    for (std::size_t j = 0; j < _K; ++j)
    {
        const vector<_T, _N> col = f.solve(_b.col(j));

        for (std::size_t i = 0; i < _N; ++i)
        {
            x(i, j) = col[i];
        }
    }

    return x;
}


// ===========================================================================
// III. SYMMETRIC POSITIVE-DEFINITE SYSTEMS  (Cholesky)
// ===========================================================================

// solve_spd
//   solve A x = b for a symmetric positive-definite A by Cholesky.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
solve_spd(
    const matrix<_T, _N, _N>& _a,
    const vector<_T, _N>&     _b
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "solve_spd: requires a floating-point element type.");

    return cholesky(_a).solve(_b);
}


// ===========================================================================
// IV.  LEAST SQUARES  (Householder QR)
// ===========================================================================

// least_squares
//   minimize ||A x - b|| for an overdetermined system (A is M x N, M >= N, of
// full column rank) using the QR factorization. Returns the N-vector x.
template<typename    _T,
         std::size_t _M,
         std::size_t _N>
D_CONSTEXPR vector<_T, _N>
least_squares(
    const matrix<_T, _M, _N>& _a,
    const vector<_T, _M>&     _b
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "least_squares: requires a floating-point element type.");
    static_assert((_M >= _N),
                  "least_squares: requires rows >= cols (M >= N).");

    const qr_decomposition<_T, _M, _N> g = qr(_a);

    const matrix<_T, _M, _M> q = g.q();
    const matrix<_T, _M, _N> r = g.r();

    // c = (Q^T b) restricted to its first N entries: c[i] = sum_k Q(k,i) b[k].
    vector<_T, _N> c;

    for (std::size_t i = 0; i < _N; ++i)
    {
        _T s = static_cast<_T>(0);

        for (std::size_t k = 0; k < _M; ++k)
        {
            s = s + q(k, i) * _b[k];
        }

        c[i] = s;
    }

    // R1 = the top-left N x N block of R (upper triangular).
    matrix<_T, _N, _N> r1;

    for (std::size_t i = 0; i < _N; ++i)
    {
        for (std::size_t j = i; j < _N; ++j)
        {
            r1(i, j) = r(i, j);
        }
    }

    // solve R1 x = c.
    return back_substitution(r1, c);
}

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINALG_SOLVE_
