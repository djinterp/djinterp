/*******************************************************************************
* djinterp [cpp/math]                                        matrix_function.hpp
*
* Linear algebra's functions of the run-time expression language's matrix
* values: det, inv, trace, norm, eig, dot, cross and solve; roadmap Phase 3,
* step 21 (3c), and their matrices of complex numbers and of quaternions,
* 3e's and 3f's third parts.
*   THE FUNCTIONS det, inv, trace, norm, eig, dot, cross and solve take
* matrices, or numbers as scalars (S256), and the function table
* (function_table.hpp) calls them by name: exactly for exact elements, by
* the generic kernels, a solution by the elimination of [A | B] (S257); in a
* dense type by the C core's, LU with partial pivoting, Gauss-Jordan and the
* eigenvalues' Jacobi and Francis's double-shift QR (S257, S258, S269), the
* norm scaled where its squares would leave the range (S256); a matrix of
* complex numbers' in the widest of its elements' types, by cmat.h's LU,
* Gauss-Jordan and elimination of [A | B] where dense (S351); and one of
* quaternions' by the generic kernels' Gauss-Jordan and elimination, each
* product in its order and each row divided on the left, as A X = B asks, on
* the kind's operands or on values whose traits say they do not commute,
* det a domain error (S277, S388). norm is of the moduli, and dot(a, b) sums
* conj(a_i) b_i (S273, S351); a quaternion is a scalar too, solve(q, B)
* q^-1 B on the left and eig(q) its standard eigenvalue (S390). The
* inverse is the kind's own Gauss-Jordan, which its negative powers take
* too (matrix_kind.hpp, 3.2.7).
*   EIGENVALUES are every one a matrix has, complex where it is (S352): a real
* matrix's as before, by Jacobi where it is symmetric and otherwise by
* Francis's double-shift QR, and where that finds a complex pair, all of them
* by decomp.h's d_decomp_eigen, each pair exactly conjugate, complex
* throughout; a Hermitian matrix's real, by cmat.h's Jacobi with complex
* rotations; any other complex matrix's by cmat.h's d_cmat_eigen. An exact
* matrix's are found in long double and each made exact where it is a Gaussian
* rational, as a root of its characteristic polynomial. A spectrum ascends by
* real part, then by imaginary part; one of a real matrix that is complex is a
* domain error under expr_context's real_results (S340). Each solver is given
* 30 QR steps a row, and at least 1000, the budget every matrix had before,
* under which one of 700 rows ran out. A matrix of quaternions has Zhang's
* standard right eigenvalues, n complex ones of imaginary part 0 or more:
* one of each conjugate pair of its complex adjoint's, 2n by 2n, found as a
* complex matrix's are, a triangular one's its diagonal's (S389).
*   ONE HEADER FOR THEM. Until chunk 48 they were section 3.5 of matrix.hpp,
* a header of 7,979 lines, the matrix kind's as well; now the kind, its
* values, its operators, transposes and blocks are matrix_kind.hpp's, and
* its functions this header's, which includes it (S401).
*   STEPS. Step 21 (3c) filled the functions; step 23 (3e), in its third
* part, chunk 43, linear algebra's functions of complex numbers and complex
* eigenvalues; step 24 (3f), in its third, chunk 46, linear algebra's
* functions of quaternions and their standard eigenvalues (S386 to S390).
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/matrix_function.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE PARTS
    ---------
    1.  Operands
    2.  The square algebra
    3.  Norms and products
    4.  Eigenvalues
    5.  expr_function_shape
2.  THE FUNCTIONS
    -------------
    1.  Of one matrix
         1.  expr_det, expr_inv, expr_trace, expr_norm, expr_eig
    2.  Of two
         1.  expr_dot, expr_cross, expr_solve
*/

#ifndef DJINTERP_MATH_EXPR_MATRIX_FUNCTION_HPP
#define DJINTERP_MATH_EXPR_MATRIX_FUNCTION_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <algorithm>  // std::stable_sort
#include <cmath>      // std::fabs, std::floor, std::frexp, std::ldexp, ...
#include <cstddef>    // std::size_t
#include <limits>     // std::numeric_limits
#include <string>     // std::string, std::to_string
#include <vector>     // std::vector
// djinterp
#include "../../../../c/modules/math/cmat.h"        // d_cmat_eigen,
                                                    // _hermitian_eigen
#include "../../../../c/modules/math/decomp.h"      // d_decomp_lu, _lu_det,
                                                    // _lu_solve, _eigenvalues,
                                                    // _sym_eigen, _eigen
#include "../../../../c/modules/math/elementary.h"  // d_math_sqrt
#include "../linear_algebra/linalg_common.hpp"      // linalg_kernel
#include "../math_common.hpp"                       // NS_MATH
#include "../value.hpp"                             // value
#include "./complex_kind.hpp"                       // expr_is_complex,
                                                    // expr_conj
#include "./expr_common.hpp"                        // expr_result
#include "./matrix_kind.hpp"                        // the matrix kind, its
                                                    // arithmetic
