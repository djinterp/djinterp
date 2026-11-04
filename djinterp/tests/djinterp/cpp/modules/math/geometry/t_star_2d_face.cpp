/*******************************************************************************
* djinterp [cpp/math]                                         t_star_2d_face.cpp
*
*   Tests for geometry/star_2d.hpp, at every level from C++98, per family: a
* pentagram's hollow centre (even-odd) and filled points, its perimeter as
* five equal chords; a 5-pointed star's area against the polygon it is; the
* hexagonal star as the 6-pointed star with inner radius R / sqrt(3); the
* traits; from C++11 the named aliases.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/geometry/t_star_2d_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_star_2d_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/star_2d.hpp"  // tested
// std
#include <cmath>    // std::sqrt, std::sin
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

        // the pentagram {5/2} of circumradius 1, its first point on +x:
        // hollow at the centre, the point filled, five chords of 2 sin(2 pi
        // / 5)
        const dm::star_polygon<car, 5, 2> penta(o, 1);

        D_TESTS_EXPECT( (!penta.contains(o)) &&
                        (penta.contains(p2(static_cast<T>(0.8), 0))) &&
                        (!penta.contains(p2(static_cast<T>(1.1), 0))) &&
                        (d_tests_near(penta.closed_form_perimeter(),
                                      static_cast<T>(10) *
                                          static_cast<T>(std::sin(
                                              2 * pi / 5)),
                                      _tol)),
                        "a pentagram");

        // a 5-pointed star, its first point on +x: filled at the centre and
        // the point, empty in the valley below it
        const dm::n_pointed_star<car, 5> five(o, 2, 1);

        D_TESTS_EXPECT( (five.contains(o)) &&
                        (five.contains(p2(static_cast<T>(1.8), 0))) &&
                        (!five.contains(p2(static_cast<T>(1.5),
                                           static_cast<T>(-0.5)))) &&
                        (d_tests_near(five.closed_form_centroid()[0], T(0),
                                      _tol)),
                        "a 5-pointed star");

        // the hexagonal star is the 6-pointed star with r = R / sqrt(3)
        const dm::hexagonal_star<car>    hex(o, 3);
        const dm::n_pointed_star<car, 6> six(o, 3,
                                             static_cast<T>(3) /
                                                 std::sqrt(static_cast<T>(3)));

        D_TESTS_EXPECT( (d_tests_near(hex.closed_form_area(),
                                      six.closed_form_area(), _tol * 10)) &&
                        (d_tests_near(hex.closed_form_perimeter(),
                                      six.closed_form_perimeter(),
                                      _tol * 10)) &&
                        (hex.contains(o)),
                        "the hexagonal star");

        return;
    }
}  // namespace

D_STATIC_ASSERT((dm::is_shape<dm::star_polygon<> >::value) &&
                (dm::has_closed_form_area<dm::n_pointed_star<
                     dm::cartesian<2>, 5> >::value),
                "the traits");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
static_assert(dm::pentagram<>::vertex_n == 5 &&
              dm::octagram<>::step_k == 3,
              "the named aliases (C++11)");
#endif


int
main()
{
    d_tests_family<float>(1e-5f);
    d_tests_family<double>(1e-13);
    d_tests_family<long double>(1e-13L);

    std::printf("t_star_2d_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
