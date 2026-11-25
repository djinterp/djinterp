/*******************************************************************************
* djinterp [cpp][math]                                matrix_kind_arithmetic.hpp
*
* One part of matrix_kind.hpp (S437), its section 3.2: the arithmetic of
* matrix values - element by element, the product, the square algebra,
* powers and indexing. matrix_kind.hpp describes the kind whole.
*
* HOW TO USE
* ----------
*   Include matrix_kind.hpp, which includes the three parts in order, or
* this part alone for what it holds; each part includes those before it
* that it needs.
*
* OPTIONS
* -------
*   None: nothing here is chosen at run time.
*
* CONFIGURATION
* -------------
*   None of its own: from C++11, as the language is (S237).
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/matrix_kind_arithmetic.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
3.  MATRIX VALUES
    -------------
    2.  Arithmetic
         1.  Arrays
         2.  The dense type
         3.  Element by element
         4.  The product
         5.  expr_matrix_operation
         6.  The transpose
         7.  The square algebra
         8.  Powers
         9.  Indexing
*/

#ifndef DJINTERP_MATH_EXPR_MATRIX_KIND_ARITHMETIC_HPP
#define DJINTERP_MATH_EXPR_MATRIX_KIND_ARITHMETIC_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <iomanip>  // std::setprecision
#include <limits>   // std::numeric_limits
#include <locale>   // std::locale
#include <memory>   // std::unique_ptr
#include <new>      // std::bad_alloc
#include <sstream>  // std::ostringstream
#include <string>   // std::string, std::to_string
#include <vector>   // std::vector
// djinterp
#include "../../../../c/modules/math/cmat.h"    // d_cmat_determinant,
                                                // _inverse, _solve
#include "../../../../c/modules/math/value.h"   // dj_math_boxed_kind,
                                                // d_math_value_cmp
#include "../linear_algebra/linalg_common.hpp"  // linalg_kernel
#include "../math_common.hpp"                   // NS_MATH
#include "../value.hpp"                         // value
#include "./complex_kind.hpp"                   // expr_is_complex,
                                                // expr_conj
#include "./expr_common.hpp"                    // expr_result
#include "./quaternion_kind.hpp"                // the quaternion kind,
                                                // its operands
#include "./matrix_kind_storage.hpp"            // the part before it


NS_DJINTERP
NS_MATH

