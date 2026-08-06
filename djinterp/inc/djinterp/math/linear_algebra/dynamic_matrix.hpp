/******************************************************************************
* djinterp [math]                                           dynamic_matrix.hpp
*
* Runtime-sized matrix companion for the linear-algebra subframework.
*   dynamic_matrix<_T> mirrors the fixed-size matrix API but chooses its
* dimensions at runtime and stores its entries on the heap (row-major, in a
* std::vector). It is the tool for when the shape is not known until run time.
*
* RELATION TO matrix<_T, _Rows, _Cols>:
*   The fixed-size matrix is compile-time sized and fully D_CONSTEXPR; it should
* be preferred whenever the dimensions are known at compile time (no allocation,
* compile-time evaluation, dimensions checked by the type system). dynamic_matrix
* is intentionally NOT constexpr -- heap storage cannot be a constant expression
* before C++20 -- and trades those guarantees for runtime flexibility.
*   The two interoperate: from_fixed / to_fixed convert between them.
*
* CONVENTIONS THAT DIFFER FROM THE FIXED-SIZE TYPE:
*   - Where the fixed type enforces shapes with static_assert, dynamic_matrix
*     checks them at run time with assert (its runtime analog). Operations have
*     dimension preconditions; violating them is a programming error.
*   - A runtime "vector" is simply a std::vector<_T>; matrix-vector products take
*     and return one.
*   - Like the fixed-size type, every operation returns a new value, so results
*     still chain: a.transposed().scaled(2.0).times(b).
*
* path:      /inc/djinterp/math/linear_algebra/dynamic_matrix.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.23
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_DYNAMIC_MATRIX_
#define DJINTERP_MATH_LINALG_DYNAMIC_MATRIX_ 1

// std
#include <cstddef>
#include <vector>
#include <cassert>
#include <cmath>
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
// I.   DYNAMIC MATRIX
// ===========================================================================

// dynamic_matrix
//   class: a runtime-sized, row-major, heap-backed matrix.
template<typename _T>
class dynamic_matrix
{
    static_assert(std::is_arithmetic<_T>::value,
                  "dynamic_matrix: element type must be arithmetic.");

public:
    using value_type   = _T;
    using size_type    = std::size_t;
    using storage_type = std::vector<_T>;
    using vector_type  = std::vector<_T>;

    // ---- construction -----------------------------------------------------

    // default: an empty 0 x 0 matrix.
    dynamic_matrix() noexcept
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
        , m_data(_rows * _cols, static_cast<_T>(0))
    {
    }

    // a _rows x _cols matrix with every entry equal to _value.
    dynamic_matrix(
        size_type _rows,
        size_type _cols,
        _T        _value
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
        _T        _value
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
            m.m_data[i * _n + i] = static_cast<_T>(1);
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
    template<std::size_t _Rows,
             std::size_t _Cols>
    static dynamic_matrix
    from_fixed(const matrix<_T, _Rows, _Cols>& _m)
    {
        dynamic_matrix out(_Rows, _Cols);

        for (size_type i = 0; i < _Rows; ++i)
        {
            for (size_type j = 0; j < _Cols; ++j)
            {
                out.m_data[i * _Cols + j] = _m(i, j);
            }
        }

        return out;
    }

    // ---- shape / access ---------------------------------------------------

    size_type
    rows() const noexcept
    {
        return m_rows;
    }

    size_type
    cols() const noexcept
    {
        return m_cols;
    }

    size_type
    size() const noexcept
    {
        return m_rows * m_cols;
    }

    bool
    is_square() const noexcept
    {
        return (m_rows == m_cols);
    }

    const _T&
    operator()(
        size_type _i,
        size_type _j
    ) const
    {
        assert((_i < m_rows) && (_j < m_cols) &&
               "dynamic_matrix::operator(): index out of range.");

        return m_data[_i * m_cols + _j];
    }

    _T&
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
    data() const noexcept
    {
        return m_data;
    }

    // row: the _i-th row as a std::vector.
    vector_type
    row(size_type _i) const
    {
        assert((_i < m_rows) && "dynamic_matrix::row: index out of range.");

        vector_type r(m_cols, static_cast<_T>(0));

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

        vector_type c(m_rows, static_cast<_T>(0));

        for (size_type i = 0; i < m_rows; ++i)
        {
            c[i] = m_data[i * m_cols + _j];
        }

        return c;
    }

    // to_fixed: convert to a fixed-size matrix (dimensions must match).
    template<std::size_t _Rows,
             std::size_t _Cols>
    matrix<_T, _Rows, _Cols>
    to_fixed() const
    {
        assert((m_rows == _Rows) && (m_cols == _Cols) &&
               "dynamic_matrix::to_fixed: dimension mismatch.");

        matrix<_T, _Rows, _Cols> out;

        for (size_type i = 0; i < _Rows; ++i)
        {
            for (size_type j = 0; j < _Cols; ++j)
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

        for (size_type i = 0; i < m_data.size(); ++i)
        {
            m.m_data[i] = m_data[i] + _o.m_data[i];
        }

        return m;
    }

    dynamic_matrix
    minus(const dynamic_matrix& _o) const
    {
        assert((m_rows == _o.m_rows) && (m_cols == _o.m_cols) &&
               "dynamic_matrix::minus: dimension mismatch.");

        dynamic_matrix m(m_rows, m_cols);

        for (size_type i = 0; i < m_data.size(); ++i)
        {
            m.m_data[i] = m_data[i] - _o.m_data[i];
        }

        return m;
    }

    dynamic_matrix
    scaled(_T _s) const
    {
        dynamic_matrix m(m_rows, m_cols);

        for (size_type i = 0; i < m_data.size(); ++i)
        {
            m.m_data[i] = m_data[i] * _s;
        }

        return m;
    }

    dynamic_matrix
    negated() const
    {
        dynamic_matrix m(m_rows, m_cols);

        for (size_type i = 0; i < m_data.size(); ++i)
        {
            m.m_data[i] = -m_data[i];
        }

        return m;
    }

    dynamic_matrix
    hadamard(const dynamic_matrix& _o) const
    {
        assert((m_rows == _o.m_rows) && (m_cols == _o.m_cols) &&
               "dynamic_matrix::hadamard: dimension mismatch.");

        dynamic_matrix m(m_rows, m_cols);

        for (size_type i = 0; i < m_data.size(); ++i)
        {
            m.m_data[i] = m_data[i] * _o.m_data[i];
        }

        return m;
    }

    // ---- transpose / products ---------------------------------------------

    dynamic_matrix
    transposed() const
    {
        dynamic_matrix m(m_cols, m_rows);

        for (size_type i = 0; i < m_rows; ++i)
        {
            for (size_type j = 0; j < m_cols; ++j)
            {
                m.m_data[j * m_rows + i] = m_data[i * m_cols + j];
            }
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

        for (size_type i = 0; i < m_rows; ++i)
        {
            for (size_type j = 0; j < _rhs.m_cols; ++j)
            {
                _T acc = static_cast<_T>(0);

                for (size_type k = 0; k < m_cols; ++k)
                {
                    acc += m_data[i * m_cols + k] * _rhs.m_data[k * _rhs.m_cols + j];
                }

                m.m_data[i * _rhs.m_cols + j] = acc;
            }
        }

        return m;
    }

    // times (vector): the matrix-vector product (*this) * _v.
    vector_type
    times(const vector_type& _v) const
    {
        assert((m_cols == _v.size()) &&
               "dynamic_matrix::times: vector length must equal column count.");

        vector_type out(m_rows, static_cast<_T>(0));

        for (size_type i = 0; i < m_rows; ++i)
        {
            _T acc = static_cast<_T>(0);

            for (size_type j = 0; j < m_cols; ++j)
            {
                acc += m_data[i * m_cols + j] * _v[j];
            }

            out[i] = acc;
        }

        return out;
    }

    // ---- square-only / norms / functional ---------------------------------

    _T
    trace() const
    {
        assert(is_square() && "dynamic_matrix::trace: matrix must be square.");

        _T acc = static_cast<_T>(0);

        for (size_type i = 0; i < m_rows; ++i)
        {
            acc += m_data[i * m_cols + i];
        }

        return acc;
    }

    // power: integer matrix power (square only); _n == 0 yields the identity.
    dynamic_matrix
    power(size_type _n) const
    {
        assert(is_square() && "dynamic_matrix::power: matrix must be square.");

        dynamic_matrix result = identity(m_rows);

        for (size_type k = 0; k < _n; ++k)
        {
            result = result.times(*this);
        }

        return result;
    }

    _T
    norm_squared() const
    {
        _T acc = static_cast<_T>(0);

        for (size_type i = 0; i < m_data.size(); ++i)
        {
            acc += m_data[i] * m_data[i];
        }

        return acc;
    }

    // norm: the Frobenius norm.
    _T
    norm() const
    {
        return static_cast<_T>(std::sqrt(static_cast<double>(norm_squared())));
    }

    // map: a new matrix with _fn applied to each entry.
    template<typename _Fn>
    dynamic_matrix
    map(_Fn _fn) const
    {
        dynamic_matrix m(m_rows, m_cols);

        for (size_type i = 0; i < m_data.size(); ++i)
        {
            m.m_data[i] = static_cast<_T>(_fn(m_data[i]));
        }

        return m;
    }

    // ---- comparison -------------------------------------------------------

    // equals: same shape and entries within _tol (different shape -> false).
    bool
    equals(
        const dynamic_matrix& _o,
        _T                    _tol = default_tolerance<_T>()
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


// ===========================================================================
// II.  OPERATORS
// ===========================================================================

template<typename _T>
dynamic_matrix<_T>
operator+(
    const dynamic_matrix<_T>& _a,
    const dynamic_matrix<_T>& _b
)
{
    return _a.plus(_b);
}

template<typename _T>
dynamic_matrix<_T>
operator-(
    const dynamic_matrix<_T>& _a,
    const dynamic_matrix<_T>& _b
)
{
    return _a.minus(_b);
}

template<typename _T>
dynamic_matrix<_T>
operator-(const dynamic_matrix<_T>& _a)
{
    return _a.negated();
}

template<typename _T,
         typename _S,
         typename std::enable_if<std::is_arithmetic<_S>::value, int>::type = 0>
dynamic_matrix<_T>
operator*(
    const dynamic_matrix<_T>& _m,
    _S                        _s
)
{
    return _m.scaled(static_cast<_T>(_s));
}

template<typename _S,
         typename _T,
         typename std::enable_if<std::is_arithmetic<_S>::value, int>::type = 0>
dynamic_matrix<_T>
operator*(
    _S                        _s,
    const dynamic_matrix<_T>& _m
)
{
    return _m.scaled(static_cast<_T>(_s));
}

template<typename _T>
dynamic_matrix<_T>
operator*(
    const dynamic_matrix<_T>& _a,
    const dynamic_matrix<_T>& _b
)
{
    return _a.times(_b);
}

template<typename _T>
std::vector<_T>
operator*(
    const dynamic_matrix<_T>& _m,
    const std::vector<_T>&    _v
)
{
    return _m.times(_v);
}

template<typename _T>
bool
operator==(
    const dynamic_matrix<_T>& _a,
    const dynamic_matrix<_T>& _b
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

template<typename _T>
bool
operator!=(
    const dynamic_matrix<_T>& _a,
    const dynamic_matrix<_T>& _b
)
{
    return !(_a == _b);
}


// ===========================================================================
// III. FREE FUNCTIONS  (procedural spelling; delegate to the members)
// ===========================================================================

template<typename _T>
dynamic_matrix<_T>
transpose(const dynamic_matrix<_T>& _m)
{
    return _m.transposed();
}

template<typename _T>
dynamic_matrix<_T>
multiply(
    const dynamic_matrix<_T>& _a,
    const dynamic_matrix<_T>& _b
)
{
    return _a.times(_b);
}

template<typename _T>
std::vector<_T>
multiply(
    const dynamic_matrix<_T>& _m,
    const std::vector<_T>&    _v
)
{
    return _m.times(_v);
}

template<typename _T>
dynamic_matrix<_T>
add(
    const dynamic_matrix<_T>& _a,
    const dynamic_matrix<_T>& _b
)
{
    return _a.plus(_b);
}

template<typename _T>
dynamic_matrix<_T>
subtract(
    const dynamic_matrix<_T>& _a,
    const dynamic_matrix<_T>& _b
)
{
    return _a.minus(_b);
}

template<typename _T>
_T
trace(const dynamic_matrix<_T>& _m)
{
    return _m.trace();
}

template<typename _T>
_T
frobenius_norm(const dynamic_matrix<_T>& _m)
{
    return _m.norm();
}

template<typename _T>
bool
approx_equal(
    const dynamic_matrix<_T>& _a,
    const dynamic_matrix<_T>& _b,
    _T                        _tol = default_tolerance<_T>()
)
{
    return _a.equals(_b, _tol);
}

}  // linalg

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINALG_DYNAMIC_MATRIX_
