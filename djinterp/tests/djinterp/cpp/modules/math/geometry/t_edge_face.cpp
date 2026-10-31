/*******************************************************************************
* djinterp [test]                                                t_edge_face.cpp
*
*   Tests for geometry/edge.hpp, at every level from C++98: a segment's
* points and length; a polyline's points, its parameter clamped at both
* ends, and its length; a quarter arc's ends and length, a full circle;
* Bezier curves' ends and midpoints; a parametric curve; the traits (every
* edge an edge, the closed forms where the header gives them); from C++11 a
* polyline from a std::array of points and an implicit unit circle.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/cpp/modules/math/geometry/t_edge_face.cpp -o t_edge -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_edge_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/edge.hpp"  // tested
// std
#include <cstdio>   // std::printf


namespace
{
    namespace dm = djinterp::math;

    long g_checks   = 0;
    long g_failures = 0;

    void d_tests_expect(
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
    bool d_tests_near(
        Float _a,
        Float _b,
        Float _tol
    )
    {
        const Float d = _a - _b;

        return ( (d <= _tol) &&
                 (d >= -_tol) );
    }

    struct t_curve
    {
        static const std::size_t output_dimension = 2;

        struct point
        {
            double c[2];

            double operator[](std::size_t _i) const { return c[_i]; }
        };

        static
        point evaluate(
            double _t
        )
        {
            point p;

            p.c[0] = _t;
            p.c[1] = _t * _t;

            return p;
        }
    };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    struct t_unit_circle
    {
        static
        double evaluate(
            const std::tuple<double, double>& _t
        )
        {
            return (std::get<0>(_t) * std::get<0>(_t)) +
                   (std::get<1>(_t) * std::get<1>(_t)) - 1.0;
        }
    };
#endif

    template<typename T>
    void d_tests_family(
        T _tol
    )
    {
        typedef dm::cartesian<2, T>      car;
        typedef typename car::point_type p2;

        const p2 o(0, 0);
        const p2 x(3, 0);
        const p2 xy(3, 4);

        const dm::line_segment<car> seg(o, xy);

        D_TESTS_EXPECT( (seg.at(static_cast<T>(0.5)) ==
                         p2(static_cast<T>(1.5), 2)) &&
                        (seg.closed_form_length() == static_cast<T>(5)),
                        "a segment's midpoint and length");

        typename dm::polyline_edge<car, 3>::point_array pts;

        pts[0] = o;
        pts[1] = x;
        pts[2] = xy;

        const dm::polyline_edge<car, 3> line(pts);

        D_TESTS_EXPECT( (line.at(static_cast<T>(1.5)) ==
                         p2(3, 2)) &&
                        (line.at(static_cast<T>(-1)) == o) &&
                        (line.at(static_cast<T>(9)) == xy) &&
                        (line.closed_form_length() == static_cast<T>(7)),
                        "a polyline: a point, both clamps, its length");

        const T                half_pi = dm::half_pi_of<T>();
        const dm::arc<car>     quarter(o, static_cast<T>(2), 0, half_pi);

        D_TESTS_EXPECT( (quarter.start() == p2(2, 0)) &&
                        (d_tests_near(quarter.end()[0], static_cast<T>(0),
                                      _tol)) &&
                        (d_tests_near(quarter.end()[1], static_cast<T>(2),
                                      _tol)) &&
                        (d_tests_near(quarter.closed_form_length(), half_pi * 2,
                                      _tol)) &&
                        (!quarter.is_full_circle()) &&
                        (dm::arc<car>().is_full_circle()),
                        "a quarter arc and a full circle");

        const dm::quadratic_bezier<car> qb(o, x, xy);
        const dm::cubic_bezier<car>     cb(o, x, xy, p2(0, 4));

        D_TESTS_EXPECT( (qb.start() == o) &&
                        (qb.end() == xy) &&
                        (qb.at(static_cast<T>(0.5)) ==
                         p2(static_cast<T>(2.25), 1)) &&
                        (cb.at(static_cast<T>(0)) == o) &&
                        (cb.at(static_cast<T>(1)) == p2(0, 4)) &&
                        (cb.at(static_cast<T>(0.5)) ==
                         p2(static_cast<T>(2.25), static_cast<T>(2))),
                        "Bezier curves' ends and midpoints");

        return;
    }
}  // namespace

D_STATIC_ASSERT((dm::is_edge<dm::line_segment<dm::cartesian<3> > >::value),
                "a line segment is an edge");
D_STATIC_ASSERT((dm::has_closed_form_length<
                     dm::arc<dm::polar<double> > >::value),
                "an arc has a closed-form length");
D_STATIC_ASSERT((!dm::has_closed_form_length<
                     dm::cubic_bezier<dm::cartesian<2> > >::value),
                "a Bezier curve has none");


int main()
{
    d_tests_family<float>(1e-6f);
    d_tests_family<double>(1e-15);
    d_tests_family<long double>(1e-15L);

    const dm::parametric_edge<t_curve> pe(0.0, 2.0);

    D_TESTS_EXPECT( (pe.at(1.5)[1] == 2.25) &&
                    (pe.end()[0] == 2.0),
                    "a parametric curve");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    typedef dm::cartesian<2>::point_type p2;
    const std::array<p2, 2> pts = {{ p2(0, 0), p2(0, 5) }};

    D_TESTS_EXPECT((dm::polyline_edge<dm::cartesian<2>, 2>(pts)
                        .closed_form_length() == 5.0),
                   "a polyline from a std::array of points");

    const dm::implicit_edge<t_unit_circle> circle;

    D_TESTS_EXPECT( (circle.on_curve(p2(0.6, 0.8))) &&
                    (!circle.on_curve(p2(0.5, 0.5))),
                    "an implicit unit circle");
#endif

    std::printf("t_edge_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
