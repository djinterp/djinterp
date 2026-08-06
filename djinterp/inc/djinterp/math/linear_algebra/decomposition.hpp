/******************************************************************************
* djinterp [math]                                            decomposition.hpp
*
* Matrix factorizations for the linear-algebra subframework.
*   Three classic decompositions, each as a small value-type object that
* performs the factorization on construction and then answers queries -- the
* factor-once, use-many pattern. A free factory function (lu / qr / cholesky)
* produces each object, so the procedural and fluent spellings coincide:
*   procedural   auto f = lu(A);  auto x = f.solve(b);
*   fluent       auto x = lu(A).solve(b);
*
* PROVIDED TYPES / FUNCTIONS:
*   lu_decomposition<_T,_N>          - partial-pivot LU, P A = L U
*     lu(A)                            l() u() p() pivot_sign() is_singular()
*                                      determinant() solve(b) inverse()
*   qr_decomposition<_T,_M,_N>       - Householder QR, A = Q R   (M >= N)
*     qr(A)                            q() r()
*   cholesky_decomposition<_T,_N>    - Cholesky, A = L L^T  (symmetric pos-def)
*     cholesky(A)                      l() is_spd()/success() determinant()
*                                      solve(b)
*
* DESIGN NOTES:
*   - All three require a floating-point element type (these are division- and
*     square-root-based algorithms). For exact integer determinants use the
*     fraction-free determinant in square.hpp instead.
*   - LU uses partial pivoting for stability; a zero pivot column sets
*     is_singular() and makes determinant() return 0 (solve/inverse are then
*     not meaningful -- guard with is_singular()).
*   - QR uses Householder reflectors (numerically stable, full Q). R is upper
*     triangular (upper-trapezoidal when M > N). Q is M x M and orthogonal.
*   - Cholesky assumes a symmetric matrix and reads only the lower triangle. It
*     does not throw: a non-positive pivot clears is_spd() and stops.
*   - The objects' methods chain with the core fluent members, e.g.
*       qr(A).q().transposed()        lu(A).inverse().times(b)
*
* path:      /inc/djinterp/math/linear_algebra/decomposition.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_DECOMPOSITION_
#define DJINTERP_MATH_LINALG_DECOMPOSITION_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "./linalg_common.hpp"
#include "./vector.hpp"
#include "./matrix.hpp"


NS_DJINTERP
NS_MATH

namespace linalg
{

// ===========================================================================
// I.   LU DECOMPOSITION  (partial pivoting, P A = L U)
// ===========================================================================

// lu_decomposition
//   class: partial-pivot LU factorization of a square matrix. The combined
// factors are held in one matrix (unit-lower L below the diagonal, U on and
// above it) together with the row permutation and its sign.
template<typename    _T,
         std::size_t _N>
class lu_decomposition
{
    static_assert(std::is_floating_point<_T>::value,
                  "lu_decomposition: requires a floating-point element type.");
    static_assert((_N > 0),
                  "lu_decomposition: dimension must be at least 1.");

public:
    using value_type  = _T;
    using size_type   = std::size_t;
    using matrix_type = matrix<_T, _N, _N>;
    using vector_type = vector<_T, _N>;

    // factorize on construction.
    D_CONSTEXPR explicit
    lu_decomposition(
        const matrix_type& _a
    ) noexcept
        : m_lu(_a)
        , m_perm{}
        , m_sign(static_cast<_T>(1))
        , m_singular(false)
    {
        // start from the identity permutation.
        for (size_type i = 0; i < _N; ++i)
        {
            m_perm[i] = i;
        }

        for (size_type col = 0; col < _N; ++col)
        {
            // partial pivot: largest magnitude on/below the diagonal.
            size_type pivot = col;
            _T        maxv  = internal::abs_c(m_lu(col, col));

            for (size_type r = col + 1; r < _N; ++r)
            {
                const _T v = internal::abs_c(m_lu(r, col));

                if (v > maxv)
                {
                    maxv  = v;
                    pivot = r;
                }
            }

            // a zero pivot column -> singular; leave it and move on.
            if (m_lu(pivot, col) == static_cast<_T>(0))
            {
                m_singular = true;
                continue;
            }

            // bring the pivot row into place (rows of L\U and the permutation).
            if (pivot != col)
            {
                for (size_type c = 0; c < _N; ++c)
                {
                    constexpr_swap(m_lu(col, c), m_lu(pivot, c));
                }

                constexpr_swap(m_perm[col], m_perm[pivot]);
                m_sign = -m_sign;
            }

            // eliminate below, storing the multipliers in the lower triangle.
            const _T pivot_value = m_lu(col, col);

            for (size_type r = col + 1; r < _N; ++r)
            {
                const _T f = m_lu(r, col) / pivot_value;

                m_lu(r, col) = f;

                for (size_type c = col + 1; c < _N; ++c)
                {
                    m_lu(r, c) = m_lu(r, c) - f * m_lu(col, c);
                }
            }
        }
    }

    // ---- factors ----------------------------------------------------------

    // l: the unit-lower-triangular factor (1 on the diagonal).
    D_CONSTEXPR matrix_type
    l() const noexcept
    {
        matrix_type out;

        for (size_type i = 0; i < _N; ++i)
        {
            for (size_type j = 0; j < _N; ++j)
            {
                if (i > j)
                {
                    out(i, j) = m_lu(i, j);
                }
                else if (i == j)
                {
                    out(i, j) = static_cast<_T>(1);
                }
            }
        }

        return out;
    }

    // u: the upper-triangular factor.
    D_CONSTEXPR matrix_type
    u() const noexcept
    {
        matrix_type out;

        for (size_type i = 0; i < _N; ++i)
        {
            for (size_type j = i; j < _N; ++j)
            {
                out(i, j) = m_lu(i, j);
            }
        }

        return out;
    }

    // p: the permutation matrix, with P A == L U.
    D_CONSTEXPR matrix_type
    p() const noexcept
    {
        matrix_type out;

        for (size_type i = 0; i < _N; ++i)
        {
            out(i, m_perm[i]) = static_cast<_T>(1);
        }

        return out;
    }

    // ---- queries ----------------------------------------------------------

    D_CONSTEXPR _T
    pivot_sign() const noexcept
    {
        return m_sign;
    }

    D_CONSTEXPR bool
    is_singular() const noexcept
    {
        return m_singular;
    }

    // determinant: sign of the permutation times the product of U's diagonal.
    D_CONSTEXPR _T
    determinant() const noexcept
    {
        _T d = m_sign;

        for (size_type i = 0; i < _N; ++i)
        {
            d = d * m_lu(i, i);
        }

        return d;
    }

    // ---- solving ----------------------------------------------------------

    // solve: solve A x = b using the factorization (P b, then L y = Pb, then
    // U x = y).
    D_CONSTEXPR vector_type
    solve(
        const vector_type& _b
    ) const noexcept
    {
        // apply the row permutation to b.
        vector_type pb;

        for (size_type i = 0; i < _N; ++i)
        {
            pb[i] = _b[m_perm[i]];
        }

        // forward substitution: L y = pb  (L is unit lower triangular).
        vector_type y;

        for (size_type i = 0; i < _N; ++i)
        {
            _T s = pb[i];

            for (size_type k = 0; k < i; ++k)
            {
                s = s - m_lu(i, k) * y[k];
            }

            y[i] = s;
        }

        // back substitution: U x = y.
        vector_type x;

        for (size_type ii = _N; ii-- > 0; )
        {
            const size_type i = ii;

            _T s = y[i];

            for (size_type k = i + 1; k < _N; ++k)
            {
                s = s - m_lu(i, k) * x[k];
            }

            x[i] = s / m_lu(i, i);
        }

        return x;
    }

    // inverse: solve against each column of the identity.
    D_CONSTEXPR matrix_type
    inverse() const noexcept
    {
        matrix_type out;

        for (size_type j = 0; j < _N; ++j)
        {
            vector_type e;
            e[j] = static_cast<_T>(1);

            const vector_type col = solve(e);

            for (size_type i = 0; i < _N; ++i)
            {
                out(i, j) = col[i];
            }
        }

        return out;
    }

private:
    matrix_type m_lu;
    size_type   m_perm[_N];
    _T          m_sign;
    bool        m_singular;
};

// lu
//   factory: the partial-pivot LU factorization of _a.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR lu_decomposition<_T, _N>
lu(const matrix<_T, _N, _N>& _a) noexcept
{
    return lu_decomposition<_T, _N>(_a);
}


// ===========================================================================
// II.  QR DECOMPOSITION  (Householder reflectors, A = Q R)
// ===========================================================================

// qr_decomposition
//   class: Householder QR factorization of an M x N matrix (M >= N). Q is the
// M x M orthogonal product of the reflectors; R is M x N upper triangular.
template<typename    _T,
         std::size_t _M,
         std::size_t _N>
class qr_decomposition
{
    static_assert(std::is_floating_point<_T>::value,
                  "qr_decomposition: requires a floating-point element type.");
    static_assert((_M >= _N),
                  "qr_decomposition: requires rows >= cols (M >= N).");
    static_assert((_N > 0),
                  "qr_decomposition: dimensions must be at least 1.");

public:
    using value_type = _T;
    using size_type  = std::size_t;
    using q_type     = matrix<_T, _M, _M>;
    using r_type     = matrix<_T, _M, _N>;

    // factorize on construction.
    D_CONSTEXPR explicit
    qr_decomposition(
        const matrix<_T, _M, _N>& _a
    ) noexcept
        : m_q(matrix<_T, _M, _M>::identity())
        , m_r(_a)
    {
        // one Householder reflector per column (N <= M of them).
        for (size_type k = 0; k < _N; ++k)
        {
            // v holds the reflector, length M, zero in rows above k.
            _T v[_M]{};

            for (size_type i = k; i < _M; ++i)
            {
                v[i] = m_r(i, k);
            }

            // norm of the subcolumn x = R[k:, k].
            _T norm_sq = static_cast<_T>(0);

            for (size_type i = k; i < _M; ++i)
            {
                norm_sq = norm_sq + v[i] * v[i];
            }

            const _T normx =
                static_cast<_T>(internal::sqrt_c(static_cast<double>(norm_sq)));

            // already zero on/below the diagonal: no reflector needed.
            if (normx == static_cast<_T>(0))
            {
                continue;
            }

            // alpha = -sign(x[k]) * ||x||  (sign chosen to avoid cancellation).
            const _T alpha =
                (v[k] >= static_cast<_T>(0)) ? -normx : normx;

            v[k] = v[k] - alpha;

            // squared norm of the reflector.
            _T vtv = static_cast<_T>(0);

            for (size_type i = k; i < _M; ++i)
            {
                vtv = vtv + v[i] * v[i];
            }

            if (vtv == static_cast<_T>(0))
            {
                continue;
            }

            const _T two_over = static_cast<_T>(2) / vtv;

            // apply the reflector to R:  R -= (2/vtv) v (v^T R).
            for (size_type j = 0; j < _N; ++j)
            {
                _T s = static_cast<_T>(0);

                for (size_type i = k; i < _M; ++i)
                {
                    s = s + v[i] * m_r(i, j);
                }

                const _T f = two_over * s;

                for (size_type i = k; i < _M; ++i)
                {
                    m_r(i, j) = m_r(i, j) - f * v[i];
                }
            }

            // accumulate into Q:  Q -= (2/vtv) (Q v) v^T.
            for (size_type r = 0; r < _M; ++r)
            {
                _T s = static_cast<_T>(0);

                for (size_type i = k; i < _M; ++i)
                {
                    s = s + m_q(r, i) * v[i];
                }

                const _T f = two_over * s;

                for (size_type i = k; i < _M; ++i)
                {
                    m_q(r, i) = m_q(r, i) - f * v[i];
                }
            }
        }
    }

    // q: the orthogonal factor (M x M).
    D_CONSTEXPR q_type
    q() const noexcept
    {
        return m_q;
    }

    // r: the upper-triangular factor (M x N).
    D_CONSTEXPR r_type
    r() const noexcept
    {
        return m_r;
    }

private:
    q_type m_q;
    r_type m_r;
};

// qr
//   factory: the Householder QR factorization of _a.
template<typename    _T,
         std::size_t _M,
         std::size_t _N>
D_CONSTEXPR qr_decomposition<_T, _M, _N>
qr(const matrix<_T, _M, _N>& _a) noexcept
{
    return qr_decomposition<_T, _M, _N>(_a);
}


// ===========================================================================
// III. CHOLESKY DECOMPOSITION  (A = L L^T, symmetric positive-definite)
// ===========================================================================

// cholesky_decomposition
//   class: Cholesky factorization of a symmetric positive-definite matrix.
// Reads only the lower triangle of the input. Does not throw -- a non-positive
// pivot clears is_spd() and stops the factorization.
template<typename    _T,
         std::size_t _N>
class cholesky_decomposition
{
    static_assert(std::is_floating_point<_T>::value,
                  "cholesky_decomposition: requires a floating-point element "
                  "type.");
    static_assert((_N > 0),
                  "cholesky_decomposition: dimension must be at least 1.");

public:
    using value_type  = _T;
    using size_type   = std::size_t;
    using matrix_type = matrix<_T, _N, _N>;
    using vector_type = vector<_T, _N>;

    // factorize on construction.
    D_CONSTEXPR explicit
    cholesky_decomposition(
        const matrix_type& _a
    ) noexcept
        : m_l{}
        , m_spd(true)
    {
        for (size_type i = 0; i < _N; ++i)
        {
            for (size_type j = 0; j <= i; ++j)
            {
                _T sum = _a(i, j);

                for (size_type k = 0; k < j; ++k)
                {
                    sum = sum - m_l(i, k) * m_l(j, k);
                }

                if (i == j)
                {
                    // a non-positive pivot means the matrix is not SPD.
                    if (sum <= static_cast<_T>(0))
                    {
                        m_spd = false;
                        return;
                    }

                    m_l(i, j) = static_cast<_T>(
                        internal::sqrt_c(static_cast<double>(sum)));
                }
                else
                {
                    m_l(i, j) = sum / m_l(j, j);
                }
            }
        }
    }

    // l: the lower-triangular factor.
    D_CONSTEXPR matrix_type
    l() const noexcept
    {
        return m_l;
    }

    D_CONSTEXPR bool
    is_spd() const noexcept
    {
        return m_spd;
    }

    D_CONSTEXPR bool
    success() const noexcept
    {
        return m_spd;
    }

    // determinant: the square of the product of L's diagonal.
    D_CONSTEXPR _T
    determinant() const noexcept
    {
        _T d = static_cast<_T>(1);

        for (size_type i = 0; i < _N; ++i)
        {
            d = d * m_l(i, i);
        }

        return d * d;
    }

    // solve: solve A x = b via L y = b (forward) then L^T x = y (back).
    D_CONSTEXPR vector_type
    solve(
        const vector_type& _b
    ) const noexcept
    {
        // forward substitution: L y = b.
        vector_type y;

        for (size_type i = 0; i < _N; ++i)
        {
            _T s = _b[i];

            for (size_type k = 0; k < i; ++k)
            {
                s = s - m_l(i, k) * y[k];
            }

            y[i] = s / m_l(i, i);
        }

        // back substitution: L^T x = y  (L^T(i,k) == L(k,i)).
        vector_type x;

        for (size_type ii = _N; ii-- > 0; )
        {
            const size_type i = ii;

            _T s = y[i];

            for (size_type k = i + 1; k < _N; ++k)
            {
                s = s - m_l(k, i) * x[k];
            }

            x[i] = s / m_l(i, i);
        }

        return x;
    }

private:
    matrix_type m_l;
    bool        m_spd;
};

// cholesky
//   factory: the Cholesky factorization of _a (symmetric positive-definite).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR cholesky_decomposition<_T, _N>
cholesky(const matrix<_T, _N, _N>& _a) noexcept
{
    return cholesky_decomposition<_T, _N>(_a);
}

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINALG_DECOMPOSITION_
