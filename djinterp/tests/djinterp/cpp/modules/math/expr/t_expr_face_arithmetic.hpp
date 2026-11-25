/*******************************************************************************
* djinterp [cpp][math]                                t_expr_face_arithmetic.hpp
*
* One part of t_expr_face.cpp, the expression language's face test (S438), its
* section 11's second third: arithmetic, powers, indexing, simplification and
* random terms of matrices. The test includes the parts in order, one program;
* each part includes the one before it.
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_face_arithmetic.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_ARITHMETIC_HPP
#define DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_ARITHMETIC_HPP 1

#include "./t_expr_face_matrices.hpp"   // the part before


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


namespace
{

    // d_tests_is_matrix
    //   whether a value is the matrix of _rows by _columns of the elements
    // given, row by row, each equal to the value's, held in the storage
    // given.
    bool
    d_tests_is_matrix(
        const m::value&                  _x,
        std::size_t                      _rows,
        std::size_t                      _columns,
        const std::vector<m::value>&     _elements,
        m::internal::expr_matrix_storage _storage
    )
    {
        const m::value want = d_tests_made(m::expr_matrix(_rows,
                                                          _columns,
                                                          _elements));

        return ( (d_tests_same_values(_x,
                                      want)) &&
                 (d_tests_stored_as(_x,
                                    _storage)) );
    }

    // d_tests_all_exact
    //   whether every element of a matrix is exact; false where the value
    // is no matrix.
    bool
    d_tests_all_exact(
        const m::value& _x
    )
    {
        const m::expr_shape shape = d_tests_shape(_x);
        bool                exact = (shape.rows > 0);

        for (std::size_t r = 0; r < shape.rows; ++r)
        {
            for (std::size_t c = 0; c < shape.columns; ++c)
            {
                exact = ( (exact) &&
                          (d_tests_made(m::expr_element(_x,
                                                        r,
                                                        c)).is_exact()) );
            }
        }

        return exact;
    }

    // d_tests_dense_in
    //   whether the kind's arithmetic keeps dense matrices of Type in Type,
    // each answer as Type computes it: A + B, A - B, -A, A * 2, A / 2,
    // A * B, A', A ^ 0 and A ^ 2, of A = [1, 2; 3, 4] and
    // B = [0.5, 1; 1.5, 2], each in Type, held in _storage.
    template<typename Type>
    bool
    d_tests_dense_in(
        m::internal::expr_matrix_storage _storage
    )
    {
        const m::value a = d_tests_made(m::expr_matrix(2,
                                                       2,
                                                       { Type(1), Type(2),
                                                         Type(3),
                                                         Type(4) }));
        const m::value b = d_tests_made(m::expr_matrix(2,
                                                       2,
                                                       { Type(0.5), Type(1),
                                                         Type(1.5),
                                                         Type(2) }));

        return ( d_tests_is_matrix(a + b,
                                   2, 2,
                                   { Type(1.5), Type(3), Type(4.5),
                                     Type(6) },
                                   _storage) &&
                 d_tests_is_matrix(a - b,
                                   2, 2,
                                   { Type(0.5), Type(1), Type(1.5),
                                     Type(2) },
                                   _storage) &&
                 d_tests_is_matrix(-a,
                                   2, 2,
                                   { Type(-1), Type(-2), Type(-3),
                                     Type(-4) },
                                   _storage) &&
                 d_tests_is_matrix(a * m::value(2),
                                   2, 2,
                                   { Type(2), Type(4), Type(6), Type(8) },
                                   _storage) &&
                 d_tests_is_matrix(a / m::value(2),
                                   2, 2,
                                   { Type(0.5), Type(1), Type(1.5),
                                     Type(2) },
                                   _storage) &&
                 d_tests_is_matrix(a * b,
                                   2, 2,
                                   { Type(3.5), Type(5), Type(7.5),
                                     Type(11) },
                                   _storage) &&
                 d_tests_is_matrix(m::internal::expr_matrix_transposed(a),
                                   2, 2,
                                   { Type(1), Type(3), Type(2), Type(4) },
                                   _storage) &&
                 d_tests_is_matrix(m::internal::expr_matrix_power(
                                       a,
                                       m::value(0)),
                                   2, 2,
                                   { Type(1), Type(0), Type(0), Type(1) },
                                   _storage) &&
                 d_tests_is_matrix(m::internal::expr_matrix_power(
                                       a,
                                       m::value(2)),
                                   2, 2,
                                   { Type(7), Type(10), Type(15),
                                     Type(22) },
                                   _storage) );
    }

    // d_tests_matrix_arithmetic
    //   step 20's arithmetic of the kind, through the value's operators
    // (S246, S247): sums and differences of one shape, the negation, a
    // matrix and a number multiplied or a matrix divided by a number, and
    // the matrix product; each result's storage, exact elements kept exact
    // and dense ones in the wider type; errors passing; and what the kind
    // does not take.
    void
    d_tests_matrix_arithmetic()
    {
        typedef m::internal::expr_matrix_storage storage;

        const m::value a      = d_tests_two_by_two();
        const m::value b      = d_tests_made(m::expr_matrix(2,
                                                            2,
                                                            { 5, 6, 7, 8 }));
        const m::value floats = d_tests_made(
                                    m::expr_matrix(2,
                                                   2,
                                                   { 0.5f, 1.0f, 1.5f,
                                                     2.0f }));
        const m::value doubles = d_tests_made(
                                     m::expr_matrix(2,
                                                    2,
                                                    { 0.5, 1.0, 1.5, 2.0 }));
        const m::value longs   = d_tests_made(
                                     m::expr_matrix(2,
                                                    2,
                                                    { 0.5L, 1.0L, 1.5L,
                                                      2.0L }));
        const m::value row     = d_tests_made(m::expr_vector({ 1, 2, 3 }));
        const m::value col     = d_tests_made(m::expr_matrix(3,
                                                             1,
                                                             { 1, 2, 3 }));
        const m::value third   = d_tests_made(
                                     m::expr_vector({ m::value::ratio(1, 3),
                                                      m::value(2) }));
        const m::value wide    = d_tests_made(
                                     m::expr_matrix(2,
                                                    3,
                                                    { 1, 2, 3, 4, 5, 6 }));
        const m::value dense   = d_tests_made(
                                     m::expr_matrix(2,
                                                    3,
                                                    { 1.0, 2.0, 3.0, 4.0,
                                                      5.0, 6.0 }));
        const m::value pole(D_MATH_VALUE_ERROR_POLE);
        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( d_tests_is_matrix(a + b,
                                          2, 2,
                                          { 6, 8, 10, 12 },
                                          storage::values) &&
                        d_tests_is_matrix(a - b,
                                          2, 2,
                                          { -4, -4, -4, -4 },
                                          storage::values) &&
                        d_tests_is_matrix(b - a,
                                          2, 2,
                                          { 4, 4, 4, 4 },
                                          storage::values) &&
                        d_tests_is_matrix(-a,
                                          2, 2,
                                          { -1, -2, -3, -4 },
                                          storage::values) &&
                        d_tests_all_exact(a + b) &&
                        d_tests_all_exact(-a),
                        "A + B and A - B element by element, -A each "
                        "negated: exact matrices exact");
        D_TESTS_EXPECT( d_tests_is_matrix(a + floats,
                                          2, 2,
                                          { 1.5f, 3.0f, 4.5f, 6.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(floats + a,
                                          2, 2,
                                          { 1.5f, 3.0f, 4.5f, 6.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(floats + doubles,
                                          2, 2,
                                          { 1.0, 2.0, 3.0, 4.0 },
                                          storage::doubles) &&
                        d_tests_is_matrix(doubles - longs,
                                          2, 2,
                                          { 0.0L, 0.0L, 0.0L, 0.0L },
                                          storage::long_doubles) &&
                        d_tests_is_matrix(-floats,
                                          2, 2,
                                          { -0.5f, -1.0f, -1.5f, -2.0f },
                                          storage::floats),
                        "a dense sum in the wider of its operands' types, "
                        "an exact matrix taking the dense one's, as the "
                        "tower takes a number");
        D_TESTS_EXPECT( d_tests_is_matrix(a * m::value(2),
                                          2, 2,
                                          { 2, 4, 6, 8 },
                                          storage::values) &&
                        d_tests_is_matrix(m::value(2) * a,
                                          2, 2,
                                          { 2, 4, 6, 8 },
                                          storage::values) &&
                        d_tests_is_matrix(a / m::value(2),
                                          2, 2,
                                          { m::value::ratio(1, 2), 1,
                                            m::value::ratio(3, 2), 2 },
                                          storage::values) &&
                        d_tests_all_exact(a / m::value(2)) &&
                        d_tests_is_matrix(third * m::value(3),
                                          1, 2,
                                          { 1, 6 },
                                          storage::values) &&
                        d_tests_is_matrix(a * m::value(0.5f),
                                          2, 2,
                                          { 0.5f, 1.0f, 1.5f, 2.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(floats * m::value(0.5),
                                          2, 2,
                                          { 0.25, 0.5, 0.75, 1.0 },
                                          storage::doubles) &&
                        d_tests_is_matrix(floats * m::value(2.0f),
                                          2, 2,
                                          { 1.0f, 2.0f, 3.0f, 4.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(doubles / m::value(0.5f),
                                          2, 2,
                                          { 1.0, 2.0, 3.0, 4.0 },
                                          storage::doubles) &&
                        d_tests_is_matrix(longs * m::value(0.5f),
                                          2, 2,
                                          { 0.25L, 0.5L, 0.75L, 1.0L },
                                          storage::long_doubles) &&
                        d_tests_is_matrix(doubles * m::value(2),
                                          2, 2,
                                          { 1.0, 2.0, 3.0, 4.0 },
                                          storage::doubles),
                        "A * s, s * A and A / s each element by the number, "
                        "exactly for exact operands, otherwise in the "
                        "wider type, an exact number taking the matrix's");
        const long double quarter = 0.25L;

        D_TESTS_EXPECT( d_tests_dense_in<float>(storage::floats) &&
                        d_tests_dense_in<double>(storage::doubles) &&
                        d_tests_dense_in<long double>(
                            storage::long_doubles) &&
                        d_tests_is_matrix(floats + longs,
                                          2, 2,
                                          { 1.0L, 2.0L, 3.0L, 4.0L },
                                          storage::long_doubles) &&
                        d_tests_same_values(floats *
                                                d_tests_real_of(&quarter),
                                            domain),
                        "each dense type's arithmetic in that type: sums, "
                        "differences, negation, scalar products and "
                        "quotients, products, transposes and powers; a "
                        "float matrix widened to a long double one's type; "
                        "a boxed inexact number on the values' path, by "
                        "its own arithmetic, which the test's kind gives "
                        "as none");
        D_TESTS_EXPECT( d_tests_same_values(a / m::value(0),
                                            pole) &&
                        d_tests_is_matrix(doubles / m::value(0),
                                          2, 2,
                                          { std::numeric_limits<
                                                double>::infinity(),
                                            std::numeric_limits<
                                                double>::infinity(),
                                            std::numeric_limits<
                                                double>::infinity(),
                                            std::numeric_limits<
                                                double>::infinity() },
                                          storage::doubles),
                        "an exact matrix divided by 0 a pole, as each "
                        "element's quotient is; a dense one's elements "
                        "infinite, as in its type");
        D_TESTS_EXPECT( d_tests_is_matrix(a * b,
                                          2, 2,
                                          { 19, 22, 43, 50 },
                                          storage::values) &&
                        d_tests_is_matrix(b * a,
                                          2, 2,
                                          { 23, 34, 31, 46 },
                                          storage::values) &&
                        d_tests_is_matrix(row * col,
                                          1, 1,
                                          { 14 },
                                          storage::values) &&
                        d_tests_is_matrix(col * row,
                                          3, 3,
                                          { 1, 2, 3, 2, 4, 6, 3, 6, 9 },
                                          storage::values) &&
                        d_tests_is_matrix(third * d_tests_made(
                                                      m::expr_matrix(
                                                          2,
                                                          1,
                                                          { 3, 3 })),
                                          1, 1,
                                          { 7 },
                                          storage::values) &&
                        d_tests_is_matrix(a * floats,
                                          2, 2,
                                          { 3.5f, 5.0f, 7.5f, 11.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(floats * doubles,
                                          2, 2,
                                          { 1.75, 2.5, 3.75, 5.5 },
                                          storage::doubles) &&
                        d_tests_is_matrix(dense * d_tests_made(
                                                      m::expr_matrix(
                                                          3,
                                                          1,
                                                          { 1.0, 0.5,
                                                            0.25 })),
                                          2, 1,
                                          { 2.75, 8.0 },
                                          storage::doubles) &&
                        d_tests_is_matrix(wide * col,
                                          2, 1,
                                          { 14, 32 },
                                          storage::values),
                        "A * B the matrix product, A's columns B's rows: "
                        "AB and BA two matrices; a row by a column 1 by 1, "
                        "still a matrix; a column by a row their outer "
                        "product; a 2 by 3 by a 3 by 1 a 2 by 1, dense or "
                        "of values");

        bool refused = true;

        // what the kind does not take
        const m::value refusals[] = { abs(a), a + m::value(1),
                                      m::value(1) + a, a - m::value(1),
                                      m::value(1) - a, m::value(1) / a,
                                      a / b, a + row, a - col, row * row,
                                      a * col, col * a, a * m::value(true),
                                      m::value(true) * a, a * m::value(),
                                      a + m::value(true) };

        for (std::size_t i = 0;
             i < (sizeof(refusals) / sizeof(refusals[0]));
             ++i)
        {
            refused = ( (refused) &&
                        (d_tests_same_values(refusals[i],
                                             domain)) );
        }

        D_TESTS_EXPECT( refused,
                        "|A|, A + s, s + A, A - s, s - A, s / A and A / B, "
                        "shapes that do not fit, a boolean or none: each a "
                        "domain error (S246)");
        D_TESTS_EXPECT( d_tests_same_values(pole * a,
                                            pole) &&
                        d_tests_same_values(a * pole,
                                            pole) &&
                        d_tests_same_values(a + pole,
                                            pole) &&
                        d_tests_same_values(-m::value(pole),
                                            pole),
                        "an error passes, before the kind is asked");

        return;
    }

    // d_tests_matrix_powers
    //   the transpose; A ^ n by squaring, of a square matrix and a whole
    // n >= 0, exact or floating-point (S248); indexing, from 1 (S249); and
    // two matrices equal or not as a relation's value.
    void
    d_tests_matrix_powers()
    {
        typedef m::internal::expr_matrix_storage storage;

        const m::value a       = d_tests_two_by_two();
        const m::value fib     = d_tests_made(m::expr_matrix(2,
                                                             2,
                                                             { 1, 1, 1, 0 }));
        const m::value floats  = d_tests_made(
                                     m::expr_matrix(2,
                                                    2,
                                                    { 0.5f, 1.0f, 1.5f,
                                                      2.0f }));
        const m::value doubles = d_tests_made(
                                     m::expr_matrix(2,
                                                    2,
                                                    { 0.5, 1.0, 1.5, 2.0 }));
        const m::value row     = d_tests_made(m::expr_vector({ 1, 2, 3 }));
        const m::value col     = d_tests_made(m::expr_matrix(3,
                                                             1,
                                                             { 1, 2, 3 }));
        const m::value wide    = d_tests_made(
                                     m::expr_matrix(2,
                                                    3,
                                                    { 1, 2, 3, 4, 5, 6 }));
        const m::value pole(D_MATH_VALUE_ERROR_POLE);
        const m::value range(D_MATH_VALUE_ERROR_RANGE);
        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( d_tests_is_matrix(m::internal::expr_matrix_transposed(
                                              wide),
                                          3, 2,
                                          { 1, 4, 2, 5, 3, 6 },
                                          storage::values) &&
                        d_tests_is_matrix(m::internal::expr_matrix_transposed(
                                              floats),
                                          2, 2,
                                          { 0.5f, 1.5f, 1.0f, 2.0f },
                                          storage::floats) &&
                        (m::internal::expr_matrix_transposed(row) == col) &&
                        (m::internal::expr_matrix_transposed(
                             m::internal::expr_matrix_transposed(wide)) ==
                             wide) &&
                        d_tests_same_values(
                            m::internal::expr_transpose_value(m::value(2)),
                            m::value(2)) &&
                        d_tests_same_values(
                            m::internal::expr_transpose_value(pole),
                            pole) &&
                        d_tests_same_values(
                            m::internal::expr_transpose_value(
                                m::value(true)),
                            domain),
                        "the transpose: columns as rows, in the matrix's "
                        "storage, twice itself; a number's the number, an "
                        "error's the error, a boolean's a domain error");

        const m::value power5 = m::internal::expr_matrix_power(a,
                                                               m::value(5));
        const m::value fib90  = m::internal::expr_matrix_power(fib,
                                                               m::value(90));

        D_TESTS_EXPECT( d_tests_is_matrix(m::internal::expr_matrix_power(
                                              a,
                                              m::value(0)),
                                          2, 2,
                                          { 1, 0, 0, 1 },
                                          storage::values) &&
                        (m::internal::expr_matrix_power(a,
                                                        m::value(1)) == a) &&
                        d_tests_is_matrix(m::internal::expr_matrix_power(
                                              a,
                                              m::value(2)),
                                          2, 2,
                                          { 7, 10, 15, 22 },
                                          storage::values) &&
                        d_tests_is_matrix(power5,
                                          2, 2,
                                          { 1069, 1558, 2337, 3406 },
                                          storage::values) &&
                        (power5 == ((a * a) * (a * a)) * a) &&
                        d_tests_all_exact(fib90) &&
                        d_tests_same_values(
                            d_tests_made(m::expr_element(fib90, 0, 1)),
                            m::value(static_cast<dj_math_imax>(
                                2880067194370816120LL))) &&
                        d_tests_same_values(
                            d_tests_made(m::expr_element(fib90, 0, 0)),
                            m::value(static_cast<dj_math_imax>(
                                4660046610375530309LL))),
                        "A ^ n by squaring: A ^ 0 the identity, A ^ 1 A, "
                        "A ^ 5 as A's products give it, exact: the 90th "
                        "power of [1, 1; 1, 0] F(91) and F(90)");
        D_TESTS_EXPECT( d_tests_is_matrix(m::internal::expr_matrix_power(
                                              floats,
                                              m::value(0)),
                                          2, 2,
                                          { 1.0f, 0.0f, 0.0f, 1.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(m::internal::expr_matrix_power(
                                              doubles,
                                              m::value(3)),
                                          2, 2,
                                          { 4.625, 6.75, 10.125, 14.75 },
                                          storage::doubles) &&
                        d_tests_is_matrix(m::internal::expr_matrix_power(
                                              a,
                                              m::value(2.0)),
                                          2, 2,
                                          { 7.0, 10.0, 15.0, 22.0 },
                                          storage::doubles) &&
                        d_tests_is_matrix(m::internal::expr_matrix_power(
                                              a,
                                              m::value(0.0f)),
                                          2, 2,
                                          { 1.0f, 0.0f, 0.0f, 1.0f },
                                          storage::floats) &&
                        d_tests_is_matrix(m::internal::expr_matrix_power(
                                              floats,
                                              m::value(1.0L)),
                                          2, 2,
                                          { 0.5L, 1.0L, 1.5L, 2.0L },
                                          storage::long_doubles),
                        "a dense matrix's power in its type, its identity "
                        "too; to a floating-point whole n, the matrix "
                        "first in the wider of its type and n's");

        bool refused = true;

        // what a power does not take
        const m::value refusals[] = {
            m::internal::expr_matrix_power(wide, m::value(2)),
            m::internal::expr_matrix_power(row, m::value(0)),
            m::internal::expr_matrix_power(row, m::value(-1)),
            m::internal::expr_matrix_power(a, m::value::ratio(1, 2)),
            m::internal::expr_matrix_power(a, m::value(2.5)),
            m::internal::expr_matrix_power(a, m::value(-1.5)),
            m::internal::expr_matrix_power(
                a,
                m::value(std::numeric_limits<double>::infinity())),
            m::internal::expr_matrix_power(
                a,
                m::value(std::numeric_limits<double>::quiet_NaN())),
            m::internal::expr_matrix_power(a, m::value(1e19)),
            m::internal::expr_matrix_power(a, m::value(true)),
            m::internal::expr_matrix_power(a, m::value()),
            m::internal::expr_matrix_power(a, a),
            m::internal::expr_matrix_power(m::value(2), a),
            m::internal::expr_matrix_power(wide, m::value(2.0)) };

        for (std::size_t i = 0;
             i < (sizeof(refusals) / sizeof(refusals[0]));
             ++i)
        {
            refused = ( (refused) &&
                        (d_tests_same_values(refusals[i],
                                             domain)) );
        }

        D_TESTS_EXPECT( refused,
                        "a power of a matrix not square, or to a "
                        "fractional, infinite, NaN or too great n, a "
                        "boolean, none or a matrix, and a number to a "
                        "matrix: a domain error");

        const long double two_real = 2.0L;
        const m::value    boxed    = d_tests_made(
                                         m::expr_matrix(1,
                                                        1,
                                                        { d_tests_real_of(
                                                              &two_real) }));

        D_TESTS_EXPECT( d_tests_same_values(
                            m::internal::expr_matrix_power(
                                a,
                                d_tests_real_of(&two_real)),
                            domain) &&
                        (d_tests_shape(m::internal::expr_matrix_power(
                                           boxed,
                                           m::value(1))) ==
                             m::expr_shape(1, 1)) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_power(boxed,
                                                           m::value(2)),
                            domain),
                        "a power by the arithmetic it asks of its operands: "
                        "to a boxed inexact whole number, or of a matrix "
                        "of one, the test's kind's arithmetic giving none, "
                        "a domain error; where no product is asked, A, a 1 "
                        "by 1 matrix");
        const m::value fib100 = m::internal::expr_matrix_power(
                                    fib,
                                    m::value(100));
        const m::value half   = d_tests_made(
                                    m::expr_matrix(2,
                                                   2,
                                                   { m::value::ratio(1, 2),
                                                     0, 0, 0 }));
        const m::value flagged = fib100 + half;

        D_TESTS_EXPECT( d_tests_stored_as(fib100,
                                          storage::long_doubles) &&
                        (fib100.flags() == D_MATH_VALUE_FLAG_INEXACT) &&
                        (d_tests_made(m::expr_element(fib100, 0, 0)).flags() ==
                             D_MATH_VALUE_FLAG_INEXACT) &&
                        (m::internal::expr_matrix_index(
                             { fib100, m::value(2), m::value(2) }).flags() ==
                             D_MATH_VALUE_FLAG_INEXACT) &&
                        d_tests_stored_as(flagged,
                                          storage::long_doubles) &&
                        (flagged.flags() == D_MATH_VALUE_FLAG_INEXACT) &&
                        (d_tests_made(m::expr_element(flagged,
                                                      1,
                                                      1)).flags() ==
                             D_MATH_VALUE_FLAG_INEXACT),
                        "a power past the exact integers' range long "
                        "doubles, flagged inexact (G2); each of its "
                        "elements, read or indexed, flagged so, and each "
                        "computed from one beside an exact matrix's, as "
                        "values, then dense again");
        D_TESTS_EXPECT( d_tests_same_values(
                            m::internal::expr_value_pow(a,
                                                        pole),
                            pole) &&
                        d_tests_same_values(
                            m::internal::expr_value_pow(pole,
                                                        a),
                            pole) &&
                        d_tests_same_values(
                            m::internal::expr_value_pow(a,
                                                        m::value(3)),
                            ((a * a) * a)),
                        "^ of a matrix the kind's power, an error passing "
                        "first");

        const m::value two(2);
        const m::value one(1);

        D_TESTS_EXPECT( d_tests_same_values(
                            m::internal::expr_matrix_index({ a, two, one }),
                            m::value(3)) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ row,
                                                             m::value(3) }),
                            m::value(3)) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ col, two }),
                            two) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ col,
                                                             m::value(2.0) }),
                            two) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ doubles,
                                                             two, two }),
                            m::value(2.0)) &&
                        d_tests_is_matrix(m::internal::expr_matrix_index(
                                              { wide, two }),
                                          1, 3,
                                          { 4, 5, 6 },
                                          storage::values) &&
                        d_tests_is_matrix(m::internal::expr_matrix_index(
                                              { floats, one }),
                                          1, 2,
                                          { 0.5f, 1.0f },
                                          storage::floats),
                        "A[i, j] an element, from 1; v[i] a vector's, a "
                        "row's or a column's; A[i] of any other matrix its "
                        "row i; an index whole in any kind");

        refused = true;

        // what indexing does not take
        const m::value bad[] = {
            m::internal::expr_matrix_index({ a, m::value(3) }),
            m::internal::expr_matrix_index({ a, m::value(0), one }),
            m::internal::expr_matrix_index({ a, one, m::value(3) }),
            m::internal::expr_matrix_index({ a, one, one, one }),
            m::internal::expr_matrix_index({ a }),
            m::internal::expr_matrix_index({ two, one }),
            m::internal::expr_matrix_index({ m::value(true), one }),
            m::internal::expr_matrix_index({ col, m::value(2.5) }),
            m::internal::expr_matrix_index({ col, m::value::ratio(1, 2) }),
            m::internal::expr_matrix_index({ col, m::value(true) }),
            m::internal::expr_matrix_index({ col, m::value() }),
            m::internal::expr_matrix_index({ col, a }),
            m::internal::expr_matrix_index(
                { col,
                  m::value(std::numeric_limits<double>::quiet_NaN()) }),
            m::internal::expr_matrix_index(
                { col,
                  m::value(std::numeric_limits<double>::infinity()) }),
            m::internal::expr_matrix_index({ col, m::value(-1) }) };

        for (std::size_t i = 0; i < (sizeof(bad) / sizeof(bad[0])); ++i)
        {
            refused = ( (refused) &&
                        (d_tests_same_values(bad[i],
                                             domain)) );
        }

        D_TESTS_EXPECT( refused,
                        "an index past the places, of 0 or below, not "
                        "whole, a boolean, none or a matrix; three indices, "
                        "or none; indexing what is no matrix: a domain "
                        "error");
        D_TESTS_EXPECT( d_tests_same_values(
                            m::internal::expr_matrix_index({ pole, range }),
                            pole) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ a, range }),
                            range) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ two, range }),
                            range) &&
                        d_tests_same_values(
                            m::internal::expr_matrix_index({ a, one,
                                                             pole }),
                            pole),
                        "an error passes, the first: the matrix's, then an "
                        "index's");

        const m::value same = d_tests_made(m::expr_matrix(2,
                                                          2,
                                                          { 1.0, 2.0, 3.0,
                                                            4.0 }));

        D_TESTS_EXPECT( d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::eq,
                                a,
                                same),
                            m::value(true)) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::ne,
                                a,
                                same),
                            m::value(false)) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::eq,
                                row,
                                col),
                            m::value(false)) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::ne,
                                row,
                                col),
                            m::value(true)) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::lt,
                                a,
                                same),
                            domain) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::eq,
                                a,
                                one),
                            domain) &&
                        d_tests_same_values(
                            m::internal::expr_relation_value(
                                m::expr_op_kind::eq,
                                pole,
                                a),
                            pole),
                        "A = B and A != B by the kind's comparison, of one "
                        "shape and equal place by place (S242); an order "
                        "of matrices, or a matrix and a number, a domain "
                        "error; an error passes");

        return;
    }

    // d_tests_index_of
    //   the term of _operand indexed by the indices given.
    m::expr_term
    d_tests_index_of(
        const m::expr_term&              _operand,
        const std::vector<m::expr_term>& _indices
    )
    {
        std::vector<m::expr_term> operands(1, _operand);

        operands.insert(operands.end(),
                        _indices.begin(),
                        _indices.end());

        return m::expr_application(m::expr_op_kind::index,
                                   operands);
    }

    // d_tests_parse_indexing
    //   step 20's indexing and transpose in the parser layer (S252): v[i]
    // and A[i, j], each index any term, tighter than any operator; the
    // transpose postfix, as tight as the factorial; both with implied
    // multiplication and bars; and every error at its place.
    void
    d_tests_parse_indexing()
    {
        const m::expr_term one = d_tests_literal("1");
        const m::expr_term two = d_tests_literal("2");
        const m::expr_term a   = m::expr_variable("A");
        const m::expr_term b   = m::expr_variable("B");
        const m::expr_term v   = m::expr_variable("v");
        const m::expr_term x   = m::expr_variable("x");
        const m::expr_term v1  = d_tests_index_of(v,
                                                  { one });
        const m::expr_term at  = d_tests_apply(m::expr_op_kind::transpose,
                                               a);
        m::expr_syntax     implied;
        m::expr_syntax     bars;

        implied.implied_multiplication = true;
        bars.absolute_bars             = true;

        D_TESTS_EXPECT( d_tests_parses("v[1]",
                                       v1) &&
                        d_tests_parses("v [1]",
                                       v1) &&
                        d_tests_parses("A[1, 2]",
                                       d_tests_index_of(a,
                                                        { one, two })) &&
                        d_tests_parses("A[x + 1, 2]",
                                       d_tests_index_of(
                                           a,
                                           { d_tests_apply(
                                                 m::expr_op_kind::add,
                                                 x,
                                                 one),
                                             two })) &&
                        d_tests_parses("v[v[1]]",
                                       d_tests_index_of(v,
                                                        { v1 })) &&
                        d_tests_parses("A[1][2]",
                                       d_tests_index_of(
                                           d_tests_index_of(a,
                                                            { one }),
                                           { two })),
                        "v[i] and A[i, j], each index any term, an index "
                        "among them; A[i][j] the row's element");
        D_TESTS_EXPECT( d_tests_parses("-v[1]",
                                       d_tests_apply(m::expr_op_kind::neg,
                                                     v1)) &&
                        d_tests_parses("2 ^ v[1]",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     two,
                                                     v1)) &&
                        d_tests_parses("v[1] ^ 2",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     v1,
                                                     two)) &&
                        d_tests_parses("v[1]!",
                                       d_tests_apply(
                                           m::expr_op_kind::factorial,
                                           v1)) &&
                        d_tests_parses("x'[1]",
                                       d_tests_index_of(
                                           d_tests_apply(
                                               m::expr_op_kind::transpose,
                                               x),
                                           { one })) &&
                        d_tests_parses("f(x)[1]",
                                       d_tests_index_of(
                                           m::expr_call("f",
                                                        { x }),
                                           { one })) &&
                        d_tests_parses("(x + 1)[2]",
                                       d_tests_index_of(
                                           d_tests_apply(
                                               m::expr_op_kind::add,
                                               x,
                                               one),
                                           { two })) &&
                        d_tests_parses("[1, 2][2]",
                                       d_tests_index_of(
                                           d_tests_vector_of(
                                               { one, two }),
                                           { two })) &&
                        d_tests_parses("1[2]",
                                       d_tests_index_of(one,
                                                        { two })),
                        "indexing tighter than any operator: of a "
                        "variable, a transpose, a call, a group, a literal "
                        "or a number");
        D_TESTS_EXPECT( d_tests_parses("A'",
                                       at) &&
                        d_tests_parses("A''",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           at)) &&
                        d_tests_parses("A' ^ 2",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     at,
                                                     two)) &&
                        d_tests_parses("A ^ 2'",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           d_tests_apply(
                                               m::expr_op_kind::pow,
                                               a,
                                               two))) &&
                        d_tests_parses("-A'",
                                       d_tests_apply(m::expr_op_kind::neg,
                                                     at)) &&
                        d_tests_parses("x!'",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           d_tests_apply(
                                               m::expr_op_kind::factorial,
                                               x))) &&
                        d_tests_parses("[1, 2]'",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           d_tests_vector_of(
                                               { one, two }))) &&
                        d_tests_parses("A' * B",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     at,
                                                     b)) &&
                        d_tests_parses("v[1]'",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           v1)),
                        "A' postfix, between the unary - and ^: A' ^ 2 is "
                        "(A') ^ 2, A ^ 2' is (A ^ 2)', as MATLAB reads "
                        "both");
        D_TESTS_EXPECT( d_tests_parses("2v[1]",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     two,
                                                     v1),
                                       implied) &&
                        d_tests_parses("v[1]x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     v1,
                                                     x),
                                       implied) &&
                        d_tests_parses("A'B",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     at,
                                                     b),
                                       implied) &&
                        d_tests_parses("2A'",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     two,
                                                     at),
                                       implied) &&
                        d_tests_parses("x[1]",
                                       d_tests_index_of(x,
                                                        { one }),
                                       implied) &&
                        d_tests_parses("|v[1]|",
                                       m::expr_call("abs",
                                                    { v1 }),
                                       bars) &&
                        d_tests_parses("|A|'",
                                       d_tests_apply(
                                           m::expr_op_kind::transpose,
                                           m::expr_call("abs",
                                                        { a })),
                                       bars),
                        "with implied multiplication a * after an index "
                        "or a transpose, and none before an index; bars "
                        "around and before them");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("x[]"),
                                         m::expr_status::missing_operand,
                                         2,
                                         "an operand is missing before "
                                         "']'") &&
                        d_tests_is_error(m::expr_parse("x[1, ]"),
                                         m::expr_status::missing_operand,
                                         5,
                                         "an operand is missing before "
                                         "']'") &&
                        d_tests_is_error(m::expr_parse("x[, 1]"),
                                         m::expr_status::missing_operand,
                                         2,
                                         "an operand is missing before "
                                         "','") &&
                        d_tests_is_error(m::expr_parse("x[1, 2, 3]"),
                                         m::expr_status::wrong_arity,
                                         1,
                                         "indexing takes 1 or 2 indices, "
                                         "not 3") &&
                        d_tests_is_error(m::expr_parse("x[1; 2]"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected ';'") &&
                        d_tests_is_error(m::expr_parse("x[1 2]"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '2'") &&
                        d_tests_is_error(m::expr_parse("x[1)"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'[' is never closed") &&
                        d_tests_is_error(m::expr_parse("x]"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "']' closes no bracket") &&
                        d_tests_is_error(m::expr_parse("x[1]]"),
                                         m::expr_status::unbalanced,
                                         4,
                                         "']' closes no bracket") &&
                        d_tests_is_error(m::expr_parse("'x"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '''") &&
                        d_tests_is_error(m::expr_parse("x[1, 2, 3"),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'[' is never closed"),
                        "an index missing, three indices, a semicolon or "
                        "two operands in an index, a bracket unbalanced, "
                        "a transpose of nothing: each error at its place, "
                        "the brackets' before the grammar's");

        return;
    }

    // d_tests_render_indexing
    //   indexing and the transpose in the renderer: an index after its
    // operand, bare where the operand is a leaf, a call, a literal, an
    // index or a postfix operator, and bracketed otherwise; the transpose
    // as the factorial is; and each read back as itself.
    void
    d_tests_render_indexing()
    {
        const char* texts[][2] = {
            { "v[1]", "v[1]" },             { "A[1,2]", "A[1, 2]" },
            { "(a+b)[1]", "(a + b)[1]" },   { "(-x)[1]", "(-x)[1]" },
            { "-x[1]", "-x[1]" },           { "(x^2)[1]", "(x ^ 2)[1]" },
            { "x'[1]", "x'[1]" },           { "(x!)[1]", "x![1]" },
            { "f(x)[1]", "f(x)[1]" },       { "[1,2][1]", "[1, 2][1]" },
            { "A[1][2]", "A[1][2]" },       { "(x[1])'", "x[1]'" },
            { "(a+b)'", "(a + b)'" },       { "A'^2", "(A') ^ 2" },
            { "A^2'", "A ^ 2'" },           { "(A^2)'", "A ^ 2'" },
            { "A^(2')", "A ^ (2')" },       { "x'!", "(x')!" },
            { "-A'", "-A'" },               { "(-A)'", "(-A)'" },
            { "2^v[1]", "2 ^ v[1]" },       { "1[2]", "1[2]" },
            { "A[i+1,j*2]", "A[i + 1, j * 2]" },
            { "v[v[1]]", "v[v[1]]" },       { "A''", "A''" },
            { "(1/3)[1]", "(1 / 3)[1]" },   { "[1;2]'[2]", "[1; 2]'[2]" } };
        bool rendered = true;
        bool back     = true;

        for (std::size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); ++i)
        {
            const m::expr_term term = d_tests_parsed(texts[i][0]);

            rendered = ( (rendered) &&
                         (d_tests_renders(texts[i][0],
                                          texts[i][1])) );
            back     = ( (back) &&
                         (d_tests_parses(texts[i][1],
                                         term)) );
        }

        D_TESTS_EXPECT( rendered,
                        "an index after its operand, bare after a leaf, a "
                        "call, a literal, an index or a postfix operator, "
                        "bracketed otherwise; the transpose as the "
                        "factorial");
        D_TESTS_EXPECT( back,
                        "and each read back as itself");
        D_TESTS_EXPECT( (m::expr_render(d_tests_index_of(
                             m::expr_number(m::value(-1), "-1"),
                             { d_tests_literal("2") })) == "(-1)[2]") &&
                        (m::expr_render(d_tests_index_of(
                             m::expr_number(m::value::ratio(1, 3), "1/3"),
                             { d_tests_literal("1") })) == "(1/3)[1]") &&
                        (m::expr_render(m::expr_application(
                             m::expr_op_kind::index,
                             { m::expr_variable("v") })) == "v[]"),
                        "a literal no unsigned decimal in brackets before "
                        "its index; an index of no index, made by hand, "
                        "its brackets alone");

        return;
    }

    // d_tests_fold_indexing
    //   both folds of matrix texts: as a value, the kind's arithmetic,
    // powers, the transpose, indexing and the relations (S246 to S249); in
    // a type, a NaN for indexing and a number's transpose the number.
    void
    d_tests_fold_indexing()
    {
        const m::value         a       = d_tests_two_by_two();
        const m::value         v       = d_tests_made(
                                             m::expr_vector({ 5, 6, 7 }));
        const m::value         wide    = d_tests_made(
                                             m::expr_matrix(2,
                                                            3,
                                                            { 1, 2, 3, 4, 5,
                                                              6 }));
        m::expr_bindings       bindings;
        const m::value         domain(D_MATH_VALUE_ERROR_DOMAIN);

        static_cast<void>(bindings.at("A", a)
                                  .at("v", v)
                                  .at("B", wide)
                                  .at("x", 2)
                                  .at("y", 3));

        D_TESTS_EXPECT( d_tests_valued("A[2, 1]", m::value(3), bindings) &&
                        d_tests_valued("v[3]", m::value(7), bindings) &&
                        d_tests_valued("v[x + 1]", m::value(7), bindings) &&
                        d_tests_valued("A[x][1]", m::value(3), bindings) &&
                        d_tests_valued("B[2]",
                                       d_tests_made(m::expr_vector(
                                           { 4, 5, 6 })),
                                       bindings) &&
                        d_tests_valued("[1, 2; 3, 4][1, 2]", m::value(2),
                                       bindings) &&
                        d_tests_valued("v[1] * v[2]", m::value(30),
                                       bindings) &&
                        d_tests_valued("-v[1]", m::value(-5), bindings),
                        "indexing from 1, an index any term: an element, "
                        "or another matrix's row");
        D_TESTS_EXPECT( d_tests_valued("A + A", a * m::value(2), bindings) &&
                        d_tests_valued("A * A", a * a, bindings) &&
                        d_tests_valued("2 * A - A", a, bindings) &&
                        d_tests_valued("A / 2", a / m::value(2), bindings) &&
                        d_tests_valued("-A", -a, bindings) &&
                        d_tests_valued("+A", a, bindings) &&
                        d_tests_valued("A ^ 3", (a * a) * a, bindings) &&
                        d_tests_valued("A ^ 0",
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 1, 0, 0, 1 })),
                                       bindings) &&
                        d_tests_valued("pow(A, 2)", a * a, bindings) &&
                        d_tests_valued("B * B'",
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 14, 32, 32, 77 })),
                                       bindings) &&
                        d_tests_valued("A'", m::internal::
                                                 expr_matrix_transposed(a),
                                       bindings) &&
                        d_tests_valued("x'", m::value(2), bindings) &&
                        d_tests_valued("[1, 2] * [3; 4]",
                                       d_tests_made(m::expr_vector({ 11 })),
                                       bindings) &&
                        d_tests_valued("[x, y] * 2",
                                       d_tests_made(m::expr_vector(
                                           { 4, 6 })),
                                       bindings),
                        "the kind's arithmetic as a value: sums, scalar "
                        "products, the product, the transpose and powers, "
                        "a 1 by 1 product a matrix");
        D_TESTS_EXPECT( d_tests_valued("A = A", m::value(true), bindings) &&
                        d_tests_valued("A != [1, 2; 3, 4]", m::value(false),
                                       bindings) &&
                        d_tests_valued("A = B", m::value(false), bindings) &&
                        d_tests_valued("A != B", m::value(true), bindings) &&
                        d_tests_valued("A' = A", m::value(false),
                                       bindings) &&
                        d_tests_valued("(A * A)' = A' * A'",
                                       m::value(true),
                                       bindings),
                        "two matrices equal, or not, as a relation's "
                        "value");

        bool        refused = true;
        const char* texts[] = { "A + 1", "1 - A", "1 / A", "A / B", "A * B'",
                                "B ^ 2", "B ^ (-1)", "A ^ (1 / 2)", "2 ^ A",
                                "A mod 2", "A!", "A < A", "A = 1", "abs(A)",
                                "v[0]", "v[4]", "v[1.5]", "v[true]", "x[1]",
                                "A[1, 2][1]", "B[3]", "sqrt(A)" };

        for (std::size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); ++i)
        {
            refused = ( (refused) &&
                        (d_tests_valued(texts[i],
                                        domain,
                                        bindings)) );
        }

        D_TESTS_EXPECT( refused,
                        "what the kind does not take, a power it does not "
                        "take, an index outside the matrix or not whole, "
                        "indexing a number: a domain error");

        const m::expr_result<double> indexed = m::expr_evaluate<double>(
                                                   d_tests_parsed("v[1]"),
                                                   bindings);
        const m::expr_result<double> number  = m::expr_evaluate<double>(
                                                   d_tests_parsed("x'"),
                                                   bindings);

        D_TESTS_EXPECT( (indexed.is_ok()) &&
                        (indexed.value() != indexed.value()) &&
                        (number.is_ok()) &&
                        (number.value() == 2.0) &&
                        d_tests_typed<float>("x'",
                                             2.0f,
                                             bindings) &&
                        d_tests_typed<long double>("x'",
                                                   2.0L,
                                                   bindings) &&
                        d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_parsed("[1, 2]'"),
                                             bindings),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_evaluate<Type>: a vector or a "
                                         "matrix has no value in a type"),
                        "in a type, indexing a NaN, as a variable bound to "
                        "a matrix is; a number's transpose the number; a "
                        "literal's bad_shape");

        return;
    }

    // d_tests_simplify_matrices
    //   simplification of terms of matrices (S251): a factor holding a
    // matrix keeps its place, so that A * B + B * A stays; numbers and
    // variables commute; no rule makes an error of matrices a value; and
    // each simplified term's value the term's.
    void
    d_tests_simplify_matrices()
    {
        const char* texts[][2] = {
            { "[1, 2; 3, 4] * [5, 6; 7, 8] + [5, 6; 7, 8] * [1, 2; 3, 4]",
              "[1, 2; 3, 4] * [5, 6; 7, 8] + [5, 6; 7, 8] * [1, 2; 3, 4]" },
            { "[1, 2; 3, 4] * [5, 6; 7, 8] + [1, 2; 3, 4] * [5, 6; 7, 8]",
              "2 * [1, 2; 3, 4] * [5, 6; 7, 8]" },
            { "2 * [1, 2] * x + x * [1, 2]", "3 * [1, 2] * x" },
            { "[1, 2] * [3; 4] - [3; 4] * [1, 2]",
              "[1, 2] * [3; 4] - [3; 4] * [1, 2]" },
            { "[1, 2] - [1, 2]", "0 * [1, 2]" },
            { "[1, 2] + [1, 2]", "2 * [1, 2]" },
            { "[1, 2] + 0", "[1, 2] + 0" },
            { "0 - [1, 2]", "0 - [1, 2]" },
            { "[1, 2] + 1 - 1", "[1, 2] + 0" },
            { "x - x + [1, 2]", "0 + [1, 2]" },
            { "[1, 2] + x - x + y - y", "[1, 2] + 0" },
            { "[1, 2] ^ 1", "[1, 2] ^ 1" },
            { "[1, 2; 3, 4] ^ 1", "[1, 2; 3, 4]" },
            { "[1, 2; 3, 4; 5, 6] ^ 1", "[1, 2; 3, 4; 5, 6] ^ 1" },
            { "[5] ^ 1", "[5]" },
            { "([1, 2] * [3; 4]) ^ 1", "([1, 2] * [3; 4]) ^ 1" },
            { "[1, 2; 3, 4] ^ 0", "[1, 2; 3, 4] ^ 0" },
            { "0 * [1, 2]", "0 * [1, 2]" },
            { "1 * [1, 2]", "[1, 2]" },
            { "[1, 2] / 1", "[1, 2]" },
            { "2 * [1, 2] * 3", "6 * [1, 2]" },
            { "-(-[1, 2])", "[1, 2]" },
            { "[1, 2]' + [1, 2]'", "2 * [1, 2]'" },
            { "x * y + y * x", "2 * x * y" },
            { "A ^ 0", "1" } };
        const char* const valued[] = {
            "[1, 2; 3, 4] * [5, 6; 7, 8] + [5, 6; 7, 8] * [1, 2; 3, 4]",
            "[1, 2; 3, 4] * [5, 6; 7, 8] + [1, 2; 3, 4] * [5, 6; 7, 8]",
            "2 * [1, 2] * x + x * [1, 2]", "[1, 2] - [1, 2]",
            "[1, 2] + [1, 2]", "[1, 2] + 0", "0 - [1, 2]", "[1, 2] + 1 - 1",
            "x - x + [1, 2]", "[1, 2] ^ 1", "[1, 2; 3, 4] ^ 1", "[5] ^ 1",
            "[1, 2; 3, 4; 5, 6] ^ 1",
            "([1, 2] * [3; 4]) ^ 1", "[1, 2; 3, 4] ^ 0", "0 * [1, 2]",
            "2 * [1, 2] * 3", "-(-[1, 2])", "[1, 2]' + [1, 2]'",
            "[1, 2] * [3; 4] - [3; 4] * [1, 2]" };
        bool             simplified = true;
        bool             kept       = true;
        m::expr_bindings bindings;

        static_cast<void>(bindings.at("x", 3)
                                  .at("y", m::value::ratio(1, 2)));

        for (std::size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); ++i)
        {
            simplified = ( (simplified) &&
                           (m::expr_render(m::expr_simplify(
                                d_tests_parsed(texts[i][0]))) ==
                                texts[i][1]) );
        }

        // each simplified term's value the term's, an error among them
        for (std::size_t i = 0;
             i < (sizeof(valued) / sizeof(valued[0]));
             ++i)
        {
            const m::expr_term             term   = d_tests_parsed(valued[i]);
            const m::expr_result<m::value> before =
                m::expr_evaluate(term,
                                 bindings,
                                 m::expr_context());
            const m::expr_result<m::value> after  =
                m::expr_evaluate(m::expr_simplify(term),
                                 bindings,
                                 m::expr_context());

            kept = ( (kept) &&
                     (before.is_ok()) &&
                     (d_tests_holds_value(after,
                                          before.value())) );
        }

        D_TESTS_EXPECT( simplified,
                        "a factor holding a matrix keeps its place, so "
                        "that AB + BA stays and AB + AB is 2AB; numbers "
                        "and variables commute; A ^ 1 is A where A is "
                        "square by its shape; a sum of a matrix keeps a "
                        "0; a variable taken for a number");
        D_TESTS_EXPECT( kept,
                        "each simplified term of matrices of the value of "
                        "the term, an error of shapes the same error");

        const m::expr_term square = m::expr_number(d_tests_two_by_two());
        const m::expr_term wide   = m::expr_number(
                                        d_tests_made(m::expr_vector(
                                            { 1, 2 })));
        const m::expr_term unit   = d_tests_literal("1");
        const m::expr_term zero   = d_tests_literal("0");

        D_TESTS_EXPECT( d_tests_same(m::expr_simplify(
                                         d_tests_apply(m::expr_op_kind::pow,
                                                       square,
                                                       unit)),
                                     square) &&
                        d_tests_same(m::expr_simplify(
                                         d_tests_apply(m::expr_op_kind::pow,
                                                       wide,
                                                       unit)),
                                     d_tests_apply(m::expr_op_kind::pow,
                                                   wide,
                                                   unit)) &&
                        d_tests_same(m::expr_simplify(
                                         d_tests_apply(m::expr_op_kind::add,
                                                       wide,
                                                       zero)),
                                     d_tests_apply(m::expr_op_kind::add,
                                                   wide,
                                                   zero)),
                        "a matrix a fold made: A ^ 1 A where it is square, "
                        "and kept where not; A + 0 kept");

        return;
    }

    // d_tests_reference
    //   struct: a matrix as the random part computes it, apart from the
    // kind: its shape and its elements, row by row, each an exact value; or
    // a domain error, where the operation takes no such operands.
    struct d_tests_reference
    {
        std::size_t           rows;
        std::size_t           columns;
        std::vector<m::value> elements;
        bool                  refused;

        // a domain error.
        d_tests_reference()
            : rows(0),
              columns(0),
              elements(),
              refused(true)
        {}

        // a matrix of _rows by _columns of 0s.
        d_tests_reference(
            std::size_t _rows,
            std::size_t _columns
        )
            : rows(_rows),
              columns(_columns),
              elements(_rows * _columns, m::value(0)),
              refused(false)
        {}

        // at
        //   the element at row _r, column _c.
        m::value&
        at(
            std::size_t _r,
            std::size_t _c
        )
        {
            return elements[(_r * columns) + _c];
        }

        const m::value&
        at(
            std::size_t _r,
            std::size_t _c
        ) const
        {
            return elements[(_r * columns) + _c];
        }
    };

    // d_tests_reference_sum, d_tests_reference_scaled,
    // d_tests_reference_product, d_tests_reference_transposed,
    // d_tests_reference_power
    //   functions: the random part's own arithmetic, by the definitions,
    // element by element in the value's exact arithmetic, apart from the
    // kind and its kernels: A + B or A - B, of one shape; s * A; A * B, by
    // the sums of products; A'; and A ^ n, by n - 1 products, not by
    // squaring.
    d_tests_reference
    d_tests_reference_sum(
        const d_tests_reference& _a,
        const d_tests_reference& _b,
        bool                     _minus
    )
    {
        // a refusal, or two shapes
        if ( (_a.refused) ||
             (_b.refused) ||
             (_a.rows != _b.rows) ||
             (_a.columns != _b.columns) )
        {
            return d_tests_reference();
        }

        d_tests_reference out(_a.rows,
                              _a.columns);

        for (std::size_t i = 0; i < out.elements.size(); ++i)
        {
            out.elements[i] = ( _minus ? (_a.elements[i] - _b.elements[i])
                                       : (_a.elements[i] + _b.elements[i]) );
        }

        return out;
    }

    d_tests_reference
    d_tests_reference_scaled(
        const m::value&          _s,
        const d_tests_reference& _a
    )
    {
        // a refusal
        if (_a.refused)
        {
            return _a;
        }

        d_tests_reference out(_a.rows,
                              _a.columns);

        for (std::size_t i = 0; i < out.elements.size(); ++i)
        {
            out.elements[i] = (_s * _a.elements[i]);
        }

        return out;
    }

    d_tests_reference
    d_tests_reference_product(
        const d_tests_reference& _a,
        const d_tests_reference& _b
    )
    {
        // a refusal, or shapes that do not fit
        if ( (_a.refused) ||
             (_b.refused) ||
             (_a.columns != _b.rows) )
        {
            return d_tests_reference();
        }

        d_tests_reference out(_a.rows,
                              _b.columns);

        for (std::size_t r = 0; r < out.rows; ++r)
        {
            for (std::size_t c = 0; c < out.columns; ++c)
            {
                m::value sum(0);

                for (std::size_t k = 0; k < _a.columns; ++k)
                {
                    sum = ( sum + (_a.at(r, k) * _b.at(k, c)) );
                }

                out.at(r, c) = sum;
            }
        }

        return out;
    }

    d_tests_reference
    d_tests_reference_transposed(
        const d_tests_reference& _a
    )
    {
        // a refusal
        if (_a.refused)
        {
            return _a;
        }

        d_tests_reference out(_a.columns,
                              _a.rows);

        for (std::size_t r = 0; r < _a.rows; ++r)
        {
            for (std::size_t c = 0; c < _a.columns; ++c)
            {
                out.at(c, r) = _a.at(r, c);
            }
        }

        return out;
    }

    d_tests_reference
    d_tests_reference_power(
        const d_tests_reference& _a,
        unsigned                 _n
    )
    {
        // a refusal, or no square
        if ( (_a.refused) ||
             (_a.rows != _a.columns) )
        {
            return d_tests_reference();
        }

        d_tests_reference out(_a.rows,
                              _a.rows);

        // the identity, then n products
        for (std::size_t i = 0; i < _a.rows; ++i)
        {
            out.at(i, i) = m::value(1);
        }

        for (unsigned k = 0; k < _n; ++k)
        {
            out = d_tests_reference_product(out,
                                            _a);
        }

        return out;
    }

    // d_tests_matrix_drawn
    //   struct: a random term of matrices and what the random part's own
    // arithmetic makes it.
    struct d_tests_matrix_drawn
    {
        m::expr_term      term;
        d_tests_reference value;
    };

    // d_tests_random_matrix
    //   a random term of _rows by _columns, of depth _depth at most: a
    // literal of whole numbers from 0 to 3 and halves, each perhaps
    // negated; a sum or a difference; a number times it; a product through
    // a random inner size; a transpose; a power, to 3 at most, of a square
    // one of depth 1 at most; or a row of a taller one; and, a draw in 27,
    // a sum or a product of shapes that do not fit, which the kind
    // refuses. Its elements stay far inside the exact integers' range.
    d_tests_matrix_drawn
    d_tests_random_matrix(
        d_tests_rng& _rng,
        std::size_t  _rows,
        std::size_t  _columns,
        unsigned     _depth
    )
    {
        unsigned             pick = ( (_depth == 0) ? 0u
                                                    : _rng.below(9u) );
        d_tests_matrix_drawn out;

        // a power of no square, or a row of no single row: a sum instead;
        // shapes that do not fit one draw in three of their ninth, a
        // product otherwise
        if ( ( (pick == 6) &&
               (_rows != _columns) ) ||
             ( (pick == 7) &&
               ( (_rows != 1) ||
                 (_columns == 1) ) ) )
        {
            pick = 1;
        }

        if ( (pick == 8) &&
             (_rng.below(3u) != 0) )
        {
            pick = 4;
        }

        // a literal
        if (pick == 0)
        {
            std::vector<std::vector<m::expr_term> > rows(_rows);

            out.value = d_tests_reference(_rows,
                                          _columns);

            for (std::size_t r = 0; r < _rows; ++r)
            {
                for (std::size_t c = 0; c < _columns; ++c)
                {
                    const unsigned whole = _rng.below(4u);
                    const bool     half  = (_rng.below(4u) == 0);
                    const bool     minus = (_rng.below(3u) == 0);
                    m::expr_term   element =
                        d_tests_literal(( half ? "0.5"
                                               : std::to_string(whole)
                                                     .c_str() ));
                    m::value       number  =
                        ( half ? m::value::ratio(1, 2)
                               : m::value(static_cast<int>(whole)) );

                    if (minus)
                    {
                        element = d_tests_apply(m::expr_op_kind::neg,
                                                element);
                        number  = -number;
                    }

                    rows[r].push_back(element);
                    out.value.at(r, c) = number;
                }
            }

            out.term = ( (_rows == 1) ? d_tests_vector_of(rows[0])
                                      : d_tests_matrix_of(rows) );

            return out;
        }

        // a sum or a difference
        if (pick <= 2)
        {
            const d_tests_matrix_drawn a = d_tests_random_matrix(_rng,
                                                                 _rows,
                                                                 _columns,
                                                                 _depth - 1);
            const d_tests_matrix_drawn b = d_tests_random_matrix(_rng,
                                                                 _rows,
                                                                 _columns,
                                                                 _depth - 1);

            out.term  = d_tests_apply( (pick == 1) ? m::expr_op_kind::add
                                                   : m::expr_op_kind::sub,
                                       a.term,
                                       b.term);
            out.value = d_tests_reference_sum(a.value,
                                              b.value,
                                              (pick == 2));

            return out;
        }

        // a number times it, on either side
        if (pick == 3)
        {
            const d_tests_matrix_drawn a     = d_tests_random_matrix(
                                                   _rng,
                                                   _rows,
                                                   _columns,
                                                   _depth - 1);
            const unsigned             whole = 1 + _rng.below(3u);
            const m::expr_term         s     = d_tests_literal(
                                                   std::to_string(whole)
                                                       .c_str());

            out.term  = ( (_rng.below(2u) == 0)
                              ? d_tests_apply(m::expr_op_kind::mul,
                                              s,
                                              a.term)
                              : d_tests_apply(m::expr_op_kind::mul,
                                              a.term,
                                              s) );
            out.value = d_tests_reference_scaled(
                            m::value(static_cast<int>(whole)),
                            a.value);

            return out;
        }

        // a product through an inner size
        if (pick == 4)
        {
            const std::size_t          inner = 1 + _rng.below(3u);
            const d_tests_matrix_drawn a     = d_tests_random_matrix(
                                                   _rng,
                                                   _rows,
                                                   inner,
                                                   _depth - 1);
            const d_tests_matrix_drawn b     = d_tests_random_matrix(
                                                   _rng,
                                                   inner,
                                                   _columns,
                                                   _depth - 1);

            out.term  = d_tests_apply(m::expr_op_kind::mul,
                                      a.term,
                                      b.term);
            out.value = d_tests_reference_product(a.value,
                                                  b.value);

            return out;
        }

        // a transpose
        if (pick == 5)
        {
            const d_tests_matrix_drawn a = d_tests_random_matrix(_rng,
                                                                 _columns,
                                                                 _rows,
                                                                 _depth - 1);

            out.term  = d_tests_apply(m::expr_op_kind::transpose,
                                      a.term);
            out.value = d_tests_reference_transposed(a.value);

            return out;
        }

        // a power of a square one
        if (pick == 6)
        {
            const d_tests_matrix_drawn a = d_tests_random_matrix(
                                               _rng,
                                               _rows,
                                               _rows,
                                               ( (_depth > 1) ? 1u
                                                              : 0u ));
            const unsigned             n = _rng.below(4u);

            out.term  = d_tests_apply(m::expr_op_kind::pow,
                                      a.term,
                                      d_tests_literal(std::to_string(n)
                                                          .c_str()));
            out.value = d_tests_reference_power(a.value,
                                                n);

            return out;
        }

        // a row of a taller matrix, of more than one column
        if (pick == 7)
        {
            const std::size_t          taller = 2 + _rng.below(2u);
            const std::size_t          chosen = _rng.below(
                                                    static_cast<unsigned>(
                                                        taller));
            const d_tests_matrix_drawn a      = d_tests_random_matrix(
                                                    _rng,
                                                    taller,
                                                    _columns,
                                                    _depth - 1);

            out.term  = d_tests_index_of(a.term,
                                         { d_tests_literal(
                                               std::to_string(chosen + 1)
                                                   .c_str()) });
            out.value = d_tests_reference(1,
                                          _columns);
            out.value.refused = a.value.refused;

            for (std::size_t c = 0;
                 ( (c < _columns) && (!a.value.refused) );
                 ++c)
            {
                out.value.at(0, c) = a.value.at(chosen, c);
            }

            return out;
        }

        // shapes that do not fit: A + B of B's columns not A's, or A * B
        // of B's rows not A's columns, each a sum or a product the kind
        // refuses
        const std::size_t          other = ( (_columns % 3) + 1 );
        const bool                 sum   = (_rng.below(2u) == 0);
        const d_tests_matrix_drawn a     = d_tests_random_matrix(_rng,
                                                                 _rows,
                                                                 _columns,
                                                                 _depth - 1);
        const d_tests_matrix_drawn b     = d_tests_random_matrix(
                                               _rng,
                                               ( sum ? _rows
                                                     : other ),
                                               ( sum ? other
                                                     : _columns ),
                                               _depth - 1);

        out.term  = d_tests_apply( sum ? m::expr_op_kind::add
                                       : m::expr_op_kind::mul,
                                   a.term,
                                   b.term);
        out.value = ( sum ? d_tests_reference_sum(a.value,
                                                  b.value,
                                                  false)
                          : d_tests_reference_product(a.value,
                                                      b.value) );

        return out;
    }

    // d_tests_agrees
    //   whether a fold's value is the reference: the same matrix, element
    // by element, each element exact as the reference's is; or a domain
    // error where the reference is refused.
    bool
    d_tests_agrees(
        const m::expr_result<m::value>& _folded,
        const d_tests_reference&        _want
    )
    {
        // a fold that failed
        if (_folded.is_err())
        {
            return false;
        }

        // a refusal: a domain error
        if (_want.refused)
        {
            return d_tests_same_values(_folded.value(),
                                       m::value(D_MATH_VALUE_ERROR_DOMAIN));
        }

        const m::expr_shape shape = d_tests_shape(_folded.value());
        bool                same  = ( (shape.rows == _want.rows) &&
                                      (shape.columns == _want.columns) );

        for (std::size_t r = 0; ( (same) && (r < _want.rows) ); ++r)
        {
            for (std::size_t c = 0; c < _want.columns; ++c)
            {
                same = ( (same) &&
                         (d_tests_same_values(
                              d_tests_made(m::expr_element(_folded.value(),
                                                           r,
                                                           c)),
                              _want.at(r, c))) );
            }
        }

        return same;
    }

    // d_tests_matrices_random
    //   1,000 random terms of matrices of one to three rows and columns,
    // drawn by a generator of their own (S254), each folded as a value and
    // compared with the random part's own arithmetic, by the definitions,
    // apart from the kind: the same matrix, exactly, or a domain error for
    // shapes that do not fit; each rendered and read back as itself;
    // simplified, no larger and of the same value; and in a type bad_shape.
    void
    d_tests_matrices_random()
    {
        d_tests_rng           rng(20261026u);
        const m::expr_context context;
        std::size_t           agreed     = 0;
        std::size_t           back       = 0;
        std::size_t           simplified = 0;
        std::size_t           smaller    = 0;
        std::size_t           typed      = 0;
        std::size_t           refused    = 0;
        const std::size_t     count      = 1000;

        for (std::size_t n = 0; n < count; ++n)
        {
            const std::size_t              rows    = 1 + rng.below(3u);
            const std::size_t              columns = 1 + rng.below(3u);
            const d_tests_matrix_drawn     drawn   =
                d_tests_random_matrix(rng,
                                      rows,
                                      columns,
                                      3u);
            const m::expr_result<m::value> folded  =
                m::expr_evaluate(drawn.term,
                                 m::expr_bindings(),
                                 context);
            const m::expr_term             simple  =
                m::expr_simplify(drawn.term);
            const m::expr_result<double>   in_type =
                m::expr_evaluate<double>(drawn.term);

            refused += ( drawn.value.refused ? 1 : 0 );
            agreed  += ( d_tests_agrees(folded,
                                        drawn.value) ? 1 : 0 );
            back    += ( d_tests_parses(m::expr_render(drawn.term).c_str(),
                                        drawn.term) ? 1 : 0 );
            smaller += ( (::djinterp::size<m::expr_op, m::expr_atom>(simple) <=
                          ::djinterp::size<m::expr_op, m::expr_atom>(
                              drawn.term)) ? 1 : 0 );
            simplified += ( d_tests_agrees(m::expr_evaluate(simple,
                                                            m::expr_bindings(),
                                                            context),
                                           drawn.value) ? 1 : 0 );
            typed   += ( ( (in_type.is_err()) &&
                           (in_type.error().status ==
                                m::expr_status::bad_shape) ) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (refused > (count / 20)) &&
                        (refused < (count / 2)),
                        "some of the random terms of shapes that do not "
                        "fit, most of shapes that do");
        D_TESTS_EXPECT( (agreed == count),
                        "1,000 random terms of matrices folded as values: "
                        "each the matrix the definitions give, exactly, or "
                        "a domain error where shapes do not fit");
        D_TESTS_EXPECT( (back == count),
                        "each rendered and read back as itself");
        D_TESTS_EXPECT( (smaller == count) &&
                        (simplified == count),
                        "each simplified no larger, and of the same value, "
                        "a domain error the same error");
        D_TESTS_EXPECT( (typed == count),
                        "in a type, each bad_shape");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_ARITHMETIC_HPP
