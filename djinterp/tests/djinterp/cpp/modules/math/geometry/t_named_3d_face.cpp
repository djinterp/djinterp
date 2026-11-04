/*******************************************************************************
* djinterp [cpp/math]                                        t_named_3d_face.cpp
*
*   Tests for geometry/named_3d.hpp, at every level from C++98, per family:
* every named solid's known volume, surface area, centroid and containment
* -- a sphere, a sphere as an ellipsoid, a box and a cube, a cylinder, a
* cone, a frustum degenerating to a cylinder, a torus, a hemisphere, a
* hemispherical cap, a square prism and pyramid -- and the traits.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/geometry/t_named_3d_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_named_3d_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/named_3d.hpp"  // tested
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
        typedef dm::cartesian<3, T>      car;
        typedef typename car::point_type p3;

        const T  pi = dm::pi_of<T>();
        const p3 o(0, 0, 0);

        const dm::sphere<car>    ball(o, 2);
        const dm::ellipsoid<car> round(o, 2, 2, 2);

        D_TESTS_EXPECT( (d_tests_near(ball.closed_form_volume(),
                                      pi * 32 / 3, _tol * 32)) &&
                        (d_tests_near(ball.closed_form_surface_area(),
                                      pi * 16, _tol * 16)) &&
                        (d_tests_near(round.closed_form_volume(),
                                      ball.closed_form_volume(), _tol * 32)) &&
                        (ball.contains(p3(1, 1, 1))) &&
                        (!ball.contains(p3(2, 1, 0))),
                        "a sphere, and the sphere as an ellipsoid");

        const dm::box<car>  bx(o, 1, 2, 3);
        const dm::cube<car> cb(o, 2);

        D_TESTS_EXPECT( (bx.closed_form_volume() == 6) &&
                        (bx.closed_form_surface_area() == 22) &&
                        (bx.closed_form_centroid() ==
                         p3(static_cast<T>(0.5), 1, static_cast<T>(1.5))) &&
                        (cb.closed_form_volume() == 8) &&
                        (cb.contains(p3(1, 1, 1))) &&
                        (!cb.contains(p3(1, 1, 3))),
                        "a box and a cube");

        const dm::cylinder<car> cyl(o, 1, 2);
        const dm::cone<car>     cn(o, 1, 3);
        const dm::frustum<car>  same(o, 1, 1, 2);

        D_TESTS_EXPECT( (d_tests_near(cyl.closed_form_volume(), 2 * pi,
                                      _tol * 4)) &&
                        (cyl.closed_form_centroid() == p3(0, 0, 1)) &&
                        (d_tests_near(cn.closed_form_volume(), pi, _tol * 4)) &&
                        (cn.closed_form_centroid() ==
                         p3(0, 0, static_cast<T>(0.75))) &&
                        (d_tests_near(same.closed_form_volume(),
                                      cyl.closed_form_volume(), _tol * 4)) &&
                        (cyl.contains(p3(0, 0, 1))) &&
                        (!cn.contains(p3(static_cast<T>(0.9), 0,
                                         static_cast<T>(2.5)))),
                        "a cylinder, a cone, a frustum as a cylinder");

        // a torus of radii 3 and 1: volume 2 pi^2 R r^2, area 4 pi^2 R r
        const dm::torus<car> tor(o, 3, 1);

        D_TESTS_EXPECT( (d_tests_near(tor.closed_form_volume(),
                                      2 * pi * pi * 3, _tol * 64)) &&
                        (d_tests_near(tor.closed_form_surface_area(),
                                      4 * pi * pi * 3, _tol * 128)) &&
                        (tor.contains(p3(3, 0, 0))) &&
                        (!tor.contains(o)),
                        "a torus");

        // a hemisphere, and a cap the height of the radius: the same solid
        const dm::hemisphere<car>    hemi(o, 1);
        const dm::spherical_cap<car> cap(o, 1, 1);

        D_TESTS_EXPECT( (d_tests_near(hemi.closed_form_volume(),
                                      pi * 2 / 3, _tol * 4)) &&
                        (d_tests_near(cap.closed_form_volume(),
                                      hemi.closed_form_volume(), _tol * 4)) &&
                        (hemi.closed_form_centroid() ==
                         p3(0, 0, static_cast<T>(0.375))),
                        "a hemisphere and a hemispherical cap");

        // a square prism and pyramid of circumradius sqrt(2): base side 2
        const T                             r = static_cast<T>(
                                                    1.4142135623730951);
        const dm::regular_prism<car, 4>   prism(o, r, 3);
        const dm::regular_pyramid<car, 4> pyramid(o, r, 3);

        D_TESTS_EXPECT( (d_tests_near(prism.closed_form_volume(),
                                      static_cast<T>(12), _tol * 16)) &&
                        (d_tests_near(pyramid.closed_form_volume(),
                                      static_cast<T>(4), _tol * 16)) &&
                        (prism.contains(p3(0, 0, 1))) &&
                        (!pyramid.contains(p3(static_cast<T>(0.9), 0,
                                              static_cast<T>(2.5)))),
                        "a square prism and pyramid");

        return;
    }
}  // namespace

D_STATIC_ASSERT((dm::is_shape<dm::sphere<> >::value) &&
                (dm::has_closed_form_volume<dm::torus<> >::value) &&
                (!dm::has_closed_form_centroid<dm::frustum<> >::value),
                "the traits");


int
main()
{
    d_tests_family<float>(2e-6f);
    d_tests_family<double>(4e-15);
    d_tests_family<long double>(4e-15L);

    std::printf("t_named_3d_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
