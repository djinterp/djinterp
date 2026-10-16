/*******************************************************************************
* djinterp [math]                                                       test.cpp
*
*
* path:      /tests/djinterp/math/test.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <cmath>
// test.cpp - exercises the merged function.hpp:
//   * the two original examples (compile-time AND runtime, vs libm)
//   * array/tuple evaluation on math_function
//   * real coordinate systems bound into the builder (correct axis indices)
//   * piecewise / vector / parametric / implicit forms
//   * the detection traits folded in from function_traits.hpp
#include <cstdio>
#include <tuple>
#include <type_traits>
// djinterp
#include "djinterp/math/function/function.hpp"

using namespace djinterp::math;

constexpr double PI        = 3.141592653589793238462643383279502884;
constexpr double PI_OVER_6 = PI / 6.0;

constexpr bool
approx(double _a, double _b, double _tol) noexcept
{
    double d = _a - _b;
    if (d < 0.0) { d = -d; }
    return d <= _tol;
}

// a vec3-valued position variable at argument index 4 (after x,y,z,t).
constexpr variable_node<4, vec3> p{};

int
main()
{
    // =======================================================================
    // EXAMPLE 1 - builder form, now on the REAL cylindrical<> system.
    //   cylindrical<>::dimension == 3, so .of(r, theta, z) validates.
    // =======================================================================
    constexpr double A = 2.0, sigma = 1.5, k = 3.0, h = 4.0;

    constexpr auto f1 =
        function(cylindrical<>{})
            .of(r, theta, z)
            .returns( A * exp(-squared(r) / squared(sigma))
                        * cos(k * theta)
                        * (constant(1.0) - squared(z) / squared(h)) );

    constexpr double c1 = f1(0.5, 0.2, 1.0);
    static_assert(c1 == f1(0.5, 0.2, 1.0), "example1 must be constant-evaluable");
    static_assert(f1.arity == 3, "example1 arity");
    static_assert(is_function_v<decltype(f1)>, "f1 is a math_function");
    static_assert(std::is_same<decltype(f1)::coordinate_system,
                               cylindrical<>>::value, "f1 carries its system");

    double r1   = f1(0.5, 0.2, 1.0);
    double ref1 = A * std::exp(-(0.5 * 0.5) / (sigma * sigma))
                    * std::cos(k * 0.2)
                    * (1.0 - (1.0 * 1.0) / (h * h));

    std::printf("example1: runtime=%.12f  compiletime=%.12f  libm_ref=%.12f\n",
                r1, c1, ref1);

    // =======================================================================
    // EXAMPLE 2 - operator form + hybrid sub-expression + vector dot.
    // =======================================================================
    constexpr auto r2 = squared(x) + squared(y) + squared(z);

    constexpr auto f2 =
          constant(2.0)
        * exp(-r2 / (constant(2.0) * squared(constant(1.25))))
        * sin( dot(value(vec3{0.2, 0.7, -0.4}), p)
               - constant(0.75) * t
               + constant(PI_OVER_6) )
        + sqrt(squared(x) + squared(y)) / (constant(1.0) + abs(z));

    constexpr double X = 0.1, Y = 0.2, Z = 0.3, T = 0.75;
    constexpr vec3   P = vec3{ 1.0, 0.0, 0.0 };

    constexpr double c2 = f2(X, Y, Z, T, P);
    static_assert(c2 == f2(X, Y, Z, T, P), "example2 must be constant-evaluable");

    double r2v  = f2(X, Y, Z, T, P);
    double rr2  = X * X + Y * Y + Z * Z;
    double dotv = 0.2 * P.x + 0.7 * P.y + (-0.4) * P.z;
    double ref2 = 2.0
                * std::exp(-rr2 / (2.0 * (1.25 * 1.25)))
                * std::sin(dotv - 0.75 * T + PI_OVER_6)
                + std::sqrt(X * X + Y * Y) / (1.0 + std::fabs(Z));

    std::printf("example2: runtime=%.12f  compiletime=%.12f  libm_ref=%.12f\n",
                r2v, c2, ref2);

    // =======================================================================
    // 3 - REAL coordinate system + correctly-indexed axes + array/tuple eval.
    //   g(rho, phi, z) = rho*cos(phi) + z, evaluated three equivalent ways.
    //   Demonstrates that phi is argument 1 here (cylindrical), not argument 2.
    // =======================================================================
    constexpr auto g =
        function(cylindrical<>{})
            .of(cylindrical_axes::rho, cylindrical_axes::phi, cylindrical_axes::z)
            .returns( cylindrical_axes::rho * cos(cylindrical_axes::phi)
                      + cylindrical_axes::z );

    constexpr double cg = g(2.0, 0.0, 5.0);                       // 2*cos(0)+5 = 7
    static_assert(cg > 6.999 && cg < 7.001, "cylindrical g compile-time");

    double g_pack  = g(2.0, 0.0, 5.0);
    double g_array = g(std::array<double, 3>{{ 2.0, 0.0, 5.0 }}); // coordinate point
    double g_tuple = g(std::make_tuple(2.0, 0.0, 5.0));

    std::printf("section3: pack=%.6f array=%.6f tuple=%.6f (expected 7)\n",
                g_pack, g_array, g_tuple);

    // phi index differs by system: cylindrical phi -> arg 1, spherical phi -> arg 2
    static_assert(function_arity<decltype(cylindrical_axes::phi)>::value == 2,
                  "cylindrical phi is argument 1");
    static_assert(function_arity<decltype(spherical_axes::phi)>::value == 3,
                  "spherical phi is argument 2");

    // =======================================================================
    // 4 - PIECEWISE: abs(x) = { -x if x < 0 ; x otherwise }.
    // =======================================================================
    constexpr auto absf = piecewise( when(x < 0.0, -x),
                                     otherwise(x) );

    constexpr double a_neg = absf(-3.0);
    constexpr double a_pos = absf( 2.5);
    static_assert(a_neg > 2.999 && a_neg < 3.001, "piecewise negative branch");
    static_assert(a_pos > 2.499 && a_pos < 2.501, "piecewise fallback branch");
    static_assert(is_piecewise_v<decltype(absf)>, "absf is piecewise");
    static_assert(decltype(absf)::num_pieces == 2, "piece count");

    // =======================================================================
    // 5 - VECTOR-VALUED: vf(x, y) = (x + y, x - y).
    // =======================================================================
    constexpr auto vf = vec(x + y, x - y);
    constexpr auto vv = vf(3.0, 1.0);                            // {4, 2}
    static_assert(vv[0] > 3.999 && vv[0] < 4.001, "vector component 0");
    static_assert(vv[1] > 1.999 && vv[1] < 2.001, "vector component 1");
    static_assert(is_vector_valued_v<decltype(vf)>, "vf is vector-valued");
    static_assert(decltype(vf)::output_dimension == 2, "vector output dim");

    // =======================================================================
    // 6 - PARAMETRIC CURVE: circle(t) = (cos t, sin t), parameter at index 0.
    // =======================================================================
    constexpr auto circle = parametric_curve( cos(var<0>), sin(var<0>) );
    constexpr auto pt = circle(0.0);                             // {1, 0}
    static_assert(pt[0] > 0.999 && pt[0] < 1.001, "circle x(0)");
    static_assert(pt[1] > -0.001 && pt[1] < 0.001, "circle y(0)");
    static_assert(is_parametric_v<decltype(circle)>, "circle is parametric");
    static_assert(is_vector_valued_v<decltype(circle)>, "parametric is vector too");
    static_assert(decltype(circle)::parameter_count == 1, "one parameter");
    static_assert(decltype(circle)::arity == 1, "arity equals parameter count");

    // =======================================================================
    // 7 - IMPLICIT: unit circle F(x, y) = x^2 + y^2 - 1 = 0.
    // =======================================================================
    constexpr auto circ = implicit( squared(x) + squared(y) - 1.0 );
    static_assert(is_implicit_v<decltype(circ)>, "circ is implicit");
    static_assert(circ.sign(0.5, 0.5) == -1, "inside the circle");
    static_assert(circ.sign(1.0, 1.0) ==  1, "outside the circle");

    bool on_curve = circ.contains(1e-9, 0.6, 0.8);               // 0.36+0.64-1 ~ 0
    std::printf("section7: (0.6,0.8) on unit circle? %s  sign(0.5,0.5)=%d\n",
                on_curve ? "yes" : "no", circ.sign(0.5, 0.5));

    // =======================================================================
    // 8 - DETECTION TRAITS folded in from function_traits.hpp.
    // =======================================================================
    static_assert(is_coordinate_system_v<cylindrical<>>,  "cylindrical is a system");
    static_assert(is_cylindrical_system_v<cylindrical<>>, "cylindrical predicate");
    static_assert(!is_spherical_system_v<cylindrical<>>,  "cylindrical is not spherical");
    static_assert(is_spherical_system_v<spherical<>>,     "spherical predicate");
    static_assert(is_cartesian_system_v<cartesian<3>>,    "cartesian predicate");

    static_assert(is_inequality_v<decltype(x < 0.0)>,             "relation -> inequality");
    static_assert(is_inequality_v<decltype((x < 0.0) && (y > 0.0))>, "logical -> inequality");
    static_assert(!is_vector_valued_v<decltype(absf)>,           "piecewise not vector");
    static_assert(!is_piecewise_v<decltype(vf)>,                 "vector not piecewise");
    static_assert(is_expression_v<decltype(x + y)>,              "x+y is an expression");
    static_assert(function_arity<decltype(g)>::value == 3,       "function_arity of g");

    // =======================================================================
    // RUNTIME vs LIBM CROSS-CHECK (examples 1 and 2)
    // =======================================================================
    if (!approx(r1, ref1, 1e-12) || !approx(r2v, ref2, 1e-12))
    {
        std::printf("FAIL: runtime path disagrees with libm reference\n");
        return 1;
    }
    if (!approx(c1, ref1, 1e-6) || !approx(c2, ref2, 1e-6))
    {
        std::printf("FAIL: compile-time kernel outside tolerance\n");
        return 1;
    }
    if (!approx(g_pack, 7.0, 1e-12) ||
        !approx(g_array, 7.0, 1e-12) ||
        !approx(g_tuple, 7.0, 1e-12))
    {
        std::printf("FAIL: array/tuple/pack evaluation disagree\n");
        return 1;
    }
    if (!on_curve)
    {
        std::printf("FAIL: implicit contains() rejected an on-curve point\n");
        return 1;
    }

    std::printf("ALL CHECKS PASSED\n");
    return 0;
}
