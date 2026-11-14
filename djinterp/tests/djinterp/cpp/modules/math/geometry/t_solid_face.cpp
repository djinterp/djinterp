/*******************************************************************************
* djinterp [cpp/math]                                           t_solid_face.cpp
*
*   Tests for geometry/solid.hpp, at every level from C++98: a unit cube as
* a triangle mesh -- its surface area 6, volume 1, centroid its centre,
* containment inside and out -- in float, double and long double; the same
* cube in spherical coordinates; a parametric volume's containment; the
* traits; from C++11 a mesh from std::arrays, an implicit unit ball and a
* solid from two surfaces.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/cpp/modules/math/geometry/t_solid_face.cpp            \
*         -o t_solid -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_solid_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.11.07
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/solid.hpp"  // tested
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

    // the unit cube [0, 1]^3: 8 vertices, 12 faces wound outward
    const std::size_t g_cube_faces[12][3] = {
        { 0, 2, 1 }, { 0, 3, 2 },   // z = 0
        { 4, 5, 6 }, { 4, 6, 7 },   // z = 1
        { 0, 1, 5 }, { 0, 5, 4 },   // y = 0
        { 3, 7, 6 }, { 3, 6, 2 },   // y = 1
        { 0, 4, 7 }, { 0, 7, 3 },   // x = 0
        { 1, 2, 6 }, { 1, 6, 5 }    // x = 1
    };

    template<typename System>
    dm::triangle_mesh<System, 8, 12>
    d_tests_cube()
    {
        typedef dm::triangle_mesh<System, 8, 12>        mesh;
        typedef typename System::value_type             T;
        typedef dm::cartesian<3, T>                     car;
        typedef typename car::point_type                p3;

        const T corners[8][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 },
                                  { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 },
                                  { 1, 1, 1 }, { 0, 1, 1 } };
        typename mesh::vertex_array v;
        typename mesh::face_array   f;

        for (std::size_t k = 0; k < 8; ++k)
        {
            v[k] = System::from_cartesian(p3(corners[k][0], corners[k][1],
                                             corners[k][2]));
        }

        for (std::size_t k = 0; k < 12; ++k)
        {
            for (std::size_t j = 0; j < 3; ++j)
            {
                f[k][j] = g_cube_faces[k][j];
            }
        }

        return mesh(v, f);
    }

    template<typename T>
    void
    d_tests_family(T _tol)
    {
        typedef dm::cartesian<3, T>       car;
        typedef typename car::point_type  p3;

        const dm::triangle_mesh<car, 8, 12> cube = d_tests_cube<car>();

        D_TESTS_EXPECT( (cube.closed_form_surface_area() ==
                         static_cast<T>(6)) &&
                        (cube.closed_form_volume() == static_cast<T>(1)),
                        "the unit cube's area and volume");
        D_TESTS_EXPECT(cube.closed_form_centroid().equals(
                           p3(0.5, 0.5, 0.5), _tol),
                       "the unit cube's centroid");
        D_TESTS_EXPECT( (cube.contains(p3(0.25, 0.5, 0.75))) &&
                        (!cube.contains(p3(1.5, 0.5, 0.5))) &&
                        (!cube.contains(p3(-0.5, 0.5, 0.5))),
                        "containment inside and out");

        return;
    }
}  // namespace

D_STATIC_ASSERT((dm::is_shape<dm::triangle_mesh<dm::cartesian<3>, 4, 4> >
                     ::value),
                "a triangle mesh is a shape");
D_STATIC_ASSERT((dm::has_closed_form_volume<
                     dm::triangle_mesh<dm::cartesian<3>, 4, 4> >::value),
                "with a closed-form volume");


int
main()
{
    d_tests_family<float>(1e-6f);
    d_tests_family<double>(1e-15);
    d_tests_family<long double>(1e-15L);

    // the same cube in spherical coordinates
    const dm::triangle_mesh<dm::spherical<double>, 8, 12> sc =
        d_tests_cube<dm::spherical<double> >();

    D_TESTS_EXPECT( (d_tests_near(sc.closed_form_volume(), 1.0, 1e-14)) &&
                    (d_tests_near(sc.closed_form_surface_area(), 6.0, 1e-14)),
                    "a cube in spherical coordinates");

    // a parametric volume around a bilinear patch: a count, sampled
    typedef dm::cartesian<3>        car;
    typedef car::point_type          p3;
    const dm::bilinear_patch<car>    patch(p3(0, -1, -1), p3(0, 1, -1),
                                           p3(0, -1, 1), p3(0, 1, 1));
    const dm::parametric_volume<dm::bilinear_patch<car> > vol(patch);

    D_TESTS_EXPECT( (vol.contains(p3(-1, 0.3, 0.2))) &&
                    (!vol.contains(p3(1, 0.3, 0.2))) &&
                    (vol.contains<8>(p3(-1, 0.3, 0.2))),
                    "a parametric volume's crossing count");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from C++11: a mesh from std::arrays, an implicit ball, two surfaces
    const std::array<p3, 4> tv = {{ p3(0, 0, 0), p3(1, 0, 0), p3(0, 1, 0),
                                    p3(0, 0, 1) }};
    const std::array<std::array<std::size_t, 3>, 4> tf = {{ {{ 0, 2, 1 }},
                                                            {{ 0, 1, 3 }},
                                                            {{ 0, 3, 2 }},
                                                            {{ 1, 2, 3 }} }};
    const dm::triangle_mesh<car, 4, 4> tet(tv, tf);

    D_TESTS_EXPECT(d_tests_near(tet.closed_form_volume(), 1.0 / 6.0, 1e-16),
                   "a tetrahedron from std::arrays");

    struct t_ball
    {
        static double
        evaluate(const std::tuple<double, double, double>& _t)
        {
            return (std::get<0>(_t) * std::get<0>(_t)) +
                   (std::get<1>(_t) * std::get<1>(_t)) +
                   (std::get<2>(_t) * std::get<2>(_t)) - 1.0;
        }
    };

    const dm::implicit_volume<t_ball> ball;

    D_TESTS_EXPECT( (ball.contains(p3(0.5, 0.5, 0.5))) &&
                    (!ball.contains(p3(1.0, 1.0, 0.0))) &&
                    (ball.on_boundary(p3(1.0, 0.0, 0.0))),
                    "an implicit ball");

    const dm::shape_from_surfaces<dm::bilinear_patch<car>,
                                  dm::bilinear_patch<car> >
        two(patch, dm::bilinear_patch<car>(p3(2, -1, -1), p3(2, 1, -1),
                                           p3(2, -1, 1), p3(2, 1, 1)));

    D_TESTS_EXPECT( (two.contains(p3(1, 0.3, 0.2))) &&
                    (!two.contains(p3(-1, 0.3, 0.2))) &&
                    (two.contains<4>(p3(1, 0.3, 0.2))),
                    "a solid between two surfaces");
#endif

    std::printf("t_solid_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
