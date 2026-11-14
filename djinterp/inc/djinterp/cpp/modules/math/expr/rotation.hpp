/*******************************************************************************
* djinterp [cpp/math]                                               rotation.hpp
*
* A quaternion's functions in the run-time expression language that give or
* take a vector or a matrix; roadmap Phase 3, step 24 (3f), its third part.
*   THE PARTS. parts(q) is the row [w, x, y, z] of a number's four parts, a
* real or complex number's missing ones 0s of its parts' kind (S275).
*   ROTATIONS. Any q not 0 names the rotation of q / |q|, active and
* right-handed, as quat.h's convention is (S276): rotate(q, v) is the
* 3-vector q v q^-1, of v's shape (S375), and rotmat(q) its 3 by 3 matrix R,
* R v = q v q^-1, as MATLAB's rotmat(q, 'point') is, each exact where q is,
* its elements sums of the parts' products over |q|^2, and otherwise quat.h's
* accurate forms, each element within a step of the correctly rounded one
* (S360). rotquat(R) is the unit quaternion of a rotation's matrix, 3 by 3 or
* the upper left of a 4 by 4, w >= 0, by Shepperd's method as quat.h computes
* it: exact where R is and the root its largest candidate takes is rational,
* and otherwise quat.h's, each part rounded once, in R's floating-point type,
* a long double for an exact R (S391).
*   INTERPOLATION. slerp(p, q, t) blends the rotations p and q name along the
* shorter arc at a constant speed, p / |p| at t = 0 and q / |q| or its
* negative at t = 1: each normalized by quat.h's scaled normalization, then
* quat.h's accurate slerp, dense in the widest of the three's floating-point
* types, a long double where all are exact (S391).
*   DOMAINS. An error passes, the first; what is no number, a quaternion or
* a matrix where a number is taken, a vector or a matrix of other than real
* numbers or of another shape, and an exact 0 where a rotation is named, is
* a domain error; a dense 0 gives NaNs, as quat.h's rotation of 0 does.
* Every result carries its arguments' flags.
*   THE HEADER. matrix.hpp's kind holds quaternions (S386), so it comes after
* quaternion.hpp, and these functions, which make or read matrices, after
* both; parts and rotate came here from quaternion.hpp in chunk 46 (S391).
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/rotation.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.07
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE PARTS
    ---------
    1.  As a vector
         1.  expr_quaternion_parts
2.  ROTATIONS
    ---------
    1.  Reading the operands
    2.  The rotation of a vector
         1.  expr_quaternion_rotate
    3.  The rotation's matrix
         1.  expr_quaternion_matrix
    4.  The quaternion of a rotation's matrix
         1.  expr_quaternion_of_rotation
3.  INTERPOLATION
    -------------
    1.  slerp
         1.  expr_quaternion_slerp
*/

#ifndef DJINTERP_MATH_EXPR_ROTATION_HPP
#define DJINTERP_MATH_EXPR_ROTATION_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <vector>   // std::vector
// djinterp
#include "../../../../c/modules/math/value.h"  // d_math_value_cmp
#include "../linear_algebra/matrix.hpp"        // linalg::matrix
#include "../math_common.hpp"                  // NS_MATH
#include "../quaternion.hpp"                   // to_matrix, from_matrix, slerp
#include "../value.hpp"                        // value, value_root_of
#include "./complex.hpp"                       // expr_complex_flagged
#include "./expr_common.hpp"                   // expr_result
#include "./matrix.hpp"                        // expr_vector, expr_matrix
#include "./quaternion.hpp"                    // the kind, its parts


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE PARTS
//==============================================================================


// 1.1    As a vector
//------------------------------------------------------------------------------
// 1.1.1
// expr_quaternion_parts
/**
 * @brief A number's four parts, w, x, y and z, as a vector [w, x, y, z], the
 *        language's parts(q) (S275).
 *
 * @param[in] _value  the number.
 * @return the vector, a matrix of one row: a quaternion's parts as it holds
 *         them, a dense part a value of its type; a complex number's two and
 *         a real number's one, each other part a 0 of the parts' kind, as a
 *         real number's imaginary part is (complex.hpp's expr_imag); an
 *         error, or a domain error, as section 2.2 says.
 */
