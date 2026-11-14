/*******************************************************************************
* djinterp [cpp/math]                                          decomposition.hpp
*
* Matrix factorizations for the linear-algebra subframework.
*   Three classic decompositions, each as a small value-type object that
* performs the factorization on construction and then answers queries -- the
* factor-once, use-many pattern. A free factory function (lu / qr / cholesky)
* produces each object, so the procedural and fluent spellings coincide:
*   procedural   lu_decomposition<double, 3> f = lu(A);  f.solve(b);
*   fluent       lu(A).solve(b)
*
* PROVIDED TYPES / FUNCTIONS:
*   lu_decomposition<T, N>           - partial-pivot LU, P A = L U
*     lu(A)                            l() u() p() pivot_sign() is_singular()
*                                      determinant() solve(b) inverse()
*   qr_decomposition<T, M, N>        - Householder QR, A = Q R   (M >= N)
*     qr(A)                            q() r()
*   cholesky_decomposition<T, N>     - Cholesky, A = L L^T  (symmetric pos-def)
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
* A FACE OVER THE C CORE:
*   The factorizations, solves and determinants are c/math/decomp.h's, the
* algorithms this header wrote, in their order, on the objects' own
* row-major arrays. QR's and Cholesky's square roots are the element type's,
* correctly rounded (they were a double's whatever the type): the C
* library's below C++14, the _c form from C++14, and from C++20 whichever
* std::is_constant_evaluated asks for. Everything compiles from C++98; it is
* constexpr from C++14, as it was.
*
*
* path:      /inc/djinterp/cpp/modules/math/linear_algebra/decomposition.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.22
*                                                            revised: 2026.11.07
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LU DECOMPOSITION
    ----------------
    1.  lu_decomposition
    2.  lu
2.  QR DECOMPOSITION
    ----------------
    1.  qr_decomposition
    2.  qr
3.  CHOLESKY DECOMPOSITION
    ----------------------
    1.  cholesky_decomposition
    2.  cholesky
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_DECOMPOSITION_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_DECOMPOSITION_HPP 1

// std
#include <cstddef>                                // std::size_t
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "../../../../c/modules/math/decomp.h"    // the C core: d_decomp_*,
                                                  // struct dj_cplx_* through
                                                  // cplx.h
#include "./linalg_common.hpp"                    // linalg_family_of
#include "./matrix.hpp"                           // matrix
#include "./vector.hpp"                           // vector
// std, from C++20 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #include <type_traits>                        // std::is_constant_evaluated
#endif


NS_DJINTERP
NS_MATH

namespace linalg
{

NS_INTERNAL

    // decomp_complex
    //   trait: the C core's complex number of a floating-point type, the
    // general eigensolver's eigenvalues and eigenvectors: cplx.h's struct
    // dj_cplx_f, dj_cplx_d or dj_cplx_ld.
    template<typename T>
    struct decomp_complex;

    template<>
    struct decomp_complex<float>
    {
        typedef dj_cplx_f type;
    };

    template<>
    struct decomp_complex<double>
    {
        typedef dj_cplx_d type;
    };

    template<>
    struct decomp_complex<long double>
    {
        typedef dj_cplx_ld type;
    };

    // decomp_roots
    //   struct: QR, Cholesky, the SVD, the symmetric eigensolver, and the
    // general eigenvalues and eigenvectors, in the form the level allows: the
    // C library's root below C++14, the _c form from C++14, and from C++20
    // whichever std::is_constant_evaluated asks for.
    template<typename T>
    struct decomp_roots
    {
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        static constexpr void
        qr(T* _q, T* _r, std::size_t _m, std::size_t _n, T* _v) noexcept
        {
            if (std::is_constant_evaluated())
            {
                d_decomp_qr_c(_q, _r, _m, _n, _v);
            }
            else
            {
                d_decomp_qr(_q, _r, _m, _n, _v);
            }

            return;
        }

        static constexpr bool
        cholesky(T* _l, const T* _a, std::size_t _n) noexcept
        {
            if (std::is_constant_evaluated())
            {
                return d_decomp_cholesky_c(_l, _a, _n);
            }

            return d_decomp_cholesky(_l, _a, _n);
        }

        static constexpr void
        svd(T* _w, T* _u, T* _s, T* _v, std::size_t _m, std::size_t _n,
            T _tol) noexcept
        {
            if (std::is_constant_evaluated())
            {
                d_decomp_svd_c(_w, _u, _s, _v, _m, _n, _tol);
            }
            else
            {
                d_decomp_svd(_w, _u, _s, _v, _m, _n, _tol);
            }

            return;
        }

        static constexpr void
        sym_eigen(T* _w, T* _values, T* _vectors, std::size_t _n,
                  T _threshold) noexcept
        {
            if (std::is_constant_evaluated())
            {
                d_decomp_sym_eigen_c(_w, _values, _vectors, _n, _threshold);
            }
            else
            {
                d_decomp_sym_eigen(_w, _values, _vectors, _n, _threshold);
            }

            return;
        }

        static constexpr bool
        eigenvalues(T* _h, std::size_t _n, T* _values, std::size_t _max_iter,
                    T _tol) noexcept
        {
            if (std::is_constant_evaluated())
            {
                return d_decomp_eigenvalues_c(_h, _n, _values, _max_iter,
                                              _tol);
            }

            return d_decomp_eigenvalues(_h, _n, _values, _max_iter, _tol);
        }

        static constexpr bool
        eigenvector(const T* _a, std::size_t _n, T _lambda, T* _lu,
                    std::size_t* _perm, T* _x, T* _y) noexcept
        {
            if (std::is_constant_evaluated())
            {
                return d_decomp_eigenvector_c(_a, _n, _lambda, _lu, _perm,
                                              _x, _y);
            }

            return d_decomp_eigenvector(_a, _n, _lambda, _lu, _perm, _x, _y);
        }

        static constexpr bool
        eigen(T*                                   _a,
              std::size_t                          _n,
              typename decomp_complex<T>::type*    _values,
              typename decomp_complex<T>::type*    _vectors,
              T*                                   _work,
              std::size_t*                         _perm,
              std::size_t                          _max_iter,
              T                                    _tol) noexcept
        {
            if (std::is_constant_evaluated())
            {
                return d_decomp_eigen_c(_a, _n, _values, _vectors, _work,
                                        _perm, _max_iter, _tol);
            }

            return d_decomp_eigen(_a, _n, _values, _vectors, _work, _perm,
                                  _max_iter, _tol);
        }
#elif D_ENV_LANG_IS_CPP14_OR_HIGHER
        static constexpr void
        qr(T* _q, T* _r, std::size_t _m, std::size_t _n, T* _v) noexcept
        {
            d_decomp_qr_c(_q, _r, _m, _n, _v);

            return;
        }

        static constexpr bool
        cholesky(T* _l, const T* _a, std::size_t _n) noexcept
        {
            return d_decomp_cholesky_c(_l, _a, _n);
        }

        static constexpr void
        svd(T* _w, T* _u, T* _s, T* _v, std::size_t _m, std::size_t _n,
            T _tol) noexcept
        {
            d_decomp_svd_c(_w, _u, _s, _v, _m, _n, _tol);

            return;
        }

        static constexpr void
        sym_eigen(T* _w, T* _values, T* _vectors, std::size_t _n,
                  T _threshold) noexcept
        {
            d_decomp_sym_eigen_c(_w, _values, _vectors, _n, _threshold);

            return;
        }

        static constexpr bool
        eigenvalues(T* _h, std::size_t _n, T* _values, std::size_t _max_iter,
                    T _tol) noexcept
        {
            return d_decomp_eigenvalues_c(_h, _n, _values, _max_iter, _tol);
        }

        static constexpr bool
        eigenvector(const T* _a, std::size_t _n, T _lambda, T* _lu,
                    std::size_t* _perm, T* _x, T* _y) noexcept
        {
            return d_decomp_eigenvector_c(_a, _n, _lambda, _lu, _perm, _x,
                                          _y);
        }

        static constexpr bool
        eigen(T*                                   _a,
              std::size_t                          _n,
              typename decomp_complex<T>::type*    _values,
              typename decomp_complex<T>::type*    _vectors,
              T*                                   _work,
              std::size_t*                         _perm,
              std::size_t                          _max_iter,
              T                                    _tol) noexcept
        {
            return d_decomp_eigen_c(_a, _n, _values, _vectors, _work, _perm,
                                    _max_iter, _tol);
        }
#else
        static void
        qr(T* _q, T* _r, std::size_t _m, std::size_t _n, T* _v) D_NOEXCEPT
        {
            d_decomp_qr(_q, _r, _m, _n, _v);

            return;
        }

        static bool
        cholesky(T* _l, const T* _a, std::size_t _n) D_NOEXCEPT
        {
            return d_decomp_cholesky(_l, _a, _n);
        }

        static void
        svd(T* _w, T* _u, T* _s, T* _v, std::size_t _m, std::size_t _n,
            T _tol) D_NOEXCEPT
        {
            d_decomp_svd(_w, _u, _s, _v, _m, _n, _tol);

            return;
        }

        static void
        sym_eigen(T* _w, T* _values, T* _vectors, std::size_t _n,
                  T _threshold) D_NOEXCEPT
        {
            d_decomp_sym_eigen(_w, _values, _vectors, _n, _threshold);

            return;
        }

        static bool
        eigenvalues(T* _h, std::size_t _n, T* _values, std::size_t _max_iter,
                    T _tol) D_NOEXCEPT
        {
            return d_decomp_eigenvalues(_h, _n, _values, _max_iter, _tol);
        }

        static bool
        eigenvector(const T* _a, std::size_t _n, T _lambda, T* _lu,
                    std::size_t* _perm, T* _x, T* _y) D_NOEXCEPT
        {
            return d_decomp_eigenvector(_a, _n, _lambda, _lu, _perm, _x, _y);
        }

        static bool
        eigen(T*                                   _a,
              std::size_t                          _n,
              typename decomp_complex<T>::type*    _values,
              typename decomp_complex<T>::type*    _vectors,
              T*                                   _work,
              std::size_t*                         _perm,
              std::size_t                          _max_iter,
              T                                    _tol) D_NOEXCEPT
        {
            return d_decomp_eigen(_a, _n, _values, _vectors, _work, _perm,
                                  _max_iter, _tol);
        }
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
    };

    // decomp_requires_float
    //   struct: the decompositions' requirement -- float, double or long
    // double -- asserted at class scope.
    template<typename T>
    struct decomp_requires_float
    {
        D_STATIC_ASSERT((linalg_family_of<T>::value != 0),
                        "decomposition: requires a floating-point element "
                        "type (float, double or long double).");

        enum
        {
            ok = 1
        };
    };

NS_END  // internal


//==============================================================================
// 1.  LU DECOMPOSITION
//==============================================================================


// 1.1    lu_decomposition
//------------------------------------------------------------------------------
// 1.1.1
// lu_decomposition
//   class: P A = L U by partial pivoting, factored on construction.
template<typename    T,
         std::size_t N>
class lu_decomposition
{
    D_STATIC_ASSERT((internal::decomp_requires_float<T>::ok == 1),
                    "lu_decomposition: requires a floating-point element "
                    "type.");
    D_STATIC_ASSERT((N > 0),
                    "lu_decomposition: dimension must be at least 1.");

public:
    typedef T               value_type;
    typedef std::size_t     size_type;
    typedef matrix<T, N, N> matrix_type;
    typedef vector<T, N>    vector_type;

    D_CONSTEXPR_CPP14 explicit
    lu_decomposition(
        const matrix_type& _a
    ) D_NOEXCEPT
        : m_lu(),
          m_perm(),
          m_sign(static_cast<T>(1)),
          m_singular(false)
    {
        const T* entries = _a.data();

        for (size_type i = 0; i < (N * N); ++i)
        {
            m_lu[i] = entries[i];
        }

        m_singular = d_decomp_lu(m_lu, N, m_perm, &m_sign);
    }

    // l
    //   the unit lower triangle.
    D_CONSTEXPR_CPP14 matrix_type
    l() const D_NOEXCEPT
    {
        matrix_type out;

        for (size_type i = 0; i < N; ++i)
        {
            for (size_type j = 0; j < N; ++j)
            {
                if (i > j)
                {
                    out(i, j) = m_lu[(i * N) + j];
                }
                else if (i == j)
                {
                    out(i, j) = static_cast<T>(1);
                }
            }
        }

        return out;
    }

    // u
    //   the upper triangle.
    D_CONSTEXPR_CPP14 matrix_type
    u() const D_NOEXCEPT
    {
        matrix_type out;

        for (size_type i = 0; i < N; ++i)
        {
            for (size_type j = i; j < N; ++j)
            {
                out(i, j) = m_lu[(i * N) + j];
            }
        }

        return out;
    }

    // p
    //   the permutation matrix, P(i, perm[i]) = 1.
    D_CONSTEXPR_CPP14 matrix_type
    p() const D_NOEXCEPT
    {
        matrix_type out;

        for (size_type i = 0; i < N; ++i)
        {
            out(i, m_perm[i]) = static_cast<T>(1);
        }

        return out;
    }

    D_CONSTEXPR T
    pivot_sign() const D_NOEXCEPT
    {
        return m_sign;
    }

    D_CONSTEXPR bool
    is_singular() const D_NOEXCEPT
    {
        return m_singular;
    }

    // determinant
    //   the parity times U's diagonal, in order; 0 for a singular matrix.
    D_CONSTEXPR_CPP14 T
    determinant() const D_NOEXCEPT
    {
        return d_decomp_lu_det(m_lu, N, m_sign);
    }

    // solve
    //   x with A x = _b.
    D_CONSTEXPR_CPP14 vector_type
    solve(const vector_type& _b) const D_NOEXCEPT
    {
        vector_type x;

        d_decomp_lu_solve(m_lu, m_perm, N, _b.data(), x.begin());

        return x;
    }

    // inverse
    //   A^-1, column by column from the solves of the unit vectors.
    D_CONSTEXPR_CPP14 matrix_type
    inverse() const D_NOEXCEPT
    {
        matrix_type out;

        for (size_type j = 0; j < N; ++j)
        {
            vector_type e;

            e[j] = static_cast<T>(1);

            const vector_type col = solve(e);

            for (size_type i = 0; i < N; ++i)
            {
                out(i, j) = col[i];
            }
        }

        return out;
    }

private:
    T         m_lu[N * N];
    size_type m_perm[N];
    T         m_sign;
    bool      m_singular;
};

// 1.2    lu
//------------------------------------------------------------------------------
// 1.2.1
// lu
//   function: the LU decomposition of _a.
template<typename    T,
         std::size_t N>
D_CONSTEXPR_CPP14 lu_decomposition<T, N>
lu(const matrix<T, N, N>& _a) D_NOEXCEPT
{
    return lu_decomposition<T, N>(_a);
}


//==============================================================================
// 2.  QR DECOMPOSITION
//==============================================================================


// 2.1    qr_decomposition
//------------------------------------------------------------------------------
// 2.1.1
// qr_decomposition
//   class: A = Q R by Householder reflectors, factored on construction.
template<typename    T,
         std::size_t M,
         std::size_t N>
class qr_decomposition
{
    D_STATIC_ASSERT((internal::decomp_requires_float<T>::ok == 1),
                    "qr_decomposition: requires a floating-point element "
                    "type.");
    D_STATIC_ASSERT((M >= N),
                    "qr_decomposition: requires rows >= cols (M >= N).");
    D_STATIC_ASSERT((N > 0),
                    "qr_decomposition: dimensions must be at least 1.");

public:
    typedef T               value_type;
    typedef std::size_t     size_type;
    typedef matrix<T, M, M> q_type;
    typedef matrix<T, M, N> r_type;

    D_CONSTEXPR_CPP14 explicit
    qr_decomposition(
        const matrix<T, M, N>& _a
    ) D_NOEXCEPT
        : m_q(),
          m_r()
    {
        const T* entries = _a.data();
        T        v[M]    = {};

        for (size_type i = 0; i < M; ++i)
        {
            m_q[(i * M) + i] = static_cast<T>(1);
        }

        for (size_type i = 0; i < (M * N); ++i)
        {
            m_r[i] = entries[i];
        }

        internal::decomp_roots<T>::qr(m_q, m_r, M, N, v);
    }

    D_CONSTEXPR_CPP14 q_type
    q() const D_NOEXCEPT
    {
        return q_type(m_q);
    }

    D_CONSTEXPR_CPP14 r_type
    r() const D_NOEXCEPT
    {
        return r_type(m_r);
    }

private:
    T m_q[M * M];
    T m_r[M * N];
};

// 2.2    qr
//------------------------------------------------------------------------------
// 2.2.1
// qr
//   function: the QR decomposition of _a.
template<typename    T,
         std::size_t M,
         std::size_t N>
D_CONSTEXPR_CPP14 qr_decomposition<T, M, N>
qr(const matrix<T, M, N>& _a) D_NOEXCEPT
{
    return qr_decomposition<T, M, N>(_a);
}


//==============================================================================
// 3.  CHOLESKY DECOMPOSITION
//==============================================================================


// 3.1    cholesky_decomposition
//------------------------------------------------------------------------------
// 3.1.1
// cholesky_decomposition
//   class: A = L L^T for a symmetric positive-definite A, factored on
// construction; a non-positive pivot clears is_spd() and stops.
template<typename    T,
         std::size_t N>
class cholesky_decomposition
{
    D_STATIC_ASSERT((internal::decomp_requires_float<T>::ok == 1),
                    "cholesky_decomposition: requires a floating-point "
                    "element type.");
    D_STATIC_ASSERT((N > 0),
                    "cholesky_decomposition: dimension must be at least 1.");

public:
    typedef T               value_type;
    typedef std::size_t     size_type;
    typedef matrix<T, N, N> matrix_type;
    typedef vector<T, N>    vector_type;

    D_CONSTEXPR_CPP14 explicit
    cholesky_decomposition(
        const matrix_type& _a
    ) D_NOEXCEPT
        : m_l(),
          m_spd(true)
    {
        m_spd = internal::decomp_roots<T>::cholesky(m_l, _a.data(), N);
    }

    D_CONSTEXPR_CPP14 matrix_type
    l() const D_NOEXCEPT
    {
        return matrix_type(m_l);
    }

    D_CONSTEXPR bool
    is_spd() const D_NOEXCEPT
    {
        return m_spd;
    }

    D_CONSTEXPR bool
    success() const D_NOEXCEPT
    {
        return m_spd;
    }

    // determinant
    //   L's diagonal product, squared.
    D_CONSTEXPR_CPP14 T
    determinant() const D_NOEXCEPT
    {
        return d_decomp_cholesky_det(m_l, N);
    }

    // solve
    //   x with A x = _b, through L and L^T.
    D_CONSTEXPR_CPP14 vector_type
    solve(const vector_type& _b) const D_NOEXCEPT
    {
        vector_type x;

        d_decomp_cholesky_solve(m_l, N, _b.data(), x.begin());

        return x;
    }

private:
    T    m_l[N * N];
    bool m_spd;
};

// 3.2    cholesky
//------------------------------------------------------------------------------
// 3.2.1
// cholesky
//   function: the Cholesky decomposition of _a.
template<typename    T,
         std::size_t N>
D_CONSTEXPR_CPP14 cholesky_decomposition<T, N>
cholesky(const matrix<T, N, N>& _a) D_NOEXCEPT
{
    return cholesky_decomposition<T, N>(_a);
}

}  // namespace linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_DECOMPOSITION_HPP
