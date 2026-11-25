/*******************************************************************************
* djinterp [cpp][math]                                  t_expr_face_matrices.hpp
*
* One part of t_expr_face.cpp, the expression language's face test (S438), its
* section 11's first third: the matrix kind, its storage, and the literals. The
* test includes the parts in order, one program; each part includes the one
* before it.
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_face_matrices.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_MATRICES_HPP
#define DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_MATRICES_HPP 1

#include "./t_expr_face_rewrites.hpp"   // the part before


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


namespace
{

    //==========================================================================
    // 11. MATRICES
    //==========================================================================

    // d_tests_same_values
    //   whether two values are the same, as two atoms' numbers are
    // (expr_same_number): of one kind, and equal, a matrix by its kind's
    // comparison.
    bool
    d_tests_same_values(
        const m::value& _a,
        const m::value& _b
    )
    {
        return m::internal::expr_same_number(_a, _b);
    }

    // d_tests_holds_value
    //   whether a fold gave a value, the same as _want.
    bool
    d_tests_holds_value(
        const m::expr_result<m::value>& _result,
        const m::value&                 _want
    )
    {
        return ( (_result.is_ok()) &&
                 (d_tests_same_values(_result.value(), _want)) );
    }

    // d_tests_made
    //   what a step made, where it made a value; none where it failed, so
    // that a check of it fails rather than read what the step does not
    // hold.
    m::value
    d_tests_made(
        const m::expr_result<m::value>& _result
    )
    {
        return ( _result.is_ok() ? _result.value() : m::value() );
    }

    // d_tests_shape
    //   a value's shape, where it is a matrix; 0 by 0 where it is not.
    m::expr_shape
    d_tests_shape(
        const m::value& _x
    )
    {
        const m::expr_result<m::expr_shape> shape = m::expr_shape_of(_x);

        return ( shape.is_ok() ? shape.value() : m::expr_shape() );
    }

    // d_tests_storage
    //   a matrix's storage, as a number; 0, no storage's, where the value
    // is no matrix.
    int
    d_tests_storage(
        const m::value& _x
    )
    {
        return ( m::expr_is_matrix(_x)
                     ? static_cast<int>(m::internal::expr_matrix_box_of(
                           _x.c_value()).storage)
                     : 0 );
    }

    // d_tests_stored_as
    //   whether a value is a matrix of the storage given.
    bool
    d_tests_stored_as(
        const m::value&                   _x,
        m::internal::expr_matrix_storage _storage
    )
    {
        return (d_tests_storage(_x) == static_cast<int>(_storage));
    }

    // d_tests_holders
    //   a matrix's holders, as its box counts them; 0 where the value is no
    // matrix.
    std::size_t
    d_tests_holders(
        const m::value& _matrix
    )
    {
        return ( m::expr_is_matrix(_matrix)
                     ? m::internal::expr_matrix_box_of(
                           _matrix.c_value()).holders
                     : 0 );
    }

    // d_tests_kind_text
    //   a matrix as the kind writes it, in a buffer of _size; empty where
    // the value is no matrix.
    std::string
    d_tests_kind_text(
        const m::value& _x,
        std::size_t     _size
    )
    {
        const dj_math_value& held = _x.c_value();
        std::string          text(_size + 1, '#');

        // no matrix
        if (!m::expr_is_matrix(_x))
        {
            return std::string();
        }

        static_cast<void>(held.type->text(&text[0],
                                          _size,
                                          held));

        return text.substr(0, text.find('\0'));
    }

    // d_tests_two_by_two
    //   the exact matrix [1, 2; 3, 4].
    m::value
    d_tests_two_by_two()
    {
        return d_tests_made(m::expr_matrix(2,
                                           2,
                                           { 1, 2, 3, 4 }));
    }

    // d_tests_matrices
    //   expr_shape; the kind, its table and its entries; expr_matrix,
    // expr_vector, expr_is_matrix, expr_shape_of and expr_element, the
    // storage that keeps each element as given, and their errors; what the
    // kind's arithmetic does not take; expr_transpose; and step 21's (3c)
    // functions' entries, each as a matrix's, a number's and bad_shape.
    void
    d_tests_matrices()
    {
        const m::expr_shape by_default;
        const m::expr_shape two_by_three(2, 3);

        D_TESTS_EXPECT( (by_default.rows == 0) &&
                        (by_default.columns == 0) &&
                        (two_by_three.rows == 2) &&
                        (two_by_three.columns == 3),
                        "a shape: 0 by 0 by default, or its two numbers");
        D_TESTS_EXPECT( (two_by_three == m::expr_shape(2, 3)) &&
                        (two_by_three != m::expr_shape(3, 2)) &&
                        (two_by_three != m::expr_shape(2, 2)) &&
                        (!(two_by_three != m::expr_shape(2, 3))),
                        "shapes equal when both numbers are");

        const m::value                  one(1);
        const m::value                  a    = d_tests_two_by_two();
        const dj_math_boxed_kind* const kind =
            m::internal::expr_matrix_kind();
        const m::value                  row  = d_tests_made(
                                                   m::expr_vector(
                                                       { 1, 2, 3 }));
        const m::value                  col  = d_tests_made(
                                                   m::expr_matrix(
                                                       3,
                                                       1,
                                                       { 1, 2, 3 }));

        D_TESTS_EXPECT( (m::expr_is_matrix(a)) &&
                        (a.kind() == D_MATH_VALUE_BOXED) &&
                        (a.c_value().type == kind) &&
                        (std::strcmp(kind->name, "matrix") == 0) &&
                        (std::strcmp(a.kind_name(), "matrix") == 0) &&
                        (kind->rank == 1000) &&
                        (m::expr_matrix_rank == 1000) &&
                        (a.rank() == 1000) &&
                        (kind->traits == 0u) &&
                        (!a.is_number()) &&
                        (!a.is_exact()),
                        "a matrix: a boxed value of the kind matrix, of rank "
                        "1000, no number");
        D_TESTS_EXPECT( (d_tests_shape(a) == m::expr_shape(2, 2)) &&
                        (d_tests_shape(row) == m::expr_shape(1, 3)) &&
                        (d_tests_shape(col) == m::expr_shape(3, 1)),
                        "its shape: a vector one row");
        D_TESTS_EXPECT( (d_tests_holds_value(m::expr_element(a, 0, 0), 1)) &&
                        (d_tests_holds_value(m::expr_element(a, 0, 1), 2)) &&
                        (d_tests_holds_value(m::expr_element(a, 1, 0), 3)) &&
                        (d_tests_holds_value(m::expr_element(a, 1, 1), 4)) &&
                        (d_tests_holds_value(m::expr_element(col, 2, 0), 3)) &&
                        (d_tests_stored_as(
                             a,
                             m::internal::expr_matrix_storage::values)),
                        "its elements, row by row, exact: held as values");
        D_TESTS_EXPECT( (!m::expr_is_matrix(one)) &&
                        (!m::expr_is_matrix(m::value(2.5))) &&
                        (!m::expr_is_matrix(m::value())) &&
                        (!m::expr_is_matrix(m::value(true))) &&
                        (!m::expr_is_matrix(d_tests_wide_of("12345"))),
                        "no number, boolean or none is a matrix");

        const long double third     = 1.0L / 3.0L;
        const m::value    with_half = d_tests_made(
                                          m::expr_vector(
                                              { 1,
                                                m::value(0.5),
                                                m::value::ratio(1, 3) }));
        const m::value    floats    = d_tests_made(
                                          m::expr_vector(
                                              { m::value(0.5f),
                                                2 }));
        const m::value    pure      = d_tests_made(
                                          m::expr_vector(
                                              { m::value(0.5f),
                                                m::value(0.25f) }));
        const m::value    doubles   = d_tests_made(
                                          m::expr_vector(
                                              { m::value(0.25),
                                                m::value(0.5f) }));
        const m::value    widest    = d_tests_made(
                                          m::expr_vector(
                                              { m::value(0.5f),
                                                m::value(0.25),
                                                m::value(third) }));
        const m::value    real      = d_tests_made(
                                          m::expr_vector(
                                              { 1,
                                                d_tests_real_of(&third) }));
        const m::value    wide      = d_tests_made(
                                          m::expr_vector(
                                              { d_tests_wide_of(
                                                    "123456789012345678901"
                                                    "234567890"),
                                                m::value::ratio(-1, 2) }));

        D_TESTS_EXPECT( (d_tests_stored_as(
                             pure,
                             m::internal::expr_matrix_storage::floats)) &&
                        (d_tests_holds_value(m::expr_element(pure, 0, 1),
                                             m::value(0.25f))) &&
                        (d_tests_stored_as(
                             doubles,
                             m::internal::expr_matrix_storage::doubles)) &&
                        (d_tests_holds_value(m::expr_element(doubles, 0, 1),
                                             m::value(0.5))) &&
                        (d_tests_stored_as(
                             widest,
                             m::internal::expr_matrix_storage::
                                 long_doubles)) &&
                        (d_tests_holds_value(m::expr_element(widest, 0, 0),
                                             m::value(0.5L))) &&
                        (d_tests_holds_value(m::expr_element(widest, 0, 2),
                                             m::value(third))),
                        "every element a float, a double or a long double: "
                        "dense, in the widest one's type, each widened "
                        "exactly");
        D_TESTS_EXPECT( (d_tests_stored_as(
                             with_half,
                             m::internal::expr_matrix_storage::values)) &&
                        (d_tests_holds_value(m::expr_element(with_half, 0, 0),
                                             1)) &&
                        (d_tests_holds_value(m::expr_element(with_half, 0, 1),
                                             m::value(0.5))) &&
                        (d_tests_holds_value(m::expr_element(with_half, 0, 2),
                                             m::value::ratio(1, 3))) &&
                        (d_tests_stored_as(
                             floats,
                             m::internal::expr_matrix_storage::values)) &&
                        (d_tests_holds_value(m::expr_element(floats, 0, 0),
                                             m::value(0.5f))) &&
                        (d_tests_holds_value(m::expr_element(floats, 0, 1),
                                             2)) &&
                        (d_tests_stored_as(
                             real,
                             m::internal::expr_matrix_storage::values)) &&
                        (d_tests_holds_value(m::expr_element(real, 0, 1),
                                             d_tests_real_of(&third))) &&
                        (d_tests_stored_as(
                             wide,
                             m::internal::expr_matrix_storage::values)),
                        "an element exact or boxed: values, each element as "
                        "it is given, none rounded");

        const m::value big = d_tests_made(
                                 m::expr_vector(
                                     { m::value(18446744073709551615ull),
                                       1 }));

        D_TESTS_EXPECT( ((big.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0u) &&
                        ((a.flags() & D_MATH_VALUE_FLAG_INEXACT) == 0u),
                        "a matrix carries its elements' flags");

        const m::value pole(D_MATH_VALUE_ERROR_POLE);
        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( (d_tests_holds_value(m::expr_vector({ 1,
                                                              pole,
                                                              domain }),
                                             pole)) &&
                        (d_tests_holds_value(m::expr_vector({ true,
                                                              pole }),
                                             pole)) &&
                        (d_tests_holds_value(m::expr_vector({ 1,
                                                              true }),
                                             domain)) &&
                        (d_tests_holds_value(m::expr_vector({ m::value(),
                                                              1 }),
                                             domain)) &&
                        (d_tests_holds_value(m::expr_vector({ a,
                                                              1 }),
                                             domain)),
                        "an element's error passes, the first; an element "
                        "that is no number a domain error");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_matrix(0, 2, {}),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: a matrix has a row "
                                         "and a column at least, or "
                                         "neither, not 0 by 2") &&
                        d_tests_is_error(m::expr_matrix(1, 0, {}),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: a matrix has a row "
                                         "and a column at least, or "
                                         "neither, not 1 by 0") &&
                        d_tests_is_error(m::expr_matrix(0, 0, { 1 }),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: 1 elements, not "
                                         "those of a matrix of 0 by 0") &&
                        d_tests_is_error(m::expr_matrix(2, 2, { 1, 2, 3 }),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: 3 elements, not "
                                         "those of a matrix of 2 by 2") &&
                        d_tests_is_error(m::expr_matrix(2, 1, { 1, 2, 3 }),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: 3 elements, not "
                                         "those of a matrix of 2 by 1") &&
                        d_tests_is_error(m::expr_matrix(1, 2, { 1, 2, 3 }),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_matrix: 3 elements, not "
                                         "those of a matrix of 1 by 2"),
                        "a shape of no element but 0 by 0, or elements not "
                        "of the shape: bad_shape");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_shape_of(one),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_shape_of: the value is no "
                                         "matrix") &&
                        d_tests_is_error(m::expr_element(one, 0, 0),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_element: the value is no "
                                         "matrix") &&
                        d_tests_is_error(m::expr_element(a, 2, 0),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_element: no element at row "
                                         "2, column 0 of a matrix of 2 by "
                                         "2") &&
                        d_tests_is_error(m::expr_element(a, 0, 2),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_element: no element at row "
                                         "0, column 2 of a matrix of 2 by "
                                         "2"),
                        "the shape or an element of no matrix, or of no "
                        "such place: bad_shape");

        // the holders: a copy one more, its end one fewer, a move none
        bool held = (d_tests_holders(a) == 1);

        {
            m::value copy = a;
            m::value other(a.c_value());

            held = ( (held) &&
                     (d_tests_holders(a) == 3) );

            m::value moved(std::move(copy));

            held = ( (held) &&
                     (d_tests_holders(a) == 3) &&
                     (copy.kind() == D_MATH_VALUE_NONE) );
        }

        D_TESTS_EXPECT( (held) &&
                        (d_tests_holders(a) == 1),
                        "a box held as an int holds its number: a copy one "
                        "holder more, a value's end one fewer");

        const m::value same  = d_tests_made(
                                   m::expr_matrix(2,
                                                  2,
                                                  { 1.0, 2.0, 3.0, 4.0 }));
        const m::value other = d_tests_made(
                                   m::expr_matrix(2,
                                                  2,
                                                  { 1, 2, 3, 5 }));

        D_TESTS_EXPECT( (a == d_tests_two_by_two()) &&
                        (a == same) &&
                        (!(a == other)) &&
                        (!(a == row)) &&
                        (!(a == one)) &&
                        (!(one == a)) &&
                        (kind->cmp(a.c_value(), one.c_value()) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (kind->cmp(one.c_value(), a.c_value()) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (kind->cmp(a.c_value(), other.c_value()) ==
                             D_MATH_VALUE_UNORDERED) &&
                        (kind->cmp(a.c_value(), same.c_value()) ==
                             D_MATH_VALUE_EQUAL),
                        "two matrices equal where of one shape and their "
                        "elements equal, whatever their storage; no order "
                        "otherwise");

        double        number    = 7.0;
        dj_math_value converted = d_math_value_none();
        int           direction = 5;

        D_TESTS_EXPECT( (d_tests_same_values(abs(a), domain)) &&
                        (d_tests_same_values(a + one, domain)) &&
                        (d_tests_same_values(one - a, domain)) &&
                        (d_tests_same_values(one / a, domain)) &&
                        (d_tests_same_values(a / a, domain)) &&
                        (d_tests_same_values(a - row, domain)) &&
                        (d_tests_same_values(row * row, domain)) &&
                        (!a.to(number)) &&
                        (number == 7.0) &&
                        (!kind->convert(&converted,
                                        &direction,
                                        a.c_value(),
                                        D_MATH_VALUE_DOUBLE)) &&
                        (converted.kind == D_MATH_VALUE_NONE) &&
                        (direction == 5),
                        "what the kind's arithmetic does not take a "
                        "domain error (S246): |A|, a matrix and a number "
                        "added or subtracted, a matrix under a number or "
                        "under a matrix, shapes that do not fit; no matrix "
                        "converts to a number");
        D_TESTS_EXPECT( (d_tests_kind_text(a, 64) == "[1, 2; 3, 4]") &&
                        (d_tests_kind_text(row, 64) == "[1, 2, 3]") &&
                        (d_tests_kind_text(col, 64) == "[1; 2; 3]") &&
                        (d_tests_kind_text(with_half, 64) ==
                             "[1, 0.5, 1/3]") &&
                        (d_tests_kind_text(floats, 64) == "[0.5, 2]") &&
                        (d_tests_kind_text(widest, 64).substr(0, 18) ==
                             "[0.5, 0.25, 0.3333") &&
                        (d_tests_kind_text(wide, 64) ==
                             "[123456789012345678901234567890, -1/2]"),
                        "its text: its rows, each element exact, by its "
                        "kind, or in max_digits10 digits, as it is held");
        D_TESTS_EXPECT( (d_tests_kind_text(a, 4) == "[1,") &&
                        (d_tests_kind_text(a, 1).empty()) &&
                        (m::expr_is_matrix(a)) &&
                        (kind->text(D_NULLPTR, 0, a.c_value()) == 12),
                        "its text cut to the buffer, its length whole");

        D_TESTS_EXPECT( d_tests_is_error(m::expr_transpose(one),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_transpose: the value is no "
                                         "matrix") &&
                        (d_tests_made(m::expr_transpose(a)) ==
                             d_tests_made(m::expr_matrix(2,
                                                         2,
                                                         { 1, 3, 2, 4 }))) &&
                        (d_tests_made(m::expr_transpose(row)) == col),
                        "expr_transpose: a matrix's, its columns its rows; "
                        "of no matrix bad_shape");
        const m::value wide3 = d_tests_made(m::expr_matrix(2,
                                                           3,
                                                           { 1, 2, 3,
                                                             4, 5, 6 }));
        const m::value flat  = d_tests_made(m::expr_matrix(2,
                                                           2,
                                                           { 1, 2, 2, 4 }));

        D_TESTS_EXPECT( d_tests_holds_value(m::expr_det(a),
                                            m::value(-2)) &&
                        d_tests_holds_value(m::expr_det(one),
                                            one) &&
                        d_tests_is_error(m::expr_det(wide3),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_det: a matrix of 2 by 3 is "
                                         "not square"),
                        "expr_det: an exact matrix's, exact; a number's, "
                        "itself; of no square matrix, bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(
                            m::expr_inv(a),
                            d_tests_made(m::expr_matrix(
                                2,
                                2,
                                { -2,
                                  1,
                                  m::value::ratio(3, 2),
                                  m::value::ratio(-1, 2) }))) &&
                        d_tests_holds_value(m::expr_inv(m::value(4)),
                                            m::value::ratio(1, 4)) &&
                        d_tests_holds_value(m::expr_inv(flat),
                                            m::value(
                                                D_MATH_VALUE_ERROR_POLE)) &&
                        d_tests_is_error(m::expr_inv(wide3),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_inv: a matrix of 2 by 3 is "
                                         "not square"),
                        "expr_inv: exact; a number's reciprocal; of a "
                        "singular matrix a pole; of no square, bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(m::expr_trace(a),
                                            m::value(5)) &&
                        d_tests_holds_value(m::expr_trace(one),
                                            one) &&
                        d_tests_is_error(m::expr_trace(row),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_trace: a matrix of 1 by 3 is "
                                         "not square"),
                        "expr_trace: the diagonal's sum; a number's itself; "
                        "of no square, bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(
                            m::expr_norm(d_tests_made(m::expr_vector(
                                { 3, 4 }))),
                            m::value(5)) &&
                        d_tests_holds_value(m::expr_norm(flat),
                                            m::value(5)) &&
                        d_tests_holds_value(m::expr_norm(m::value(-2)),
                                            m::value(2)) &&
                        d_tests_holds_value(m::expr_norm(wide3),
                                            m::internal::value_root_of(
                                                m::value(91))),
                        "expr_norm: Euclidean of a vector, Frobenius of a "
                        "matrix of any shape, exact where its square's "
                        "root is; a number's magnitude");
        D_TESTS_EXPECT( d_tests_holds_value(
                            m::expr_eig(d_tests_made(m::expr_matrix(
                                2,
                                2,
                                { 2, 1, 1, 2 }))),
                            d_tests_made(m::expr_vector({ 1, 3 }))) &&
                        d_tests_holds_value(m::expr_eig(one),
                                            one) &&
                        d_tests_is_error(m::expr_eig(col),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_eig: a matrix of 3 by 1 is "
                                         "not square"),
                        "expr_eig: the eigenvalues, ascending, a row; a "
                        "number's itself; of no square, bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(m::expr_dot(row,
                                                        col),
                                            m::value(14)) &&
                        d_tests_holds_value(m::expr_dot(m::value(2),
                                                        m::value(3)),
                                            m::value(6)) &&
                        d_tests_is_error(m::expr_dot(row,
                                                     one),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_dot: two vectors of one "
                                         "length, not 1 by 3 and 1 by 1") &&
                        d_tests_is_error(m::expr_dot(row,
                                                     a),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_dot: two vectors of one "
                                         "length, not 1 by 3 and 2 by 2"),
                        "expr_dot: two vectors of one length, a row and a "
                        "column among them; two numbers' product; else "
                        "bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(
                            m::expr_cross(d_tests_made(m::expr_vector(
                                              { 1, 0, 0 })),
                                          d_tests_made(m::expr_vector(
                                              { 0, 1, 0 }))),
                            d_tests_made(m::expr_vector({ 0, 0, 1 }))) &&
                        d_tests_holds_value(
                            m::expr_cross(col,
                                          row),
                            d_tests_made(m::expr_matrix(3,
                                                        1,
                                                        { 0, 0, 0 }))) &&
                        d_tests_is_error(m::expr_cross(one,
                                                       one),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_cross: two vectors of 3 "
                                         "elements, not 1 by 1 and 1 by 1"),
                        "expr_cross: the vector product, of the first's "
                        "shape; of no vectors of 3, bad_shape");
        D_TESTS_EXPECT( d_tests_holds_value(
                            m::expr_solve(a,
                                          d_tests_made(m::expr_vector(
                                              { 5, 6 }))),
                            d_tests_made(m::expr_vector(
                                { -4, m::value::ratio(9, 2) }))) &&
                        d_tests_holds_value(
                            m::expr_solve(m::value(2),
                                          row),
                            d_tests_made(m::expr_vector(
                                { m::value::ratio(1, 2),
                                  1,
                                  m::value::ratio(3, 2) }))) &&
                        d_tests_holds_value(
                            m::expr_solve(flat,
                                          d_tests_made(m::expr_vector(
                                              { 1, 1 }))),
                            m::value(D_MATH_VALUE_ERROR_POLE)) &&
                        d_tests_is_error(m::expr_solve(a,
                                                       row),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_solve: a square matrix and "
                                         "one of its rows, not 2 by 2 and "
                                         "1 by 3") &&
                        d_tests_is_error(m::expr_solve(
                                             d_tests_made(m::expr_matrix(
                                                 2,
                                                 3,
                                                 { 1, 2, 3, 4, 5, 6 })),
                                             d_tests_made(m::expr_matrix(
                                                 2,
                                                 1,
                                                 { 1, 2 }))),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_solve: a square matrix and "
                                         "one of its rows, not 2 by 3 and "
                                         "2 by 1"),
                        "expr_solve: X of A X = B, a row solved as a "
                        "column and given as a row; B / a of a number a; "
                        "a singular A a pole; else bad_shape, an A not "
                        "square among them");

        return;
    }

    // d_tests_vector_of, d_tests_matrix_of
    //   the term of a vector literal of its elements; of a matrix literal of
    // its rows, each of its elements.
    m::expr_term
    d_tests_vector_of(
        const std::vector<m::expr_term>& _elements
    )
    {
        return m::expr_application(m::expr_op_kind::vector,
                                   _elements);
    }

    m::expr_term
    d_tests_matrix_of(
        const std::vector<std::vector<m::expr_term> >& _rows
    )
    {
        std::vector<m::expr_term> rows;

        for (std::size_t r = 0; r < _rows.size(); ++r)
        {
            rows.push_back(m::expr_application(m::expr_op_kind::row,
                                               _rows[r]));
        }

        return m::expr_application(m::expr_op_kind::matrix,
                                   rows);
    }

    // d_tests_parse_literals
    //   step 19's literals in the parser layer: a vector of its elements, a
    // matrix of its rows, each element any term; one operand where it
    // stands; a [ after an operand indexing, and no implied product; and
    // every error at its place.
    void
    d_tests_parse_literals()
    {
        const m::expr_term one   = d_tests_literal("1");
        const m::expr_term two   = d_tests_literal("2");
        const m::expr_term three = d_tests_literal("3");
        const m::expr_term four  = d_tests_literal("4");
        const m::expr_term x     = m::expr_variable("x");
        m::expr_syntax     implied;
        m::expr_syntax     bars;

        implied.implied_multiplication = true;
        bars.absolute_bars             = true;

        D_TESTS_EXPECT( d_tests_parses("[1, 2, 3]",
                                       d_tests_vector_of({ one,
                                                           two,
                                                           three })) &&
                        d_tests_parses("[ 1 ]",
                                       d_tests_vector_of({ one })),
                        "[1, 2, 3]: the term vector of its elements");
        D_TESTS_EXPECT( d_tests_parses("[1, 2; 3, 4]",
                                       d_tests_matrix_of({ { one, two },
                                                           { three,
                                                             four } })) &&
                        d_tests_parses("[1; 2]",
                                       d_tests_matrix_of({ { one },
                                                           { two } })),
                        "[1, 2; 3, 4]: the term matrix of its rows, each "
                        "the term row of its elements");
        D_TESTS_EXPECT( d_tests_parses(
                            "[x, 2 * x; f(1, 2), (3); [1], -x]",
                            d_tests_matrix_of(
                                { { x,
                                    d_tests_apply(m::expr_op_kind::mul,
                                                  two,
                                                  x) },
                                  { m::expr_call("f",
                                                 { one,
                                                   two }),
                                    three },
                                  { d_tests_vector_of({ one }),
                                    d_tests_apply(m::expr_op_kind::neg,
                                                  x) } })),
                        "an element any term: a call's commas and what "
                        "brackets hold no element's end");
        D_TESTS_EXPECT( d_tests_parses("-[1] ^ 2 + [1]!",
                                       d_tests_apply(
                                           m::expr_op_kind::add,
                                           d_tests_apply(
                                               m::expr_op_kind::neg,
                                               d_tests_apply(
                                                   m::expr_op_kind::pow,
                                                   d_tests_vector_of(
                                                       { one }),
                                                   two)),
                                           d_tests_apply(
                                               m::expr_op_kind::factorial,
                                               d_tests_vector_of(
                                                   { one })))) &&
                        d_tests_parses("[1] x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     d_tests_vector_of(
                                                         { one }),
                                                     x),
                                       implied) &&
                        d_tests_parses("[|x|]",
                                       d_tests_vector_of(
                                           { m::expr_call("abs",
                                                          { x }) }),
                                       bars) &&
                        d_tests_parses("|[1]| - |x|",
                                       d_tests_apply(
                                           m::expr_op_kind::sub,
                                           m::expr_call(
                                               "abs",
                                               { d_tests_vector_of(
                                                     { one }) }),
                                           m::expr_call("abs",
                                                        { x })),
                                       bars),
                        "a literal one operand where it stands; a * "
                        "implied after it; bars in it");
        D_TESTS_EXPECT( d_tests_parses("x[1]",
                                       d_tests_apply(m::expr_op_kind::index,
                                                     x,
                                                     one)) &&
                        d_tests_parses("[1][2]",
                                       d_tests_apply(m::expr_op_kind::index,
                                                     d_tests_vector_of(
                                                         { one }),
                                                     two)) &&
                        d_tests_parses("2[1]",
                                       d_tests_apply(m::expr_op_kind::index,
                                                     two,
                                                     one),
                                       implied) &&
                        d_tests_is_error(m::expr_parse("x[1"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'[' is never closed"),
                        "a [ after an operand: indexing, and no implied "
                        "product");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("[,]"),
                                         m::expr_status::missing_operand,
                                         1,
                                         "an operand is missing before "
                                         "','") &&
                        d_tests_is_error(m::expr_parse("[1, ]"),
                                         m::expr_status::missing_operand,
                                         4,
                                         "an operand is missing before "
                                         "']'") &&
                        d_tests_is_error(m::expr_parse("[1;]"),
                                         m::expr_status::missing_operand,
                                         3,
                                         "an operand is missing before "
                                         "']'") &&
                        d_tests_is_error(m::expr_parse("[;1]"),
                                         m::expr_status::missing_operand,
                                         1,
                                         "an operand is missing before "
                                         "';'") &&
                        d_tests_is_error(m::expr_parse("[1,,2]"),
                                         m::expr_status::missing_operand,
                                         3,
                                         "an operand is missing before "
                                         "','"),
                        "an empty element: an operand missing before what "
                        "ends it");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("[1, 2; 3]"),
                                         m::expr_status::bad_shape,
                                         8,
                                         "a row of 1 element, where the "
                                         "first row has 2 elements") &&
                        d_tests_is_error(m::expr_parse("[1; 2, 3; 4]"),
                                         m::expr_status::bad_shape,
                                         8,
                                         "a row of 2 elements, where the "
                                         "first row has 1 element") &&
                        d_tests_is_error(m::expr_parse("[1, 2; 3, 4; 5]"),
                                         m::expr_status::bad_shape,
                                         14,
                                         "a row of 1 element, where the "
                                         "first row has 2 elements") &&
                        d_tests_is_error(m::expr_parse("[1; 2, ]"),
                                         m::expr_status::missing_operand,
                                         7,
                                         "an operand is missing before "
                                         "']'") &&
                        d_tests_is_error(m::expr_parse("[1; 2, 3] y"),
                                         m::expr_status::bad_shape,
                                         8,
                                         "a row of 2 elements, where the "
                                         "first row has 1 element"),
                        "a row of more or fewer elements than the first: "
                        "bad_shape, at what ends it, the further left error");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("[1, 2"),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'[' is never closed") &&
                        d_tests_is_error(m::expr_parse("1]"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "']' closes no bracket") &&
                        d_tests_is_error(m::expr_parse("[1)"),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'[' is never closed") &&
                        d_tests_is_error(m::expr_parse("(1]"),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'(' is never closed") &&
                        d_tests_is_error(m::expr_parse("|1]",
                                                       bars),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'|' is never closed") &&
                        d_tests_is_error(m::expr_parse("[(1]"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'(' is never closed"),
                        "a square bracket unmatched, or closing an open of "
                        "another kind: unbalanced");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("1; 2"),
                                         m::expr_status::unexpected_token,
                                         1,
                                         "unexpected ';'") &&
                        d_tests_is_error(m::expr_parse("f(1; 2)"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected ';'") &&
                        d_tests_is_error(m::expr_parse("[(1; 2)]"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected ';'") &&
                        d_tests_is_error(m::expr_parse("[1, 2 3]"),
                                         m::expr_status::unexpected_token,
                                         6,
                                         "unexpected '3'"),
                        "a semicolon outside a literal's brackets, and an "
                        "element's own error: unexpected, at its place");

        return;
    }

    // d_tests_render_literals
    //   literals written back, between square brackets, and read back as
    // themselves; a matrix a fold made written as a literal, and read back
    // to its value.
    void
    d_tests_render_literals()
    {
        D_TESTS_EXPECT( d_tests_renders("[1,2;3,4]", "[1, 2; 3, 4]") &&
                        d_tests_renders("[ x ,2*y ]", "[x, 2 * y]") &&
                        d_tests_renders("[1;2;3]", "[1; 2; 3]") &&
                        d_tests_renders("-[1]^2", "-[1] ^ 2") &&
                        d_tests_renders("([1, 2])!", "[1, 2]!") &&
                        d_tests_renders("[[1, 2], [3]; f([4]), 5]",
                                        "[[1, 2], [3]; f([4]), 5]") &&
                        d_tests_renders("[piecewise(when(x < 0, -x), "
                                        "otherwise(x)), a < b <= c]",
                                        "[piecewise(when(x < 0, -x), "
                                        "otherwise(x)), a < b and b <= c]"),
                        "a literal written as parse.hpp reads it, its "
                        "elements in turn, one operand with no brackets "
                        "of its own");

        const m::value exact = d_tests_made(
                                   m::expr_matrix(
                                       2,
                                       2,
                                       { 1,
                                         m::value::ratio(1, 2),
                                         -3,
                                         m::value::ratio(-1, 4) }));
        const m::value dense = d_tests_made(
                                   m::expr_vector({ m::value(0.1),
                                                    m::value(-2.5),
                                                    m::value(3.0) }));
        const std::string exact_text = m::expr_render(
                                           m::expr_number(exact));
        const std::string dense_text = m::expr_render(
                                           m::expr_number(dense));

        D_TESTS_EXPECT( (exact_text == "[1, (1/2); (-3), (-1/4)]") &&
                        (dense_text == "[0.1, (-2.5), 3]") &&
                        (d_tests_holds_value(
                            m::expr_evaluate(d_tests_parsed(
                                                 exact_text.c_str()),
                                             m::expr_bindings(),
                                             m::expr_context()),
                            exact)) &&
                        (d_tests_holds_value(
                            m::expr_evaluate(d_tests_parsed("[0.1, -2.5, "
                                                            "3]"),
                                             m::expr_bindings(),
                                             m::expr_context()),
                            d_tests_made(
                                m::expr_vector({ m::value::ratio(1, 10),
                                                 m::value::ratio(-5, 2),
                                                 3 })))),
                        "a matrix a fold made: a literal of its elements, "
                        "each as a number with no literal; exact, it reads "
                        "back to itself");
        D_TESTS_EXPECT( (m::expr_render(m::expr_application(
                             m::expr_op_kind::matrix,
                             { d_tests_literal("1"),
                               m::expr_application(
                                   m::expr_op_kind::row,
                                   d_tests_literal("2"),
                                   d_tests_literal("3")) })) ==
                             "[1; 2, 3]") &&
                        (m::expr_render(m::expr_application(
                             m::expr_op_kind::row,
                             d_tests_literal("1"),
                             d_tests_literal("2"))) == "[1, 2]"),
                        "a matrix of a term that is no row, and a row of no "
                        "matrix, as only a term made by hand has: written "
                        "as one element, and as a vector");

        return;
    }

    // d_tests_fold_literals
    //   literals folded: as a value, the matrix of their elements' values,
    // each element kept as it is, an element's error passing, the first;
    // in a type no value; and the language around them, the operators, the
    // rewrites and the handle, as step 19 leaves them.
    void
    d_tests_fold_literals()
    {
        m::expr_bindings at_two;

        static_cast<void>(at_two.at("x", 2));

        const m::expr_context context;
        const m::value        a = d_tests_two_by_two();

        D_TESTS_EXPECT( (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[1, 2; 3, "
                                                                "4]"),
                                                 m::expr_bindings(),
                                                 context),
                                             a)) &&
                        (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[x, 2 * x, "
                                                                "x ^ 2]"),
                                                 at_two,
                                                 context),
                                             d_tests_made(
                                                 m::expr_vector({ 2,
                                                                  4,
                                                                  4 })))) &&
                        (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[1; 2]"),
                                                 m::expr_bindings(),
                                                 context),
                                             d_tests_made(
                                                 m::expr_matrix(2,
                                                                1,
                                                                { 1, 2 })))),
                        "as a value: the matrix of its elements' values, a "
                        "vector one row");

        const m::expr_result<m::value> root = m::expr_evaluate(
                                                  d_tests_parsed("[sqrt(2), "
                                                                 "sqrt(3)]"),
                                                  m::expr_bindings(),
                                                  context);

        D_TESTS_EXPECT( (d_tests_stored_as(
                             d_tests_made(root),
                             m::internal::expr_matrix_storage::
                                 long_doubles)) &&
                        (d_tests_stored_as(
                             d_tests_made(m::expr_evaluate(
                                 d_tests_parsed("[sqrt(2), 0.5]"),
                                 m::expr_bindings(),
                                 context)),
                             m::internal::expr_matrix_storage::values)) &&
                        (d_tests_stored_as(
                             d_tests_made(m::expr_evaluate(
                                 d_tests_parsed("[0.5, 1]"),
                                 m::expr_bindings(),
                                 context)),
                             m::internal::expr_matrix_storage::values)),
                        "dense where each element is inexact, in the widest "
                        "one's type; values where any is exact");
        D_TESTS_EXPECT( (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[1, 1 / 0; "
                                                                "true, "
                                                                "2]"),
                                                 m::expr_bindings(),
                                                 context),
                                             m::value(
                                                 D_MATH_VALUE_ERROR_POLE))) &&
                        (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[1 < 2, "
                                                                "1]"),
                                                 m::expr_bindings(),
                                                 context),
                                             m::value(
                                                 D_MATH_VALUE_ERROR_DOMAIN))) &&
                        (d_tests_holds_value(m::expr_evaluate(
                                                 d_tests_parsed("[[1; 2], "
                                                                "2]"),
                                                 m::expr_bindings(),
                                                 context),
                                             m::value(
                                                 D_MATH_VALUE_ERROR_DOMAIN))) &&
                        d_tests_is_error(m::expr_evaluate(
                                             d_tests_parsed("[1, y]"),
                                             at_two,
                                             context),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable y is bound to no "
                                         "value"),
                        "an element's error passes, the first; one that is "
                        "no number and no matrix, and blocks that do not "
                        "fit, a domain error; a fold's error before");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_parsed("[x, 1]"),
                                             at_two),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_evaluate<Type>: a vector or a "
                                         "matrix has no value in a type") &&
                        d_tests_is_error(m::expr_evaluate<float>(
                                             d_tests_parsed("[y, 1]"),
                                             at_two),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable y is bound to no "
                                         "value") &&
                        d_tests_is_error(m::expr_evaluate<float>(
                                             d_tests_parsed("[y; 1]"),
                                             at_two),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable y is bound to no "
                                         "value") &&
                        d_tests_is_error(m::expr_evaluate<float>(
                                             d_tests_parsed("[1; y]"),
                                             at_two),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_evaluate<Type>: a vector or a "
                                         "matrix has no value in a type"),
                        "in a type: no value, an operand's error first, as "
                        "at any node: a vector's element's, a matrix's first "
                        "row's, each row a literal");

        m::expr_bindings to_matrix;

        static_cast<void>(to_matrix.at("x", a));

        const m::expr_result<double> bound =
            m::expr_evaluate<double>(m::expr_variable("x"),
                                     to_matrix);

        D_TESTS_EXPECT( (bound.is_ok()) &&
                        (bound.value() != bound.value()) &&
                        (d_tests_holds_value(m::expr_evaluate(
                                                 m::expr_variable("x"),
                                                 to_matrix,
                                                 context),
                                             a)),
                        "a variable bound to a matrix: a NaN in a type, "
                        "the matrix as a value");

        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);
        bool           arithmetic = true;
        const char*    texts[]    = { "[1, 2] + 1", "1 / [1]", "[1] mod 2",
                                      "[3]!", "[1] < [2]", "[1] = 1",
                                      "not [1]", "abs([1])", "sin([1])",
                                      "atan2([1], 1)", "2 ^ [1]" };

        for (std::size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); ++i)
        {
            arithmetic = ( (arithmetic) &&
                           (d_tests_holds_value(m::expr_evaluate(
                                                    d_tests_parsed(texts[i]),
                                                    m::expr_bindings(),
                                                    context),
                                                domain)) );
        }

        D_TESTS_EXPECT( arithmetic,
                        "what the kind does not take, a matrix plus or "
                        "under a number, mod, the factorial, an order, "
                        "logic, the functions and a number to a matrix's "
                        "power: a domain error");
        const m::value empty = d_tests_made(m::expr_matrix(0,
                                                           0,
                                                           {}));

        D_TESTS_EXPECT( d_tests_holds_value(m::expr_evaluate(
                                                m::expr_application(
                                                    m::expr_op_kind::matrix,
                                                    std::vector<
                                                        m::expr_term>()),
                                                m::expr_bindings(),
                                                context),
                                            empty) &&
                        d_tests_holds_value(m::expr_evaluate(
                                                d_tests_vector_of({}),
                                                m::expr_bindings(),
                                                context),
                                            empty) &&
                        d_tests_holds_value(m::expr_evaluate(
                                                m::expr_application(
                                                    m::expr_op_kind::matrix,
                                                    d_tests_literal("1")),
                                                m::expr_bindings(),
                                                context),
                                            d_tests_made(
                                                m::expr_matrix(1,
                                                               1,
                                                               { 1 }))) &&
                        d_tests_holds_value(m::expr_evaluate(
                                                m::expr_application(
                                                    m::expr_op_kind::matrix,
                                                    d_tests_matrix_of(
                                                        { { d_tests_literal(
                                                                "1") },
                                                          { d_tests_literal(
                                                                "2") } })),
                                                m::expr_bindings(),
                                                context),
                                            d_tests_made(
                                                m::expr_matrix(2,
                                                               1,
                                                               { 1, 2 }))) &&
                        d_tests_holds_value(m::expr_evaluate(
                                                d_tests_matrix_of(
                                                    { { d_tests_literal("1"),
                                                        d_tests_literal(
                                                            "2") },
                                                      { d_tests_literal(
                                                          "3") } }),
                                                m::expr_bindings(),
                                                context),
                                            m::value(
                                                D_MATH_VALUE_ERROR_DOMAIN)) &&
                        d_tests_holds_value(m::expr_evaluate(
                                                d_tests_matrix_of(
                                                    { { d_tests_literal(
                                                            "1") },
                                                      { d_tests_apply(
                                                            m::expr_op_kind::
                                                                div,
                                                            d_tests_literal(
                                                                "1"),
                                                            d_tests_literal(
                                                                "0")) } }),
                                                m::expr_bindings(),
                                                context),
                                            m::value(
                                                D_MATH_VALUE_ERROR_POLE)),
                        "a term made by hand: a matrix of no row, or a "
                        "vector of no element, the empty matrix; a matrix "
                        "of a term that is no row, a block; rows unlike, a "
                        "domain error, as blocks that do not fit are; a "
                        "row's error passes");
        D_TESTS_EXPECT( (m::expr_render(m::expr_simplify(
                             d_tests_parsed("[x + x, 1 + 1; 2 * x * 3, "
                                            "x ^ 1]"))) ==
                             "[2 * x, 2; 6 * x, x]") &&
                        (m::expr_render(m::expr_simplify(
                             d_tests_parsed("[1, 2] + 0 * [3]"))) ==
                             "[1, 2] + 0 * [3]") &&
                        (m::expr_render(m::expr_substitute(
                             d_tests_parsed("[x, 2 * x]"),
                             "x",
                             d_tests_parsed("y + 1"))) ==
                             "[y + 1, 2 * (y + 1)]"),
                        "simplification inside a literal, never of it, and "
                        "no part dropped beside one; substitution in it");
        D_TESTS_EXPECT( (m::parse("[1,2;3,4]").str() == "[1, 2; 3, 4]") &&
                        (d_tests_holds_value(m::parse("[1, 2; 3, 4]").eval(),
                                             a)) &&
                        (d_tests_is_error(m::parse("[1]").eval<double>(),
                                          m::expr_status::bad_shape,
                                          m::expr_no_offset,
                                          "expr_evaluate<Type>: a vector or "
                                          "a matrix has no value in a type")),
                        "the handle: a literal's text and its value; no "
                        "value in a type");

        return;
    }

    // d_tests_literals_random
    //   1,000 random literals of one to three rows of one to three elements,
    // each element a random term of 2d's language (d_tests_language), drawn
    // by a generator of their own: each rendered and read back as itself,
    // implied multiplication off and on; folded as a value, the matrix of
    // its elements' values or their first error; in a type, its first row's
    // first error or bad_shape; simplified, the literal of its elements
    // simplified; and x replaced in it, the literal of its elements with x
    // replaced.
    void
    d_tests_literals_random()
    {
        const d_tests_alphabet alphabet = d_tests_language();
        const m::expr_term     put      = d_tests_apply(
                                              m::expr_op_kind::add,
                                              m::expr_variable("a"),
                                              d_tests_literal("1"));
        d_tests_rng            rng(20261025u);
        m::expr_syntax         implied;
        m::expr_bindings       bindings;
        const m::expr_context  context;
        std::size_t            back         = 0;
        std::size_t            back_implied = 0;
        std::size_t            valued       = 0;
        std::size_t            typed        = 0;
        std::size_t            simplified   = 0;
        std::size_t            substituted  = 0;
        const std::size_t      count        = 1000;

        implied.implied_multiplication = true;
        static_cast<void>(bindings.at("a", 1)
                                  .at("b", 2)
                                  .at("x", 3)
                                  .at("y_1", m::value::ratio(1, 2))
                                  .at("mod2", 5)
                                  .at("when", 0));

        for (std::size_t n = 0; n < count; ++n)
        {
            const unsigned                        rows    = 1 +
                                                            rng.below(3u);
            const unsigned                        columns = 1 +
                                                            rng.below(3u);
            std::vector<std::vector<m::expr_term> > drawn(rows);
            std::vector<std::vector<m::expr_term> > simple(rows);
            std::vector<std::vector<m::expr_term> > replaced(rows);
            std::vector<m::value>                 values;
            bool                                  failed  = false;
            m::expr_error                         first;
            bool                                  failed_typed = false;
            m::expr_error                         first_typed;
            bool                                  decided = false;
            m::expr_error                         expected;

            // its elements, row by row, each folded both ways in turn; the
            // literal's answer the first row's that fails, by its first
            // error, a complex element among the others' values since step
            // 23's third part (S348)
            for (unsigned r = 0; r < rows; ++r)
            {
                bool          row_failed  = false;
                m::expr_error row_first;

                for (unsigned c = 0; c < columns; ++c)
                {
                    const m::expr_term element =
                        d_tests_random_term(rng,
                                            alphabet,
                                            2u);
                    const m::expr_result<m::value> folded =
                        m::expr_evaluate(element,
                                         bindings,
                                         context);
                    const m::expr_result<double>   in_double =
                        m::expr_evaluate<double>(element,
                                                 bindings);

                    drawn[r].push_back(element);
                    simple[r].push_back(m::expr_simplify(element));
                    replaced[r].push_back(m::expr_substitute(element,
                                                             "x",
                                                             put));

                    if ( (folded.is_err()) &&
                         (!failed) )
                    {
                        failed = true;
                        first  = folded.error();
                    }

                    if ( (folded.is_err()) &&
                         (!row_failed) )
                    {
                        row_failed = true;
                        row_first  = folded.error();
                    }

                    if ( (in_double.is_err()) &&
                         (!failed_typed) &&
                         (r == 0) )
                    {
                        failed_typed = true;
                        first_typed  = in_double.error();
                    }

                    if (folded.is_ok())
                    {
                        values.push_back(folded.value());
                    }
                }

                // the first row that fails
                if ( (!decided) &&
                     (row_failed) )
                {
                    decided  = true;
                    expected = row_first;
                }
            }

            const bool         one_row = (rows == 1);
            const m::expr_term literal =
                ( one_row ? d_tests_vector_of(drawn[0])
                          : d_tests_matrix_of(drawn) );
            const std::string  text    = m::expr_render(literal);

            back += ( d_tests_parses(text.c_str(),
                                     literal) ? 1 : 0 );
            back_implied += ( d_tests_parses(text.c_str(),
                                             literal,
                                             implied) ? 1 : 0 );

            const m::expr_result<m::value> value =
                m::expr_evaluate(literal,
                                 bindings,
                                 context);

            if (decided)
            {
                valued += ( ( (value.is_err()) &&
                              (value.error() == expected) ) ? 1 : 0 );
            }
            else
            {
                const m::expr_result<m::value> want =
                    m::expr_matrix(rows,
                                   columns,
                                   values);

                valued += ( d_tests_holds_value(value,
                                                d_tests_made(want)) ? 1 : 0 );
            }

            const m::expr_result<double> in_type =
                m::expr_evaluate<double>(literal,
                                         bindings);

            typed += ( ( (in_type.is_err()) &&
                         ( (failed_typed)
                               ? (in_type.error() == first_typed)
                               : (in_type.error().status ==
                                      m::expr_status::bad_shape) ) )
                           ? 1 : 0 );
            simplified += ( d_tests_same(m::expr_simplify(literal),
                                         ( one_row
                                               ? d_tests_vector_of(simple[0])
                                               : d_tests_matrix_of(
                                                     simple) )) ? 1 : 0 );
            substituted += ( d_tests_same(m::expr_substitute(literal,
                                                             "x",
                                                             put),
                                          ( one_row
                                                ? d_tests_vector_of(
                                                      replaced[0])
                                                : d_tests_matrix_of(
                                                      replaced) )) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (back == count),
                        "1,000 random literals rendered and read back as "
                        "themselves");
        D_TESTS_EXPECT( (back_implied == count),
                        "and so with implied multiplication on");
        D_TESTS_EXPECT( (valued == count),
                        "as a value, the matrix of their elements' values, "
                        "complex ones among them (S348), or the first "
                        "error");
        D_TESTS_EXPECT( (typed == count),
                        "in a type, their first row's first error, or "
                        "bad_shape");
        D_TESTS_EXPECT( (simplified == count),
                        "simplified, the literal of their elements "
                        "simplified");
        D_TESTS_EXPECT( (substituted == count),
                        "x replaced, the literal of their elements with x "
                        "replaced");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_MATRICES_HPP
