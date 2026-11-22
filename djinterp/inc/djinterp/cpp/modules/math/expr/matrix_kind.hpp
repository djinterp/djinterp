/*******************************************************************************
* djinterp [cpp][math]                                           matrix_kind.hpp
*
* Vectors and matrices as values of the run-time expression language: the
* matrix kind, its values, its operators, transposes and blocks; roadmap
* Phase 3, 3a and 3b, and 3c's empty matrix, blocks and negative powers,
* matrices of complex numbers, 3e's third part, and matrices of quaternions,
* 3f's third part. Linear algebra's functions of them are
* matrix_function.hpp's.
*   THE KIND (3a) is a boxed kind of the value layer, as the wide integer is
* (c/math/value_wide.h), so that value.h meets it without change: its table is
* written here, in C++, from C++11, as the language that makes matrices is
* (S237). It is one kind, "matrix", of the rank 1000, past every number's, so
* that in an operation of a matrix and a number the matrix's table handles it;
* a matrix is no number (S238). It keeps each element as it is given: as a
* dense array of float, double or long double where every element is one of
* those, in the widest one's type, to which each widens exactly, or of
* math::complex of the widest type where any element is a complex number of
* such parts, a real element x then x + 0i (S348), or of math::quaternion of
* the widest type where any is a quaternion of such parts, a real element x
* then x + 0im + 0jm + 0km and a complex one x + yi x + yim + 0jm + 0km
* (S386); and as math::values, each of its own kind, where any element is
* exact or boxed, an exact complex number or quaternion among them. So no
* element is rounded by being put in a matrix, and a
* matrix of exact numbers is exact (S239). A vector is a matrix of one row or
* one column: expr_vector's, a vector literal's, is one row (S240). An element
* is a number: an element's error passes, the first, and an element that is no
* number, a boolean, none or a matrix, is a domain error, as in arithmetic; a
* matrix has a row and a column at least (S241), or is the empty matrix, of
* neither, 0 by 0 (S260). A matrix compares equal to a matrix of its shape
* whose elements are equal, place by place, and stands in no order otherwise
* (S242). A vector literal [1, 2, 3] and a matrix literal [1, 2; 3, 4] make
* one (parse.hpp, evaluate.hpp), [] the empty one; this header makes and reads
* them in C++.
*   THE OPERATORS (3b) are the value's own, through the kind's table: A + B
* and A - B of one shape, -A, A * s, s * A and A / s of a matrix and a number,
* and A * B where A's columns are B's rows; any other operation on a matrix is
* a domain error, A + s, s / A and A / B among them, and so is one of shapes
* that do not fit; a 1 by 1 matrix stays a matrix (S246). Each is computed by
* linear algebra's kernels, a dense matrix's by the C core's vec.h and mat.h
* in the wider type, one holding values by the generic path in the value's own
* arithmetic, exact wherever its elements are (S247); a dense complex one's in
* the wider type, complex where either operand is, element by element by
* math::complex's operators, a real operand's element or a real scalar taken
* as itself (C99's G.5), a product's each element's parts rounded once from
* Dot2's pair, as the face's kernels sum it, a real operand's products with
* the other's parts alone (S349, S412); and a dense one of quaternions' in the
* wider type, by the generic kernels on the quaternion kind's operands, each
* element as the kind computes its two, a real or complex operand taken as
* itself, each product in its order, a number's product with a matrix too,
* A / q being A inv(q) (S277, S387). Beside them, the conjugate transpose A'
* (S273); A ^ n of a square matrix and a whole n, by repeated squaring as the
* C core's d_mat_power squares (S248, S262), A ^ 0 the identity of A's
* storage and A ^ (-n) the inverse's power (S259); and indexing, from 1:
* A[i, j] an element, v[i] a vector's, A[i] another matrix's row i, an index
* any whole number, exact or not (S249).
*   BLOCKS. A literal's elements are blocks (S261): a row's numbers and
* matrices stand beside one another, each of the first's rows, and a
* matrix's rows one above another, each of the first's columns, so that
* [A, b] is A with the column b beside it and [A; B] A above B; the empty
* matrix is skipped, and blocks that do not fit are a domain error.
*   COMPLEX NUMBERS (complex_kind.hpp) are matrices' elements from step 23's
* third part (S273, S348). A' conjugates each, as a complex number's z' is
* its conjugate (S333), and transpose(A) does not; real, imag, conj and arg
* of a matrix are its elements', and complex(A, B) of two matrices of one
* shape the matrix of complex(a_ij, b_ij) (S350).
*   QUATERNIONS (quaternion_kind.hpp, S275) are matrices' elements from step
* 24's third part, chunk 46 (S386 to S390), whose expr_status::not_yet (S369) is
* gone: A' conjugates each, transpose(A) does not, real and conj of a matrix
* are its elements', and imag, arg and complex of one a domain error; a sum
* of a matrix and a quaternion stays a domain error, as a matrix plus a
* number is (S246). Their rotations' matrices are rotation.hpp's (S391).
*   ONE HEADER FOR IT. Until chunk 48 this header was matrix.hpp, of the
* name of linear algebra's class (linear_algebra/matrix.hpp), and held
* linear algebra's functions of matrix values too, 7,979 lines; they are
* matrix_function.hpp's now, which includes this header (S400, S401).
*   STEPS. Step 19 (3a) filled the kind, expr_matrix, expr_vector,
* expr_is_matrix, expr_shape_of and expr_element; step 20 (3b) filled
* expr_transpose and the kind's arithmetic, powers and indexing; step 21
* (3c) filled the empty matrix, blocks and negative powers; step 23 (3e) a
* complex number's transpose, and in its third part, chunk 43, matrices of
* complex numbers: their storage, arithmetic and transposes, the parts of a
* matrix and complex(A, B); step 24 (3f), its second part, in chunk 45, the
* refusals of quaternions (S369), and its third, chunk 46, matrices of
* them: their storage, arithmetic and transposes (S386, S387).
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/matrix_kind.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.15
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
    3.  The transpose
         1.  expr_transpose
         2.  expr_transpose_value
         3.  expr_matrix_each, expr_matrix_complex_of
    4.  Blocks
         1.  expr_matrix_beside, expr_matrix_stacked
*/

