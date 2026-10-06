/*******************************************************************************
* djinterp [math]                                                     matrix.hpp
*
* Fixed-size, row-major matrix for the linear-algebra subframework.
*   matrix<Type, Rows, Cols> stores its entries by value in a flat array
* (row-major: entry (i, j) lives at i * Cols + j), and every operation returns
* a new value, so matrices chain the same way vectors do. A face over the C
* core: for float, double and long double each operation is one of
* c/math/mat.h's kernels, or vec.h's over the entries, on the matrix's own
* array; for any other element type, the generic path (linalg_common.hpp).
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
* LEVELS:
*   Everything compiles from C++98. Operations are constexpr from C++14 --
* the norm too, on the C core's correctly rounded root -- built from an array
* of Rows * Cols entries at every level, from std::array from C++11.
*
*
* path:      /inc/djinterp/math/linear_algebra/matrix.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.22
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  MATRIX
    ------
    1.  matrix
2.  FREE FUNCTIONS
    --------------
    1.  Operators
    2.  Procedural spellings
3.  CONVENIENCE ALIASES
    -------------------
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_MATRIX_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_MATRIX_HPP 1

// std
#include <cstddef>                     // std::size_t
// djinterp
#include "../../djinterp.hpp"          // framework root
#include "./linalg_common.hpp"         // internal::linalg_kernel,
                                       // default_tolerance
#include "./vector.hpp"                // vector
// re_std
#include "../../../re_std/type_traits/enable_if.hpp"      // enable_if
#include "../../../re_std/type_traits/is_arithmetic.hpp"  // is_arithmetic
// std, from C++11 (the level is known only once the root is in)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <array>                   // std::array
#endif


NS_DJINTERP
NS_MATH
namespace linalg
{


//==============================================================================
// 1.  MATRIX
//==============================================================================


// 1.1    matrix
//------------------------------------------------------------------------------
// matrix
//   class: fixed-size, row-major matrix of Rows x Cols entries of Type.
template<typename    Type,
         std::size_t Rows,
         std::size_t Cols>
class matrix
{
private:
    typedef internal::linalg_kernel<Type> kernel;

    // the other shapes write this one's storage (transposed, times)
    template<typename,
             std::size_t,
             std::size_t>
    friend class matrix;

public:
    typedef Type                 value_type;
    typedef std::size_t          size_type;
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    typedef std::array<Type, Rows * Cols> array_type;
#endif
    typedef vector<Type, Cols>   row_vector;
    typedef vector<Type, Rows>   column_vector;

    D_STATIC_ASSERT(((Rows > 0) && (Cols > 0)),
                    "matrix: dimensions must be at least 1x1.");

    // ---- construction -----------------------------------------------------

    // default: the zero matrix.
    D_CONSTEXPR
    matrix() D_NOEXCEPT
        : m_data()
    {}

    // from a flat, row-major array of entries (every level).
    D_CONSTEXPR_CPP14 explicit
    matrix(
        const Type (&_entries)[Rows * Cols]
    ) D_NOEXCEPT
        : m_data()
    {
        for (size_type i = 0; i < (Rows * Cols); ++i)
        {
            m_data[i] = _entries[i];
        }
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from a flat, row-major std::array of entries (C++11).
    D_CONSTEXPR_CPP14 explicit
    matrix(
        const array_type& _entries
    ) D_NOEXCEPT
        : m_data()
    {
        for (size_type i = 0; i < (Rows * Cols); ++i)
        {
            m_data[i] = _entries[i];
        }
    }
#endif

    // ---- named factories --------------------------------------------------

    static D_CONSTEXPR matrix
    zeros() D_NOEXCEPT
    {
        return matrix();
    }

    static D_CONSTEXPR_CPP14 matrix
    filled(Type _value) D_NOEXCEPT
    {
        matrix m;

        kernel::fill(m.m_data, Rows * Cols, _value);

        return m;
    }

    static D_CONSTEXPR_CPP14 matrix
    from_row_major(const Type (&_entries)[Rows * Cols]) D_NOEXCEPT
    {
        return matrix(_entries);
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static D_CONSTEXPR_CPP14 matrix
    from_row_major(const array_type& _entries) D_NOEXCEPT
    {
        return matrix(_entries);
    }
#endif

    // identity: the identity matrix (square only).
    static D_CONSTEXPR_CPP14 matrix
    identity() D_NOEXCEPT
    {
        // matrix::identity: only defined for square matrices
        (void)internal::linalg_requires<(Rows == Cols)>::check();

        matrix m;

        kernel::identity(m.m_data, Rows);

        return m;
    }

    // diagonal: square matrix with _d on the main diagonal (square only).
    static D_CONSTEXPR_CPP14 matrix
    diagonal(const vector<Type, Rows>& _d) D_NOEXCEPT
    {
        // matrix::diagonal: only defined for square matrices
        (void)internal::linalg_requires<(Rows == Cols)>::check();

        matrix m;

        kernel::diagonal(m.m_data, _d.data(), Rows);

        return m;
    }

    // ---- shape / access ---------------------------------------------------

    static D_CONSTEXPR size_type
    rows() D_NOEXCEPT
    {
        return Rows;
    }

    static D_CONSTEXPR size_type
    cols() D_NOEXCEPT
    {
        return Cols;
    }

    static D_CONSTEXPR size_type
    size() D_NOEXCEPT
    {
        return Rows * Cols;
    }

    static D_CONSTEXPR bool
    is_square() D_NOEXCEPT
    {
        return (Rows == Cols);
    }

    // entry access (i, j).
    D_CONSTEXPR const Type&
    operator()(
        size_type _i,
        size_type _j
    ) const D_NOEXCEPT
    {
        return m_data[(_i * Cols) + _j];
    }

    D_CONSTEXPR_CPP14 Type&
    operator()(
        size_type _i,
        size_type _j
    ) D_NOEXCEPT
    {
        return m_data[(_i * Cols) + _j];
    }

    D_CONSTEXPR const Type*
    data() const D_NOEXCEPT
    {
        return m_data;
    }

    // row: the _i-th row as a vector<Type, Cols>.
    D_CONSTEXPR_CPP14 row_vector
    row(size_type _i) const D_NOEXCEPT
    {
        row_vector r;

        kernel::row(&r[0], m_data, _i, Cols);

        return r;
    }

    // col: the _j-th column as a vector<Type, Rows>.
    D_CONSTEXPR_CPP14 column_vector
    col(size_type _j) const D_NOEXCEPT
    {
        column_vector c;

        kernel::col(&c[0], m_data, _j, Rows, Cols);

        return c;
    }

    // with: a copy with entry (i, j) replaced (immutable set).
    D_CONSTEXPR_CPP14 matrix
    with(
        size_type _i,
        size_type _j,
        Type      _value
    ) const D_NOEXCEPT
    {
        matrix m = *this;

        m.m_data[(_i * Cols) + _j] = _value;

        return m;
    }

    // ---- additive / scalar arithmetic (fluent) ----------------------------

    D_CONSTEXPR_CPP14 matrix
    plus(const matrix& _o) const D_NOEXCEPT
    {
        matrix m;

        kernel::add(m.m_data, m_data, _o.m_data, Rows * Cols);

        return m;
    }

    D_CONSTEXPR_CPP14 matrix
    minus(const matrix& _o) const D_NOEXCEPT
    {
        matrix m;

        kernel::sub(m.m_data, m_data, _o.m_data, Rows * Cols);

        return m;
    }

    D_CONSTEXPR_CPP14 matrix
    scaled(Type _s) const D_NOEXCEPT
    {
        matrix m;

        kernel::scale(m.m_data, m_data, _s, Rows * Cols);

        return m;
    }

    D_CONSTEXPR_CPP14 matrix
    negated() const D_NOEXCEPT
    {
        matrix m;

        kernel::negate(m.m_data, m_data, Rows * Cols);

        return m;
    }

    // hadamard: entry-wise product.
    D_CONSTEXPR_CPP14 matrix
    hadamard(const matrix& _o) const D_NOEXCEPT
    {
        matrix m;

        kernel::hadamard(m.m_data, m_data, _o.m_data, Rows * Cols);

        return m;
    }

    // ---- transpose --------------------------------------------------------

    D_CONSTEXPR_CPP14 matrix<Type, Cols, Rows>
    transposed() const D_NOEXCEPT
    {
        matrix<Type, Cols, Rows> result;

        kernel::transpose(result.m_data, m_data, Rows, Cols);

        return result;
    }

    // ---- products ---------------------------------------------------------

    // times (matrix): the matrix product (*this) * _rhs, each entry its dot
    // product in order of k.
    template<std::size_t OtherCols>
    D_CONSTEXPR_CPP14 matrix<Type, Rows, OtherCols>
    times(const matrix<Type, Cols, OtherCols>& _rhs) const D_NOEXCEPT
    {
        matrix<Type, Rows, OtherCols> result;

        kernel::multiply(result.m_data,
                         m_data,
                         _rhs.m_data,
                         Rows,
                         Cols,
                         OtherCols);

        return result;
    }

    // times (vector): the matrix-vector product (*this) * _v.
    D_CONSTEXPR_CPP14 column_vector
    times(const row_vector& _v) const D_NOEXCEPT
    {
        column_vector result;

        kernel::multiply_vector(&result[0], m_data, _v.data(), Rows, Cols);

        return result;
    }

    // operator(): apply the matrix as a linear map (functional bridge).
    D_CONSTEXPR_CPP14 column_vector
    operator()(const row_vector& _v) const D_NOEXCEPT
    {
        return times(_v);
    }

    // ---- square-only operations -------------------------------------------

    // trace: sum of the main-diagonal entries (square only).
    D_CONSTEXPR_CPP14 Type
    trace() const D_NOEXCEPT
    {
        // matrix::trace: only defined for square matrices
        (void)internal::linalg_requires<(Rows == Cols)>::check();

        return kernel::trace(m_data, Rows);
    }

    // power: integer matrix power (square only); _n == 0 yields the
    // identity, and A^k = A * A^(k-1).
    D_CONSTEXPR_CPP14 matrix
    power(std::size_t _n) const D_NOEXCEPT
    {
        // matrix::power: only defined for square matrices
        (void)internal::linalg_requires<(Rows == Cols)>::check();

        matrix result;
        matrix scratch;

        kernel::power(result.m_data, scratch.m_data, m_data, Rows, _n);

        return result;
    }

    // is_symmetric: true when (*this) equals its transpose within _tol
    // (square only).
    D_CONSTEXPR_CPP14 bool
    is_symmetric(Type _tol = default_tolerance<Type>()) const D_NOEXCEPT
    {
        // matrix::is_symmetric: only defined for square matrices
        (void)internal::linalg_requires<(Rows == Cols)>::check();

        return kernel::is_symmetric(m_data, Rows, _tol);
    }

    // is_identity: true when every entry is within _tol of the identity's
    // (square only).
    D_CONSTEXPR_CPP14 bool
    is_identity(Type _tol = default_tolerance<Type>()) const D_NOEXCEPT
    {
        // matrix::is_identity: only defined for square matrices
        (void)internal::linalg_requires<(Rows == Cols)>::check();

        return kernel::is_identity(m_data, Rows, _tol);
    }

    // ---- norm / functional-style maps -------------------------------------

    D_CONSTEXPR_CPP14 Type
    norm_squared() const D_NOEXCEPT
    {
        return kernel::norm_squared(m_data, Rows * Cols);
    }

    // norm: the Frobenius norm, the root of the sum of squared entries.
    D_CONSTEXPR_CPP14 Type
    norm() const D_NOEXCEPT
    {
        return kernel::norm(m_data, Rows * Cols);
    }

    // map: a new matrix with _fn applied to each entry.
    template<typename Fn>
    D_CONSTEXPR_CPP14 matrix
    map(Fn _fn) const
    {
        matrix m;

        for (size_type i = 0; i < (Rows * Cols); ++i)
        {
            m.m_data[i] = static_cast<Type>(_fn(m_data[i]));
        }

        return m;
    }

    // ---- comparison -------------------------------------------------------

    // equals: every entry within _tol of _o's.
    D_CONSTEXPR_CPP14 bool
    equals(
        const matrix& _o,
        Type          _tol = default_tolerance<Type>()
    ) const D_NOEXCEPT
    {
        return kernel::equals(m_data, _o.m_data, _tol, Rows * Cols);
    }

    // equal_to: every entry == _o's (operator== spells it).
    D_CONSTEXPR_CPP14 bool
    equal_to(const matrix& _o) const D_NOEXCEPT
    {
        return kernel::equal(m_data, _o.m_data, Rows * Cols);
    }

private:
    // raw, flat, row-major storage: the array the C core's kernels take.
    Type m_data[Rows * Cols];
};


//==============================================================================
// 2.  FREE FUNCTIONS
//==============================================================================


// 2.1    Operators
//------------------------------------------------------------------------------
// A scalar factor is any arithmetic type, converted to Type; the constraint
// is on the return type, which C++98 can spell.

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 matrix<Type, Rows, Cols>
operator+(
    const matrix<Type, Rows, Cols>& _a,
    const matrix<Type, Rows, Cols>& _b
) D_NOEXCEPT
{
    return _a.plus(_b);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 matrix<Type, Rows, Cols>
operator-(
    const matrix<Type, Rows, Cols>& _a,
    const matrix<Type, Rows, Cols>& _b
) D_NOEXCEPT
{
    return _a.minus(_b);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 matrix<Type, Rows, Cols>
operator-(
    const matrix<Type, Rows, Cols>& _a
) D_NOEXCEPT
{
    return _a.negated();
}

template<typename Type, std::size_t Rows, std::size_t Cols, typename Scalar>
D_CONSTEXPR_CPP14
typename re_std::enable_if<re_std::is_arithmetic<Scalar>::value,
                           matrix<Type, Rows, Cols> >::type
operator*(
    const matrix<Type, Rows, Cols>& _m,
    Scalar                          _s
) D_NOEXCEPT
{
    return _m.scaled(static_cast<Type>(_s));
}

template<typename Scalar, typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14
typename re_std::enable_if<re_std::is_arithmetic<Scalar>::value,
                           matrix<Type, Rows, Cols> >::type
operator*(
    Scalar                          _s,
    const matrix<Type, Rows, Cols>& _m
) D_NOEXCEPT
{
    return _m.scaled(static_cast<Type>(_s));
}

template<typename    Type,
         std::size_t Rows,
         std::size_t Inner,
         std::size_t Cols>
D_CONSTEXPR_CPP14 matrix<Type, Rows, Cols>
operator*(
    const matrix<Type, Rows, Inner>& _a,
    const matrix<Type, Inner, Cols>& _b
) D_NOEXCEPT
{
    return _a.times(_b);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 vector<Type, Rows>
operator*(
    const matrix<Type, Rows, Cols>& _m,
    const vector<Type, Cols>&       _v
) D_NOEXCEPT
{
    return _m.times(_v);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 bool
operator==(
    const matrix<Type, Rows, Cols>& _a,
    const matrix<Type, Rows, Cols>& _b
) D_NOEXCEPT
{
    return _a.equal_to(_b);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 bool
operator!=(
    const matrix<Type, Rows, Cols>& _a,
    const matrix<Type, Rows, Cols>& _b
) D_NOEXCEPT
{
    return !(_a == _b);
}

// 2.2    Procedural spellings
//------------------------------------------------------------------------------
// Each delegates to its member; see the member for the meaning.

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 matrix<Type, Cols, Rows>
transpose(const matrix<Type, Rows, Cols>& _m) D_NOEXCEPT
{
    return _m.transposed();
}

template<typename    Type,
         std::size_t Rows,
         std::size_t Inner,
         std::size_t Cols>
D_CONSTEXPR_CPP14 matrix<Type, Rows, Cols>
multiply(
    const matrix<Type, Rows, Inner>& _a,
    const matrix<Type, Inner, Cols>& _b
) D_NOEXCEPT
{
    return _a.times(_b);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 vector<Type, Rows>
multiply(
    const matrix<Type, Rows, Cols>& _m,
    const vector<Type, Cols>&       _v
) D_NOEXCEPT
{
    return _m.times(_v);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 matrix<Type, Rows, Cols>
add(
    const matrix<Type, Rows, Cols>& _a,
    const matrix<Type, Rows, Cols>& _b
) D_NOEXCEPT
{
    return _a.plus(_b);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 matrix<Type, Rows, Cols>
subtract(
    const matrix<Type, Rows, Cols>& _a,
    const matrix<Type, Rows, Cols>& _b
) D_NOEXCEPT
{
    return _a.minus(_b);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 matrix<Type, Rows, Cols>
scale(
    const matrix<Type, Rows, Cols>& _m,
    Type                            _s
) D_NOEXCEPT
{
    return _m.scaled(_s);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 matrix<Type, Rows, Cols>
hadamard(
    const matrix<Type, Rows, Cols>& _a,
    const matrix<Type, Rows, Cols>& _b
) D_NOEXCEPT
{
    return _a.hadamard(_b);
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 Type
trace(const matrix<Type, N, N>& _m) D_NOEXCEPT
{
    return _m.trace();
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 Type
frobenius_norm(const matrix<Type, Rows, Cols>& _m) D_NOEXCEPT
{
    return _m.norm();
}

// identity<Type, N>(): the N x N identity matrix.
template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 matrix<Type, N, N>
identity() D_NOEXCEPT
{
    return matrix<Type, N, N>::identity();
}

template<typename Type, std::size_t N>
D_CONSTEXPR_CPP14 matrix<Type, N, N>
diagonal(const vector<Type, N>& _d) D_NOEXCEPT
{
    return matrix<Type, N, N>::diagonal(_d);
}

template<typename Type, std::size_t Rows, std::size_t Cols>
D_CONSTEXPR_CPP14 bool
approx_equal(
    const matrix<Type, Rows, Cols>& _a,
    const matrix<Type, Rows, Cols>& _b,
    Type                            _tol = default_tolerance<Type>()
) D_NOEXCEPT
{
    return _a.equals(_b, _tol);
}


//==============================================================================
// 3.  CONVENIENCE ALIASES
//==============================================================================
// The alias templates are C++11's; the double ones are types at every level.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename Type = double> using matrix2 = matrix<Type, 2, 2>;
template<typename Type = double> using matrix3 = matrix<Type, 3, 3>;
template<typename Type = double> using matrix4 = matrix<Type, 4, 4>;
#endif

typedef matrix<double, 2, 2> mat2d;
typedef matrix<double, 3, 3> mat3d;
typedef matrix<double, 4, 4> mat4d;


}  // linalg
NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_MATRIX_HPP
