/*******************************************************************************
* djinterp [cpp/math]                                      t_measure_2d_face.cpp
*
*   Tests for geometry/measure_2d.hpp, at every level from C++98 -- the
* header compiled at no level before, missing named_2d.hpp: a segment's
* length (closed form) against a straight Bezier's (Simpson's rule); a
* circle's perimeter, area and centroid; a circle's area by Green's theorem
* against its closed form; a polygon's and a circle's bounding boxes.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/geometry/t_measure_2d_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_measure_2d_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/measure_2d.hpp"  // tested
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

    template<typename Float>
    bool
    d_tests_near(Float _a, Float _b, Float _tol)
    {
        const Float d = _a - _b;

        return ( (d <= _tol) &&
                 (d >= -_tol) );
    }

    template<typename T>
    void
    d_tests_family(T _tol)
    {
        typedef dm::cartesian<2, T>      car;
        typedef typename car::point_type p2;

        const p2 a(0, 0);
        const p2 b(3, 4);

        // a straight Bezier's length by Simpson's rule is the segment's
        D_TESTS_EXPECT( (dm::curve_length(dm::line_segment<car>(a, b)) ==
                         static_cast<T>(5)) &&
                        (d_tests_near(dm::curve_length(
                                          dm::quadratic_bezier<car>(
                                              a,
                                              p2(static_cast<T>(1.5), 2),
                                              b)),
                                      static_cast<T>(5), _tol * 100)),
                        "lengths, closed and by Simpson's rule");

        const dm::circle<car> c(p2(1, 2), 3);

        // (near: 2 pi r and pi r r round where 6 pi and 9 pi round once)
        D_TESTS_EXPECT( (d_tests_near(dm::perimeter(c), 6 * dm::pi_of<T>(),
                                      _tol)) &&
                        (d_tests_near(dm::area(c), 9 * dm::pi_of<T>(),
                                      _tol)) &&
                        (dm::centroid(c) == p2(1, 2)),
                        "a circle's closed forms");
        // (Green's integrand takes central differences of step 2 pi /
        // 16384, which in float cost about 5e-6 of the area to rounding:
        // the check is relative, 1e-4)
        D_TESTS_EXPECT(d_tests_near(dm::area_green(dm::arc<car>(
                                        p2(1, 2), 3, 0,
                                        dm::two_pi_of<T>())),
                                    9 * dm::pi_of<T>(),
                                    9 * dm::pi_of<T>() *
                                        static_cast<T>(1e-4)),
                       "a circle's area by Green's theorem");

        typename dm::polygon<car, 3>::vertex_array v;

        v[0] = p2(-1, 2);
        v[1] = p2(4, -3);
        v[2] = p2(0, 5);

        const dm::aabb<2, T> pb = dm::bounding_box_of_polygon(
                                      dm::polygon<car, 3>(v));
        const dm::aabb<2, T> cb = dm::bounding_box_of_circle(c);

        D_TESTS_EXPECT( (pb.m_min == p2(-1, -3)) &&
                        (pb.m_max == p2(4, 5)) &&
                        (cb.m_min == p2(-2, -1)) &&
                        (cb.m_max == p2(4, 5)),
                        "bounding boxes");

        return;
    }
}  // namespace


int
main()
{
    d_tests_family<float>(1e-5f);
    d_tests_family<double>(1e-12);
    d_tests_family<long double>(1e-12L);

    std::printf("t_measure_2d_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
