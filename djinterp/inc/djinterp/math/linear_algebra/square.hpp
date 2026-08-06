/******************************************************************************
* djinterp [math]                                                   square.hpp
*
* Square-matrix operations for the linear-algebra subframework.
*   Free function templates over matrix<_T, _N, _N>: the determinant, the
* inverse, invertibility tests, the submatrix / minor / cofactor family, the
* adjugate, and an orthogonality test. Every routine is D_CONSTEXPR, so each
* evaluates at compile time or at runtime unchanged.
*
* PROVIDED FUNCTIONS:
*   determinant(m) / det(m)         - determinant (fraction-free elimination)
*   is_invertible(m [, tol])        - nonzero-determinant test
*   is_singular(m [, tol])          - complement of is_invertible
*   inverse(m) / inv(m)             - inverse (Gauss-Jordan, partial pivoting)
*   submatrix(m, i, j)              - (N-1)x(N-1) block with row i, col j removed
*   minor(m, i, j)                  - determinant of that block
*   cofactor(m, i, j)              - signed minor (-1)^(i+j) * minor
*   cofactor_matrix(m)             - matrix of cofactors
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
*     largest-magnitude entry for numerical stability.
*   - submatrix / minor / cofactor / cofactor_matrix / adjugate require N >= 2.
*   - These are free functions (mirroring the geometry measure headers) but
*     compose directly with the core fluent members, e.g.
*       inverse(a).transposed()        determinant(a.transposed())
*
* path:      /inc/djinterp/math/linear_algebra/square.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_SQUARE_
#define DJINTERP_MATH_LINALG_SQUARE_ 1

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
// I.   DETERMINANT
// ===========================================================================

// determinant
//   the determinant of a square matrix, via the Bareiss fraction-free
// elimination (exact for integral types; correct for floating-point types).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
determinant(const matrix<_T, _N, _N>& _m) noexcept
{
    matrix<_T, _N, _N> a    = _m;
    _T                 prev = static_cast<_T>(1);
    _T                 sign = static_cast<_T>(1);

    for (std::size_t k = 0; k < _N; ++k)
    {
        // ensure a nonzero pivot at (k, k), swapping in a lower row if needed.
        if (a(k, k) == static_cast<_T>(0))
        {
            std::size_t swap_row = k;

            // search the rows below for a nonzero entry in this column.
            for (std::size_t r = k + 1; r < _N; ++r)
            {
                if (a(r, k) != static_cast<_T>(0))
                {
                    swap_row = r;
                    break;
                }
            }

            // an all-zero pivot column means the matrix is singular.
            if (swap_row == k)
            {
                return static_cast<_T>(0);
            }

            // swap the two rows; each swap negates the determinant.
            for (std::size_t c = 0; c < _N; ++c)
            {
                constexpr_swap(a(k, c), a(swap_row, c));
            }

            sign = -sign;
        }

        // fraction-free (Bareiss) update of the trailing submatrix. The
        // division by the previous pivot is exact in integer arithmetic.
        for (std::size_t i = k + 1; i < _N; ++i)
        {
            for (std::size_t j = k + 1; j < _N; ++j)
            {
                a(i, j) =
                    ( a(k, k) * a(i, j) - a(i, k) * a(k, j) ) / prev;
            }
        }

        prev = a(k, k);
    }

    return sign * prev;
}

// det
//   short alias for determinant.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
det(const matrix<_T, _N, _N>& _m) noexcept
{
    return determinant(_m);
}


// ===========================================================================
// II.  INVERTIBILITY
// ===========================================================================

// is_invertible
//   true when the determinant is nonzero (within _tol). For an integral
// element type the default tolerance is 0, so this is an exact nonzero test.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR bool
is_invertible(
    const matrix<_T, _N, _N>& _m,
    _T                        _tol = default_tolerance<_T>()
) noexcept
{
    return ( internal::abs_c(determinant(_m)) > _tol );
}

// is_singular
//   complement of is_invertible.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR bool
is_singular(
    const matrix<_T, _N, _N>& _m,
    _T                        _tol = default_tolerance<_T>()
) noexcept
{
    return !is_invertible(_m, _tol);
}


// ===========================================================================
// III. INVERSE
// ===========================================================================

// inverse
//   the matrix inverse by Gauss-Jordan elimination with partial pivoting.
// Requires a floating-point element type; returns the zero matrix when the
// input is singular (guard with is_invertible).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N, _N>
inverse(const matrix<_T, _N, _N>& _m) noexcept
{
    static_assert(std::is_floating_point<_T>::value,
                  "inverse: requires a floating-point element type (an "
                  "integer matrix has no integer inverse in general).");

    matrix<_T, _N, _N> a   = _m;
    matrix<_T, _N, _N> inv = matrix<_T, _N, _N>::identity();

    for (std::size_t col = 0; col < _N; ++col)
    {
        // partial pivot: pick the largest-magnitude entry on/below the
        // diagonal in this column for numerical stability.
        std::size_t pivot = col;
        _T          maxv  = internal::abs_c(a(col, col));

        for (std::size_t r = col + 1; r < _N; ++r)
        {
            const _T v = internal::abs_c(a(r, col));

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // a zero pivot column means the matrix is singular.
        if (a(pivot, col) == static_cast<_T>(0))
        {
            return matrix<_T, _N, _N>::zeros();
        }

        // move the pivot row into place in both matrices.
        if (pivot != col)
        {
            for (std::size_t c = 0; c < _N; ++c)
            {
                constexpr_swap(a(col, c), a(pivot, c));
                constexpr_swap(inv(col, c), inv(pivot, c));
            }
        }

        // scale the pivot row so that a(col, col) becomes 1.
        const _T d = a(col, col);

        for (std::size_t c = 0; c < _N; ++c)
        {
            a(col, c)   = a(col, c) / d;
            inv(col, c) = inv(col, c) / d;
        }

        // eliminate this column from every other row.
        for (std::size_t r = 0; r < _N; ++r)
        {
            if (r != col)
            {
                const _T f = a(r, col);

                for (std::size_t c = 0; c < _N; ++c)
                {
                    a(r, c)   = a(r, c) - f * a(col, c);
                    inv(r, c) = inv(r, c) - f * inv(col, c);
                }
            }
        }
    }

    return inv;
}

// inv
//   short alias for inverse.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N, _N>
inv(const matrix<_T, _N, _N>& _m) noexcept
{
    return inverse(_m);
}


// ===========================================================================
// IV.  SUBMATRIX / MINORS / COFACTORS
// ===========================================================================

// submatrix
//   the (N-1) x (N-1) block obtained by deleting row _row and column _col.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N - 1, _N - 1>
submatrix(
    const matrix<_T, _N, _N>& _m,
    std::size_t               _row,
    std::size_t               _col
) noexcept
{
    static_assert((_N >= 2), "submatrix: requires dimension >= 2.");

    matrix<_T, _N - 1, _N - 1> s;
    std::size_t                rr = 0;

    for (std::size_t r = 0; r < _N; ++r)
    {
        // copy every row except the deleted one.
        if (r != _row)
        {
            std::size_t cc = 0;

            for (std::size_t c = 0; c < _N; ++c)
            {
                // copy every column except the deleted one.
                if (c != _col)
                {
                    s(rr, cc) = _m(r, c);
                    ++cc;
                }
            }

            ++rr;
        }
    }

    return s;
}

// minor
//   the (i, j) minor: the determinant of the submatrix with row i, col j
// removed.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
minor(
    const matrix<_T, _N, _N>& _m,
    std::size_t               _row,
    std::size_t               _col
) noexcept
{
    static_assert((_N >= 2), "minor: requires dimension >= 2.");

    return determinant(submatrix(_m, _row, _col));
}

// cofactor
//   the (i, j) cofactor: (-1)^(i+j) times the (i, j) minor.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
cofactor(
    const matrix<_T, _N, _N>& _m,
    std::size_t               _row,
    std::size_t               _col
) noexcept
{
    static_assert((_N >= 2), "cofactor: requires dimension >= 2.");

    const _T sign =
        ( ((_row + _col) % 2) == 0 ) ? static_cast<_T>(1)
                                     : static_cast<_T>(-1);

    return sign * minor(_m, _row, _col);
}

// cofactor_matrix
//   the matrix whose (i, j) entry is the (i, j) cofactor.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N, _N>
cofactor_matrix(const matrix<_T, _N, _N>& _m) noexcept
{
    static_assert((_N >= 2), "cofactor_matrix: requires dimension >= 2.");

    matrix<_T, _N, _N> c;

    for (std::size_t i = 0; i < _N; ++i)
    {
        for (std::size_t j = 0; j < _N; ++j)
        {
            c(i, j) = cofactor(_m, i, j);
        }
    }

    return c;
}


// ===========================================================================
// V.   ADJUGATE
// ===========================================================================

// adjugate
//   the classical adjoint: the transpose of the cofactor matrix. Satisfies
// A * adjugate(A) == determinant(A) * I (exactly, for integral types).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N, _N>
adjugate(const matrix<_T, _N, _N>& _m) noexcept
{
    static_assert((_N >= 2), "adjugate: requires dimension >= 2.");

    return cofactor_matrix(_m).transposed();
}


// ===========================================================================
// VI.  ORTHOGONALITY
// ===========================================================================

// is_orthogonal
//   true when M^T M equals the identity within _tol (columns orthonormal).
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR bool
is_orthogonal(
    const matrix<_T, _N, _N>& _m,
    _T                        _tol = default_tolerance<_T>()
) noexcept
{
    return _m.transposed().times(_m).is_identity(_tol);
}

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINALG_SQUARE_
