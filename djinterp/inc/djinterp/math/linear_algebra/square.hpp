/*******************************************************************************
* djinterp [math]                                                     square.hpp
*
* Square-matrix operations for the linear-algebra subframework.
*   Free function templates over matrix<Type, N, N>: the determinant, the
* inverse, invertibility tests, the submatrix / minor / cofactor family, the
* adjugate, and an orthogonality test. A face over the C core: for float,
* double and long double each routine is one of c/math/mat.h's kernels on
* the matrix's own array, with its scratch on the stack; for any other
* element type the generic path (linalg_common.hpp) runs the same
* algorithms in the type's arithmetic. Everything compiles from C++98, and
* every routine is a constant expression from C++14.
*
* PROVIDED FUNCTIONS:
*   determinant(m) / det(m)         - determinant (fraction-free elimination)
*   is_invertible(m [, tol])        - nonzero-determinant test
*   is_singular(m [, tol])          - complement of is_invertible
*   inverse(m) / inv(m)             - inverse (Gauss-Jordan, partial pivoting)
*   submatrix(m, i, j)              - (N-1)x(N-1) block, row i and col j gone
*   minor(m, i, j)                  - determinant of that block
*   cofactor(m, i, j)               - signed minor (-1)^(i+j) * minor
*   cofactor_matrix(m)              - matrix of cofactors
*   adjugate(m)                     - transpose of the cofactor matrix
*   is_orthogonal(m [, tol])        - test M^T M == I
*
* DESIGN NOTES:
*   - determinant uses the Bareiss fraction-free algorithm: a single code path
*     that is EXACT for integral element types (every intermediate division is
*     exact) and correct for floating-point types. As a consequence the whole
*     minor/cofactor/adjugate family is integer-exact, and the identity
*     A * adjugate(A) == determinant(A) * I holds exactly for integral matrices.
*     For floating-point matrices the algorithm pivots only to avoid a zero
*     leading entry; it is well suited to the small fixed dimensions this
*     library targets.
*   - inverse requires a floating-point element type (an integer matrix has no
*     integer inverse in general) and returns the zero matrix for a singular
*     input -- pair it with is_invertible to guard. It pivots on the
*     largest-magnitude entry for numerical stability; in floating point a
*     singular matrix's last pivot can round to a tiny non-zero, so a
*     near-singular result is the caller's to recognize, as it always was.
*   - submatrix / minor / cofactor / cofactor_matrix / adjugate require N >= 2.
*   - These are free functions (mirroring the geometry measure headers) but
*     compose directly with the core fluent members, e.g.
*       inverse(a).transposed()        determinant(a.transposed())
*
*
* path:      /inc/djinterp/math/linear_algebra/square.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.22
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DETERMINANT AND INVERSE
    -----------------------
2.  MINORS AND COFACTORS
    --------------------
3.  ORTHOGONALITY
    -------------
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_SQUARE_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_SQUARE_HPP 1

// std
#include <cstddef>                 // std::size_t
// djinterp
#include "../../djinterp.hpp"      // framework root
#include "./linalg_common.hpp"     // internal::linalg_kernel,
                                   // internal::linalg_requires
#include "./matrix.hpp"            // matrix
#include "./vector.hpp"            // vector
// re_std
#include "../../../re_std/type_traits/is_floating_point.hpp"  // is_floating_
                                                              // point


NS_DJINTERP
NS_MATH
namespace linalg
{


//==============================================================================
// 1.  DETERMINANT AND INVERSE
//==============================================================================


// determinant
//   function: det(_m), by Bareiss's fraction-free elimination.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 Type
determinant(const matrix<Type, N, N>& _m) D_NOEXCEPT
{
    Type scratch[N * N] = { Type() };

    return internal::linalg_kernel<Type>::determinant(_m.data(), scratch, N);
}

// det
//   function: short alias for determinant.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 Type
det(const matrix<Type, N, N>& _m) D_NOEXCEPT
{
    return determinant(_m);
}

// is_invertible
//   function: true when |det(_m)| exceeds _tol.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 bool
is_invertible(
    const matrix<Type, N, N>& _m,
    Type                      _tol = default_tolerance<Type>()
) D_NOEXCEPT
{
    return (internal::abs_c(determinant(_m)) > _tol);
}

// is_singular
//   function: complement of is_invertible.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 bool
is_singular(
    const matrix<Type, N, N>& _m,
    Type                      _tol = default_tolerance<Type>()
) D_NOEXCEPT
{
    return !is_invertible(_m, _tol);
}

// inverse
//   function: _m^-1 by Gauss-Jordan with partial pivoting; the zero matrix
// when no pivot can be found. Floating-point element types only.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 matrix<Type, N, N>
inverse(const matrix<Type, N, N>& _m) D_NOEXCEPT
{
    // inverse: requires a floating-point element type (an integer matrix
    // has no integer inverse in general)
    (void)internal::linalg_requires<
        re_std::is_floating_point<Type>::value>::check();

    matrix<Type, N, N> result;
    Type               scratch[N * N] = { Type() };

    (void)internal::linalg_kernel<Type>::inverse(&result(0, 0),
                                                 scratch,
                                                 _m.data(),
                                                 N);

    return result;
}

// inv
//   function: short alias for inverse.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 matrix<Type, N, N>
inv(const matrix<Type, N, N>& _m) D_NOEXCEPT
{
    return inverse(_m);
}


//==============================================================================
// 2.  MINORS AND COFACTORS
//==============================================================================
// Each needs N >= 2; the minors are determinants, so integer-exact.


// submatrix
//   function: the (N-1)x(N-1) block with row _row and column _col removed.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 matrix<Type, N - 1, N - 1>
submatrix(
    const matrix<Type, N, N>& _m,
    std::size_t               _row,
    std::size_t               _col
) D_NOEXCEPT
{
    // submatrix: requires dimension >= 2
    (void)internal::linalg_requires<(N >= 2)>::check();

    matrix<Type, N - 1, N - 1> s;

    internal::linalg_kernel<Type>::submatrix(&s(0, 0),
                                             _m.data(),
                                             N,
                                             _row,
                                             _col);

    return s;
}

// minor
//   function: the determinant of submatrix(_m, _row, _col).
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 Type
minor(
    const matrix<Type, N, N>& _m,
    std::size_t               _row,
    std::size_t               _col
) D_NOEXCEPT
{
    // minor: requires dimension >= 2
    (void)internal::linalg_requires<(N >= 2)>::check();

    return determinant(submatrix(_m, _row, _col));
}

// cofactor
//   function: (-1)^(_row + _col) * minor(_m, _row, _col).
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 Type
cofactor(
    const matrix<Type, N, N>& _m,
    std::size_t               _row,
    std::size_t               _col
) D_NOEXCEPT
{
    // cofactor: requires dimension >= 2
    (void)internal::linalg_requires<(N >= 2)>::check();

    Type scratch[2 * N * N] = { Type() };

    return internal::linalg_kernel<Type>::cofactor(_m.data(),
                                                   scratch,
                                                   N,
                                                   _row,
                                                   _col);
}

// cofactor_matrix
//   function: the matrix whose (i, j) entry is cofactor(_m, i, j).
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 matrix<Type, N, N>
cofactor_matrix(const matrix<Type, N, N>& _m) D_NOEXCEPT
{
    // cofactor_matrix: requires dimension >= 2
    (void)internal::linalg_requires<(N >= 2)>::check();

    matrix<Type, N, N> c;
    Type               scratch[2 * N * N] = { Type() };

    internal::linalg_kernel<Type>::cofactor_matrix(&c(0, 0),
                                                   _m.data(),
                                                   scratch,
                                                   N,
                                                   false);

    return c;
}

// adjugate
//   function: the transpose of the cofactor matrix; A * adj(A) = det(A) * I.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 matrix<Type, N, N>
adjugate(const matrix<Type, N, N>& _m) D_NOEXCEPT
{
    // adjugate: requires dimension >= 2
    (void)internal::linalg_requires<(N >= 2)>::check();

    matrix<Type, N, N> a;
    Type               scratch[2 * N * N] = { Type() };

    internal::linalg_kernel<Type>::cofactor_matrix(&a(0, 0),
                                                   _m.data(),
                                                   scratch,
                                                   N,
                                                   true);

    return a;
}


//==============================================================================
// 3.  ORTHOGONALITY
//==============================================================================


// is_orthogonal
//   function: true when _m^T * _m equals the identity within _tol.
template<typename    Type,
         std::size_t N>
D_CONSTEXPR_CPP14 bool
is_orthogonal(
    const matrix<Type, N, N>& _m,
    Type                      _tol = default_tolerance<Type>()
) D_NOEXCEPT
{
    return _m.transposed().times(_m).is_identity(_tol);
}


}  // linalg
NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_SQUARE_HPP
