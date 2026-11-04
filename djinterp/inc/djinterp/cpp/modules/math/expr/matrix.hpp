/*******************************************************************************
* djinterp [cpp/math]                                                 matrix.hpp
*
* Vectors and matrices as values of the run-time expression language;
* roadmap Phase 3, 3a to 3c, and matrices of complex numbers, 3e's third
* part.
*   THE KIND (3a) is a boxed kind of the value layer, as the wide integer is
* (c/math/value_wide.h), so that value.h meets it without change: its table is
* written here, in C++, from C++11, as the language that makes matrices is
* (S237). It is one kind, "matrix", of the rank 1000, past every number's, so
* that in an operation of a matrix and a number the matrix's table handles it;
* a matrix is no number (S238). It keeps each element as it is given: as a
* dense array of float, double or long double where every element is one of
* those, in the widest one's type, to which each widens exactly, or of
* math::complex of the widest type where any element is a complex number of
* such parts, a real element x then x + 0i (S348); and as math::values, each
* of its own kind, where any element is exact or boxed, an exact complex
* number among them. So no element is rounded by being put in a matrix, and a
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
* as itself (C99's G.5), a product's each element summed from 0 in order, as
* the face's kernels sum it (S349). Beside them, the conjugate transpose A'
* (S273); A ^ n of a square matrix and a whole n, by repeated squaring as the
* C core's d_mat_power squares (S248, S262), A ^ 0 the identity and A ^ (-n)
* the inverse's power (S259); and indexing, from 1: A[i, j] an element, v[i] a
* vector's, A[i] another matrix's row i, an index any whole number, exact or
* not (S249).
*   BLOCKS. A literal's elements are blocks (S261): a row's numbers and
* matrices stand beside one another, each of the first's rows, and a
* matrix's rows one above another, each of the first's columns, so that
* [A, b] is A with the column b beside it and [A; B] A above B; the empty
* matrix is skipped, and blocks that do not fit are a domain error.
*   THE FUNCTIONS (3c) det, inv, trace, norm, eig, dot, cross and solve, of
* matrices, or of numbers as scalars (S256), which the function table
* (function.hpp) calls by name: exactly for exact elements, by the generic
* kernels, a solution by the elimination of [A | B] (S257); in a dense type
* by the C core's, LU with partial pivoting, Gauss-Jordan and the
* eigenvalues' Jacobi and Francis's double-shift QR (S257, S258, S269), the
* norm scaled where its squares would leave the range (S256); a matrix of
* complex numbers' in the widest of its elements' types, by cmat.h's LU,
* Gauss-Jordan and elimination of [A | B] where dense (S351). norm is of the
* moduli, and dot(a, b) sums conj(a_i) b_i (S273, S351).
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
* under which one of 700 rows ran out.
*   COMPLEX NUMBERS (complex.hpp) are matrices' elements from step 23's
* third part (S273, S348). A' conjugates each, as a complex number's z' is
* its conjugate (S333), and transpose(A) does not; real, imag, conj and arg
* of a matrix are its elements', and complex(A, B) of two matrices of one
* shape the matrix of complex(a_ij, b_ij) (S350).
*   STEPS. Step 19 (3a) filled the kind, expr_matrix, expr_vector,
* expr_is_matrix, expr_shape_of and expr_element; step 20 (3b) filled
* expr_transpose and the kind's arithmetic, powers and indexing; step 21
* (3c) filled the functions, the empty matrix, blocks and negative powers;
* step 23 (3e) a complex number's transpose, and in its third part, chunk
* 43, matrices of complex numbers: their storage, arithmetic and
* transposes, the parts of a matrix, complex(A, B), linear algebra's
* functions of them, and complex eigenvalues.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/matrix.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.02
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
    5.  The functions
         1.  Operands
         2.  The square algebra
         3.  Norms and products
         4.  Eigenvalues
         5.  expr_function_shape
         6.  Of one matrix
         7.  Of two
*/

#ifndef DJINTERP_MATH_EXPR_MATRIX_HPP
#define DJINTERP_MATH_EXPR_MATRIX_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <algorithm>  // std::stable_sort
#include <cmath>      // std::floor, std::frexp, std::ldexp, std::sqrt
#include <cstddef>    // std::size_t
#include <iomanip>    // std::setprecision
#include <limits>     // std::numeric_limits
#include <locale>     // std::locale
#include <memory>     // std::unique_ptr
#include <new>        // std::bad_alloc
#include <sstream>    // std::ostringstream
#include <string>     // std::string, std::to_string
#include <vector>     // std::vector
// djinterp
#include "../../../../c/modules/math/cmat.h"        // d_cmat_determinant,
                                                    // _inverse, _solve,
                                                    // _eigen, _hermitian_eigen
