/*******************************************************************************
* djinterp [cpp][math]                                 matrix_kind_transpose.hpp
*
* One part of matrix_kind.hpp (S437), its sections 3.3 and 3.4: the
* transpose and the conjugate transpose, each element's function, and
* blocks. matrix_kind.hpp describes the kind whole.
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
* path:      /inc/djinterp/cpp/modules/math/expr/matrix_kind_transpose.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
3.  MATRIX VALUES
    -------------
    3.  The transpose
         1.  expr_transpose
         2.  expr_transpose_value
         3.  expr_matrix_each, expr_matrix_complex_of
    4.  Blocks
         1.  expr_matrix_beside, expr_matrix_stacked
*/

#ifndef DJINTERP_MATH_EXPR_MATRIX_KIND_TRANSPOSE_HPP
#define DJINTERP_MATH_EXPR_MATRIX_KIND_TRANSPOSE_HPP 1

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
#include "./matrix_kind_arithmetic.hpp"         // the part before it


NS_DJINTERP
NS_MATH

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


#endif  // DJINTERP_MATH_EXPR_MATRIX_KIND_TRANSPOSE_HPP
