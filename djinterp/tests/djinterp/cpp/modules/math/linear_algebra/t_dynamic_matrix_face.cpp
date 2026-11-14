/*******************************************************************************
* djinterp [cpp/math]                                  t_dynamic_matrix_face.cpp
*
*   Tests for linear_algebra/dynamic_matrix.hpp, at every level from C++98,
* per family and for int: shapes chosen at run time -- construction,
* element access, rows and columns; the arithmetic and products against
* the fixed-size matrix's, element for element; the trace, norms and
* powers; the round trip through the fixed-size type; the operators,
* scalar products of either order included. And of every other element
* type the fixed-size matrix takes (S354): math::complex<T> of float and
* double, math::value and math::inline_value -- the arithmetic, products,
* trace, norms, a complex matrix's Frobenius norm a T, identity, powers,
* zeros and equality with a tolerance of the traits' magnitude type, each
* the fixed-size matrix's.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*     tests/djinterp/cpp/modules/math/linear_algebra/t_dynamic_matrix_face.cpp \
*     -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/linear_algebra/t_dynamic_matrix_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/linear_algebra/dynamic_matrix.hpp"
// djinterp
#include "../../../../../../inc/djinterp/cpp/modules/math/complex.hpp"  // complex<T>
#include "../../../../../../inc/djinterp/cpp/modules/math/value.hpp"    // value
// std
#include <cstdio>   // std::printf
#include <vector>   // std::vector


namespace
{
    namespace la = djinterp::math::linalg;

    long g_checks   = 0;
    long g_failures = 0;

    void
    d_tests_expect(
        bool        _ok,
        const char* _what,
        int         _line
    )
    {
        ++g_checks;

        // report the first few failures with what was checked, and where
        if (!_ok)
        {
            ++g_failures;

            if (g_failures <= 40)
            {
                std::printf("  FAIL line %d: %s\n", _line, _what);
            }
        }

        return;
    }

    #define D_TESTS_EXPECT(ok, what)                                          \
        d_tests_expect((ok), (what), __LINE__)

    template<typename T>
    void
    d_tests_family()
    {
        typedef la::dynamic_matrix<T> dm;
        typedef la::matrix<T, 2, 3>   f23;
        typedef la::matrix<T, 3, 2>   f32;

        const T  ae[6] = { 1, 2, 3, 4, 5, 6 };
        const T  be[6] = { 6, 5, 4, 3, 2, 1 };
        const f23 fa(ae);
        const f23 fb(be);
        const dm  a = dm::from_fixed(fa);
        const dm  b = dm::from_fixed(fb);

        D_TESTS_EXPECT( (a.rows() == 2) &&
                        (a.cols() == 3) &&
                        (a(1, 2) == T(6)) &&
                        (a.row(1)[0] == T(4)) &&
                        (a.col(2)[0] == T(3)),
                        "construction and access");
        // (to_fixed named through template: a is of a dependent type)
        D_TESTS_EXPECT( (a.plus(b).template to_fixed<2, 3>() ==
                         fa.plus(fb)) &&
                        (a.minus(b).template to_fixed<2, 3>() ==
                         fa.minus(fb)) &&
                        (a.scaled(T(3)).template to_fixed<2, 3>() ==
                         fa.scaled(T(3))) &&
                        (a.negated().template to_fixed<2, 3>() ==
                         fa.negated()) &&
                        (a.transposed().template to_fixed<3, 2>() ==
                         fa.transposed()),
                        "the arithmetic is the fixed-size type's");

        const f32 fc = fb.transposed();
        const dm  c  = dm::from_fixed(fc);

        // (the norms stored before they are compared: on an x87 a returned
        // value may carry excess precision until it is)
        volatile T dn = a.norm();
        volatile T fn = fa.norm();

        D_TESTS_EXPECT( (a.times(c).template to_fixed<2, 2>() ==
                         fa.times(fc)) &&
                        (a.times(c).trace() == fa.times(fc).trace()) &&
                        (a.norm_squared() == fa.norm_squared()) &&
                        (dn == fn),
                        "products, trace and norms");

        std::vector<T> v(3, T(1));
        const std::vector<T> av = a.times(v);

        D_TESTS_EXPECT( (av.size() == 2) &&
                        (av[0] == T(6)) &&
                        (av[1] == T(15)),
                        "a matrix-vector product");

        const dm sq = a.times(c);

        D_TESTS_EXPECT( (sq.power(0) == dm::identity(2)) &&
                        (sq.power(2) == sq.times(sq)) &&
                        ((a + b) == a.plus(b)) &&
                        ((a * T(2)) == (T(2) * a)) &&
                        ((-a) == a.negated()) &&
                        (a != b),
                        "powers and the operators");

        return;
    }

    // d_tests_element
    //   S354: a dynamic matrix of an element type the fixed-size matrix
    // takes, the six entries of _a and _b, each operation the fixed-size
    // type's, entry for entry.
    template<typename T>
    void
    d_tests_element(
        const T (&_a)[6],
        const T (&_b)[6],
        const char* _name
    )
    {
        typedef la::dynamic_matrix<T> dm;
        typedef la::matrix<T, 2, 3>   f23;
        typedef la::matrix<T, 3, 2>   f32;
        typedef la::matrix<T, 2, 2>   f22;

        const f23 fa(_a);
        const f23 fb(_b);
        const f32 fc = fb.transposed();
        const dm  a  = dm::from_fixed(fa);
        const dm  b  = dm::from_fixed(fb);
        const dm  c  = dm::from_fixed(fc);
        const dm  sq = a.times(c);

        D_TESTS_EXPECT( (a.plus(b).template to_fixed<2, 3>() ==
                         fa.plus(fb)) &&
                        (a.minus(b).template to_fixed<2, 3>() ==
                         fa.minus(fb)) &&
                        (a.scaled(_a[1]).template to_fixed<2, 3>() ==
                         fa.scaled(_a[1])) &&
                        (a.negated().template to_fixed<2, 3>() ==
                         fa.negated()) &&
                        (a.transposed().template to_fixed<3, 2>() ==
                         fa.transposed()) &&
                        (sq.template to_fixed<2, 2>() == fa.times(fc)) &&
                        (sq.trace() == fa.times(fc).trace()) &&
                        (a.norm_squared() == fa.norm_squared()) &&
                        (a.row(1)[2] == _a[5]) &&
                        (a.col(0)[1] == _a[3]),
                        _name);
        D_TESTS_EXPECT( (dm::identity(2).template to_fixed<2, 2>() ==
                         f22::identity()) &&
                        (sq.power(0) == dm::identity(2)) &&
                        (sq.power(2) == sq.times(sq)) &&
                        (dm(2, 3).template to_fixed<2, 3>() == f23()) &&
                        (a.equals(a)) &&
                        (!a.equals(b)) &&
                        (la::approx_equal(a, a,
                                          djinterp::math::number_traits<T>::
                                              magnitude(_a[0] - _a[0]))) &&
                        ((a * 2) == (2 * a)) &&
                        ((a * 2) == a.scaled(T(2))),
                        _name);

        return;
    }

    // d_tests_elements
    //   S354: complex numbers of float and double, values and inline
    // values; a complex matrix's Frobenius norm a T, the fixed-size one's.
    void
    d_tests_elements()
    {
        typedef djinterp::math::complex<double> cd;
        typedef djinterp::math::complex<float>  cf;
        typedef djinterp::math::value           v;
        typedef djinterp::math::inline_value    iv;

        const cd zd_a[6] = { cd(1.0, 2.0), cd(3.0, -1.0), cd(0.5, 0.0),
                             cd(-2.0, 1.5), cd(4.0, 4.0), cd(1.0, -3.0) };
        const cd zd_b[6] = { cd(2.0, -1.0), cd(0.0, 1.0), cd(-1.5, 2.0),
                             cd(1.0, 1.0), cd(-3.0, 0.5), cd(2.0, 2.0) };
        const cf zf_a[6] = { cf(1.0f, 2.0f), cf(3.0f, -1.0f), cf(0.5f, 0.0f),
                             cf(-2.0f, 1.5f), cf(4.0f, 4.0f), cf(1.0f, -3.0f) };
        const cf zf_b[6] = { cf(2.0f, -1.0f), cf(0.0f, 1.0f), cf(-1.5f, 2.0f),
                             cf(1.0f, 1.0f), cf(-3.0f, 0.5f), cf(2.0f, 2.0f) };
        const v  x_a[6]  = { v(1), v::ratio(1, 3), v(2.5), v(-2), v(4),
                             v::ratio(-7, 2) };
        const v  x_b[6]  = { v(2), v(0), v::ratio(5, 4), v(1), v(-3), v(2) };
        const iv i_a[6]  = { iv(1), iv(2.5), iv(-3), iv(0.25), iv(4), iv(-1) };
        const iv i_b[6]  = { iv(2), iv(1), iv(-1.5), iv(3), iv(0), iv(2) };

        d_tests_element<cd>(zd_a, zd_b,
                            "a dynamic matrix of complex doubles: each "
                            "operation the fixed-size matrix's (S354)");
        d_tests_element<cf>(zf_a, zf_b,
                            "of complex floats: the fixed-size matrix's");
        d_tests_element<v>(x_a, x_b,
                           "of values, exact and inexact: the fixed-size "
                           "matrix's, exactly where they are exact");
        d_tests_element<iv>(i_a, i_b,
                            "of inline values: the fixed-size matrix's");

        const la::dynamic_matrix<cd> a = la::dynamic_matrix<cd>::from_fixed(
            la::matrix<cd, 2, 3>(zd_a));
        volatile double dn = la::frobenius_norm(a);
        volatile double fn = la::frobenius_norm(la::matrix<cd, 2, 3>(zd_a));

        D_TESTS_EXPECT( (dn == fn) &&
                        (la::frobenius_norm(la::dynamic_matrix<cd>()) ==
                             0.0),
                        "a complex matrix's Frobenius norm a double, the "
                        "C core's scaled norm of its parts, the fixed-size "
                        "matrix's; of the empty matrix 0");

        return;
    }
}  // namespace


int
main()
{
    d_tests_family<float>();
    d_tests_family<double>();
    d_tests_family<long double>();
    d_tests_family<int>();
    d_tests_elements();

    std::printf("t_dynamic_matrix_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