D_NODISCARD inline value
expr_quaternion_parts(
    const value& _value
)
{
    value failed;

    // an error, or no number
    if (internal::expr_quaternion_failed(_value, failed))
    {
        return failed;
    }

    const internal::expr_quaternion_parts_held parts =
        internal::expr_quaternion_parts_of(_value.c_value());
    const value zero = internal::expr_quaternion_zero_like(parts.w);
    const unsigned flags = _value.flags();
    std::vector<value> elements;

    // a missing part a 0 of the parts' kind
    elements.push_back(internal::expr_complex_flagged(parts.w, flags));
    elements.push_back(internal::expr_complex_flagged(
        ( (parts.held >= 2) ? parts.x : zero ), flags));
    elements.push_back(internal::expr_complex_flagged(
        ( (parts.held == 4) ? parts.y : zero ), flags));
    elements.push_back(internal::expr_complex_flagged(
        ( (parts.held == 4) ? parts.z : zero ), flags));

    const expr_result<value> vector = expr_vector(elements);

    return ( (vector.is_ok()) ? vector.value()
                              : value(D_MATH_VALUE_ERROR_DOMAIN) );
}


//==============================================================================
// 2.  ROTATIONS
//==============================================================================


// 2.1    Reading the operands
//------------------------------------------------------------------------------
// A vector or a matrix of real numbers, its elements as values, and the
// matrix of a rotation an exact q names, from which both rotate and rotmat
// compute it exactly.
NS_INTERNAL

    // expr_quaternion_vector_of
    //   function: whether _v is a 3-vector of real numbers, a matrix of 1
    // by 3 or 3 by 1; and if so its elements, in _elements.
    inline bool
    expr_quaternion_vector_of(
        const value&        _v,
        std::vector<value>& _elements
    )
    {
        const expr_result<expr_shape> shape = expr_shape_of(_v);

        // no matrix, or not of three elements in a row or a column
        if ( (shape.is_err()) ||
             ( ( (shape.value().rows != 1) ||
                 (shape.value().columns != 3) ) &&
               ( (shape.value().rows != 3) ||
                 (shape.value().columns != 1) ) ) )
        {
            return false;
        }

        // each element, a real number
        for (std::size_t i = 0; i < 3; ++i)
        {
            const std::size_t row    = ( (shape.value().rows == 1) ? 0 : i );
            const std::size_t column = ( (shape.value().rows == 1) ? i : 0 );
            const expr_result<value> element = expr_element(_v,
                                                            row,
                                                            column);

            if ( (element.is_err()) ||
                 (!expr_complex_real_number(element.value())) )
            {
                return false;
            }

            _elements.push_back(element.value());
        }

        return true;
    }

    // expr_rotation_elements
    //   function: whether _m is a matrix of 3 by 3 or 4 by 4 real numbers;
    // and if so its upper-left 3 by 3 elements, row by row, in _elements.
    inline bool
    expr_rotation_elements(
        const value&        _m,
        std::vector<value>& _elements
    )
    {
        const expr_result<expr_shape> shape = expr_shape_of(_m);

        // no matrix, or neither 3 by 3 nor 4 by 4
        if ( (shape.is_err()) ||
             (shape.value().rows != shape.value().columns) ||
             ( (shape.value().rows != 3) &&
               (shape.value().rows != 4) ) )
        {
            return false;
        }

        // the upper-left 3 by 3, each element a real number
        for (std::size_t i = 0; i < 9; ++i)
        {
            const expr_result<value> element = expr_element(_m,
                                                            (i / 3),
                                                            (i % 3));

            if ( (element.is_err()) ||
                 (!expr_complex_real_number(element.value())) )
            {
                return false;
            }

            _elements.push_back(element.value());
        }

        return true;
    }

    // expr_rotation_scaled
    //   function: the matrix of the rotation the exact q names times |q|^2,
    // row by row, into _r, its nine elements each a sum of the parts'
    // products; and |q|^2, the sum of their squares.
    inline value
    expr_rotation_scaled(
        const expr_quaternion_parts_held& _q,
        value*                            _r
    )
    {
        const value& w = _q.w;
        const value& x = _q.x;
        const value& y = _q.y;
        const value& z = _q.z;
        const value  two(2);

        _r[0] = ( ( (w * w) + (x * x) ) - ( (y * y) + (z * z) ) );
        _r[1] = (two * ( (x * y) - (w * z) ));
        _r[2] = (two * ( (x * z) + (w * y) ));
        _r[3] = (two * ( (x * y) + (w * z) ));
        _r[4] = ( ( (w * w) - (x * x) ) + ( (y * y) - (z * z) ) );
        _r[5] = (two * ( (y * z) - (w * x) ));
        _r[6] = (two * ( (x * z) - (w * y) ));
        _r[7] = (two * ( (y * z) + (w * x) ));
        _r[8] = ( ( (w * w) - (x * x) ) - ( (y * y) - (z * z) ) );

        return ( ( (w * w) + (x * x) ) + ( (y * y) + (z * z) ) );
    }

    // expr_rotation_names_none
    //   function: whether _q, exact, is 0, which names no rotation.
    inline bool
    expr_rotation_names_none(
        const value& _q
    )
    {
        return ( (_q.is_exact()) &&
                 (d_math_value_cmp(_q.c_value(), d_math_value_from_int(0)) ==
                      D_MATH_VALUE_EQUAL) );
    }

    // expr_rotation_operand_failed
    //   function: whether _q, which names a rotation, is an error, which
    // passes, or no number, or an exact 0, a domain error; and if so that,
    // in _out.
    inline bool
    expr_rotation_operand_failed(
        const value& _q,
        value&       _out
    )
    {
        // an error, or no number
        if (expr_quaternion_failed(_q, _out))
        {
            return true;
        }

        // an exact 0: no rotation
        if (expr_rotation_names_none(_q))
        {
            _out = value(D_MATH_VALUE_ERROR_DOMAIN);

            return true;
        }

        return false;
    }

    // expr_quaternion_rotated_exactly
    //   function: v rotated by the exact q, q v q^-1, by its matrix over
    // |q|^2, exactly: each row of R |q|^2 times v, over |q|^2.
    inline std::vector<value>
    expr_quaternion_rotated_exactly(
        const expr_quaternion_parts_held& _q,
        const std::vector<value>&         _v
    )
    {
        value              r[9];
        const value        n = expr_rotation_scaled(_q, r);
        std::vector<value> out;

        // each row of R times v, over |q|^2
        for (std::size_t i = 0; i < 3; ++i)
        {
            out.push_back( ( ( (r[3 * i] * _v[0]) +
                               (r[(3 * i) + 1] * _v[1]) ) +
                             (r[(3 * i) + 2] * _v[2]) ) / n );
        }

        return out;
    }

    // expr_quaternion_rotated_as
    //   function: v rotated by q in Type, quat.h's accurate rotation of any q
    // not 0 (S360), each its nearest there.
    template<typename Type>
    std::vector<value>
    expr_quaternion_rotated_as(
        const value&              _q,
        const std::vector<value>& _v
    )
    {
        math::quaternion<Type>  q(Type(0), Type(0), Type(0), Type(0));
        linalg::vector<Type, 3> v;
        std::vector<value>      out;

        static_cast<void>(expr_quaternion_to(_q, q));

        for (std::size_t i = 0; i < 3; ++i)
        {
            Type element = Type(0);

            static_cast<void>(_v[i].to(element));
            v[i] = element;
        }

        const linalg::vector<Type, 3> r = rotate(q, v);

        // each element a value of Type
        for (std::size_t i = 0; i < 3; ++i)
        {
            out.push_back(value(r[i]));
        }

        return out;
    }

