/*******************************************************************************
* djinterp [cpp][math]                                   matrix_kind_storage.hpp
*
* One part of matrix_kind.hpp (S437), its sections 1, 2 and 3.1: the
* shape, the matrix kind's storage, entries and table, and making and
* reading matrix values. matrix_kind.hpp describes the kind whole.
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
* path:      /inc/djinterp/cpp/modules/math/expr/matrix_kind_storage.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SHAPES
    ------
    1.  The shape
         1.  expr_shape
2.  THE KIND
    --------
    1.  Its storage
         1.  expr_matrix_rank
         2.  expr_matrix_storage
         3.  expr_matrix_box
         4.  Reading a box
    2.  Its entries
         1.  expr_matrix_retain, expr_matrix_release
         2.  expr_matrix_unary, expr_matrix_binary
         3.  expr_matrix_cmp
         4.  expr_matrix_convert
         5.  expr_matrix_text
    3.  Its table
         1.  expr_matrix_kind
         2.  expr_matrix_adopt
3.  MATRIX VALUES
    -------------
    1.  Making and reading
         1.  expr_matrix
         2.  expr_vector
         3.  expr_is_matrix
         4.  expr_shape_of
         5.  expr_element
*/

#ifndef DJINTERP_MATH_EXPR_MATRIX_KIND_STORAGE_HPP
#define DJINTERP_MATH_EXPR_MATRIX_KIND_STORAGE_HPP 1

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


NS_DJINTERP
NS_MATH

//==============================================================================
// 1.  SHAPES
//==============================================================================


// 1.1    The shape
//------------------------------------------------------------------------------
// 1.1.1
// expr_shape
//   struct: a matrix's numbers of rows and columns. A shape made by default
// is 0 by 0. Two shapes are equal when both numbers are.
struct expr_shape
{
    std::size_t rows;
    std::size_t columns;

    // 0 by 0.
    expr_shape() D_NOEXCEPT
        : rows(0),
          columns(0)
    {}

    expr_shape(
        std::size_t _rows,
        std::size_t _columns
    ) D_NOEXCEPT
        : rows(_rows),
          columns(_columns)
    {}
};

// operator==, operator!= (expr_shape)
//   function: two shapes are equal when their rows and columns are.
inline bool
operator==(
    const expr_shape& _a,
    const expr_shape& _b
) D_NOEXCEPT
{
    return ( (_a.rows == _b.rows) &&
             (_a.columns == _b.columns) );
}

inline bool
operator!=(
    const expr_shape& _a,
    const expr_shape& _b
) D_NOEXCEPT
{
    return !(_a == _b);
}


//==============================================================================
// 2.  THE KIND
//==============================================================================


// 2.1    Its storage
//------------------------------------------------------------------------------
// 2.1.1
// expr_matrix_rank
//   constant: the kind's rank, its place in the tower and its own: past
// every number's, the long double's 500 and the wide integer's 210, so that
// in an operation of a matrix and a number the matrix's table handles it,
// as step 20 (3b)'s scalar products ask; and far enough past them to
// leave the numbers to come, bignum's and the complex numbers, their places.
D_CONSTEXPR_VAR int expr_matrix_rank = 1000;

