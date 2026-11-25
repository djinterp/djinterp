/*******************************************************************************
* djinterp [cpp][math]                                       t_function_face.cpp
*
*   Tests for math/function/function.hpp, the compile-time core's functions,
* at every level from C++98 (step 30, 5b). Below C++11 the header is empty.
* From C++11: the relations and the logic over them, implicit functions and
* level sets, the axis sets and the old names, each a constant expression;
* piecewise functions, vector-valued functions and parametric curves and
* surfaces, and math_function from a scalar pack, a std::array, a std::tuple
* and a coordinate system's point, each by the builder function(system);
* the detection traits. From C++14: the forms over std::tuple and the calls
* from a std::array, a std::tuple or a point constant expressions too, where
* std::get, std::array's operator[] and the point's constructors are; the _v
* variable templates.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/t_function_face.cpp                  \
*         -o t_function -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_function_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.22
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/function/function.hpp"  // the tested header
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/elementary.h"  // cos_c
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstring>  // std::memcpy


namespace
{
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

}  // namespace


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

namespace
{
    namespace dm = djinterp::math;
    namespace dv = djinterp::math::variables;

    // d_tests_same_bits
    //   whether two doubles have the same bits.
    bool
    d_tests_same_bits(
        double _a,
        double _b
    )
    {
        unsigned char a[sizeof(double)];
        unsigned char b[sizeof(double)];

        std::memcpy(a, &_a, sizeof(double));
        std::memcpy(b, &_b, sizeof(double));

        return (std::memcmp(a, b, sizeof(double)) == 0);
    }

    // d_tests_no_system
    //   a coordinate system with no dimension: the builder takes any
    // parameter count.
    struct d_tests_no_system
    {
    };


    //==========================================================================
    // 1.  RELATIONS AND LOGIC, CONSTANT EXPRESSIONS FROM C++11
    //==========================================================================

    // each relation, with a scalar on either side, and the logic over them
    D_CONSTEXPR bool g_relations[8] = {
        (dv::x <  2.0)(1.0),
        (dv::x <= dv::y)(2.0, 2.0),
        (dv::x >  dv::y)(3.0, 2.0),
        (2.0 >= dv::x)(2.0),
        (dv::x == 1.5)(1.5),
        (dv::x != dv::y)(1.0, 2.0),
        (1.0 < dv::x)(0.5),
        ( (dv::x * 2.0) > (dv::y + 1.0) )(1.0, 2.0) };
    D_CONSTEXPR bool g_logic[6] = {
        ( (dv::x > 0.0) && (dv::y > 0.0) )(1.0, -1.0),
        ( (dv::x > 0.0) || (dv::y > 0.0) )(1.0, -1.0),
        (!(dv::x > 0.0))(-1.0),
        dm::land(dv::x > 0.0, dv::x < 1.0)(0.5),
        dm::lor(dv::x < 0.0, dv::x > 1.0)(0.5),
        dm::lnot(dv::x < 0.0)(0.5) };
    D_CONSTEXPR bool g_folds[4] = {
        dm::all_of(dv::x > 0.0, dv::y > 0.0, dv::z > 0.0)(1.0, 2.0, 3.0),
        dm::all_of(dv::x > 0.0, dv::y > 0.0, dv::z > 0.0)(1.0, 2.0, -3.0),
        dm::any_of(dv::x > 0.0, dv::y > 0.0, dv::z > 0.0)(-1.0, -2.0, 3.0),
        dm::any_of(dv::x > 0.0, dv::y > 0.0, dv::z > 0.0)(-1.0, -2.0, -3.0) };

    // d_tests_relations
    //   the relations' and the logic's values, folded at compile time; their
    // arities and markers.
    void
    d_tests_relations()
    {
        D_TESTS_EXPECT( (g_relations[0]) && (g_relations[1]) &&
                        (g_relations[2]) && (g_relations[3]) &&
                        (g_relations[4]) && (g_relations[5]) &&
                        (!g_relations[6]) && (!g_relations[7]),
                        "< <= > >= == != over expressions, a scalar on "
                        "either side, each a constant expression from C++11");
        D_TESTS_EXPECT( (!g_logic[0]) && (g_logic[1]) && (g_logic[2]) &&
                        (g_logic[3]) && (!g_logic[4]) && (g_logic[5]),
                        "&& || ! and land, lor, lnot, each a constant "
                        "expression from C++11");
        D_TESTS_EXPECT( (g_folds[0]) && (!g_folds[1]) &&
                        (g_folds[2]) && (!g_folds[3]),
                        "all_of and any_of over three predicates, constant "
                        "expressions from C++11");

        typedef decltype(dv::x < dv::z)                       rel_type;
        typedef decltype((dv::x > 0.0) && (dv::y > 0.0))      and_type;
        typedef decltype(dv::x + dv::y)                       sum_type;

        D_TESTS_EXPECT( (rel_type::arity == 3) &&
                        (and_type::arity == 2) &&
                        (dm::is_inequality<rel_type>::value) &&
                        (dm::is_inequality<and_type>::value) &&
                        (!dm::is_inequality<sum_type>::value) &&
                        (!dm::is_inequality<double>::value) &&
                        (dm::is_expression<rel_type>::value) &&
                        (sizeof(rel_type::value_type) == sizeof(bool)),
                        "a relation's arity its operands' most, its value a "
                        "bool; is_inequality of relations and logic alone");

        return;
    }


    //==========================================================================
    // 2.  IMPLICIT FUNCTIONS AND LEVEL SETS, CONSTANT EXPRESSIONS FROM C++11
    //==========================================================================

    D_CONSTEXPR double g_residual =
        dm::implicit( (dv::x * dv::x) + (dv::y * dv::y) - 25.0 )(3.0, 4.0);
    D_CONSTEXPR int    g_signs[3] = {
        dm::implicit(dv::x - 1.0).sign(3.0),
        dm::implicit(dv::x - 1.0).sign(1.0),
        dm::implicit(dv::x - 1.0).sign(-3.0) };
    D_CONSTEXPR bool   g_contains[3] = {
        dm::level_set(dv::x * dv::x, 4.0).contains(1e-12, 2.0),
        dm::level_set(dv::x * dv::x, 4.0).contains(1e-12, 2.001),
        dm::level_set(dv::x + dv::y, 1.0).contains(0.25, 0.4, 0.4) };

    // d_tests_implicit
    //   residuals, signs and membership, folded at compile time.
    void
    d_tests_implicit()
    {
        typedef decltype(dm::implicit(dv::x - dv::y)) implicit_type;

        D_TESTS_EXPECT( (g_residual == 0.0) &&
                        (g_signs[0] == 1) &&
                        (g_signs[1] == 0) &&
                        (g_signs[2] == -1),
                        "an implicit function's residual and the side of its "
                        "zero set a point is on, constant expressions from "
                        "C++11");
        D_TESTS_EXPECT( (g_contains[0]) &&
                        (!g_contains[1]) &&
                        (g_contains[2]),
                        "level_set(F, c) holds F = c within epsilon, a "
                        "constant expression from C++11");
        D_TESTS_EXPECT( (dm::is_implicit<implicit_type>::value) &&
                        (implicit_type::arity == 2) &&
                        (!dm::is_implicit<decltype(dv::x - dv::y)>::value),
                        "is_implicit of implicit functions alone");

        return;
    }


    //==========================================================================
    // 3.  AXIS SETS AND THE OLD NAMES, CONSTANT EXPRESSIONS FROM C++11
    //==========================================================================

    D_CONSTEXPR double g_axes[6] = {
        dm::cartesian_axes::w(1.0, 2.0, 3.0, 4.0),
        dm::polar_axes::theta(1.0, 2.0),
        dm::cylindrical_axes::phi(1.0, 2.0, 3.0),
        dm::spherical_axes::phi(1.0, 2.0, 3.0),
        dm::cylindrical_axes::z(1.0, 2.0, 3.0),
        dm::spherical_axes::r(1.0, 2.0, 3.0) };

    // d_tests_axes
    //   each system's placeholders at the arguments the system uses; the old
    // positional names.
    void
    d_tests_axes()
    {
        D_TESTS_EXPECT( (g_axes[0] == 4.0) &&
                        (g_axes[1] == 2.0) &&
                        (g_axes[2] == 2.0) &&
                        (g_axes[3] == 3.0) &&
                        (g_axes[4] == 3.0) &&
                        (g_axes[5] == 1.0),
                        "w the fourth argument; phi the second in "
                        "cylindrical, the third in spherical; constant "
                        "expressions from C++11");
        D_TESTS_EXPECT( (dm::mvar_x<>()(1.0, 2.0, 3.0, 4.0) == 1.0) &&
                        (dm::mvar_y<>()(1.0, 2.0, 3.0, 4.0) == 2.0) &&
                        (dm::mvar_z<>()(1.0, 2.0, 3.0, 4.0) == 3.0) &&
                        (dm::mvar_w<>()(1.0, 2.0, 3.0, 4.0) == 4.0) &&
                        (dm::mvar_u<>()(1.0, 2.0) == 1.0) &&
                        (dm::mvar_v<>()(1.0, 2.0) == 2.0) &&
                        (std::is_same<dm::multivariable<double, 2>,
                                      dm::variable_node<2, double> >::value),
                        "the old names mvar_x to mvar_v and multivariable, "
                        "the variables at their arguments");

        return;
    }


    //==========================================================================
    // 4.  PIECEWISE, VECTOR-VALUED AND PARAMETRIC FORMS
    //==========================================================================

    // d_tests_forms
    //   the forms over std::tuple, at run time from C++11; their markers and
    // dimensions.
    void
    d_tests_forms()
    {
        const auto pw = dm::piecewise(dm::when(dv::x < 0.0, -dv::x),
                                      dm::when(dv::x < 1.0, dv::x * dv::x),
                                      dm::otherwise(1.0));
        const auto v  = dm::vec(dv::x, dv::x * 2.0, 3.0);
        const auto vf = dm::vector_function(dv::x + dv::y, dv::x - dv::y);
        const auto c  = dm::parametric_curve(dv::x * 2.0, dv::x + 1.0);
        const auto s  = dm::parametric_surface(dv::x, dv::y, dv::x * dv::y);

        const std::array<double, 3> at_v  = v(1.5);
        const std::array<double, 2> at_vf = vf(3.0, 1.0);
        const std::array<double, 2> at_c  = c(2.0);
        const std::array<double, 3> at_s  = s(2.0, 3.0);

        typedef decltype(pw) pw_type;
        typedef decltype(v)  v_type;
        typedef decltype(c)  c_type;
        typedef decltype(s)  s_type;

        D_TESTS_EXPECT( (pw(-3.0) == 3.0) &&
                        (pw(0.5) == 0.25) &&
                        (pw(4.0) == 1.0) &&
                        (pw(0.0) == 0.0) &&
                        (pw(1.0) == 1.0),
                        "a piecewise function takes its first branch that "
                        "holds, the last its fallback");
        D_TESTS_EXPECT( (at_v[0] == 1.5) && (at_v[1] == 3.0) &&
                        (at_v[2] == 3.0) &&
                        (at_vf[0] == 4.0) && (at_vf[1] == 2.0),
                        "vec and vector_function give each component's "
                        "value, a scalar component lifted");
        D_TESTS_EXPECT( (at_c[0] == 4.0) && (at_c[1] == 3.0) &&
                        (at_s[0] == 2.0) && (at_s[1] == 3.0) &&
                        (at_s[2] == 6.0),
                        "a parametric curve at t and a surface at (u, v)");
        D_TESTS_EXPECT( (dm::is_piecewise<pw_type>::value) &&
                        (pw_type::num_pieces == 3) &&
                        (pw_type::arity == 1) &&
                        (dm::is_vector_valued<v_type>::value) &&
                        (v_type::output_dimension == 3) &&
                        (!dm::is_parametric<v_type>::value) &&
                        (dm::is_parametric<c_type>::value) &&
                        (dm::is_vector_valued<c_type>::value) &&
                        (c_type::parameter_count == 1) &&
                        (s_type::parameter_count == 2) &&
                        (s_type::arity == 2) &&
                        (s_type::output_dimension == 3) &&
                        (!dm::is_piecewise<v_type>::value),
                        "the forms' markers, piece counts, dimensions and "
                        "parameter counts");

        return;
    }


    //==========================================================================
    // 5.  MATH_FUNCTION AND THE BUILDER
    //==========================================================================

    // d_tests_builder
    //   math_function from each kind of argument, by the builder over each
    // system; the system traits and the accessors.
    void
    d_tests_builder()
    {
        const auto plane  = dm::function(dm::cartesian<2>())
                                .of(dv::x, dv::y)
                                .returns( (dv::x * 3.0) + dv::y );
        const auto volume = dm::function(dm::cartesian<3>())
                                .returns( dv::x * (dv::y * dv::z) );
        const auto radial = dm::function(dm::polar<>())
                                .returns( dm::polar_axes::r *
                                          dm::cos(dm::polar_axes::theta) );
        const auto free_1 = dm::function(d_tests_no_system())
                                .of(dv::x)
                                .returns(dv::x + 1.0);
        const auto free_0 = dm::function(d_tests_no_system())
                                .returns(dm::constant(5.0));

        const std::array<double, 2> pair = {{ 2.0, 1.0 }};
        const dm::cartesian<3>::point_type point(2.0, 3.0, 4.0);
        const dm::polar<>::point_type      polar_point(2.0, 0.5);

        D_TESTS_EXPECT( (plane(2.0, 1.0) == 7.0) &&
                        (plane(pair) == 7.0) &&
                        (plane(std::make_tuple(2.0, 1.0)) == 7.0) &&
                        (volume(point) == 24.0) &&
                        (volume(2.0, 3.0, 4.0) == 24.0),
                        "math_function from a scalar pack, a std::array, a "
                        "std::tuple and a system's point, a linalg::vector");
        D_TESTS_EXPECT( (d_tests_same_bits(radial(polar_point),
                                           2.0 * d_math_cos_c(0.5))) &&
                        (free_1(1.0) == 2.0) &&
                        (free_0() == 5.0),
                        "over polar, its point's r and theta; over a system "
                        "with no dimension, any parameter count");
        D_TESTS_EXPECT( (decltype(plane)::arity == 2) &&
                        (decltype(volume)::arity == 3) &&
                        (decltype(radial)::arity == 2) &&
                        (decltype(free_1)::arity == 1) &&
                        (decltype(free_0)::arity == 0) &&
                        (dm::is_function<decltype(plane)>::value) &&
                        (!dm::is_function<decltype(dv::x + 1.0)>::value) &&
                        (std::is_same<decltype(volume)::system_type,
                                      dm::cartesian<3> >::value),
                        "the arity .of(...) declares, else the system's "
                        "dimension; is_function of math_functions alone");
        D_TESTS_EXPECT( (dm::is_cartesian_system<dm::cartesian<3> >::value) &&
                        (dm::is_polar_system<dm::polar<> >::value) &&
                        (dm::is_cylindrical_system<
                             dm::cylindrical<> >::value) &&
                        (dm::is_spherical_system<dm::spherical<> >::value) &&
                        (!dm::is_polar_system<dm::cartesian<2> >::value) &&
                        (!dm::is_cartesian_system<d_tests_no_system>::value) &&
                        (dm::is_coordinate_system<dm::spherical<> >::value) &&
                        (!dm::is_coordinate_system<double>::value),
                        "each system's trait by its marker; "
                        "is_coordinate_system of systems alone");
        D_TESTS_EXPECT( (dm::function_arity<decltype(dv::x + dv::z)>::value ==
                             3) &&
                        (std::is_same<
                             dm::function_value_type<decltype(dv::x + 1.0)>,
                             double>::value) &&
                        (dm::internal::system_dimension<
                             dm::spherical<> >::value == 3) &&
                        (dm::internal::system_dimension<
                             d_tests_no_system>::value == 0),
                        "function_arity and function_value_type; a system's "
                        "dimension, 0 where it has none");

        return;
    }


    //==========================================================================
    // 6.  CONSTANT EXPRESSIONS FROM C++14
    //==========================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CONSTEXPR double g_piecewise =
        dm::piecewise(dm::when(dv::x < 0.0, -dv::x),
                      dm::otherwise(dv::x * 2.0))(-3.0);
    D_CONSTEXPR std::array<double, 2> g_vector =
        dm::vec(dv::x + dv::y, dv::x * dv::y)(2.0, 5.0);
    D_CONSTEXPR std::array<double, 2> g_curve =
        dm::parametric_curve(dv::x, dv::x * dv::x)(3.0);
    D_CONSTEXPR double g_from_array =
        dm::function(dm::cartesian<2>()).returns(dv::x - dv::y)(
            std::array<double, 2>{{ 5.0, 2.0 }});
    D_CONSTEXPR double g_from_tuple =
        dm::function(dm::cartesian<2>()).returns(dv::x - dv::y)(
            std::make_tuple(5.0, 2.0));
    D_CONSTEXPR double g_from_point =
        dm::function(dm::cartesian<3>()).returns(dv::x + (dv::y * dv::z))(
            dm::cartesian<3>::point_type(1.0, 2.0, 3.0));

    // d_tests_cpp14
    //   the forms over std::tuple and math_function's container calls,
    // folded at compile time from C++14.
    void
    d_tests_cpp14()
    {
        D_TESTS_EXPECT( (g_piecewise == 3.0) &&
                        (g_vector[0] == 7.0) && (g_vector[1] == 10.0) &&
                        (g_curve[0] == 3.0) && (g_curve[1] == 9.0),
                        "piecewise, vec and a parametric curve constant "
                        "expressions from C++14, where std::get is");
        D_TESTS_EXPECT( (g_from_array == 3.0) &&
                        (g_from_tuple == 3.0) &&
                        (g_from_point == 7.0),
                        "math_function from a std::array, a std::tuple and a "
                        "point constant expressions from C++14");

    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        D_TESTS_EXPECT( (dm::is_inequality_v<decltype(dv::x < 1.0)>) &&
                        (!dm::is_piecewise_v<double>) &&
                        (dm::is_cartesian_system_v<dm::cartesian<2> >) &&
                        (dm::is_coordinate_system_v<dm::polar<> >),
                        "the traits' _v variable templates, where variable "
                        "templates are");
    #endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

        return;
    }
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    D_TESTS_EXPECT(DJINTERP_MATH_FUNCTION_FUNCTION_HPP == 1,
                   "the header reads at every level, empty below C++11");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_relations();
    d_tests_implicit();
    d_tests_axes();
    d_tests_forms();
    d_tests_builder();
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    d_tests_cpp14();
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

    std::printf("t_function_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