#ifndef DJINTERP_MATH_EXPR_MATRIX_KIND_HPP
#define DJINTERP_MATH_EXPR_MATRIX_KIND_HPP 1

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
    // Gauss-Jordan inverse and its elimination of [A | B],
    // d_cmat_solve_fast, unrefined as the language's other solves are, as
    // LAPACK's zgetrf, zgetri and zgesv, each product and quotient cplx.h's
    // (S269, S351, S413); a pole for a singular A; the trace by the
    // kernels', each part Sum2's, rounded once (S411).
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
        std::vector<core_type>       scratch(_n * _n);

        // singular: a pole, by cmat.h's elimination, unrefined, as the
        // language's other solves are (S406, S413)
        if (!d_cmat_solve_fast(x.data(),
                               scratch.data(),
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

// 3.3    The transpose
//------------------------------------------------------------------------------
// 3.3.1
// expr_transpose
/**
 * @brief A matrix value's conjugate transpose, the language's A', or its
 *        transpose.
 *
 * @param[in] _value      the matrix.
 * @param[in] _conjugate  true, the default, for A', the conjugate
 *                        transpose (S273); false for transpose(A).
 * @return the matrix of its columns as its rows, each complex element and
 *         quaternion conjugated where _conjugate and every other as it is, a
 *         value of its own; or bad_shape, where the value is no matrix. (The
 *         language's A' of a real number is the number itself, of a complex
 *         number or a quaternion its conjugate, and of an error the error:
 *         expr_transpose_value.)
 */
D_NODISCARD inline expr_result<value>
expr_transpose(
    const value& _value,
    bool         _conjugate = true
)
{
    // no matrix
    if (!expr_is_matrix(_value))
    {
        return err<value, expr_error>(
            expr_error(expr_status::bad_shape,
                       expr_no_offset,
                       "expr_transpose: the value is no matrix"));
    }

    return ok<value, expr_error>(internal::expr_matrix_transposed(_value,
                                                                  _conjugate));
}

NS_INTERNAL

    // 3.3.2
    // expr_transpose_value
    //   function: the language's A' as a value, or, where _conjugate is
    // false, its transpose(A): a matrix's conjugate transpose, or its
    // transpose; a complex number's or a quaternion's conjugate, as A' is
    // the conjugate transpose (S273, S277), or itself; a real number itself,
    // as a number is its own transpose, and an error itself; a domain error
    // for a boolean or none.
    inline value
    expr_transpose_value(
        const value& _value,
        bool         _conjugate = true
    )
    {
        // a matrix: its conjugate transpose, or its transpose
        if (expr_is_matrix(_value))
        {
            return expr_matrix_transposed(_value,
                                          _conjugate);
        }

        // a complex number: its conjugate, or itself
        if (expr_is_complex(_value))
        {
            return ( _conjugate ? expr_conj(_value)
                                : _value );
        }

        // a quaternion: its conjugate, or itself
        if (expr_is_quaternion(_value))
        {
            return ( _conjugate ? expr_quaternion_conj(_value)
                                : _value );
        }

        // a number, or an error: itself
        if ( (_value.is_number()) ||
             (_value.kind() == D_MATH_VALUE_ERROR) )
        {
            return _value;
        }

        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

NS_END  // internal

// 3.3.3
// expr_matrix_each, expr_matrix_complex_of
//   functions: the language's real, imag, conj and arg of a matrix, each
// element's by the function _f, complex_kind.hpp's or, for real and conj,
// quaternion_kind.hpp's, which takes every number, and complex(A, B) of two
// matrices of one shape, each pair's, as MATLAB's, NumPy's and Julia's are
// element by element (S350): the matrix of the results, as expr_matrix
// keeps them, dense where each is; an element's error passes, the first,
// imag and arg of a quaternion element and complex of one a domain error
// (S275). complex(A, B) of a matrix and a number, or of two shapes, is a
// domain error, as A + b is (S246).
NS_INTERNAL

    inline value
    expr_matrix_each(
        const value& _a,
        value      (*_f)(const value&)
    )
    {
        const std::vector<value> x = expr_matrix_values_of(_a);
        std::vector<value>       out;

        out.reserve(x.size());

        // element by element
        for (std::size_t i = 0; i < x.size(); ++i)
        {
            out.push_back(_f(x[i]));
        }

        return expr_matrix_of_values(expr_matrix_box_of(_a.c_value()).shape,
                                     out);
    }

    inline value
    expr_matrix_complex_of(
        const value& _a,
        const value& _b
    )
    {
        // an error passes, the first
        if (_a.kind() == D_MATH_VALUE_ERROR)
        {
            return _a;
        }

        if (_b.kind() == D_MATH_VALUE_ERROR)
        {
            return _b;
        }

        // two matrices of one shape
        if ( (!expr_matrix_held(_a.c_value())) ||
             (!expr_matrix_held(_b.c_value())) ||
             (expr_matrix_box_of(_a.c_value()).shape !=
                  expr_matrix_box_of(_b.c_value()).shape) )
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        const std::vector<value> re = expr_matrix_values_of(_a);
        const std::vector<value> im = expr_matrix_values_of(_b);
        std::vector<value>       made;

        made.reserve(re.size());

        // pair by pair
        for (std::size_t i = 0; i < re.size(); ++i)
        {
            made.push_back(expr_complex(re[i],
                                        im[i]));
        }

        return expr_matrix_of_values(expr_matrix_box_of(_a.c_value()).shape,
                                     made);
    }

NS_END  // internal

// 3.4    Blocks
//------------------------------------------------------------------------------
// 3.4.1
// expr_matrix_beside, expr_matrix_stacked
//   functions: a literal's value of its elements' (S261): a row of blocks,
// numbers and matrices, beside one another, each of the first's rows, its
// columns theirs in turn; and a matrix of rows of blocks, one above
// another, each of the first's columns. A number is a block of 1 by 1, and
// the empty matrix one that is skipped, so that [A, []] is A; of none,
// the empty matrix. An element's error passes, the first; one that is no
// number and no matrix, a boolean or none, is a domain error, and so are
// blocks of rows, or rows of columns, that do not fit, as in the
// arithmetic of shapes (S246). A row of numbers alone is expr_vector's.
NS_INTERNAL

    // expr_block_shape
    //   function: a block's shape: a matrix's, or a number's 1 by 1.
    inline expr_shape
    expr_block_shape(
        const value& _block
    )
    {
        return ( expr_matrix_held(_block.c_value())
                     ? expr_matrix_box_of(_block.c_value()).shape
                     : expr_shape(1,
                                  1) );
    }

    // expr_blocks_fail
    //   function: whether _blocks hold an error, and if so the first, in
    // _out, which passes before any shape is asked of them. What is no
    // number and no matrix, a boolean or none, is a block of 1 by 1 whose
    // element expr_matrix refuses, a domain error, as it refuses any; a
    // quaternion is a number, a block of 1 by 1 (S386).
    inline bool
    expr_blocks_fail(
        const std::vector<value>& _blocks,
        value&                    _out
    )
    {
        // an error passes, the first
        for (std::size_t i = 0; i < _blocks.size(); ++i)
        {
            if (_blocks[i].kind() == D_MATH_VALUE_ERROR)
            {
                _out = _blocks[i];

                return true;
            }
        }

        return false;
    }

    // expr_block_elements
    //   function: a block's elements, row by row: a matrix's, each dense
    // one's with its flags, or the number itself.
    inline std::vector<value>
    expr_block_elements(
        const value& _block
    )
    {
        return ( expr_matrix_held(_block.c_value())
                     ? expr_matrix_values_of(_block)
                     : std::vector<value>(1,
                                          _block) );
    }

    // expr_matrix_beside
    //   function: a row of blocks, beside one another.
    inline value
    expr_matrix_beside(
        const std::vector<value>& _blocks
    )
    {
        std::vector<std::vector<value> > elements;
        std::vector<value>               out;
        std::size_t                      rows    = 0;
        std::size_t                      columns = 0;
        value                            failed;

        // an error passes, the first
        if (expr_blocks_fail(_blocks,
                             failed))
        {
            return failed;
        }

        // each block not empty, of the first's rows
        for (std::size_t i = 0; i < _blocks.size(); ++i)
        {
            const expr_shape shape = expr_block_shape(_blocks[i]);

            if ( (shape.rows == 0) &&
                 (shape.columns == 0) )
            {
                continue;
            }

            if ( (!elements.empty()) &&
                 (shape.rows != rows) )
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            rows     = shape.rows;
            columns += shape.columns;
            elements.push_back(expr_block_elements(_blocks[i]));
        }

        // row by row, each block's row in turn
        for (std::size_t r = 0; r < rows; ++r)
        {
            for (std::size_t b = 0; b < elements.size(); ++b)
            {
                const std::size_t width = (elements[b].size() / rows);

                out.insert(out.end(),
                           elements[b].begin() + (r * width),
                           elements[b].begin() + ((r + 1) * width));
            }
        }

        return expr_matrix_of_values(expr_shape(rows,
                                                columns),
                                     out);
    }

    // expr_matrix_stacked
    //   function: rows of blocks, each a row's value, one above another.
    inline value
    expr_matrix_stacked(
        const std::vector<value>& _rows
    )
    {
        std::vector<value> out;
        std::size_t        rows    = 0;
        std::size_t        columns = 0;
        bool               begun   = false;
        value              failed;

        // an error passes, the first
        if (expr_blocks_fail(_rows,
                             failed))
        {
            return failed;
        }

        // each row not empty, of the first's columns
        for (std::size_t i = 0; i < _rows.size(); ++i)
        {
            const expr_shape shape = expr_block_shape(_rows[i]);

            if ( (shape.rows == 0) &&
                 (shape.columns == 0) )
            {
                continue;
            }

            if ( (begun) &&
                 (shape.columns != columns) )
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            const std::vector<value> elements =
                expr_block_elements(_rows[i]);

            begun    = true;
            rows    += shape.rows;
            columns  = shape.columns;
            out.insert(out.end(),
                       elements.begin(),
                       elements.end());
        }

        return expr_matrix_of_values(expr_shape(rows,
                                                columns),
                                     out);
    }

NS_END  // internal


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_MATRIX_KIND_HPP
