/*******************************************************************************
* djinterp [cpp/math]                                                  solve.hpp
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
*   solve(A, b)                     - solve a square system A x = b
*   solve(A, B)                     - solve A X = B for a matrix RHS
*   solve_spd(A, b)                 - solve an SPD system A x = b   (Cholesky)
*   least_squares(A, b)             - minimize ||A x - b||, M x N, M >= N (QR)
*
* DESIGN NOTES:
*   - solve is Gaussian elimination of [A | B] with partial pivoting, then
*     back-substitution (S257), the kernels': for float, double and long
*     double the C core's d_mat_solve, each column LU's bits; for any other
*     number type but an integer type, its own arithmetic -- exact for an
*     exact type, products kept in order where they do not commute. A
*     singular A gives the zero solution, as inverse gives the zero matrix.
*   - forward_substitution / back_substitution read only the relevant triangle
*     and divide by the diagonal (general, not unit-diagonal, triangular).
*   - solve_spd uses Cholesky and assumes A is symmetric positive-definite; for
*     a general system use solve, and test cholesky(A).is_spd() if unsure.
*   - least_squares uses Q R: with A = Q R it solves R1 x = (Q^T b)[0:N], the
*     normal-equation solution for a full-column-rank A, without forming A^T A.
*   - solve takes any number type but an integer type; the others require a
*     floating-point element type. They are free functions (like
*     square.hpp) and their results chain with the core fluent members; the
*     factor-once objects in decomposition.hpp remain available when the
*     factorization itself is wanted, e.g. lu(A).inverse().
*
* A FACE OVER THE C CORE:
*   The substitutions and least squares' Q^T b are c/math/decomp.h's, the
* loops this header wrote, in their order; solve is c/math/mat.h's
* d_mat_solve through the kernels, and solve_spd goes through
* decomposition.hpp's Cholesky as before. Everything compiles from
* C++98, constexpr from C++14 as it was (least squares' QR from C++14
* where decomposition.hpp's is).
*
*
*
* path:      /inc/djinterp/cpp/modules/math/linear_algebra/solve.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.22
*                                                            revised: 2026.10.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRIANGULAR SUBSTITUTION
    -----------------------
2.  SQUARE SYSTEMS
    --------------
3.  LEAST SQUARES
    -------------
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_SOLVE_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_SOLVE_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../../../../c/modules/math/decomp.h"    // the C core: d_decomp_*
#include "./decomposition.hpp"                    // lu, qr, cholesky
#include "./linalg_common.hpp"                    // linalg_family_of
#include "./matrix.hpp"                           // matrix
#include "./vector.hpp"                           // vector


NS_DJINTERP
NS_MATH

namespace linalg
{

NS_INTERNAL

    // solve_requires_float
    //   struct: the solvers' requirement -- float, double or long double --
    // asserted at class scope, once for every solver that names it.
    template<typename T>
    struct solve_requires_float
    {
        D_STATIC_ASSERT((linalg_family_of<T>::value != 0),
                        "solve: requires a floating-point element type "
                        "(float, double or long double).");

        enum
        {
            ok = 1
        };
    };

    template<std::size_t M,
             std::size_t N>
    struct solve_requires_tall
    {
        D_STATIC_ASSERT((M >= N),
                        "least_squares: requires rows >= cols (M >= N).");

        enum
        {
            ok = 1
        };
    };

NS_END  // internal


//==============================================================================
// 1.  TRIANGULAR SUBSTITUTION
//==============================================================================


// forward_substitution
//   function: x with L x = _b, L the lower triangle of _l (a general
// diagonal).
template<typename    T,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<T, N>
forward_substitution(
    const matrix<T, N, N>& _l,
    const vector<T, N>&    _b
) D_NOEXCEPT
{
    (void)internal::solve_requires_float<T>::ok;

    vector<T, N> x;

    d_decomp_forward_sub(_l.data(), N, _b.data(), x.begin());

    return x;
}

// back_substitution
//   function: x with U x = _b, U the upper triangle of _u (a general
// diagonal).
template<typename    T,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<T, N>
back_substitution(
    const matrix<T, N, N>& _u,
    const vector<T, N>&    _b
) D_NOEXCEPT
{
    (void)internal::solve_requires_float<T>::ok;

    vector<T, N> x;

    d_decomp_back_sub(_u.data(), N, _b.data(), x.begin());

    return x;
}


//==============================================================================
// 2.  SQUARE SYSTEMS
//==============================================================================


// solve
//   function: x with _a x = _b, by Gaussian elimination of [_a | _b] with
// partial pivoting and back-substitution (S257): for float, double and long
// double the C core's d_mat_solve, LU's bits; for any other number type but
// an integer type its own arithmetic, exact where it is exact, products in
// their order where they do not commute. The zero vector for a singular _a,
// as inverse gives the zero matrix.
template<typename    T,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<T, N>
solve(
    const matrix<T, N, N>& _a,
    const vector<T, N>&    _b
) D_NOEXCEPT
{
    D_STATIC_ASSERT((!number_traits<T>::is_integer),
                    "solve: an integer element type has no integer "
                    "solution in general.");

    vector<T, N> x;
    T            scratch[N * N] = { T() };

    (void)internal::linalg_kernel<T>::solve(x.begin(),
                                            scratch,
                                            _a.data(),
                                            _b.data(),
                                            N,
                                            1);

    return x;
}

// solve
//   function: X with _a X = _b, by one elimination of [_a | _b] and a
// back-substitution per column, as solve(_a, _b) of a vector; each column
// the bits that solve of that column gives.
template<typename    T,
         std::size_t N,
         std::size_t K>
D_CONSTEXPR_CPP14 matrix<T, N, K>
solve(
    const matrix<T, N, N>& _a,
    const matrix<T, N, K>& _b
) D_NOEXCEPT
{
    D_STATIC_ASSERT((!number_traits<T>::is_integer),
                    "solve: an integer element type has no integer "
                    "solution in general.");

    matrix<T, N, K> x;
    T               scratch[N * N] = { T() };

    (void)internal::linalg_kernel<T>::solve(&x(0, 0),
                                            scratch,
                                            _a.data(),
                                            _b.data(),
                                            N,
                                            K);

    return x;
}

// solve_spd
//   function: x with _a x = _b for a symmetric positive-definite _a, by
// Cholesky.
template<typename    T,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<T, N>
solve_spd(
    const matrix<T, N, N>& _a,
    const vector<T, N>&    _b
) D_NOEXCEPT
{
    (void)internal::solve_requires_float<T>::ok;

    return cholesky(_a).solve(_b);
}


//==============================================================================
// 3.  LEAST SQUARES
//==============================================================================


// least_squares
//   function: x minimizing ||_a x - _b|| for an M-by-N _a, M >= N, of full
// column rank: with _a = Q R, R1 x = (Q^T _b)[0:N], R1 R's upper N-by-N.
template<typename    T,
         std::size_t M,
         std::size_t N>
D_CONSTEXPR_CPP14 vector<T, N>
least_squares(
    const matrix<T, M, N>& _a,
    const vector<T, M>&    _b
) D_NOEXCEPT
{
    (void)internal::solve_requires_float<T>::ok;
    (void)internal::solve_requires_tall<M, N>::ok;

    const qr_decomposition<T, M, N> g = qr(_a);
    const matrix<T, M, M>           q = g.q();
    const matrix<T, M, N>           r = g.r();
    vector<T, N>                    c;
    vector<T, N>                    x;

    d_decomp_qt_mul(q.data(), M, N, _b.data(), c.begin());

    // R's first N rows are R1, stored as an N-by-N matrix
    d_decomp_back_sub(r.data(), N, c.data(), x.begin());

    return x;
}

}  // namespace linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_SOLVE_HPP
