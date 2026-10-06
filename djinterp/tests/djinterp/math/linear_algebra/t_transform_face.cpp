/*******************************************************************************
* djinterp [test]                                           t_transform_face.cpp
*
*   Tests for linear_algebra/transform.hpp, at every level from C++98: each
* rotation's entries are the C library's cosine and sine of the angle, bit
* for bit, in float, double and long double; rotations compose and invert
* to rounding and are orthogonal; rotation_axis about z is rotation_z; the
* homogeneous embedding, translations, points and directions; the
* perspective divide, w = 0 included; integer transforms (the generic
* path); from C++11 the geometry bridge (to_array as a constant expression);
* from C++14 the angle-free builders as constant expressions, and from C++20
* a rotation.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/linear_algebra/t_transform_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/math/linear_algebra/t_transform_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.05
*                                                            revised: 2026.10.05
*******************************************************************************/
#include "../../../../inc/djinterp/math/linear_algebra/transform.hpp"  // test
// djinterp
#include "../../../../inc/djinterp/math/linear_algebra/square.hpp"  // is_
                                                                    // ortho-
                                                                    // gonal
// std
#include <cmath>    // std::cos, std::sin
#include <cstdio>   // std::printf


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// to_array is a constant expression from C++11 (std::array's indexing is
// one only from C++14)
constexpr std::array<double, 2> g_bridged = djinterp::math::linalg::to_array(
    djinterp::math::linalg::vector<double, 2>());
#endif


namespace
{
    using djinterp::math::linalg::matrix;
    using djinterp::math::linalg::vector;

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

    template<typename Float>
    bool
    d_tests_same(Float _a, Float _b)
    {
        volatile Float a = _a;
        volatile Float b = _b;

        return (a == b);
    }

    unsigned long g_state = 123456789u;

    double
    d_tests_random()
    {
        g_state = (g_state * 1103515245u) + 12345u;

        return (static_cast<double>((g_state >> 4) % 2000003u) / 250000.0) -
               4.0;
    }