#include "./quaternion_kind.hpp"                    // the quaternion kind,
                                                    // its operands


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE PARTS
//==============================================================================
// Linear algebra's functions of matrix values (S256), which the function
// table (function_table.hpp) calls by name: det, the determinant of a square
// matrix; inv, its inverse; trace, the sum of its diagonal; norm, a
// vector's Euclidean norm, a matrix's Frobenius norm; eig, a square matrix's
// eigenvalues, a vector of one row, ascending; dot, two vectors' scalar
// product; cross, the vector product of two vectors of 3 elements; and
// solve(A, B), the X for which A X = B. A number is a scalar, as in Julia:
// det, trace and eig of it are itself, inv its reciprocal, norm its
// magnitude, dot of two its product, solve(a, B) B / a; cross takes none.
// A quaternion is one too (S390): det of it is a domain error, as of a
// matrix of quaternions (S277); eig of it its standard eigenvalue, w +
// |v|i; dot(p, q) is conj(p) q; and solve(a, B), where a is no real number
// and a or B holds a quaternion, a^-1 B, on the left, as A X = B asks.
// The square algebra is computed in the type S257 gives: exactly, by the
// generic kernels, for exact elements; by the C core's, in a dense type:
// LU with partial pivoting for det and solve, Gauss-Jordan for inv
// (matrix_kind.hpp, 3.2.7), Jacobi for the eigenvalues of a symmetric matrix
// and Hessenberg QR for any other's (S258). Each entry gives the value, an
// error value where its arithmetic fails, a singular matrix's inverse and
// solution among them, or bad_shape where an operand's shape is one the
// function does not take; the value's own functions, which the table calls,
// give a domain error there. An error passes, the first; a boolean or none is a
// domain error.
NS_INTERNAL

    // 1.1
    // Operands
    //   functions: whether an operand passes as it is: an error, the first,
    // or what is no number and no matrix, a domain error, into _out; a
    // shape in words; a vector's number of elements, of a matrix of one row
    // or one column, or the empty matrix, and npos for any other shape; and
    // a scalar computed in Type, stored so that the x87 rounds it to Type,
    // with the flags given.
    D_CONSTEXPR_VAR std::size_t expr_no_vector = static_cast<std::size_t>(-1);

    inline bool
    expr_matrix_passes(
        const value* _x,
        std::size_t  _count,
        value&       _out
    )
    {
        // an error passes, the first
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (_x[i].kind() == D_MATH_VALUE_ERROR)
            {
                _out = _x[i];

                return true;
            }
        }

        // what is no number and no matrix
        for (std::size_t i = 0; i < _count; ++i)
        {
            if ( (!_x[i].is_number()) &&
                 (!expr_matrix_held(_x[i].c_value())) )
            {
                _out = value(D_MATH_VALUE_ERROR_DOMAIN);

                return true;
            }
        }

        return false;
    }

    inline std::string
    expr_shape_text(
        const expr_shape& _shape
    )
    {
        return std::to_string(_shape.rows) + " by " +
               std::to_string(_shape.columns);
    }

    inline std::size_t
    expr_vector_length(
        const expr_shape& _shape
    )
    {
        // one row, or one column, or no element
        if ( (_shape.rows == 1) ||
             (_shape.columns == 1) ||
             ( (_shape.rows == 0) &&
               (_shape.columns == 0) ) )
        {
            return (_shape.rows * _shape.columns);
        }

        return expr_no_vector;
    }

    template<typename Type>
    value
    expr_rounded(
        Type     _x,
        unsigned _flags
    )
    {
        volatile Type stored = _x;

        return expr_flagged(value(static_cast<Type>(stored)),
                            _flags);
    }

    // 1.2
    // The square algebra
    //   functions: det(A), trace(A) and solve(A, B), A square; B of A's
    // rows, or a vector, a row or a column, of as many elements, solved as
    // a column and given in its own orientation.

    // expr_matrix_det_in
    //   function: det(A), of _n rows, in Type: LU with partial pivoting,
    // the parity times U's diagonal.
    template<typename Type>
    value
    expr_matrix_det_in(
        const value& _a,
        std::size_t  _n
    )
    {
        std::vector<Type>        lu   = expr_matrix_dense_of<Type>(_a);
        std::vector<std::size_t> perm(_n);
        Type                     sign = Type(1);

        static_cast<void>(d_decomp_lu(lu.data(),
                                      _n,
                                      perm.data(),
                                      &sign));

        return expr_rounded<Type>(d_decomp_lu_det(lu.data(),
                                                  _n,
                                                  sign),
                                  expr_matrix_flags_of(_a));
    }

    inline value
    expr_matrix_det(
        const value& _a
    )
    {
        value             out;
        const expr_shape  shape = ( expr_matrix_held(_a.c_value())
                                        ? expr_matrix_box_of(
                                              _a.c_value()).shape
                                        : expr_shape() );
        const std::size_t n     = shape.rows;

        // an error, or no operand; a quaternion, or a matrix holding one,
        // none (S277); a number, its own
        if (expr_matrix_passes(&_a,
                               1,
                               out))
        {
            return out;
        }

        if (expr_is_quaternion(_a))
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        if (!expr_matrix_held(_a.c_value()))
        {
            return _a;
        }

        // square, of no quaternion
        if ( (shape.rows != shape.columns) ||
             (expr_matrix_has_quaternion(_a)) )
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // by the type it is computed in
        switch (expr_matrix_algebra_storage(_a))
        {
            case expr_matrix_storage::floats:
                return expr_matrix_det_in<float>(_a,
                                                 n);

            case expr_matrix_storage::doubles:
                return expr_matrix_det_in<double>(_a,
                                                  n);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_det_in<long double>(_a,
                                                       n);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_det_complex<float>(_a,
                                                      n);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_det_complex<double>(_a,
                                                       n);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_det_complex<long double>(_a,
                                                            n);

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> x = expr_matrix_values_of(_a);
        std::vector<value>       scratch(n * n);

        // Bareiss's elimination, exact for exact elements
        return kernel::determinant(x.data(),
                                   scratch.data(),
                                   n);
    }

    // expr_matrix_inv
    //   function: inv(A): A's inverse, a pole where A is singular
    // (matrix_kind.hpp, 3.2.7); of a number, its reciprocal.
    inline value
    expr_matrix_inv(
        const value& _a
    )
    {
        value out;

        // an error, or no operand; a number, its reciprocal
        if (expr_matrix_passes(&_a,
                               1,
                               out))
        {
            return out;
        }

        return ( expr_matrix_held(_a.c_value())
                     ? expr_matrix_inverse(_a)
                     : (value(1) / _a) );
    }

    // expr_matrix_trace
    //   function: trace(A), the sum of A's diagonal, A square, by the
    // kernels' trace, in A's dense type or on its values.
    template<typename Type>
    value
    expr_matrix_trace_in(
        const value& _a,
        std::size_t  _n
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        const std::vector<Type> a = expr_matrix_dense_of<Type>(_a);

        return expr_rounded<Type>(kernel::trace(a.data(),
                                                _n),
                                  _a.flags());
    }

    inline value
    expr_matrix_trace(
        const value& _a
    )
    {
        value             out;
        const expr_shape  shape = ( expr_matrix_held(_a.c_value())
                                        ? expr_matrix_box_of(
                                              _a.c_value()).shape
                                        : expr_shape() );
        const std::size_t n     = shape.rows;

        // an error, or no operand; a number, its own
        if (expr_matrix_passes(&_a,
                               1,
                               out))
        {
            return out;
        }

        if (!expr_matrix_held(_a.c_value()))
        {
            return _a;
        }

        // square
        if (shape.rows != shape.columns)
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // by storage
        switch (expr_matrix_box_of(_a.c_value()).storage)
        {
            case expr_matrix_storage::floats:
                return expr_matrix_trace_in<float>(_a,
                                                   n);

            case expr_matrix_storage::doubles:
                return expr_matrix_trace_in<double>(_a,
                                                    n);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_trace_in<long double>(_a,
                                                         n);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_trace_complex<float>(_a,
                                                        n);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_trace_complex<double>(_a,
                                                         n);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_trace_complex<long double>(_a,
                                                              n);

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_trace_quaternion<float>(_a,
                                                           n);

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_trace_quaternion<double>(_a,
                                                            n);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_trace_quaternion<long double>(_a,
                                                                 n);

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> x = expr_matrix_values_of(_a);

        return kernel::trace(x.data(),
                             n);
    }

    // expr_matrix_solve_in
    //   function: X for A X = B, A of _n rows, B of _columns columns, its
    // elements row by row in _b's, in Type: A's LU with partial pivoting,
    // and each of B's columns solved by it; a pole for a singular A. X is of
    // _shape.
    template<typename Type>
    value
    expr_matrix_solve_in(
        const value&      _a,
        const value&      _b,
        std::size_t       _n,
        std::size_t       _columns,
        const expr_shape& _shape
    )
    {
        std::vector<Type>        lu = expr_matrix_dense_of<Type>(_a);
        const std::vector<Type>  b  = expr_matrix_dense_of<Type>(_b);
        std::vector<std::size_t> perm(_n);
        std::vector<Type>        column(_n);
        std::vector<Type>        x(_n);
        std::vector<Type>        out(_n * _columns);
        Type                     sign = Type(1);

        // singular: a pole
        if (d_decomp_lu(lu.data(),
                        _n,
                        perm.data(),
                        &sign))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        // each column of B
        for (std::size_t j = 0; j < _columns; ++j)
        {
            for (std::size_t i = 0; i < _n; ++i)
            {
                column[i] = b[(i * _columns) + j];
            }

            d_decomp_lu_solve(lu.data(),
                              perm.data(),
                              _n,
                              column.data(),
                              x.data());

            for (std::size_t i = 0; i < _n; ++i)
            {
                out[(i * _columns) + j] = x[i];
            }
        }

        return expr_matrix_of_dense<Type>(_shape,
                                          out,
                                          ( expr_matrix_flags_of(_a) |
                                            expr_matrix_flags_of(_b) ));
    }

    // expr_matrix_left_quotient
    //   function: a^-1 b of two numbers, on the left (S390): exactly where
    // both are exact, a's inverse times b; otherwise in the wider of their
    // floating-point types, each its nearest operand there, by the
    // quaternion kind's operands' left quotient; with both flags.
    template<typename Type>
    value
    expr_matrix_left_quotient_in(
        const value& _a,
        const value& _b,
        unsigned     _flags
    )
    {
        const expr_quaternion_operand<Type> a =
            expr_quaternion_operand_of<Type>(
                expr_quaternion_parts_of(_a.c_value()));
        const expr_quaternion_operand<Type> b =
            expr_quaternion_operand_of<Type>(
                expr_quaternion_parts_of(_b.c_value()));

        return expr_quaternion_operand_value(
            expr_quaternion_left_quotient(a, b),
            _flags);
    }

    inline value
    expr_matrix_left_quotient(
        const value& _a,
        const value& _b
    )
    {
        const value    operands[2] = { _a, _b };
        const unsigned flags       = (_a.flags() | _b.flags());

        // exactly
        if ( (_a.is_exact()) &&
             (_b.is_exact()) )
        {
            return ( (value(1) / _a) * _b );
        }

        // in the wider type
        switch (expr_quaternion_widest(operands, 2))
        {
            case D_MATH_VALUE_FLOAT:
                return expr_matrix_left_quotient_in<float>(_a, _b, flags);

            case D_MATH_VALUE_DOUBLE:
                return expr_matrix_left_quotient_in<double>(_a, _b, flags);

            default:
                break;
        }

        return expr_matrix_left_quotient_in<long double>(_a, _b, flags);
    }

    // expr_matrix_over_number
    //   function: solve(a, B) of a number a: B / a; or, where a is no real
    // number and a or B holds a quaternion, a^-1 B, on the left, as a X = B
    // asks, each element of a matrix B so (S390).
    inline value
    expr_matrix_over_number(
        const value& _a,
        const value& _b
    )
    {
        const bool b_matrix = expr_matrix_held(_b.c_value());
        const bool skew     = ( (!expr_complex_real_number(_a)) &&
                                ( (expr_is_quaternion(_a)) ||
                                  (expr_is_quaternion(_b)) ||
                                  ( (b_matrix) &&
                                    (expr_matrix_has_quaternion(_b)) ) ) );

        // B / a, where the order does not count
        if (!skew)
        {
            return (_b / _a);
        }

        // a^-1 b, or each element of B so
        if (!b_matrix)
        {
            return expr_matrix_left_quotient(_a,
                                             _b);
        }

        const std::vector<value> elements = expr_matrix_values_of(_b);
        std::vector<value>       out;

        out.reserve(elements.size());

        for (std::size_t i = 0; i < elements.size(); ++i)
        {
            out.push_back(expr_matrix_left_quotient(_a,
                                                    elements[i]));
        }

        return expr_matrix_of_values(expr_matrix_box_of(_b.c_value()).shape,
                                     out);
    }

    // expr_matrix_solve_skew
    //   function: X for A X = B, of values one of which holds a quaternion,
    // as skew elements: the elimination of [A | B], each product in its
    // order and each row divided on the left, exact for exact elements; a
    // pole for a singular A (S388).
    inline value
    expr_matrix_solve_skew(
        const value&      _a,
        const value&      _b,
        std::size_t       _n,
        std::size_t       _columns,
        const expr_shape& _shape
    )
    {
        typedef linalg::internal::linalg_kernel<expr_matrix_skew> kernel;

        const std::vector<expr_matrix_skew> a =
            expr_matrix_skewed(expr_matrix_values_of(_a));
        const std::vector<expr_matrix_skew> b =
            expr_matrix_skewed(expr_matrix_values_of(_b));
        std::vector<expr_matrix_skew>       scratch(_n * _n);
        std::vector<expr_matrix_skew>       solution(_n * _columns);

        // singular: a pole, by the generic kernel's elimination,
        // which its solve and solve_fast share (S406)
        if (!kernel::solve_fast(solution.data(),
                                scratch.data(),
                                a.data(),
                                b.data(),
                                _n,
                                _columns))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        return expr_matrix_of_values(_shape,
                                     expr_matrix_unskewed(solution));
    }

    // expr_matrix_solve_storage
    //   function: the storage solve(A, B) is computed in: the wider of A's
    // and B's, an exact one taking the other's; values where either is of
    // values and not exact; of quaternions where either holds one (S388),
    // else of complex numbers where either holds one (S351).
    inline expr_matrix_storage
    expr_matrix_solve_storage(
        const value& _a,
        const value& _b
    )
    {
        const expr_matrix_storage storage =
            expr_matrix_wider(expr_matrix_algebra_storage(_a),
                              expr_matrix_algebra_storage(_b));
        const bool                generic =
            ( ( (expr_matrix_algebra_storage(_a) ==
                     expr_matrix_storage::values) &&
                (!expr_matrix_all_exact(_a)) ) ||
              ( (expr_matrix_algebra_storage(_b) ==
                     expr_matrix_storage::values) &&
                (!expr_matrix_all_exact(_b)) ) );

        // an exact matrix takes the other's storage
        const expr_matrix_storage chosen =
            ( generic ? expr_matrix_storage::values
                      : ( (expr_matrix_all_exact(_a))
                              ? expr_matrix_algebra_storage(_b)
                              : ( (expr_matrix_all_exact(_b))
                                      ? expr_matrix_algebra_storage(_a)
                                      : storage ) ) );
        const bool                quaternion =
            ( (expr_matrix_has_quaternion(_a)) ||
              (expr_matrix_has_quaternion(_b)) );

        return ( (quaternion) ? expr_storage_quaternion_of(chosen)
               : ( (expr_matrix_has_complex(_a)) ||
                   (expr_matrix_has_complex(_b)) )
                     ? expr_storage_complex_of(chosen)
                     : chosen );
    }

    // expr_matrix_solve_values
    //   function: solve(A, B) on values, n by n and n by _columns: as skew
    // elements where either holds a quaternion (S388), else the generic
    // kernels' elimination of [A | B], exact for exact elements (S257); a
    // pole for a singular A.
    inline value
    expr_matrix_solve_values(
        const value&      _a,
        const value&      _b,
        std::size_t       _n,
        std::size_t       _columns,
        const expr_shape& _shape
    )
    {
        // values holding a quaternion: as skew elements
        if ( (expr_matrix_has_quaternion(_a)) ||
             (expr_matrix_has_quaternion(_b)) )
        {
            return expr_matrix_solve_skew(_a,
                                          _b,
                                          _n,
                                          _columns,
                                          _shape);
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> y = expr_matrix_values_of(_a);
        const std::vector<value> z = expr_matrix_values_of(_b);
        std::vector<value>       scratch(_n * _n);
        std::vector<value>       solution(_n * _columns);

        // a pole for a singular A, by the generic kernel's elimination,
        // which its solve and solve_fast share (S406)
        if (!kernel::solve_fast(solution.data(),
                                scratch.data(),
                                y.data(),
                                z.data(),
                                _n,
                                _columns))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        return expr_matrix_of_values(_shape,
                                     solution);
    }

    // expr_matrix_solve_as
    //   function: solve(A, B), n by n and n by _columns, computed in
    // _storage.
    inline value
    expr_matrix_solve_as(
        const value&        _a,
        const value&        _b,
        std::size_t         _n,
        std::size_t         _columns,
        const expr_shape&   _shape,
        expr_matrix_storage _storage
    )
    {
        switch (_storage)
        {
            case expr_matrix_storage::floats:
                return expr_matrix_solve_in<float>(_a,
                                                   _b,
                                                   _n,
                                                   _columns,
                                                   _shape);

            case expr_matrix_storage::doubles:
                return expr_matrix_solve_in<double>(_a,
                                                    _b,
                                                    _n,
                                                    _columns,
                                                    _shape);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_solve_in<long double>(_a,
                                                         _b,
                                                         _n,
                                                         _columns,
                                                         _shape);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_solve_complex<float>(_a,
                                                        _b,
                                                        _n,
                                                        _columns,
                                                        _shape);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_solve_complex<double>(_a,
                                                         _b,
                                                         _n,
                                                         _columns,
                                                         _shape);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_solve_complex<long double>(_a,
                                                              _b,
                                                              _n,
                                                              _columns,
                                                              _shape);

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_solve_quaternion<float>(_a,
                                                           _b,
                                                           _n,
                                                           _columns,
                                                           _shape);

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_solve_quaternion<double>(_a,
                                                            _b,
                                                            _n,
                                                            _columns,
                                                            _shape);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_solve_quaternion<long double>(_a,
                                                                 _b,
                                                                 _n,
                                                                 _columns,
                                                                 _shape);

            case expr_matrix_storage::values:
            default:
                break;
        }

        return expr_matrix_solve_values(_a,
                                        _b,
                                        _n,
                                        _columns,
                                        _shape);
    }

    // expr_matrix_solve
    //   function: solve(A, B): X for A X = B; of a number a, B / a, or a^-1
    // B where the order counts (expr_matrix_over_number).
    inline value
    expr_matrix_solve(
        const value& _a,
        const value& _b
    )
    {
        const value x[2] = { _a, _b };
        value       out;

        // an error, or no operand; a number a
        if (expr_matrix_passes(x,
                               2,
                               out))
        {
            return out;
        }

        if (!expr_matrix_held(_a.c_value()))
        {
            return expr_matrix_over_number(_a,
                                           _b);
        }

        // A square, B a matrix of its rows, or a vector of as many elements
        if (!expr_matrix_held(_b.c_value()))
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        const expr_shape  a = expr_matrix_box_of(_a.c_value()).shape;
        const expr_shape  b = expr_matrix_box_of(_b.c_value()).shape;
        const std::size_t n = a.rows;

        if ( (a.rows != a.columns) ||
             ( (b.rows != n) &&
               (expr_vector_length(b) != n) ) )
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // B's columns: its own, or one, a row solved as a column
        const std::size_t columns = ( (b.rows == n) ? b.columns : 1 );
        const expr_shape  shape   = ( (b.rows == n) ? expr_shape(n,
                                                                 columns)
                                                    : b );
        return expr_matrix_solve_as(_a,
                                    _b,
                                    n,
                                    columns,
                                    shape,
                                    expr_matrix_solve_storage(_a,
                                                              _b));
    }

    // 1.3
    // Norms and products
    //   functions: a value's square root as the tower takes it, exact
    // where its value is a rational square (value.hpp) and otherwise in its
    // own floating-point type, a long double for an exact or a boxed one;
    // norm(A), the root of the sum of the squares of A's elements, by the
    // kernels' norm in a dense type, else the root of their sum on values,
    // exact for exact elements; where that sum, in floating point, passes
    // its type's range or falls below the precision of its normal numbers,
    // of A scaled exactly by a power of two to a largest magnitude in
    // [1, 2), the norm scaled back, as LAPACK's dnrm2 and Julia's norm
    // scale, so that a norm the type holds is never an infinity or 0 for a
    // square that overflowed or underflowed (S256); dot(a, b), two vectors
    // of one length, row or column, their elements' products summed;
    // cross(a, b), two vectors of 3 elements, their vector product, of a's
    // shape.
    inline value
    expr_matrix_root(
        const value& _x
    )
    {
        float       f = 0.0f;
        double      d = 0.0;
        long double l = 0.0L;

        // by kind: an inline floating-point one in its own type
        switch (_x.kind())
        {
            case D_MATH_VALUE_FLOAT:
                static_cast<void>(_x.to(f));

                return expr_rounded<float>(d_math_sqrt(f),
                                           _x.flags());

            case D_MATH_VALUE_DOUBLE:
                static_cast<void>(_x.to(d));

                return expr_rounded<double>(d_math_sqrt(d),
                                            _x.flags());

            case D_MATH_VALUE_LONG_DOUBLE:
                static_cast<void>(_x.to(l));

                return expr_rounded<long double>(d_math_sqrt(l),
                                                 _x.flags());

            default:
                break;
        }

        return value_root_of(_x);
    }

    // expr_norm_in_range
    //   function: whether a norm in Type is in the range its sum of
    // squares had no square overflow or lose its precision to underflow:
    // at most Type's largest value, and at least the root of its least
    // normal value over its epsilon.
    template<typename Type>
    bool
    expr_norm_in_range(
        Type _norm
    )
    {
        const Type small = std::sqrt(std::numeric_limits<Type>::min() /
                                     std::numeric_limits<Type>::epsilon());

        return ( (_norm <= std::numeric_limits<Type>::max()) &&
                 (!(_norm < small)) );
    }

    // expr_norm_scaled
    //   function: the Euclidean norm of _x in Type, the kernels': the C
    // core's d_vec_norm, which scales by a power of two where the sum of
    // squares leaves the range, as this layer did around it until chunk 38
    // (S256), the same bits.
    template<typename Type>
    Type
    expr_norm_scaled(
        const std::vector<Type>& _x
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        return kernel::norm(_x.data(),
                            _x.size());
    }

    template<typename Type>
    value
    expr_matrix_norm_in(
        const value& _a
    )
    {
        return expr_rounded<Type>(
            expr_norm_scaled<Type>(expr_matrix_dense_of<Type>(_a)),
            _a.flags());
    }

    // expr_matrix_parts_of
    //   function: a matrix's elements' parts, in Type, each element's real
    // part and then its imaginary part, a real one's 0: so that the norm of
    // a matrix of complex numbers, the root of the sum of their moduli's
    // squares, is the scaled norm of its parts (S351).
    template<typename Type>
    std::vector<Type>
    expr_matrix_parts_of(
        const value& _a
    )
    {
        const std::vector<math::complex<Type> > z =
            expr_matrix_dense_of<math::complex<Type> >(_a);
        std::vector<Type>                       parts;

        parts.reserve(2 * z.size());

        // each element's two parts
        for (std::size_t i = 0; i < z.size(); ++i)
        {
            parts.push_back(z[i].real());
            parts.push_back(z[i].imag());
        }

        return parts;
    }

    template<typename Type>
    value
    expr_matrix_norm_complex(
        const value& _a
    )
    {
        return expr_rounded<Type>(
            expr_norm_scaled<Type>(expr_matrix_parts_of<Type>(_a)),
            _a.flags());
    }

    // expr_matrix_quaternion_parts_of
    //   function: a matrix's elements' parts, in Type, each element's w, x,
    // y and z, a real or complex one's missing ones 0: so that the norm of a
    // matrix of quaternions, the root of the sum of their norms' squares,
    // is the scaled norm of its parts (S388).
    template<typename Type>
    std::vector<Type>
    expr_matrix_quaternion_parts_of(
        const value& _a
    )
    {
        const std::vector<math::quaternion<Type> > q =
            expr_matrix_dense_of<math::quaternion<Type> >(_a);
        std::vector<Type>                          parts;

        parts.reserve(4 * q.size());

        // each element's four parts
        for (std::size_t i = 0; i < q.size(); ++i)
        {
            parts.push_back(q[i].m_w);
            parts.push_back(q[i].m_x);
            parts.push_back(q[i].m_y);
            parts.push_back(q[i].m_z);
        }

        return parts;
    }

    template<typename Type>
    value
    expr_matrix_norm_quaternion(
        const value& _a
    )
    {
        return expr_rounded<Type>(
            expr_norm_scaled<Type>(expr_matrix_quaternion_parts_of<Type>(_a)),
            _a.flags());
    }

    // expr_matrix_moduli_squared
    //   function: the sum, on values, of a matrix's elements' squared moduli,
    // w^2 + x^2 + y^2 + z^2 of a quaternion, re^2 + im^2 of a complex one
    // and x^2 of a real one, from 0 in order: exact for exact elements
    // (S351, S388).
    inline value
    expr_matrix_moduli_squared(
        const std::vector<value>& _x
    )
    {
        value sum(0);

        // element by element
        for (std::size_t i = 0; i < _x.size(); ++i)
        {
            if (expr_is_quaternion(_x[i]))
            {
                const expr_quaternion_parts_held q =
                    expr_quaternion_parts_of(_x[i].c_value());

                sum = (sum + ( ( (q.w * q.w) + (q.x * q.x) ) +
                               ( (q.y * q.y) + (q.z * q.z) ) ));
            }
            else if (expr_is_complex(_x[i]))
            {
                const value re = expr_real(_x[i]);
                const value im = expr_imag(_x[i]);

                sum = (sum + ((re * re) + (im * im)));
            }
            else
            {
                sum = (sum + (_x[i] * _x[i]));
            }
        }

        return sum;
    }

    // expr_norm_rescued
    //   function: _norm, a norm of values in Type, or, out of
    // expr_norm_in_range's range, A's norm scaled, in Type, each element its
    // nearest there, with _norm's flags.
    template<typename Type>
    value
    expr_norm_rescued(
        const value& _a,
        const value& _norm
    )
    {
        Type norm = Type(0);

        static_cast<void>(_norm.to(norm));

        return ( expr_norm_in_range<Type>(norm)
                     ? _norm
                     : expr_rounded<Type>(
                           expr_norm_scaled<Type>(
                               ( expr_matrix_has_quaternion(_a)
                                     ? expr_matrix_quaternion_parts_of<Type>(
                                           _a)
                               : expr_matrix_has_complex(_a)
                                     ? expr_matrix_parts_of<Type>(_a)
                                     : expr_matrix_dense_of<Type>(_a) )),
                           _norm.flags()) );
    }

    inline value
    expr_matrix_norm(
        const value& _a
    )
    {
        value out;

        // an error, or no operand; a number, its magnitude
        if (expr_matrix_passes(&_a,
                               1,
                               out))
        {
            return out;
        }

        if (!expr_matrix_held(_a.c_value()))
        {
            return abs(_a);
        }

        // by storage
        switch (expr_matrix_box_of(_a.c_value()).storage)
        {
            case expr_matrix_storage::floats:
                return expr_matrix_norm_in<float>(_a);

            case expr_matrix_storage::doubles:
                return expr_matrix_norm_in<double>(_a);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_norm_in<long double>(_a);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_norm_complex<float>(_a);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_norm_complex<double>(_a);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_norm_complex<long double>(_a);

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_norm_quaternion<float>(_a);

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_norm_quaternion<double>(_a);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_norm_quaternion<long double>(_a);

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> x    = expr_matrix_values_of(_a);
        const value              norm = expr_matrix_root(
            ( (expr_matrix_has_complex(_a)) ||
              (expr_matrix_has_quaternion(_a)) )
                ? expr_matrix_moduli_squared(x)
                : kernel::norm_squared(x.data(),
                                       x.size()));

        // in floating point, scaled where a square overflowed or underflowed
        switch (norm.kind())
        {
            case D_MATH_VALUE_FLOAT:
                return expr_norm_rescued<float>(_a,
                                                norm);

            case D_MATH_VALUE_DOUBLE:
                return expr_norm_rescued<double>(_a,
                                                 norm);

            case D_MATH_VALUE_LONG_DOUBLE:
                return expr_norm_rescued<long double>(_a,
                                                      norm);

            default:
                break;
        }

        return norm;
    }

    template<typename Type>
    value
    expr_matrix_dot_in(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        unsigned               _flags
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        const std::vector<Type> a = expr_matrix_dense_as<Type>(_a);
        const std::vector<Type> b = expr_matrix_dense_as<Type>(_b);

        return expr_rounded<Type>(kernel::dot(a.data(),
                                              b.data(),
                                              a.size()),
                                  _flags);
    }

    // expr_matrix_dot_complex
    //   function: dot(a, b) of dense vectors, one complex at least, in
    // math::complex<Type>: the sum from 0, in order, of conj(a_i) b_i, a
    // real element taken as itself, its conjugate itself (S351), as
    // Julia's dot, NumPy's vdot and BLAS's zdotc conjugate a.
    template<typename Type>
    const Type&
    expr_matrix_conjugate_of(
        const Type& _x
    )
    {
        return _x;
    }

    template<typename Type>
    math::complex<Type>
    expr_matrix_conjugate_of(
        const math::complex<Type>& _z
    )
    {
        return conj(_z);
    }

    template<typename Type,
             typename X,
             typename Y>
    math::complex<Type>
    expr_matrix_conjugate_dot(
        const std::vector<X>& _a,
        const std::vector<Y>& _b
    )
    {
        math::complex<Type> sum;

        // in order
        for (std::size_t i = 0; i < _a.size(); ++i)
        {
            sum += (expr_matrix_conjugate_of(_a[i]) * _b[i]);
        }

        return sum;
    }

    template<typename Type>
    value
    expr_matrix_dot_complex(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        unsigned               _flags
    )
    {
        const expr_matrix_side<Type> a = expr_matrix_side_of<Type>(_a);
        const expr_matrix_side<Type> b = expr_matrix_side_of<Type>(_b);

        // each side as it is held
        return expr_complex_flagged_of(
            ( (a.complex)
                  ? ( (b.complex)
                          ? expr_matrix_conjugate_dot<Type>(a.z, b.z)
                          : expr_matrix_conjugate_dot<Type>(a.z, b.real) )
                  : expr_matrix_conjugate_dot<Type>(a.real, b.z) ),
            _flags);
    }

    // expr_matrix_dot_quaternion
    //   function: dot(a, b) of dense vectors, one of quaternions at least,
    // in Type: the sum from 0, in order, of conj(a_i) b_i, on the quaternion
    // kind's operands, each product in that order (S388).
    template<typename Type>
    value
    expr_matrix_dot_quaternion(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        unsigned               _flags
    )
    {
        typedef expr_quaternion_operand<Type> operand;

        const std::vector<operand> a   = expr_matrix_operands_as<Type>(_a);
        const std::vector<operand> b   = expr_matrix_operands_as<Type>(_b);
        operand                    sum = number_traits<operand>::zero();

        // in order
        for (std::size_t i = 0; i < a.size(); ++i)
        {
            sum += (expr_quaternion_operand_conj(a[i]) * b[i]);
        }

        return expr_quaternion_dense(sum.q,
                                     _flags);
    }

    // expr_matrix_conjugate_value
    //   function: a number's conjugate as dot takes it: a complex number's
    // or a quaternion's, and a real one itself.
    inline value
    expr_matrix_conjugate_value(
        const value& _x
    )
    {
        return ( expr_is_complex(_x)    ? expr_conj(_x)
               : expr_is_quaternion(_x) ? expr_quaternion_conj(_x)
                                        : _x );
    }

    // expr_matrix_dot_values
    //   function: dot(a, b) on values: the sum from 0, in order, of
    // conj(a_i) b_i, exact for exact elements.
    inline value
    expr_matrix_dot_values(
        const std::vector<value>& _a,
        const std::vector<value>& _b
    )
    {
        value sum(0);

        // in order
        for (std::size_t i = 0; i < _a.size(); ++i)
        {
            sum = (sum + (expr_matrix_conjugate_value(_a[i]) * _b[i]));
        }

        return sum;
    }

    // expr_matrix_dot_as
    //   function: dot(a, b) of two vectors of _length elements, by the type
    // it is computed in, the wider of the two's storages; on values, conj(a_i)
    // b_i where either holds a complex number or a quaternion, else the
    // kernels'.
    inline value
    expr_matrix_dot_as(
        const value& _a,
        const value& _b,
        std::size_t  _length
    )
    {
        const expr_matrix_box& a = expr_matrix_box_of(_a.c_value());
        const expr_matrix_box& b = expr_matrix_box_of(_b.c_value());
        const unsigned flags = (_a.flags() | _b.flags());

        // by the type it is computed in
        switch (expr_matrix_wider(a.storage,
                                  b.storage))
        {
            case expr_matrix_storage::floats:
                return expr_matrix_dot_in<float>(a,
                                                 b,
                                                 flags);

            case expr_matrix_storage::doubles:
                return expr_matrix_dot_in<double>(a,
                                                  b,
                                                  flags);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_dot_in<long double>(a,
                                                       b,
                                                       flags);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_dot_complex<float>(a,
                                                      b,
                                                      flags);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_dot_complex<double>(a,
                                                       b,
                                                       flags);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_dot_complex<long double>(a,
                                                            b,
                                                            flags);

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_dot_quaternion<float>(a,
                                                         b,
                                                         flags);

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_dot_quaternion<double>(a,
                                                          b,
                                                          flags);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_dot_quaternion<long double>(a,
                                                               b,
                                                               flags);

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> y = expr_matrix_values_of(_a);
        const std::vector<value> z = expr_matrix_values_of(_b);

        // of complex numbers or quaternions: conj(a_i) b_i; of real ones,
        // the kernels'
        if ( (expr_matrix_has_complex(_a)) ||
             (expr_matrix_has_complex(_b)) ||
             (expr_matrix_has_quaternion(_a)) ||
             (expr_matrix_has_quaternion(_b)) )
        {
            return expr_matrix_dot_values(y,
                                          z);
        }

        return kernel::dot(y.data(),
                           z.data(),
                           _length);
    }

    inline value
    expr_matrix_dot(
        const value& _a,
        const value& _b
    )
    {
        const value x[2] = { _a, _b };
        value       out;

        // an error, or no operand; two numbers, their product
        if (expr_matrix_passes(x,
                               2,
                               out))
        {
            return out;
        }

        const bool a_matrix = expr_matrix_held(_a.c_value());
        const bool b_matrix = expr_matrix_held(_b.c_value());

        if ( (!a_matrix) &&
             (!b_matrix) )
        {
            return (expr_matrix_conjugate_value(_a) * _b);
        }

        // two vectors of one length
        if ( (!a_matrix) ||
             (!b_matrix) )
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        const expr_matrix_box& a      = expr_matrix_box_of(_a.c_value());
        const expr_matrix_box& b      = expr_matrix_box_of(_b.c_value());
        const std::size_t      length = expr_vector_length(a.shape);

        if ( (length == expr_no_vector) ||
             (expr_vector_length(b.shape) != length) )
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        return expr_matrix_dot_as(_a,
                                  _b,
                                  length);
    }

    template<typename Type>
    value
    expr_matrix_cross_in(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        unsigned               _flags
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        const std::vector<Type> a = expr_matrix_dense_as<Type>(_a);
        const std::vector<Type> b = expr_matrix_dense_as<Type>(_b);
        std::vector<Type>       out(3);

        kernel::cross(out.data(),
                      a.data(),
                      b.data());

        return expr_matrix_of_dense<Type>(_a.shape,
                                          out,
                                          _flags);
    }

    // expr_matrix_cross_complex
    //   function: cross(a, b) of dense 3-vectors, one complex at least, in
    // math::complex<Type>, nothing conjugated (S273): a_2 b_3 - a_3 b_2,
    // a_3 b_1 - a_1 b_3, a_1 b_2 - a_2 b_1, a real element taken as itself.
    template<typename Type,
             typename X,
             typename Y>
    void
    expr_matrix_crossed(
        std::vector<math::complex<Type> >& _out,
        const std::vector<X>&              _a,
        const std::vector<Y>&              _b
    )
    {
        _out[0] = math::complex<Type>((_a[1] * _b[2]) - (_a[2] * _b[1]));
        _out[1] = math::complex<Type>((_a[2] * _b[0]) - (_a[0] * _b[2]));
        _out[2] = math::complex<Type>((_a[0] * _b[1]) - (_a[1] * _b[0]));

        return;
    }

    template<typename Type>
    value
    expr_matrix_cross_complex(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        unsigned               _flags
    )
    {
        const expr_matrix_side<Type>      a = expr_matrix_side_of<Type>(_a);
        const expr_matrix_side<Type>      b = expr_matrix_side_of<Type>(_b);
        std::vector<math::complex<Type> > out(3);

        // each side as it is held
        if ( (a.complex) &&
             (b.complex) )
        {
            expr_matrix_crossed<Type>(out, a.z, b.z);
        }
        else if (a.complex)
        {
            expr_matrix_crossed<Type>(out, a.z, b.real);
        }
        else
        {
            expr_matrix_crossed<Type>(out, a.real, b.z);
        }

        return expr_matrix_of_dense<math::complex<Type> >(_a.shape,
                                                          out,
                                                          _flags);
    }

    // expr_matrix_cross_quaternion
    //   function: cross(a, b) of dense 3-vectors, one of quaternions at
    // least, in Type, nothing conjugated: the kernels' a_2 b_3 - a_3 b_2,
    // a_3 b_1 - a_1 b_3, a_1 b_2 - a_2 b_1 on the quaternion kind's operands,
    // each product in that order, a's element on the left (S388).
    template<typename Type>
    value
    expr_matrix_cross_quaternion(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        unsigned               _flags
    )
    {
        typedef expr_quaternion_operand<Type>            operand;
        typedef linalg::internal::linalg_kernel<operand> kernel;

        const std::vector<operand> a = expr_matrix_operands_as<Type>(_a);
        const std::vector<operand> b = expr_matrix_operands_as<Type>(_b);
        std::vector<operand>       out(3);

        kernel::cross(out.data(),
                      a.data(),
                      b.data());

        return expr_matrix_of_operands<Type>(_a.shape,
                                             out,
                                             _flags);
    }

    inline value
    expr_matrix_cross(
        const value& _a,
        const value& _b
    )
    {
        const value x[2] = { _a, _b };
        value       out;

        // an error, or no operand
        if (expr_matrix_passes(x,
                               2,
                               out))
        {
            return out;
        }

        // two vectors of 3 elements
        if ( (!expr_matrix_held(_a.c_value())) ||
             (!expr_matrix_held(_b.c_value())) ||
             (expr_vector_length(
                  expr_matrix_box_of(_a.c_value()).shape) != 3) ||
             (expr_vector_length(
                  expr_matrix_box_of(_b.c_value()).shape) != 3) )
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        const expr_matrix_box& a     = expr_matrix_box_of(_a.c_value());
        const expr_matrix_box& b     = expr_matrix_box_of(_b.c_value());
        const unsigned         flags = (_a.flags() | _b.flags());

        // by the type it is computed in
        switch (expr_matrix_wider(a.storage,
                                  b.storage))
        {
            case expr_matrix_storage::floats:
                return expr_matrix_cross_in<float>(a,
                                                   b,
                                                   flags);

            case expr_matrix_storage::doubles:
                return expr_matrix_cross_in<double>(a,
                                                    b,
                                                    flags);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_cross_in<long double>(a,
                                                         b,
                                                         flags);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_cross_complex<float>(a,
                                                        b,
                                                        flags);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_cross_complex<double>(a,
                                                         b,
                                                         flags);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_cross_complex<long double>(a,
                                                              b,
                                                              flags);

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_cross_quaternion<float>(a,
                                                           b,
                                                           flags);

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_cross_quaternion<double>(a,
                                                            b,
                                                            flags);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_cross_quaternion<long double>(a,
                                                                 b,
                                                                 flags);

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> y = expr_matrix_values_of(_a);
        const std::vector<value> z = expr_matrix_values_of(_b);
        std::vector<value>       product(3);

        kernel::cross(product.data(),
                      y.data(),
                      z.data());

        return expr_matrix_of_values(a.shape,
                                     product);
    }

    // 1.4
    // Eigenvalues
    //   functions: eig(A), A square: every eigenvalue, as a vector of one
    // row (S258, S352). In A's algebra type: a real symmetric A's by the
    // cyclic Jacobi method, on A scaled exactly by a power of two to a
    // largest magnitude in [1, 2), to its Frobenius norm times the type's
    // epsilon, and any other real A's by Francis's double-shift QR, the C
    // core's, ascending where each is real, and otherwise every one, complex,
    // by decomp.h's d_decomp_eigen; a Hermitian A's real, by cmat.h's
    // Jacobi method with complex rotations, and any other complex A's by
    // cmat.h's d_cmat_eigen, ascending by real part, then by imaginary
    // part; NaNs, of A's type, where A holds a NaN or an infinity; and a
    // domain error where they are not each found, the solvers given
    // expr_eigen_budget's steps. An exact A's are found so in long double,
    // and each made exact where it is a Gaussian rational: A's
    // characteristic polynomial, Faddeev and LeVerrier's, exactly; each
    // eigenvalue's nearest number of parts that are multiples of 1/d, d the
    // least common multiple of A's parts' denominators, which every
    // Gaussian rational eigenvalue is; and each such one a root, the
    // polynomial divided by it as often as it divides, so that a repeated
    // eigenvalue is found as often as it is repeated.

    // expr_eigen_budget
    //   function: the QR steps an _n by _n matrix's eigenvalues are given in
    // all: 30 a row, some ten times what Francis's QR takes, and at least
    // the 1000 every matrix was given until chunk 43, under which one of
    // 700 rows ran out (S352). A matrix that converges within either takes
    // the same steps, so the same bits.
    D_CONSTEXPR inline std::size_t
    expr_eigen_budget(
        std::size_t _n
    ) D_NOEXCEPT
    {
        return ( ((30u * _n) > 1000u) ? (30u * _n)
                                      : static_cast<std::size_t>(1000u) );
    }

    // expr_matrix_eigenvalues_in
    //   function: the eigenvalues of the _n by _n _a, in Type, ascending,
    // into _values; whether they are all found real.
    template<typename Type>
    bool
    expr_matrix_eigenvalues_in(
        std::vector<Type>  _a,
        std::size_t        _n,
        std::vector<Type>& _values
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        bool symmetric = true;
        Type largest   = Type(0);

        _values.assign(_n,
                       Type(0));

        // a NaN or an infinity: NaNs
        for (std::size_t i = 0; i < _a.size(); ++i)
        {
            const Type magnitude = ( (_a[i] < Type(0)) ? -_a[i] : _a[i] );

            if (!(magnitude <= std::numeric_limits<Type>::max()))
            {
                _values.assign(_n,
                               std::numeric_limits<Type>::quiet_NaN());

                return true;
            }

            largest = ( (magnitude > largest) ? magnitude : largest );
        }

        // symmetric: each pair equal
        for (std::size_t i = 0; i < _n; ++i)
        {
            for (std::size_t j = i + 1; j < _n; ++j)
            {
                symmetric = ( (symmetric) &&
                              (_a[(i * _n) + j] == _a[(j * _n) + i]) );
            }
        }

        // any other: Hessenberg QR, which scales A itself
        if (!symmetric)
        {
            return d_decomp_eigenvalues(_a.data(),
                                        _n,
                                        _values.data(),
                                        expr_eigen_budget(_n),
                                        Type(0));
        }

        int  exponent = 0;
        Type scale    = Type(1);

        // the zero matrix's are its zeros
        if (largest == Type(0))
        {
            return true;
        }

        static_cast<void>(std::frexp(largest,
                                     &exponent));
        scale = std::ldexp(Type(1),
                           1 - exponent);

        for (std::size_t i = 0; i < _a.size(); ++i)
        {
            _a[i] *= scale;
        }

        std::vector<Type> vectors(_n * _n);
        const Type        threshold =
            ( std::numeric_limits<Type>::epsilon() *
              kernel::norm(_a.data(),
                           _a.size()) );

        kernel::identity(vectors.data(),
                         _n);
        d_decomp_sym_eigen(_a.data(),
                           _values.data(),
                           vectors.data(),
                           _n,
                           threshold);

        for (std::size_t i = 0; i < _n; ++i)
        {
            _values[i] = std::ldexp(_values[i],
                                    exponent - 1);
        }

        return true;
    }

    // expr_matrix_spectrum_in
    //   function: every eigenvalue of the real _n by _n _a, in
    // math::complex<Type>, into _values, as decomp.h's d_decomp_eigen
    // finds them: ascending by real part, then by imaginary part, a pair
    // exactly conjugate, a real one's imaginary part +0 (S273); whether
    // each was found.
    template<typename Type>
    bool
    expr_matrix_spectrum_in(
        std::vector<Type>                  _a,
        std::size_t                        _n,
        std::vector<math::complex<Type> >& _values
    )
    {
        typedef typename math::complex<Type>::core_type core_type;

        std::vector<core_type> found(_n);
        std::vector<Type>      work(_n + 1);
        const bool             all = d_decomp_eigen(_a.data(),
                                                    _n,
                                                    found.data(),
                                                    D_NULLPTR,
                                                    work.data(),
                                                    D_NULLPTR,
                                                    expr_eigen_budget(_n),
                                                    Type(0));

        _values = expr_matrix_of_cores<Type>(found);

        return all;
    }

    // expr_matrix_complex_spectrum_in
    //   function: every eigenvalue of the complex _n by _n _a, in
    // math::complex<Type>, into _values: a Hermitian A's, A^H = A exactly,
    // by cmat.h's cyclic Jacobi method with complex rotations, on A scaled
    // exactly by a power of two to a largest part in [1, 2), to its
    // Frobenius norm times Type's epsilon, real and ascending, into _real,
    // with _hermitian set; any other A's by cmat.h's d_cmat_eigen, LAPACK's
    // zgeev's path, ascending by real part, then by imaginary part (S273,
    // S352); NaNs where A holds a NaN or an infinity. Whether each was
    // found.
    template<typename Type>
    bool
    expr_matrix_complex_spectrum_in(
        const std::vector<math::complex<Type> >& _a,
        std::size_t                              _n,
        std::vector<math::complex<Type> >&       _values,
        std::vector<Type>&                       _real,
        bool&                                    _hermitian
    )
    {
        typedef typename math::complex<Type>::core_type core_type;

        Type largest = Type(0);

        _hermitian = true;
        _values.assign(_n, math::complex<Type>());

        // a NaN or an infinity: NaNs
        for (std::size_t i = 0; i < _a.size(); ++i)
        {
            const Type re = std::fabs(_a[i].real());
            const Type im = std::fabs(_a[i].imag());

            if ( (!(re <= std::numeric_limits<Type>::max())) ||
                 (!(im <= std::numeric_limits<Type>::max())) )
            {
                const Type nan = std::numeric_limits<Type>::quiet_NaN();

                _hermitian = false;
                _values.assign(_n, math::complex<Type>(nan, nan));

                return true;
            }

            largest = ( (re > largest) ? re : largest );
            largest = ( (im > largest) ? im : largest );
        }

        // Hermitian: each pair the other's conjugate, the diagonal real
        for (std::size_t i = 0; i < _n; ++i)
        {
            for (std::size_t j = i; j < _n; ++j)
            {
                const math::complex<Type>& x = _a[(i * _n) + j];
                const math::complex<Type>& y = _a[(j * _n) + i];

                _hermitian = ( (_hermitian) &&
                               (x.real() == y.real()) &&
                               (x.imag() == -y.imag()) );
            }
        }

        std::vector<core_type> a = expr_matrix_cores<Type>(_a);

        // any other: cmat.h's general path, which scales A itself
        if (!_hermitian)
        {
            std::vector<core_type> found(_n);
            std::vector<Type>      rwork((2 * _n) + 1);
            const bool             all = d_cmat_eigen(a.data(),
                                                      _n,
                                                      found.data(),
                                                      D_NULLPTR,
                                                      D_NULLPTR,
                                                      rwork.data(),
                                                      D_NULLPTR,
                                                      expr_eigen_budget(_n),
                                                      Type(0));

            _values = expr_matrix_of_cores<Type>(found);

            return all;
        }

        _real.assign(_n,
                     Type(0));

        // the zero matrix's are its zeros
        if (largest == Type(0))
        {
            return true;
        }

        int  exponent = 0;
        Type scale    = Type(1);

        static_cast<void>(std::frexp(largest,
                                     &exponent));
        scale = std::ldexp(Type(1),
                           1 - exponent);

        std::vector<Type> parts;

        // scaled exactly, and its parts for the norm
        for (std::size_t i = 0; i < a.size(); ++i)
        {
            a[i].part[0] *= scale;
            a[i].part[1] *= scale;
            parts.push_back(a[i].part[0]);
            parts.push_back(a[i].part[1]);
        }

        std::vector<core_type> vectors(_n * _n);
        const Type             threshold =
            ( std::numeric_limits<Type>::epsilon() *
              expr_norm_scaled<Type>(parts) );

        // the identity, to receive the eigenvectors
        for (std::size_t i = 0; i < _n; ++i)
        {
            for (std::size_t j = 0; j < _n; ++j)
            {
                vectors[(i * _n) + j].part[0] = ( (i == j) ? Type(1)
                                                           : Type(0) );
                vectors[(i * _n) + j].part[1] = Type(0);
            }
        }

        d_cmat_hermitian_eigen(a.data(),
                               _real.data(),
                               vectors.data(),
                               _n,
                               threshold);

        // back to A's scale, exactly
        for (std::size_t i = 0; i < _n; ++i)
        {
            _real[i] = std::ldexp(_real[i],
                                  exponent - 1);
        }

        return true;
    }

    template<typename Type>
    value
    expr_matrix_eig_in(
        const value& _a,
        std::size_t  _n
    )
    {
        std::vector<Type> values;

        // not all real: every eigenvalue, complex throughout (S352)
        if (!expr_matrix_eigenvalues_in<Type>(expr_matrix_dense_of<Type>(_a),
                                              _n,
                                              values))
        {
            std::vector<math::complex<Type> > spectrum;

            // not each found: a domain error
            if (!expr_matrix_spectrum_in<Type>(expr_matrix_dense_of<Type>(_a),
                                               _n,
                                               spectrum))
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            return expr_matrix_of_dense<math::complex<Type> >(
                expr_shape(1,
                           _n),
                spectrum,
                expr_matrix_flags_of(_a));
        }

        return expr_matrix_of_dense<Type>(expr_shape(1,
                                                     _n),
                                          values,
                                          expr_matrix_flags_of(_a));
    }

    // expr_matrix_eig_complex
    //   function: eig(A) of a complex A, in math::complex<Type>: a
    // Hermitian one's real, of Type, any other's complex (S352); a domain
    // error where they are not each found.
    template<typename Type>
    value
    expr_matrix_eig_complex(
        const value& _a,
        std::size_t  _n
    )
    {
        std::vector<math::complex<Type> > spectrum;
        std::vector<Type>                 real;
        bool                              hermitian = false;

        // not each found: a domain error
        if (!expr_matrix_complex_spectrum_in<Type>(
                 expr_matrix_dense_of<math::complex<Type> >(_a),
                 _n,
                 spectrum,
                 real,
                 hermitian))
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // Hermitian: real
        if (hermitian)
        {
            return expr_matrix_of_dense<Type>(expr_shape(1,
                                                         _n),
                                              real,
                                              expr_matrix_flags_of(_a));
        }

        return expr_matrix_of_dense<math::complex<Type> >(
            expr_shape(1,
                       _n),
            spectrum,
            expr_matrix_flags_of(_a));
    }

    // expr_char_poly
    //   function: the characteristic polynomial of the _n by _n _a, det(x I
    // - A), its coefficients from x^0's to x^n's, into _c, by Faddeev and
    // LeVerrier: M_1 = I, c_(n - k) = -trace(A M_k) / k, M_(k + 1) = A M_k
    // + c_(n - k) I; whether each is exact.
    inline bool
    expr_char_poly(
        const std::vector<value>& _a,
        std::size_t               _n,
        std::vector<value>&       _c
    )
    {
        typedef linalg::internal::linalg_kernel<value> kernel;

        std::vector<value> m(_n * _n);
        std::vector<value> am(_n * _n);

        _c.assign(_n + 1,
                  value(0));
        _c[_n] = value(1);
        kernel::identity(m.data(),
                         _n);

        // each coefficient below x^n's
        for (std::size_t k = 1; k <= _n; ++k)
        {
            kernel::multiply(am.data(),
                             _a.data(),
                             m.data(),
                             _n,
                             _n,
                             _n);

            const value trace = kernel::trace(am.data(),
                                              _n);

            _c[_n - k] = ( value(0) -
                           ( trace /
                             value(static_cast<dj_math_imax>(k)) ) );

            // an inexact coefficient, the exact arithmetic past its range
            if (!_c[_n - k].is_exact())
            {
                return false;
            }

            m = am;

            for (std::size_t i = 0; i < _n; ++i)
            {
                m[(i * _n) + i] = m[(i * _n) + i] + _c[_n - k];
            }
        }

        return true;
    }

    // expr_poly_at, expr_poly_divided
    //   functions: the polynomial _c at _x, by Horner's rule; and _c
    // divided by x - _x, of which _x is a root, by synthetic division.
    inline value
    expr_poly_at(
        const std::vector<value>& _c,
        const value&              _x
    )
    {
        value at = _c.back();

        // from x^(n - 1)'s down
        for (std::size_t j = _c.size() - 1; j > 0; --j)
        {
            at = (at * _x) + _c[j - 1];
        }

        return at;
    }

    inline std::vector<value>
    expr_poly_divided(
        const std::vector<value>& _c,
        const value&              _x
    )
    {
        const std::size_t  degree = (_c.size() - 1);
        std::vector<value> quotient(degree);

        quotient[degree - 1] = _c[degree];

        // from x^(n - 2)'s down
        for (std::size_t j = degree - 1; j > 0; --j)
        {
            quotient[j - 1] = _c[j] + (_x * quotient[j]);
        }

        return quotient;
    }

    // expr_denominators_lcm
    //   function: the least common multiple of the denominators of _a,
    // exact rationals of the family, into _lcm; false where one is none, or
    // the multiple passes the exact integers.
    inline bool
    expr_denominators_lcm(
        const std::vector<value>& _a,
        dj_math_imax&             _lcm
    )
    {
        _lcm = 1;

        for (std::size_t i = 0; i < _a.size(); ++i)
        {
            dj_rational  q      = { 0, 1 };
            dj_math_imax common = _lcm;
            dj_math_imax other  = 0;

            if (!_a[i].to(q))
            {
                return false;
            }

            other = q.den;

            // the greatest common divisor, by Euclid's algorithm
            while (other != 0)
            {
                const dj_math_imax rest = (common % other);

                common = other;
                other  = rest;
            }

            if ( (_lcm / common) >
                 (std::numeric_limits<dj_math_imax>::max() / q.den) )
            {
                return false;
            }

            _lcm = ( (_lcm / common) * q.den );
        }

        return true;
    }

    // expr_parts_denominators_lcm
    //   function: the least common multiple of the denominators of the
    // parts of _a, exact numbers, real or complex, into _lcm; false where
    // one is none, or the multiple passes the exact integers.
    inline bool
    expr_parts_denominators_lcm(
        const std::vector<value>& _a,
        dj_math_imax&             _lcm
    )
    {
        std::vector<value> parts;

        // each element's parts, a real one itself
        for (std::size_t i = 0; i < _a.size(); ++i)
        {
            if (expr_is_complex(_a[i]))
            {
                parts.push_back(expr_real(_a[i]));
                parts.push_back(expr_imag(_a[i]));
            }
            else
            {
                parts.push_back(_a[i]);
            }
        }

        return expr_denominators_lcm(parts,
                                     _lcm);
    }

    // expr_gaussian_candidate
    //   function: the number nearest _z whose parts are multiples of 1/_lcm,
    // exactly: the complex number of those parts, or the real one where the
    // imaginary part comes to 0; false where a part's multiple passes the
    // exact integers.
    inline bool
    expr_gaussian_candidate(
        const math::complex<long double>& _z,
        dj_math_imax                      _lcm,
        value&                            _out
    )
    {
        const long double re = std::floor((_z.real() *
                                           static_cast<long double>(_lcm)) +
                                          0.5L);
        const long double im = std::floor((_z.imag() *
                                           static_cast<long double>(_lcm)) +
                                          0.5L);

        // each multiple within the exact integers
        if (!( (re > -9.0e18L) &&
               (re < 9.0e18L) &&
               (im > -9.0e18L) &&
               (im < 9.0e18L) ))
        {
            return false;
        }

        const value real_part = ( value(static_cast<dj_math_imax>(re)) /
                                  value(_lcm) );
        const value imag_part = ( value(static_cast<dj_math_imax>(im)) /
                                  value(_lcm) );

        _out = ( (im == 0.0L) ? real_part
                              : expr_complex(real_part,
                                             imag_part) );

        return true;
    }

    // expr_eigenvalue_before
    //   function: whether the eigenvalue _x comes before _y: by real part,
    // then by imaginary part, in value.h's exact order, a real one's
    // imaginary part 0 (S273).
    inline bool
    expr_eigenvalue_before(
        const value& _x,
        const value& _y
    )
    {
        const value xr = expr_real(_x);
        const value yr = expr_real(_y);

        // by real part, then by imaginary part
        if (!(xr == yr))
        {
            return (xr < yr);
        }

        return (expr_imag(_x) < expr_imag(_y));
    }

    // expr_matrix_eig_exact
    //   function: an exact A's eigenvalues (S258, S352). Each is found in
    // long double, as a dense A's of real or complex elements is, a real
    // A's real where each is found real and every one otherwise; and each
    // made exact where it is a Gaussian rational, a rational among them: A's
    // characteristic polynomial, Faddeev and LeVerrier's, exactly; each
    // eigenvalue's nearest number of parts that are multiples of 1/d, d the
    // least common multiple of A's parts' denominators, which every Gaussian
    // rational eigenvalue is, d A's characteristic polynomial being monic
    // over the Gaussian integers; and each such one a root, the polynomial
    // divided by it once for each, so that a repeated eigenvalue is found as
    // often as it is repeated. A spectrum found real, or each of whose
    // eigenvalues is a rational, is real, ascending; any other is in the
    // order of expr_eigenvalue_before, each inexact one a complex long
    // double, a real one's imaginary part +0; a domain error where they are
    // not each found.
    inline value
    expr_matrix_eig_exact(
        const value& _a,
        std::size_t  _n
    )
    {
        typedef math::complex<long double> complex_ld;

        const std::vector<value> a         = expr_matrix_values_of(_a);
        const bool               complex_a = expr_matrix_has_complex(_a);
        std::vector<long double> found_real;
        std::vector<complex_ld>  found;
        std::vector<value>       poly;
        std::vector<value>       eigenvalues(_n);
        std::vector<bool>        made(_n,
                                      false);
        dj_math_imax             lcm       = 1;
        std::size_t              exact     = 0;
        bool                     real      = false;
        bool                     all       = true;

        // found in long double: a real A's real, where they are, else
        // every one; a complex A's, a Hermitian one's real
        if (complex_a)
        {
            all = expr_matrix_complex_spectrum_in<long double>(
                expr_matrix_dense_of<complex_ld>(_a),
                _n,
                found,
                found_real,
                real);
        }
        else
        {
            real = expr_matrix_eigenvalues_in<long double>(
                expr_matrix_dense_of<long double>(_a),
                _n,
                found_real);

            // a complex pair: every one, complex
            if (!real)
            {
                all = expr_matrix_spectrum_in<long double>(
                    expr_matrix_dense_of<long double>(_a),
                    _n,
                    found);
            }
        }

        // not each found: a domain error
        if (!all)
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // each as found: a real one real, where all are
        if (real)
        {
            found.clear();
        }

        // each eigenvalue as found, in long double
        for (std::size_t i = 0; i < _n; ++i)
        {
            if (real)
            {
                found.push_back(complex_ld(found_real[i],
                                           0.0L));
                eigenvalues[i] = value(found_real[i]);
            }
            else
            {
                eigenvalues[i] = expr_complex_of(found[i]);
            }
        }

        // each Gaussian rational root its nearest found eigenvalue's, as
        // often as it is a root
        if ( (expr_char_poly(a,
                             _n,
                             poly)) &&
             (expr_parts_denominators_lcm(a,
                                          lcm)) )
        {
            // each found eigenvalue's candidate
            for (std::size_t i = 0; i < _n; ++i)
            {
                value candidate;

                if (!expr_gaussian_candidate(found[i],
                                             lcm,
                                             candidate))
                {
                    continue;
                }

                // a root, once for each time it divides the polynomial
                while ( (poly.size() > 1) &&
                        (expr_poly_at(poly,
                                      candidate) == value(0)) )
                {
                    complex_ld  q;
                    std::size_t nearest = _n;
                    long double gap     = 0.0L;

                    static_cast<void>(expr_complex_to(candidate,
                                                      q));

                    // the nearest found one not yet made exact
                    for (std::size_t k = 0; k < _n; ++k)
                    {
                        const long double d =
                            ( std::fabs(found[k].real() - q.real()) +
                              std::fabs(found[k].imag() - q.imag()) );

                        if ( (!made[k]) &&
                             ( (nearest == _n) ||
                               (d < gap) ) )
                        {
                            nearest = k;
                            gap     = d;
                        }
                    }

                    eigenvalues[nearest] = candidate;
                    made[nearest]        = true;
                    poly                 = expr_poly_divided(poly,
                                                             candidate);
                    ++exact;
                }
            }
        }

        bool rational = (exact == _n);

        // each exact and real: a real spectrum
        for (std::size_t i = 0; ( (rational) && (i < _n) ); ++i)
        {
            rational = (!expr_is_complex(eigenvalues[i]));
        }

        // a real spectrum: ascending
        if ( (real) ||
             (rational) )
        {
            std::stable_sort(eigenvalues.begin(),
                             eigenvalues.end(),
                             [](const value& _x,
                                const value& _y) -> bool
                             {
                                 return (_x < _y);
                             });
        }
        else
        {
            std::stable_sort(eigenvalues.begin(),
                             eigenvalues.end(),
                             &expr_eigenvalue_before);
        }

        return expr_matrix_of_values(expr_shape( ( (_n == 0) ? 0 : 1 ),
                                                _n),
                                     eigenvalues);
    }

    // expr_quaternion_standard
    //   function: a number's standard eigenvalue, as eig of a number gives
    // it (S390): a quaternion's w + |v|i, |v| the norm of its im, jm and km
    // parts, the complex number of imaginary part 0 or more similar to it,
    // u q u^-1, which a 1 by 1 matrix of it has (S389): exact where q is
    // and |v| rational, otherwise in q's floating-point type, |v| quat.h's
    // scaled norm, a long double for an exact q of an irrational |v|; with
    // q's flags. Any other number is its own.
    template<typename Type>
    value
    expr_quaternion_standard_in(
        const value& _q
    )
    {
        math::quaternion<Type> q(Type(0), Type(0), Type(0), Type(0));

        static_cast<void>(expr_quaternion_to(_q, q));

        const Type v = norm(math::quaternion<Type>(q.m_x, q.m_y, q.m_z, Type(0)));

        return expr_flagged(expr_complex_of(math::complex<Type>(q.m_w, v)),
                            _q.flags());
    }

    inline value
    expr_quaternion_standard(
        const value& _q
    )
    {
        // no quaternion: its own
        if (!expr_is_quaternion(_q))
        {
            return _q;
        }

        // exactly, where |v| is rational
        if (_q.is_exact())
        {
            const expr_quaternion_parts_held q =
                expr_quaternion_parts_of(_q.c_value());

            return expr_flagged(
                expr_complex(q.w,
                             value_root_of( ( (q.x * q.x) + (q.y * q.y) ) +
                                            (q.z * q.z) )),
                _q.flags());
        }

        switch (expr_quaternion_widest(&_q, 1))
        {
            case D_MATH_VALUE_FLOAT:
                return expr_quaternion_standard_in<float>(_q);

            case D_MATH_VALUE_DOUBLE:
                return expr_quaternion_standard_in<double>(_q);

            default:
                break;
        }

        return expr_quaternion_standard_in<long double>(_q);
    }

    // expr_matrix_adjoint
    //   functions: the complex adjoint of an n by n A of quaternions, 2n by
    // 2n, chi(A) = [A1, A2; -conj(A2), conj(A1)], A = A1 + A2 jm, each
    // element w + x im + y jm + z km being (w + x im) + (y + z im) jm, so
    // that chi(A B) = chi(A) chi(B) (S389): in Type, dense, each part as it
    // is; or exactly, of exact complex numbers.
    template<typename Type>
    value
    expr_matrix_adjoint_in(
        const value& _a,
        std::size_t  _n
    )
    {
        const std::vector<math::quaternion<Type> > q =
            expr_matrix_dense_of<math::quaternion<Type> >(_a);
        const std::size_t                          m = (2 * _n);
        std::vector<math::complex<Type> >          z(m * m);

        // each element's four places
        for (std::size_t i = 0; i < _n; ++i)
        {
            for (std::size_t j = 0; j < _n; ++j)
            {
                const math::quaternion<Type>& e = q[(i * _n) + j];

                z[(i * m) + j]               = math::complex<Type>(e.m_w, e.m_x);
                z[(i * m) + _n + j]          = math::complex<Type>(e.m_y, e.m_z);
                z[((_n + i) * m) + j]        = math::complex<Type>(-e.m_y,
                                                                   e.m_z);
                z[((_n + i) * m) + _n + j]   = math::complex<Type>(e.m_w,
                                                                   -e.m_x);
            }
        }

        return expr_matrix_of_dense<math::complex<Type> >(
            expr_shape(m,
                       m),
            z,
            expr_matrix_flags_of(_a));
    }

    inline value
    expr_matrix_adjoint_exact(
        const value& _a,
        std::size_t  _n
    )
    {
        const std::vector<value> x = expr_matrix_values_of(_a);
        const std::size_t        m = (2 * _n);
        std::vector<value>       z(m * m);

        // each element's four places
        for (std::size_t i = 0; i < _n; ++i)
        {
            for (std::size_t j = 0; j < _n; ++j)
            {
                const expr_quaternion_parts_held e =
                    expr_quaternion_parts_of(x[(i * _n) + j].c_value());

                z[(i * m) + j]             = expr_complex(e.w, e.x);
                z[(i * m) + _n + j]        = expr_complex(e.y, e.z);
                z[((_n + i) * m) + j]      = expr_complex(-e.y, e.z);
                z[((_n + i) * m) + _n + j] = expr_complex(e.w, -e.x);
            }
        }

        const expr_result<value> made = expr_matrix(m,
                                                    m,
                                                    z);

        return ( (made.is_ok()) ? made.value()
                                : value(D_MATH_VALUE_ERROR_DOMAIN) );
    }

    // expr_eigen_standard
    //   function: of chi(A)'s 2n eigenvalues _all, which come in conjugate
    // pairs, A's n standard ones, one of each pair, of imaginary part 0 or
    // more (S389): the pairs found greedily, each eigenvalue in turn with
    // the nearest one still unpaired once each is folded into the upper half
    // plane, (re, |im|), by |re - re'| + ||im| - |im'||, so that a pair that
    // rounding leaves a few steps from conjugate, and a real eigenvalue,
    // found twice, pair as they should, and a pair mistaken for another lies
    // within those steps of it; of each pair, the one of the larger
    // imaginary part, conjugated where that is below 0.
    inline std::vector<value>
    expr_eigen_standard(
        const std::vector<value>& _all
    )
    {
        const std::size_t        count = _all.size();
        std::vector<long double> re(count, 0.0L);
        std::vector<long double> im(count, 0.0L);
        std::vector<bool>        paired(count, false);
        std::vector<value>       out;

        // each one's parts, in long double, a real one's im 0
        for (std::size_t i = 0; i < count; ++i)
        {
            static_cast<void>(expr_real(_all[i]).to(re[i]));
            static_cast<void>(expr_imag(_all[i]).to(im[i]));
        }

        // each unpaired one with its nearest unpaired one
        for (std::size_t i = 0; i < count; ++i)
        {
            std::size_t best     = count;
            long double distance = 0.0L;

            if (paired[i])
            {
                continue;
            }

            for (std::size_t j = (i + 1); j < count; ++j)
            {
                const long double d =
                    ( std::fabs(re[i] - re[j]) +
                      std::fabs(std::fabs(im[i]) - std::fabs(im[j])) );

                if ( (!paired[j]) &&
                     ( (best == count) ||
                       (d < distance) ) )
                {
                    best     = j;
                    distance = d;
                }
            }

            std::size_t chosen = i;

            paired[i] = true;

            if (best != count)
            {
                paired[best] = true;
                chosen       = ( (im[best] > im[i]) ? best : i );
            }

            out.push_back( (im[chosen] < 0.0L) ? expr_conj(_all[chosen])
                                               : _all[chosen] );
        }

        return out;
    }

    // expr_matrix_spectrum_row
    //   function: eigenvalues as eig gives them, a row, real ascending where
    // each is real and otherwise in the order of expr_eigenvalue_before.
    inline value
    expr_matrix_spectrum_row(
        std::vector<value> _spectrum
    )
    {
        std::vector<value>& standard = _spectrum;
        bool                real     = true;

        for (std::size_t i = 0; ( (real) && (i < standard.size()) ); ++i)
        {
            real = (!expr_is_complex(standard[i]));
        }

        // a real spectrum ascending, any other by real part, then imaginary
        if (real)
        {
            std::stable_sort(standard.begin(),
                             standard.end(),
                             [](const value& _x,
                                const value& _y) -> bool
                             {
                                 return (_x < _y);
                             });
        }
        else
        {
            std::stable_sort(standard.begin(),
                             standard.end(),
                             &expr_eigenvalue_before);
        }

        return expr_matrix_of_values(expr_shape(1,
                                                standard.size()),
                                     standard);
    }

    // expr_matrix_eig_standard
    //   function: of chi(A)'s spectrum _all, a row of 2n, A's n standard
    // eigenvalues (expr_eigen_standard), as eig gives them
    // (expr_matrix_spectrum_row); an error passes.
    inline value
    expr_matrix_eig_standard(
        const value& _all
    )
    {
        // an error passes
        if (_all.kind() == D_MATH_VALUE_ERROR)
        {
            return _all;
        }

        return expr_matrix_spectrum_row(
            expr_eigen_standard(expr_matrix_values_of(_all)));
    }

    // expr_matrix_triangular, expr_matrix_eig_diagonal
    //   functions: whether the n by n A is triangular, each element below
    // its diagonal, or each above it, 0, each part of a quaternion; and the
    // standard eigenvalues of its diagonal's elements, which a triangular
    // quaternion matrix's are, its complex adjoint, rows and columns
    // interleaved, being block triangular, each diagonal element's own 2 by
    // 2 adjoint a block; as eig gives them, each found by itself (S389).
    inline bool
    expr_matrix_triangular(
        const value& _a,
        std::size_t  _n
    )
    {
        bool upper = true;
        bool lower = true;

        // every element off the diagonal, by its side
        for (std::size_t i = 0; ( ( (upper) || (lower) ) && (i < _n) ); ++i)
        {
            for (std::size_t j = 0; j < _n; ++j)
            {
                const bool zero = ( (i == j) ||
                                    (expr_matrix_element_of(
                                         _a,
                                         (i * _n) + j) == value(0)) );

                upper = ( (upper) && ( (i <= j) || (zero) ) );
                lower = ( (lower) && ( (i >= j) || (zero) ) );
            }
        }

        return ( (upper) ||
                 (lower) );
    }

    inline value
    expr_matrix_eig_diagonal(
        const value& _a,
        std::size_t  _n
    )
    {
        std::vector<value> spectrum;

        spectrum.reserve(_n);

        // each diagonal element's
        for (std::size_t i = 0; i < _n; ++i)
        {
            spectrum.push_back(expr_quaternion_standard(
                expr_matrix_element_of(_a,
                                       (i * _n) + i)));
        }

        return expr_matrix_spectrum_row(spectrum);
    }

    // expr_matrix_eig_quaternion, expr_matrix_eig_quaternion_exact
    //   functions: eig(A) of an n by n A holding quaternions (S277, S389):
    // Zhang's standard right eigenvalues, the n complex numbers of
    // imaginary part 0 or more to one of which each right eigenvalue,
    // A v = v lambda, is similar, u lambda u^-1: those of the complex
    // adjoint chi(A), 2n by 2n, whose eigenvalues come in conjugate pairs,
    // found as a complex matrix's are (S352), by cmat.h's Jacobi method
    // where A, and so chi(A), is Hermitian, by its complex QR otherwise, in
    // Type, or exactly where A is and each is a Gaussian rational
    // (expr_matrix_eig_exact); one of each pair (expr_eigen_standard). Of a
    // triangular A, a 1 by 1 among them, its diagonal's standard
    // eigenvalues, w + |v|i each (expr_matrix_eig_diagonal).
    template<typename Type>
    value
    expr_matrix_eig_quaternion(
        const value& _a,
        std::size_t  _n
    )
    {
        // triangular: its diagonal's
        if (expr_matrix_triangular(_a,
                                   _n))
        {
            return expr_matrix_eig_diagonal(_a,
                                            _n);
        }

        const value adjoint = expr_matrix_adjoint_in<Type>(_a,
                                                           _n);

        return expr_matrix_eig_standard(
            (adjoint.kind() == D_MATH_VALUE_ERROR)
                ? adjoint
                : expr_matrix_eig_complex<Type>(adjoint,
                                                (2 * _n)));
    }

    inline value
    expr_matrix_eig_quaternion_exact(
        const value& _a,
        std::size_t  _n
    )
    {
        // triangular: its diagonal's
        if (expr_matrix_triangular(_a,
                                   _n))
        {
            return expr_matrix_eig_diagonal(_a,
                                            _n);
        }

        const value adjoint = expr_matrix_adjoint_exact(_a,
                                                        _n);

        return expr_matrix_eig_standard(
            (adjoint.kind() == D_MATH_VALUE_ERROR)
                ? adjoint
                : expr_matrix_eig_exact(adjoint,
                                        (2 * _n)));
    }

    inline value
    expr_matrix_eig(
        const value& _a
    )
    {
        value             out;
        const expr_shape  shape = ( expr_matrix_held(_a.c_value())
                                        ? expr_matrix_box_of(
                                              _a.c_value()).shape
                                        : expr_shape() );
        const std::size_t n     = shape.rows;

        // an error, or no operand; a number, its own, a quaternion's its
        // standard eigenvalue (S390)
        if (expr_matrix_passes(&_a,
                               1,
                               out))
        {
            return out;
        }

        if (!expr_matrix_held(_a.c_value()))
        {
            return expr_quaternion_standard(_a);
        }

        // square
        if (shape.rows != shape.columns)
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // by the type it is computed in
        switch (expr_matrix_algebra_storage(_a))
        {
            case expr_matrix_storage::floats:
                return expr_matrix_eig_in<float>(_a,
                                                 n);

            case expr_matrix_storage::doubles:
                return expr_matrix_eig_in<double>(_a,
                                                  n);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_eig_in<long double>(_a,
                                                       n);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_eig_complex<float>(_a,
                                                      n);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_eig_complex<double>(_a,
                                                       n);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_eig_complex<long double>(_a,
                                                            n);

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_eig_quaternion<float>(_a,
                                                         n);

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_eig_quaternion<double>(_a,
                                                          n);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_eig_quaternion<long double>(_a,
                                                               n);

            case expr_matrix_storage::values:
            default:
                break;
        }

        // exact, of quaternions or not; or of a boxed inexact element, in
        // long double
        if ( (expr_matrix_all_exact(_a)) &&
             (expr_matrix_has_quaternion(_a)) )
        {
            return expr_matrix_eig_quaternion_exact(_a,
                                                    n);
        }

        return ( expr_matrix_all_exact(_a)
                     ? expr_matrix_eig_exact(_a,
                                             n)
                     : ( expr_matrix_has_complex(_a)
                             ? expr_matrix_eig_complex<long double>(_a,
                                                                    n)
                             : expr_matrix_eig_in<long double>(_a,
                                                               n) ) );
    }

    // 1.5
    // expr_function_shape
    //   function: where an operand's shape is one the function _name does
    // not take, the message of its bad_shape; else an empty one.
    inline std::string
    expr_function_shape(
        const char*  _name,
        const value& _a,
        const value* _b
    )
    {
        const std::string name = _name;
        const value       operands[2] = { _a,
                                          ( (_b != D_NULLPTR) ? *_b : _a ) };
        value             passed;

        // an error, or no operand: the value's own answer
        if (expr_matrix_passes(operands,
                               2,
                               passed))
        {
            return std::string();
        }

        const bool        a    = expr_matrix_held(_a.c_value());
        const bool        b    = ( (_b != D_NULLPTR) &&
                                   (expr_matrix_held(_b->c_value())) );
        const expr_shape  one(1,
                              1);
        const expr_shape  x    = ( a ? expr_matrix_box_of(_a.c_value()).shape
                                     : one );
        const expr_shape  y    = ( b ? expr_matrix_box_of(_b->c_value()).shape
                                     : one );

        // of one matrix: square; norm, which takes any, asks none
        if (_b == D_NULLPTR)
        {
            return ( ( (a) &&
                       (x.rows != x.columns) )
                         ? ("expr_" + name + ": a matrix of " +
                            expr_shape_text(x) + " is not square")
                         : std::string() );
        }

        // dot: two numbers, or two vectors of one length
        if (name == "dot")
        {
            return ( ( (a == b) &&
                       ( (!a) ||
                         ( (expr_vector_length(x) != expr_no_vector) &&
                           (expr_vector_length(x) ==
                                expr_vector_length(y)) ) ) )
                         ? std::string()
                         : ("expr_dot: two vectors of one length, not " +
                            expr_shape_text(x) + " and " +
                            expr_shape_text(y)) );
        }

        // cross: two vectors of 3 elements
        if (name == "cross")
        {
            return ( ( (a) &&
                       (b) &&
                       (expr_vector_length(x) == 3) &&
                       (expr_vector_length(y) == 3) )
                         ? std::string()
                         : ("expr_cross: two vectors of 3 elements, not " +
                            expr_shape_text(x) + " and " +
                            expr_shape_text(y)) );
        }

        // solve: a number and anything; or A square and B of its rows, or
        // a vector of as many elements
        return ( ( (!a) ||
                   ( (b) &&
                     (x.rows == x.columns) &&
                     ( (y.rows == x.rows) ||
                       (expr_vector_length(y) == x.rows) ) ) )
                     ? std::string()
                     : ("expr_solve: a square matrix and one of its rows, "
                        "not " + expr_shape_text(x) + " and " +
                        expr_shape_text(y)) );
    }

    // expr_function_result
    //   function: _answer, or bad_shape of _message where it is not empty.
    inline expr_result<value>
    expr_function_result(
        const std::string& _message,
        const value&       _answer
    )
    {
        return ( _message.empty()
                     ? ok<value, expr_error>(_answer)
                     : err<value, expr_error>(
                           expr_error(expr_status::bad_shape,
                                      expr_no_offset,
                                      _message)) );
    }

NS_END  // internal

//==============================================================================
// 2.  THE FUNCTIONS
//==============================================================================
// The language's functions of linear algebra as C++ calls: each a
// function's shape checked first, then its part of section 1.

// 2.1
// Of one matrix
/**
 * @brief det, inv, trace, norm and eig of a matrix value, or of a number,
 *        a scalar (S256).
 *
 * @param[in] _matrix  the matrix, or a number.
 * @return expr_det: the determinant; expr_inv: the inverse, a pole where
 *         the matrix is singular; expr_trace: the sum of the diagonal;
 *         expr_norm: a vector's Euclidean norm, a matrix's Frobenius norm,
 *         of the elements' moduli; expr_eig: every eigenvalue, a vector
 *         of one row, ascending by real part, then by imaginary part,
 *         complex where any is, a Hermitian matrix's real, a domain error
 *         where they are not each found, or are complex for a real matrix
 *         under real_results (S352), a matrix of quaternions' Zhang's
 *         standard ones (S389). Exact for an exact matrix, each
 *         eigenvalue where it is a Gaussian rational; in a dense one's
 *         type otherwise, complex where any element is (S257, S258,
 *         S351). Of a number: itself, its reciprocal, itself, its
 *         magnitude, itself; of a quaternion, or a matrix holding one,
 *         det a domain error (S277), and of a quaternion eig its standard
 *         eigenvalue, w + |v|i (S390). An error passes; a boolean or none
 *         is a domain error. bad_shape, where the matrix is not square,
 *         for each but expr_norm.
 */
D_NODISCARD inline expr_result<value>
expr_det(
    const value& _matrix
)
{
    return internal::expr_function_result(
        internal::expr_function_shape("det",
                                      _matrix,
                                      D_NULLPTR),
        internal::expr_matrix_det(_matrix));
}

D_NODISCARD inline expr_result<value>
expr_inv(
    const value& _matrix
)
{
    return internal::expr_function_result(
        internal::expr_function_shape("inv",
                                      _matrix,
                                      D_NULLPTR),
        internal::expr_matrix_inv(_matrix));
}

D_NODISCARD inline expr_result<value>
expr_trace(
    const value& _matrix
)
{
    return internal::expr_function_result(
        internal::expr_function_shape("trace",
                                      _matrix,
                                      D_NULLPTR),
        internal::expr_matrix_trace(_matrix));
}

D_NODISCARD inline expr_result<value>
expr_norm(
    const value& _matrix
)
{
    return ok<value, expr_error>(internal::expr_matrix_norm(_matrix));
}

D_NODISCARD inline expr_result<value>
expr_eig(
    const value& _matrix
)
{
    return internal::expr_function_result(
        internal::expr_function_shape("eig",
                                      _matrix,
                                      D_NULLPTR),
        internal::expr_matrix_eig(_matrix));
}

// 2.2
// Of two
/**
 * @brief dot, cross and solve of two matrix values, or of numbers (S256).
 *
 * @param[in] _a  the first: a vector, a matrix, or a number.
 * @param[in] _b  the second.
 * @return expr_dot: the sum of conj(a_i) b_i over two vectors of one
 *         length, each a row or a column, a real a_i itself (S273, S351),
 *         or conj(a) b of two numbers; expr_cross: two vectors of 3
 *         elements' vector product, of _a's shape, conjugating nothing,
 *         each product in its order; expr_solve: the X for which _a X =
 *         _b, _a square and _b of its rows, or a vector of as many
 *         elements, X then of _b's shape, a pole where _a is singular; of a
 *         number _a, _b / _a, or _a^-1 _b on the left where _a is no real
 *         number and either holds a quaternion (S390). Exact for exact
 *         operands, otherwise in their wider dense type, of quaternions
 *         where either holds one, else complex where either holds a
 *         complex number (S257, S351, S388). An error
 *         passes, the first; a boolean or none is a domain error; bad_shape
 *         for shapes the function does not take, a vector and a number
 *         among them.
 */
D_NODISCARD inline expr_result<value>
expr_dot(
    const value& _a,
    const value& _b
)
{
    return internal::expr_function_result(
        internal::expr_function_shape("dot",
                                      _a,
                                      &_b),
        internal::expr_matrix_dot(_a,
                                  _b));
}

D_NODISCARD inline expr_result<value>
expr_cross(
    const value& _a,
    const value& _b
)
{
    return internal::expr_function_result(
        internal::expr_function_shape("cross",
                                      _a,
                                      &_b),
        internal::expr_matrix_cross(_a,
                                    _b));
}

D_NODISCARD inline expr_result<value>
expr_solve(
    const value& _matrix,
    const value& _right
)
{
    return internal::expr_function_result(
        internal::expr_function_shape("solve",
                                      _matrix,
                                      &_right),
        internal::expr_matrix_solve(_matrix,
                                    _right));
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_MATRIX_FUNCTION_HPP
