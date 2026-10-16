/*******************************************************************************
* djinterp [math]                                      test_constants_values.cpp
*
*
* path:      /tests/djinterp/math/test_constants_values.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <cstdio>
// djinterp
#include "djinterp/math/math.hpp"
using namespace djinterp::math;

int main()
{
    // ---- constants (compile-time) ------------------------------------------
    static_assert(constants::pi<double> > 3.14159 && constants::pi<double> < 3.14160, "pi");
    static_assert(constants::two_pi<double> > 6.2831 && constants::two_pi<double> < 6.2833, "two_pi");
    static_assert(constants::e<float> > 2.71f && constants::e<float> < 2.72f, "e");
    static_assert(constants::golden_ratio<double> > 1.6180 && constants::golden_ratio<double> < 1.6181, "phi");
    static_assert(constants::phi<double> == constants::golden_ratio<double>, "phi == golden_ratio");

    // ---- rational: reduction, sign, value, arithmetic ----------------------
    static_assert(rational<6,4>::numerator == 3 && rational<6,4>::denominator == 2, "reduce 6/4 -> 3/2");
    static_assert(rational<6,-4>::numerator == -3 && rational<6,-4>::denominator == 2, "sign onto numerator");
    constexpr rational<3,2> r32{};
    static_assert(r32() == 1.5, "rational value");
    using sum = rational<1,2>::add<1,3>;        // 1/2 + 1/3 = 5/6
    static_assert(sum::numerator == 5 && sum::denominator == 6, "rational add");

    // ---- rational composes as an expression leaf ---------------------------
    constexpr auto e1 = x + rational<3,2>{};    // f(x) = x + 3/2
    static_assert(e1(2.0) == 3.5, "rational in expression");

    // ---- sampling: f(x) = x*x at 0,1,2,3 -> 0,1,4,9 ------------------------
    constexpr auto sq = x * x;
    constexpr auto s = sample<4>(sq, 0.0, 1.0);
    static_assert(s[0]==0.0 && s[1]==1.0 && s[2]==4.0 && s[3]==9.0, "sample");

    // ---- sample_over a discrete interval: first point = lower, then +step --
    using I = interval<int,0,8,false,false,2>;
    constexpr auto s2 = sample_over<I>(x);
    static_assert(s2[0]==0.0 && s2[1]==2.0, "sample_over first/step");

    // ---- folded traits from math_traits.hpp --------------------------------
    static_assert( is_rational<rational<3,2>>::value,         "rational is_rational");
    static_assert(!is_rational<constant_node<double>>::value, "constant_node not rational");
    static_assert( has_number_base<decimal_base<>>::value,    "decimal_base has number base");
    static_assert( is_decimal<decimal_base<>>::value,         "is_decimal");
    static_assert( is_binary<binary_base<>>::value,           "is_binary");
    static_assert( is_hexadecimal<hexadecimal_base<>>::value, "is_hexadecimal");
    static_assert(!is_binary<decimal_base<>>::value,          "decimal not binary");
    static_assert( is_floating_point_type<double>::value,     "double is fp");
    static_assert( is_integer_type<int>::value,               "int is integer");
    static_assert(!is_integer_type<double>::value,            "double not integer");
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert( is_rational_v<rational<1,3>>,              "is_rational_v");
    static_assert( is_decimal_v<decimal_base<>>,             "is_decimal_v");
    static_assert(!is_octal_v<decimal_base<>>,               "is_octal_v negative");
#endif

    printf("OK  pi=%.4f phi=%.4f  r32=%.2f  sample=[%.0f %.0f %.0f %.0f]  s2.size=%zu (s2[last]=%.0f)\n",
           constants::pi<double>, constants::golden_ratio<double>, r32(),
           s[0], s[1], s[2], s[3], s2.size(), s2[s2.size()-1]);
    return 0;
}
