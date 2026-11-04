/*******************************************************************************
* djinterp [cpp/math]                                        t_shape_2d_face.cpp
*
*   Tests for geometry/shape_2d.hpp, at every level from C++98: a square
* and an L-shaped hexagon as polygons -- containment, perimeter, area,
* centroid -- in float, double and long double and in polar coordinates; a
* degenerate polygon's centroid (its first vertex); an integer polygon
* through double; a disc as a parametric region; the traits; from C++11 a
* polygon from a std::array, an implicit disc and a region from four
* segments.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/geometry/t_shape_2d_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_shape_2d_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/shape_2d.hpp"  // tested
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
    d_tests_family()
    {
        typedef dm::cartesian<2, T>      car;
        typedef typename car::point_type p2;

        typename dm::polygon<car, 4>::vertex_array sq;

        sq[0] = p2(0, 0);
        sq[1] = p2(2, 0);
        sq[2] = p2(2, 2);
        sq[3] = p2(0, 2);

        const dm::polygon<car, 4> square(sq);

        D_TESTS_EXPECT( (square.contains(p2(1, 1))) &&
                        (!square.contains(p2(3, 1))) &&
                        (square.closed_form_perimeter() == static_cast<T>(8)) &&
                        (square.closed_form_area() == static_cast<T>(4)) &&
                        (square.closed_form_centroid() == p2(1, 1)),
                        "a square");

        // an L: the unit squares (0..2, 0..1) and (0..1, 1..2)
        typename dm::polygon<car, 6>::vertex_array l;

        l[0] = p2(0, 0);
        l[1] = p2(2, 0);
        l[2] = p2(2, 1);
        l[3] = p2(1, 1);
        l[4] = p2(1, 2);
        l[5] = p2(0, 2);

        const dm::polygon<car, 6> ell(l);

        D_TESTS_EXPECT( (ell.contains(p2(static_cast<T>(0.5),
                                         static_cast<T>(1.5)))) &&
                        (!ell.contains(p2(static_cast<T>(1.5),
                                          static_cast<T>(1.5)))) &&
                        (ell.closed_form_area() == static_cast<T>(3)) &&
                        (ell.closed_form_centroid().equals(
                             p2(static_cast<T>(5) / 6, static_cast<T>(5) / 6),
                             static_cast<T>(1e-6))),
                        "an L-shaped hexagon");

        // a degenerate polygon: its first vertex
        typename dm::polygon<car, 3>::vertex_array flat;

        flat[0] = p2(1, 1);
        flat[1] = p2(2, 2);
        flat[2] = p2(3, 3);

        D_TESTS_EXPECT( (dm::polygon<car, 3>(flat).closed_form_centroid() ==
                         p2(1, 1)) &&
                        (dm::polygon<car, 3>(flat).closed_form_area() ==
                         static_cast<T>(0)),
                        "a degenerate polygon");

        return;
    }
}  // namespace

D_STATIC_ASSERT((dm::is_shape<dm::polygon<dm::cartesian<2>, 5> >::value),
                "a polygon is a shape");
D_STATIC_ASSERT((dm::has_closed_form_area<
                     dm::polygon<dm::cartesian<2>, 5> >::value),
                "with a closed-form area");


int
main()
{
    d_tests_family<float>();
    d_tests_family<double>();
    d_tests_family<long double>();

    // the square in polar coordinates
    typedef dm::polar<double> pol;
    typedef dm::cartesian<2>  car;
    typedef car::point_type   p2;
    dm::polygon<pol, 4>::vertex_array ps;

    ps[0] = pol::from_cartesian(p2(0, 0));
    ps[1] = pol::from_cartesian(p2(2, 0));
    ps[2] = pol::from_cartesian(p2(2, 2));
    ps[3] = pol::from_cartesian(p2(0, 2));

    const dm::polygon<pol, 4> polar_square(ps);

    D_TESTS_EXPECT( (d_tests_near(polar_square.closed_form_area(), 4.0,
                                  1e-14)) &&
                    (polar_square.contains(pol::from_cartesian(p2(1, 1)))),
                    "a square in polar coordinates");

    // an integer polygon, through double
    typedef dm::cartesian<2, int> icar;
    dm::polygon<icar, 3>::vertex_array it;

    it[0] = icar::point_type(0, 0);
    it[1] = icar::point_type(4, 0);
    it[2] = icar::point_type(0, 4);

    D_TESTS_EXPECT( (dm::polygon<icar, 3>(it).closed_form_area() == 8) &&
                    (dm::polygon<icar, 3>(it).contains(
                         icar::point_type(1, 1))),
                    "an int polygon");

    // a disc as a parametric region
    const dm::arc<car>                         circle(p2(0, 0), 1.0, 0.0,
                                                      dm::two_pi_of<double>());
    const dm::parametric_region<dm::arc<car> > disc(circle);

    D_TESTS_EXPECT( (disc.contains(p2(0.5, 0.25))) &&
                    (!disc.contains(p2(1.5, 0.25))) &&
                    (disc.contains<16>(p2(0.25, 0.25))),
                    "a disc");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from C++11: a polygon from a std::array, an implicit disc, segments
    const std::array<p2, 3> tri = {{ p2(0, 0), p2(3, 0), p2(0, 3) }};

    D_TESTS_EXPECT((dm::polygon<car, 3>(tri).closed_form_area() == 4.5),
                   "a polygon from a std::array of points");

    struct t_disc
    {
        static double
        evaluate(const std::tuple<double, double>& _t)
        {
            return (std::get<0>(_t) * std::get<0>(_t)) +
                   (std::get<1>(_t) * std::get<1>(_t)) - 1.0;
        }
    };

    const dm::implicit_region<t_disc> idisc;

    D_TESTS_EXPECT( (idisc.contains(p2(0.5, 0.5))) &&
                    (!idisc.contains(p2(1.0, 1.0))) &&
                    (idisc.on_boundary(p2(0.0, 1.0))),
                    "an implicit disc");

    typedef dm::line_segment<car> seg;
    const dm::shape_from_edges<seg, seg, seg, seg> box(
        seg(p2(0, 0), p2(2, 0)), seg(p2(2, 0), p2(2, 2)),
        seg(p2(2, 2), p2(0, 2)), seg(p2(0, 2), p2(0, 0)));

    D_TESTS_EXPECT( (box.contains(p2(1, 1))) &&
                    (!box.contains(p2(3, 1))) &&
                    (box.contains<4>(p2(0.5, 1.5))),
                    "a region from four segments");
#endif

    std::printf("t_shape_2d_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
