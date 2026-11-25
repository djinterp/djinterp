/*******************************************************************************
* djinterp [cpp][math]                                   t_expr_face_algebra.hpp
*
* One part of t_expr_face.cpp, the expression language's face test (S438), its
* section 11's last third: linear algebra's functions, eigenvalues, negative
* powers, the empty matrix and blocks. The test includes the parts in order,
* one program; each part includes the one before it.
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_face_algebra.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_ALGEBRA_HPP
#define DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_ALGEBRA_HPP 1

#include "./t_expr_face_arithmetic.hpp" // the part before


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


namespace
{

    // d_tests_dense_values
    //   a dense matrix's elements, row by row, as values of Type.
    template<typename Type>
    std::vector<m::value>
    d_tests_dense_values(
        const Type*  _entries,
        std::size_t  _count
    )
    {
        std::vector<m::value> elements;

        for (std::size_t i = 0; i < _count; ++i)
        {
            elements.push_back(m::value(_entries[i]));
        }

        return elements;
    }

    // d_tests_algebra_in
    //   whether the square algebra of the dense A = [4, 3, 1; 6, 3, 2;
    // 1, 5, 7] in Type is the C core's in Type, bit for bit: det the
    // parity times U's diagonal of d_decomp_lu, inv d_mat_inverse's, solve
    // of b = [1, 2, 3] d_mat_solve's, refined (S428), given as a row; and
    // trace 14, each a dense Type, held in _storage.
    template<typename Type>
    bool
    d_tests_algebra_in(
        m::internal::expr_matrix_storage _storage
    )
    {
        const Type     entries[9] = { Type(4), Type(3), Type(1),
                                      Type(6), Type(3), Type(2),
                                      Type(1), Type(5), Type(7) };
        const Type     right[3]   = { Type(1), Type(2), Type(3) };
        const m::value a          = d_tests_made(
                                        m::expr_matrix(3,
                                                       3,
                                                       d_tests_dense_values(
                                                           entries,
                                                           9)));
        const m::value b          = d_tests_made(
                                        m::expr_vector(d_tests_dense_values(
                                                           right,
                                                           3)));
        Type           lu[9]      = {};
        Type           inverse[9] = {};
        Type           scratch[9] = {};
        Type           x[3]       = {};
        Type           work[15]   = {};
        std::size_t    perm[3]    = {};
        std::size_t    pivots[3]  = {};
        Type           sign       = Type(1);

        for (std::size_t i = 0; i < 9; ++i)
        {
            lu[i] = entries[i];
        }

        static_cast<void>(d_decomp_lu(lu,
                                      3,
                                      perm,
                                      &sign));
        static_cast<void>(d_mat_solve(x, work, pivots, entries, right, 3, 1));
        static_cast<void>(d_mat_inverse(inverse,
                                        scratch,
                                        entries,
                                        3));

        volatile Type det = d_decomp_lu_det(lu,
                                            3,
                                            sign);

        const Type     flat[4]    = { Type(1), Type(2), Type(2), Type(4) };
        const Type     general[4] = { Type(2), Type(1), Type(0), Type(3) };
        const Type     turn[4]    = { Type(0), Type(-1), Type(1), Type(0) };
        const Type     zeros[4]   = { Type(0), Type(0), Type(0), Type(0) };
        const Type     nan[4]     = { std::numeric_limits<Type>::quiet_NaN(),
                                      Type(0), Type(0), Type(1) };
        const Type     across[3]  = { Type(0), Type(1), Type(0) };
        const Type     product[3] = { Type(-3), Type(0), Type(1) };
        const m::value singular   = d_tests_made(
                                        m::expr_matrix(2,
                                                       2,
                                                       d_tests_dense_values(
                                                           flat,
                                                           4)));
        const m::value pair       = d_tests_made(
                                        m::expr_vector(d_tests_dense_values(
                                                           right,
                                                           2)));
        const m::value other      = d_tests_made(
                                        m::expr_vector(d_tests_dense_values(
                                                           across,
                                                           3)));
        const m::value pole(D_MATH_VALUE_ERROR_POLE);
        const m::value nans       = d_tests_made(m::expr_eig(
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            d_tests_dense_values(nan,
                                                                 4)))));
        Type           first      = Type(0);
        Type           nan_norm   = Type(0);
        volatile Type  dot        = Type(14);
        volatile Type  norm       = d_math_sqrt(Type(14));

        static_cast<void>(d_tests_made(m::expr_element(nans,
                                                       0,
                                                       0)).to(first));

        return ( (d_tests_same_values(d_tests_made(m::expr_det(a)),
                                      m::value(static_cast<Type>(det)))) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_inv(a)),
                                    3, 3,
                                    d_tests_dense_values(inverse,
                                                         9),
                                    _storage)) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_solve(a,
                                                               b)),
                                    1, 3,
                                    d_tests_dense_values(x,
                                                         3),
                                    _storage)) &&
                 (d_tests_same_values(d_tests_made(m::expr_trace(a)),
                                      m::value(Type(14)))) &&
                 (d_tests_same_values(d_tests_made(m::expr_inv(singular)),
                                      pole)) &&
                 (d_tests_same_values(d_tests_made(m::expr_solve(singular,
                                                                 pair)),
                                      pole)) &&
                 (d_tests_same_values(d_tests_made(m::expr_dot(b,
                                                               b)),
                                      m::value(static_cast<Type>(dot)))) &&
                 (d_tests_same_values(d_tests_made(m::expr_norm(b)),
                                      m::value(static_cast<Type>(norm)))) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_cross(b,
                                                               other)),
                                    1, 3,
                                    d_tests_dense_values(product,
                                                         3),
                                    _storage)) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_cross(
                                        d_tests_made(m::expr_matrix(
                                            3,
                                            1,
                                            d_tests_dense_values(right,
                                                                 3))),
                                        other)),
                                    3, 1,
                                    d_tests_dense_values(product,
                                                         3),
                                    _storage)) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_eig(
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            d_tests_dense_values(general,
                                                                 4))))),
                                    1, 2,
                                    { m::value(Type(2)), m::value(Type(3)) },
                                    _storage)) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_eig(
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            d_tests_dense_values(zeros,
                                                                 4))))),
                                    1, 2,
                                    { m::value(Type(0)), m::value(Type(0)) },
                                    _storage)) &&
                 (d_tests_is_matrix(d_tests_made(m::expr_eig(
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            d_tests_dense_values(turn,
                                                                 4))))),
                                    1, 2,
                                    { m::expr_complex_of(
                                          m::complex<Type>(Type(0),
                                                           Type(-1))),
                                      m::expr_complex_of(
                                          m::complex<Type>(Type(0),
                                                           Type(1))) },
                                    m::internal::expr_storage_complex_of(
                                        _storage))) &&
                 (first != first) &&
                 (d_tests_stored_as(nans,
                                    _storage)) &&
                 (d_tests_same_values(d_tests_made(m::expr_norm(
                                          d_tests_made(m::expr_matrix(
                                              2,
                                              2,
                                              d_tests_dense_values(zeros,
                                                                   4))))),
                                      m::value(Type(0)))) &&
                 (d_tests_made(m::expr_norm(
                      d_tests_made(m::expr_matrix(
                          2,
                          2,
                          d_tests_dense_values(nan,
                                               4))))).to(nan_norm)) &&
                 (nan_norm != nan_norm) );
    }

    // d_tests_cofactors
    //   the test's own determinant of an exact square matrix, of _n rows,
    // its elements row by row: the cofactor expansion along the first
    // row, apart from the kind and its kernels.
    m::value
    d_tests_cofactors(
        const std::vector<m::value>& _a,
        std::size_t                  _n
    )
    {
        m::value sum(0);

        // 0 by 0: 1; 1 by 1: the element
        if (_n == 0)
        {
            return m::value(1);
        }

        if (_n == 1)
        {
            return _a[0];
        }

        for (std::size_t j = 0; j < _n; ++j)
        {
            std::vector<m::value> minor;

            for (std::size_t r = 1; r < _n; ++r)
            {
                for (std::size_t c = 0; c < _n; ++c)
                {
                    if (c != j)
                    {
                        minor.push_back(_a[(r * _n) + c]);
                    }
                }
            }

            const m::value term = _a[j] * d_tests_cofactors(minor,
                                                            _n - 1);

            sum = ( ((j % 2) == 0) ? (sum + term) : (sum - term) );
        }

        return sum;
    }

    // d_tests_elements_of
    //   a matrix's elements, row by row; none where the value is no matrix.
    std::vector<m::value>
    d_tests_elements_of(
        const m::value& _x
    )
    {
        const m::expr_shape   shape = d_tests_shape(_x);
        std::vector<m::value> elements;

        for (std::size_t r = 0; r < shape.rows; ++r)
        {
            for (std::size_t c = 0; c < shape.columns; ++c)
            {
                elements.push_back(d_tests_made(m::expr_element(_x,
                                                                r,
                                                                c)));
            }
        }

        return elements;
    }

    // d_tests_near
    //   whether a value is a number within _tolerance of _want.
    bool
    d_tests_near(
        const m::value& _x,
        long double     _want,
        long double     _tolerance
    )
    {
        long double x = 0.0L;

        return ( (_x.is_number()) &&
                 (_x.to(x)) &&
                 (std::fabs(x - _want) <= _tolerance) );
    }

    // d_tests_element_near
    //   whether a matrix's element at _place, row by row, is within
    // _tolerance of _want.
    bool
    d_tests_element_near(
        const m::value& _x,
        std::size_t     _place,
        long double     _want,
        long double     _tolerance
    )
    {
        const std::vector<m::value> elements = d_tests_elements_of(_x);

        return ( (_place < elements.size()) &&
                 (d_tests_near(elements[_place],
                               _want,
                               _tolerance)) );
    }

    // d_tests_matrix_algebra
    //   step 21's square algebra as values: det, inv, trace and solve,
    // each dense type's the C core's in that type, bit for bit; a matrix of
    // values with an inexact element in the widest inexact type; an exact
    // matrix's exactly, the test's own cofactors its determinant; a
    // singular one's inverse and solution a pole; a boxed inexact element's
    // own arithmetic; the flags carried; and an error passing first, a
    // boolean or none a domain error.
    void
    d_tests_matrix_algebra()
    {
        typedef m::internal::expr_matrix_storage storage;

        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);
        const m::value pole(D_MATH_VALUE_ERROR_POLE);

        D_TESTS_EXPECT( (d_tests_algebra_in<float>(storage::floats)) &&
                        (d_tests_algebra_in<double>(storage::doubles)) &&
                        (d_tests_algebra_in<long double>(
                            storage::long_doubles)),
                        "each dense type's algebra in that type, the C "
                        "core's, bit for bit: det and inv by LU with "
                        "partial pivoting and Gauss-Jordan, solve refined "
                        "(S428), a singular "
                        "matrix's a pole; trace, dot, norm and cross, the "
                        "zero matrix's norm 0 and a NaN's a NaN; eig by QR, "
                        "of the zero matrix its zeros, of a turn -i and i, "
                        "complex (S352), of a NaN NaNs");

        const long double root_two = 1.4142135623730950488L;
        const long double mixed[4] = { root_two, 1.0L, 1.0L, 1.0L };
        const m::value    with_ld  = d_tests_made(
                                         m::expr_matrix(2,
                                                        2,
                                                        { m::value(root_two),
                                                          1,
                                                          1,
                                                          1 }));
        const m::value    with_f   = d_tests_made(
                                         m::expr_matrix(2,
                                                        2,
                                                        { m::value(0.5f),
                                                          1,
                                                          1,
                                                          2 }));
        long double       lu[4]    = { root_two, 1.0L, 1.0L, 1.0L };
        long double       inv[4]   = {};
        long double       work[4]  = {};
        std::size_t       perm[2]  = {};
        long double       sign     = 1.0L;
        float             lu_f[4]  = { 0.5f, 1.0f, 1.0f, 2.0f };
        std::size_t       perm_f[2] = {};
        float             sign_f   = 1.0f;
        const m::value    with_two = d_tests_made(
                                         m::expr_matrix(2,
                                                        2,
                                                        { m::value(0.1L),
                                                          1,
                                                          1,
                                                          m::value(0.25f) }));
        long double       lu_two[4] = { 0.1L, 1.0L, 1.0L, 0.25L };
        std::size_t       perm_two[2] = {};
        long double       sign_two = 1.0L;
        const m::value    exact_a  = d_tests_made(m::expr_matrix(2,
                                                                 2,
                                                                 { 3, 1,
                                                                   2, 7 }));
        const double      b_d[2]   = { 0.1, 0.3 };
        const double      a_d[4]   = { 3.0, 1.0, 2.0, 7.0 };
        double            work_d[8] = {};
        double            x_d[2]   = {};
        std::size_t       pivots_d[2] = {};

        static_cast<void>(d_decomp_lu(lu,
                                      2,
                                      perm,
                                      &sign));
        static_cast<void>(d_decomp_lu(lu_f,
                                      2,
                                      perm_f,
                                      &sign_f));
        static_cast<void>(d_mat_inverse(inv,
                                        work,
                                        mixed,
                                        2));
        static_cast<void>(d_decomp_lu(lu_two,
                                      2,
                                      perm_two,
                                      &sign_two));
        static_cast<void>(d_mat_solve(x_d,
                                      work_d,
                                      pivots_d,
                                      a_d,
                                      b_d,
                                      2,
                                      1));

        volatile long double det_ld  = d_decomp_lu_det(lu,
                                                       2,
                                                       sign);
        volatile long double det_two = d_decomp_lu_det(lu_two,
                                                       2,
                                                       sign_two);
        volatile float       det_f  = d_decomp_lu_det(lu_f,
                                                      2,
                                                      sign_f);

        D_TESTS_EXPECT( (d_tests_same_values(
                            d_tests_made(m::expr_det(with_ld)),
                            m::value(static_cast<long double>(det_ld)))) &&
                        (d_tests_is_matrix(d_tests_made(m::expr_inv(with_ld)),
                                           2, 2,
                                           d_tests_dense_values(inv,
                                                                4),
                                           storage::long_doubles)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_det(with_f)),
                            m::value(static_cast<float>(det_f)))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_det(with_two)),
                            m::value(static_cast<long double>(det_two)))) &&
                        (d_tests_is_matrix(
                            d_tests_made(m::expr_solve(
                                exact_a,
                                d_tests_made(m::expr_vector(
                                    d_tests_dense_values(b_d,
                                                         2))))),
                            1, 2,
                            d_tests_dense_values(x_d,
                                                 2),
                            storage::doubles)),
                        "a matrix of values with an inexact element: its "
                        "algebra in the widest inexact element's type, "
                        "wherever it stands, each exact element its "
                        "nearest there; an exact matrix's solution with a "
                        "dense one in the dense one's type");

        volatile float       norm_f  = d_math_sqrt(1.25f);
        volatile double      norm_d  = d_math_sqrt(1.25);
        volatile long double norm_ld = d_math_sqrt(1.25L);

        D_TESTS_EXPECT( (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(0.5f), 1 })))),
                            m::value(static_cast<float>(norm_f)))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(0.5), 1 })))),
                            m::value(static_cast<double>(norm_d)))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(0.5L), 1 })))),
                            m::value(static_cast<long double>(norm_ld)))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ 1, 1 })))),
                            m::value(d_math_sqrt(2.0L)))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(std::ldexp(1.0,
                                                                     -13)),
                                                 1 })))),
                            m::value(1.0 + std::ldexp(1.0,
                                                      -27)))),
                        "a norm of values, the root of the sum of squares "
                        "in the sum's own type, as the tower takes sqrt, a "
                        "double's 1 + 2^-26 its root in double; of an exact "
                        "sum not a square, in long double");

        const long double wide_ld  = std::ldexp(1.0L,
                                                9000);
        const bool        wide     =
            (std::numeric_limits<long double>::max_exponent > 9001);
        const m::value    big      = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(1e200),
                                               m::value(1e200) }))));
        const m::value    tiny     = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(1e-200),
                                               m::value(1e-200) }))));
        const m::value    big_f    = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(1e30f),
                                               m::value(1e30f) }))));
        const m::value    big_ld   = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(wide_ld),
                                               m::value(wide_ld) }))));
        const m::value    big_mix  = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(1e200),
                                               1 }))));
        const m::value    nan_big  = d_tests_made(m::expr_norm(
                                         d_tests_made(m::expr_vector(
                                             { m::value(std::numeric_limits<
                                                   double>::quiet_NaN()),
                                               m::value(1e200) }))));
        double            nan_out  = 0.0;

        static_cast<void>(nan_big.to(nan_out));

        D_TESTS_EXPECT( (big.kind() == D_MATH_VALUE_DOUBLE) &&
                        (d_tests_near(big,
                                      1.4142135623730950488e200L,
                                      1e185L)) &&
                        (d_tests_near(tiny,
                                      1.4142135623730950488e-200L,
                                      1e-215L)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(
                                    std::numeric_limits<double>::
                                        denorm_min()) })))),
                            m::value(std::numeric_limits<double>::
                                         denorm_min()))) &&
                        (big_f.kind() == D_MATH_VALUE_FLOAT) &&
                        (d_tests_near(big_f,
                                      1.41421356e30L,
                                      1e24L)) &&
                        ( (!wide) ||
                          ( (big_ld.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                            (d_tests_near(big_ld,
                                          1.4142135623730950488L * wide_ld,
                                          wide_ld * 1e-15L)) ) ) &&
                        (big_mix.kind() == D_MATH_VALUE_DOUBLE) &&
                        (d_tests_near(big_mix,
                                      1e200L,
                                      1e185L)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_norm(d_tests_made(
                                m::expr_vector({ m::value(
                                    std::numeric_limits<double>::
                                        infinity()),
                                                 m::value(1.0) })))),
                            m::value(std::numeric_limits<double>::
                                         infinity()))) &&
                        (nan_out != nan_out),
                        "a norm whose squares pass the type's range, or "
                        "fall below its normal numbers' precision, scaled "
                        "by a power of two, dense or of values, in each "
                        "type: the norm the type holds, not an infinity or "
                        "0; of an infinity an infinity, of a NaN a NaN");

        const std::vector<m::value> exact_elements = {
            m::value::ratio(1, 2), m::value::ratio(1, 3), 1,
            2,                     0,                     m::value::ratio(1, 5),
            1,                     1,                     1 };
        const m::value exact  = d_tests_made(m::expr_matrix(3,
                                                            3,
                                                            exact_elements));
        const m::value right  = d_tests_made(m::expr_matrix(3,
                                                            1,
                                                            { 1, 2, 3 }));
        const m::value inv_a  = d_tests_made(m::expr_inv(exact));
        const m::value x      = d_tests_made(m::expr_solve(exact,
                                                           right));
        const m::value eye    = d_tests_made(m::expr_matrix(3,
                                                            3,
                                                            { 1, 0, 0,
                                                              0, 1, 0,
                                                              0, 0, 1 }));

        D_TESTS_EXPECT( (d_tests_same_values(d_tests_made(m::expr_det(exact)),
                                             d_tests_cofactors(
                                                 exact_elements,
                                                 3))) &&
                        (d_tests_same_values(inv_a * exact,
                                             eye)) &&
                        (d_tests_same_values(exact * inv_a,
                                             eye)) &&
                        (d_tests_same_values(exact * x,
                                             right)) &&
                        (d_tests_all_exact(inv_a)) &&
                        (d_tests_all_exact(x)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_trace(exact)),
                            m::value::ratio(3, 2))),
                        "an exact matrix's det, the test's own cofactors "
                        "exactly; inv and solve exact, A inv(A) = I and "
                        "A x = b; trace exact");

        const m::value singular = d_tests_made(m::expr_matrix(2,
                                                              2,
                                                              { 1, 2,
                                                                2, 4 }));
        const double   flat[4]  = { 1.0, 2.0, 2.0, 4.0 };
        const m::value dense    = d_tests_made(m::expr_matrix(
                                      2,
                                      2,
                                      d_tests_dense_values(flat,
                                                           4)));
        const m::value pair     = d_tests_made(m::expr_vector({ 1, 1 }));

        D_TESTS_EXPECT( (d_tests_same_values(
                            d_tests_made(m::expr_det(singular)),
                            m::value(0))) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_inv(singular)),
                            pole)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_solve(singular,
                                                       pair)),
                            pole)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_inv(dense)),
                            pole)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_solve(dense,
                                                       pair)),
                            pole)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_det(dense)),
                            m::value(0.0))),
                        "a singular matrix's inverse and solution, exact "
                        "or dense, a pole, as an exact number over 0 is; "
                        "its determinant 0");

        const long double two_real = 2.0L;
        const m::value    boxed    = d_tests_made(
                                         m::expr_matrix(1,
                                                        1,
                                                        { d_tests_real_of(
                                                              &two_real) }));
        const m::value    beside   = d_tests_made(
                                         m::expr_matrix(2,
                                                        2,
                                                        { d_tests_real_of(
                                                              &two_real),
                                                          m::value(0.5),
                                                          1,
                                                          1 }));

        D_TESTS_EXPECT( (d_tests_same_values(m::internal::expr_matrix_det(
                                                 boxed),
                                             m::value())) &&
                        (m::internal::expr_matrix_det(beside).kind() !=
                             D_MATH_VALUE_DOUBLE) &&
                        (d_tests_same_values(m::internal::expr_matrix_inverse(
                                                 boxed),
                                             domain)) &&
                        (d_tests_same_values(m::internal::expr_matrix_solve(
                                                 boxed,
                                                 boxed),
                                             domain)),
                        "a boxed inexact element's algebra on the generic "
                        "path, by its own arithmetic, which the test's "
                        "kind gives as none: the determinant none, the "
                        "inverse and the solution a domain error; beside a "
                        "double, on the generic path still, not in double");

        dj_math_value   held    = m::value(2.0L).detach();

        held.flags |= D_MATH_VALUE_FLAG_INEXACT;

        const m::value flagged = d_tests_made(m::expr_matrix(
                                     2,
                                     2,
                                     { m::value::adopt(held),
                                       m::value(1.0L),
                                       m::value(1.0L),
                                       m::value(3.0L) }));
        const m::value column  = d_tests_made(m::expr_matrix(2,
                                                             1,
                                                             { 1, 2 }));
        const unsigned inexact = D_MATH_VALUE_FLAG_INEXACT;
        dj_math_value  held_b  = m::value(2.0L).detach();
        dj_math_value  held_r  = d_tests_real_of(&two_real).detach();

        held_b.flags |= D_MATH_VALUE_FLAG_INEXACT;
        held_r.flags |= D_MATH_VALUE_FLAG_INEXACT;

        const m::value right_f = d_tests_made(m::expr_matrix(
                                     2,
                                     1,
                                     { m::value::adopt(held_b),
                                       m::value(1.0L) }));
        const m::value real_f  = d_tests_made(m::expr_matrix(
                                     1,
                                     1,
                                     { m::value::adopt(held_r) }));

        D_TESTS_EXPECT( ((flagged.flags() & inexact) != 0u) &&
                        ((d_tests_made(m::expr_det(flagged)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_inv(flagged)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_trace(flagged)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_norm(flagged)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_eig(flagged)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_solve(flagged,
                                                     column)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_solve(d_tests_two_by_two(),
                                                     right_f)).flags() &
                          inexact) != 0u) &&
                        ((d_tests_made(m::expr_eig(real_f)).flags() &
                          inexact) != 0u) &&
                        (m::expr_is_matrix(d_tests_made(m::expr_solve(
                            d_tests_two_by_two(),
                            column)))) &&
                        ((d_tests_made(m::expr_solve(d_tests_two_by_two(),
                                                     column)).flags() &
                          inexact) == 0u),
                        "each result carries its operands' flags, a dense "
                        "matrix's of an element flagged inexact (G2), the "
                        "second's of solve, a boxed one's of eig");

        const m::value error_value(D_MATH_VALUE_ERROR_POLE);
        const m::value a = d_tests_two_by_two();
        bool           passed = true;
        const m::value answers[] = {
            m::internal::expr_matrix_det(error_value),
            m::internal::expr_matrix_trace(error_value),
            m::internal::expr_matrix_norm(error_value),
            m::internal::expr_matrix_eig(error_value),
            m::internal::expr_matrix_dot(error_value, m::value(true)),
            m::internal::expr_matrix_cross(a, error_value),
            m::internal::expr_matrix_solve(error_value, a) };
        const m::value refused[] = {
            m::internal::expr_matrix_det(m::value(true)),
            m::internal::expr_matrix_trace(m::value()),
            m::internal::expr_matrix_norm(m::value(false)),
            m::internal::expr_matrix_eig(m::value()),
            m::internal::expr_matrix_dot(a, m::value(true)),
            m::internal::expr_matrix_solve(m::value(true), a),
            m::internal::expr_matrix_solve(a, m::value(2)),
            m::internal::expr_matrix_dot(a, a),
            m::internal::expr_matrix_dot(a, m::value(2)),
            m::internal::expr_matrix_cross(m::value(1), m::value(2)) };

        for (std::size_t i = 0;
             i < (sizeof(answers) / sizeof(answers[0]));
             ++i)
        {
            passed = ( (passed) &&
                       (d_tests_same_values(answers[i],
                                            error_value)) );
        }

        for (std::size_t i = 0;
             i < (sizeof(refused) / sizeof(refused[0]));
             ++i)
        {
            passed = ( (passed) &&
                       (d_tests_same_values(refused[i],
                                            domain)) );
        }

        D_TESTS_EXPECT( passed,
                        "an error passes, the first; a boolean or none, a "
                        "matrix and a number where a function takes two "
                        "of one sort, a matrix dot a matrix and a number's "
                        "cross: a domain error");
        D_TESTS_EXPECT( (d_tests_holds_value(m::expr_det(m::value(true)),
                                             domain)) &&
                        (d_tests_holds_value(m::expr_inv(error_value),
                                             error_value)) &&
                        (d_tests_holds_value(m::expr_eig(m::value()),
                                             domain)) &&
                        (d_tests_holds_value(m::expr_dot(error_value,
                                                         a),
                                             error_value)) &&
                        (d_tests_holds_value(m::expr_cross(a,
                                                           m::value(true)),
                                             domain)) &&
                        (d_tests_holds_value(m::expr_solve(a,
                                                           error_value),
                                             error_value)),
                        "the entries: an operand's error, or a boolean or "
                        "none, the value's own answer, no bad_shape");

        return;
    }

    // d_tests_matrix_eigenvalues
    //   eig as a value: an exact matrix's eigenvalues exact where rational,
    // a repeated one as often as it is repeated, one found as a complex
    // pair's real part among them; irrational ones in long double beside
    // exact ones; a complex pair a domain error; a dense symmetric matrix's
    // by Jacobi, one of entries past the square of the type's range among
    // them, any other's by QR, each in its type, ascending; NaNs for a NaN.
    void
    d_tests_matrix_eigenvalues()
    {
        typedef m::internal::expr_matrix_storage storage;

        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( (d_tests_is_matrix(
                            d_tests_value_of("eig([2, 1; 1, 2])"),
                            1, 2,
                            { 1, 3 },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([1/2, 1/3; 1/3, 1/2])"),
                            1, 2,
                            { m::value::ratio(1, 6), m::value::ratio(5, 6) },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([2, 0, 0; 1, 3, 0; "
                                             "4, 5, 6])"),
                            1, 3,
                            { 2, 3, 6 },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([5])"),
                            1, 1,
                            { 5 },
                            storage::values)),
                        "an exact matrix's rational eigenvalues, exact, "
                        "ascending, a row");
        D_TESTS_EXPECT( (d_tests_is_matrix(
                            d_tests_value_of("eig([2, 1; 0, 2])"),
                            1, 2,
                            { 2, 2 },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([1, 1; -1, 3])"),
                            1, 2,
                            { 2, 2 },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([4, 1, 0; -1, 2, 0; "
                                             "0, 0, 3])"),
                            1, 3,
                            { 3, 3, 3 },
                            storage::values)) &&
                        (d_tests_is_matrix(
                            d_tests_value_of("eig([0, 0; 0, 0])"),
                            1, 2,
                            { 0, 0 },
                            storage::values)),
                        "a repeated eigenvalue as often as it is repeated, "
                        "where it is found as a complex pair's real part "
                        "too");

        const m::value golden = d_tests_value_of("eig([2, 0, 0; 0, 1, 1; "
                                                 "0, 1, 0])");
        const std::vector<m::value> found = d_tests_elements_of(golden);

        D_TESTS_EXPECT( (found.size() == 3) &&
                        (found[0].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (found[1].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_same_values(found[2],
                                             m::value(2))) &&
                        (d_tests_near(found[0],
                                      -0.6180339887498948482L,
                                      1e-17L)) &&
                        (d_tests_near(found[1],
                                      1.6180339887498948482L,
                                      1e-17L)),
                        "irrational eigenvalues in long double, beside "
                        "an exact rational one, ascending");
        const m::value pair = d_tests_value_of("eig([1, 2; -3, 1])");
        m::complex<long double> low;
        m::complex<long double> high;

        static_cast<void>(m::expr_complex_to(d_tests_made(
                                                 m::expr_element(pair, 0, 0)),
                                             low));
        static_cast<void>(m::expr_complex_to(d_tests_made(
                                                 m::expr_element(pair, 0, 1)),
                                             high));

        D_TESTS_EXPECT( (d_tests_same_values(
                            d_tests_value_of("eig([0, -1; 1, 0])"),
                            d_tests_value_of("[-im, im]"))) &&
                        (d_tests_value_of("eig([0, -1; 1, 0])").kind() !=
                             D_MATH_VALUE_ERROR) &&
                        (d_tests_all_exact(
                            d_tests_value_of("eig([0, -1; 1, 0])"))) &&
                        (d_tests_shape(pair) == m::expr_shape(1, 2)) &&
                        (m::expr_is_complex(d_tests_made(
                             m::expr_element(pair, 0, 0)))) &&
                        (low.real() == high.real()) &&
                        (low.imag() == -high.imag()) &&
                        (std::fabs(high.imag() -
                                   2.4494897427831780982L) < 1e-17L) &&
                        (std::fabs(low.real() - 1.0L) < 1e-17L) &&
                        (d_tests_same_values(
                            m::internal::expr_matrix_eig(
                                d_tests_made(m::expr_matrix(
                                    2,
                                    2,
                                    { 0.0, -1.0, 1.0, 0.0 }))),
                            d_tests_made(m::expr_matrix(
                                1,
                                2,
                                { m::expr_complex_of(
                                      m::complex<double>(0.0, -1.0)),
                                  m::expr_complex_of(
                                      m::complex<double>(0.0, 1.0)) })))),
                        "a complex pair: of an exact matrix exact where it "
                        "is a Gaussian rational, -im and im, and a long "
                        "double pair otherwise, exactly conjugate, the "
                        "negative first; of a dense one its type's (S258, "
                        "S273, S352)");

        const m::value symmetric = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 2.0, 1.0, 1.0, 2.0 })));
        const m::value huge      = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 1e307, 1e307,
                                             1e307, 1e307 })));
        const m::value floats    = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 2.0f, 1.0f, 1.0f, 2.0f })));
        const m::value general   = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { 1.0, 2.0, 3.0, 4.0 })));
        const m::value repeated  = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           4,
                                           4,
                                           { 10.0, 8.0, 8.0, 4.0,
                                             8.0, 10.0, 10.0, 8.0,
                                             8.0, 10.0, 10.0, 8.0,
                                             4.0, 8.0, 8.0, 8.0 })));

        D_TESTS_EXPECT( (d_tests_stored_as(symmetric,
                                           storage::doubles)) &&
                        (d_tests_element_near(symmetric,
                                              0,
                                              1.0L,
                                              1e-15L)) &&
                        (d_tests_element_near(symmetric,
                                              1,
                                              3.0L,
                                              1e-15L)) &&
                        (d_tests_stored_as(huge,
                                           storage::doubles)) &&
                        (d_tests_element_near(huge,
                                              0,
                                              0.0L,
                                              1e292L)) &&
                        (d_tests_element_near(huge,
                                              1,
                                              2e307L,
                                              2e292L)) &&
                        (d_tests_stored_as(floats,
                                           storage::floats)) &&
                        (d_tests_element_near(floats,
                                              1,
                                              3.0L,
                                              1e-6L)) &&
                        (d_tests_stored_as(repeated,
                                           storage::doubles)) &&
                        (d_tests_element_near(repeated,
                                              0,
                                              0.0L,
                                              1e-13L)) &&
                        (d_tests_element_near(repeated,
                                              1,
                                              0.0L,
                                              1e-13L)) &&
                        (d_tests_element_near(repeated,
                                              2,
                                              5.1075560105501954925L,
                                              1e-13L)) &&
                        (d_tests_element_near(repeated,
                                              3,
                                              32.89244398944980452L,
                                              1e-13L)),
                        "a dense symmetric matrix's by Jacobi, in its "
                        "type, scaled by a power of two, so that entries "
                        "whose squares pass the type's range are found; "
                        "a repeated eigenvalue, 0 twice, which Hessenberg "
                        "QR finds as a complex pair, real");
        D_TESTS_EXPECT( (d_tests_stored_as(general,
                                           storage::doubles)) &&
                        (d_tests_element_near(general,
                                              0,
                                              -0.3722813232690143L,
                                              1e-14L)) &&
                        (d_tests_element_near(general,
                                              1,
                                              5.3722813232690143L,
                                              1e-14L)),
                        "any other dense matrix's by Hessenberg QR, in "
                        "its type, ascending");

        const m::value wide_eig  = m::internal::expr_matrix_eig(
                                       d_tests_made(m::expr_matrix(
                                           2,
                                           2,
                                           { d_tests_wide_of(
                                                 "100000000000000000000"),
                                             0,
                                             0,
                                             1 })));
        const m::value past      = d_tests_value_of(
                                       "eig([1099511627776, 1; 1, "
                                       "1099511627776])");
        const m::value lcm       = d_tests_value_of(
                                       "eig([0, 1 / 4611686018427387904; "
                                       "0, 1 / 3])");
        const m::value far       = d_tests_value_of(
                                       "eig([9000000000000000000])");
        const std::vector<m::value> wide_values =
            d_tests_elements_of(wide_eig);
        const std::vector<m::value> past_values = d_tests_elements_of(past);
        const std::vector<m::value> lcm_values  = d_tests_elements_of(lcm);
        const std::vector<m::value> far_values  = d_tests_elements_of(far);

        D_TESTS_EXPECT( (wide_values.size() == 2) &&
                        (wide_values[0].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_near(wide_values[1],
                                      1e20L,
                                      1e5L)) &&
                        (past_values.size() == 2) &&
                        (past_values[1].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_near(past_values[1],
                                      1099511627777.0L,
                                      1e-6L)) &&
                        (lcm_values.size() == 2) &&
                        (lcm_values[1].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_near(lcm_values[1],
                                      1.0L / 3.0L,
                                      1e-18L)) &&
                        (far_values.size() == 1) &&
                        (far_values[0].kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (d_tests_near(far_values[0],
                                      9e18L,
                                      1.0L)),
                        "an exact matrix's eigenvalues in long double alone "
                        "where none can be made exact: of a wide integer, "
                        "a characteristic polynomial past the exact "
                        "integers, denominators whose multiple passes them, "
                        "or an eigenvalue whose multiple does");

        const m::value nan = m::internal::expr_matrix_eig(
                                 d_tests_made(m::expr_matrix(
                                     2,
                                     2,
                                     { std::numeric_limits<double>::
                                           quiet_NaN(),
                                       0.0,
                                       0.0,
                                       1.0 })));
        const std::vector<m::value> nans = d_tests_elements_of(nan);
        double                      first = 0.0;
        double                      second = 0.0;

        D_TESTS_EXPECT( (nans.size() == 2) &&
                        (nans[0].to(first)) &&
                        (nans[1].to(second)) &&
                        (first != first) &&
                        (second != second) &&
                        (d_tests_stored_as(nan,
                                           storage::doubles)),
                        "a NaN or an infinity in a dense matrix: NaNs, "
                        "in its type");

        return;
    }

    // d_tests_matrix_negative_powers
    //   A ^ (-n), the inverse's power: exact; a singular matrix's a pole;
    // dense, the C core's inverse then its power; and every dense power the
    // C core's d_mat_power, bit for bit.
    void
    d_tests_matrix_negative_powers()
    {
        typedef m::internal::expr_matrix_storage storage;

        const m::value a = d_tests_two_by_two();

        D_TESTS_EXPECT( (d_tests_same_values(
                            m::internal::expr_matrix_power(a,
                                                           m::value(-1)),
                            d_tests_made(m::expr_inv(a)))) &&
                        (d_tests_is_matrix(
                            m::internal::expr_matrix_power(a,
                                                           m::value(-2)),
                            2, 2,
                            { m::value::ratio(11, 2),
                              m::value::ratio(-5, 2),
                              m::value::ratio(-15, 4),
                              m::value::ratio(7, 4) },
                            storage::values)) &&
                        (d_tests_valued("[1, 1; 1, 0] ^ (-1)",
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            { 0, 1, 1, -1 })))) &&
                        (d_tests_valued("pow([1, 2; 3, 4], -1) = "
                                        "inv([1, 2; 3, 4])",
                                        m::value(true))),
                        "A ^ (-n), the inverse's power, exact, pow(A, -n) "
                        "the same (S259)");

        const m::value pole(D_MATH_VALUE_ERROR_POLE);
        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( (d_tests_valued("[1, 2; 2, 4] ^ (-1)",
                                        pole)) &&
                        (d_tests_valued("[1, 2; 2, 4] ^ (-3)",
                                        pole)) &&
                        (d_tests_valued("[1, 2, 3] ^ (-1)",
                                        domain)),
                        "a singular matrix's negative power a pole; a "
                        "matrix not square's a domain error");

        const double   entries[4] = { 1.0, 2.0, 3.0, 4.0 };
        double         inverse[4] = {};
        double         scratch[4] = {};
        double         squared[4] = {};
        const m::value dense      = d_tests_made(m::expr_matrix(
                                        2,
                                        2,
                                        d_tests_dense_values(entries,
                                                             4)));

        static_cast<void>(d_mat_inverse(inverse,
                                        scratch,
                                        entries,
                                        2));
        d_mat_power(squared,
                    scratch,
                    inverse,
                    2,
                    2);

        D_TESTS_EXPECT( (d_tests_is_matrix(
                            m::internal::expr_matrix_power(dense,
                                                           m::value(-2)),
                            2, 2,
                            d_tests_dense_values(squared,
                                                 4),
                            storage::doubles)) &&
                        (d_tests_is_matrix(
                            m::internal::expr_matrix_power(a,
                                                           m::value(-2.0)),
                            2, 2,
                            d_tests_dense_values(squared,
                                                 4),
                            storage::doubles)),
                        "a dense matrix's negative power, the C core's "
                        "inverse then its power; an exact one to a "
                        "floating-point -n, in its type");

        d_tests_rng rng(20261027u);
        double      raw[16] = {};
        bool        same    = true;

        for (std::size_t i = 0; i < 16; ++i)
        {
            raw[i] = ( static_cast<double>(rng.below(1000u)) / 997.0 ) - 0.5;
        }

        const m::value random = d_tests_made(m::expr_matrix(
                                    4,
                                    4,
                                    d_tests_dense_values(raw,
                                                         16)));

        // each power to 40, as the C core makes it
        for (std::size_t k = 0; k <= 40; ++k)
        {
            double out[16]  = {};
            double work[16] = {};

            d_mat_power(out,
                        work,
                        raw,
                        4,
                        k);

            same = ( (same) &&
                     (d_tests_is_matrix(
                         m::internal::expr_matrix_power(
                             random,
                             m::value(static_cast<long long>(k))),
                         4, 4,
                         d_tests_dense_values(out,
                                              16),
                         storage::doubles)) );
        }

        const double   infinite[4] = { std::numeric_limits<double>::
                                           infinity(),
                                       1.0,
                                       0.0,
                                       1.0 };
        const m::value with_inf    = d_tests_made(m::expr_matrix(
                                         2,
                                         2,
                                         d_tests_dense_values(infinite,
                                                              4)));

        D_TESTS_EXPECT( (same) &&
                        (d_tests_same_values(
                            m::internal::expr_matrix_power(with_inf,
                                                           m::value(1)),
                            with_inf)),
                        "every dense power the C core's d_mat_power, bit "
                        "for bit, each to 40, as both square from the "
                        "exponent's highest bit (S262); A ^ 1 A itself");

        return;
    }

    // d_tests_empty_matrix
    //   the empty matrix (S260): made, read and written; its arithmetic and
    // its functions, each by the rules of shapes; a block that is skipped.
    void
    d_tests_empty_matrix()
    {
        const m::value empty = d_tests_made(m::expr_matrix(0,
                                                           0,
                                                           {}));
        const m::value one   = d_tests_made(m::expr_vector({ 1 }));

        D_TESTS_EXPECT( (m::expr_is_matrix(empty)) &&
                        (d_tests_shape(empty) == m::expr_shape()) &&
                        (d_tests_stored_as(
                            empty,
                            m::internal::expr_matrix_storage::values)) &&
                        (d_tests_same_values(
                            d_tests_made(m::expr_vector({})),
                            empty)) &&
                        (d_tests_kind_text(empty, 64) == "[]") &&
                        (m::expr_render(m::expr_number(empty)) == "[]") &&
                        (empty == empty) &&
                        (empty != one),
                        "the empty matrix: expr_matrix of 0 by 0, and "
                        "expr_vector of none, of values, its text [], "
                        "equal to itself alone");

        const m::expr_term parsed = d_tests_parsed("[]");

        // that [] parsed, asked first: where it does not, parsed is a term
        // made by default, which has no layer to read
        D_TESTS_EXPECT( (m::expr_parse("[]").is_ok()) &&
                        (m::expr_is_application(parsed)) &&
                        (m::expr_op_of(parsed).kind ==
                             m::expr_op_kind::vector) &&
                        (m::expr_operands_of(parsed).empty()) &&
                        (m::expr_render(parsed) == "[]") &&
                        (d_tests_valued("[]", empty)) &&
                        (d_tests_valued("[[]]", empty)) &&
                        (d_tests_is_error(m::expr_evaluate<double>(parsed),
                                          m::expr_status::bad_shape,
                                          m::expr_no_offset,
                                          "expr_evaluate<Type>: a vector or "
                                          "a matrix has no value in a "
                                          "type")),
                        "[], the empty literal: the term vector of no "
                        "element, written back as it is, the empty matrix "
                        "as a value and no value in a type");

        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);
        bool           kept    = true;
        bool           refused = true;
        const char*    empties[] = { "[] + []", "[] - []", "-[]", "2 * []",
                                     "[] / 2", "[] * []", "[]'", "[] ^ 0",
                                     "[] ^ 3", "[] ^ (-1)", "inv([])",
                                     "eig([])", "solve([], [])",
                                     "[[], []]", "[[]; []]" };
        const char*    refusals[] = { "[] + [1]", "[1, 2] * []", "[] * [1]",
                                      "[][1]", "cross([], [])",
                                      "dot([], [1])", "solve([], [1])" };

        for (std::size_t i = 0; i < (sizeof(empties) / sizeof(empties[0]));
             ++i)
        {
            kept = ( (kept) &&
                     (d_tests_valued(empties[i],
                                     empty)) );
        }

        for (std::size_t i = 0;
             i < (sizeof(refusals) / sizeof(refusals[0]));
             ++i)
        {
            refused = ( (refused) &&
                        (d_tests_valued(refusals[i],
                                        domain)) );
        }

        D_TESTS_EXPECT( kept,
                        "its sums, negation, scalar products, product, "
                        "transpose, powers, inverse, eigenvalues and "
                        "solution: the empty matrix");
        D_TESTS_EXPECT( (refused) &&
                        (d_tests_valued("det([])", m::value(1))) &&
                        (d_tests_valued("trace([])", m::value(0))) &&
                        (d_tests_valued("norm([])", m::value(0))) &&
                        (d_tests_valued("dot([], [])", m::value(0))) &&
                        (d_tests_valued("[] = []", m::value(true))) &&
                        (d_tests_valued("[] != [1]", m::value(true))),
                        "its determinant 1, trace, norm and dot product 0; "
                        "a shape of other matrices, an index, and cross of "
                        "it: a domain error");

        return;
    }

    // d_tests_blocks
    //   literals of blocks (S261): read with no shape the parser cannot
    // know; each block beside or above another as a value, its storage and
    // flags; blocks that do not fit, a domain error; written back; never
    // square by structure; no value in a type.
    void
    d_tests_blocks()
    {
        const m::value       a        = d_tests_two_by_two();
        const m::expr_bindings bindings = m::at("A", a).at("x", 1);
        const m::value       domain(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( (m::expr_parse("[A, b]").is_ok()) &&
                        (m::expr_parse("[A; B]").is_ok()) &&
                        (m::expr_parse("[[1, 2], 3; 4, 5, 6]").is_ok()) &&
                        (m::expr_parse("[x, 2; 3]").is_ok()) &&
                        (m::expr_parse("[1, 2; 3, x]").is_ok()) &&
                        (m::expr_parse("[pi, 1; 2]").is_ok()) &&
                        (m::expr_parse("[2 * x, 2; 3]").is_ok()) &&
                        (m::expr_parse("[[1, 2; 3, 4]; 5, 6]").is_ok()) &&
                        (m::expr_parse("[1, 2; A]").is_ok()) &&
                        (d_tests_is_error(m::expr_parse("[1, 2; 3]"),
                                          m::expr_status::bad_shape,
                                          8,
                                          "a row of 1 element, where the "
                                          "first row has 2 elements")) &&
                        (d_tests_is_error(m::expr_parse("[1, 2; x, 1; 3]"),
                                          m::expr_status::bad_shape,
                                          14,
                                          "a row of 1 element, where the "
                                          "first row has 2 elements")) &&
                        (d_tests_is_error(m::expr_parse("[1 + 2, 3!; "
                                                        "-4]"),
                                          m::expr_status::bad_shape,
                                          14,
                                          "a row of 1 element, where the "
                                          "first row has 2 elements")),
                        "a literal whose rows may hold a matrix, of a "
                        "variable, a constant, a literal or an operation "
                        "of one, read whatever its rows' numbers of "
                        "elements; rows of numbers alone of other numbers, "
                        "bad_shape where they end, each row judged anew");
        D_TESTS_EXPECT( (d_tests_valued("[A, A]",
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            4,
                                            { 1, 2, 1, 2, 3, 4, 3, 4 })),
                                        bindings)) &&
                        (d_tests_valued("[A; A]",
                                        d_tests_made(m::expr_matrix(
                                            4,
                                            2,
                                            { 1, 2, 3, 4, 1, 2, 3, 4 })),
                                        bindings)) &&
                        (d_tests_valued("[A, [5; 6]]",
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            3,
                                            { 1, 2, 5, 3, 4, 6 })),
                                        bindings)) &&
                        (d_tests_valued("[[1, 2], 3; 4, 5, 6]",
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            3,
                                            { 1, 2, 3, 4, 5, 6 })))) &&
                        (d_tests_valued("[[1; 2], [3; 4]]",
                                        d_tests_made(m::expr_matrix(
                                            2,
                                            2,
                                            { 1, 3, 2, 4 })))) &&
                        (d_tests_valued("[A, []; [], A]",
                                        d_tests_made(m::expr_matrix(
                                            4,
                                            2,
                                            { 1, 2, 3, 4, 1, 2, 3, 4 })),
                                        bindings)) &&
                        (d_tests_valued("[[1, 2; 3, 4]; 5, 6]",
                                        d_tests_made(m::expr_matrix(
                                            3,
                                            2,
                                            { 1, 2, 3, 4, 5, 6 })))) &&
                        (d_tests_valued("[1, 2; A]",
                                        d_tests_made(m::expr_matrix(
                                            3,
                                            2,
                                            { 1, 2, 1, 2, 3, 4 })),
                                        bindings)) &&
                        (d_tests_valued("[A; []]", a, bindings)) &&
                        (d_tests_valued("[[]; A]", a, bindings)),
                        "blocks as values: a row's beside one another, a "
                        "matrix's rows one above another, a number a block "
                        "of 1 by 1, the empty matrix skipped, beside or "
                        "above");
        D_TESTS_EXPECT( (d_tests_valued("[A, 1]", domain, bindings)) &&
                        (d_tests_valued("[A; 1, 2, 3]", domain, bindings)) &&
                        (d_tests_valued("[x, 2; 3]", domain, bindings)) &&
                        (d_tests_valued("[[1; 2], 2]", domain)) &&
                        (d_tests_valued("[[1, 2]; [3, 4, 5]]", domain)) &&
                        (d_tests_valued("[[1, 2], [3, 4; 5, 6; 7, 8], "
                                        "[9; 10]]",
                                        domain)) &&
                        (d_tests_valued("[[1, 2, 3]; [4]; [5, 6]]",
                                        domain)) &&
                        (d_tests_valued("[A, true]", domain, bindings)) &&
                        (d_tests_valued("[A, 1 / 0]",
                                        m::value(D_MATH_VALUE_ERROR_POLE),
                                        bindings)),
                        "blocks that do not fit, of other rows beside one "
                        "another or other columns one above another, even "
                        "where their elements would fill a shape, and a "
                        "boolean among them: a domain error; an error "
                        "passes, the first");

        typedef m::internal::expr_matrix_storage storage;

        const m::value halves = d_tests_made(m::expr_vector({ 0.5f }));
        const m::value quarter = d_tests_made(m::expr_vector({ 0.25 }));
        const m::value fib100  = m::internal::expr_matrix_power(
                                     d_tests_made(m::expr_matrix(2,
                                                                 2,
                                                                 { 1, 1,
                                                                   1, 0 })),
                                     m::value(100));
        const m::value dense   = d_tests_value_of("[F, Q]",
                                                  m::at("F", halves)
                                                      .at("Q", quarter));
        const m::value mixed   = d_tests_value_of("[Q, 1]",
                                                  m::at("Q", quarter));
        const m::value flagged = d_tests_value_of("[G; G]",
                                                  m::at("G", fib100));

        D_TESTS_EXPECT( (d_tests_is_matrix(dense,
                                           1, 2,
                                           { 0.5, 0.25 },
                                           storage::doubles)) &&
                        (d_tests_stored_as(mixed,
                                           storage::values)) &&
                        ((flagged.flags() & D_MATH_VALUE_FLAG_INEXACT) !=
                             0u) &&
                        (d_tests_shape(flagged) == m::expr_shape(4, 2)),
                        "a matrix of blocks stored as S239 stores any, "
                        "dense where each element is floating, values "
                        "otherwise; its blocks' flags carried");
        D_TESTS_EXPECT( (d_tests_parses("[[1, 2], 3; 4, 5, 6]",
                                        d_tests_parsed("[[1, 2], 3; 4, 5, "
                                                       "6]"))) &&
                        (m::expr_render(d_tests_parsed("[A,[5;6]]")) ==
                             "[A, [5; 6]]") &&
                        (m::expr_render(m::expr_simplify(
                             d_tests_parsed("[[1, 2]] ^ 1"))) ==
                             "[[1, 2]] ^ 1") &&
                        (m::expr_render(m::expr_simplify(
                             d_tests_parsed("[[1; 2], [3; 4]] ^ 1"))) ==
                             "[[1; 2], [3; 4]] ^ 1") &&
                        (d_tests_valued("[[1, 2]] ^ 1", domain)),
                        "a literal of blocks written as it is read, read "
                        "back as itself; never square by its structure, "
                        "so that [[1, 2]] ^ 1, a domain error, stays");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_parsed("[A, b]"),
                                             m::at("A", 1).at("b", 2)),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_evaluate<Type>: a vector or a "
                                         "matrix has no value in a type"),
                        "in a type a literal of blocks has no value, as "
                        "any literal");

        return;
    }

    // d_tests_algebra_typed
    //   whether linear algebra's functions in Type are a scalar's at x = 2:
    // det, trace and eig 2, inv 1/2, norm(-x) 2, dot(x, 3) 6, solve(x, 6)
    // 3, and cross a NaN.
    template<typename Type>
    bool
    d_tests_algebra_typed()
    {
        const m::expr_bindings at = m::at("x", 2);

        return ( (d_tests_typed<Type>("det(x)", Type(2), at)) &&
                 (d_tests_typed<Type>("inv(x)", Type(0.5), at)) &&
                 (d_tests_typed<Type>("trace(x)", Type(2), at)) &&
                 (d_tests_typed<Type>("norm(-x)", Type(2), at)) &&
                 (d_tests_typed<Type>("eig(x)", Type(2), at)) &&
                 (d_tests_typed<Type>("dot(x, 3)", Type(6), at)) &&
                 (d_tests_typed<Type>("solve(x, 6)", Type(3), at)) &&
                 (d_tests_typed<Type>(
                     "cross(x, x)",
                     std::numeric_limits<Type>::quiet_NaN(),
                     at)) );
    }

    // d_tests_fold_algebra
    //   linear algebra's functions in the language: as values, of matrices
    // bound and of literals, and of numbers as scalars; in a type, a
    // scalar's; what each refuses; a call of a number of arguments it does
    // not take, wrong_arity.
    void
    d_tests_fold_algebra()
    {
        const m::value         a        = d_tests_two_by_two();
        const m::value         b        = d_tests_made(m::expr_matrix(
                                              2,
                                              3,
                                              { 1, 2, 3, 4, 5, 6 }));
        const m::value         v        = d_tests_made(m::expr_vector(
                                              { 5, 6, 7 }));
        const m::value         c        = d_tests_made(m::expr_matrix(
                                              2,
                                              3,
                                              { 0.5, 1.0, 1.5,
                                                2.0, 2.5, 3.0 }));
        const m::expr_bindings bindings = m::at("A", a).at("B", b)
                                               .at("C", c).at("v", v)
                                               .at("x", 2);

        D_TESTS_EXPECT( (d_tests_valued("det(A)", m::value(-2), bindings)) &&
                        (d_tests_valued("inv(A) * A = A ^ 0",
                                        m::value(true),
                                        bindings)) &&
                        (d_tests_valued("trace(A)", m::value(5), bindings)) &&
                        (d_tests_valued("norm([3, 4])", m::value(5))) &&
                        (d_tests_valued("dot(v, v)", m::value(110),
                                        bindings)) &&
                        (d_tests_valued("cross(v, [1, 0, 0])",
                                        d_tests_made(m::expr_vector(
                                            { 0, 7, -6 })),
                                        bindings)) &&
                        (d_tests_valued("solve(A, [1, 2])",
                                        d_tests_made(m::expr_vector(
                                            { 0, m::value::ratio(1, 2) })),
                                        bindings)) &&
                        (d_tests_valued("A * solve(A, B) = B",
                                        m::value(true),
                                        bindings)) &&
                        (d_tests_valued("eig([2, 1; 1, 2])",
                                        d_tests_made(m::expr_vector(
                                            { 1, 3 })))),
                        "as values: det, inv, trace, norm, dot, cross, "
                        "solve and eig of matrices bound and of literals, "
                        "exact");
        D_TESTS_EXPECT( (d_tests_valued("det(x)", m::value(2), bindings)) &&
                        (d_tests_valued("inv(x)",
                                        m::value::ratio(1, 2),
                                        bindings)) &&
                        (d_tests_valued("inv(0)",
                                        m::value(D_MATH_VALUE_ERROR_POLE))) &&
                        (d_tests_valued("trace(x)", m::value(2),
                                        bindings)) &&
                        (d_tests_valued("norm(-3)", m::value(3))) &&
                        (d_tests_valued("eig(x)", m::value(2), bindings)) &&
                        (d_tests_valued("dot(2, 3)", m::value(6))) &&
                        (d_tests_valued("solve(2, 6)", m::value(3))) &&
                        (d_tests_valued("solve(2, [4, 6])",
                                        d_tests_made(m::expr_vector(
                                            { 2, 3 })))),
                        "of numbers, scalars, as in Julia: det, trace and "
                        "eig itself, inv the reciprocal, norm the "
                        "magnitude, dot the product, solve(a, B) B / a");
        D_TESTS_EXPECT( (d_tests_algebra_typed<float>()) &&
                        (d_tests_algebra_typed<double>()) &&
                        (d_tests_algebra_typed<long double>()) &&
                        (d_tests_typed<double>(
                            "det(A)",
                            std::numeric_limits<double>::quiet_NaN(),
                            m::at("A", a))),
                        "in a type, a scalar's: det, trace and eig x, inv "
                        "1 / x, norm |x|, dot x y, solve b / a; cross a "
                        "NaN; of a variable bound to a matrix, a NaN");

        const m::value domain(D_MATH_VALUE_ERROR_DOMAIN);
        bool           refused = true;
        const char*    texts[] = { "det(B)", "inv(B)", "trace(v)", "eig(B)",
                                   "dot(v, A)", "dot(v, [1, 2])",
                                   "dot([1, 2], v)",
                                   "cross(v, [1, 2])", "cross(1, 2)",
                                   "solve(B, v)", "solve(A, 1)",
                                   "solve(A, v)", "det(C)", "inv(C)",
                                   "trace(C)", "eig(C)", "det(true)",
                                   "inv(true)",
                                   "norm(1 < 2)", "sqrt(det(A) * A)" };

        for (std::size_t i = 0; i < (sizeof(texts) / sizeof(texts[0])); ++i)
        {
            refused = ( (refused) &&
                        (d_tests_valued(texts[i],
                                        domain,
                                        bindings)) );
        }

        D_TESTS_EXPECT( (refused) &&
                        (d_tests_valued("det([1, 1 / 0])",
                                        m::value(D_MATH_VALUE_ERROR_POLE))),
                        "a shape a function does not take, a boolean, and "
                        "a matrix in any other function: a domain error; "
                        "an argument's error passes");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate(
                                             d_tests_parsed("det(A, A)"),
                                             bindings,
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "det takes 1 argument, not 2") &&
                        d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_parsed("dot(x)"),
                                             m::at("x", 2)),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "dot takes 2 arguments, not 1"),
                        "a call of a number of arguments the function does "
                        "not take: wrong_arity, in both folds");

        return;
    }

    // d_tests_random_square
    //   a random exact square matrix of _n rows: each element a whole number
    // from -3 to 3, a third of them 0, one in five a half of one; upper
    // triangular where _upper.
    std::vector<m::value>
    d_tests_random_square(
        d_tests_rng& _rng,
        std::size_t  _n,
        bool         _upper
    )
    {
        std::vector<m::value> elements;

        for (std::size_t r = 0; r < _n; ++r)
        {
            for (std::size_t c = 0; c < _n; ++c)
            {
                const unsigned zero  = _rng.below(3u);
                const long long whole =
                    static_cast<long long>(_rng.below(7u)) - 3;
                const bool      half  = (_rng.below(5u) == 0);

                elements.push_back( ( ( (_upper) && (c < r) ) ||
                                      (zero == 0) )
                                        ? m::value(0)
                                        : ( half ? m::value::ratio(whole, 2)
                                                 : m::value(whole) ) );
            }
        }

        return elements;
    }

    // d_tests_algebra_random
    //   linear algebra's functions on 1,000 random exact matrices of 1 to 4
    // rows, against the test's own arithmetic: det the cofactor expansion,
    // exactly; inv and solve exact, A inv(A) = I and A x = b, a pole where
    // det is 0; trace the diagonal's sum; each exact eigenvalue a root of
    // det(A - x I), exactly, and a real spectrum's sum the trace; and a
    // random upper triangular matrix's eigenvalues its diagonal, exact and
    // ascending (S263).
    void
    d_tests_algebra_random()
    {
        d_tests_rng       rng(20261028u);
        const std::size_t count      = 1000;
        std::size_t       determined = 0;
        std::size_t       inverted   = 0;
        std::size_t       solved     = 0;
        std::size_t       singular   = 0;
        std::size_t       traced     = 0;
        std::size_t       rooted     = 0;
        std::size_t       complex_spectra = 0;
        std::size_t       triangular = 0;

        for (std::size_t i = 0; i < count; ++i)
        {
            const std::size_t           n = 1 + rng.below(4u);
            const std::vector<m::value> elements =
                d_tests_random_square(rng,
                                      n,
                                      false);
            const m::value              a = d_tests_made(
                                                m::expr_matrix(n,
                                                               n,
                                                               elements));
            std::vector<m::value>       right;
            std::vector<m::value>       identity;
            m::value                    diagonal(0);

            for (std::size_t r = 0; r < n; ++r)
            {
                right.push_back(m::value(static_cast<long long>(r) + 1));
                diagonal = diagonal + elements[(r * n) + r];

                for (std::size_t c = 0; c < n; ++c)
                {
                    identity.push_back(m::value( (r == c) ? 1 : 0 ));
                }
            }

            const m::value b   = d_tests_made(m::expr_matrix(n,
                                                             1,
                                                             right));
            const m::value eye = d_tests_made(m::expr_matrix(n,
                                                             n,
                                                             identity));
            const m::value det = d_tests_cofactors(elements,
                                                   n);
            const m::value inv = d_tests_made(m::expr_inv(a));
            const m::value x   = d_tests_made(m::expr_solve(a,
                                                            b));

            determined += ( d_tests_same_values(d_tests_made(m::expr_det(a)),
                                                det) ? 1 : 0 );
            traced     += ( d_tests_same_values(
                                d_tests_made(m::expr_trace(a)),
                                diagonal) ? 1 : 0 );

            // invertible: exact; singular: a pole
            if (det == m::value(0))
            {
                const m::value pole(D_MATH_VALUE_ERROR_POLE);

                singular += ( ( (d_tests_same_values(inv,
                                                     pole)) &&
                                (d_tests_same_values(x,
                                                     pole)) ) ? 1 : 0 );
            }
            else
            {
                inverted += ( ( (d_tests_same_values(inv * a,
                                                     eye)) &&
                                (d_tests_all_exact(inv)) ) ? 1 : 0 );
                solved   += ( d_tests_same_values(a * x,
                                                  b) ? 1 : 0 );
            }

            const m::value              eig    = d_tests_made(
                                                     m::expr_eig(a));
            const std::vector<m::value> values = d_tests_elements_of(eig);
            bool                        roots  = true;
            long double                 sum    = 0.0L;
            long double                 turn   = 0.0L;
            long double                 trace  = 0.0L;
            bool                        pairs  = false;

            // each exact eigenvalue a root; the spectrum's sum the trace;
            // by real part, then by imaginary part (S352)
            for (std::size_t k = 0; k < values.size(); ++k)
            {
                m::complex<long double> part;
                std::vector<m::value>   shifted = elements;

                for (std::size_t r = 0; r < n; ++r)
                {
                    shifted[(r * n) + r] = shifted[(r * n) + r] - values[k];
                }

                roots = ( (roots) &&
                          ( (!values[k].is_exact()) ||
                            (d_tests_cofactors(shifted,
                                               n) == m::value(0)) ) &&
                          ( (k == 0) ||
                            (!m::internal::expr_eigenvalue_before(
                                 values[k],
                                 values[k - 1])) ) &&
                          (m::expr_complex_to(values[k],
                                              part)) );
                pairs = ( (pairs) ||
                          (m::expr_is_complex(values[k])) );
                sum  += part.real();
                turn += part.imag();
            }

            static_cast<void>(diagonal.to(trace));

            complex_spectra += ( pairs ? 1 : 0 );
            rooted += ( ( (eig.kind() != D_MATH_VALUE_ERROR) &&
                          (values.size() == n) &&
                          (roots) &&
                          (std::fabs(sum - trace) <=
                               (1e-12L * (1.0L + std::fabs(trace)))) &&
                          (std::fabs(turn) <=
                               (1e-12L * (1.0L + std::fabs(trace)))) )
                            ? 1 : 0 );

            // upper triangular: the diagonal, exact and ascending
            const std::vector<m::value> upper = d_tests_random_square(rng,
                                                                      n,
                                                                      true);
            std::vector<m::value>       sorted;

            for (std::size_t r = 0; r < n; ++r)
            {
                sorted.push_back(upper[(r * n) + r]);
            }

            std::sort(sorted.begin(),
                      sorted.end(),
                      [](const m::value& _x,
                         const m::value& _y) -> bool
                      {
                          return (_x < _y);
                      });

            triangular += ( d_tests_same_values(
                                d_tests_made(m::expr_eig(d_tests_made(
                                    m::expr_matrix(n,
                                                   n,
                                                   upper)))),
                                d_tests_made(m::expr_vector(sorted)))
                                ? 1 : 0 );
        }

        D_TESTS_EXPECT( (determined == count) &&
                        (traced == count),
                        "1,000 random exact matrices: det the test's own "
                        "cofactor expansion, and trace the diagonal's sum, "
                        "exactly");
        D_TESTS_EXPECT( (inverted + singular == count) &&
                        (singular > (count / 20)) &&
                        (solved == inverted),
                        "inv and solve exact, A inv(A) = I and A x = b, "
                        "each invertible matrix's; a singular one's a pole, "
                        "some of them");
        D_TESTS_EXPECT( (rooted == count) &&
                        (complex_spectra > (count / 20)),
                        "each exact eigenvalue a root of det(A - x I), "
                        "exactly, by real part, then by imaginary part; the "
                        "spectrum's sum the trace; complex pairs among them "
                        "(S352)");
        D_TESTS_EXPECT( (triangular == count),
                        "a random upper triangular matrix's eigenvalues "
                        "its diagonal, exact and ascending");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_ALGEBRA_HPP
