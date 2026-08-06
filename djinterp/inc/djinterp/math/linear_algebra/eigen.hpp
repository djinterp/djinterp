/******************************************************************************
* djinterp [math]                                                    eigen.hpp
*
* Eigenvalue / eigenvector routines for the linear-algebra subframework.
*   Three complementary approaches, each as a free factory returning a small
* value-type result:
*   - the cyclic Jacobi method for symmetric matrices (all eigenvalues plus an
*     orthonormal set of eigenvectors) -- the robust, fully featured path;
*   - power iteration for the single dominant eigenpair;
*   - the shifted QR algorithm with deflation for the real eigenvalues of a
*     general matrix.
*
* PROVIDED TYPES / FUNCTIONS:
*   symmetric_eigen<_T,_N>            - A = V L V^T for symmetric A (Jacobi)
*     eigen_symmetric(A [, tol])        eigenvalues() eigenvectors()
*                                        eigenvalue(i) eigenvector(i)
*   eigenpair<_T,_N>                  - a (value, vector) pair
*     power_iteration(A [, iters, tol]) value() eigenvector()
*   eigenvalue_set<_T,_N>            - eigenvalues plus a convergence flag
*     eigenvalues_qr(A [, iters, tol])  values() converged()
*   general_eigen<_T,_N>             - real eigenvalues + eigenvectors (general A)
*     eigen_general(A [, iters, tol])   eigenvalues() eigenvectors()
*                                        eigenvalue(i) eigenvector(i) converged()
*
* DESIGN NOTES:
*   - All routines require a floating-point element type.
*   - eigen_symmetric ASSUMES a symmetric matrix. Eigenvalues are returned in
*     ascending order with the eigenvector columns permuted to match. The
*     eigenvectors are orthonormal: eigenvectors() is the orthogonal V with
*     A == V * diag(eigenvalues()) * V^T and A * eigenvector(i) ==
*     eigenvalue(i) * eigenvector(i).
*   - power_iteration finds the eigenvalue of largest magnitude and its
*     eigenvector; it assumes a unique dominant eigenvalue and a starting vector
*     not orthogonal to the corresponding eigenvector (it starts from the all-
*     ones direction).
*   - eigenvalues_qr returns REAL eigenvalues (ascending) via the Wilkinson-
*     shifted QR algorithm with deflation. converged() reports whether every
*     subdiagonal deflated within the iteration budget; a matrix with complex-
*     conjugate eigenvalues will not converge and its result is unreliable.
*     This routine returns eigenvalues only -- use eigen_symmetric (symmetric)
*     or power_iteration (dominant) when eigenvectors are needed.
*   - The result objects' getters return core value types, so they chain with
*     the fluent members, e.g. eigen_symmetric(A).eigenvector(0).normalized().
*
* path:      /inc/djinterp/math/linear_algebra/eigen.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_EIGEN_
#define DJINTERP_MATH_LINALG_EIGEN_ 1

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
// I.   SYMMETRIC EIGENSOLVER  (cyclic Jacobi)
// ===========================================================================

// symmetric_eigen
//   class: the eigendecomposition of a symmetric matrix by the cyclic Jacobi
// method. Holds the (ascending) eigenvalues and the orthonormal eigenvectors
// as the columns of V, with A == V diag(values) V^T.
template<typename    _T,
         std::size_t _N>
class symmetric_eigen
{
    static_assert(std::is_floating_point<_T>::value,
                  "symmetric_eigen: requires a floating-point element type.");
    static_assert((_N > 0),
                  "symmetric_eigen: dimension must be at least 1.");

public:
    using value_type  = _T;
    using size_type   = std::size_t;
    using vector_type = vector<_T, _N>;
    using matrix_type = matrix<_T, _N, _N>;

    // diagonalize on construction.
    D_CONSTEXPR explicit
    symmetric_eigen(
        const matrix_type& _a,
        _T                 _tol = default_tolerance<_T>()
    ) noexcept
        : m_values{}
        , m_vectors(matrix_type::identity())
    {
        matrix_type work = _a;

        // convergence threshold relative to the matrix scale.
        const _T scale     = _a.norm();
        const _T threshold =
            (scale > static_cast<_T>(0)) ? (_tol * scale) : static_cast<_T>(0);

        const size_type max_sweeps = 100;

        for (size_type sweep = 0; sweep < max_sweeps; ++sweep)
        {
            // Frobenius magnitude of the strict upper triangle.
            _T off_sq = static_cast<_T>(0);

            for (size_type p = 0; p < _N; ++p)
            {
                for (size_type q = p + 1; q < _N; ++q)
                {
                    off_sq = off_sq + work(p, q) * work(p, q);
                }
            }

            // stop once the off-diagonal part is negligible.
            if (static_cast<_T>(internal::sqrt_c(static_cast<double>(off_sq)))
                <= threshold)
            {
                break;
            }

            // one sweep over every (p, q) pair above the diagonal.
            for (size_type p = 0; p < _N; ++p)
            {
                for (size_type q = p + 1; q < _N; ++q)
                {
                    const _T apq = work(p, q);

                    if (apq != static_cast<_T>(0))
                    {
                        // rotation that annihilates the (p, q) entry.
                        const _T theta =
                            (work(q, q) - work(p, p)) /
                            (static_cast<_T>(2) * apq);

                        _T t = static_cast<_T>(0);

                        if (theta == static_cast<_T>(0))
                        {
                            t = static_cast<_T>(1);
                        }
                        else
                        {
                            const _T sgn =
                                (theta > static_cast<_T>(0)) ? static_cast<_T>(1)
                                                             : static_cast<_T>(-1);

                            t = sgn / ( internal::abs_c(theta) +
                                static_cast<_T>(internal::sqrt_c(
                                    static_cast<double>(theta * theta +
                                                        static_cast<_T>(1)))) );
                        }

                        const _T c = static_cast<_T>(1) /
                            static_cast<_T>(internal::sqrt_c(
                                static_cast<double>(t * t + static_cast<_T>(1))));
                        const _T s = t * c;

                        // work <- J^T work J : rotate columns p, q ...
                        for (size_type i = 0; i < _N; ++i)
                        {
                            const _T wip = work(i, p);
                            const _T wiq = work(i, q);

                            work(i, p) = c * wip - s * wiq;
                            work(i, q) = s * wip + c * wiq;
                        }

                        // ... then rotate rows p, q.
                        for (size_type j = 0; j < _N; ++j)
                        {
                            const _T wpj = work(p, j);
                            const _T wqj = work(q, j);

                            work(p, j) = c * wpj - s * wqj;
                            work(q, j) = s * wpj + c * wqj;
                        }

                        // accumulate the rotation into the eigenvectors.
                        for (size_type i = 0; i < _N; ++i)
                        {
                            const _T vip = m_vectors(i, p);
                            const _T viq = m_vectors(i, q);

                            m_vectors(i, p) = c * vip - s * viq;
                            m_vectors(i, q) = s * vip + c * viq;
                        }
                    }
                }
            }
        }

        // eigenvalues are the diagonal of the converged matrix.
        for (size_type i = 0; i < _N; ++i)
        {
            m_values[i] = work(i, i);
        }

        // sort ascending, carrying the eigenvector columns along.
        for (size_type i = 0; i < _N; ++i)
        {
            size_type sel = i;

            for (size_type k = i + 1; k < _N; ++k)
            {
                if (m_values[k] < m_values[sel])
                {
                    sel = k;
                }
            }

            if (sel != i)
            {
                constexpr_swap(m_values[i], m_values[sel]);

                for (size_type r = 0; r < _N; ++r)
                {
                    constexpr_swap(m_vectors(r, i), m_vectors(r, sel));
                }
            }
        }
    }

    // eigenvalues: the eigenvalues in ascending order.
    D_CONSTEXPR vector_type
    eigenvalues() const noexcept
    {
        return m_values;
    }

    // eigenvectors: the orthonormal eigenvectors as columns.
    D_CONSTEXPR matrix_type
    eigenvectors() const noexcept
    {
        return m_vectors;
    }

    D_CONSTEXPR _T
    eigenvalue(size_type _i) const noexcept
    {
        return m_values[_i];
    }

    // eigenvector: the i-th eigenvector (column i of V).
    D_CONSTEXPR vector_type
    eigenvector(size_type _i) const noexcept
    {
        return m_vectors.col(_i);
    }

private:
    vector_type m_values;
    matrix_type m_vectors;
};

// eigen_symmetric
//   factory: the Jacobi eigendecomposition of a symmetric matrix.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR symmetric_eigen<_T, _N>
eigen_symmetric(
    const matrix<_T, _N, _N>& _a,
    _T                        _tol = default_tolerance<_T>()
) noexcept
{
    return symmetric_eigen<_T, _N>(_a, _tol);
}


// ===========================================================================
// II.  POWER ITERATION  (dominant eigenpair)
// ===========================================================================

// eigenpair
//   class: an eigenvalue together with its eigenvector.
template<typename    _T,
         std::size_t _N>
class eigenpair
{
public:
    using value_type  = _T;
    using vector_type = vector<_T, _N>;

    D_CONSTEXPR
    eigenpair(
        _T                 _value,
        const vector_type& _vector
    ) noexcept
        : m_value(_value)
        , m_vector(_vector)
    {
    }

    D_CONSTEXPR _T
    value() const noexcept
    {
        return m_value;
    }

    D_CONSTEXPR vector_type
    eigenvector() const noexcept
    {
        return m_vector;
    }

private:
    _T          m_value;
    vector_type m_vector;
};

// power_iteration
//   the dominant eigenpair (largest-magnitude eigenvalue and its eigenvector)
// via the power method, with the eigenvalue recovered as a Rayleigh quotient.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR eigenpair<_T, _N>
power_iteration(
    const matrix<_T, _N, _N>& _a,
    std::size_t               _max_iter = 1000,
    _T                        _tol      = default_tolerance<_T>()
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "power_iteration: requires a floating-point element type.");

    // start from the normalized all-ones direction.
    vector<_T, _N> v = vector<_T, _N>::filled(static_cast<_T>(1)).normalized();

    for (std::size_t iter = 0; iter < _max_iter; ++iter)
    {
        const vector<_T, _N> w  = _a.times(v);
        const _T             nw = w.norm();

        // a zero image means we cannot continue iterating.
        if (nw == static_cast<_T>(0))
        {
            break;
        }

        vector<_T, _N> vn = w.divided(nw);

        // keep the direction stable across iterations (negative eigenvalues).
        if (vn.dot(v) < static_cast<_T>(0))
        {
            vn = vn.negated();
        }

        const _T diff = vn.minus(v).norm();

        v = vn;

        if (diff <= _tol)
        {
            break;
        }
    }

    // Rayleigh quotient: lambda = (v . A v) / (v . v).
    const vector<_T, _N> av    = _a.times(v);
    const _T             denom = v.dot(v);
    const _T             lambda =
        (denom != static_cast<_T>(0)) ? (v.dot(av) / denom)
                                      : static_cast<_T>(0);

    return eigenpair<_T, _N>(lambda, v);
}


// ===========================================================================
// III. QR ALGORITHM  (real eigenvalues, Wilkinson shift + deflation)
// ===========================================================================

// eigenvalue_set
//   class: a set of eigenvalues together with a convergence flag.
template<typename    _T,
         std::size_t _N>
class eigenvalue_set
{
public:
    using value_type  = _T;
    using vector_type = vector<_T, _N>;

    D_CONSTEXPR
    eigenvalue_set(
        const vector_type& _values,
        bool               _converged
    ) noexcept
        : m_values(_values)
        , m_converged(_converged)
    {
    }

    D_CONSTEXPR vector_type
    values() const noexcept
    {
        return m_values;
    }

    D_CONSTEXPR bool
    converged() const noexcept
    {
        return m_converged;
    }

private:
    vector_type m_values;
    bool        m_converged;
};

// eigenvalues_qr
//   the real eigenvalues (ascending) of a general matrix via the Wilkinson-
// shifted QR algorithm with deflation. converged() is false when a subdiagonal
// failed to deflate within the budget (e.g. complex-conjugate eigenvalues).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR eigenvalue_set<_T, _N>
eigenvalues_qr(
    const matrix<_T, _N, _N>& _a,
    std::size_t               _max_iter = 1000,
    _T                        _tol      = default_tolerance<_T>()
) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "eigenvalues_qr: requires a floating-point element type.");

    matrix<_T, _N, _N>       h  = _a;
    const matrix<_T, _N, _N> id = matrix<_T, _N, _N>::identity();

    std::size_t active = _N;   // size of the leading, not-yet-deflated block
    std::size_t iter   = 0;

    while ((active > 1) && (iter < _max_iter))
    {
        ++iter;

        const std::size_t n = active;

        // deflate the bottom active subdiagonal when it is negligible.
        const _T local_scale =
            internal::abs_c(h(n - 1, n - 1)) + internal::abs_c(h(n - 2, n - 2));

        if (internal::abs_c(h(n - 1, n - 2))
            <= _tol * (local_scale + static_cast<_T>(1)))
        {
            h(n - 1, n - 2) = static_cast<_T>(0);
            active          = active - 1;
            continue;
        }

        // Wilkinson shift from the trailing 2x2 of the active block.
        const _T a     = h(n - 2, n - 2);
        const _T b     = h(n - 2, n - 1);
        const _T c     = h(n - 1, n - 2);
        const _T d     = h(n - 1, n - 1);
        const _T delta = (a - d) / static_cast<_T>(2);
        const _T bc    = b * c;
        const _T root  = static_cast<_T>(
            internal::sqrt_c(static_cast<double>(delta * delta + bc)));
        const _T denom = internal::abs_c(delta) + root;

        _T mu = static_cast<_T>(0);

        if (denom == static_cast<_T>(0))
        {
            mu = d;
        }
        else
        {
            const _T sgn =
                (delta >= static_cast<_T>(0)) ? static_cast<_T>(1)
                                              : static_cast<_T>(-1);

            mu = d - sgn * bc / denom;
        }

        // one shifted QR step on the full matrix: H - mu I = Q R, H = R Q + mu I.
        const matrix<_T, _N, _N>       shifted = h.minus(id.scaled(mu));
        const qr_decomposition<_T, _N, _N> g    = qr(shifted);

        h = multiply(g.r(), g.q()).plus(id.scaled(mu));
    }

    const bool converged = (active <= 1);

    // eigenvalues are the diagonal entries.
    vector<_T, _N> vals;

    for (std::size_t i = 0; i < _N; ++i)
    {
        vals[i] = h(i, i);
    }

    // sort ascending.
    for (std::size_t i = 0; i < _N; ++i)
    {
        std::size_t sel = i;

        for (std::size_t k = i + 1; k < _N; ++k)
        {
            if (vals[k] < vals[sel])
            {
                sel = k;
            }
        }

        if (sel != i)
        {
            constexpr_swap(vals[i], vals[sel]);
        }
    }

    return eigenvalue_set<_T, _N>(vals, converged);
}


// ===========================================================================
// IV.  GENERAL EIGENVECTORS  (real spectrum, inverse iteration)
// ===========================================================================

// general_eigen
//   class: the real eigenvalues and eigenvectors of a general (not necessarily
// symmetric) matrix. The eigenvalues come from the shifted QR algorithm; each
// eigenvector is recovered by inverse iteration with a slightly perturbed shift
// (so A - shift I stays invertible). Eigenvalues are ascending and the
// eigenvector columns match.
template<typename    _T,
         std::size_t _N>
class general_eigen
{
    static_assert(std::is_floating_point<_T>::value,
                  "general_eigen: requires a floating-point element type.");
    static_assert((_N > 0),
                  "general_eigen: dimension must be at least 1.");

public:
    using value_type  = _T;
    using size_type   = std::size_t;
    using vector_type = vector<_T, _N>;
    using matrix_type = matrix<_T, _N, _N>;

    D_CONSTEXPR explicit
    general_eigen(
        const matrix_type& _a,
        std::size_t        _max_iter = 1000,
        _T                 _tol      = default_tolerance<_T>()
    ) noexcept
        : m_values{}
        , m_vectors(matrix_type::identity())
        , m_converged(false)
    {
        // eigenvalues (and the convergence verdict) from the QR algorithm.
        const eigenvalue_set<_T, _N> es = eigenvalues_qr(_a, _max_iter, _tol);

        m_values    = es.values();
        m_converged = es.converged();

        // a small shift perturbation keeps A - shift I invertible while staying
        // close enough that inverse iteration locks onto the eigenvector.
        const _T scale = _a.norm();
        const _T delta =
            ((scale > static_cast<_T>(0)) ? scale : static_cast<_T>(1)) *
            static_cast<_T>(1e-9);

        for (size_type idx = 0; idx < _N; ++idx)
        {
            const _T shift = m_values[idx] + delta;

            // M = A - shift I.
            const matrix_type m =
                _a.minus(matrix_type::identity().scaled(shift));

            const lu_decomposition<_T, _N> f = lu(m);

            // inverse iteration: x <- normalize( (A - shift I)^{-1} x ).
            vector_type x =
                vector_type::filled(static_cast<_T>(1)).normalized();

            for (size_type it = 0; it < 6; ++it)
            {
                const vector_type y  = f.solve(x);
                const _T          ny = y.norm();

                if (ny == static_cast<_T>(0))
                {
                    break;
                }

                x = y.divided(ny);
            }

            // store the eigenvector as column idx.
            for (size_type r = 0; r < _N; ++r)
            {
                m_vectors(r, idx) = x[r];
            }
        }
    }

    // eigenvalues: the real eigenvalues in ascending order.
    D_CONSTEXPR vector_type
    eigenvalues() const noexcept
    {
        return m_values;
    }

    // eigenvectors: the (normalized) eigenvectors as columns.
    D_CONSTEXPR matrix_type
    eigenvectors() const noexcept
    {
        return m_vectors;
    }

    D_CONSTEXPR _T
    eigenvalue(size_type _i) const noexcept
    {
        return m_values[_i];
    }

    // eigenvector: the i-th eigenvector (column i).
    D_CONSTEXPR vector_type
    eigenvector(size_type _i) const noexcept
    {
        return m_vectors.col(_i);
    }

    // converged: whether the eigenvalue iteration converged (false implies a
    // complex spectrum, in which case the results are unreliable).
    D_CONSTEXPR bool
    converged() const noexcept
    {
        return m_converged;
    }

private:
    vector_type m_values;
    matrix_type m_vectors;
    bool        m_converged;
};

// eigen_general
//   factory: the real eigenvalues and eigenvectors of a general matrix.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR general_eigen<_T, _N>
eigen_general(
    const matrix<_T, _N, _N>& _a,
    std::size_t               _max_iter = 1000,
    _T                        _tol      = default_tolerance<_T>()
) noexcept
{
    return general_eigen<_T, _N>(_a, _max_iter, _tol);
}

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINALG_EIGEN_
