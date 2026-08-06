/******************************************************************************
* djinterp [math]                                                   matrix.hpp
*
* Fixed-size, row-major matrix for the linear-algebra subframework.
*   matrix<_T, _Rows, _Cols> stores its entries by value in a flat std::array
* (row-major: entry (i, j) lives at i * _Cols + j) and every operation is
* D_CONSTEXPR and returns a new value, so matrices chain the same way vectors
* do and evaluate at compile time or runtime unchanged.
*
* TWO SPELLINGS (see linalg_common.hpp):
*   fluent      a.transposed().times(a).trace()
*   procedural  trace(multiply(transpose(a), a))
*
* FUNCTIONAL BRIDGE:
*   A matrix *is* a linear map. operator()(const vector&) applies it, so a
* matrix satisfies the functional subframework's is_callable / unary-transformer
* role directly:
*     compose(b, a)                       -> the map x |-> b(a(x))   (math order)
*     pipeline_from(xs).map(m).to_vector()-> apply m to each x in xs
* No adapter is needed; the matrix is already a vector -> vector callable.
*
* SCOPE:
*   This header covers construction, element/row/column access, additive and
* scalar arithmetic, the matrix*matrix and matrix*vector products, transpose,
* trace, the Frobenius norm, and integer powers. Determinant, inverse, the
* LU/QR/Cholesky decompositions, linear-system solvers, and the eigen routines
* live in their own headers and build on this one.
*
* path:      /inc/djinterp/math/linear_algebra/matrix.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_MATRIX_
#define DJINTERP_MATH_LINALG_MATRIX_ 1

// std
#include <cstddef>
#include <array>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "./linalg_common.hpp"
#include "./vector.hpp"


NS_DJINTERP
NS_MATH

namespace linalg
{

// ===========================================================================
// I.   MATRIX
// ===========================================================================

// matrix
//   class: fixed-size, row-major matrix of _Rows x _Cols entries of type _T.
template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
class matrix
{
    static_assert(((_Rows > 0) && (_Cols > 0)),
                  "matrix: dimensions must be at least 1x1.");

public:
    using value_type  = _T;
    using size_type   = std::size_t;
    using array_type  = std::array<_T, _Rows * _Cols>;
    using row_vector  = vector<_T, _Cols>;
    using column_vector = vector<_T, _Rows>;

    // ---- construction -----------------------------------------------------

    // default: the zero matrix.
    D_CONSTEXPR
    matrix() noexcept
        : m_data{}
    {
    }

    // from a flat, row-major array of entries (element-wise copy keeps this
    // constexpr from C++14; see the storage note at the bottom of the class).
    D_CONSTEXPR explicit
    matrix(
        const array_type& _entries
    ) noexcept
        : m_data{}
    {
        for (size_type i = 0; i < (_Rows * _Cols); ++i)
        {
            m_data[i] = _entries[i];
        }
    }

    // ---- named factories --------------------------------------------------

    static D_CONSTEXPR matrix
    zeros() noexcept
    {
        return matrix();
    }

    static D_CONSTEXPR matrix
    filled(_T _value) noexcept
    {
        matrix m;

        for (size_type i = 0; i < (_Rows * _Cols); ++i)
        {
            m.m_data[i] = _value;
        }

        return m;
    }

    static D_CONSTEXPR matrix
    from_row_major(const array_type& _entries) noexcept
    {
        return matrix(_entries);
    }

    // identity: the identity matrix (square only).
    static D_CONSTEXPR matrix
    identity() noexcept
    {
        static_assert((_Rows == _Cols),
                      "matrix::identity: only defined for square matrices.");

        matrix m;

        for (size_type i = 0; i < _Rows; ++i)
        {
            m.m_data[i * _Cols + i] = static_cast<_T>(1);
        }

        return m;
    }

    // diagonal: square matrix with _d on the main diagonal (square only).
    static D_CONSTEXPR matrix
    diagonal(const vector<_T, _Rows>& _d) noexcept
    {
        static_assert((_Rows == _Cols),
                      "matrix::diagonal: only defined for square matrices.");

        matrix m;

        for (size_type i = 0; i < _Rows; ++i)
        {
            m.m_data[i * _Cols + i] = _d[i];
        }

        return m;
    }

    // ---- shape / access ---------------------------------------------------

    static D_CONSTEXPR size_type
    rows() noexcept
    {
        return _Rows;
    }

    static D_CONSTEXPR size_type
    cols() noexcept
    {
        return _Cols;
    }

    static D_CONSTEXPR size_type
    size() noexcept
    {
        return _Rows * _Cols;
    }

    static D_CONSTEXPR bool
    is_square() noexcept
    {
        return (_Rows == _Cols);
    }

    // entry access (i, j).
    D_CONSTEXPR const _T&
    operator()(
        size_type _i,
        size_type _j
    ) const noexcept
    {
        return m_data[_i * _Cols + _j];
    }

    D_CONSTEXPR _T&
    operator()(
        size_type _i,
        size_type _j
    ) noexcept
    {
        return m_data[_i * _Cols + _j];
    }

    D_CONSTEXPR const _T*
    data() const noexcept
    {
        return m_data;
    }

    // row: the _i-th row as a vector<_T, _Cols>.
    D_CONSTEXPR row_vector
    row(size_type _i) const noexcept
    {
        row_vector r;

        for (size_type j = 0; j < _Cols; ++j)
        {
            r[j] = m_data[_i * _Cols + j];
        }

        return r;
    }

    // col: the _j-th column as a vector<_T, _Rows>.
    D_CONSTEXPR column_vector
    col(size_type _j) const noexcept
    {
        column_vector c;

        for (size_type i = 0; i < _Rows; ++i)
        {
            c[i] = m_data[i * _Cols + _j];
        }

        return c;
    }

    // with: a copy with entry (i, j) replaced (immutable set).
    D_CONSTEXPR matrix
    with(
        size_type _i,
        size_type _j,
        _T        _value
    ) const noexcept
    {
        matrix m                     = *this;
        m.m_data[_i * _Cols + _j]    = _value;

        return m;
    }

    // ---- additive / scalar arithmetic (fluent) ----------------------------

    D_CONSTEXPR matrix
    plus(const matrix& _o) const noexcept
    {
        matrix m;

        for (size_type i = 0; i < (_Rows * _Cols); ++i)
        {
            m.m_data[i] = m_data[i] + _o.m_data[i];
        }

        return m;
    }

    D_CONSTEXPR matrix
    minus(const matrix& _o) const noexcept
    {
        matrix m;

        for (size_type i = 0; i < (_Rows * _Cols); ++i)
        {
            m.m_data[i] = m_data[i] - _o.m_data[i];
        }

        return m;
    }

    D_CONSTEXPR matrix
    scaled(_T _s) const noexcept
    {
        matrix m;

        for (size_type i = 0; i < (_Rows * _Cols); ++i)
        {
            m.m_data[i] = m_data[i] * _s;
        }

        return m;
    }

    D_CONSTEXPR matrix
    negated() const noexcept
    {
        matrix m;

        for (size_type i = 0; i < (_Rows * _Cols); ++i)
        {
            m.m_data[i] = -m_data[i];
        }

        return m;
    }

    // hadamard: entry-wise product.
    D_CONSTEXPR matrix
    hadamard(const matrix& _o) const noexcept
    {
        matrix m;

        for (size_type i = 0; i < (_Rows * _Cols); ++i)
        {
            m.m_data[i] = m_data[i] * _o.m_data[i];
        }

        return m;
    }

    // ---- transpose --------------------------------------------------------

    D_CONSTEXPR matrix<_T, _Cols, _Rows>
    transposed() const noexcept
    {
        matrix<_T, _Cols, _Rows> result;

        for (size_type i = 0; i < _Rows; ++i)
        {
            for (size_type j = 0; j < _Cols; ++j)
            {
                result(j, i) = m_data[i * _Cols + j];
            }
        }

        return result;
    }

    // ---- products ---------------------------------------------------------

    // times (matrix): the matrix product (*this) * _rhs.
    template<std::size_t _OtherCols>
    D_CONSTEXPR matrix<_T, _Rows, _OtherCols>
    times(const matrix<_T, _Cols, _OtherCols>& _rhs) const noexcept
    {
        matrix<_T, _Rows, _OtherCols> result;

        for (size_type i = 0; i < _Rows; ++i)
        {
            for (size_type j = 0; j < _OtherCols; ++j)
            {
                _T acc = static_cast<_T>(0);

                for (size_type k = 0; k < _Cols; ++k)
                {
                    acc += (*this)(i, k) * _rhs(k, j);
                }

                result(i, j) = acc;
            }
        }

        return result;
    }

    // times (vector): the matrix-vector product (*this) * _v.
    D_CONSTEXPR column_vector
    times(const row_vector& _v) const noexcept
    {
        column_vector result;

        for (size_type i = 0; i < _Rows; ++i)
        {
            _T acc = static_cast<_T>(0);

            for (size_type j = 0; j < _Cols; ++j)
            {
                acc += m_data[i * _Cols + j] * _v[j];
            }

            result[i] = acc;
        }

        return result;
    }

    // operator(): apply the matrix as a linear map (functional bridge).
    D_CONSTEXPR column_vector
    operator()(const row_vector& _v) const noexcept
    {
        return times(_v);
    }

    // ---- square-only operations -------------------------------------------

    // trace: sum of the main-diagonal entries (square only).
    D_CONSTEXPR _T
    trace() const noexcept
    {
        static_assert((_Rows == _Cols),
                      "matrix::trace: only defined for square matrices.");

        _T acc = static_cast<_T>(0);

        for (size_type i = 0; i < _Rows; ++i)
        {
            acc += m_data[i * _Cols + i];
        }

        return acc;
    }

    // power: integer matrix power (square only). _n == 0 yields the identity.
    // Expressed recursively (A^n = A * A^(n-1)) so it needs no whole-matrix
    // copy-assignment, which is not a constant expression before C++17.
    D_CONSTEXPR matrix
    power(std::size_t _n) const noexcept
    {
        static_assert((_Rows == _Cols),
                      "matrix::power: only defined for square matrices.");

        return (_n == 0) ? matrix::identity() : times(power(_n - 1));
    }

    // is_symmetric: true when (*this) equals its transpose within _tol
    // (square only).
    D_CONSTEXPR bool
    is_symmetric(_T _tol = default_tolerance<_T>()) const noexcept
    {
        static_assert((_Rows == _Cols),
                      "matrix::is_symmetric: only defined for square "
                      "matrices.");

        for (size_type i = 0; i < _Rows; ++i)
        {
            for (size_type j = i + 1; j < _Cols; ++j)
            {
                const _T a = m_data[i * _Cols + j];
                const _T b = m_data[j * _Cols + i];

                if (internal::abs_c(a - b) > _tol)
                {
                    return false;
                }
            }
        }

        return true;
    }

    D_CONSTEXPR bool
    is_identity(_T _tol = default_tolerance<_T>()) const noexcept
    {
        static_assert((_Rows == _Cols),
                      "matrix::is_identity: only defined for square "
                      "matrices.");

        for (size_type i = 0; i < _Rows; ++i)
        {
            for (size_type j = 0; j < _Cols; ++j)
            {
                const _T expected =
                    (i == j) ? static_cast<_T>(1) : static_cast<_T>(0);

                if (internal::abs_c(m_data[i * _Cols + j] - expected) > _tol)
                {
                    return false;
                }
            }
        }

        return true;
    }

    // ---- norm / functional-style maps -------------------------------------

    D_CONSTEXPR _T
    norm_squared() const noexcept
    {
        _T acc = static_cast<_T>(0);

        for (size_type i = 0; i < (_Rows * _Cols); ++i)
        {
            acc += m_data[i] * m_data[i];
        }

        return acc;
    }

    // frobenius_norm / norm: square root of the sum of squared entries.
    D_CONSTEXPR _T
    norm() const noexcept
    {
        return static_cast<_T>(
            internal::sqrt_c(static_cast<double>(norm_squared())));
    }

    // map: a new matrix with _fn applied to each entry.
    template<typename _Fn>
    D_CONSTEXPR matrix
    map(_Fn _fn) const
    {
        matrix m;

        for (size_type i = 0; i < (_Rows * _Cols); ++i)
        {
            m.m_data[i] = static_cast<_T>(_fn(m_data[i]));
        }

        return m;
    }

    // ---- comparison -------------------------------------------------------

    D_CONSTEXPR bool
    equals(
        const matrix& _o,
        _T            _tol = default_tolerance<_T>()
    ) const noexcept
    {
        for (size_type i = 0; i < (_Rows * _Cols); ++i)
        {
            if (internal::abs_c(m_data[i] - _o.m_data[i]) > _tol)
            {
                return false;
            }
        }

        return true;
    }

private:
    // raw, flat, row-major storage. A built-in subscript write is a constant
    // expression from C++14, unlike std::array::operator[] (non-const), which
    // is only constexpr from C++17. array_type (std::array) is retained above
    // purely as the ergonomic entry type for the factories/operators.
    _T m_data[_Rows * _Cols];
};


// ===========================================================================
// II.  OPERATORS
// ===========================================================================

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
operator+(
    const matrix<_T, _Rows, _Cols>& _a,
    const matrix<_T, _Rows, _Cols>& _b
) noexcept
{
    return _a.plus(_b);
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
operator-(
    const matrix<_T, _Rows, _Cols>& _a,
    const matrix<_T, _Rows, _Cols>& _b
) noexcept
{
    return _a.minus(_b);
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
operator-(
    const matrix<_T, _Rows, _Cols>& _a
) noexcept
{
    return _a.negated();
}

// scalar on the right: m * s.
template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols,
         typename    _S,
         typename std::enable_if<std::is_arithmetic<_S>::value, int>::type = 0>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
operator*(
    const matrix<_T, _Rows, _Cols>& _m,
    _S                              _s
) noexcept
{
    return _m.scaled(static_cast<_T>(_s));
}

// scalar on the left: s * m.
template<typename    _S,
         typename    _T,
         std::size_t _Rows,
         std::size_t _Cols,
         typename std::enable_if<std::is_arithmetic<_S>::value, int>::type = 0>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
operator*(
    _S                              _s,
    const matrix<_T, _Rows, _Cols>& _m
) noexcept
{
    return _m.scaled(static_cast<_T>(_s));
}

// matrix * matrix.
template<typename    _T,
         std::size_t _Rows,
         std::size_t _Inner,
         std::size_t _Cols>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
operator*(
    const matrix<_T, _Rows, _Inner>& _a,
    const matrix<_T, _Inner, _Cols>& _b
) noexcept
{
    return _a.times(_b);
}

// matrix * vector.
template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR vector<_T, _Rows>
operator*(
    const matrix<_T, _Rows, _Cols>& _m,
    const vector<_T, _Cols>&        _v
) noexcept
{
    return _m.times(_v);
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR bool
operator==(
    const matrix<_T, _Rows, _Cols>& _a,
    const matrix<_T, _Rows, _Cols>& _b
) noexcept
{
    for (std::size_t i = 0; i < (_Rows * _Cols); ++i)
    {
        if (!(_a.data()[i] == _b.data()[i]))
        {
            return false;
        }
    }

    return true;
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR bool
operator!=(
    const matrix<_T, _Rows, _Cols>& _a,
    const matrix<_T, _Rows, _Cols>& _b
) noexcept
{
    return !(_a == _b);
}


// ===========================================================================
// III. FREE FUNCTIONS  (procedural spelling; delegate to the members)
// ===========================================================================

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR matrix<_T, _Cols, _Rows>
transpose(const matrix<_T, _Rows, _Cols>& _m) noexcept
{
    return _m.transposed();
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Inner,
         std::size_t _Cols>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
multiply(
    const matrix<_T, _Rows, _Inner>& _a,
    const matrix<_T, _Inner, _Cols>& _b
) noexcept
{
    return _a.times(_b);
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR vector<_T, _Rows>
multiply(
    const matrix<_T, _Rows, _Cols>& _m,
    const vector<_T, _Cols>&        _v
) noexcept
{
    return _m.times(_v);
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
add(
    const matrix<_T, _Rows, _Cols>& _a,
    const matrix<_T, _Rows, _Cols>& _b
) noexcept
{
    return _a.plus(_b);
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
subtract(
    const matrix<_T, _Rows, _Cols>& _a,
    const matrix<_T, _Rows, _Cols>& _b
) noexcept
{
    return _a.minus(_b);
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
scale(
    const matrix<_T, _Rows, _Cols>& _m,
    _T                              _s
) noexcept
{
    return _m.scaled(_s);
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR matrix<_T, _Rows, _Cols>
hadamard(
    const matrix<_T, _Rows, _Cols>& _a,
    const matrix<_T, _Rows, _Cols>& _b
) noexcept
{
    return _a.hadamard(_b);
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR _T
trace(const matrix<_T, _N, _N>& _m) noexcept
{
    return _m.trace();
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR _T
frobenius_norm(const matrix<_T, _Rows, _Cols>& _m) noexcept
{
    return _m.norm();
}

// identity<T, N>(): the N x N identity matrix.
template<typename    _T,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N, _N>
identity() noexcept
{
    return matrix<_T, _N, _N>::identity();
}

template<typename    _T,
         std::size_t _N>
D_CONSTEXPR matrix<_T, _N, _N>
diagonal(const vector<_T, _N>& _d) noexcept
{
    return matrix<_T, _N, _N>::diagonal(_d);
}

template<typename    _T,
         std::size_t _Rows,
         std::size_t _Cols>
D_CONSTEXPR bool
approx_equal(
    const matrix<_T, _Rows, _Cols>& _a,
    const matrix<_T, _Rows, _Cols>& _b,
    _T                              _tol = default_tolerance<_T>()
) noexcept
{
    return _a.equals(_b, _tol);
}


// ===========================================================================
// IV.  CONVENIENCE ALIASES
// ===========================================================================

template<typename _T = double> using matrix2 = matrix<_T, 2, 2>;
template<typename _T = double> using matrix3 = matrix<_T, 3, 3>;
template<typename _T = double> using matrix4 = matrix<_T, 4, 4>;

using mat2d = matrix<double, 2, 2>;
using mat3d = matrix<double, 3, 3>;
using mat4d = matrix<double, 4, 4>;

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINALG_MATRIX_
