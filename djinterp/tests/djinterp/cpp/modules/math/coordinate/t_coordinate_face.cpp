/*******************************************************************************
* djinterp [test]                                          t_coordinate_face.cpp
*
*   Tests for the coordinate headers -- cartesian, polar, cylindrical,
* spherical, coordinate -- at every level from C++98: each system's
* conversions as the C core's, round trips through Cartesian, distances
* against Cartesian distances, canonical forms in their ranges, the scale
* factors and Jacobians, coord_point and coord_cast (through Cartesian and
* directly between cylindrical and spherical), differential's line element,
* the traits, an integer polar system (through double), and constant
* expressions: the exact forms from C++14, a conversion from C++20.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/cpp/modules/math/coordinate/t_coordinate_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/coordinate/t_coordinate_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/coordinate/coordinate.hpp"  // tested
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

    unsigned long g_state = 31337u;

    double d_tests_random()
    {
        g_state = (g_state * 1103515245u) + 12345u;

        return (static_cast<double>((g_state >> 4) % 2000003u) / 250000.0) -
               4.0;
    }

    template<typename Type>
    void d_tests_family(
        Type _tol
    )
    {
        typedef dm::polar<Type>        pol;
        typedef dm::cylindrical<Type>  cyl;
        typedef dm::spherical<Type>    sph;
        typedef dm::cartesian<3, Type> car3;
        typedef dm::cartesian<2, Type> car2;

        for (int trial = 0; trial < 3000; ++trial)
        {
            const Type a = static_cast<Type>(d_tests_random() + 4.5);
            const Type b = static_cast<Type>(d_tests_random() * 0.7);
            const Type c = static_cast<Type>(d_tests_random());

            // each conversion is the C core's
            const typename pol::point_type pp = pol::make_point(a, b);
            const typename pol::point_type pc = pol::to_cartesian(pp);
            Type                           want[2] = { Type(), Type() };

            d_coord_polar_to_cartesian_auto(want, pp.data());
            D_TESTS_EXPECT( (pc[0] == want[0]) &&
                            (pc[1] == want[1]),
                            "polar::to_cartesian is the C core's");

            // round trips through Cartesian, once canonical
            const typename sph::point_type sp =
                sph::canonicalize(sph::make_point(a, b, c));
            const typename sph::point_type sb =
                sph::from_cartesian(sph::to_cartesian(sp));

            D_TESTS_EXPECT( (d_tests_near(sb[0], sp[0], _tol * 16)) &&
                            (d_tests_near(sb[1], sp[1], _tol * 16)),
                            "spherical through Cartesian and back");

            const typename cyl::point_type cp = cyl::make_point(a, b, c);

            D_TESTS_EXPECT(cyl::from_spherical(cyl::to_spherical(cp))
                               .equals(cp, _tol * 16),
                           "cylindrical through spherical and back");

            // distances against Cartesian distances
            const typename cyl::point_type cq = cyl::make_point(c + 5, a, b);

            D_TESTS_EXPECT(d_tests_near(cyl::distance(cp, cq),
                                        car3::distance(cyl::to_cartesian(cp),
                                                       cyl::to_cartesian(cq)),
                                        _tol * 64),
                           "the cylindrical distance is the Cartesian one");
            D_TESTS_EXPECT(d_tests_near(pol::distance(pp, pol::make_point(c + 5,
                                                                          a)),
                                        car2::distance(pc, pol::to_cartesian(
                                            pol::make_point(c + 5, a))),
                                        _tol * 64),
                           "the polar distance is the Cartesian one");

            // canonical forms in their ranges; the measures
            const Type two_pi = static_cast<Type>(6.28318530717958647692L);

            D_TESTS_EXPECT( (sp[0] >= Type()) &&
                            (sp[1] >= Type()) &&
                            (sp[1] <= two_pi / 2) &&
                            (sp[2] >= Type()) &&
                            (sp[2] < two_pi) &&
                            (pol::normalize_angle(b * 10) >= Type()) &&
                            (pol::normalize_angle(b * 10) < two_pi),
                            "the canonical forms' ranges");
            D_TESTS_EXPECT( (d_tests_near(sph::jacobian(sp),
                                          sph::scale_factors(sp).product(),
                                          _tol * 64)) &&
                            (cyl::jacobian(cp) == cp[cyl::RHO]) &&
                            (pol::scale_factors(pp)[1] == pp[pol::R]),
                            "Jacobians are the scale factors' products");

            // coord_point and coord_cast
            const dm::coord_point<cyl> tagged(cp);
            const dm::coord_point<sph> as_sph = dm::coord_cast<sph>(tagged);
            const dm::coord_point<cyl> back   = dm::coord_cast<cyl>(as_sph);

            D_TESTS_EXPECT( (back.raw().equals(cp, _tol * 16)) &&
                            (tagged.to_cartesian().raw() ==
                             cyl::to_cartesian(cp)),
                            "coord_cast, both ways, and to_cartesian");

            // the line element on Cartesian coordinates is the norm
            const typename car3::vector_type dq(a, b, c);

            // (both stored: on an x87 a returned value may carry excess
            // precision until it is)
            volatile Type ds = dm::differential<car3>::line_element(
                                   car3::origin(), dq);
            volatile Type nq = dq.norm();

            D_TESTS_EXPECT(ds == nq, "the Cartesian line element is the norm");
        }

        return;
    }

    void d_tests_traits_and_generic()
    {
        D_TESTS_EXPECT( (dm::is_coord_system<dm::polar<double> >::value) &&
                        (dm::is_coord_system<dm::cartesian<4, float> >
                             ::value) &&
                        (!dm::is_coord_system<int>::value) &&
                        (!dm::is_coord_system<
                             dm::linalg::vector<double, 3> >::value),
                        "is_coord_system");
        D_TESTS_EXPECT( (dm::are_compatible_systems<
                             dm::cylindrical<double>,
                             dm::spherical<double> >::value) &&
                        (!dm::are_compatible_systems<
                             dm::polar<double>,
                             dm::spherical<double> >::value) &&
                        (dm::are_same_system<dm::polar<float>,
                                             dm::polar<float> >::value),
                        "are_compatible_systems, are_same_system");

        // an integer system: through double
        const dm::polar<int>::point_type p = dm::polar<int>::make_point(3, 0);

        D_TESTS_EXPECT( (dm::polar<int>::to_cartesian(p)[0] == 3) &&
                        (dm::polar<int>::to_cartesian(p)[1] == 0),
                        "an int polar system");

        return;
    }
}  // namespace

D_STATIC_ASSERT((dm::spherical<double>::dimension == 3),
                "a spherical system is three-dimensional");
D_STATIC_ASSERT((dm::polar<float>::is_polar && !dm::polar<float>::is_cartesian),
                "polar's flags");

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
static_assert(dm::polar<double>::canonicalize(
                  dm::polar<double>::make_point(-2.0, 0.0))[0] == 2.0,
              "an exact canonical form, from C++14");
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
static_assert(dm::spherical<double>::to_cartesian(
                  dm::spherical<double>::make_point(2.0, 0.0, 0.0))[2] == 2.0,
              "a conversion, from C++20");
#endif


int main()
{
    d_tests_family<float>(1e-4f);
    d_tests_family<double>(1e-12);
    d_tests_family<long double>(1e-12L);
    d_tests_traits_and_generic();

    std::printf("t_coordinate_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
