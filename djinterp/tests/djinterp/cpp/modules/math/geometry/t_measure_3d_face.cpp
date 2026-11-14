/*******************************************************************************
* djinterp [cpp/math]                                      t_measure_3d_face.cpp
*
*   Tests for geometry/measure_3d.hpp, at every level from C++98, per family:
* a flat bilinear patch's area by the iterated Simpson rule against a
* triangle pair's closed form; a sphere's closed-form volume and area; the
* volume a parametric sphere bounds by the divergence theorem; a cone's
* centroid; the bounding boxes.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/cpp/modules/math/geometry/t_measure_3d_face.cpp       \
*         -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_measure_3d_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.11.07
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/measure_3d.hpp"  // tested
// std
#include <cmath>    // std::sin, std::cos
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

    // a closed surface: the sphere of radius m_r about the origin
    template<typename T>
    struct t_sphere_surface
    {
        typedef dm::cartesian<3, T>                    coordinate_system;
        typedef T                                      value_type;
        typedef typename coordinate_system::point_type point_type;

        static const std::size_t dimension            = 3;
        static const std::size_t parametric_dimension = 2;
        static const bool        is_surface           = true;

        T m_r;

        explicit t_sphere_surface(T _r) : m_r(_r) {}

        T parameter_min_u() const { return T(0); }
        T parameter_max_u() const { return dm::pi_of<T>(); }
        T parameter_min_v() const { return T(0); }
        T parameter_max_v() const { return dm::two_pi_of<T>(); }

        point_type
        at(T _u, T _v) const
        {
            return point_type(m_r * std::sin(_u) * std::cos(_v),
                              m_r * std::sin(_u) * std::sin(_v),
                              m_r * std::cos(_u));
        }
    };

    template<typename T>
    void
    d_tests_family(T _tol)
    {
        typedef dm::cartesian<3, T>      car;
        typedef typename car::point_type p3;

        const T  pi = dm::pi_of<T>();
        const p3 o(0, 0, 0);

        // a flat 2-by-3 rectangle: Simpson's area against the closed form
        const dm::bilinear_patch<car> flat(o, p3(2, 0, 0), p3(0, 3, 0),
                                           p3(2, 3, 0));

        D_TESTS_EXPECT( (d_tests_near(dm::patch_area(flat, 8, 8), T(6),
                                      _tol * 64)) &&
                        (dm::patch_area(dm::triangle_surface<car>(
                             o, p3(2, 0, 0), p3(0, 3, 0))) == T(3)),
                        "patch areas, by Simpson's rule and closed");

        const dm::sphere<car> ball(o, 2);

        D_TESTS_EXPECT( (d_tests_near(dm::volume(ball), pi * 32 / 3,
                                      _tol * 32)) &&
                        (d_tests_near(dm::surface_area(ball), pi * 16,
                                      _tol * 16)),
                        "a sphere's closed forms");

        // the divergence theorem over a parametric sphere (relative 1e-3:
        // central differences and a 24-by-24 iterated Simpson rule)
        D_TESTS_EXPECT(d_tests_near(dm::volume_divergence(
                                        t_sphere_surface<T>(2), 24, 24),
                                    pi * 32 / 3, pi * 32 / 3 * T(1e-3)),
                       "a sphere's volume by the divergence theorem");

        D_TESTS_EXPECT(dm::centroid_3d(dm::cone<car>(o, 1, 4)) == p3(0, 0, 1),
                       "a cone's centroid");

        const dm::aabb<3, T> cb = dm::bounding_box_of_cylinder(
                                      dm::cylinder<car>(p3(1, 1, 1), 2, 3));

        D_TESTS_EXPECT( (cb.m_min == p3(-1, -1, 1)) &&
                        (cb.m_max == p3(3, 3, 4)) &&
                        (dm::bounding_box_of_sphere(ball).m_max ==
                         p3(2, 2, 2)) &&
                        (dm::bounding_box_of_box(dm::box<car>(o, 1, 2, 3))
                             .m_max == p3(1, 2, 3)),
                        "bounding boxes");

        return;
    }
}  // namespace


int
main()
{
    d_tests_family<float>(1e-5f);
    d_tests_family<double>(1e-13);
    d_tests_family<long double>(1e-13L);

    std::printf("t_measure_3d_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