// 3.2    Arithmetic
//------------------------------------------------------------------------------
// Step 20's (3b) operations on matrix values, each computed by linear
// algebra's kernels (linear_algebra/linalg_common.hpp), so that each is
// written once (S247): a dense matrix's by the C core's, c/math/vec.h's and
// mat.h's, in the wider of its operands' types, each widened exactly first,
// as the tower would; a matrix holding values by the generic path, in
// math::value's own arithmetic, exact wherever its elements are. A result
// keeps each of its elements as it is made (S239).
NS_INTERNAL

    // 3.2.1
    // Arrays
    //   functions: a dense matrix's elements as an array of Type, no
    // narrower than its own, each widened exactly; any matrix's as values; a
    // matrix made of a dense array of Type, with the flags given; and one
    // made of values, by expr_matrix, as S239 stores them.

    // expr_matrix_storage_for
    //   function: the dense storage of Type.
    template<typename Type>
    D_CONSTEXPR expr_matrix_storage
    expr_matrix_storage_for();

    template<>
    D_CONSTEXPR expr_matrix_storage
    expr_matrix_storage_for<float>()
    {
        return expr_matrix_storage::floats;
    }

    template<>
    D_CONSTEXPR expr_matrix_storage
    expr_matrix_storage_for<double>()
    {
        return expr_matrix_storage::doubles;
    }

    template<>
    D_CONSTEXPR expr_matrix_storage
    expr_matrix_storage_for<long double>()
    {
        return expr_matrix_storage::long_doubles;
    }

    template<>
    D_CONSTEXPR expr_matrix_storage
    expr_matrix_storage_for<math::complex<float> >()
    {
        return expr_matrix_storage::complex_floats;
    }

    template<>
    D_CONSTEXPR expr_matrix_storage
    expr_matrix_storage_for<math::complex<double> >()
    {
        return expr_matrix_storage::complex_doubles;
    }

    template<>
    D_CONSTEXPR expr_matrix_storage
    expr_matrix_storage_for<math::complex<long double> >()
    {
        return expr_matrix_storage::complex_long_doubles;
    }

    template<>
    D_CONSTEXPR expr_matrix_storage
    expr_matrix_storage_for<math::quaternion<float> >()
    {
        return expr_matrix_storage::quaternion_floats;
    }

    template<>
    D_CONSTEXPR expr_matrix_storage
    expr_matrix_storage_for<math::quaternion<double> >()
    {
        return expr_matrix_storage::quaternion_doubles;
    }

    template<>
    D_CONSTEXPR expr_matrix_storage
    expr_matrix_storage_for<math::quaternion<long double> >()
    {
        return expr_matrix_storage::quaternion_long_doubles;
    }

    // expr_matrix_array
    //   function: a box's dense array of Type.
    template<typename Type>
    std::vector<Type>&
    expr_matrix_array(
        expr_matrix_box& _box
    );

    template<>
    inline std::vector<float>&
    expr_matrix_array<float>(
        expr_matrix_box& _box
    )
    {
        return _box.floats;
    }

    template<>
    inline std::vector<double>&
    expr_matrix_array<double>(
        expr_matrix_box& _box
    )
    {
        return _box.doubles;
    }

    template<>
    inline std::vector<long double>&
    expr_matrix_array<long double>(
        expr_matrix_box& _box
    )
    {
        return _box.long_doubles;
    }

    template<>
    inline std::vector<math::complex<float> >&
    expr_matrix_array<math::complex<float> >(
        expr_matrix_box& _box
    )
    {
        return _box.complex_floats;
    }

    template<>
    inline std::vector<math::complex<double> >&
    expr_matrix_array<math::complex<double> >(
        expr_matrix_box& _box
    )
    {
        return _box.complex_doubles;
    }

    template<>
    inline std::vector<math::complex<long double> >&
    expr_matrix_array<math::complex<long double> >(
        expr_matrix_box& _box
    )
    {
        return _box.complex_long_doubles;
    }

    template<>
    inline std::vector<math::quaternion<float> >&
    expr_matrix_array<math::quaternion<float> >(
        expr_matrix_box& _box
    )
    {
        return _box.quaternion_floats;
    }

    template<>
    inline std::vector<math::quaternion<double> >&
    expr_matrix_array<math::quaternion<double> >(
        expr_matrix_box& _box
    )
    {
        return _box.quaternion_doubles;
    }

    template<>
    inline std::vector<math::quaternion<long double> >&
    expr_matrix_array<math::quaternion<long double> >(
        expr_matrix_box& _box
    )
    {
        return _box.quaternion_long_doubles;
    }

    // expr_matrix_widened
    //   function: _from's elements, each widened exactly to Type.
    template<typename Type,
             typename From>
    std::vector<Type>
    expr_matrix_widened(
        const std::vector<From>& _from
    )
    {
        std::vector<Type> widened;

        widened.reserve(_from.size());

        // in order
        for (std::size_t i = 0; i < _from.size(); ++i)
        {
            widened.push_back(static_cast<Type>(_from[i]));
        }

        return widened;
    }

    // expr_matrix_dense_as
    //   functions: a dense box's elements in Type, no narrower than its own,
    // as the arithmetic asks them, in the wider of its operands' types: of
    // floats, its own; of doubles, its own or a float box's widened; of
    // long doubles, its own or a narrower box's widened.
    template<typename Type>
    std::vector<Type>
    expr_matrix_dense_as(
        const expr_matrix_box& _box
    );

    template<>
    inline std::vector<float>
    expr_matrix_dense_as<float>(
        const expr_matrix_box& _box
    )
    {
        return _box.floats;
    }

    template<>
    inline std::vector<double>
    expr_matrix_dense_as<double>(
        const expr_matrix_box& _box
    )
    {
        return ( (_box.storage == expr_matrix_storage::floats)
                     ? expr_matrix_widened<double>(_box.floats)
                     : _box.doubles );
    }

    template<>
    inline std::vector<long double>
    expr_matrix_dense_as<long double>(
        const expr_matrix_box& _box
    )
    {
        // by storage
        if (_box.storage == expr_matrix_storage::floats)
        {
            return expr_matrix_widened<long double>(_box.floats);
        }

        return ( (_box.storage == expr_matrix_storage::doubles)
                     ? expr_matrix_widened<long double>(_box.doubles)
                     : _box.long_doubles );
    }

    // expr_matrix_complex_as
    //   function: a dense box's elements as math::complex<Type>, Type no
    // narrower than its parts' type: a complex box's each part widened
    // exactly, and a real box's x as x + 0i (S348).
    template<typename Type,
             typename From>
    void
    expr_matrix_complex_from(
        std::vector<math::complex<Type> >&       _out,
        const std::vector<math::complex<From> >& _from
    )
    {
        _out.reserve(_from.size());

        // each part widened
        for (std::size_t i = 0; i < _from.size(); ++i)
        {
            _out.push_back(math::complex<Type>(
                static_cast<Type>(_from[i].real()),
                static_cast<Type>(_from[i].imag())));
        }

        return;
    }

    template<typename Type,
             typename From>
    void
    expr_matrix_complex_from(
        std::vector<math::complex<Type> >& _out,
        const std::vector<From>&           _from
    )
    {
        _out.reserve(_from.size());

        // x + 0i
        for (std::size_t i = 0; i < _from.size(); ++i)
        {
            _out.push_back(math::complex<Type>(static_cast<Type>(_from[i]),
                                               Type(0)));
        }

        return;
    }

    template<typename Type>
    std::vector<math::complex<Type> >
    expr_matrix_complex_as(
        const expr_matrix_box& _box
    )
    {
        std::vector<math::complex<Type> > out;

        // by storage, each no wider than Type
        switch (_box.storage)
        {
            case expr_matrix_storage::floats:
                expr_matrix_complex_from(out, _box.floats);
                break;

            case expr_matrix_storage::doubles:
                expr_matrix_complex_from(out, _box.doubles);
                break;

            case expr_matrix_storage::long_doubles:
                expr_matrix_complex_from(out, _box.long_doubles);
                break;

            case expr_matrix_storage::complex_floats:
                expr_matrix_complex_from(out, _box.complex_floats);
                break;

            case expr_matrix_storage::complex_doubles:
                expr_matrix_complex_from(out, _box.complex_doubles);
                break;

            // complex long doubles; a box of values, which no caller
            // passes, none, its complex long doubles being none
            case expr_matrix_storage::complex_long_doubles:
            default:
                expr_matrix_complex_from(out, _box.complex_long_doubles);
                break;
        }

        return out;
    }

    template<>
    inline std::vector<math::complex<float> >
    expr_matrix_dense_as<math::complex<float> >(
        const expr_matrix_box& _box
    )
    {
        return expr_matrix_complex_as<float>(_box);
    }

    template<>
    inline std::vector<math::complex<double> >
    expr_matrix_dense_as<math::complex<double> >(
        const expr_matrix_box& _box
    )
    {
        return expr_matrix_complex_as<double>(_box);
    }

    template<>
    inline std::vector<math::complex<long double> >
    expr_matrix_dense_as<math::complex<long double> >(
        const expr_matrix_box& _box
    )
    {
        return expr_matrix_complex_as<long double>(_box);
    }

    // expr_matrix_quaternion_as
    //   function: a dense box's elements as math::quaternion<Type>, Type no
    // narrower than its parts' type: a quaternion box's each part widened
    // exactly, a complex box's x + yi as x + yim + 0jm + 0km, and a real
    // box's x as x + 0im + 0jm + 0km (S386).
    template<typename Type,
             typename From>
    void
    expr_matrix_quaternion_from(
        std::vector<math::quaternion<Type> >&       _out,
        const std::vector<math::quaternion<From> >& _from
    )
    {
        _out.reserve(_from.size());

        // each part widened
        for (std::size_t i = 0; i < _from.size(); ++i)
        {
            _out.push_back(math::quaternion<Type>(
                static_cast<Type>(_from[i].m_x),
                static_cast<Type>(_from[i].m_y),
                static_cast<Type>(_from[i].m_z),
                static_cast<Type>(_from[i].m_w)));
        }

        return;
    }

    template<typename Type,
             typename From>
    void
    expr_matrix_quaternion_from(
        std::vector<math::quaternion<Type> >&    _out,
        const std::vector<math::complex<From> >& _from
    )
    {
        _out.reserve(_from.size());

        // x + yim + 0jm + 0km
        for (std::size_t i = 0; i < _from.size(); ++i)
        {
            _out.push_back(math::quaternion<Type>(
                static_cast<Type>(_from[i].imag()),
                Type(0),
                Type(0),
                static_cast<Type>(_from[i].real())));
        }

        return;
    }

    template<typename Type,
             typename From>
    void
    expr_matrix_quaternion_from(
        std::vector<math::quaternion<Type> >& _out,
        const std::vector<From>&              _from
    )
    {
        _out.reserve(_from.size());

        // x + 0im + 0jm + 0km
        for (std::size_t i = 0; i < _from.size(); ++i)
        {
            _out.push_back(math::quaternion<Type>(Type(0),
                                                  Type(0),
                                                  Type(0),
                                                  static_cast<Type>(
                                                      _from[i])));
        }

        return;
    }

    template<typename Type>
    std::vector<math::quaternion<Type> >
    expr_matrix_quaternion_as(
        const expr_matrix_box& _box
    )
    {
        std::vector<math::quaternion<Type> > out;

        // by storage, each no wider than Type
        switch (_box.storage)
        {
            case expr_matrix_storage::floats:
                expr_matrix_quaternion_from(out, _box.floats);
                break;

            case expr_matrix_storage::doubles:
                expr_matrix_quaternion_from(out, _box.doubles);
                break;

            case expr_matrix_storage::long_doubles:
                expr_matrix_quaternion_from(out, _box.long_doubles);
                break;

            case expr_matrix_storage::complex_floats:
                expr_matrix_quaternion_from(out, _box.complex_floats);
                break;

            case expr_matrix_storage::complex_doubles:
                expr_matrix_quaternion_from(out, _box.complex_doubles);
                break;

            case expr_matrix_storage::complex_long_doubles:
                expr_matrix_quaternion_from(out, _box.complex_long_doubles);
                break;

            case expr_matrix_storage::quaternion_floats:
                expr_matrix_quaternion_from(out, _box.quaternion_floats);
                break;

            case expr_matrix_storage::quaternion_doubles:
                expr_matrix_quaternion_from(out, _box.quaternion_doubles);
                break;

            // quaternions of long doubles; a box of values, which no caller
            // passes, none, its quaternions of long doubles being none
            case expr_matrix_storage::quaternion_long_doubles:
            default:
                expr_matrix_quaternion_from(out,
                                            _box.quaternion_long_doubles);
                break;
        }

        return out;
    }

    template<>
    inline std::vector<math::quaternion<float> >
    expr_matrix_dense_as<math::quaternion<float> >(
        const expr_matrix_box& _box
    )
    {
        return expr_matrix_quaternion_as<float>(_box);
    }

    template<>
    inline std::vector<math::quaternion<double> >
    expr_matrix_dense_as<math::quaternion<double> >(
        const expr_matrix_box& _box
    )
    {
        return expr_matrix_quaternion_as<double>(_box);
    }

    template<>
    inline std::vector<math::quaternion<long double> >
    expr_matrix_dense_as<math::quaternion<long double> >(
        const expr_matrix_box& _box
    )
    {
        return expr_matrix_quaternion_as<long double>(_box);
    }

    // expr_matrix_held_of, expr_matrix_operands_as
    //   functions: how many parts each element of a dense storage holds, 1
    // for a real one, 2 for a complex one and 4 for a quaternion one; and a
    // dense box's elements as the quaternion kind's operands in Type, each
    // holding its storage's parts, so that a real or complex element is
    // taken as itself (quaternion_kind.hpp, S387).
    inline std::size_t
    expr_matrix_held_of(
        expr_matrix_storage _storage
    )
    {
        return ( expr_storage_is_quaternion(_storage) ? 4
               : expr_storage_is_complex(_storage)    ? 2
                                                      : 1 );
    }

    template<typename Type>
    std::vector<expr_quaternion_operand<Type> >
    expr_matrix_operands_as(
        const expr_matrix_box& _box
    )
    {
        const std::vector<math::quaternion<Type> > q =
            expr_matrix_quaternion_as<Type>(_box);
        const std::size_t held = expr_matrix_held_of(_box.storage);
        std::vector<expr_quaternion_operand<Type> > operands;

        operands.reserve(q.size());

        // each holding its storage's parts
        for (std::size_t i = 0; i < q.size(); ++i)
        {
            operands.push_back(expr_quaternion_operand_made(q[i], held));
        }

        return operands;
    }

    // expr_matrix_values_of
    //   function: a matrix's elements, row by row, as values, a dense
    // one's each with the matrix's flags, so that none is lost (G2).
    inline std::vector<value>
    expr_matrix_values_of(
        const value& _a
    )
    {
        const expr_matrix_box& box   = expr_matrix_box_of(_a.c_value());
        const std::size_t      count = (box.shape.rows * box.shape.columns);
        std::vector<value>     elements;

        elements.reserve(count);

        // row by row
        for (std::size_t i = 0; i < count; ++i)
        {
            elements.push_back(expr_matrix_element_of(_a,
                                                      i));
        }

        return elements;
    }

    // expr_matrix_of_dense
    //   function: a matrix of _shape holding _dense, which it takes, with
    // _flags.
    template<typename Type>
    value
    expr_matrix_of_dense(
        const expr_shape&  _shape,
        std::vector<Type>& _dense,
        unsigned           _flags
    )
    {
        std::unique_ptr<expr_matrix_box> box(
            new expr_matrix_box(_shape,
                                expr_matrix_storage_for<Type>()));

        expr_matrix_array<Type>(*box).swap(_dense);

        return expr_matrix_adopt(box.release(),
                                 _flags);
    }

    // expr_matrix_of_operands
    //   function: a matrix of _shape of the quaternions of Type the operands
    // _operands hold, with _flags (S386).
    template<typename Type>
    value
    expr_matrix_of_operands(
        const expr_shape&                                  _shape,
        const std::vector<expr_quaternion_operand<Type> >& _operands,
        unsigned                                           _flags
    )
    {
        std::vector<math::quaternion<Type> > dense;

        dense.reserve(_operands.size());

        // each operand's parts
        for (std::size_t i = 0; i < _operands.size(); ++i)
        {
            dense.push_back(_operands[i].q);
        }

        return expr_matrix_of_dense<math::quaternion<Type> >(_shape,
                                                             dense,
                                                             _flags);
    }

    // expr_matrix_of_values
    //   function: a matrix of _shape of _elements, by expr_matrix: the first
    // element that is an error, where one is; a domain error, where one is
    // no number; and otherwise each kept as it is (S239).
    inline value
    expr_matrix_of_values(
        const expr_shape&         _shape,
        const std::vector<value>& _elements
    )
    {
        const expr_result<value> made = expr_matrix(_shape.rows,
                                                    _shape.columns,
                                                    _elements);

        // the shape is the elements': never bad_shape
        return ( made.is_ok() ? made.value()
                              : value(D_MATH_VALUE_ERROR_DOMAIN) );
    }

    // expr_matrix_side
    //   struct: a dense operand of complex arithmetic in Type: its
    // elements as Type where it is real, and as math::complex<Type> where it
    // is complex, the other array empty, so that a real operand is taken as
    // itself, as C99's G.5 takes one (S349).
    template<typename Type>
    struct expr_matrix_side
    {
        bool                              complex;
        std::vector<Type>                 real;
        std::vector<math::complex<Type> > z;
    };

    template<typename Type>
    expr_matrix_side<Type>
    expr_matrix_side_of(
        const expr_matrix_box& _box
    )
    {
        expr_matrix_side<Type> side;

        side.complex = expr_storage_is_complex(_box.storage);

        // as it is held
        if (side.complex)
        {
            side.z = expr_matrix_complex_as<Type>(_box);
        }
        else
        {
            side.real = expr_matrix_dense_as<Type>(_box);
        }

        return side;
    }

    // expr_matrix_combined
    //   function: _x + _y, or _x - _y where _minus, either real or
    // complex, by math::complex's operators, cplx.h's: a real operand
    // taken as itself.
    template<typename Type,
             typename X,
             typename Y>
    math::complex<Type>
    expr_matrix_combined(
        const X& _x,
        const Y& _y,
        bool     _minus
    )
    {
        return ( _minus ? math::complex<Type>(_x - _y)
                        : math::complex<Type>(_x + _y) );
    }

    // expr_matrix_sum_complex
    //   function: A + B, or A - B where _minus, of one shape, dense, one of
    // them complex at least, in math::complex<Type>, element by element, a
    // real operand's element taken as itself (S349).
    template<typename Type>
    value
    expr_matrix_sum_complex(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        bool                   _minus,
        unsigned               _flags
    )
    {
        typedef math::complex<Type> complex_type;

        const expr_matrix_side<Type> a = expr_matrix_side_of<Type>(_a);
        const expr_matrix_side<Type> b = expr_matrix_side_of<Type>(_b);
        const std::size_t            n = (_a.shape.rows * _a.shape.columns);
        std::vector<complex_type>    out(n);

        // element by element, each side as it is held
        for (std::size_t i = 0; i < n; ++i)
        {
            out[i] = ( (a.complex)
                           ? ( (b.complex)
                                   ? expr_matrix_combined<Type>(a.z[i],
                                                                b.z[i],
                                                                _minus)
                                   : expr_matrix_combined<Type>(a.z[i],
                                                                b.real[i],
                                                                _minus) )
                           : expr_matrix_combined<Type>(a.real[i],
                                                        b.z[i],
                                                        _minus) );
        }

        return expr_matrix_of_dense<complex_type>(_a.shape,
                                                  out,
                                                  _flags);
    }

    // expr_matrix_scaled_complex
    //   function: each element of a dense A times _number, or divided by it
    // where _divide, A or the number complex, in math::complex<Type>: the
    // number its nearest there, a real one as itself, by math::complex's
    // operators, cplx.h's (S349).
    template<typename Type,
             typename X,
             typename Y>
    math::complex<Type>
    expr_matrix_by(
        const X& _x,
        const Y& _y,
        bool     _divide
    )
    {
        return ( _divide ? math::complex<Type>(_x / _y)
                         : math::complex<Type>(_x * _y) );
    }

    template<typename Type>
    value
    expr_matrix_scaled_complex(
        const expr_matrix_box& _a,
        const value&           _number,
        bool                   _divide,
        unsigned               _flags
    )
    {
        typedef math::complex<Type> complex_type;

        const expr_matrix_side<Type> a       = expr_matrix_side_of<Type>(_a);
        const bool                   complex = expr_is_complex(_number);
        const std::size_t            n       = ( _a.shape.rows *
                                                 _a.shape.columns );
        std::vector<complex_type>    out(n);
        complex_type                 z;
        Type                         x = Type(0);

        // the number, its nearest, real or complex
        if (complex)
        {
            static_cast<void>(expr_complex_to(_number,
                                              z));
        }
        else
        {
            static_cast<void>(_number.to(x));
        }

        // element by element, each side as it is held
        for (std::size_t i = 0; i < n; ++i)
        {
            out[i] = ( (a.complex)
                           ? ( (complex)
                                   ? expr_matrix_by<Type>(a.z[i],
                                                          z,
                                                          _divide)
                                   : expr_matrix_by<Type>(a.z[i],
                                                          x,
                                                          _divide) )
                           : expr_matrix_by<Type>(a.real[i],
                                                  z,
                                                  _divide) );
        }

        return expr_matrix_of_dense<complex_type>(_a.shape,
                                                  out,
                                                  _flags);
    }

    // expr_matrix_product_complex
    //   function: A * B, dense, one of them complex at least, in
    // math::complex<Type>: each element a row's products with a column's,
    // each part rounded once from Dot2's pair, as the kernels' product
    // rounds a math::complex<T> matrix's, bit for bit (S349, S412); a real
    // operand's element taken as itself, its products with the other's
    // parts alone; where an operand has a NaN or an infinity, the sum in
    // order from 0 of math::complex's products, as written.
    template<typename Type,
             typename X,
             typename Y>
    void
    expr_matrix_multiplied(
        std::vector<math::complex<Type> >& _out,
        const std::vector<X>&              _a,
        const std::vector<Y>&              _b,
        std::size_t                        _rows,
        std::size_t                        _inner,
        std::size_t                        _columns
    )
    {
        typedef linalg::internal::linalg_sums<math::complex<Type> > sums;

        // each row by each column; with no inner terms, each element +0
        for (std::size_t i = 0; i < _rows; ++i)
        {
            for (std::size_t j = 0; j < _columns; ++j)
            {
                _out[(i * _columns) + j] =
                    (_inner == 0)
                        ? math::complex<Type>()
                        : sums::dot2(math::complex<Type>(), false,
                                     _a.data() + (i * _inner), 1,
                                     _b.data() + j, _columns, _b.data(),
                                     false, _inner);
            }
        }

        return;
    }

    template<typename Type>
    value
    expr_matrix_product_complex(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        unsigned               _flags
    )
    {
        typedef math::complex<Type> complex_type;

        const expr_matrix_side<Type> a = expr_matrix_side_of<Type>(_a);
        const expr_matrix_side<Type> b = expr_matrix_side_of<Type>(_b);
        const std::size_t            rows    = _a.shape.rows;
        const std::size_t            inner   = _a.shape.columns;
        const std::size_t            columns = _b.shape.columns;
        std::vector<complex_type>    out(rows * columns);

        // each side as it is held
        if ( (a.complex) &&
             (b.complex) )
        {
            expr_matrix_multiplied<Type>(out, a.z, b.z, rows, inner, columns);
        }
        else if (a.complex)
        {
            expr_matrix_multiplied<Type>(out, a.z, b.real, rows, inner,
                                         columns);
        }
        else
        {
            expr_matrix_multiplied<Type>(out, a.real, b.z, rows, inner,
                                         columns);
        }

        return expr_matrix_of_dense<complex_type>(expr_shape(rows,
                                                             columns),
                                                  out,
                                                  _flags);
    }

    // expr_matrix_reordered_complex
    //   function: -A, or A's transpose, conjugated where _conjugate, or
    // neither, of a complex dense A in math::complex<Type>: each element
    // negated by cplx.h's negation, or moved and conjugated by its conj.
    template<typename Type>
    value
    expr_matrix_reordered_complex(
        const expr_matrix_box& _a,
        bool                   _negate,
        bool                   _transpose,
        bool                   _conjugate,
        unsigned               _flags
    )
    {
        typedef math::complex<Type> complex_type;

        const std::vector<complex_type> a       =
            expr_matrix_complex_as<Type>(_a);
        const std::size_t               rows    = _a.shape.rows;
        const std::size_t               columns = _a.shape.columns;
        std::vector<complex_type>       out(a.size());

        // each element to its place
        for (std::size_t i = 0; i < rows; ++i)
        {
            for (std::size_t j = 0; j < columns; ++j)
            {
                const complex_type& x     = a[(i * columns) + j];
                const std::size_t   place = ( _transpose
                                                  ? ((j * rows) + i)
                                                  : ((i * columns) + j) );

                out[place] = ( _negate ? complex_type(-x)
                             : _conjugate ? complex_type(conj(x))
                                          : x );
            }
        }

        return expr_matrix_of_dense<complex_type>(
            ( _transpose ? expr_shape(columns,
                                      rows)
                         : _a.shape ),
            out,
            _flags);
    }

    // expr_matrix_sum_quaternion
    //   function: A + B, or A - B where _minus, of one shape, dense, one of
    // them of quaternions at least, in Type: the kernels' sum of the
    // quaternion kind's operands, each element as the kind adds its two, a
    // real or complex one taken as itself (S387).
    template<typename Type>
    value
    expr_matrix_sum_quaternion(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        bool                   _minus,
        unsigned               _flags
    )
    {
        typedef expr_quaternion_operand<Type>            operand;
        typedef linalg::internal::linalg_kernel<operand> kernel;

        const std::vector<operand> a = expr_matrix_operands_as<Type>(_a);
        const std::vector<operand> b = expr_matrix_operands_as<Type>(_b);
        std::vector<operand>       out(a.size());

        // the kernel of the sum, or of the difference
        if (_minus)
        {
            kernel::sub(out.data(),
                        a.data(),
                        b.data(),
                        a.size());
        }
        else
        {
            kernel::add(out.data(),
                        a.data(),
                        b.data(),
                        a.size());
        }

        return expr_matrix_of_operands<Type>(_a.shape,
                                             out,
                                             _flags);
    }

    // expr_matrix_scaled_quaternion
    //   function: each element of a dense A times _number, on its right, or
    // on its left where _left, or divided by it, on the right, where
    // _divide, A or the number holding quaternions, in Type: the number its
    // nearest operand there, each element as the kind multiplies or divides
    // its two, in their order (S277, S387).
    template<typename Type>
    value
    expr_matrix_scaled_quaternion(
        const expr_matrix_box& _a,
        const value&           _number,
        bool                   _divide,
        bool                   _left,
        unsigned               _flags
    )
    {
        typedef expr_quaternion_operand<Type> operand;

        const std::vector<operand> a      = expr_matrix_operands_as<Type>(_a);
        const operand              number = expr_quaternion_operand_of<Type>(
            expr_quaternion_parts_of(_number.c_value()));
        std::vector<operand>       out(a.size());

        // each element, the number on its side
        for (std::size_t i = 0; i < a.size(); ++i)
        {
            out[i] = ( (_divide) ? (a[i] / number)
                     : (_left)   ? (number * a[i])
                                 : (a[i] * number) );
        }

        return expr_matrix_of_operands<Type>(_a.shape,
                                             out,
                                             _flags);
    }

    // expr_matrix_product_quaternion
    //   function: A * B, dense, one of them of quaternions at least, in
    // Type: the kernels' product of the quaternion kind's operands, each
    // element a row's products with a column's, a_ik b_kj in that order,
    // summed from 0 in order (S277, S387).
    template<typename Type>
    value
    expr_matrix_product_quaternion(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        unsigned               _flags
    )
    {
        typedef expr_quaternion_operand<Type>            operand;
        typedef linalg::internal::linalg_kernel<operand> kernel;

        const std::vector<operand> a = expr_matrix_operands_as<Type>(_a);
        const std::vector<operand> b = expr_matrix_operands_as<Type>(_b);
        const std::size_t          rows    = _a.shape.rows;
        const std::size_t          inner   = _a.shape.columns;
        const std::size_t          columns = _b.shape.columns;
        std::vector<operand>       out(rows * columns);

        kernel::multiply(out.data(),
                         a.data(),
                         b.data(),
                         rows,
                         inner,
                         columns);

        return expr_matrix_of_operands<Type>(expr_shape(rows,
                                                        columns),
                                             out,
                                             _flags);
    }

    // expr_matrix_reordered_quaternion
    //   function: -A, or A's transpose, conjugated where _conjugate, or
    // neither, of a dense A of quaternions in Type: each element negated,
    // each part's sign flipped, as the kind negates one, or moved and
    // conjugated by quat.h's conjugate.
    template<typename Type>
    value
    expr_matrix_reordered_quaternion(
        const expr_matrix_box& _a,
        bool                   _negate,
        bool                   _transpose,
        bool                   _conjugate,
        unsigned               _flags
    )
    {
        typedef math::quaternion<Type> quaternion_type;

        const std::vector<quaternion_type> a       =
            expr_matrix_quaternion_as<Type>(_a);
        const std::size_t                  rows    = _a.shape.rows;
        const std::size_t                  columns = _a.shape.columns;
        std::vector<quaternion_type>       out(a.size());

        // each element to its place
        for (std::size_t i = 0; i < rows; ++i)
        {
            for (std::size_t j = 0; j < columns; ++j)
            {
                const quaternion_type& x     = a[(i * columns) + j];
                const std::size_t      place = ( _transpose
                                                     ? ((j * rows) + i)
                                                     : ((i * columns) + j) );

                out[place] = ( _negate ? quaternion_type(-x.m_x,
                                                         -x.m_y,
                                                         -x.m_z,
                                                         -x.m_w)
                             : _conjugate ? conjugate(x)
                                          : x );
            }
        }

        return expr_matrix_of_dense<quaternion_type>(
            ( _transpose ? expr_shape(columns,
                                      rows)
                         : _a.shape ),
            out,
            _flags);
    }

    // 3.2.2
    // The dense type
    //   functions: the storage two matrices' arithmetic is computed in, and
    // a matrix's and a number's: the wider of two dense types, of
    // quaternions where either is, else complex where either is
    // (expr_matrix_wider, 2.1.4); a number's own, a float's, a double's or a
    // long double's, or a complex number's or a quaternion's of such parts;
    // or an exact number's, which takes the matrix's, as the tower converts
    // an exact number to the inexact one's type, complex or of quaternions
    // where it is; and values, where either holds values or the number is a
    // boxed inexact one, which handles each element itself.
    inline expr_matrix_storage
    expr_matrix_number_storage(
        const value& _number
    )
    {
        // by kind: the floating-point ones their own
        switch (_number.kind())
        {
            case D_MATH_VALUE_FLOAT:
                return expr_matrix_storage::floats;

            case D_MATH_VALUE_DOUBLE:
                return expr_matrix_storage::doubles;

            case D_MATH_VALUE_LONG_DOUBLE:
                return expr_matrix_storage::long_doubles;

            default:
                break;
        }

        // a complex number: of its parts' type, complex; an exact one the
        // matrix's type, complex
        if (expr_is_complex(_number))
        {
            return ( _number.is_exact()
                         ? expr_matrix_storage::complex_floats
                         : expr_matrix_element_storage(_number) );
        }

        // a quaternion: of its parts' type, of quaternions; an exact one the
        // matrix's type, of quaternions
        if (expr_is_quaternion(_number))
        {
            return ( _number.is_exact()
                         ? expr_matrix_storage::quaternion_floats
                         : expr_matrix_element_storage(_number) );
        }

        // an exact number below the float in the tower: the matrix's type
        if ( (_number.is_exact()) &&
             (_number.rank() < D_MATH_VALUE_RANK_FLOAT) )
        {
            return expr_matrix_storage::floats;
        }

        return expr_matrix_storage::values;
    }

    // 3.2.3
    // Element by element
    //   functions: A + B and A - B, of one shape; A * b, b * A and A / b,
    // each element by the number; and -A. Each by the kernels, dense in the
    // operands' wider type, else on values.

    // expr_matrix_sum_in
    //   function: A + B, or A - B where _minus, both dense, in Type.
    template<typename Type>
    value
    expr_matrix_sum_in(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        bool                   _minus,
        unsigned               _flags
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        const std::vector<Type> a   = expr_matrix_dense_as<Type>(_a);
        const std::vector<Type> b   = expr_matrix_dense_as<Type>(_b);
        std::vector<Type>       out(a.size());

        // the kernel of the sum, or of the difference
        if (_minus)
        {
            kernel::sub(out.data(),
                        a.data(),
                        b.data(),
                        a.size());
        }
        else
        {
            kernel::add(out.data(),
                        a.data(),
                        b.data(),
                        a.size());
        }

        return expr_matrix_of_dense<Type>(_a.shape,
                                          out,
                                          _flags);
    }

    // expr_matrix_sum
    //   function: A + B, or A - B where _minus, of two matrices of one shape;
    // a domain error for two shapes.
    inline value
    expr_matrix_sum(
        const value& _a,
        const value& _b,
        bool         _minus
    )
    {
        const expr_matrix_box& a     = expr_matrix_box_of(_a.c_value());
        const expr_matrix_box& b     = expr_matrix_box_of(_b.c_value());
        const unsigned         flags = (_a.flags() | _b.flags());

        // two shapes
        if (a.shape != b.shape)
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // by the type it is computed in
        switch (expr_matrix_wider(a.storage,
                                  b.storage))
        {
            case expr_matrix_storage::floats:
                return expr_matrix_sum_in<float>(a,
                                                 b,
                                                 _minus,
                                                 flags);

            case expr_matrix_storage::doubles:
                return expr_matrix_sum_in<double>(a,
                                                  b,
                                                  _minus,
                                                  flags);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_sum_in<long double>(a,
                                                       b,
                                                       _minus,
                                                       flags);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_sum_complex<float>(a,
                                                      b,
                                                      _minus,
                                                      flags);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_sum_complex<double>(a,
                                                       b,
                                                       _minus,
                                                       flags);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_sum_complex<long double>(a,
                                                            b,
                                                            _minus,
                                                            flags);

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_sum_quaternion<float>(a,
                                                         b,
                                                         _minus,
                                                         flags);

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_sum_quaternion<double>(a,
                                                          b,
                                                          _minus,
                                                          flags);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_sum_quaternion<long double>(a,
                                                               b,
                                                               _minus,
                                                               flags);

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> x = expr_matrix_values_of(_a);
        const std::vector<value> y = expr_matrix_values_of(_b);
        std::vector<value>       out(x.size());

        // on values
        if (_minus)
        {
            kernel::sub(out.data(),
                        x.data(),
                        y.data(),
                        x.size());
        }
        else
        {
            kernel::add(out.data(),
                        x.data(),
                        y.data(),
                        x.size());
        }

        return expr_matrix_of_values(a.shape,
                                     out);
    }

    // expr_matrix_scaled_in
    //   function: each element of a dense A times _number, or divided by it
    // where _divide, in Type, the number converted to Type as the tower
    // converts it.
    template<typename Type>
    value
    expr_matrix_scaled_in(
        const expr_matrix_box& _a,
        const value&           _number,
        bool                   _divide,
        unsigned               _flags
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        const std::vector<Type> a      = expr_matrix_dense_as<Type>(_a);
        std::vector<Type>       out(a.size());
        Type                    number = Type(0);

        static_cast<void>(_number.to(number));

        // the kernel of the product, or of the quotient
        if (_divide)
        {
            kernel::divide(out.data(),
                           a.data(),
                           number,
                           a.size());
        }
        else
        {
            kernel::scale(out.data(),
                          a.data(),
                          number,
                          a.size());
        }

        return expr_matrix_of_dense<Type>(_a.shape,
                                          out,
                                          _flags);
    }

    // expr_matrix_noncommuting
    //   function: whether a matrix's product with a number hangs on their
    // order: where the number is a quaternion or the matrix holds one.
    inline bool
    expr_matrix_noncommuting(
        const value& _a,
        const value& _number
    );

    // expr_matrix_scaled_values
    //   function: expr_matrix_scaled on values, each element the value's
    // own product or quotient: the number on the left where _left and the
    // order counts, a quaternion among the two (S387).
    inline value
    expr_matrix_scaled_values(
        const value& _a,
        const value& _number,
        bool         _divide,
        bool         _left
    )
    {
        typedef linalg::internal::linalg_kernel<value> kernel;

        const expr_matrix_box&   a = expr_matrix_box_of(_a.c_value());
        const std::vector<value> x = expr_matrix_values_of(_a);
        std::vector<value>       out(x.size());

        // the number on the left, where the order counts
        if ( (_left) &&
             (!_divide) &&
             (expr_matrix_noncommuting(_a, _number)) )
        {
            for (std::size_t i = 0; i < x.size(); ++i)
            {
                out[i] = (_number * x[i]);
            }
        }
        else if (_divide)
        {
            kernel::divide(out.data(),
                           x.data(),
                           _number,
                           x.size());
        }
        else
        {
            kernel::scale(out.data(),
                          x.data(),
                          _number,
                          x.size());
        }

        return expr_matrix_of_values(a.shape,
                                     out);
    }

    // expr_matrix_scaled
    //   function: each element of A times _number, on its right, or on its
    // left where _left, or divided by it, on the right, where _divide, each
    // product in its order (S277): a real or complex number's product with
    // A is A's with it, as the product of two such numbers does not hang on
    // their order.
    inline value
    expr_matrix_scaled(
        const value& _a,
        const value& _number,
        bool         _divide,
        bool         _left = false
    )
    {
        const expr_matrix_box& a     = expr_matrix_box_of(_a.c_value());
        const unsigned         flags = (_a.flags() | _number.flags());

        // by the type it is computed in
        switch (expr_matrix_wider(a.storage,
                                  expr_matrix_number_storage(_number)))
        {
            case expr_matrix_storage::floats:
                return expr_matrix_scaled_in<float>(a,
                                                    _number,
                                                    _divide,
                                                    flags);

            case expr_matrix_storage::doubles:
                return expr_matrix_scaled_in<double>(a,
                                                     _number,
                                                     _divide,
                                                     flags);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_scaled_in<long double>(a,
                                                          _number,
                                                          _divide,
                                                          flags);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_scaled_complex<float>(a,
                                                         _number,
                                                         _divide,
                                                         flags);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_scaled_complex<double>(a,
                                                          _number,
                                                          _divide,
                                                          flags);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_scaled_complex<long double>(a,
                                                               _number,
                                                               _divide,
                                                               flags);

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_scaled_quaternion<float>(a,
                                                            _number,
                                                            _divide,
                                                            _left,
                                                            flags);

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_scaled_quaternion<double>(a,
                                                             _number,
                                                             _divide,
                                                             _left,
                                                             flags);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_scaled_quaternion<long double>(a,
                                                                  _number,
                                                                  _divide,
                                                                  _left,
                                                                  flags);

            case expr_matrix_storage::values:
            default:
                break;
        }

        return expr_matrix_scaled_values(_a,
                                         _number,
                                         _divide,
                                         _left);
    }

    // expr_matrix_negated_in
    //   function: -A, dense, in its own type.
    template<typename Type>
    value
    expr_matrix_negated_in(
        const expr_matrix_box& _a,
        unsigned               _flags
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        const std::vector<Type> a = expr_matrix_dense_as<Type>(_a);
        std::vector<Type>       out(a.size());

        kernel::negate(out.data(),
                       a.data(),
                       a.size());

        return expr_matrix_of_dense<Type>(_a.shape,
                                          out,
                                          _flags);
    }

    inline value
    expr_matrix_negated(
        const value& _a
    )
    {
        const expr_matrix_box& a = expr_matrix_box_of(_a.c_value());

        // by storage
        switch (a.storage)
        {
            case expr_matrix_storage::floats:
                return expr_matrix_negated_in<float>(a,
                                                     _a.flags());

            case expr_matrix_storage::doubles:
                return expr_matrix_negated_in<double>(a,
                                                      _a.flags());

            case expr_matrix_storage::long_doubles:
                return expr_matrix_negated_in<long double>(a,
                                                           _a.flags());

            case expr_matrix_storage::complex_floats:
                return expr_matrix_reordered_complex<float>(a, true, false,
                                                            false,
                                                            _a.flags());

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_reordered_complex<double>(a, true, false,
                                                             false,
                                                             _a.flags());

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_reordered_complex<long double>(
                    a, true, false, false, _a.flags());

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_reordered_quaternion<float>(
                    a, true, false, false, _a.flags());

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_reordered_quaternion<double>(
                    a, true, false, false, _a.flags());

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_reordered_quaternion<long double>(
                    a, true, false, false, _a.flags());

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> x = expr_matrix_values_of(_a);
        std::vector<value>       out(x.size());

        kernel::negate(out.data(),
                       x.data(),
                       x.size());

        return expr_matrix_of_values(a.shape,
                                     out);
    }

    // 3.2.4
    // The product
    //   function: A * B, A's columns as many as B's rows, the matrix of
    // A's rows' products with B's columns, by the kernels' product; a domain
    // error otherwise.
    template<typename Type>
    value
    expr_matrix_product_in(
        const expr_matrix_box& _a,
        const expr_matrix_box& _b,
        unsigned               _flags
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        const std::vector<Type> a = expr_matrix_dense_as<Type>(_a);
        const std::vector<Type> b = expr_matrix_dense_as<Type>(_b);
        std::vector<Type>       out(_a.shape.rows * _b.shape.columns);

        kernel::multiply(out.data(),
                         a.data(),
                         b.data(),
                         _a.shape.rows,
                         _a.shape.columns,
                         _b.shape.columns);

        const expr_shape shape(_a.shape.rows,
                               _b.shape.columns);

        return expr_matrix_of_dense<Type>(shape,
                                          out,
                                          _flags);
    }

    inline value
    expr_matrix_product(
        const value& _a,
        const value& _b
    )
    {
        const expr_matrix_box& a     = expr_matrix_box_of(_a.c_value());
        const expr_matrix_box& b     = expr_matrix_box_of(_b.c_value());
        const unsigned         flags = (_a.flags() | _b.flags());

        // A's columns not as many as B's rows
        if (a.shape.columns != b.shape.rows)
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // by the type it is computed in
        switch (expr_matrix_wider(a.storage,
                                  b.storage))
        {
            case expr_matrix_storage::floats:
                return expr_matrix_product_in<float>(a,
                                                     b,
                                                     flags);

            case expr_matrix_storage::doubles:
                return expr_matrix_product_in<double>(a,
                                                      b,
                                                      flags);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_product_in<long double>(a,
                                                           b,
                                                           flags);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_product_complex<float>(a,
                                                          b,
                                                          flags);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_product_complex<double>(a,
                                                           b,
                                                           flags);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_product_complex<long double>(a,
                                                                b,
                                                                flags);

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_product_quaternion<float>(a,
                                                             b,
                                                             flags);

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_product_quaternion<double>(a,
                                                              b,
                                                              flags);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_product_quaternion<long double>(a,
                                                                   b,
                                                                   flags);

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> x = expr_matrix_values_of(_a);
        const std::vector<value> y = expr_matrix_values_of(_b);
        std::vector<value>       out(a.shape.rows * b.shape.columns);

        kernel::multiply(out.data(),
                         x.data(),
                         y.data(),
                         a.shape.rows,
                         a.shape.columns,
                         b.shape.columns);

        return expr_matrix_of_values(expr_shape(a.shape.rows,
                                                b.shape.columns),
                                     out);
    }

    // 3.2.5
    // expr_matrix_operation
    //   function: _a _op _b, one of them a matrix, as the kind's binary entry
    // computes it (2.2.2), a complex number and a quaternion among the
    // numbers it takes, a product of a number and a matrix in its order
    // (S349, S387).
    inline value
    expr_matrix_operation(
        dj_math_value_op _op,
        const value&     _a,
        const value&     _b
    )
    {
        const bool a = expr_matrix_held(_a.c_value());
        const bool b = expr_matrix_held(_b.c_value());

        // two matrices: a sum, a difference or a product; no quotient
        if ( (a) &&
             (b) )
        {
            if ( (_op == D_MATH_VALUE_OP_ADD) ||
                 (_op == D_MATH_VALUE_OP_SUB) )
            {
                return expr_matrix_sum(_a,
                                       _b,
                                       (_op == D_MATH_VALUE_OP_SUB));
            }

            return ( (_op == D_MATH_VALUE_OP_MUL)
                         ? expr_matrix_product(_a,
                                               _b)
                         : value(D_MATH_VALUE_ERROR_DOMAIN) );
        }

        // a matrix and a number: a product either way, or A / b
        if ( (_op == D_MATH_VALUE_OP_MUL) ||
             ( (_op == D_MATH_VALUE_OP_DIV) &&
               (a) ) )
        {
            return expr_matrix_scaled( (a ? _a : _b),
                                       (a ? _b : _a),
                                       (_op == D_MATH_VALUE_OP_DIV),
                                       (!a) );
        }

        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    // 3.2.6
    // The transpose
    //   function: A's transpose, the matrix of A's columns as its rows, by
    // the kernels' transpose, each element as it is; or, where _conjugate,
    // the conjugate transpose, A', each complex element and quaternion
    // conjugated and each real one as it is (S273, S277, S350).
    template<typename Type>
    value
    expr_matrix_transposed_in(
        const expr_matrix_box& _a,
        unsigned               _flags
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        const std::vector<Type> a = expr_matrix_dense_as<Type>(_a);
        std::vector<Type>       out(a.size());

        kernel::transpose(out.data(),
                          a.data(),
                          _a.shape.rows,
                          _a.shape.columns);

        const expr_shape shape(_a.shape.columns,
                               _a.shape.rows);

        return expr_matrix_of_dense<Type>(shape,
                                          out,
                                          _flags);
    }

    inline value
    expr_matrix_transposed(
        const value& _a,
        bool         _conjugate = true
    )
    {
        const expr_matrix_box& a = expr_matrix_box_of(_a.c_value());

        // by storage
        switch (a.storage)
        {
            case expr_matrix_storage::floats:
                return expr_matrix_transposed_in<float>(a,
                                                        _a.flags());

            case expr_matrix_storage::doubles:
                return expr_matrix_transposed_in<double>(a,
                                                         _a.flags());

            case expr_matrix_storage::long_doubles:
                return expr_matrix_transposed_in<long double>(a,
                                                              _a.flags());

            case expr_matrix_storage::complex_floats:
                return expr_matrix_reordered_complex<float>(a, false, true,
                                                            _conjugate,
                                                            _a.flags());

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_reordered_complex<double>(a, false, true,
                                                             _conjugate,
                                                             _a.flags());

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_reordered_complex<long double>(
                    a, false, true, _conjugate, _a.flags());

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_reordered_quaternion<float>(
                    a, false, true, _conjugate, _a.flags());

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_reordered_quaternion<double>(
                    a, false, true, _conjugate, _a.flags());

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_reordered_quaternion<long double>(
                    a, false, true, _conjugate, _a.flags());

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        std::vector<value> x = expr_matrix_values_of(_a);
        std::vector<value> out(x.size());

        // each complex element and quaternion conjugated, where asked
        for (std::size_t i = 0; ( (_conjugate) && (i < x.size()) ); ++i)
        {
            x[i] = ( expr_is_complex(x[i])    ? expr_conj(x[i])
                   : expr_is_quaternion(x[i]) ? expr_quaternion_conj(x[i])
                                              : x[i] );
        }

        kernel::transpose(out.data(),
                          x.data(),
                          a.shape.rows,
                          a.shape.columns);

        return expr_matrix_of_values(expr_shape(a.shape.columns,
                                                a.shape.rows),
                                     out);
    }

    // 3.2.7
    // The square algebra
    //   functions: the type a matrix's determinant, inverse, solutions and
    // eigenvalues are computed in (S257): a dense matrix's own; for one of
    // values, the exact arithmetic where every element is exact, the
    // value's own where any is a boxed inexact number, and otherwise the
    // widest of its inexact elements' types, each exact element its nearest
    // there, as the tower takes an operation's arguments; a matrix's
    // elements in that type; the flags a result computed from them carries;
    // and A's inverse, A square: by the kernels' Gauss-Jordan elimination,
    // with partial pivoting, the C core's in a floating-point type and the
    // generic one's on values, exact for an exact A; a pole for a singular
    // A, as an exact number over 0 is (S259); and a domain error for any
    // other A. The inverse of the empty matrix is itself. Where A holds a
    // quaternion, each product keeps its order and each row is divided on
    // the left, as A X = B asks: the generic kernels' on the quaternion
    // kind's operands, or on values whose traits say they do not commute
    // (S269, S277, S388).

    // expr_flagged
    //   function: _x with _flags as well as its own.
    inline value
    expr_flagged(
        const value& _x,
        unsigned     _flags
    )
    {
        dj_math_value held = value(_x).detach();

        held.flags |= _flags;

        return value::adopt(held);
    }

    // expr_matrix_flags_of
    //   function: every flag a matrix's elements carry: a dense one's, the
    // matrix's own; one of values, each element's and the matrix's.
    inline unsigned
    expr_matrix_flags_of(
        const value& _a
    )
    {
        const expr_matrix_box& a     = expr_matrix_box_of(_a.c_value());
        unsigned               flags = _a.flags();

        // each element's
        for (std::size_t i = 0; i < a.values.size(); ++i)
        {
            flags |= a.values[i].flags();
        }

        return flags;
    }

    // expr_matrix_all_exact
    //   function: whether a matrix holds values, each exact.
    inline bool
    expr_matrix_all_exact(
        const value& _a
    )
    {
        const expr_matrix_box& a = expr_matrix_box_of(_a.c_value());

        // a dense matrix holds none
        if (a.storage != expr_matrix_storage::values)
        {
            return false;
        }

        for (std::size_t i = 0; i < a.values.size(); ++i)
        {
            if (!a.values[i].is_exact())
            {
                return false;
            }
        }

        return true;
    }

    // expr_matrix_has_complex
    //   function: whether a matrix holds a complex number: a complex dense
    // one, or one of values of which one is.
    inline bool
    expr_matrix_has_complex(
        const value& _a
    )
    {
        const expr_matrix_box& a = expr_matrix_box_of(_a.c_value());

        // a dense one, by its storage
        if (a.storage != expr_matrix_storage::values)
        {
            return expr_storage_is_complex(a.storage);
        }

        // values: whether any is a complex number
        for (std::size_t i = 0; i < a.values.size(); ++i)
        {
            if (expr_is_complex(a.values[i]))
            {
                return true;
            }
        }

        return false;
    }

    // expr_matrix_has_quaternion
    //   function: whether a matrix holds a quaternion: a dense one of
    // quaternions, or one of values of which one is.
    inline bool
    expr_matrix_has_quaternion(
        const value& _a
    )
    {
        const expr_matrix_box& a = expr_matrix_box_of(_a.c_value());

        // a dense one, by its storage
        if (a.storage != expr_matrix_storage::values)
        {
            return expr_storage_is_quaternion(a.storage);
        }

        // values: whether any is a quaternion
        for (std::size_t i = 0; i < a.values.size(); ++i)
        {
            if (expr_is_quaternion(a.values[i]))
            {
                return true;
            }
        }

        return false;
    }

    inline bool
    expr_matrix_noncommuting(
        const value& _a,
        const value& _number
    )
    {
        return ( (expr_is_quaternion(_number)) ||
                 (expr_matrix_has_quaternion(_a)) );
    }

    // expr_matrix_algebra_storage
    //   function: the storage a matrix's square algebra is computed in: a
    // dense matrix's own; values for one of exact values, or of a boxed
    // inexact one; otherwise the widest inexact element's type, of
    // quaternions where any element, exact or not, is one (S388), and
    // otherwise of complex numbers where any is one (S351).
    inline expr_matrix_storage
    expr_matrix_algebra_storage(
        const value& _a
    )
    {
        const expr_matrix_box& a      = expr_matrix_box_of(_a.c_value());
        expr_matrix_storage    widest = expr_matrix_storage::values;

        // a dense matrix: its own type
        if (a.storage != expr_matrix_storage::values)
        {
            return a.storage;
        }

        // the widest inline inexact element's, of quaternions or complex
        // where an element is, each exact one taken to it; a boxed inexact
        // one: values
        bool complex    = false;
        bool quaternion = false;

        for (std::size_t i = 0; i < a.values.size(); ++i)
        {
            const expr_matrix_storage storage =
                expr_matrix_element_storage(a.values[i]);

            complex    = ( (complex) || (expr_is_complex(a.values[i])) );
            quaternion = ( (quaternion) ||
                           (expr_is_quaternion(a.values[i])) );

            if (a.values[i].is_exact())
            {
                continue;
            }

            if (storage == expr_matrix_storage::values)
            {
                return expr_matrix_storage::values;
            }

            widest = ( (widest == expr_matrix_storage::values)
                           ? storage
                           : expr_matrix_wider(widest,
                                               storage) );
        }

        return ( (quaternion) ? expr_storage_quaternion_of(widest)
               : (complex)    ? expr_storage_complex_of(widest)
                              : widest );
    }

    // expr_matrix_nearest
    //   functions: a number's nearest float, double or long double; its
    // nearest math::complex<T>, a real one's x + 0i; its nearest
    // math::quaternion<T>; and its nearest operand of the quaternion kind in
    // T, holding the parts its kind holds (S387).
    template<typename Type>
    bool
    expr_matrix_nearest(
        const value& _x,
        Type&        _out
    )
    {
        return _x.to(_out);
    }

    template<typename Type>
    bool
    expr_matrix_nearest(
        const value&         _x,
        math::complex<Type>& _out
    )
    {
        return expr_complex_to(_x,
                               _out);
    }

    template<typename Type>
    bool
    expr_matrix_nearest(
        const value&            _x,
        math::quaternion<Type>& _out
    )
    {
        return expr_quaternion_to(_x,
                                  _out);
    }

    template<typename Type>
    bool
    expr_matrix_nearest(
        const value&                   _x,
        expr_quaternion_operand<Type>& _out
    )
    {
        _out = expr_quaternion_operand_of<Type>(
            expr_quaternion_parts_of(_x.c_value()));

        return true;
    }

    // expr_matrix_dense_as (operands)
    //   functions: a dense box's elements as the quaternion kind's operands
    // in Type (expr_matrix_operands_as).
    template<>
    inline std::vector<expr_quaternion_operand<float> >
    expr_matrix_dense_as<expr_quaternion_operand<float> >(
        const expr_matrix_box& _box
    )
    {
        return expr_matrix_operands_as<float>(_box);
    }

    template<>
    inline std::vector<expr_quaternion_operand<double> >
    expr_matrix_dense_as<expr_quaternion_operand<double> >(
        const expr_matrix_box& _box
    )
    {
        return expr_matrix_operands_as<double>(_box);
    }

    template<>
    inline std::vector<expr_quaternion_operand<long double> >
    expr_matrix_dense_as<expr_quaternion_operand<long double> >(
        const expr_matrix_box& _box
    )
    {
        return expr_matrix_operands_as<long double>(_box);
    }

    // expr_matrix_dense_of
    //   function: a matrix's elements, row by row, in Type, no narrower
    // than its algebra's: a dense one's widened exactly, and the nearest
    // Type of each of a matrix of values.
    template<typename Type>
    std::vector<Type>
    expr_matrix_dense_of(
        const value& _a
    )
    {
        const expr_matrix_box& a = expr_matrix_box_of(_a.c_value());

        // a dense matrix
        if (a.storage != expr_matrix_storage::values)
        {
            return expr_matrix_dense_as<Type>(a);
        }

        std::vector<Type> dense;

        dense.reserve(a.values.size());

        // each element its nearest Type
        for (std::size_t i = 0; i < a.values.size(); ++i)
        {
            Type x = Type();

            static_cast<void>(expr_matrix_nearest(a.values[i],
                                                  x));
            dense.push_back(x);
        }

        return dense;
    }

    // expr_matrix_cores, expr_matrix_of_cores
    //   functions: math::complex<Type>s as the C core's structs, which
    // cmat.h's kernels take, and back.
    template<typename Type>
    std::vector<typename math::complex<Type>::core_type>
    expr_matrix_cores(
        const std::vector<math::complex<Type> >& _z
    )
    {
        std::vector<typename math::complex<Type>::core_type> cores;

        cores.reserve(_z.size());

        // in order
        for (std::size_t i = 0; i < _z.size(); ++i)
        {
            cores.push_back(_z[i].c_value());
        }

        return cores;
    }

    template<typename Type>
    std::vector<math::complex<Type> >
    expr_matrix_of_cores(
        const std::vector<typename math::complex<Type>::core_type>& _cores
    )
    {
        std::vector<math::complex<Type> > z;

        z.reserve(_cores.size());

        // in order
        for (std::size_t i = 0; i < _cores.size(); ++i)
        {
            z.push_back(math::complex<Type>(_cores[i]));
        }

        return z;
    }

    // expr_complex_flagged_of
    //   function: the value of a math::complex<Type>, with _flags.
    template<typename Type>
    value
    expr_complex_flagged_of(
        const math::complex<Type>& _z,
        unsigned                   _flags
    )
    {
        return expr_flagged(expr_complex_of(_z),
                            _flags);
    }

    // expr_matrix_det_complex, expr_matrix_inverse_complex,
    // expr_matrix_trace_complex, expr_matrix_solve_complex
    //   functions: det(A), inv(A), trace(A) and solve(A, B) of A, B or both
    // of complex numbers, in math::complex<Type>, each other element x as x
    // + 0i: cmat.h's LU with partial pivoting by |re| + |im|, its
    // Gauss-Jordan inverse and its solve, d_cmat_solve, A's LU and each of
    // B's columns refined with extra precision, as la::solve's are, as
    // LAPACK's zgetrf, zgetri and zgesvxx, each product and quotient
    // cplx.h's (S269, S351, S410, S428); a pole for a singular A; the trace
    // by the kernels', each part Sum2's, rounded once (S411).
    template<typename Type>
    value
    expr_matrix_det_complex(
        const value& _a,
        std::size_t  _n
    )
    {
        typedef typename math::complex<Type>::core_type core_type;

        const std::vector<core_type> a = expr_matrix_cores<Type>(
            expr_matrix_dense_of<math::complex<Type> >(_a));
        std::vector<core_type>       scratch(_n * _n);

        return expr_complex_flagged_of(
            math::complex<Type>(d_cmat_determinant(a.data(),
                                                   scratch.data(),
                                                   _n)),
            expr_matrix_flags_of(_a));
    }

    template<typename Type>
    value
    expr_matrix_inverse_complex(
        const value& _a,
        std::size_t  _n
    )
    {
        typedef typename math::complex<Type>::core_type core_type;

        const std::vector<core_type> a = expr_matrix_cores<Type>(
            expr_matrix_dense_of<math::complex<Type> >(_a));
        std::vector<core_type>       out(_n * _n);
        std::vector<core_type>       scratch(_n * _n);

        // singular: a pole
        if (!d_cmat_inverse(out.data(),
                            scratch.data(),
                            a.data(),
                            _n))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        std::vector<math::complex<Type> > inverse =
            expr_matrix_of_cores<Type>(out);

        return expr_matrix_of_dense<math::complex<Type> >(
            expr_shape(_n,
                       _n),
            inverse,
            expr_matrix_flags_of(_a));
    }

    template<typename Type>
    value
    expr_matrix_trace_complex(
        const value& _a,
        std::size_t  _n
    )
    {
        typedef linalg::internal::linalg_kernel<math::complex<Type> > kernel;

        const std::vector<math::complex<Type> > a =
            expr_matrix_dense_of<math::complex<Type> >(_a);

        return expr_complex_flagged_of(kernel::trace(a.data(),
                                                     _n),
                                       _a.flags());
    }

    template<typename Type>
    value
    expr_matrix_solve_complex(
        const value&      _a,
        const value&      _b,
        std::size_t       _n,
        std::size_t       _columns,
        const expr_shape& _shape
    )
    {
        typedef typename math::complex<Type>::core_type core_type;

        const std::vector<core_type> a = expr_matrix_cores<Type>(
            expr_matrix_dense_of<math::complex<Type> >(_a));
        const std::vector<core_type> b = expr_matrix_cores<Type>(
            expr_matrix_dense_of<math::complex<Type> >(_b));
        std::vector<core_type>       x(_n * _columns);
        std::vector<core_type>       scratch((_n * _n) + (4 * _n));
        std::vector<std::size_t>     pivots(_n);

        // singular: a pole; otherwise refined, as the language's real
        // solves are (S410, S428)
        if (!d_cmat_solve(x.data(),
                          scratch.data(),
                          pivots.data(),
                          a.data(),
                          b.data(),
                          _n,
                          _columns))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        std::vector<math::complex<Type> > solution =
            expr_matrix_of_cores<Type>(x);

        return expr_matrix_of_dense<math::complex<Type> >(
            _shape,
            solution,
            ( expr_matrix_flags_of(_a) |
              expr_matrix_flags_of(_b) ));
    }

    // expr_matrix_inverse_quaternion, expr_matrix_trace_quaternion,
    // expr_matrix_solve_quaternion
    //   functions: inv(A), trace(A) and solve(A, B) of A, B or both holding
    // quaternions, in Type: the generic kernels' Gauss-Jordan inverse and
    // their elimination of [A | B], with partial pivoting by |w| + |x| + |y|
    // + |z|, on the quaternion kind's operands, each product in its order
    // and each row divided on the left (S269, S277, S388); a pole for a
    // singular A, whose pivot comes to 0; the trace the kernels', from 0.
    template<typename Type>
    value
    expr_matrix_inverse_quaternion(
        const value& _a,
        std::size_t  _n
    )
    {
        typedef expr_quaternion_operand<Type>            operand;
        typedef linalg::internal::linalg_kernel<operand> kernel;

        const std::vector<operand> a = expr_matrix_dense_of<operand>(_a);
        std::vector<operand>       out(_n * _n);
        std::vector<operand>       scratch(_n * _n);

        // singular: a pole
        if (!kernel::inverse(out.data(),
                             scratch.data(),
                             a.data(),
                             _n))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        return expr_matrix_of_operands<Type>(expr_shape(_n,
                                                        _n),
                                             out,
                                             expr_matrix_flags_of(_a));
    }

    template<typename Type>
    value
    expr_matrix_trace_quaternion(
        const value& _a,
        std::size_t  _n
    )
    {
        typedef expr_quaternion_operand<Type>            operand;
        typedef linalg::internal::linalg_kernel<operand> kernel;

        const std::vector<operand> a = expr_matrix_dense_of<operand>(_a);

        return expr_quaternion_dense(kernel::trace(a.data(),
                                                   _n).q,
                                     expr_matrix_flags_of(_a));
    }

    template<typename Type>
    value
    expr_matrix_solve_quaternion(
        const value&      _a,
        const value&      _b,
        std::size_t       _n,
        std::size_t       _columns,
        const expr_shape& _shape
    )
    {
        typedef expr_quaternion_operand<Type>            operand;
        typedef linalg::internal::linalg_kernel<operand> kernel;

        const std::vector<operand> a = expr_matrix_dense_of<operand>(_a);
        const std::vector<operand> b = expr_matrix_dense_of<operand>(_b);
        std::vector<operand>       x(_n * _columns);
        std::vector<operand>       scratch(_n * _n);

        // singular: a pole, by the generic kernel's elimination,
        // which its solve and solve_fast share (S406)
        if (!kernel::solve_fast(x.data(),
                                scratch.data(),
                                a.data(),
                                b.data(),
                                _n,
                                _columns))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        return expr_matrix_of_operands<Type>(_shape,
                                             x,
                                             ( expr_matrix_flags_of(_a) |
                                               expr_matrix_flags_of(_b) ));
    }

    // expr_matrix_skew
    //   struct: an element of a matrix of values that holds a quaternion, as
    // the generic kernels compute with it: the value's own arithmetic, each
    // operation the kind's own, with traits saying products do not commute,
    // so that each row is divided on the left, as A X = B asks; exact for
    // exact elements (S269, S277, S388).
    struct expr_matrix_skew
    {
        value x;
    };

    // expr_matrix_skewed, expr_matrix_unskewed
    //   functions: values as skew elements, and back.
    inline std::vector<expr_matrix_skew>
    expr_matrix_skewed(
        const std::vector<value>& _x
    )
    {
        std::vector<expr_matrix_skew> out;

        out.reserve(_x.size());

        // in order
        for (std::size_t i = 0; i < _x.size(); ++i)
        {
            const expr_matrix_skew element = { _x[i] };

            out.push_back(element);
        }

        return out;
    }

    inline std::vector<value>
    expr_matrix_unskewed(
        const std::vector<expr_matrix_skew>& _x
    )
    {
        std::vector<value> out;

        out.reserve(_x.size());

        // in order
        for (std::size_t i = 0; i < _x.size(); ++i)
        {
            out.push_back(_x[i].x);
        }

        return out;
    }

    // operator+, operator-, operator*, operator/ (expr_matrix_skew)
    //   functions: the values' own, each in its order; -a, += and -=; a == b
    // where the values compare equal.
    inline expr_matrix_skew
    operator+(
        const expr_matrix_skew& _a,
        const expr_matrix_skew& _b
    )
    {
        const expr_matrix_skew out = { (_a.x + _b.x) };

        return out;
    }

    inline expr_matrix_skew
    operator-(
        const expr_matrix_skew& _a,
        const expr_matrix_skew& _b
    )
    {
        const expr_matrix_skew out = { (_a.x - _b.x) };

        return out;
    }

    inline expr_matrix_skew
    operator-(
        const expr_matrix_skew& _a
    )
    {
        const expr_matrix_skew out = { -_a.x };

        return out;
    }

    inline expr_matrix_skew
    operator*(
        const expr_matrix_skew& _a,
        const expr_matrix_skew& _b
    )
    {
        const expr_matrix_skew out = { (_a.x * _b.x) };

        return out;
    }

    inline expr_matrix_skew
    operator/(
        const expr_matrix_skew& _a,
        const expr_matrix_skew& _b
    )
    {
        const expr_matrix_skew out = { (_a.x / _b.x) };

        return out;
    }

    inline expr_matrix_skew&
    operator+=(
        expr_matrix_skew&       _a,
        const expr_matrix_skew& _b
    )
    {
        _a.x = (_a.x + _b.x);

        return _a;
    }

    inline expr_matrix_skew&
    operator-=(
        expr_matrix_skew&       _a,
        const expr_matrix_skew& _b
    )
    {
        _a.x = (_a.x - _b.x);

        return _a;
    }

    inline bool
    operator==(
        const expr_matrix_skew& _a,
        const expr_matrix_skew& _b
    )
    {
        return (_a.x == _b.x);
    }

    inline bool
    operator!=(
        const expr_matrix_skew& _a,
        const expr_matrix_skew& _b
    )
    {
        return !(_a == _b);
    }

NS_END  // internal

// number_traits<internal::expr_matrix_skew>
//   trait: a value of a matrix that holds a quaternion, as the generic
// kernels take it: value's own traits, but for products, which do not
// commute (S388).
template<>
struct number_traits<internal::expr_matrix_skew>
{
    typedef number_traits<value>::magnitude_type magnitude_type;

    static const bool is_specialized = true;
    static const bool is_exact       = false;
    static const bool is_integer     = false;
    static const bool is_field       = true;
    static const bool is_commutative = false;
    static const bool is_ordered     = false;

    // the additive and multiplicative identities, exact.
    static internal::expr_matrix_skew
    zero()
    {
        const internal::expr_matrix_skew out = { value(0) };

        return out;
    }

    static internal::expr_matrix_skew
    one()
    {
        const internal::expr_matrix_skew out = { value(1) };

        return out;
    }

    // what a pivot is chosen by: the value's magnitude, a quaternion's
    // norm.
    static magnitude_type
    magnitude(
        const internal::expr_matrix_skew& _a
    )
    {
        return number_traits<value>::magnitude(_a.x);
    }
};

NS_INTERNAL

    // expr_matrix_inverse_in
    //   function: A's inverse, of _n rows, in Type, or a pole.
    template<typename Type>
    value
    expr_matrix_inverse_in(
        const value& _a,
        std::size_t  _n
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        const std::vector<Type> a = expr_matrix_dense_of<Type>(_a);
        std::vector<Type>       out(_n * _n);
        std::vector<Type>       scratch(_n * _n);

        // singular: a pole
        if (!kernel::inverse(out.data(),
                             scratch.data(),
                             a.data(),
                             _n))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        return expr_matrix_of_dense<Type>(expr_shape(_n,
                                                     _n),
                                          out,
                                          expr_matrix_flags_of(_a));
    }

    // expr_matrix_inverse_skew
    //   function: the inverse of A, of values holding a quaternion, as skew
    // elements, exact for an exact A, or a pole.
    inline value
    expr_matrix_inverse_skew(
        const value& _a,
        std::size_t  _n
    )
    {
        typedef linalg::internal::linalg_kernel<expr_matrix_skew> kernel;

        const std::vector<expr_matrix_skew> x =
            expr_matrix_skewed(expr_matrix_values_of(_a));
        std::vector<expr_matrix_skew>       out(_n * _n);
        std::vector<expr_matrix_skew>       scratch(_n * _n);

        // singular: a pole
        if (!kernel::inverse(out.data(),
                             scratch.data(),
                             x.data(),
                             _n))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        return expr_matrix_of_values(expr_shape(_n,
                                                _n),
                                     expr_matrix_unskewed(out));
    }

    inline value
    expr_matrix_inverse(
        const value& _a
    )
    {
        const expr_shape  shape = expr_matrix_box_of(_a.c_value()).shape;
        const std::size_t n     = shape.rows;

        // square
        if (shape.rows != shape.columns)
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // by the type it is computed in
        switch (expr_matrix_algebra_storage(_a))
        {
            case expr_matrix_storage::floats:
                return expr_matrix_inverse_in<float>(_a,
                                                     n);

            case expr_matrix_storage::doubles:
                return expr_matrix_inverse_in<double>(_a,
                                                      n);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_inverse_in<long double>(_a,
                                                           n);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_inverse_complex<float>(_a,
                                                          n);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_inverse_complex<double>(_a,
                                                           n);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_inverse_complex<long double>(_a,
                                                                n);

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_inverse_quaternion<float>(_a,
                                                             n);

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_inverse_quaternion<double>(_a,
                                                              n);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_inverse_quaternion<long double>(_a,
                                                                   n);

            case expr_matrix_storage::values:
            default:
                break;
        }

        // values holding a quaternion: as skew elements
        if (expr_matrix_has_quaternion(_a))
        {
            return expr_matrix_inverse_skew(_a,
                                            n);
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> x = expr_matrix_values_of(_a);
        std::vector<value>       out(n * n);
        std::vector<value>       scratch(n * n);

        // singular: a pole
        if (!kernel::inverse(out.data(),
                             scratch.data(),
                             x.data(),
                             n))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        return expr_matrix_of_values(shape,
                                     out);
    }

    // 3.2.8
    // Powers
    //   function: A ^ n, of a square A and a whole n, an exact number or a
    // floating-point one; for a floating-point n, A first in the wider of
    // its type and n's, as the tower would take a number to it: the identity
    // of A's size, of A's storage, for n = 0; for a negative n, A's
    // inverse's power, a pole where A is singular (S259); otherwise by
    // squaring, as the C core's d_mat_power: from n's highest bit down, the
    // power begun as A and at each bit below it squared, then multiplied by
    // A on the left where the bit is 1, each product the kernels', so that a
    // dense power is the C core's, bit for bit (S262). A domain error for any
    // other A or n.
    template<typename Type>
    value
    expr_matrix_identity_in(
        std::size_t _size,
        unsigned    _flags
    )
    {
        typedef linalg::internal::linalg_kernel<Type> kernel;

        std::vector<Type> out(_size * _size);

        kernel::identity(out.data(),
                         _size);

        return expr_matrix_of_dense<Type>(expr_shape(_size,
                                                     _size),
                                          out,
                                          _flags);
    }

    template<typename Type>
    value
    expr_matrix_identity_quaternion(
        std::size_t _size,
        unsigned    _flags
    )
    {
        typedef expr_quaternion_operand<Type>            operand;
        typedef linalg::internal::linalg_kernel<operand> kernel;

        std::vector<operand> out(_size * _size);

        kernel::identity(out.data(),
                         _size);

        return expr_matrix_of_operands<Type>(expr_shape(_size,
                                                        _size),
                                             out,
                                             _flags);
    }

    inline value
    expr_matrix_identity(
        const value& _a
    )
    {
        const expr_matrix_box& a = expr_matrix_box_of(_a.c_value());
        const std::size_t      n = a.shape.rows;

        // by storage
        switch (a.storage)
        {
            case expr_matrix_storage::floats:
                return expr_matrix_identity_in<float>(n,
                                                      _a.flags());

            case expr_matrix_storage::doubles:
                return expr_matrix_identity_in<double>(n,
                                                       _a.flags());

            case expr_matrix_storage::long_doubles:
                return expr_matrix_identity_in<long double>(n,
                                                            _a.flags());

            case expr_matrix_storage::complex_floats:
                return expr_matrix_identity_in<math::complex<float> >(
                    n, _a.flags());

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_identity_in<math::complex<double> >(
                    n, _a.flags());

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_identity_in<math::complex<long double> >(
                    n, _a.flags());

            case expr_matrix_storage::quaternion_floats:
                return expr_matrix_identity_quaternion<float>(n,
                                                              _a.flags());

            case expr_matrix_storage::quaternion_doubles:
                return expr_matrix_identity_quaternion<double>(n,
                                                               _a.flags());

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_matrix_identity_quaternion<long double>(
                    n, _a.flags());

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        std::vector<value> out(n * n);

        kernel::identity(out.data(),
                         n);

        return expr_matrix_of_values(expr_shape(n,
                                                n),
                                     out);
    }

    inline value
    expr_matrix_power(
        const value& _a,
        const value& _exponent
    )
    {
        const dj_math_imax most     = std::numeric_limits<dj_math_imax>::max();
        dj_math_imax       exponent = 0;
        long double        real     = 0.0L;
        value              base     = _a;

        // a matrix
        if (!expr_matrix_held(_a.c_value()))
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        const expr_shape shape = expr_matrix_box_of(_a.c_value()).shape;

        // square, to an exact whole power whose magnitude is a whole number
        if (_exponent.is_exact())
        {
            if ( (shape.rows != shape.columns) ||
                 (!_exponent.to(exponent)) ||
                 (exponent < -most) )
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }
        }
        // or to a floating-point whole one, A first in its type: times
        // n - n + 1, the 1 of n's kind
        else
        {
            if ( (shape.rows != shape.columns) ||
                 (!_exponent.is_number()) ||
                 (!_exponent.to(real)) ||
                 (!(real > -9.0e18L)) ||
                 (!(real < 9.0e18L)) ||
                 (real != static_cast<long double>(
                              static_cast<dj_math_imax>(real))) )
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            exponent = static_cast<dj_math_imax>(real);
            base     = expr_matrix_scaled(_a,
                                          ( (_exponent - _exponent) +
                                            value(1) ),
                                          false);

            if (base.kind() == D_MATH_VALUE_ERROR)
            {
                return base;
            }
        }

        // a negative power: the inverse's
        if (exponent < 0)
        {
            base     = expr_matrix_inverse(base);
            exponent = -exponent;

            if (base.kind() == D_MATH_VALUE_ERROR)
            {
                return base;
            }
        }

        // A ^ 0: the identity
        if (exponent == 0)
        {
            return expr_matrix_identity(base);
        }

        dj_math_imax top   = 1;
        value        power = base;

        // the exponent's highest bit
        while (top <= (exponent / 2))
        {
            top *= 2;
        }

        // each bit below it: the power squared, then times A on the left
        // where the bit is 1; an error, where one is made
        for (dj_math_imax bit = (top / 2); bit > 0; bit /= 2)
        {
            power = expr_matrix_product(power,
                                        power);

            if ( (power.kind() != D_MATH_VALUE_ERROR) &&
                 ((exponent & bit) != 0) )
            {
                power = expr_matrix_product(base,
                                            power);
            }

            if (power.kind() == D_MATH_VALUE_ERROR)
            {
                return power;
            }
        }

        return power;
    }

    // 3.2.9
    // Indexing
    //   function: A[i], and A[i, j], counted from 1 (S249): of a vector, a
    // matrix of one row or of one column, its i-th element; of any other
    // matrix, its i-th row, a matrix of one row; A[i, j] the element in row
    // i, column j. An index is a whole number of any kind, an exact one or a
    // floating-point one, from 1 to the number of places. An error passes,
    // the first, A's then the indices'; a domain error for anything else: a
    // value that is no matrix, an index that is no whole number or past the
    // matrix's places, or more indices than two. An element keeps its own
    // value, and a dense matrix's element the matrix's flags.

    // expr_matrix_place
    //   function: whether _index is a whole number from 1 to _count, and if
    // so it less 1, from 0, in _place.
    inline bool
    expr_matrix_place(
        const value& _index,
        std::size_t  _count,
        std::size_t& _place
    )
    {
        dj_math_imax whole = 0;
        long double real   = 0.0L;

        // an exact whole number
        if (_index.is_exact())
        {
            if ( (!_index.to(whole)) ||
                 (whole < 1) ||
                 (static_cast<unsigned long long>(whole) > _count) )
            {
                return false;
            }

            _place = static_cast<std::size_t>(whole - 1);

            return true;
        }

        // a floating-point whole number
        if ( (!_index.is_number()) ||
             (!_index.to(real)) ||
             (!(real >= 1.0L)) ||
             (!(real <= static_cast<long double>(_count))) ||
             (real != static_cast<long double>(
                          static_cast<unsigned long long>(real))) )
        {
            return false;
        }

        _place = static_cast<std::size_t>(real) - 1;

        return true;
    }

    // expr_matrix_row_of
    //   function: A's row _row, a matrix of one row, each element as it is.
    inline value
    expr_matrix_row_of(
        const value& _a,
        std::size_t  _row
    )
    {
        const expr_matrix_box& a       = expr_matrix_box_of(_a.c_value());
        const std::size_t      columns = a.shape.columns;
        std::vector<value>     row;

        row.reserve(columns);

        // its elements, in order
        for (std::size_t c = 0; c < columns; ++c)
        {
            row.push_back(expr_matrix_element_of(_a,
                                                 (_row * columns) + c));
        }

        return expr_matrix_of_values(expr_shape(1,
                                                columns),
                                     row);
    }

    inline value
    expr_matrix_index(
        const std::vector<value>& _operands
    )
    {
        // an error passes, the first
        for (std::size_t i = 0; i < _operands.size(); ++i)
        {
            if (_operands[i].kind() == D_MATH_VALUE_ERROR)
            {
                return _operands[i];
            }
        }

        // a matrix, and one index or two
        if ( (_operands.size() < 2) ||
             (_operands.size() > 3) ||
             (!expr_matrix_held(_operands[0].c_value())) )
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        const expr_shape shape =
            expr_matrix_box_of(_operands[0].c_value()).shape;
        const bool       vector = ( (shape.rows == 1) ||
                                    (shape.columns == 1) );
        std::size_t      row    = 0;
        std::size_t      column = 0;

        // A[i, j]: the element of row i, column j
        if (_operands.size() == 3)
        {
            if ( (!expr_matrix_place(_operands[1],
                                     shape.rows,
                                     row)) ||
                 (!expr_matrix_place(_operands[2],
                                     shape.columns,
                                     column)) )
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            return expr_matrix_element_of(_operands[0],
                                          (row * shape.columns) + column);
        }

        // v[i]: a vector's element; A[i]: any other matrix's row
        if (!expr_matrix_place(_operands[1],
                               ( vector ? (shape.rows * shape.columns)
                                        : shape.rows ),
                               row))
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        return ( vector ? expr_matrix_element_of(_operands[0],
                                                 row)
                        : expr_matrix_row_of(_operands[0],
                                             row) );
    }

NS_END  // internal


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_MATRIX_KIND_ARITHMETIC_HPP