    template<typename Type>
    void
    d_tests_family(Type _tolerance)
    {
        typedef matrix<Type, 3, 3> m3;

        for (int trial = 0; trial < 3000; ++trial)
        {
            const Type a = static_cast<Type>(d_tests_random());
            const Type b = static_cast<Type>(d_tests_random());
            const Type c = std::cos(a);
            const Type s = std::sin(a);

            // the entries are the library's cosine and sine, bit for bit
            const matrix<Type, 2, 2> r2 =
                djinterp::math::linalg::rotation_2d(a);

            D_TESTS_EXPECT( (d_tests_same(r2(0, 0), c)) &&
                            (d_tests_same(r2(0, 1), static_cast<Type>(-s))) &&
                            (d_tests_same(r2(1, 0), s)) &&
                            (d_tests_same(r2(1, 1), c)),
                            "rotation_2d is the library's cosine and sine");

            const m3 rz = djinterp::math::linalg::rotation_z(a);
            const m3 rx = djinterp::math::linalg::rotation_x(a);
            const m3 ry = djinterp::math::linalg::rotation_y(a);

            D_TESTS_EXPECT( (rz(0, 0) == r2(0, 0)) &&
                            (rz(1, 0) == r2(1, 0)) &&
                            (rz(2, 2) == static_cast<Type>(1)),
                            "rotation_z is rotation_2d in the xy plane");
            D_TESTS_EXPECT( (rx(1, 2) == -s) &&
                            (ry(0, 2) == s) &&
                            (ry(2, 0) == -s),
                            "rotation_x and rotation_y's layouts");

            // composition and inversion, to rounding
            D_TESTS_EXPECT(rz.times(djinterp::math::linalg::rotation_z(b))
                               .equals(djinterp::math::linalg::rotation_z(
                                           static_cast<Type>(a + b)),
                                       _tolerance),
                           "R(a) R(b) = R(a + b)");
            D_TESTS_EXPECT(rx.times(djinterp::math::linalg::rotation_x(
                                        static_cast<Type>(-a)))
                               .is_identity(_tolerance),
                           "R(a) R(-a) = I");
            D_TESTS_EXPECT(djinterp::math::linalg::is_orthogonal(
                               djinterp::math::linalg::rotation_axis(
                                   vector<Type, 3>(1, 2, 3), a),
                               _tolerance),
                           "an axis rotation is orthogonal");
            D_TESTS_EXPECT(djinterp::math::linalg::rotation_axis(
                               vector<Type, 3>(0, 0, 5), a)
                               .equals(rz, _tolerance),
                           "rotation_axis about z is rotation_z");
        }

        // homogeneous transforms: a rotation and an offset, a point and a
        // direction
        const vector<Type, 2> offset(3, 4);
        const matrix<Type, 3, 3> t = djinterp::math::linalg::homogeneous(
            djinterp::math::linalg::scaling_2d(static_cast<Type>(2)),
            offset);

        D_TESTS_EXPECT( (djinterp::math::linalg::transform_point(
                             t, vector<Type, 2>(1, 1)) ==
                         vector<Type, 2>(5, 6)) &&
                        (djinterp::math::linalg::transform_direction(
                             t, vector<Type, 2>(1, 1)) ==
                         vector<Type, 2>(2, 2)),
                        "a point moves, a direction does not");
        D_TESTS_EXPECT( (djinterp::math::linalg::from_homogeneous(
                             vector<Type, 3>(2, 4, 2)) ==
                         vector<Type, 2>(1, 2)) &&
                        (djinterp::math::linalg::from_homogeneous(
                             vector<Type, 3>(2, 4, 0)) ==
                         vector<Type, 2>(2, 4)),
                        "the perspective divide, w = 0 included");
        D_TESTS_EXPECT(djinterp::math::linalg::translation_3d(
                           static_cast<Type>(1),
                           static_cast<Type>(2),
                           static_cast<Type>(3))(2, 3) ==
                       static_cast<Type>(3),
                       "translation_3d");

        return;
    }

    void
    d_tests_integers()
    {
        const matrix<int, 3, 3> t = djinterp::math::linalg::homogeneous(
            djinterp::math::linalg::scaling_2d(3),
            vector<int, 2>(1, -1));

        D_TESTS_EXPECT( (djinterp::math::linalg::transform_direction(
                             t, vector<int, 2>(2, 5)) ==
                         vector<int, 2>(6, 15)) &&
                        (djinterp::math::linalg::to_homogeneous(
                             vector<int, 2>(7, 8))[2] == 1) &&
                        (djinterp::math::linalg::translation_2d(4, 5)(1, 2)
                             == 5),
                        "integer transforms");

        return;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    void
    d_tests_bridge()
    {
        const vector<double, 3>     v(1, 2, 3);
        const std::array<double, 3> a = djinterp::math::linalg::to_array(v);

        (void)g_bridged;

        D_TESTS_EXPECT( (a[2] == 3.0) &&
                        (djinterp::math::linalg::to_vector(a) == v),
                        "the geometry bridge round-trips");

        return;
    }
#endif
}  // namespace

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
static_assert(g_bridged[1] == 0.0, "to_array's result, at compile time");
#endif

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
static_assert(djinterp::math::linalg::transform_point(
                  djinterp::math::linalg::translation_2d(1.0, 2.0),
                  vector<double, 2>(3.0, 4.0))[1] == 6.0,
              "the angle-free builders are constant expressions from C++14");
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
static_assert(djinterp::math::linalg::rotation_z(0.0).is_identity(0.0),
              "a rotation is a constant expression from C++20");
#endif


int
main()
{
    d_tests_family<float>(1e-4f);
    d_tests_family<double>(1e-12);
    d_tests_family<long double>(1e-12L);
    d_tests_integers();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_bridge();
#endif

    std::printf("t_transform_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
