/*******************************************************************************
* djinterp [cpp/math]                                                    svd.hpp
*
* Singular value decomposition and pseudoinverse for the linear-algebra
* subframework.
*   The (thin) SVD of an M x N matrix (M >= N) by the one-sided Jacobi
* (Hestenes) method: A = U S V^T with U an M x N matrix of orthonormal columns,
* S the N non-negative singular values (descending), and V an N x N orthogonal
* matrix. Built on construction as a factor-once object, like the other
* decompositions, with a free factory and the Moore-Penrose pseudoinverse
* layered on top.
*
* PROVIDED TYPES / FUNCTIONS:
*   svd_decomposition<T, M, N>       - thin SVD, A = U S V^T   (M >= N)
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
* A FACE OVER THE C CORE:
*   The Jacobi sweeps, the singular values, U and the sort are
* c/math/decomp.h's, the algorithm this header wrote, in its order. Its
* square roots are the element type's, correctly rounded (they were a
* double's by an iteration not correctly rounded): the C library's below
* C++14, the _c form from C++14, from C++20 whichever
* std::is_constant_evaluated asks for. Everything compiles from C++98;
* pseudoinverse picks its shape by a class template where it took an
* enable_if default.
*
*
*
* path:      /inc/djinterp/cpp/modules/math/linear_algebra/svd.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.23
*                                                            revised: 2026.10.07
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE SINGULAR VALUE DECOMPOSITION
    --------------------------------
    1.  svd_decomposition
    2.  svd
2.  THE PSEUDOINVERSE
    -----------------
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_SVD_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_SVD_HPP 1

// std
#include <cstddef>                                // std::size_t
#include <limits>                                 // std::numeric_limits
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "./decomposition.hpp"                    // internal::decomp_roots
#include "./linalg_common.hpp"                    // default_tolerance
#include "./matrix.hpp"                           // matrix, multiply
#include "./vector.hpp"                           // vector


NS_DJINTERP
NS_MATH

namespace linalg
{


//==============================================================================
// 1.  THE SINGULAR VALUE DECOMPOSITION
//==============================================================================


// 1.1    svd_decomposition
//------------------------------------------------------------------------------
// 1.1.1
// svd_decomposition
//   class: the thin SVD A = U S V^T of an M-by-N matrix, M >= N, factored
// on construction.
template<typename    T,
         std::size_t M,
         std::size_t N>
class svd_decomposition
{
    D_STATIC_ASSERT((internal::decomp_requires_float<T>::ok == 1),
                    "svd_decomposition: requires a floating-point element "
                    "type.");
    D_STATIC_ASSERT((M >= N),
                    "svd_decomposition: requires rows >= cols (M >= N); for "
                    "a wide matrix, decompose its transpose.");
    D_STATIC_ASSERT((N > 0),
                    "svd_decomposition: dimensions must be at least 1.");

public:
    typedef T               value_type;
    typedef std::size_t     size_type;
    typedef matrix<T, M, N> u_type;
    typedef matrix<T, N, N> v_type;
    typedef matrix<T, N, N> sigma_type;
    typedef vector<T, N>    values_type;

    D_CONSTEXPR_CPP14 explicit
    svd_decomposition(
        const matrix<T, M, N>& _a,
        T                      _tol = default_tolerance<T>()
    ) D_NOEXCEPT
        : m_u(),
          m_s(),
          m_v()
    {
        const T* entries = _a.data();
        T        work[M * N] = {};

        for (size_type i = 0; i < (M * N); ++i)
        {
            work[i] = entries[i];
        }

        for (size_type i = 0; i < N; ++i)
        {
            m_v[(i * N) + i] = static_cast<T>(1);
        }

        internal::decomp_roots<T>::svd(work, m_u, m_s, m_v, M, N, _tol);
    }

    D_CONSTEXPR_CPP14 u_type
    u() const D_NOEXCEPT
    {
        return u_type(m_u);
    }

    D_CONSTEXPR_CPP14 values_type
    singular_values() const D_NOEXCEPT
    {
        return values_type(m_s);
    }

    D_CONSTEXPR_CPP14 v_type
    v() const D_NOEXCEPT
    {
        return v_type(m_v);
    }

    // sigma
    //   the singular values on a diagonal.
    D_CONSTEXPR_CPP14 sigma_type
    sigma() const D_NOEXCEPT
    {
        return sigma_type::diagonal(values_type(m_s));
    }

    // rank
    //   how many singular values exceed _rcond times the largest.
    D_CONSTEXPR_CPP14 size_type
    rank(T _rcond = default_tolerance<T>()) const D_NOEXCEPT
    {
        const T   cutoff = _rcond * m_s[0];
        size_type r      = 0;

        for (size_type i = 0; i < N; ++i)
        {
            if (m_s[i] > cutoff)
            {
                ++r;
            }
        }

        return r;
    }

    // condition_number
    //   the largest singular value over the smallest; infinity for a
    // singular matrix.
    D_CONSTEXPR_CPP14 T
    condition_number() const D_NOEXCEPT
    {
        return (m_s[N - 1] > static_cast<T>(0))
                   ? (m_s[0] / m_s[N - 1])
                   : std::numeric_limits<T>::infinity();
    }

private:
    T m_u[M * N];
    T m_s[N];
    T m_v[N * N];
};

// 1.2    svd
//------------------------------------------------------------------------------
// 1.2.1
// svd
//   function: the thin SVD of _a.
template<typename    T,
         std::size_t M,
         std::size_t N>
D_CONSTEXPR_CPP14 svd_decomposition<T, M, N>
svd(
    const matrix<T, M, N>& _a,
    T                      _tol = default_tolerance<T>()
) D_NOEXCEPT
{
    return svd_decomposition<T, M, N>(_a, _tol);
}


//==============================================================================
// 2.  THE PSEUDOINVERSE
//==============================================================================


NS_INTERNAL

    // pinv_impl
    //   struct: V diag(s+) U^T for M >= N; through the transpose for M < N,
    // since pinv(A) == pinv(A^T)^T.
    template<typename    T,
             std::size_t M,
             std::size_t N,
             bool        Tall = (M >= N)>
    struct pinv_impl
    {
        static D_CONSTEXPR_CPP14 matrix<T, N, M>
        compute(
            const matrix<T, M, N>& _a,
            T                      _rcond
        ) D_NOEXCEPT
        {
            const svd_decomposition<T, M, N> d      = svd(_a);
            const matrix<T, M, N>            uu     = d.u();
            const matrix<T, N, N>            vv     = d.v();
            const vector<T, N>               ss     = d.singular_values();
            const T                          cutoff = _rcond * ss[0];
            matrix<T, N, N>                  scaled;

            for (std::size_t i = 0; i < N; ++i)
            {
                for (std::size_t j = 0; j < N; ++j)
                {
                    const T inv = (ss[j] > cutoff)
                                      ? (static_cast<T>(1) / ss[j])
                                      : static_cast<T>(0);

                    scaled(i, j) = vv(i, j) * inv;
                }
            }

            return multiply(scaled, uu.transposed());
        }
    };

    template<typename    T,
             std::size_t M,
             std::size_t N>
    struct pinv_impl<T, M, N, false>
    {
        static D_CONSTEXPR_CPP14 matrix<T, N, M>
        compute(
            const matrix<T, M, N>& _a,
            T                      _rcond
        ) D_NOEXCEPT
        {
            return pinv_impl<T, N, M>::compute(_a.transposed(), _rcond)
                .transposed();
        }
    };

NS_END  // internal

// pseudoinverse
//   function: the Moore-Penrose inverse of _a, any shape: V diag(s+) U^T,
// s+_i = 1 / s_i above the cutoff _rcond s_max and 0 below.
template<typename    T,
         std::size_t M,
         std::size_t N>
D_CONSTEXPR_CPP14 matrix<T, N, M>
pseudoinverse(
    const matrix<T, M, N>& _a,
    T                      _rcond = default_tolerance<T>()
) D_NOEXCEPT
{
    return internal::pinv_impl<T, M, N>::compute(_a, _rcond);
}

// pinv
//   function: pseudoinverse's short name.
template<typename    T,
         std::size_t M,
         std::size_t N>
D_CONSTEXPR_CPP14 matrix<T, N, M>
pinv(
    const matrix<T, M, N>& _a,
    T                      _rcond = default_tolerance<T>()
) D_NOEXCEPT
{
    return pseudoinverse(_a, _rcond);
}

}  // namespace linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_SVD_HPP