#include "../../../../c/modules/math/decomp.h"      // d_decomp_lu, _lu_det,
                                                    // _lu_solve, _eigenvalues,
                                                    // _sym_eigen
#include "../../../../c/modules/math/elementary.h"  // d_math_sqrt
#include "../../../../c/modules/math/value.h"       // d_math_boxed_kind,
                                                    // d_math_value_cmp
#include "../linear_algebra/linalg_common.hpp"      // linalg_kernel
#include "../math_common.hpp"                       // NS_MATH
#include "../value.hpp"                             // value
#include "./complex.hpp"                            // expr_is_complex,
                                                    // expr_conj
#include "./expr_common.hpp"                        // expr_result


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
    // kind; or dense, as floats, doubles or long doubles (S239), or as
    // complex numbers of them, math::complex<T>, the C core's struct, row by
    // row as LAPACK lays a complex matrix out but by rows (S273, S348).
    // Each complex storage lies 3 past its parts' real one.
    enum class expr_matrix_storage
    {
        values               = 1,   // math::values, each a number of its own
                                    // kind
        floats               = 2,   // a dense array of float
        doubles              = 3,   // of double
        long_doubles         = 4,   // of long double
        complex_floats       = 5,   // of math::complex<float>
        complex_doubles      = 6,   // of math::complex<double>
        complex_long_doubles = 7    // of math::complex<long double>
    };

    // expr_storage_is_complex, expr_storage_real_of, expr_storage_complex_of
    //   functions: whether a storage holds complex numbers; a dense
    // storage's real type, a complex one's parts' and a real one's own; and
    // the complex storage of a dense one's type. Values are values in each.
    D_CONSTEXPR inline bool
    expr_storage_is_complex(
        expr_matrix_storage _storage
    ) D_NOEXCEPT
    {
        return (static_cast<int>(_storage) >
                static_cast<int>(expr_matrix_storage::long_doubles));
    }

    D_CONSTEXPR inline expr_matrix_storage
    expr_storage_real_of(
        expr_matrix_storage _storage
    ) D_NOEXCEPT
    {
        return ( expr_storage_is_complex(_storage)
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
                   (expr_storage_is_complex(_storage)) )
                     ? _storage
                     : static_cast<expr_matrix_storage>(
                           static_cast<int>(_storage) + 3) );
    }

    // 2.1.3
    // expr_matrix_box
    //   struct: a matrix's storage: its holders, its shape, how it holds its
    // elements, and the elements, row by row, in the one array its storage
    // names, the others empty. A box is made with one holder.
    struct expr_matrix_box
    {
        std::size_t                              holders;
        expr_shape                               shape;
        expr_matrix_storage                      storage;
        std::vector<value>                       values;
        std::vector<float>                       floats;
        std::vector<double>                      doubles;
        std::vector<long double>                 long_doubles;
        std::vector<math::complex<float> >       complex_floats;
        std::vector<math::complex<double> >      complex_doubles;
        std::vector<math::complex<long double> > complex_long_doubles;

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
              complex_long_doubles()
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
        const d_math_value& _x
    )
    {
        return ( (_x.kind == D_MATH_VALUE_BOXED) &&
                 (_x.type != D_NULLPTR) &&
                 (_x.type->rank == expr_matrix_rank) );
    }

    inline const expr_matrix_box&
    expr_matrix_box_of(
        const d_math_value& _x
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
            d_math_value held = element.detach();

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
// value.h's d_math_boxed_kind entries, each given values of the kind as that
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
        d_math_value_op _op,
        const value&    _a,
        const value&    _b
    );

    inline d_math_value
    expr_matrix_unary(
        d_math_value_op _op,
        d_math_value    _x
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

    inline d_math_value
    expr_matrix_binary(
        d_math_value_op _op,
        d_math_value    _a,
        d_math_value    _b
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
    inline d_math_value_order
    expr_matrix_cmp(
        d_math_value _a,
        d_math_value _b
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
        d_math_value*     _out,
        int*              _direction,
        d_math_value      _x,
        d_math_value_kind _to
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
        d_rational         exact = { 0, 1 };
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
            const d_math_value& held   = _x.c_value();
            const std::size_t   length = held.type->text(D_NULLPTR,
                                                         0,
                                                         held);
            std::string         text(length + 1, '\0');

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
        char*        _buffer,
        std::size_t  _size,
        d_math_value _x
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
    inline const d_math_boxed_kind*
    expr_matrix_kind()
    {
        static const d_math_boxed_kind table = {
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
        d_math_value held = d_internal_math_value_blank(D_MATH_VALUE_BOXED,
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

    // expr_matrix_kind_storage
    //   function: the dense storage of an element of the kind _kind: an
    // inline float's, double's or long double's own type; values for any
    // other kind, exact or boxed.
    inline expr_matrix_storage
    expr_matrix_kind_storage(
        d_math_value_kind _kind
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
    // their complex storage (S348); values for any other number, exact or
    // boxed, an exact complex number among them.
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

        return expr_matrix_kind_storage(_x.kind());
    }

    // expr_matrix_wider
    //   function: the storage two dense storages' elements are kept in
    // together: values where either is; the wider of their types, of
    // complex numbers where either is (S348).
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

        return ( ( (expr_storage_is_complex(_a)) ||
                   (expr_storage_is_complex(_b)) )
                     ? expr_storage_complex_of(wider)
                     : wider );
    }

    // expr_matrix_storage_of
    //   function: the storage that keeps each of _elements, numbers, one at
    // least, as it is: the widest one's type where each is a float, a double
    // or a long double, to which each widens exactly, or a complex number of
    // such parts, the widest type's complex numbers where any is, a real
    // element x then x + 0i (S348); values where any is exact or boxed
    // (S239).
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
 *         (S239), dense, real or complex, where every element is a float, a
 *         double, a long double or a complex number of such parts (S348),
 *         carrying every flag they do, of values where it has none; the
 *         first element that is an error, where one is; a domain error,
 *         where an element is no number (S241); or bad_shape, where the
 *         shape has no row or no column but not neither, or _elements holds
 *         other than _rows * _columns.
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
    switch (storage)
    {
        case internal::expr_matrix_storage::floats:
            internal::expr_matrix_fill(box->floats,
                                       _elements);
            break;

        case internal::expr_matrix_storage::doubles:
            internal::expr_matrix_fill(box->doubles,
                                       _elements);
            break;

        case internal::expr_matrix_storage::long_doubles:
            internal::expr_matrix_fill(box->long_doubles,
                                       _elements);
            break;

        case internal::expr_matrix_storage::complex_floats:
            internal::expr_matrix_fill_complex(box->complex_floats,
                                               _elements);
            break;

        case internal::expr_matrix_storage::complex_doubles:
            internal::expr_matrix_fill_complex(box->complex_doubles,
                                               _elements);
            break;

        case internal::expr_matrix_storage::complex_long_doubles:
            internal::expr_matrix_fill_complex(box->complex_long_doubles,
                                               _elements);
            break;

        case internal::expr_matrix_storage::values:
        default:
            box->values = _elements;
            break;
    }

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
    // a real operand's element taken as itself, summed in order from 0, as
    // the kernels' product sums a math::complex<T> matrix's (S349).
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
        // each row by each column
        for (std::size_t i = 0; i < _rows; ++i)
        {
            for (std::size_t j = 0; j < _columns; ++j)
            {
                math::complex<Type> sum;

                for (std::size_t k = 0; k < _inner; ++k)
                {
                    sum += (_a[(i * _inner) + k] * _b[(k * _columns) + j]);
                }

                _out[(i * _columns) + j] = sum;
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

    // 3.2.2
    // The dense type
    //   functions: the storage two matrices' arithmetic is computed in, and
    // a matrix's and a number's: the wider of two dense types, complex where
    // either is (expr_matrix_wider, 2.1.4); a number's own, a float's, a
    // double's or a long double's, or a complex number's of such parts; or
    // an exact number's, which takes the matrix's, as the tower converts an
    // exact number to the inexact one's type, complex where it is; and
    // values, where either holds values or the number is a boxed inexact
    // one, which handles each element itself.
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

    // expr_matrix_scaled
    //   function: each element of A times _number, or divided by it where
    // _divide; a number's product with A is A's with it, as the product of
    // two numbers does not hang on their order.
    inline value
    expr_matrix_scaled(
        const value& _a,
        const value& _number,
        bool         _divide
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

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> x = expr_matrix_values_of(_a);
        std::vector<value>       out(x.size());

        // on values
        if (_divide)
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
    // computes it (2.2.2), a complex number among the numbers it takes
    // (S349).
    inline value
    expr_matrix_operation(
        d_math_value_op _op,
        const value&    _a,
        const value&    _b
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
                                       (_op == D_MATH_VALUE_OP_DIV) );
        }

        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    // 3.2.6
    // The transpose
    //   function: A's transpose, the matrix of A's columns as its rows, by
    // the kernels' transpose, each element as it is; or, where _conjugate,
    // the conjugate transpose, A', each complex element conjugated and each
    // real one as it is (S273, S350).
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

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        std::vector<value> x = expr_matrix_values_of(_a);
        std::vector<value> out(x.size());

        // each complex element conjugated, where asked
        for (std::size_t i = 0; ( (_conjugate) && (i < x.size()) ); ++i)
        {
            x[i] = ( expr_is_complex(x[i]) ? expr_conj(x[i])
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
    // other A. The inverse of the empty matrix is itself.

    // expr_flagged
    //   function: _x with _flags as well as its own.
    inline value
    expr_flagged(
        const value& _x,
        unsigned     _flags
    )
    {
        d_math_value held = value(_x).detach();

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

    // expr_matrix_algebra_storage
    //   function: the storage a matrix's square algebra is computed in: a
    // dense matrix's own; values for one of exact values, or of a boxed
    // inexact one; otherwise the widest inexact element's type, of complex
    // numbers where any element, exact or not, is one (S351).
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

        // the widest inline inexact element's, complex where an element is,
        // each exact one taken to it; a boxed inexact one: values
        bool complex = false;

        for (std::size_t i = 0; i < a.values.size(); ++i)
        {
            const expr_matrix_storage storage =
                expr_matrix_element_storage(a.values[i]);

            complex = ( (complex) || (expr_is_complex(a.values[i])) );

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

        return ( (complex) ? expr_storage_complex_of(widest)
                           : widest );
    }

    // expr_matrix_nearest
    //   functions: a number's nearest float, double or long double, and its
    // nearest math::complex<T>, a real one's x + 0i.
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
    // Gauss-Jordan inverse and its elimination of [A | B], as LAPACK's
    // zgetrf, zgetri and zgesv, each product and quotient cplx.h's (S269,
    // S351); a pole for a singular A; the trace by the kernels', from 0.
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

        // singular: a pole
        if (!d_cmat_solve(x.data(),
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

            case expr_matrix_storage::values:
            default:
                break;
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
        const d_math_imax most     = std::numeric_limits<d_math_imax>::max();
        d_math_imax       exponent = 0;
        long double       real     = 0.0L;
        value             base     = _a;

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
                              static_cast<d_math_imax>(real))) )
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            exponent = static_cast<d_math_imax>(real);
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

        d_math_imax top   = 1;
        value       power = base;

        // the exponent's highest bit
        while (top <= (exponent / 2))
        {
            top *= 2;
        }

        // each bit below it: the power squared, then times A on the left
        // where the bit is 1; an error, where one is made
        for (d_math_imax bit = (top / 2); bit > 0; bit /= 2)
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
        d_math_imax whole = 0;
        long double real  = 0.0L;

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
 * @return the matrix of its columns as its rows, each complex element
 *         conjugated where _conjugate and every other as it is, a value of
 *         its own; or bad_shape, where the value is no matrix. (The
 *         language's A' of a real number is the number itself, of a complex
 *         number its conjugate, and of an error the error:
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
    // transpose; a complex number's conjugate, as A' is the conjugate
    // transpose (S273), or itself; a real number itself, as a number is its
    // own transpose, and an error itself; a domain error for a boolean or
    // none.
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
// element's by complex.hpp's function _f, and complex(A, B) of two matrices
// of one shape, each pair's, as MATLAB's, NumPy's and Julia's are element
// by element (S350): the matrix of the results, as expr_matrix keeps them,
// dense where each is; an element's error passes, the first. complex(A, B)
// of a matrix and a number, or of two shapes, is a domain error, as A + b
// is (S246).
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
    // element expr_matrix refuses, a domain error, as it refuses any.
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

// 3.5    The functions
//------------------------------------------------------------------------------
// Linear algebra's functions of matrix values (S256), which the function
// table (function.hpp) calls by name: det, the determinant of a square
// matrix; inv, its inverse; trace, the sum of its diagonal; norm, a
// vector's Euclidean norm, a matrix's Frobenius norm; eig, a square matrix's
// eigenvalues, a vector of one row, ascending; dot, two vectors' scalar
// product; cross, the vector product of two vectors of 3 elements; and
// solve(A, B), the X for which A X = B. A number is a scalar, as in Julia:
// det, trace and eig of it are itself, inv its reciprocal, norm its
// magnitude, dot of two its product, solve(a, B) B / a; cross takes none.
// The square algebra is computed in the type S257 gives: exactly, by the
// generic kernels, for exact elements; by the C core's, in a dense type:
// LU with partial pivoting for det and solve, Gauss-Jordan for inv (3.2.7),
// Jacobi for the eigenvalues of a symmetric matrix and Hessenberg QR for
// any other's (S258). Each entry gives the value, an error value where its
// arithmetic fails, a singular matrix's inverse and solution among them, or
// bad_shape where an operand's shape is one the function does not take;
// the value's own functions, which the table calls, give a domain error
// there. An error passes, the first; a boolean or none is a domain error.
NS_INTERNAL

    // 3.5.1
    // Operands
    //   functions: whether an operand passes as it is: an error, the first,
    // or what is no number and no matrix, or a complex number until step
    // 23's third part (S331), a domain error, into _out; a
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

    // 3.5.2
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
    //   function: inv(A): A's inverse, a pole where A is singular (3.2.7);
    // of a number, its reciprocal.
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

    // expr_matrix_solve
    //   function: solve(A, B): X for A X = B; of a number a, B / a.
    inline value
    expr_matrix_solve(
        const value& _a,
        const value& _b
    )
    {
        const value x[2] = { _a, _b };
        value       out;

        // an error, or no operand; a number a, B / a
        if (expr_matrix_passes(x,
                               2,
                               out))
        {
            return out;
        }

        if (!expr_matrix_held(_a.c_value()))
        {
            return (_b / _a);
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

        // by the type it is computed in: the wider of A's and B's, an
        // exact one taking the other's, of complex numbers where either
        // holds one (S351)
        const expr_matrix_storage chosen =
            ( generic ? expr_matrix_storage::values
                      : ( (expr_matrix_all_exact(_a))
                              ? expr_matrix_algebra_storage(_b)
                              : ( (expr_matrix_all_exact(_b))
                                      ? expr_matrix_algebra_storage(_a)
                                      : storage ) ) );

        switch ( ( (expr_matrix_has_complex(_a)) ||
                   (expr_matrix_has_complex(_b)) )
                     ? expr_storage_complex_of(chosen)
                     : chosen )
        {
            case expr_matrix_storage::floats:
                return expr_matrix_solve_in<float>(_a,
                                                   _b,
                                                   n,
                                                   columns,
                                                   shape);

            case expr_matrix_storage::doubles:
                return expr_matrix_solve_in<double>(_a,
                                                    _b,
                                                    n,
                                                    columns,
                                                    shape);

            case expr_matrix_storage::long_doubles:
                return expr_matrix_solve_in<long double>(_a,
                                                         _b,
                                                         n,
                                                         columns,
                                                         shape);

            case expr_matrix_storage::complex_floats:
                return expr_matrix_solve_complex<float>(_a,
                                                        _b,
                                                        n,
                                                        columns,
                                                        shape);

            case expr_matrix_storage::complex_doubles:
                return expr_matrix_solve_complex<double>(_a,
                                                         _b,
                                                         n,
                                                         columns,
                                                         shape);

            case expr_matrix_storage::complex_long_doubles:
                return expr_matrix_solve_complex<long double>(_a,
                                                              _b,
                                                              n,
                                                              columns,
                                                              shape);

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> y = expr_matrix_values_of(_a);
        const std::vector<value> z = expr_matrix_values_of(_b);
        std::vector<value>       scratch(n * n);
        std::vector<value>       solution(n * columns);

        // the elimination of [A | B], exact for exact elements (S257); a
        // pole for a singular A
        if (!kernel::solve(solution.data(),
                           scratch.data(),
                           y.data(),
                           z.data(),
                           n,
                           columns))
        {
            return value(D_MATH_VALUE_ERROR_POLE);
        }

        return expr_matrix_of_values(shape,
                                     solution);
    }

    // 3.5.3
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

    // expr_matrix_moduli_squared
    //   function: the sum, on values, of a matrix's elements' squared moduli,
    // re^2 + im^2 of a complex one and x^2 of a real one, from 0 in order:
    // exact for exact elements (S351).
    inline value
    expr_matrix_moduli_squared(
        const std::vector<value>& _x
    )
    {
        value sum(0);

        // element by element
        for (std::size_t i = 0; i < _x.size(); ++i)
        {
            if (expr_is_complex(_x[i]))
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
                               ( expr_matrix_has_complex(_a)
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

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> x    = expr_matrix_values_of(_a);
        const value              norm = expr_matrix_root(
            expr_matrix_has_complex(_a)
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
            sum = (sum + (( expr_is_complex(_a[i]) ? expr_conj(_a[i])
                                                   : _a[i] ) * _b[i]));
        }

        return sum;
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
            return ( ( expr_is_complex(_a) ? expr_conj(_a)
                                           : _a ) * _b );
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

            case expr_matrix_storage::values:
            default:
                break;
        }

        typedef linalg::internal::linalg_kernel<value> kernel;

        const std::vector<value> y = expr_matrix_values_of(_a);
        const std::vector<value> z = expr_matrix_values_of(_b);

        // of complex numbers: conj(a_i) b_i; of real ones, the kernels'
        if ( (expr_matrix_has_complex(_a)) ||
             (expr_matrix_has_complex(_b)) )
        {
            return expr_matrix_dot_values(y,
                                          z);
        }

        return kernel::dot(y.data(),
                           z.data(),
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

    // 3.5.4
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
                             value(static_cast<d_math_imax>(k)) ) );

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
        d_math_imax&              _lcm
    )
    {
        _lcm = 1;

        for (std::size_t i = 0; i < _a.size(); ++i)
        {
            d_rational  q      = { 0, 1 };
            d_math_imax common = _lcm;
            d_math_imax other  = 0;

            if (!_a[i].to(q))
            {
                return false;
            }

            other = q.den;

            // the greatest common divisor, by Euclid's algorithm
            while (other != 0)
            {
                const d_math_imax rest = (common % other);

                common = other;
                other  = rest;
            }

            if ( (_lcm / common) >
                 (std::numeric_limits<d_math_imax>::max() / q.den) )
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
        d_math_imax&              _lcm
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
        d_math_imax                       _lcm,
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

        const value real_part = ( value(static_cast<d_math_imax>(re)) /
                                  value(_lcm) );
        const value imag_part = ( value(static_cast<d_math_imax>(im)) /
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
        d_math_imax              lcm       = 1;
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

            case expr_matrix_storage::values:
            default:
                break;
        }

        // exact; or of a boxed inexact element, in long double
        return ( expr_matrix_all_exact(_a)
                     ? expr_matrix_eig_exact(_a,
                                             n)
                     : ( expr_matrix_has_complex(_a)
                             ? expr_matrix_eig_complex<long double>(_a,
                                                                    n)
                             : expr_matrix_eig_in<long double>(_a,
                                                               n) ) );
    }

    // 3.5.5
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

// 3.5.6
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
 *         under real_results (S352). Exact for an exact matrix, each
 *         eigenvalue where it is a Gaussian rational; in a dense one's
 *         type otherwise, complex where any element is (S257, S258,
 *         S351). Of a number: itself, its reciprocal, itself, its
 *         magnitude, itself. An error passes; a boolean or none is a
 *         domain error. bad_shape, where the matrix is not square, for
 *         each but expr_norm.
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

// 3.5.7
// Of two
/**
 * @brief dot, cross and solve of two matrix values, or of numbers (S256).
 *
 * @param[in] _a  the first: a vector, a matrix, or a number.
 * @param[in] _b  the second.
 * @return expr_dot: the sum of conj(a_i) b_i over two vectors of one
 *         length, each a row or a column, a real a_i itself (S273, S351),
 *         or conj(a) b of two numbers; expr_cross: two vectors of 3
 *         elements' vector product, of _a's shape, conjugating nothing;
 *         expr_solve: the X for which _a X = _b, _a square and _b of its
 *         rows, or a vector of as many elements, X then of _b's shape, a
 *         pole where _a is singular; of a number _a, _b / _a. Exact for
 *         exact operands, otherwise in their wider dense type, complex
 *         where either holds a complex number (S257, S351). An error
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


#endif  // DJINTERP_MATH_EXPR_MATRIX_HPP
