/*******************************************************************************
* djinterp [cpp/math]                                         t_star_3d_face.cpp
*
*   Tests for geometry/star_3d.hpp, at every level from C++98, per family:
* stellate at height 0 keeps the base solid's volume and at a positive
* height adds to it; the stella octangula's closed-form volume, a^3 sqrt(2)
* / 8 with a its edge, and its centre inside; each Kepler-Poinsot solid's
* hull mesh holding its centre, its closed-form centroid the centre; the
* traits.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/geometry/t_star_3d_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_star_3d_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/star_3d.hpp"   // tested
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/platonic.hpp"  // base meshes
// std
#include <cmath>    // std::sqrt
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

    // (a point just off the centre: the +x ray from the centre itself runs
    // through the hull's vertices and edges, where a crossing count is
    // ambiguous)
    template<typename Solid>
    bool
    d_tests_hull_holds_centre(const Solid& _s)
    {
        typedef typename Solid::value_type T;
        typedef typename Solid::point_type p3;

        const p3 near_centre = _s.m_center.plus(p3(T(0.01), T(0.02),
                                                   T(0.03)));

        return ( (_s.as_triangle_mesh().contains(near_centre)) &&
                 (_s.closed_form_centroid() == _s.m_center) );
    }

    template<typename T>
    void
    d_tests_family(T _tol)
    {
        typedef dm::cartesian<3, T>      car;
        typedef typename car::point_type p3;

        // stellate: no height, no volume added; some height, some added
        const T base = dm::icosahedron_mesh<T>(1).closed_form_volume();

        D_TESTS_EXPECT( (d_tests_near(dm::stellate(dm::icosahedron_mesh<T>(1),
                                                   T(0)).closed_form_volume(),
                                      base, _tol)) &&
                        (dm::stellate(dm::icosahedron_mesh<T>(1), T(0.25))
                             .closed_form_volume() > base),
                        "stellate");

        // the stella octangula of circumradius 1: edge 2 sqrt(2/3)
        const p3                          c(T(0.5), T(-1), T(2));
        const dm::stella_octangula<car>   so(c, 1);
        const T                           a = so.edge_length();

        D_TESTS_EXPECT( (d_tests_near(so.closed_form_volume(),
                                      a * a * a * std::sqrt(T(2)) / 8,
                                      _tol)) &&
                        (so.contains(c)) &&
                        (!so.contains(c.plus(p3(2, 0, 0)))),
                        "the stella octangula");

        D_TESTS_EXPECT( (d_tests_hull_holds_centre(
                             dm::small_stellated_dodecahedron<car>(c, 1))) &&
                        (d_tests_hull_holds_centre(
                             dm::great_dodecahedron<car>(c, 1))) &&
                        (d_tests_hull_holds_centre(
                             dm::great_stellated_dodecahedron<car>(c, 1))) &&
                        (d_tests_hull_holds_centre(
                             dm::great_icosahedron<car>(c, 1))),
                        "the Kepler-Poinsot solids' hulls");

        return;
    }
}  // namespace

D_STATIC_ASSERT((dm::is_shape<dm::stella_octangula<> >::value) &&
                (dm::has_closed_form_volume<dm::great_icosahedron<> >::value),
                "the traits");


int
main()
{
    d_tests_family<float>(1e-5f);
    d_tests_family<double>(1e-13);
    d_tests_family<long double>(1e-13L);

    std::printf("t_star_3d_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