NS_END  // internal

// 2.2    The rotation of a vector
//------------------------------------------------------------------------------
// 2.2.1
// expr_quaternion_rotate
/**
 * @brief A 3-vector rotated by a number, q v q^-1, the language's rotate(q,
 *        v) (S275): any q not 0 names the rotation of q / |q|, R v as
 *        expr_quaternion_matrix gives R.
 *
 * @param[in] _q  the quaternion, or a real or complex number, taken as one.
 * @param[in] _v  the vector: a matrix of 1 by 3 or 3 by 1 of real numbers.
 * @return the rotated vector, of _v's shape: exactly where q's parts and v's
 *         elements are exact; otherwise in the widest of their
 *         floating-point types, a long double for exact ones, by quat.h's
 *         accurate rotation, each element within a step of the correctly
 *         rounded one (S360); carrying q's flags and v's. An error passes,
 *         q's first; a domain error where q is no number or an exact 0, or
 *         v no such vector.
 */
D_NODISCARD inline value
expr_quaternion_rotate(
    const value& _q,
    const value& _v
)
{
    std::vector<value> v;
    std::vector<value> rotated;
    value              failed;

    // an error passes, q's first; then what is no number
    if (_q.kind() == D_MATH_VALUE_ERROR)
    {
        return _q;
    }

    if (_v.kind() == D_MATH_VALUE_ERROR)
    {
        return _v;
    }

    if ( (internal::expr_quaternion_failed(_q, failed)) ||
         (!internal::expr_quaternion_vector_of(_v, v)) )
    {
        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    bool exact = _q.is_exact();

    for (std::size_t i = 0; i < 3; ++i)
    {
        exact = ( (exact) && (v[i].is_exact()) );
    }

    // an exact 0 names no rotation
    if ( (exact) &&
         (d_math_value_cmp(_q.c_value(), d_math_value_from_int(0)) ==
              D_MATH_VALUE_EQUAL) )
    {
        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    // exactly, or in the widest type
    if (exact)
    {
        rotated = internal::expr_quaternion_rotated_exactly(
            internal::expr_quaternion_parts_of(_q.c_value()),
            v);
    }
    else
    {
        const value operands[4] = { _q, v[0], v[1], v[2] };

        switch (internal::expr_quaternion_widest(operands, 4))
        {
            case D_MATH_VALUE_FLOAT:
                rotated = internal::expr_quaternion_rotated_as<float>(_q, v);
                break;

            case D_MATH_VALUE_DOUBLE:
                rotated = internal::expr_quaternion_rotated_as<double>(_q, v);
                break;

            default:
                rotated =
                    internal::expr_quaternion_rotated_as<long double>(_q, v);
                break;
        }
    }

    const unsigned flags = (_q.flags() | _v.flags());

    for (std::size_t i = 0; i < 3; ++i)
    {
        rotated[i] = internal::expr_complex_flagged(rotated[i], flags);
    }

    const expr_result<expr_shape> shape = expr_shape_of(_v);
    const expr_result<value>      out   =
        expr_matrix(shape.value().rows, shape.value().columns, rotated);

    return ( (out.is_ok()) ? out.value() : value(D_MATH_VALUE_ERROR_DOMAIN) );
}

// 2.3    The rotation's matrix
//------------------------------------------------------------------------------
NS_INTERNAL

    // expr_rotation_matrix_as
    //   function: the matrix of the rotation q names, in Type, by quat.h's
    // to_matrix3 of any q not 0, each element within a step of the
    // correctly rounded one (S360), row by row, each a value of Type.
    template<typename Type>
    std::vector<value>
    expr_rotation_matrix_as(
        const value& _q
    )
    {
        math::quaternion<Type> q(Type(0), Type(0), Type(0), Type(0));
        std::vector<value>     out;

        static_cast<void>(expr_quaternion_to(_q, q));

        const linalg::matrix<Type, 3, 3> m = to_matrix(q);

        // row by row
        for (std::size_t i = 0; i < 9; ++i)
        {
            out.push_back(value(m((i / 3), (i % 3))));
        }

        return out;
    }

NS_END  // internal

// 2.3.1
// expr_quaternion_matrix
/**
 * @brief The matrix of the rotation a number names, the language's
 *        rotmat(q) (S391): of any q not 0, the 3 by 3 R of the rotation of
 *        q / |q|, R v = q v q^-1, as MATLAB's rotmat(q, 'point') gives it.
 *
 * @param[in] _q  the quaternion, or a real or complex number, taken as one.
 * @return the matrix: exactly where q's parts are exact, each element a sum
 *         of their products over |q|^2; otherwise in their floating-point
 *         type by quat.h's to_matrix3, each element within a step of the
 *         correctly rounded one (S360), and NaNs for a dense 0; carrying q's
 *         flags. An error passes; a domain error where q is no number or an
 *         exact 0.
 */
D_NODISCARD inline value
expr_quaternion_matrix(
    const value& _q
)
{
    std::vector<value> elements;
    value              failed;

    // an error, no number, or an exact 0
    if (internal::expr_rotation_operand_failed(_q, failed))
    {
        return failed;
    }

    // exactly, or in the parts' type
    if (_q.is_exact())
    {
        value       r[9];
        const value n = internal::expr_rotation_scaled(
            internal::expr_quaternion_parts_of(_q.c_value()),
            r);

        for (std::size_t i = 0; i < 9; ++i)
        {
            elements.push_back(r[i] / n);
        }
    }
    else
    {
        switch (internal::expr_quaternion_widest(&_q, 1))
        {
            case D_MATH_VALUE_FLOAT:
                elements = internal::expr_rotation_matrix_as<float>(_q);
                break;

            case D_MATH_VALUE_DOUBLE:
                elements = internal::expr_rotation_matrix_as<double>(_q);
                break;

            default:
                elements =
                    internal::expr_rotation_matrix_as<long double>(_q);
                break;
        }
    }

    // q's flags, on each element
    for (std::size_t i = 0; i < 9; ++i)
    {
        elements[i] = internal::expr_complex_flagged(elements[i],
                                                     _q.flags());
    }

    const expr_result<value> out = expr_matrix(3, 3, elements);

    return ( (out.is_ok()) ? out.value() : value(D_MATH_VALUE_ERROR_DOMAIN) );
}

// 2.4    The quaternion of a rotation's matrix
//------------------------------------------------------------------------------
NS_INTERNAL

    // expr_rotation_numerators
    //   function: Shepperd's numerators of the three parts the largest
    // candidate _k does not give, in x, y, z, w order, the off-diagonal sums
    // and differences of the 3 by 3 _m, as quat.h pairs them; _k's own 0.
    inline void
    expr_rotation_numerators(
        const std::vector<value>& _m,
        std::size_t               _k,
        value*                    _num
    )
    {
        const value& m01 = _m[1];
        const value& m02 = _m[2];
        const value& m10 = _m[3];
        const value& m12 = _m[5];
        const value& m20 = _m[6];
        const value& m21 = _m[7];

        // by the part the largest candidate gives
        switch (_k)
        {
            case 0:
                _num[1] = (m01 + m10);
                _num[2] = (m02 + m20);
                _num[3] = (m21 - m12);
                break;

            case 1:
                _num[0] = (m01 + m10);
                _num[2] = (m12 + m21);
                _num[3] = (m02 - m20);
                break;

            case 2:
                _num[0] = (m02 + m20);
                _num[1] = (m12 + m21);
                _num[3] = (m10 - m01);
                break;

            default:
                _num[0] = (m21 - m12);
                _num[1] = (m02 - m20);
                _num[2] = (m10 - m01);
                break;
        }

        return;
    }

    // expr_rotation_quaternion_exactly
    //   function: whether the unit quaternion of the exact rotation's matrix
    // _m, 3 by 3, is exact, by Shepperd's method as quat.h computes it: of
    // the candidates 4x^2 = 1 + m00 - m11 - m22, 4y^2 = 1 - m00 + m11 - m22,
    // 4z^2 = 1 - m00 - m11 + m22 and 4w^2 = 1 + m00 + m11 + m22 the largest,
    // w first among equals, then x, y and z, gives its part, its root over
    // 2, where that root is rational, and the others are the numerators over
    // twice the root; all four negated where w is negative. If so, the
    // quaternion, with _flags, in _out.
    inline bool
    expr_rotation_quaternion_exactly(
        const std::vector<value>& _m,
        unsigned                  _flags,
        value&                    _out
    )
    {
        const value one(1);
        const value c[4] = { ( ( (one + _m[0]) - _m[4]) - _m[8]),
                             ( ( (one - _m[0]) + _m[4]) - _m[8]),
                             ( ( (one - _m[0]) - _m[4]) + _m[8]),
                             ( ( (one + _m[0]) + _m[4]) + _m[8]) };
        std::size_t k = 3;
        value       part[4];

        // the largest candidate, w first among equals, then x, y and z
        for (std::size_t i = 0; i < 3; ++i)
        {
            if (d_math_value_cmp(c[i].c_value(), c[k].c_value()) ==
                    D_MATH_VALUE_GREATER)
            {
                k = i;
            }
        }

        const value s = value_root_of(c[k]);

        // an irrational root: not exact
        if (!s.is_exact())
        {
            return false;
        }

        expr_rotation_numerators(_m, k, part);

        // k's part the root over 2, the others over twice the root
        for (std::size_t i = 0; i < 4; ++i)
        {
            part[i] = ( (i == k) ? (s / value(2))
                                 : (part[i] / (s * value(2))) );
        }

        // q or -q: the one with w >= 0
        if (d_math_value_cmp(part[3].c_value(), d_math_value_from_int(0)) ==
                D_MATH_VALUE_LESS)
        {
            for (std::size_t i = 0; i < 4; ++i)
            {
                part[i] = -part[i];
            }
        }

        _out = expr_quaternion_made(part[3], part[0], part[1], part[2],
                                    _flags);

        return true;
    }

    // expr_rotation_quaternion_as
    //   function: the unit quaternion of the 3 by 3 _m in Type, by quat.h's
    // from_matrix3, each part rounded once, with _flags.
    template<typename Type>
    value
    expr_rotation_quaternion_as(
        const std::vector<value>& _m,
        unsigned                  _flags
    )
    {
        Type entries[9] = { Type(0), Type(0), Type(0), Type(0), Type(0),
                            Type(0), Type(0), Type(0), Type(0) };

        // each element its nearest Type
        for (std::size_t i = 0; i < 9; ++i)
        {
            static_cast<void>(_m[i].to(entries[i]));
        }

        return expr_quaternion_dense(
            from_matrix(linalg::matrix<Type, 3, 3>(entries)),
            _flags);
    }

NS_END  // internal

// 2.4.1
// expr_quaternion_of_rotation
/**
 * @brief The unit quaternion of a rotation's matrix, the language's
 *        rotquat(R) (S391): rotmat's inverse, q or -q, w >= 0, by Shepperd's
 *        method; of a 4 by 4 matrix, its upper-left 3 by 3.
 *
 * @param[in] _m  the matrix, 3 by 3 or 4 by 4, of real numbers.
 * @return the quaternion: exactly where R's elements are exact and the root
 *         Shepperd's largest candidate takes is rational, as the kind makes
 *         an exact one, rotquat of the identity being 1; otherwise in their
 *         widest floating-point type, a long double for exact ones, by
 *         quat.h's from_matrix3, each part rounded once, NaNs for a NaN or
 *         infinite element; a matrix a few steps from orthogonal gives a
 *         quaternion as many from unit length. Carrying R's flags. An error
 *         passes; a domain error where R is no such matrix.
 */
D_NODISCARD inline value
expr_quaternion_of_rotation(
    const value& _m
)
{
    std::vector<value> m;
    value              exact;

    // an error passes; no matrix of real numbers, 3 by 3 or 4 by 4
    if (_m.kind() == D_MATH_VALUE_ERROR)
    {
        return _m;
    }

    if (!internal::expr_rotation_elements(_m, m))
    {
        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    bool     all_exact = true;
    unsigned flags     = _m.flags();

    for (std::size_t i = 0; i < 9; ++i)
    {
        all_exact = ( (all_exact) && (m[i].is_exact()) );
        flags    |= m[i].flags();
    }

    // exactly, where the root is rational
    if ( (all_exact) &&
         (internal::expr_rotation_quaternion_exactly(m, flags, exact)) )
    {
        return exact;
    }

    // in the elements' widest type
    switch (internal::expr_quaternion_widest(m.data(), m.size()))
    {
        case D_MATH_VALUE_FLOAT:
            return internal::expr_rotation_quaternion_as<float>(m, flags);

        case D_MATH_VALUE_DOUBLE:
            return internal::expr_rotation_quaternion_as<double>(m, flags);

        default:
            break;
    }

    return internal::expr_rotation_quaternion_as<long double>(m, flags);
}


//==============================================================================
// 3.  INTERPOLATION
//==============================================================================


// 3.1    slerp
//------------------------------------------------------------------------------
NS_INTERNAL

    // expr_quaternion_slerp_as
    //   function: slerp from _p's rotation to _q's by _t, in Type: each
    // normalized by quat.h's scaled normalization (S361), then quat.h's
    // accurate slerp (S276), dense, with _flags; NaNs where either is 0.
    template<typename Type>
    value
    expr_quaternion_slerp_as(
        const value& _p,
        const value& _q,
        const value& _t,
        unsigned     _flags
    )
    {
        math::quaternion<Type> p(Type(0), Type(0), Type(0), Type(0));
        math::quaternion<Type> q(Type(0), Type(0), Type(0), Type(0));
        Type                   t = Type(0);

        static_cast<void>(expr_quaternion_to(_p, p));
        static_cast<void>(expr_quaternion_to(_q, q));
        static_cast<void>(_t.to(t));

        // a 0 names no rotation: NaNs
        if ( (norm(p) == Type(0)) ||
             (norm(q) == Type(0)) )
        {
            const Type nan = std::numeric_limits<Type>::quiet_NaN();

            return expr_quaternion_dense(
                math::quaternion<Type>(nan, nan, nan, nan),
                _flags);
        }

        return expr_quaternion_dense(slerp(normalize(p), normalize(q), t),
                                     _flags);
    }

NS_END  // internal

// 3.1.1
// expr_quaternion_slerp
/**
 * @brief Spherical linear interpolation between the rotations two numbers
 *        name, the language's slerp(p, q, t) (S391): p / |p| at t = 0, and
 *        q / |q| or its negative at t = 1, along the shorter arc at a
 *        constant speed.
 *
 * @param[in] _p  the first rotation's quaternion, or a real or complex
 *                number, taken as one.
 * @param[in] _q  the second's.
 * @param[in] _t  the parameter, a real number, any, 0 at p and 1 at q.
 * @return the quaternion, dense, in the widest of the three's floating-point
 *         types, a long double where all are exact: p and q each normalized
 *         by quat.h's scaled normalization (S361), then quat.h's accurate
 *         slerp (S276); NaNs for a dense 0; carrying every argument's flags.
 *         An error passes, the first; a domain error where p or q is no
 *         number or an exact 0, or t no real number.
 */
D_NODISCARD inline value
expr_quaternion_slerp(
    const value& _p,
    const value& _q,
    const value& _t
)
{
    const value operands[3] = { _p, _q, _t };
    value       failed;

    // an error passes, the first
    for (std::size_t i = 0; i < 3; ++i)
    {
        if (operands[i].kind() == D_MATH_VALUE_ERROR)
        {
            return operands[i];
        }
    }

    // no number, an exact 0, or no real t
    if ( (internal::expr_rotation_operand_failed(_p, failed)) ||
         (internal::expr_rotation_operand_failed(_q, failed)) ||
         (!internal::expr_complex_real_number(_t)) )
    {
        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    const unsigned flags = ( (_p.flags() | _q.flags()) | _t.flags() );

    // in the widest type
    switch (internal::expr_quaternion_widest(operands, 3))
    {
        case D_MATH_VALUE_FLOAT:
            return internal::expr_quaternion_slerp_as<float>(_p, _q, _t,
                                                             flags);

        case D_MATH_VALUE_DOUBLE:
            return internal::expr_quaternion_slerp_as<double>(_p, _q, _t,
                                                              flags);

        default:
            break;
    }

    return internal::expr_quaternion_slerp_as<long double>(_p, _q, _t,
                                                           flags);
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_ROTATION_HPP