NS_INTERNAL

    // 2.1.2
    // expr_matrix_storage
    //   enum: how a matrix holds its elements: as values, each of its own
    // kind; or dense, as floats, doubles or long doubles (S239), as complex
    // numbers of them, math::complex<T>, the C core's struct, row by row as
    // LAPACK lays a complex matrix out but by rows (S273, S348), or as
    // quaternions of them, math::quaternion<T>, quat.h's array [x, y, z, w]
    // (S386). Each complex storage lies 3 past its parts' real one, and each
    // quaternion storage 6.
    enum class expr_matrix_storage
    {
        values                  = 1,    // math::values, each a number of its
                                        // own kind
        floats                  = 2,    // a dense array of float
        doubles                 = 3,    // of double
        long_doubles            = 4,    // of long double
        complex_floats          = 5,    // of math::complex<float>
        complex_doubles         = 6,    // of math::complex<double>
        complex_long_doubles    = 7,    // of math::complex<long double>
        quaternion_floats       = 8,    // of math::quaternion<float>
        quaternion_doubles      = 9,    // of math::quaternion<double>
        quaternion_long_doubles = 10    // of math::quaternion<long double>
    };

    // expr_storage_is_complex, expr_storage_is_quaternion,
    // expr_storage_real_of, expr_storage_complex_of,
    // expr_storage_quaternion_of
    //   functions: whether a storage holds complex numbers, and whether
    // quaternions; a dense storage's real type, a complex or quaternion
    // one's parts' and a real one's own; the complex storage of a real one's
    // type, a complex or quaternion one itself; and the quaternion storage
    // of a dense one's type. Values are values in each.
    D_CONSTEXPR inline bool
    expr_storage_is_complex(
        expr_matrix_storage _storage
    ) D_NOEXCEPT
    {
        return ( (static_cast<int>(_storage) >
                  static_cast<int>(expr_matrix_storage::long_doubles)) &&
                 (static_cast<int>(_storage) <
                  static_cast<int>(expr_matrix_storage::quaternion_floats)) );
    }

    D_CONSTEXPR inline bool
    expr_storage_is_quaternion(
        expr_matrix_storage _storage
    ) D_NOEXCEPT
    {
        return (static_cast<int>(_storage) >
                static_cast<int>(expr_matrix_storage::complex_long_doubles));
    }

    D_CONSTEXPR inline expr_matrix_storage
    expr_storage_real_of(
        expr_matrix_storage _storage
    ) D_NOEXCEPT
    {
        return ( expr_storage_is_quaternion(_storage)
                     ? static_cast<expr_matrix_storage>(
                           static_cast<int>(_storage) - 6)
                 : expr_storage_is_complex(_storage)
                     ? static_cast<expr_matrix_storage>(
                           static_cast<int>(_storage) - 3)
                     : _storage );
    }

    D_CONSTEXPR inline expr_matrix_storage
    expr_storage_complex_of(
        expr_matrix_storage _storage
    ) D_NOEXCEPT
    {
        return ( ( (_storage == expr_matrix_storage::values) ||
                   (expr_storage_is_complex(_storage)) ||
                   (expr_storage_is_quaternion(_storage)) )
                     ? _storage
                     : static_cast<expr_matrix_storage>(
                           static_cast<int>(_storage) + 3) );
    }

    D_CONSTEXPR inline expr_matrix_storage
    expr_storage_quaternion_of(
        expr_matrix_storage _storage
    ) D_NOEXCEPT
    {
        return ( (_storage == expr_matrix_storage::values)
                     ? _storage
                     : static_cast<expr_matrix_storage>(
                           static_cast<int>(expr_storage_real_of(_storage)) +
                           6) );
    }

    // 2.1.3
    // expr_matrix_box
    //   struct: a matrix's storage: its holders, its shape, how it holds its
    // elements, and the elements, row by row, in the one array its storage
    // names, the others empty. A box is made with one holder.
    struct expr_matrix_box
    {
        std::size_t                                 holders;
        expr_shape                                  shape;
        expr_matrix_storage                         storage;
        std::vector<value>                          values;
        std::vector<float>                          floats;
        std::vector<double>                         doubles;
        std::vector<long double>                    long_doubles;
        std::vector<math::complex<float> >          complex_floats;
        std::vector<math::complex<double> >         complex_doubles;
        std::vector<math::complex<long double> >    complex_long_doubles;
        std::vector<math::quaternion<float> >       quaternion_floats;
        std::vector<math::quaternion<double> >      quaternion_doubles;
        std::vector<math::quaternion<long double> > quaternion_long_doubles;

        expr_matrix_box(
            const expr_shape&   _shape,
            expr_matrix_storage _storage
        )
            : holders(1),
              shape(_shape),
              storage(_storage),
              values(),
              floats(),
              doubles(),
              long_doubles(),
              complex_floats(),
              complex_doubles(),
              complex_long_doubles(),
              quaternion_floats(),
              quaternion_doubles(),
              quaternion_long_doubles()
        {}
    };

    // 2.1.4
    // Reading a box
    //   functions: whether a C value is a matrix, by its kind's rank, as
    // value.h's kinds know their values; a matrix's box; its element at
    // _index, row by row, as a value, of the box alone, and of the matrix,
    // a dense element with the matrix's flags, as a dense box keeps none of
    // its own; and every element, row by row.
    inline bool
    expr_matrix_held(
        const dj_math_value& _x
    )
    {
        return ( (_x.kind == D_MATH_VALUE_BOXED) &&
                 (_x.type != D_NULLPTR) &&
                 (_x.type->rank == expr_matrix_rank) );
    }

    inline const expr_matrix_box&
    expr_matrix_box_of(
        const dj_math_value& _x
    )
    {
        return *static_cast<const expr_matrix_box*>(_x.box);
    }

    inline value
    expr_matrix_at(
        const expr_matrix_box& _box,
        std::size_t            _index
    )
    {
        // by storage
        switch (_box.storage)
        {
            case expr_matrix_storage::floats:
                return value(_box.floats[_index]);

            case expr_matrix_storage::doubles:
                return value(_box.doubles[_index]);

            case expr_matrix_storage::long_doubles:
                return value(_box.long_doubles[_index]);

            case expr_matrix_storage::complex_floats:
                return expr_complex_of(_box.complex_floats[_index]);

            case expr_matrix_storage::complex_doubles:
                return expr_complex_of(_box.complex_doubles[_index]);

            case expr_matrix_storage::complex_long_doubles:
                return expr_complex_of(_box.complex_long_doubles[_index]);

            case expr_matrix_storage::quaternion_floats:
                return expr_quaternion_of(_box.quaternion_floats[_index]);

            case expr_matrix_storage::quaternion_doubles:
                return expr_quaternion_of(_box.quaternion_doubles[_index]);

            case expr_matrix_storage::quaternion_long_doubles:
                return expr_quaternion_of(
                    _box.quaternion_long_doubles[_index]);

            case expr_matrix_storage::values:
            default:
                break;
        }

        return _box.values[_index];
    }

    inline value
    expr_matrix_element_of(
        const value& _a,
        std::size_t  _place
    )
    {
        const expr_matrix_box& a       = expr_matrix_box_of(_a.c_value());
        value                  element = expr_matrix_at(a,
                                                        _place);

        // a dense element: the matrix's flags
        if ( (a.storage != expr_matrix_storage::values) &&
             (_a.flags() != 0u) )
        {
            dj_math_value held = element.detach();

            held.flags |= _a.flags();
            element     = value::adopt(held);
        }

        return element;
    }

    inline std::vector<value>
    expr_matrix_elements(
        const value& _matrix
    )
    {
        const expr_matrix_box& box   = expr_matrix_box_of(_matrix.c_value());
        const std::size_t      count = (box.shape.rows * box.shape.columns);
        std::vector<value>     elements;

        // row by row
        for (std::size_t i = 0; i < count; ++i)
        {
            elements.push_back(expr_matrix_at(box, i));
        }

        return elements;
    }

