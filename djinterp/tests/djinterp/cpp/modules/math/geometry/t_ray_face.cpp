/*******************************************************************************
* djinterp [cpp/math]                                             t_ray_face.cpp
*
*   Tests for geometry/ray.hpp, at every level from C++98: points along a
* ray; the box test as the C core's -- the answer and the parameters -- for
* corners and for an aabb; the triangle test likewise; an integer ray
* through double; from C++11 std::array corners and vertices, and from C++14
* the tests as constant expressions.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/cpp/modules/math/geometry/t_ray_face.cpp -o t_ray -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_ray_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.11.07
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/ray.hpp"  // tested
// std
#include <cstdio>   // std::printf


namespace
{
    namespace dm = djinterp::math;

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

    unsigned long g_state = 4242u;

    double
    d_tests_random()
    {
        g_state = (g_state * 1103515245u) + 12345u;

        return (static_cast<double>((g_state >> 4) % 2000003u) / 250000.0) -
               4.0;
    }

    template<typename T>
    void
    d_tests_family()
    {
        typedef dm::ray<T>                ray_t;
        typedef typename ray_t::point_type point;

        for (int trial = 0; trial < 3000; ++trial)
        {
            T e[15];

            for (int i = 0; i < 15; ++i)
            {
                e[i] = static_cast<T>(d_tests_random());
            }

            const point o(e[0], e[1], e[2]);
            const point d(e[3], e[4], e[5]);
            const point lo(e[6] < e[9] ? e[6] : e[9],
                           e[7] < e[10] ? e[7] : e[10],
                           e[8] < e[11] ? e[8] : e[11]);
            const point hi(e[6] < e[9] ? e[9] : e[6],
                           e[7] < e[10] ? e[10] : e[7],
                           e[8] < e[11] ? e[11] : e[8]);
            const ray_t r(o, d);
            T           tn = T(), tf = T(), wn = T(), wf = T();

            // the face's answer and parameters are the C core's
            const bool hit  = dm::intersect_aabb(r, lo, hi, tn, tf);
            const bool want = d_isect_ray_aabb(o.data(), d.data(), lo.data(),
                                               hi.data(), &wn, &wf);

            D_TESTS_EXPECT( (hit == want) &&
                            ((!hit) || ((tn == wn) && (tf == wf))),
                            "intersect_aabb is the C core's");
            D_TESTS_EXPECT(dm::intersect_aabb(r, dm::aabb<3, T>(lo, hi), wn,
                                              wf) == hit,
                           "intersect_aabb on an aabb");

            // the triangle likewise
            const point v1(e[12], e[13], e[14]);
            T           t = T(), w = T();

            D_TESTS_EXPECT(dm::intersect_triangle(r, lo, hi, v1, t) ==
                           d_isect_ray_triangle(o.data(), d.data(),
                                                lo.data(), hi.data(),
                                                v1.data(),
                                                static_cast<T>(1e-9), &w),
                           "intersect_triangle is the C core's");

            // a point along the ray
            D_TESTS_EXPECT(r.at(static_cast<T>(2)) == o.plus(d.scaled(2)),
                           "at");
        }

        return;
    }

    void
    d_tests_generic()
    {
        // an integer ray, through double: from (-5, 0, 0) along +x into the
        // unit box
        const dm::ray<int> r(dm::ray<int>::point_type(-5, 0, 0),
                             dm::ray<int>::point_type(1, 0, 0));
        int                tn = 0;
        int                tf = 0;

        D_TESTS_EXPECT( (dm::intersect_aabb(r,
                                            dm::ray<int>::point_type(-1, -1,
                                                                     -1),
                                            dm::ray<int>::point_type(1, 1, 1),
                                            tn,
                                            tf)) &&
                        (tn == 4) &&
                        (tf == 6) &&
                        (r.at(3)[0] == -2),
                        "an int ray");

        return;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    void
    d_tests_cpp11()
    {
        const dm::ray<double>       r({{ -5.0, 0.25, 0.25 }},
                                      {{ 1.0, 0.0, 0.0 }});
        const std::array<double, 3> lo = {{ -1.0, -1.0, -1.0 }};
        const std::array<double, 3> hi = {{ 1.0, 1.0, 1.0 }};
        const std::array<double, 3> v0 = {{ 0.0, 0.0, 0.0 }};
        const std::array<double, 3> v1 = {{ 0.0, 1.0, 0.0 }};
        const std::array<double, 3> v2 = {{ 0.0, 0.0, 1.0 }};
        double                      tn = 0.0;
        double                      tf = 0.0;
        double                      t  = 0.0;

        // std::array corners and vertices, as ui's picking passes them
        D_TESTS_EXPECT( (dm::intersect_aabb(r, lo, hi, tn, tf)) &&
                        (tn == 4.0) &&
                        (tf == 6.0) &&
                        (dm::intersect_triangle(r, v0, v1, v2, t)) &&
                        (t == 5.0),
                        "std::array corners and vertices");

        return;
    }
#endif
}  // namespace

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
constexpr double
d_tests_entry()
{
    const djinterp::math::ray<double> r(
        djinterp::math::ray<double>::point_type(-3.0, 0.0, 0.0),
        djinterp::math::ray<double>::point_type(1.0, 0.0, 0.0));
    double tn = 0.0;
    double tf = 0.0;

    return djinterp::math::intersect_aabb(
               r,
               djinterp::math::ray<double>::point_type(-1.0, -1.0, -1.0),
               djinterp::math::ray<double>::point_type(1.0, 1.0, 1.0),
               tn,
               tf)
               ? tn
               : -1.0;
}

static_assert(d_tests_entry() == 2.0,
              "a box test as a constant expression, from C++14");
#endif


int
main()
{
    d_tests_family<float>();
    d_tests_family<double>();
    d_tests_family<long double>();
    d_tests_generic();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_cpp11();
#endif

    std::printf("t_ray_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
