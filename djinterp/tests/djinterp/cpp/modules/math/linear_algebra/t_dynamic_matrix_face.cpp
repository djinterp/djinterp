/*******************************************************************************
* djinterp [cpp/math]                                  t_dynamic_matrix_face.cpp
*
*   Tests for linear_algebra/dynamic_matrix.hpp, at every level from C++98,
* per family and for int: shapes chosen at run time -- construction,
* element access, rows and columns; the arithmetic and products against
* the fixed-size matrix's, element for element; the trace, norms and
* powers; the round trip through the fixed-size type; the operators,
* scalar products of either order included.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/linear_algebra/t_dynamic_matrix_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/linear_algebra/t_dynamic_matrix_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.07
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/linear_algebra/dynamic_matrix.hpp"
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
}  // namespace


int
main()
{
    d_tests_family<float>();
    d_tests_family<double>();
    d_tests_family<long double>();
    d_tests_family<int>();

    std::printf("t_dynamic_matrix_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
