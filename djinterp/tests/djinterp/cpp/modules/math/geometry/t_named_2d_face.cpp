/*******************************************************************************
* djinterp [cpp/math]                                        t_named_2d_face.cpp
*
*   Tests for geometry/named_2d.hpp, at every level from C++98, per family:
* every named shape's known areas, perimeters, centroids and containment --
* a unit circle, a 2-by-1 ellipse rotated a quarter turn, a rectangle, a
* square, a right triangle, a regular hexagon, an annulus, a half-disc
* sector and segment -- and the traits.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/geometry/t_named_2d_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_named_2d_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/named_2d.hpp"  // tested
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

        const T  pi = dm::pi_of<T>();
        const p2 o(0, 0);

        const dm::circle<car> unit(o, 1);

        D_TESTS_EXPECT( (unit.closed_form_area() == pi) &&
                        (unit.closed_form_perimeter() == 2 * pi) &&
                        (unit.contains(p2(static_cast<T>(0.6),
                                          static_cast<T>(0.6)))) &&
                        (!unit.contains(p2(static_cast<T>(0.8),
                                           static_cast<T>(0.8)))),
                        "a unit circle");

        // 2 by 1, turned a quarter: (0, 1.5) inside, (1.5, 0) outside
        const dm::ellipse<car> e(o, 2, 1, pi / 2);

        D_TESTS_EXPECT( (d_tests_near(e.closed_form_area(), 2 * pi, _tol)) &&
                        (e.contains(p2(0, static_cast<T>(1.5)))) &&
                        (!e.contains(p2(static_cast<T>(1.5), 0))),
                        "a rotated ellipse");

        const dm::rectangle<car> rect(p2(1, 1), 4, 2);
        const dm::square<car>    sq(p2(1, 1), 3);

        D_TESTS_EXPECT( (rect.closed_form_area() == 8) &&
                        (rect.closed_form_perimeter() == 12) &&
                        (rect.closed_form_centroid() == p2(3, 2)) &&
                        (sq.closed_form_area() == 9) &&
                        (sq.contains(p2(4, 4))) &&
                        (!sq.contains(p2(static_cast<T>(4.5), 4))),
                        "a rectangle and a square");

        const dm::triangle<car> tri(o, p2(3, 0), p2(0, 4));

        D_TESTS_EXPECT( (tri.closed_form_area() == 6) &&
                        (tri.closed_form_perimeter() == 12) &&
                        (tri.closed_form_centroid() == p2(1, static_cast<T>(4) /
                                                             3)) &&
                        (tri.contains(p2(1, 1))) &&
                        (!tri.contains(p2(3, 3))),
                        "a 3-4-5 triangle");

        // a regular hexagon of circumradius 1: area 3 sqrt(3) / 2
        const dm::regular_polygon<car, 6> hex(o, 1);

        D_TESTS_EXPECT( (d_tests_near(hex.closed_form_area(),
                                      static_cast<T>(2.598076211353316),
                                      _tol)) &&
                        (d_tests_near(hex.closed_form_perimeter(),
                                      static_cast<T>(6), _tol)) &&
                        (hex.contains(p2(static_cast<T>(0.8), 0))) &&
                        (!hex.contains(p2(0, static_cast<T>(0.9)))),
                        "a regular hexagon");

        const dm::annulus<car> ring(o, 1, 2);

        D_TESTS_EXPECT( (d_tests_near(ring.closed_form_area(), 3 * pi, _tol)) &&
                        (ring.contains(p2(static_cast<T>(1.5), 0))) &&
                        (!ring.contains(p2(static_cast<T>(0.5), 0))),
                        "an annulus");

        // the upper half disc, as a sector and as a segment
        const dm::circular_sector<car>  half_sector(o, 1, 0, pi);
        const dm::circular_segment<car> half_segment(o, 1, 0, pi);

        D_TESTS_EXPECT( (d_tests_near(half_sector.closed_form_area(), pi / 2,
                                      _tol)) &&
                        (d_tests_near(half_segment.closed_form_area(), pi / 2,
                                      _tol)) &&
                        (half_sector.contains(p2(0, static_cast<T>(0.5)))) &&
                        (!half_sector.contains(p2(0, static_cast<T>(-0.5)))),
                        "a half-disc sector and segment");

        return;
    }
}  // namespace

D_STATIC_ASSERT((dm::is_shape<dm::circle<> >::value) &&
                (dm::has_closed_form_area<dm::regular_polygon<
                     dm::cartesian<2>, 7> >::value) &&
                (!dm::has_closed_form_centroid<dm::circular_sector<> >::value),
                "the traits");


int
main()
{
    d_tests_family<float>(1e-5f);
    d_tests_family<double>(1e-14);
    d_tests_family<long double>(1e-14L);

    std::printf("t_named_2d_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