NS_END  // internal

// 2.2    Its entries
//------------------------------------------------------------------------------
// value.h's dj_math_boxed_kind entries, each given values of the kind as that
// header says: one operand a matrix, neither an error, a boolean or none.
NS_INTERNAL

    // 2.2.1
    // expr_matrix_retain, expr_matrix_release
    //   functions: one holder more of _box, and one fewer; the last release
    // gives the box back. Counted by value.h's d_math_boxed_hold and
    // d_math_boxed_unhold, atomically wherever the compiler can (S134), so
    // that a matrix may be copied and given back in several threads at
    // once; a box never changes once it is made.
    inline void
    expr_matrix_retain(
        void* _box
    )
    {
        d_math_boxed_hold(&static_cast<expr_matrix_box*>(_box)->holders);

        return;
    }

    inline void
    expr_matrix_release(
        void* _box
    )
    {
        expr_matrix_box* box = static_cast<expr_matrix_box*>(_box);

        // the last holder: the box given back
        if (d_math_boxed_unhold(&box->holders))
        {
            delete box;
        }

        return;
    }

    // 2.2.2
    // expr_matrix_unary, expr_matrix_binary
    //   functions: -A, and A op b or b op A, by section 3.2's arithmetic:
    // -A each element negated; A + B and A - B of two matrices of one shape,
    // element by element; A * b, b * A and A / b, of a matrix and a number,
    // each element by the number; and A * B, of two matrices, A's columns as
    // many as B's rows, the matrix product. Anything else is a domain
    // error: |A|, a matrix and a number added or subtracted, a number
    // divided by a matrix, a matrix by a matrix, and two matrices of shapes
    // the operation does not take (S246). Storage that cannot be had is
    // D_MATH_VALUE_ERROR_MEMORY, as value.h asks, and nothing throws.
    inline value
    expr_matrix_negated(
        const value& _a
    );

    inline value
    expr_matrix_operation(
        dj_math_value_op _op,
        const value&     _a,
        const value&     _b
    );

    inline dj_math_value
    expr_matrix_unary(
        dj_math_value_op _op,
        dj_math_value    _x
    )
    {
        // |A|: no operation of a matrix's
        if (_op != D_MATH_VALUE_OP_NEG)
        {
            return d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // -A, or no storage for it
        try
        {
            return expr_matrix_negated(value(_x)).detach();
        }
        catch (const std::bad_alloc&)
        {
            return d_math_value_from_error(D_MATH_VALUE_ERROR_MEMORY);
        }
    }

    inline dj_math_value
    expr_matrix_binary(
        dj_math_value_op _op,
        dj_math_value    _a,
        dj_math_value    _b
    )
    {
        // the operation, or no storage for it
        try
        {
            return expr_matrix_operation(_op,
                                         value(_a),
                                         value(_b)).detach();
        }
        catch (const std::bad_alloc&)
        {
            return d_math_value_from_error(D_MATH_VALUE_ERROR_MEMORY);
        }
    }

    // 2.2.3
    // expr_matrix_cmp
    //   function: how _a stands to _b, one of them a matrix: equal where both
    // are matrices of one shape whose elements are equal, place by place, by
    // value.h's exact order, whatever their storage; unordered otherwise, as
    // a matrix has no order, and stands in none with a number (S242).
    inline dj_math_value_order
    expr_matrix_cmp(
        dj_math_value _a,
        dj_math_value _b
    )
    {
        // not two matrices, or two of two shapes
        if ( (!expr_matrix_held(_a)) ||
             (!expr_matrix_held(_b)) ||
             (expr_matrix_box_of(_a).shape != expr_matrix_box_of(_b).shape) )
        {
            return D_MATH_VALUE_UNORDERED;
        }

        const expr_matrix_box& a     = expr_matrix_box_of(_a);
        const expr_matrix_box& b     = expr_matrix_box_of(_b);
        const std::size_t      count = (a.shape.rows * a.shape.columns);

        // place by place
        for (std::size_t i = 0; i < count; ++i)
        {
            if (d_math_value_cmp(expr_matrix_at(a, i).c_value(),
                                 expr_matrix_at(b, i).c_value()) !=
                    D_MATH_VALUE_EQUAL)
            {
                return D_MATH_VALUE_UNORDERED;
            }
        }

        return D_MATH_VALUE_EQUAL;
    }

    // 2.2.4
    // expr_matrix_convert
    //   function: a matrix as an inline number: never, as a matrix is no
    // number; false, and *_out as it was. (value.h asks no kind that is no
    // number to convert; the entry must be set all the same.)
    inline bool
    expr_matrix_convert(
        dj_math_value*     _out,
        int*               _direction,
        dj_math_value      _x,
        dj_math_value_kind _to
    )
    {
        static_cast<void>(_out);
        static_cast<void>(_direction);
        static_cast<void>(_x);
        static_cast<void>(_to);

        return false;
    }

    // 2.2.5
    // expr_matrix_text
    //   function: a matrix as the value layer's text: its rows between square
    // brackets, each row's elements apart by commas and the rows by
    // semicolons, [1, 2; 3, 4], and one row alone [1, 2, 3]; each element an
    // exact number's integer or fraction, -1/2, a boxed number's its kind's
    // text, and a float, a double or a long double in max_digits10
    // significant digits, in the classic locale. Into _buffer, at most _size
    // characters, the closing NUL among them; the whole text's length
    // returned, as value.h asks. (The language writes a matrix by
    // render.hpp.)
    inline std::string
    expr_matrix_element_text(
        const value& _x
    )
    {
        std::ostringstream out;
        dj_rational        exact = { 0, 1 };
        float              f     = 0.0f;
        double             d     = 0.0;
        long double        ld    = 0.0L;

        out.imbue(std::locale::classic());

        // an exact number of the family: its integer or its fraction
        if (_x.to(exact))
        {
            out << exact.num;

            if (exact.den != 1)
            {
                out << '/' << exact.den;
            }

            return out.str();
        }

        // a boxed number: its kind's text
        if (_x.kind() == D_MATH_VALUE_BOXED)
        {
            const dj_math_value& held   = _x.c_value();
            const std::size_t    length = held.type->text(D_NULLPTR,
                                                          0,
                                                          held);
            std::string          text(length + 1, '\0');

            static_cast<void>(held.type->text(&text[0],
                                              length + 1,
                                              held));
            text.resize(length);

            return text;
        }

        // a float, a double or a long double, by kind
        if (_x.kind() == D_MATH_VALUE_FLOAT)
        {
            static_cast<void>(_x.to(f));
            out << std::setprecision(std::numeric_limits<float>::max_digits10)
                << f;
        }
        else if (_x.kind() == D_MATH_VALUE_DOUBLE)
        {
            static_cast<void>(_x.to(d));
            out << std::setprecision(
                       std::numeric_limits<double>::max_digits10)
                << d;
        }
        else
        {
            static_cast<void>(_x.to(ld));
            out << std::setprecision(
                       std::numeric_limits<long double>::max_digits10)
                << ld;
        }

        return out.str();
    }

    inline std::size_t
    expr_matrix_text(
        char*         _buffer,
        std::size_t   _size,
        dj_math_value _x
    )
    {
        const expr_matrix_box& box  = expr_matrix_box_of(_x);
        std::string            text = "[";

        // row by row
        for (std::size_t r = 0; r < box.shape.rows; ++r)
        {
            text += ( (r == 0) ? "" : "; " );

            for (std::size_t c = 0; c < box.shape.columns; ++c)
            {
                text += ( (c == 0) ? "" : ", " );
                text += expr_matrix_element_text(
                    expr_matrix_at(box,
                                   (r * box.shape.columns) + c));
            }
        }

        text += "]";

        // as much as _size holds, the closing NUL among it
        if (_size > 0)
        {
            const std::size_t kept = ( (text.size() < _size)
                                           ? text.size()
                                           : (_size - 1) );

            static_cast<void>(text.copy(_buffer, kept));
            _buffer[kept] = '\0';
        }

        return text.size();
    }

NS_END  // internal

// 2.3    Its table
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.3.1
    // expr_matrix_kind
    //   function: the kind's table: its name, "matrix", its rank, no trait,
    // as a matrix is no number, and its entries, with no exact entry, as no
    // matrix is a number; one for the program, as an inline function's
    // static is.
    inline const dj_math_boxed_kind*
    expr_matrix_kind()
    {
        static const dj_math_boxed_kind table = {
            "matrix",
            expr_matrix_rank,
            0u,
            &expr_matrix_retain,
            &expr_matrix_release,
            &expr_matrix_unary,
            &expr_matrix_binary,
            &expr_matrix_cmp,
            &expr_matrix_convert,
            &expr_matrix_text,
            D_NULLPTR
        };

        return &table;
    }

    // 2.3.2
    // expr_matrix_adopt
    //   function: a value of the kind holding _box, with _flags; the box's
    // one holder becomes the value's.
    inline value
    expr_matrix_adopt(
        expr_matrix_box* _box,
        unsigned         _flags
    )
    {
        dj_math_value held = d_internal_math_value_blank(D_MATH_VALUE_BOXED,
                                                         _flags);

        held.type = expr_matrix_kind();
        held.box  = _box;

        return value::adopt(held);
    }

    // expr_matrix_fill
    //   function: _elements, each a float, a double or a long double no
    // wider than Type, each widened exactly to Type, into the dense array
    // _dense.
    template<typename Type>
    void
    expr_matrix_fill(
        std::vector<Type>&        _dense,
        const std::vector<value>& _elements
    )
    {
        _dense.reserve(_elements.size());

        // in order
        for (std::size_t i = 0; i < _elements.size(); ++i)
        {
            Type x = Type(0);

            static_cast<void>(_elements[i].to(x));
            _dense.push_back(x);
        }

        return;
    }

    // expr_matrix_fill_complex
    //   function: _elements, each a number whose parts are floats, doubles
    // or long doubles no wider than Type, or a real one x, as x + 0i, each
    // widened exactly to math::complex<Type>, into _dense (S348).
    template<typename Type>
    void
    expr_matrix_fill_complex(
        std::vector<math::complex<Type> >& _dense,
        const std::vector<value>&          _elements
    )
    {
        _dense.reserve(_elements.size());

        // in order
        for (std::size_t i = 0; i < _elements.size(); ++i)
        {
            math::complex<Type> z;

            static_cast<void>(expr_complex_to(_elements[i],
                                              z));
            _dense.push_back(z);
        }

        return;
    }

    // expr_matrix_fill_quaternion
    //   function: _elements, each a number whose parts are floats, doubles
    // or long doubles no wider than Type, a real one x as x + 0im + 0jm +
    // 0km and a complex one x + yi as x + yim + 0jm + 0km, each widened
    // exactly to math::quaternion<Type>, into _dense (S386).
    template<typename Type>
    void
    expr_matrix_fill_quaternion(
        std::vector<math::quaternion<Type> >& _dense,
        const std::vector<value>&             _elements
    )
    {
        _dense.reserve(_elements.size());

        // in order
        for (std::size_t i = 0; i < _elements.size(); ++i)
        {
            math::quaternion<Type> q(Type(0), Type(0), Type(0), Type(0));

            static_cast<void>(expr_quaternion_to(_elements[i],
                                                 q));
            _dense.push_back(q);
        }

        return;
    }

    // expr_matrix_fill_box
    //   function: _elements into _box, as its storage holds them: densely,
    // each widened exactly, or as values, each kept as it is (S239, S348,
    // S386).
    inline void
    expr_matrix_fill_box(
        expr_matrix_box&          _box,
        const std::vector<value>& _elements
    )
    {
        // by storage
        switch (_box.storage)
        {
            case expr_matrix_storage::floats:
                expr_matrix_fill(_box.floats,
                                 _elements);
                break;

            case expr_matrix_storage::doubles:
                expr_matrix_fill(_box.doubles,
                                 _elements);
                break;

            case expr_matrix_storage::long_doubles:
                expr_matrix_fill(_box.long_doubles,
                                 _elements);
                break;

            case expr_matrix_storage::complex_floats:
                expr_matrix_fill_complex(_box.complex_floats,
                                         _elements);
                break;

            case expr_matrix_storage::complex_doubles:
                expr_matrix_fill_complex(_box.complex_doubles,
                                         _elements);
                break;

            case expr_matrix_storage::complex_long_doubles:
                expr_matrix_fill_complex(_box.complex_long_doubles,
                                         _elements);
                break;

            case expr_matrix_storage::quaternion_floats:
                expr_matrix_fill_quaternion(_box.quaternion_floats,
                                            _elements);
                break;

            case expr_matrix_storage::quaternion_doubles:
                expr_matrix_fill_quaternion(_box.quaternion_doubles,
                                            _elements);
                break;

            case expr_matrix_storage::quaternion_long_doubles:
                expr_matrix_fill_quaternion(_box.quaternion_long_doubles,
                                            _elements);
                break;

            case expr_matrix_storage::values:
            default:
                _box.values = _elements;
                break;
        }

        return;
    }

    // expr_matrix_kind_storage
    //   function: the dense storage of an element of the kind _kind: an
    // inline float's, double's or long double's own type; values for any
    // other kind, exact or boxed.
    inline expr_matrix_storage
    expr_matrix_kind_storage(
        dj_math_value_kind _kind
    ) D_NOEXCEPT
    {
        return ( (_kind == D_MATH_VALUE_FLOAT)
                     ? expr_matrix_storage::floats
                 : (_kind == D_MATH_VALUE_DOUBLE)
                     ? expr_matrix_storage::doubles
                 : (_kind == D_MATH_VALUE_LONG_DOUBLE)
                     ? expr_matrix_storage::long_doubles
                     : expr_matrix_storage::values );
    }

    // expr_matrix_element_storage
    //   function: the dense storage of an element: an inline float's,
    // double's or long double's own type; a complex number of such parts,
    // their complex storage (S348), and a quaternion of them, their
    // quaternion storage (S386); values for any other number, exact or
    // boxed, an exact complex number or quaternion among them.
    inline expr_matrix_storage
    expr_matrix_element_storage(
        const value& _x
    )
    {
        // a complex number: its parts' type, complex, where they are dense
        if (expr_is_complex(_x))
        {
            const expr_matrix_storage parts = expr_matrix_kind_storage(
                expr_complex_type_of(_x.c_value()));

            return expr_storage_complex_of(parts);
        }

        // a quaternion: its parts' type, quaternion, where they are dense
        if (expr_is_quaternion(_x))
        {
            const expr_matrix_storage parts = expr_matrix_kind_storage(
                expr_quaternion_type_of(_x.c_value()));

            return expr_storage_quaternion_of(parts);
        }

        return expr_matrix_kind_storage(_x.kind());
    }

    // expr_matrix_wider
    //   function: the storage two dense storages' elements are kept in
    // together: values where either is; the wider of their types, of
    // quaternions where either is (S386), and otherwise of complex numbers
    // where either is (S348).
    inline expr_matrix_storage
    expr_matrix_wider(
        expr_matrix_storage _a,
        expr_matrix_storage _b
    )
    {
        // values, where either is
        if ( (_a == expr_matrix_storage::values) ||
             (_b == expr_matrix_storage::values) )
        {
            return expr_matrix_storage::values;
        }

        const expr_matrix_storage a     = expr_storage_real_of(_a);
        const expr_matrix_storage b     = expr_storage_real_of(_b);
        const expr_matrix_storage wider = ( (a > b) ? a : b );

        // quaternions, complex numbers, or real ones
        if ( (expr_storage_is_quaternion(_a)) ||
             (expr_storage_is_quaternion(_b)) )
        {
            return expr_storage_quaternion_of(wider);
        }

        return ( ( (expr_storage_is_complex(_a)) ||
                   (expr_storage_is_complex(_b)) )
                     ? expr_storage_complex_of(wider)
                     : wider );
    }

    // expr_matrix_storage_of
    //   function: the storage that keeps each of _elements, numbers, one at
    // least, as it is: the widest one's type where each is a float, a double
    // or a long double, to which each widens exactly, or a complex number or
    // a quaternion of such parts, the widest type's quaternions where any is
    // one, a real element x then x + 0im + 0jm + 0km and a complex one x + yi
    // x + yim + 0jm + 0km (S386), and otherwise its complex numbers where any
    // is one, a real element x then x + 0i (S348); values where any is exact
    // or boxed (S239).
    inline expr_matrix_storage
    expr_matrix_storage_of(
        const std::vector<value>& _elements
    )
    {
        expr_matrix_storage widest = expr_matrix_storage::floats;

        // the widest element's type, while each is a dense one
        for (std::size_t i = 0; i < _elements.size(); ++i)
        {
            const expr_matrix_storage storage =
                expr_matrix_element_storage(_elements[i]);

            // an exact element, or a boxed one: values
            if (storage == expr_matrix_storage::values)
            {
                return expr_matrix_storage::values;
            }

            widest = expr_matrix_wider(widest,
                                       storage);
        }

        return widest;
    }

NS_END  // internal


//==============================================================================
// 3.  MATRIX VALUES
//==============================================================================


// 3.1    Making and reading
//------------------------------------------------------------------------------
// 3.1.1
// expr_matrix
/**
 * @brief A matrix value of the shape given, from its elements.
 *
 * @param[in] _rows      its number of rows, 1 at least, or 0 with 0
 *                       columns, the empty matrix (S260).
 * @param[in] _columns   its number of columns, 1 at least, or 0 with 0
 *                       rows.
 * @param[in] _elements  its elements, row by row, each a number.
 * @return the matrix, a value of its own, each element kept as it is given
 *         (S239), dense, real, complex or of quaternions, where every element
 *         is a float, a double, a long double or a complex number or a
 *         quaternion of such parts (S348, S386), carrying every flag they
 *         do, of values where it has none; the first element that is an
 *         error, where one is; a domain error, where an element is no number
 *         (S241); or bad_shape, where the shape has no row or no column but
 *         not neither, or _elements holds other than _rows * _columns.
 */
D_NODISCARD inline expr_result<value>
expr_matrix(
    std::size_t               _rows,
    std::size_t               _columns,
    const std::vector<value>& _elements
)
{
    // a shape of no element but the empty matrix's
    if ( ( (_rows == 0) ||
           (_columns == 0) ) &&
         ( (_rows != 0) ||
           (_columns != 0) ) )
    {
        return err<value, expr_error>(
            expr_error(expr_status::bad_shape,
                       expr_no_offset,
                       "expr_matrix: a matrix has a row and a column at "
                       "least, or neither, not " + std::to_string(_rows) +
                           " by " + std::to_string(_columns)));
    }

    // elements not of the shape: none for the empty matrix
    if ( (_rows == 0)
             ? (!_elements.empty())
             : ( ((_elements.size() % _rows) != 0) ||
                 ((_elements.size() / _rows) != _columns) ) )
    {
        return err<value, expr_error>(
            expr_error(expr_status::bad_shape,
                       expr_no_offset,
                       "expr_matrix: " + std::to_string(_elements.size()) +
                           " elements, not those of a matrix of " +
                           std::to_string(_rows) + " by " +
                           std::to_string(_columns)));
    }

    unsigned flags = 0u;

    // an element's error passes, the first
    for (std::size_t i = 0; i < _elements.size(); ++i)
    {
        if (_elements[i].kind() == D_MATH_VALUE_ERROR)
        {
            return ok<value, expr_error>(_elements[i]);
        }
    }

    // an element that is no number: a domain error; every element's flags
    for (std::size_t i = 0; i < _elements.size(); ++i)
    {
        if (!_elements[i].is_number())
        {
            return ok<value, expr_error>(value(D_MATH_VALUE_ERROR_DOMAIN));
        }

        flags |= _elements[i].flags();
    }

    const internal::expr_matrix_storage storage =
        ( _elements.empty() ? internal::expr_matrix_storage::values
                            : internal::expr_matrix_storage_of(_elements) );
    std::unique_ptr<internal::expr_matrix_box> box(
        new internal::expr_matrix_box(expr_shape(_rows,
                                                 _columns),
                                      storage));

    // the elements, as the storage holds them
    internal::expr_matrix_fill_box(*box,
                                   _elements);

    return ok<value, expr_error>(internal::expr_matrix_adopt(box.release(),
                                                             flags));
}

// 3.1.2
// expr_vector
/**
 * @brief A vector value, from its elements: a matrix of one row, the shape
 *        a vector literal [1, 2, 3] is (S240).
 *
 * @param[in] _elements  its elements, in order, each a number.
 * @return the vector, a matrix of 1 by the elements' number, as expr_matrix
 *         gives it; of no element, the empty matrix, 0 by 0 (S260).
 */
D_NODISCARD inline expr_result<value>
expr_vector(
    const std::vector<value>& _elements
)
{
    return expr_matrix( ( _elements.empty() ? 0 : 1 ),
                       _elements.size(),
                       _elements);
}

// 3.1.3
// expr_is_matrix
//   function: whether a value is a vector or a matrix: a value of the kind,
// by its rank.
D_NODISCARD inline bool
expr_is_matrix(
    const value& _value
)
{
    return internal::expr_matrix_held(_value.c_value());
}

// 3.1.4
// expr_shape_of
/**
 * @brief A matrix value's shape.
 *
 * @param[in] _value  the value.
 * @return its shape; or bad_shape, where it is no matrix.
 */
D_NODISCARD inline expr_result<expr_shape>
expr_shape_of(
    const value& _value
)
{
    // no matrix
    if (!expr_is_matrix(_value))
    {
        return err<expr_shape, expr_error>(
            expr_error(expr_status::bad_shape,
                       expr_no_offset,
                       "expr_shape_of: the value is no matrix"));
    }

    return ok<expr_shape, expr_error>(
        internal::expr_matrix_box_of(_value.c_value()).shape);
}

// 3.1.5
// expr_element
/**
 * @brief An element of a matrix value, by its row and column from 0.
 *
 * @param[in] _value   the matrix.
 * @param[in] _row     the row.
 * @param[in] _column  the column.
 * @return the element, a number: one of the matrix's values, or a float, a
 *         double or a long double of its dense array; or bad_shape, where
 *         the value is no matrix or has no such place.
 */
D_NODISCARD inline expr_result<value>
expr_element(
    const value& _value,
    std::size_t  _row,
    std::size_t  _column
)
{
    // no matrix
    if (!expr_is_matrix(_value))
    {
        return err<value, expr_error>(
            expr_error(expr_status::bad_shape,
                       expr_no_offset,
                       "expr_element: the value is no matrix"));
    }

    const internal::expr_matrix_box& box =
        internal::expr_matrix_box_of(_value.c_value());

    // no such place
    if ( (_row >= box.shape.rows) ||
         (_column >= box.shape.columns) )
    {
        return err<value, expr_error>(
            expr_error(expr_status::bad_shape,
                       expr_no_offset,
                       "expr_element: no element at row " +
                           std::to_string(_row) + ", column " +
                           std::to_string(_column) + " of a matrix of " +
                           std::to_string(box.shape.rows) + " by " +
                           std::to_string(box.shape.columns)));
    }

    return ok<value, expr_error>(
        internal::expr_matrix_element_of(_value,
                                         (_row * box.shape.columns) +
                                             _column));
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// the arithmetic the kind's table names, which section 3.2 defines
// (expr_matrix_negated, expr_matrix_operation): included after this
// part, so that the part stands alone
#include "./matrix_kind_arithmetic.hpp"  // section 3.2


#endif  // DJINTERP_MATH_EXPR_MATRIX_KIND_STORAGE_HPP
