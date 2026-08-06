/******************************************************************************
* djinterp [math]                                                      svd.hpp
*
* Singular value decomposition and pseudoinverse for the linear-algebra
* subframework.
*   The (thin) SVD of an M x N matrix (M >= N) by the one-sided Jacobi
* (Hestenes) method: A = U S V^T with U an M x N matrix of orthonormal columns,
* S the N non-negative singular values (descending), and V an N x N orthogonal
* matrix. Built on construction as a factor-once object, like the other
* decompositions, with a free factory and the Moore-Penrose pseudoinverse layered
* on top.
*
* PROVIDED TYPES / FUNCTIONS:
*   svd_decomposition<_T,_M,_N>      - thin SVD, A = U S V^T   (M >= N)
*     svd(A [, tol])                   u() singular_values() v() sigma()
*                                      rank([rcond]) condition_number()
*   pseudoinverse(A [, rcond]) / pinv  - Moore-Penrose inverse (any M, N)
*
* DESIGN NOTES:
*   - One-sided Jacobi orthogonalizes the columns of A by a sequence of plane
*     rotations; the column norms of the converged matrix are the singular
*     values, the normalized columns form U, and the accumulated rotations form
*     V. It is numerically accurate and needs only the constexpr scalar kernel.
*   - svd_decomposition requires M >= N (the thin U has orthonormal columns).
*     pseudoinverse accepts any shape: for M < N it dispatches through the
*     transpose, since pinv(A) == pinv(A^T)^T.
*   - Requires a floating-point element type.
*   - pseudoinverse(A) == V diag(s+) U^T with s+_i = 1/s_i when s_i exceeds the
*     cutoff rcond * s_max, else 0. For a square nonsingular A it equals the
*     inverse; for an overdetermined full-rank A, pinv(A) * b equals the
*     least-squares solution (see solve.hpp).
*   - The getters return core value types, so they chain with the fluent
*     members, e.g. svd(A).v().transposed().
*
* path:      /inc/djinterp/math/linear_algebra/svd.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.23
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_SVD_
#define DJINTERP_MATH_LINALG_SVD_ 1

// std
#include <cstddef>
#include <limits>
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
// I.   SVD DECOMPOSITION  (one-sided Jacobi, A = U S V^T)
// ===========================================================================

// svd_decomposition
//   class: the thin singular value decomposition of an M x N matrix (M >= N).
template<typename    _T,
         std::size_t _M,
         std::size_t _N>
class svd_decomposition
{
    static_assert(std::is_floating_point<_T>::value,
                  "svd_decomposition: requires a floating-point element type.");
    static_assert((_M >= _N),
                  "svd_decomposition: requires rows >= cols (M >= N); for a "
                  "wide matrix, decompose its transpose.");
    static_assert((_N > 0),
                  "svd_decomposition: dimensions must be at least 1.");

public:
    using value_type   = _T;
    using size_type    = std::size_t;
    using u_type       = matrix<_T, _M, _N>;
    using v_type       = matrix<_T, _N, _N>;
    using sigma_type   = matrix<_T, _N, _N>;
    using values_type  = vector<_T, _N>;

    // decompose on construction.
    D_CONSTEXPR explicit
    svd_decomposition(
        const matrix<_T, _M, _N>& _a,
        _T                        _tol = default_tolerance<_T>()
    ) noexcept
        : m_u{}
        , m_s{}
        , m_v(v_type::identity())
    {
        // columns of work are rotated until mutually orthogonal; they become
        // U scaled by the singular values.
        matrix<_T, _M, _N> work = _a;

        const size_type max_sweeps = 60;

        for (size_type sweep = 0; sweep < max_sweeps; ++sweep)
        {
            bool rotated = false;

            for (size_type i = 0; i < _N; ++i)
            {
                for (size_type j = i + 1; j < _N; ++j)
                {
                    // 2x2 sub-Gram of columns i and j.
                    _T alpha = static_cast<_T>(0);
                    _T beta  = static_cast<_T>(0);
                    _T gamma = static_cast<_T>(0);

                    for (size_type k = 0; k < _M; ++k)
                    {
                        alpha = alpha + work(k, i) * work(k, i);
                        beta  = beta + work(k, j) * work(k, j);
                        gamma = gamma + work(k, i) * work(k, j);
                    }

                    // skip when the columns are already orthogonal enough.
                    const _T product = static_cast<_T>(
                        internal::sqrt_c(static_cast<double>(alpha * beta)));

                    if (internal::abs_c(gamma) > (_tol * product))
                    {
                        rotated = true;

                        // rotation that orthogonalizes the two columns.
                        const _T zeta =
                            (beta - alpha) / (static_cast<_T>(2) * gamma);

                        _T t = static_cast<_T>(0);

                        if (zeta == static_cast<_T>(0))
                        {
                            t = static_cast<_T>(1);
                        }
                        else
                        {
                            const _T sgn =
                                (zeta > static_cast<_T>(0)) ? static_cast<_T>(1)
                                                            : static_cast<_T>(-1);

                            t = sgn / ( internal::abs_c(zeta) +
                                static_cast<_T>(internal::sqrt_c(
                                    static_cast<double>(static_cast<_T>(1) +
                                                        zeta * zeta))) );
                        }

                        const _T c = static_cast<_T>(1) /
                            static_cast<_T>(internal::sqrt_c(
                                static_cast<double>(static_cast<_T>(1) +
                                                    t * t)));
                        const _T s = c * t;

                        // rotate columns i, j of work.
                        for (size_type k = 0; k < _M; ++k)
                        {
                            const _T wki = work(k, i);
                            const _T wkj = work(k, j);

                            work(k, i) = c * wki - s * wkj;
                            work(k, j) = s * wki + c * wkj;
                        }

                        // apply the same rotation to V.
                        for (size_type k = 0; k < _N; ++k)
                        {
                            const _T vki = m_v(k, i);
                            const _T vkj = m_v(k, j);

                            m_v(k, i) = c * vki - s * vkj;
                            m_v(k, j) = s * vki + c * vkj;
                        }
                    }
                }
            }

            if (!rotated)
            {
                break;
            }
        }

        // singular values are the column norms; U is the normalized columns.
        for (size_type j = 0; j < _N; ++j)
        {
            _T norm_sq = static_cast<_T>(0);

            for (size_type k = 0; k < _M; ++k)
            {
                norm_sq = norm_sq + work(k, j) * work(k, j);
            }

            const _T sigma =
                static_cast<_T>(internal::sqrt_c(static_cast<double>(norm_sq)));

            m_s[j] = sigma;

            if (sigma > static_cast<_T>(0))
            {
                for (size_type k = 0; k < _M; ++k)
                {
                    m_u(k, j) = work(k, j) / sigma;
                }
            }
            else
            {
                // degenerate column: leave a zero column of U (its singular
                // value is zero, so it does not contribute).
                for (size_type k = 0; k < _M; ++k)
                {
                    m_u(k, j) = static_cast<_T>(0);
                }
            }
        }

        // sort the singular values descending, carrying U and V columns along.
        for (size_type i = 0; i < _N; ++i)
        {
            size_type sel = i;

            for (size_type k = i + 1; k < _N; ++k)
            {
                if (m_s[k] > m_s[sel])
                {
                    sel = k;
                }
            }

            if (sel != i)
            {
                constexpr_swap(m_s[i], m_s[sel]);

                for (size_type r = 0; r < _M; ++r)
                {
                    constexpr_swap(m_u(r, i), m_u(r, sel));
                }

                for (size_type r = 0; r < _N; ++r)
                {
                    constexpr_swap(m_v(r, i), m_v(r, sel));
                }
            }
        }
    }

    // u: the M x N matrix of left singular vectors (orthonormal columns).
    D_CONSTEXPR u_type
    u() const noexcept
    {
        return m_u;
    }

    // singular_values: the N singular values in descending order.
    D_CONSTEXPR values_type
    singular_values() const noexcept
    {
        return m_s;
    }

    // v: the N x N orthogonal matrix of right singular vectors.
    D_CONSTEXPR v_type
    v() const noexcept
    {
        return m_v;
    }

    // sigma: the N x N diagonal matrix of singular values.
    D_CONSTEXPR sigma_type
    sigma() const noexcept
    {
        return sigma_type::diagonal(m_s);
    }

    // rank: the number of singular values above rcond * largest singular value.
    D_CONSTEXPR size_type
    rank(_T _rcond = default_tolerance<_T>()) const noexcept
    {
        const _T cutoff = _rcond * m_s[0];

        size_type r = 0;

        for (size_type i = 0; i < _N; ++i)
        {
            if (m_s[i] > cutoff)
            {
                ++r;
            }
        }

        return r;
    }

    // condition_number: ratio of the largest to the smallest singular value
    // (infinity when the smallest is zero).
    D_CONSTEXPR _T
    condition_number() const noexcept
    {
        return (m_s[_N - 1] > static_cast<_T>(0))
                   ? (m_s[0] / m_s[_N - 1])
                   : std::numeric_limits<_T>::infinity();
    }

private:
    u_type      m_u;
    values_type m_s;
    v_type      m_v;
};

// svd
//   factory: the thin singular value decomposition of _a (M >= N).
template<typename    _T,
         std::size_t _M,
         std::size_t _N>
D_CONSTEXPR svd_decomposition<_T, _M, _N>
svd(
    const matrix<_T, _M, _N>& _a,
    _T                        _tol = default_tolerance<_T>()
) noexcept
{
    return svd_decomposition<_T, _M, _N>(_a, _tol);
}


// ===========================================================================
// II.  PSEUDOINVERSE  (Moore-Penrose)
// ===========================================================================

// pseudoinverse  (tall or square: M >= N, directly via the SVD)
//   A+ == V diag(s+) U^T, with reciprocals of the singular values above the
// cutoff and zeros elsewhere.
template<typename    _T,
         std::size_t _M,
         std::size_t _N,
         typename std::enable_if<(_M >= _N), int>::type = 0>
D_CONSTEXPR matrix<_T, _N, _M>
pseudoinverse(
    const matrix<_T, _M, _N>& _a,
    _T                        _rcond = default_tolerance<_T>()
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "pseudoinverse: requires a floating-point element type.");

    const svd_decomposition<_T, _M, _N> d = svd(_a);

    const matrix<_T, _M, _N> uu = d.u();
    const matrix<_T, _N, _N> vv = d.v();
    const vector<_T, _N>     ss = d.singular_values();

    const _T cutoff = _rcond * ss[0];

    // scaled = V diag(s+) : column j of V multiplied by the reciprocal sigma.
    matrix<_T, _N, _N> scaled;

    for (std::size_t i = 0; i < _N; ++i)
    {
        for (std::size_t j = 0; j < _N; ++j)
        {
            const _T inv =
                (ss[j] > cutoff) ? (static_cast<_T>(1) / ss[j])
                                 : static_cast<_T>(0);

            scaled(i, j) = vv(i, j) * inv;
        }
    }

    // A+ = (V diag(s+)) U^T.
    return multiply(scaled, uu.transposed());
}

// pseudoinverse  (wide: M < N, via the transpose -- pinv(A) == pinv(A^T)^T)
template<typename    _T,
         std::size_t _M,
         std::size_t _N,
         typename std::enable_if<(_M < _N), int>::type = 0>
D_CONSTEXPR matrix<_T, _N, _M>
pseudoinverse(
    const matrix<_T, _M, _N>& _a,
    _T                        _rcond = default_tolerance<_T>()
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "pseudoinverse: requires a floating-point element type.");

    return pseudoinverse(_a.transposed(), _rcond).transposed();
}

// pinv
//   short alias for pseudoinverse.
template<typename    _T,
         std::size_t _M,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N, _M>
pinv(
    const matrix<_T, _M, _N>& _a,
    _T                        _rcond = default_tolerance<_T>()
) noexcept
{
    return pseudoinverse(_a, _rcond);
}

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINALG_SVD_
