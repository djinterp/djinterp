/*******************************************************************************
* djinterp [cpp/math]                                                  eigen.hpp
*
* Eigenvalue / eigenvector routines for the linear-algebra subframework.
*   Three complementary approaches, each as a free factory returning a small
* value-type result:
*   - the cyclic Jacobi method for symmetric matrices (all eigenvalues plus an
*     orthonormal set of eigenvectors) -- the robust, fully featured path;
*   - power iteration for the single dominant eigenpair;
*   - the shifted QR algorithm with deflation for the eigenvalues of a
*     general matrix, real or complex, and their eigenvectors.
*
* PROVIDED TYPES / FUNCTIONS:
*   symmetric_eigen<T, N>            - A = V L V^T for symmetric A (Jacobi)
*     eigen_symmetric(A [, tol])        eigenvalues() eigenvectors()
*                                        eigenvalue(i) eigenvector(i)
*   eigenpair<T, N>                  - a (value, vector) pair
*     power_iteration(A [, iters, tol]) value() eigenvector()
*   eigenvalue_set<T, N>             - eigenvalues plus a convergence flag
*     eigenvalues_qr(A [, iters, tol])  values() converged()
*   general_eigen<T, N>              - eigenvalues + eigenvectors (any A),
*     eigen_general(A [, iters, tol])   as math::complex<T>: eigenvalues()
*                                        eigenvectors() eigenvalue(i)
*                                        eigenvector(i) converged() is_real()
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
*   - eigenvalues_qr returns REAL eigenvalues (ascending): the matrix
*     balanced, its Hessenberg form, then Francis's double-shift QR steps,
*     LAPACK's dlahqr (decomp.h, S269), a negligible subdiagonal splitting
*     the matrix.
*     converged() is false where a complex pair was found (its real part is
*     given twice; the rest of the spectrum is still found), where the step
*     budget ran out, or where the matrix holds a NaN or an infinity (every
*     eigenvalue NaN). Eigenvalues only: general_eigen adds a vector for
*     each, eigen_symmetric an orthonormal set for a symmetric matrix.
*   - general_eigen gives every eigenvalue and an eigenvector for each as
*     math::complex<T> (S308), decomp.h's d_decomp_eigen's, as LAPACK's
*     dgeev: the eigenvalues ascending by real part, then by imaginary
*     part, a pair exactly conjugate, their real parts eigenvalues_qr's bit
*     for bit; the eigenvectors from the real Schur form by back-
*     substitution, as dtrevc: unit length, the entry of largest modulus
*     real and positive, a pair's conjugate, independent for a repeated
*     eigenvalue where the matrix has independent ones. converged() is
*     false only where not every eigenvalue was found, every vector then
*     NaNs, as the driver gives them.
*   - The result objects' getters return core value types, so they chain with
*     the fluent members, e.g. eigen_symmetric(A).eigenvector(0).normalized().
*
* A FACE OVER THE C CORE:
*   The symmetric Jacobi sweeps and their sort, and the general eigenvalues
* and eigenvectors, are c/math/decomp.h's, the complex numbers that hold
* them c/math/cplx.h's (math::complex); power iteration is the vector and
* matrix faces' operations. The square roots are
* the element type's, correctly rounded: the C library's below C++14, the _c
* form from C++14, and from C++20 whichever std::is_constant_evaluated asks
* for. The general eigenvalues and eigenvectors are found on the matrix
* scaled by a power of two, exactly, so 2^k A has 2^k times A's
* eigenvalues, bit for bit, barring overflow and underflow. Everything
* compiles from C++98; it is constexpr from C++14.
*
*
* path:      /inc/djinterp/cpp/modules/math/linear_algebra/eigen.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.22
*                                                            revised: 2026.10.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SYMMETRIC MATRICES
    ------------------
    1.  symmetric_eigen
    2.  eigen_symmetric
2.  THE DOMINANT EIGENPAIR
    ----------------------
    1.  eigenpair
    2.  power_iteration
3.  GENERAL MATRICES
    ----------------
    1.  eigenvalue_set
    2.  eigenvalues_qr
    3.  general_eigen
    4.  eigen_general
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_EIGEN_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_EIGEN_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../complex.hpp"                          // complex
#include "./decomposition.hpp"                    // decomp_roots,
                                                  // decomp_requires_float
#include "./linalg_common.hpp"                    // default_tolerance
#include "./matrix.hpp"                           // matrix
#include "./vector.hpp"                           // vector


NS_DJINTERP
NS_MATH

namespace linalg
{


//==============================================================================
// 1.  SYMMETRIC MATRICES
//==============================================================================


// 1.1    symmetric_eigen
//------------------------------------------------------------------------------
// 1.1.1
// symmetric_eigen
//   class: A = V L V^T for a symmetric A, by cyclic Jacobi: the eigenvalues
// ascending and an orthonormal set of eigenvectors.
template<typename    T,
         std::size_t N>
class symmetric_eigen
{
    D_STATIC_ASSERT((internal::decomp_requires_float<T>::ok == 1),
                    "symmetric_eigen: requires a floating-point element "
                    "type.");
    D_STATIC_ASSERT((N > 0),
                    "symmetric_eigen: dimension must be at least 1.");

public:
    typedef T               value_type;
    typedef std::size_t     size_type;
    typedef vector<T, N>    vector_type;
    typedef matrix<T, N, N> matrix_type;

    D_CONSTEXPR_CPP14 explicit
    symmetric_eigen(
        const matrix_type& _a,
        T                  _tol = default_tolerance<T>()
    ) D_NOEXCEPT
        : m_values(),
          m_vectors()
    {
        const T* entries   = _a.data();
        const T  scale     = _a.norm();
        const T  threshold = (scale > static_cast<T>(0)) ? (_tol * scale)
                                                         : static_cast<T>(0);
        T        work[N * N] = {};

        for (size_type i = 0; i < (N * N); ++i)
        {
            work[i] = entries[i];
        }

        for (size_type i = 0; i < N; ++i)
        {
            m_vectors[(i * N) + i] = static_cast<T>(1);
        }

        internal::decomp_roots<T>::sym_eigen(work, m_values, m_vectors, N,
                                             threshold);
    }

    D_CONSTEXPR_CPP14 vector_type
    eigenvalues() const D_NOEXCEPT
    {
        return vector_type(m_values);
    }

    D_CONSTEXPR_CPP14 matrix_type
    eigenvectors() const D_NOEXCEPT
    {
        return matrix_type(m_vectors);
    }

    D_CONSTEXPR T
    eigenvalue(size_type _i) const D_NOEXCEPT
    {
        return m_values[_i];
    }

    D_CONSTEXPR_CPP14 vector_type
    eigenvector(size_type _i) const D_NOEXCEPT
    {
        vector_type v;

        for (size_type r = 0; r < N; ++r)
        {
            v[r] = m_vectors[(r * N) + _i];
        }

        return v;
    }

private:
    T m_values[N];
    T m_vectors[N * N];
};

// 1.2    eigen_symmetric
//------------------------------------------------------------------------------
// 1.2.1
// eigen_symmetric
//   function: the symmetric eigendecomposition of _a.
template<typename    T,
         std::size_t N>
D_CONSTEXPR_CPP14 symmetric_eigen<T, N>
eigen_symmetric(
    const matrix<T, N, N>& _a,
    T                      _tol = default_tolerance<T>()
) D_NOEXCEPT
{
    return symmetric_eigen<T, N>(_a, _tol);
}


//==============================================================================
// 2.  THE DOMINANT EIGENPAIR
//==============================================================================


// 2.1    eigenpair
//------------------------------------------------------------------------------
// 2.1.1
// eigenpair
//   class: an eigenvalue and its eigenvector.
template<typename    T,
         std::size_t N>
class eigenpair
{
public:
    typedef T            value_type;
    typedef vector<T, N> vector_type;

    D_CONSTEXPR
    eigenpair(
        T                  _value,
        const vector_type& _vector
    ) D_NOEXCEPT
        : m_value(_value),
          m_vector(_vector)
    {}

    D_CONSTEXPR T
    value() const D_NOEXCEPT
    {
        return m_value;
    }

    D_CONSTEXPR vector_type
    eigenvector() const D_NOEXCEPT
    {
        return m_vector;
    }

private:
    T           m_value;
    vector_type m_vector;
};

// 2.2    power_iteration
//------------------------------------------------------------------------------
// 2.2.1
// power_iteration
//   function: the eigenvalue of largest magnitude and its eigenvector, from
// the all-ones direction, until a step moves the unit vector by at most
// _tol or _max_iter steps; the value as the Rayleigh quotient.
template<typename    T,
         std::size_t N>
D_CONSTEXPR_CPP14 eigenpair<T, N>
power_iteration(
    const matrix<T, N, N>& _a,
    std::size_t            _max_iter = 1000,
    T                      _tol      = default_tolerance<T>()
) D_NOEXCEPT
{
    (void)internal::decomp_requires_float<T>::ok;

    vector<T, N> v = vector<T, N>::filled(static_cast<T>(1)).normalized();

    for (std::size_t iter = 0; iter < _max_iter; ++iter)
    {
        const vector<T, N> w  = _a.times(v);
        const T            nw = w.norm();

        // the matrix annihilates the vector: no direction to follow
        if (nw == static_cast<T>(0))
        {
            break;
        }

        vector<T, N> vn = w.divided(nw);

        // keep the sign, so the iterates converge rather than alternate
        if (vn.dot(v) < static_cast<T>(0))
        {
            vn = vn.negated();
        }

        const T diff = vn.minus(v).norm();

        v = vn;

        // the direction has settled
        if (diff <= _tol)
        {
            break;
        }
    }

    const vector<T, N> av     = _a.times(v);
    const T            denom  = v.dot(v);
    const T            lambda = (denom != static_cast<T>(0))
                                    ? (v.dot(av) / denom)
                                    : static_cast<T>(0);

    return eigenpair<T, N>(lambda, v);
}


//==============================================================================
// 3.  GENERAL MATRICES
//==============================================================================


// 3.1    eigenvalue_set
//------------------------------------------------------------------------------
// 3.1.1
// eigenvalue_set
//   class: eigenvalues and whether the iteration that found them converged.
template<typename    T,
         std::size_t N>
class eigenvalue_set
{
public:
    typedef T            value_type;
    typedef vector<T, N> vector_type;

    D_CONSTEXPR
    eigenvalue_set(
        const vector_type& _values,
        bool               _converged
    ) D_NOEXCEPT
        : m_values(_values),
          m_converged(_converged)
    {}

    D_CONSTEXPR vector_type
    values() const D_NOEXCEPT
    {
        return m_values;
    }

    D_CONSTEXPR bool
    converged() const D_NOEXCEPT
    {
        return m_converged;
    }

private:
    vector_type m_values;
    bool        m_converged;
};

// 3.2    eigenvalues_qr
//------------------------------------------------------------------------------
// 3.2.1
// eigenvalues_qr
//   function: the real eigenvalues of _a, ascending, by decomp.h: balanced,
// the Hessenberg form, then Francis's double-shift QR, a subdiagonal entry
// negligible beside its neighbours by Ahues and Kressner's test, _tol its
// tolerance (_tol below the type's epsilon -- 0, the default -- is the
// epsilon), splitting it, at most _max_iter steps in all. converged() is false
// where a complex pair was found (its real part twice), where the budget
// ran out (the unfinished entries the diagonal's), or where _a holds a NaN
// or an infinity (every eigenvalue NaN).
template<typename    T,
         std::size_t N>
D_CONSTEXPR_CPP14 eigenvalue_set<T, N>
eigenvalues_qr(
    const matrix<T, N, N>& _a,
    std::size_t            _max_iter = 1000,
    T                      _tol      = static_cast<T>(0)
) D_NOEXCEPT
{
    (void)internal::decomp_requires_float<T>::ok;

    const T* entries     = _a.data();
    T        work[N * N] = {};
    T        values[N]   = {};

    for (std::size_t i = 0; i < (N * N); ++i)
    {
        work[i] = entries[i];
    }

    const bool converged = internal::decomp_roots<T>::eigenvalues(
        work, N, values, _max_iter, _tol);

    return eigenvalue_set<T, N>(vector<T, N>(values), converged);
}

// 3.3    general_eigen
//------------------------------------------------------------------------------
// 3.3.1
// general_eigen
//   class: the eigenvalues of a general real matrix and an eigenvector for
// each, as complex numbers (S308), decomp.h's d_decomp_eigen's, as LAPACK's
// dgeev finds them (S269, S49's recommendation): the eigenvalues ascending
// by real part, then by imaginary part, a complex pair exactly conjugate
// and side by side, a real eigenvalue's imaginary part +0, the real parts
// eigenvalues_qr's bit for bit; each eigenvector from the real Schur form
// by back-substitution, as dtrevc, of unit length with its entry of
// largest modulus real and positive (the first among equals), a real
// eigenvalue's real and a pair's conjugate; a repeated eigenvalue of a
// matrix with independent vectors gets independent ones. converged() is
// false where not every eigenvalue was found (a NaN or an infinity in the
// matrix, or the step budget spent): the eigenvalues then decomp.h's, those
// found and the rest the diagonal's, or every one a NaN, and every vector's
// every entry a NaN, as the driver gives them. is_real() says whether every
// eigenvalue is real.
template<typename    T,
         std::size_t N>
class general_eigen
{
    D_STATIC_ASSERT((internal::decomp_requires_float<T>::ok == 1),
                    "general_eigen: requires a floating-point element type.");
    D_STATIC_ASSERT((N > 0),
                    "general_eigen: dimension must be at least 1.");

public:
    typedef T                         value_type;
    typedef complex<T>                complex_type;
    typedef std::size_t               size_type;
    typedef vector<complex<T>, N>     vector_type;
    typedef matrix<complex<T>, N, N>  matrix_type;

    D_CONSTEXPR_CPP14 explicit
    general_eigen(
        const matrix<T, N, N>& _a,
        std::size_t            _max_iter = 1000,
        T                      _tol      = static_cast<T>(0)
    ) D_NOEXCEPT
        : m_values(),
          m_vectors(),
          m_converged(false)
    {
        typedef typename internal::decomp_complex<T>::type core_type;

        const T*    entries              = _a.data();
        T           work[N * N]          = {};
        T           scratch[N * (N + 4)] = {};
        std::size_t perm[N]              = {};
        core_type   values[N]            = {};
        core_type   vectors[N * N]       = {};

        // A, which the driver consumes
        for (size_type i = 0; i < (N * N); ++i)
        {
            work[i] = entries[i];
        }

        m_converged = internal::decomp_roots<T>::eigen(work, N, values,
                                                       vectors, scratch,
                                                       perm, _max_iter,
                                                       _tol);

        // every eigenvalue, as the driver gives it
        for (size_type idx = 0; idx < N; ++idx)
        {
            m_values[idx] = complex_type(values[idx]);
        }

        // every vector's every entry, as the driver gives it, column k % N
        // the vector of eigenvalue k % N: NaNs where not every eigenvalue
        // was found
        for (size_type k = 0; k < (N * N); ++k)
        {
            m_vectors(k / N, k % N) = complex_type(vectors[k]);
        }
    }

    D_CONSTEXPR_CPP14 vector_type
    eigenvalues() const D_NOEXCEPT
    {
        return m_values;
    }

    D_CONSTEXPR_CPP14 matrix_type
    eigenvectors() const D_NOEXCEPT
    {
        return m_vectors;
    }

    D_CONSTEXPR_CPP14 complex_type
    eigenvalue(size_type _i) const D_NOEXCEPT
    {
        return m_values[_i];
    }

    D_CONSTEXPR_CPP14 vector_type
    eigenvector(size_type _i) const D_NOEXCEPT
    {
        return m_vectors.col(_i);
    }

    D_CONSTEXPR bool
    converged() const D_NOEXCEPT
    {
        return m_converged;
    }

    // whether every eigenvalue is real: its imaginary part 0.
    D_CONSTEXPR_CPP14 bool
    is_real() const D_NOEXCEPT
    {
        // each eigenvalue in turn
        for (size_type idx = 0; idx < N; ++idx)
        {
            // one off the real line: not every one is real
            if (!(m_values[idx].imag() == static_cast<T>(0)))
            {
                return false;
            }
        }

        return true;
    }

private:
    vector_type m_values;
    matrix_type m_vectors;
    bool        m_converged;
};

// 3.4    eigen_general
//------------------------------------------------------------------------------
// 3.4.1
// eigen_general
//   function: the general eigendecomposition of _a.
template<typename    T,
         std::size_t N>
D_CONSTEXPR_CPP14 general_eigen<T, N>
eigen_general(
    const matrix<T, N, N>& _a,
    std::size_t            _max_iter = 1000,
    T                      _tol      = static_cast<T>(0)
) D_NOEXCEPT
{
    return general_eigen<T, N>(_a, _max_iter, _tol);
}

}  // namespace linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_EIGEN_HPP
