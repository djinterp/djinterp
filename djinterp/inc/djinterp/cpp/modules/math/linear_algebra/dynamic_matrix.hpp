/*******************************************************************************
* djinterp [cpp/math]                                         dynamic_matrix.hpp
*
* Runtime-sized matrix companion for the linear-algebra subframework.
*   dynamic_matrix<T> mirrors the fixed-size matrix API but chooses its
* dimensions at runtime and stores its entries on the heap (row-major, in a
* std::vector). It is the tool for when the shape is not known until run time.
*
* RELATION TO matrix<T, Rows, Cols>:
*   The fixed-size matrix is compile-time sized and fully D_CONSTEXPR; it should
* be preferred whenever the dimensions are known at compile time (no allocation,
* compile-time evaluation, dimensions checked by the type system).
* dynamic_matrix is intentionally NOT constexpr -- heap storage cannot be a
* constant expression before C++20 -- and trades those guarantees for runtime
* flexibility.
*   The two interoperate: from_fixed / to_fixed convert between them.
*
* A FACE OVER THE C CORE:
*   The element-wise arithmetic, the transpose, the products, the trace and
* the norms are c/math's kernels through linalg_kernel, the loops this header
* wrote, in their order, on the vector's storage -- through double for an
* element type no family holds, as the fixed-size matrix does. The norm's
* root is the element type's, correctly rounded (it was a double's whatever
* the type). Everything compiles from C++98; the scalar products select
* arithmetic scalars by an enable_if on their return type.
*
* CONVENTIONS THAT DIFFER FROM THE FIXED-SIZE TYPE:
*   - Where the fixed type enforces shapes with static_assert, dynamic_matrix
*     checks them at run time with assert (its runtime analog). Operations have
*     dimension preconditions; violating them is a programming error.
*   - A runtime "vector" is simply a std::vector<T>; matrix-vector products take
*     and return one.
*   - Like the fixed-size type, every operation returns a new value, so results
*     still chain: a.transposed().scaled(2.0).times(b).
*
*
* path:      /inc/djinterp/cpp/modules/math/linear_algebra/dynamic_matrix.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.23
*                                                            revised: 2026.10.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DYNAMIC MATRIX
    --------------
2.  OPERATORS
    ---------
3.  FREE FUNCTIONS  (PROCEDURAL SPELLING; DELEGATE TO THE MEMBERS)
    --------------------------------------------------------------
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_DYNAMIC_MATRIX_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_DYNAMIC_MATRIX_HPP 1

// std
#include <cassert>                                // assert
#include <cstddef>                                // std::size_t
#include <vector>                                 // std::vector
// djinterp
#include "../../../djinterp.hpp"                  // framework root
#include "./linalg_common.hpp"                    // linalg_kernel,
                                                  // default_tolerance
#include "./matrix.hpp"                           // matrix
#include "./vector.hpp"                           // vector
// re_std
#include "../../../../../re_std/cpp/type_traits/enable_if.hpp"      // enable_if
#include "../../../../../re_std/cpp/type_traits/is_arithmetic.hpp"  // is_arithmetic


NS_DJINTERP
NS_MATH

namespace linalg
{

//==============================================================================
// 1.  DYNAMIC MATRIX
//==============================================================================

// dynamic_matrix
//   class: a runtime-sized, row-major, heap-backed matrix.
template<typename T>
class dynamic_matrix
{
    D_STATIC_ASSERT((re_std::is_arithmetic<T>::value),
                    "dynamic_matrix: element type must be arithmetic.");

public:
    typedef T value_type;
    typedef std::size_t size_type;
    typedef std::vector<T> storage_type;
    typedef std::vector<T> vector_type;

private:
    // the C core's kernels for T (through double for a type no family
    // holds)
    typedef internal::linalg_kernel<T> kernel;

public:

    // ---- construction -----------------------------------------------------

    // default: an empty 0 x 0 matrix.
    dynamic_matrix() D_NOEXCEPT
        : m_rows(0)
        , m_cols(0)
        , m_data()
    {
    }

    // a _rows x _cols zero matrix.
    dynamic_matrix(
        size_type _rows,
        size_type _cols
    )
        : m_rows(_rows)
        , m_cols(_cols)
        , m_data(_rows * _cols, static_cast<T>(0))
    {
    }

    // a _rows x _cols matrix with every entry equal to _value.
    dynamic_matrix(
        size_type _rows,
        size_type _cols,
        T        _value
    )
        : m_rows(_rows)
        , m_cols(_cols)
        , m_data(_rows * _cols, _value)
    {
    }

    // ---- named factories --------------------------------------------------

    static dynamic_matrix
    zeros(
        size_type _rows,
        size_type _cols
    )
    {
        return dynamic_matrix(_rows, _cols);
    }

    static dynamic_matrix
    filled(
        size_type _rows,
        size_type _cols,
        T        _value
    )
    {
        return dynamic_matrix(_rows, _cols, _value);
    }

    // identity: the _n x _n identity matrix.
    static dynamic_matrix
    identity(size_type _n)
    {
        dynamic_matrix m(_n, _n);

        for (size_type i = 0; i < _n; ++i)
        {
            m.m_data[i * _n + i] = static_cast<T>(1);
        }

        return m;
    }

    // from_row_major: build from a flat, row-major buffer (size must be r*c).
    static dynamic_matrix
    from_row_major(
        size_type           _rows,
        size_type           _cols,
        const storage_type& _entries
    )
    {
        assert((_entries.size() == (_rows * _cols)) &&
               "dynamic_matrix::from_row_major: buffer size mismatch.");

        dynamic_matrix m(_rows, _cols);
        m.m_data = _entries;

        return m;
    }

    // from_fixed: copy a fixed-size matrix into a dynamic one.
    template<std::size_t Rows,
             std::size_t Cols>
    static dynamic_matrix
    from_fixed(const matrix<T, Rows, Cols>& _m)
    {
        dynamic_matrix out(Rows, Cols);

        for (size_type i = 0; i < Rows; ++i)
        {
            for (size_type j = 0; j < Cols; ++j)
            {
                out.m_data[i * Cols + j] = _m(i, j);
            }
        }

        return out;
    }

    // ---- shape / access ---------------------------------------------------

    size_type
    rows() const D_NOEXCEPT
    {
        return m_rows;
    }

    size_type
    cols() const D_NOEXCEPT
    {
        return m_cols;
    }

    size_type
    size() const D_NOEXCEPT
    {
        return m_rows * m_cols;
    }

    bool
    is_square() const D_NOEXCEPT
    {
        return (m_rows == m_cols);
    }

    const T&
    operator()(
        size_type _i,
        size_type _j
    ) const
    {
        assert((_i < m_rows) && (_j < m_cols) &&
               "dynamic_matrix::operator(): index out of range.");

        return m_data[_i * m_cols + _j];
    }

    T&
    operator()(
        size_type _i,
        size_type _j
    )
    {
        assert((_i < m_rows) && (_j < m_cols) &&
               "dynamic_matrix::operator(): index out of range.");

        return m_data[_i * m_cols + _j];
    }

    const storage_type&
    data() const D_NOEXCEPT
    {
        return m_data;
    }

    // row: the _i-th row as a std::vector.
    vector_type
    row(size_type _i) const
    {
        assert((_i < m_rows) && "dynamic_matrix::row: index out of range.");

        vector_type r(m_cols, static_cast<T>(0));

        for (size_type j = 0; j < m_cols; ++j)
        {
            r[j] = m_data[_i * m_cols + j];
        }

        return r;
    }

    // col: the _j-th column as a std::vector.
    vector_type
    col(size_type _j) const
    {
        assert((_j < m_cols) && "dynamic_matrix::col: index out of range.");

        vector_type c(m_rows, static_cast<T>(0));

        for (size_type i = 0; i < m_rows; ++i)
        {
            c[i] = m_data[i * m_cols + _j];
        }

        return c;
    }

    // to_fixed: convert to a fixed-size matrix (dimensions must match).
    template<std::size_t Rows,
             std::size_t Cols>
    matrix<T, Rows, Cols>
    to_fixed() const
    {
        assert((m_rows == Rows) && (m_cols == Cols) &&
               "dynamic_matrix::to_fixed: dimension mismatch.");

        matrix<T, Rows, Cols> out;

        for (size_type i = 0; i < Rows; ++i)
        {
            for (size_type j = 0; j < Cols; ++j)
            {
                out(i, j) = m_data[i * m_cols + j];
            }
        }

        return out;
    }

    // ---- additive / scalar arithmetic (fluent) ----------------------------

    dynamic_matrix
    plus(const dynamic_matrix& _o) const
    {
        assert((m_rows == _o.m_rows) && (m_cols == _o.m_cols) &&
               "dynamic_matrix::plus: dimension mismatch.");

        dynamic_matrix m(m_rows, m_cols);

        // element by element, the C core's
        if (!m_data.empty())
        {
            kernel::add(&m.m_data[0], &m_data[0], &_o.m_data[0],
                        m_data.size());
        }

        return m;
    }

    dynamic_matrix
    minus(const dynamic_matrix& _o) const
    {
        assert((m_rows == _o.m_rows) && (m_cols == _o.m_cols) &&
               "dynamic_matrix::minus: dimension mismatch.");

        dynamic_matrix m(m_rows, m_cols);

        // element by element, the C core's
        if (!m_data.empty())
        {
            kernel::sub(&m.m_data[0], &m_data[0], &_o.m_data[0],
                        m_data.size());
        }

        return m;
    }

    dynamic_matrix
    scaled(T _s) const
    {
        dynamic_matrix m(m_rows, m_cols);

        // element by element, the C core's
        if (!m_data.empty())
        {
            kernel::scale(&m.m_data[0], &m_data[0], _s, m_data.size());
        }

        return m;
    }

    dynamic_matrix
    negated() const
    {
        dynamic_matrix m(m_rows, m_cols);

        // element by element, the C core's
        if (!m_data.empty())
        {
            kernel::negate(&m.m_data[0], &m_data[0], m_data.size());
        }

        return m;
    }

    dynamic_matrix
    hadamard(const dynamic_matrix& _o) const
    {
        assert((m_rows == _o.m_rows) && (m_cols == _o.m_cols) &&
               "dynamic_matrix::hadamard: dimension mismatch.");

        dynamic_matrix m(m_rows, m_cols);

        // element by element, the C core's
        if (!m_data.empty())
        {
            kernel::hadamard(&m.m_data[0], &m_data[0], &_o.m_data[0],
                             m_data.size());
        }

        return m;
    }

    // ---- transpose / products ---------------------------------------------

    dynamic_matrix
    transposed() const
    {
        dynamic_matrix m(m_cols, m_rows);

        // the C core's
        if (!m_data.empty())
        {
            kernel::transpose(&m.m_data[0], &m_data[0], m_rows, m_cols);
        }

        return m;
    }

    // times (matrix): the matrix product (*this) * _rhs.
    dynamic_matrix
    times(const dynamic_matrix& _rhs) const
    {
        assert((m_cols == _rhs.m_rows) &&
               "dynamic_matrix::times: inner dimensions must agree.");

        dynamic_matrix m(m_rows, _rhs.m_cols);

        // the C core's product, row by column in order
        if ( (!m.m_data.empty()) &&
             (m_cols > 0) )
        {
            kernel::multiply(&m.m_data[0], &m_data[0], &_rhs.m_data[0],
                             m_rows, m_cols, _rhs.m_cols);
        }

        return m;
    }

    // times (vector): the matrix-vector product (*this) * _v.
    vector_type
    times(const vector_type& _v) const
    {
        assert((m_cols == _v.size()) &&
               "dynamic_matrix::times: vector length must equal column "
               "count.");

        vector_type out(m_rows, static_cast<T>(0));

        // the C core's product, row by row in order
        if ( (!out.empty()) &&
             (m_cols > 0) )
        {
            kernel::multiply_vector(&out[0], &m_data[0], &_v[0], m_rows,
                                    m_cols);
        }

        return out;
    }

    // ---- square-only / norms / functional ---------------------------------

    T
    trace() const
    {
        assert(is_square() && "dynamic_matrix::trace: matrix must be square.");

        // the C core's
        return m_data.empty() ? static_cast<T>(0)
                              : kernel::trace(&m_data[0], m_rows);
    }

    // power: integer matrix power (square only); _n == 0 yields the
    // identity; otherwise the C core's, by squaring (d_mat_power): from the
    // exponent's highest bit down, the power squared, then times A on the
    // left where the bit is 1.
    dynamic_matrix
    power(size_type _n) const
    {
        assert(is_square() && "dynamic_matrix::power: matrix must be square.");

        dynamic_matrix result = identity(m_rows);

        // the C core's, where there is an entry
        if (!m_data.empty())
        {
            storage_type scratch(m_data.size(), static_cast<T>(0));

            kernel::power(&result.m_data[0],
                          &scratch[0],
                          &m_data[0],
                          m_rows,
                          _n);
        }

        return result;
    }

    T
    norm_squared() const
    {
        // the C core's
        return m_data.empty() ? static_cast<T>(0)
                              : kernel::norm_squared(&m_data[0],
                                                     m_data.size());
    }

    // norm: the Frobenius norm.
    T
    norm() const
    {
        // the C core's: its root the element type's, correctly rounded (it
        // was a double's whatever the type)
        return m_data.empty() ? static_cast<T>(0)
                              : kernel::norm(&m_data[0], m_data.size());
    }

    // map: a new matrix with _fn applied to each entry.
    template<typename Fn>
    dynamic_matrix
    map(Fn _fn) const
    {
        dynamic_matrix m(m_rows, m_cols);

        for (size_type i = 0; i < m_data.size(); ++i)
        {
            m.m_data[i] = static_cast<T>(_fn(m_data[i]));
        }

        return m;
    }

    // ---- comparison -------------------------------------------------------

    // equals: same shape and entries within _tol (different shape -> false);
    // given none, within four of the largest epsilon the entries carry times
    // their largest magnitude, exactly where they are exact (S157, S158).
    bool
    equals(const dynamic_matrix& _o) const
    {
        // a shape apart: unequal, whatever the tolerance
        if ((m_rows != _o.m_rows) || (m_cols != _o.m_cols))
        {
            return false;
        }

        return equals(_o,
                      ( m_data.empty()
                            ? number_traits<T>::zero()
                            : internal::linalg_tolerance(&m_data[0],
                                                         &_o.m_data[0],
                                                         m_data.size(),
                                                         false) ));
    }

    bool
    equals(
        const dynamic_matrix& _o,
        T                    _tol
    ) const
    {
        if ((m_rows != _o.m_rows) || (m_cols != _o.m_cols))
        {
            return false;
        }

        for (size_type i = 0; i < m_data.size(); ++i)
        {
            if (internal::abs_c(m_data[i] - _o.m_data[i]) > _tol)
            {
                return false;
            }
        }

        return true;
    }

private:
    size_type    m_rows;
    size_type    m_cols;
    storage_type m_data;
};


//==============================================================================
// 2.  OPERATORS
//==============================================================================

template<typename T>
dynamic_matrix<T>
operator+(
    const dynamic_matrix<T>& _a,
    const dynamic_matrix<T>& _b
)
{
    return _a.plus(_b);
}

template<typename T>
dynamic_matrix<T>
operator-(
    const dynamic_matrix<T>& _a,
    const dynamic_matrix<T>& _b
)
{
    return _a.minus(_b);
}

template<typename T>
dynamic_matrix<T>
operator-(const dynamic_matrix<T>& _a)
{
    return _a.negated();
}

template<typename T,
         typename S>
typename re_std::enable_if<re_std::is_arithmetic<S>::value,
                           dynamic_matrix<T> >::type
operator*(
    const dynamic_matrix<T>& _m,
    S                        _s
)
{
    return _m.scaled(static_cast<T>(_s));
}

template<typename S,
         typename T>
typename re_std::enable_if<re_std::is_arithmetic<S>::value,
                           dynamic_matrix<T> >::type
operator*(
    S                        _s,
    const dynamic_matrix<T>& _m
)
{
    return _m.scaled(static_cast<T>(_s));
}

template<typename T>
dynamic_matrix<T>
operator*(
    const dynamic_matrix<T>& _a,
    const dynamic_matrix<T>& _b
)
{
    return _a.times(_b);
}

template<typename T>
std::vector<T>
operator*(
    const dynamic_matrix<T>& _m,
    const std::vector<T>&    _v
)
{
    return _m.times(_v);
}

template<typename T>
bool
operator==(
    const dynamic_matrix<T>& _a,
    const dynamic_matrix<T>& _b
)
{
    if ((_a.rows() != _b.rows()) || (_a.cols() != _b.cols()))
    {
        return false;
    }

    for (std::size_t i = 0; i < _a.data().size(); ++i)
    {
        if (!(_a.data()[i] == _b.data()[i]))
        {
            return false;
        }
    }

    return true;
}

template<typename T>
bool
operator!=(
    const dynamic_matrix<T>& _a,
    const dynamic_matrix<T>& _b
)
{
    return !(_a == _b);
}


//==============================================================================
// 3.  FREE FUNCTIONS  (PROCEDURAL SPELLING; DELEGATE TO THE MEMBERS)
//==============================================================================

template<typename T>
dynamic_matrix<T>
transpose(const dynamic_matrix<T>& _m)
{
    return _m.transposed();
}

template<typename T>
dynamic_matrix<T>
multiply(
    const dynamic_matrix<T>& _a,
    const dynamic_matrix<T>& _b
)
{
    return _a.times(_b);
}

template<typename T>
std::vector<T>
multiply(
    const dynamic_matrix<T>& _m,
    const std::vector<T>&    _v
)
{
    return _m.times(_v);
}

template<typename T>
dynamic_matrix<T>
add(
    const dynamic_matrix<T>& _a,
    const dynamic_matrix<T>& _b
)
{
    return _a.plus(_b);
}

template<typename T>
dynamic_matrix<T>
subtract(
    const dynamic_matrix<T>& _a,
    const dynamic_matrix<T>& _b
)
{
    return _a.minus(_b);
}

template<typename T>
T
trace(const dynamic_matrix<T>& _m)
{
    return _m.trace();
}

template<typename T>
T
frobenius_norm(const dynamic_matrix<T>& _m)
{
    return _m.norm();
}

template<typename T>
bool
approx_equal(
    const dynamic_matrix<T>& _a,
    const dynamic_matrix<T>& _b
)
{
    return _a.equals(_b);
}

template<typename T>
bool
approx_equal(
    const dynamic_matrix<T>& _a,
    const dynamic_matrix<T>& _b,
    T                        _tol
)
{
    return _a.equals(_b, _tol);
}

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_DYNAMIC_MATRIX_HPP
