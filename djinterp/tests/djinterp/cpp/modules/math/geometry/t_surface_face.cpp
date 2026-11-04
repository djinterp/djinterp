/*******************************************************************************
* djinterp [cpp/math]                                         t_surface_face.cpp
*
*   Tests for geometry/surface.hpp, at every level from C++98: a triangle's
* corners at (0,0), (1,0), (0,1) and its area and normal; a unit square's
* area as a triangle pair and as a planar polygon; a bilinear patch's
* corners and centre; a parametric surface's points; the traits -- 3D
* surfaces have a closed-form area, a 4D one has none, as before; from C++11
* a polygon from a std::array of points and implicit_surface.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/geometry/t_surface_face.cpp -o t_surface -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_surface_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/surface.hpp"  // tested
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

    struct t_saddle
    {
        static const std::size_t output_dimension = 3;

        struct point
        {
            double c[3];

            double operator[](std::size_t _i) const { return c[_i]; }
        };

        static point
        evaluate(double _u, double _v)
        {
            point p;

            p.c[0] = _u;
            p.c[1] = _v;
            p.c[2] = (_u * _u) - (_v * _v);

            return p;
        }
    };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    struct t_unit_sphere
    {
        template<typename Tuple>
        static double
        evaluate(const Tuple& _t)
        {
            return (std::get<0>(_t) * std::get<0>(_t)) +
                   (std::get<1>(_t) * std::get<1>(_t)) +
                   (std::get<2>(_t) * std::get<2>(_t)) - 1.0;
        }
    };
#endif

    template<typename T>
    void
    d_tests_family()
    {
        typedef dm::cartesian<3, T>  car;
        typedef typename car::point_type p3;

        const p3 o(0, 0, 0);
        const p3 x(1, 0, 0);
        const p3 y(0, 1, 0);
        const p3 xy(1, 1, 0);
        const dm::triangle_surface<car> tri(o, x, y);

        D_TESTS_EXPECT( (tri.at(0, 0) == o) &&
                        (tri.at(1, 0) == x) &&
                        (tri.at(0, 1) == y),
                        "a triangle's corners");
        D_TESTS_EXPECT( (tri.closed_form_area() == static_cast<T>(0.5)) &&
                        (tri.normal() == p3(0, 0, 1)),
                        "a triangle's area and normal");

        typename dm::planar_polygon_surface<car, 4>::vertex_array v;

        v[0] = o;
        v[1] = x;
        v[2] = xy;
        v[3] = y;

        D_TESTS_EXPECT((dm::planar_polygon_surface<car, 4>(v)
                            .closed_form_area() == static_cast<T>(1)),
                       "a unit square's area as a polygon");

        const dm::bilinear_patch<car> patch(o, x, y, xy);

        D_TESTS_EXPECT( (patch.at(0, 0) == o) &&
                        (patch.at(1, 1) == xy) &&
                        (patch.at(static_cast<T>(0.5), static_cast<T>(0.5)) ==
                         p3(static_cast<T>(0.5), static_cast<T>(0.5), 0)),
                        "a bilinear patch's corners and centre");

        return;
    }
}  // namespace

// as before, a 3D triangle has a closed-form area and a 4D one none
D_STATIC_ASSERT((dm::has_closed_form_area<
                     dm::triangle_surface<dm::cartesian<3> > >::value),
                "a 3D triangle has a closed-form area");
D_STATIC_ASSERT((!dm::has_closed_form_area<
                     dm::triangle_surface<dm::cartesian<4> > >::value),
                "a 4D one has none");
D_STATIC_ASSERT((dm::is_surface<dm::bilinear_patch<> >::value),
                "a bilinear patch is a surface");


int
main()
{
    d_tests_family<float>();
    d_tests_family<double>();
    d_tests_family<long double>();

    // a parametric surface's points
    const dm::parametric_surface_edge<t_saddle> saddle(-1.0, 1.0, -1.0, 1.0);

    D_TESTS_EXPECT( (saddle.at(0.5, 0.25)[2] == 0.1875) &&
                    (saddle.parameter_max_v() == 1.0),
                    "a parametric surface");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from C++11: a polygon from a std::array of points, an implicit surface
    typedef dm::cartesian<3>::point_type p3;
    const std::array<p3, 3> verts = {{ p3(0, 0, 0), p3(2, 0, 0), p3(0, 2, 0) }};

    D_TESTS_EXPECT((dm::planar_polygon_surface<dm::cartesian<3>, 3>(verts)
                        .closed_form_area() == 2.0),
                   "a polygon from a std::array of points");

    const dm::implicit_surface<t_unit_sphere> sphere;

    D_TESTS_EXPECT( (sphere.on_surface(p3(0, 1, 0))) &&
                    (!sphere.on_surface(p3(0, 2, 0))),
                    "an implicit surface");
#endif

    std::printf("t_surface_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
