/*******************************************************************************
* djinterp [cpp/math]                                 t_geometry_common_face.cpp
*
*   Tests for geometry/geometry_common.hpp, at every level from C++98: the
* detection traits on types that have, lack, and set false their flags and
* closed forms (the same answers at every level); cartesian_lerp and
* cartesian_distance through a curvilinear system; the Cartesian products;
* the bounding box -- grown from empty, its center, extent and containment;
* Simpson's rule on polynomials it integrates exactly; the constants; and
* from C++11 the std::array overloads and from C++14 pi_v.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/cpp/modules/math/geometry/t_geometry_common_face.cpp  \
*         -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_geometry_common_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.11.07
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/geometry_common.hpp"  // test
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

    // an edge with a closed-form length; a shape with an area and a
    // centroid; a type flagged false; a type with nothing
    struct t_edge
    {
        static const bool is_edge = true;

        double closed_form_length() const { return 2.0; }
    };

    struct t_shape
    {
        static const bool is_shape = true;

        double closed_form_area() const { return 1.0; }
        void   closed_form_centroid() const {}
    };

    struct t_not_a_shape
    {
        static const bool is_shape = false;
    };

    struct t_plain
    {};

    struct t_cube
    {
        double operator()(double _x) const { return _x * _x * _x; }
    };

    template<typename Float>
    bool
    d_tests_near(Float _a, Float _b, Float _tol)
    {
        const Float d = _a - _b;

        return ( (d <= _tol) &&
                 (d >= -_tol) );
    }

    void
    d_tests_traits()
    {
        D_TESTS_EXPECT( (dm::is_edge<t_edge>::value) &&
                        (!dm::is_edge<t_shape>::value) &&
                        (!dm::is_edge<int>::value),
                        "is_edge");
        D_TESTS_EXPECT( (dm::is_shape<t_shape>::value) &&
                        (!dm::is_shape<t_not_a_shape>::value) &&
                        (!dm::is_shape<t_plain>::value),
                        "is_shape: present and true, present and false, "
                        "absent");
        D_TESTS_EXPECT( (dm::has_closed_form_length<t_edge>::value) &&
                        (!dm::has_closed_form_length<t_shape>::value) &&
                        (dm::has_closed_form_area<t_shape>::value) &&
                        (dm::has_closed_form_centroid<t_shape>::value) &&
                        (!dm::has_closed_form_volume<t_shape>::value) &&
                        (!dm::has_closed_form_perimeter<t_plain>::value) &&
                        (!dm::has_closed_form_surface_area<int>::value),
                        "the closed forms, a void one included");

        return;
    }

    void
    d_tests_helpers()
    {
        typedef dm::polar<double>      pol;
        typedef dm::cartesian<3>       car;
        typedef car::point_type        p3;

        // halfway between two polar points is halfway along the chord
        const pol::point_type a = pol::make_point(1.0, 0.0);
        const pol::point_type b =
            pol::make_point(1.0, dm::half_pi_of<double>());
        const pol::point_type m = dm::cartesian_lerp<pol>(a, b, 0.5);
        const pol::point_type c = pol::to_cartesian(m);

        D_TESTS_EXPECT( (d_tests_near(c[0], 0.5, 1e-15)) &&
                        (d_tests_near(c[1], 0.5, 1e-15)),
                        "cartesian_lerp through polar coordinates");
        D_TESTS_EXPECT(d_tests_near(dm::cartesian_distance<pol>(a, b),
                                    1.4142135623730951, 1e-15),
                       "cartesian_distance through polar coordinates");

        const double xa[3] = { 1.0, 0.0, 0.0 };
        const double ya[3] = { 0.0, 1.0, 0.0 };
        const p3     x(xa);
        const p3     y(ya);

        D_TESTS_EXPECT( (dm::cartesian_cross_3(x, y)[2] == 1.0) &&
                        (dm::cartesian_dot(x, y) == 0.0) &&
                        (dm::cartesian_sub_3(x, y)[1] == -1.0),
                        "the Cartesian products");

        // a box grown from empty
        dm::aabb<3> box = dm::aabb<3>::empty();

        D_TESTS_EXPECT(box.is_empty(), "the empty box is empty");

        box.include(x);
        box.include(y);

        D_TESTS_EXPECT( (!box.is_empty()) &&
                        (box.extent(0) == 1.0) &&
                        (box.extent(2) == 0.0) &&
                        (box.center()[1] == 0.5) &&
                        (box.contains(box.center())) &&
                        (!box.contains(x.scaled(2.0))),
                        "a box grown from empty");

        // Simpson's rule is exact on cubics
        D_TESTS_EXPECT(d_tests_near(dm::integrate_simpson(t_cube(), 0.0, 2.0),
                                    4.0, 1e-13),
                       "Simpson's rule on a cubic");
        // (the literal cast: an x87 compiler reads a float literal at 64
        // bits)
        D_TESTS_EXPECT( (dm::two_pi_of<double>() ==
                         2.0 * dm::pi_of<double>()) &&
                        (dm::pi_of<float>() ==
                         static_cast<float>(3.14159265358979323846)),
                        "the constants");

        return;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    void
    d_tests_cpp11()
    {
        const std::array<double, 3> a = {{ 1.0, 0.0, 0.0 }};
        const std::array<double, 3> b = {{ 0.0, 1.0, 0.0 }};

        // std::array points, as the geometry took before
        D_TESTS_EXPECT( (dm::cartesian_cross_3(a, b)[2] == 1.0) &&
                        (dm::cartesian_dot(a, b) == 0.0),
                        "the products on std::array points");

        const dm::aabb<3> box(a, b);

        D_TESTS_EXPECT(box.extent(0) == -1.0,
                       "a box from std::array corners");

        return;
    }
#endif
}  // namespace

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
static_assert(dm::pi_v<double> == dm::pi_of<double>(), "pi_v (C++14)");
static_assert(dm::is_edge_v<t_edge>, "is_edge_v (C++14)");
#endif


int
main()
{
    d_tests_traits();
    d_tests_helpers();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_cpp11();
#endif

    std::printf("t_geometry_common_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
