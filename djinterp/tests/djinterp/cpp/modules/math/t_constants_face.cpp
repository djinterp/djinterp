/*******************************************************************************
* djinterp [cpp/math]                                       t_constants_face.cpp
*
*   Tests for math/constants.hpp at every level from C++98: each of the
* nineteen constants, in float and double, is the correctly rounded value,
* from a 120-digit computation, and in long double too where it has 64 bits;
* the exact relations among them hold (two_pi is 2 pi, inv_sqrt2 sqrt2 / 2);
* e, ln2, ln10, sqrt2 and sqrt3 are what the C core's correctly rounded
* exp_c, log_c and sqrt_c give for 1, 2, 10, 2 and 3. number_base's members
* and the number traits, over the library's types and a user's, at every
* level -- the values the header gave before its port, at C++14 and up,
* where it compiled, but that a non-static radix is no radix. From C++11 the
* constants are constant expressions; from C++14 the variable templates are
* of's constants and rational composes as it did.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/t_constants_face.cpp -o t_constants -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_constants_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.08
*                                                            revised: 2026.10.08
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/constants.hpp"  // tested
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/exponential.h"  // d_math_*_exp_c,
                                                        // _log_c, _sqrt_c
// std
#include <cfloat>   // LDBL_MANT_DIG
#include <cstdio>   // std::printf
#include <cstring>  // std::strcmp
// re_std
#include "../../../../../inc/re_std/cpp/type_traits/is_same.hpp"  // re_std::is_same


namespace
{
    namespace dm = djinterp::math;
    namespace dk = djinterp::math::constants;

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

    // stored first, so that an x87's excess precision cannot tell two equal
    // values apart
    template<typename Float>
    bool
    d_tests_same(Float _a, Float _b)
    {
        volatile Float a = _a;
        volatile Float b = _b;

        return (a == b);
    }

    // the correctly rounded float, double and 64-bit long double of each
    // constant, in of's order: the shortest decimals that round back to them
    struct d_tests_row
    {
        const char* name;
        float       f;
        double      d;
        long double ld;
    };

    const d_tests_row g_rows[] = {
        { "pi",           3.1415927e+0f, 3.141592653589793e+0,
                          3.1415926535897932385e+0L },
        { "two_pi",       6.2831855e+0f, 6.283185307179586e+0,
                          6.283185307179586477e+0L },
        { "half_pi",      1.5707964e+0f, 1.5707963267948966e+0,
                          1.5707963267948966193e+0L },
        { "quarter_pi",   7.853982e-1f,  7.853981633974483e-1,
                          7.8539816339744830963e-1L },
        { "inv_pi",       3.1830987e-1f, 3.183098861837907e-1,
                          3.1830988618379067154e-1L },
        { "tau",          6.2831855e+0f, 6.283185307179586e+0,
                          6.283185307179586477e+0L },
        { "e",            2.7182817e+0f, 2.718281828459045e+0,
                          2.7182818284590452354e+0L },
        { "ln2",          6.931472e-1f,  6.931471805599453e-1,
                          6.9314718055994530943e-1L },
        { "ln10",         2.3025851e+0f, 2.302585092994046e+0,
                          2.302585092994045684e+0L },
        { "log2e",        1.442695e+0f,  1.4426950408889634e+0,
                          1.4426950408889634074e+0L },
        { "log10e",       4.342945e-1f,  4.342944819032518e-1,
                          4.3429448190325182765e-1L },
        { "sqrt2",        1.4142135e+0f, 1.4142135623730951e+0,
                          1.4142135623730950488e+0L },
        { "sqrt3",        1.7320508e+0f, 1.7320508075688772e+0,
                          1.7320508075688772936e+0L },
        { "inv_sqrt2",    7.0710677e-1f, 7.071067811865476e-1,
                          7.071067811865475244e-1L },
        { "euler_gamma",  5.772157e-1f,  5.772156649015329e-1,
                          5.772156649015328606e-1L },
        { "golden_ratio", 1.618034e+0f,  1.618033988749895e+0,
                          1.6180339887498948482e+0L },
        { "phi",          1.618034e+0f,  1.618033988749895e+0,
                          1.6180339887498948482e+0L },
        { "rad_per_deg",  1.7453292e-2f, 1.7453292519943295e-2,
                          1.7453292519943295769e-2L },
        { "deg_per_rad",  5.729578e+1f,  5.729577951308232e+1,
                          5.7295779513082320877e+1L }
    };

    // d_tests_of
    //   of<Type>'s nineteen constants, in the table's order.
    template<typename Type>
    void
    d_tests_of(Type* _out)
    {
        typedef dk::of<Type> k;

        _out[0]  = k::pi();
        _out[1]  = k::two_pi();
        _out[2]  = k::half_pi();
        _out[3]  = k::quarter_pi();
        _out[4]  = k::inv_pi();
        _out[5]  = k::tau();
        _out[6]  = k::e();
        _out[7]  = k::ln2();
        _out[8]  = k::ln10();
        _out[9]  = k::log2e();
        _out[10] = k::log10e();
        _out[11] = k::sqrt2();
        _out[12] = k::sqrt3();
        _out[13] = k::inv_sqrt2();
        _out[14] = k::euler_gamma();
        _out[15] = k::golden_ratio();
        _out[16] = k::phi();
        _out[17] = k::rad_per_deg();
        _out[18] = k::deg_per_rad();

        return;
    }

    // d_tests_relations
    //   the relations that hold exactly in binary: a power of two's scaling
    // of a correctly rounded value is the correctly rounded scaling.
    template<typename Type>
    void
    d_tests_relations(const char* _type)
    {
        typedef dk::of<Type> k;

        D_TESTS_EXPECT( (d_tests_same(k::two_pi(), k::pi() * 2)) &&
                        (d_tests_same(k::half_pi(), k::pi() / 2)) &&
                        (d_tests_same(k::quarter_pi(), k::pi() / 4)) &&
                        (d_tests_same(k::tau(), k::two_pi())) &&
                        (d_tests_same(k::phi(), k::golden_ratio())) &&
                        (d_tests_same(k::inv_sqrt2(), k::sqrt2() / 2)),
                        _type);

        return;
    }

    void
    d_tests_constants()
    {
        const std::size_t count = sizeof(g_rows) / sizeof(g_rows[0]);
        float             f[19];
        double            d[19];
        long double       ld[19];
        std::size_t       i = 0;

        d_tests_of(f);
        d_tests_of(d);
        d_tests_of(ld);

        for (i = 0; i < count; ++i)
        {
            D_TESTS_EXPECT(d_tests_same(f[i], g_rows[i].f), g_rows[i].name);
            D_TESTS_EXPECT(d_tests_same(d[i], g_rows[i].d), g_rows[i].name);
#if (LDBL_MANT_DIG == 64)
            D_TESTS_EXPECT(d_tests_same(ld[i], g_rows[i].ld), g_rows[i].name);
#endif
        }

        d_tests_relations<float>("float relations");
        d_tests_relations<double>("double relations");
        d_tests_relations<long double>("long double relations");

        // the C core's correctly rounded kernels at 1, 2, 10, 2 and 3
        D_TESTS_EXPECT( (d_tests_same(dk::of<double>::e(),
                                      d_math_d_exp_c(1.0))) &&
                        (d_tests_same(dk::of<double>::ln2(),
                                      d_math_d_log_c(2.0))) &&
                        (d_tests_same(dk::of<double>::ln10(),
                                      d_math_d_log_c(10.0))) &&
                        (d_tests_same(dk::of<double>::sqrt2(),
                                      d_math_d_sqrt_c(2.0))) &&
                        (d_tests_same(dk::of<double>::sqrt3(),
                                      d_math_d_sqrt_c(3.0))),
                        "double: e, ln2, ln10, sqrt2, sqrt3 as the C core's");
        D_TESTS_EXPECT( (d_tests_same(dk::of<float>::e(),
                                      d_math_f_exp_c(1.0f))) &&
                        (d_tests_same(dk::of<float>::ln2(),
                                      d_math_f_log_c(2.0f))) &&
                        (d_tests_same(dk::of<float>::ln10(),
                                      d_math_f_log_c(10.0f))) &&
                        (d_tests_same(dk::of<float>::sqrt2(),
                                      d_math_f_sqrt_c(2.0f))),
                        "float: e, ln2, ln10, sqrt2 as the C core's");
        D_TESTS_EXPECT(d_tests_same(dk::of<long double>::sqrt2(),
                                    d_math_ld_sqrt_c(2.0L)),
                       "long double: sqrt2 as the C core's");

        // the default type is double; any arithmetic type converts once
        D_TESTS_EXPECT( (d_tests_same(dk::of<>::pi(), d[0])) &&
                        (dk::of<int>::pi() == 3) &&
                        (dk::of<int>::deg_per_rad() == 57),
                        "of<> is of<double>; of<int> truncates");

        return;
    }

    // ---- number bases and traits -------------------------------------------

    // a user's types: a base, a rational, half a rational, a radix that is
    // a function, and one that is not static -- which counts as absent at
    // every level (before the port it counted from C++14, the only level the
    // header compiled at, and by name alone it would at C++98 with GCC but
    // not with Clang)
    struct t_user16
    {
        static const std::size_t radix = 16;
    };

    struct t_user_rational
    {
        static const int numerator   = 1;
        static const int denominator = 3;
    };

    struct t_only_numerator
    {
        static const int numerator = 1;
    };

    struct t_radix_function
    {
        static int radix() { return 2; }
    };

    struct t_radix_member
    {
        int radix;
    };

    // d_tests_traits_of
    //   the ten traits' values for Type, as one number: bit 9 is_rational ...
    // bit 0 is_floating_point_type, is_using_base at 7 between them.
    template<typename Type>
    unsigned
    d_tests_traits_of()
    {
        return ( (dm::is_rational<Type>::value            ? 512u : 0u) |
                 (dm::has_number_base<Type>::value        ? 256u : 0u) |
                 (dm::is_binary<Type>::value              ? 128u : 0u) |
                 (dm::is_octal<Type>::value               ?  64u : 0u) |
                 (dm::is_decimal<Type>::value             ?  32u : 0u) |
                 (dm::is_hexadecimal<Type>::value         ?  16u : 0u) |
                 (dm::is_using_base<Type, 7>::value       ?   8u : 0u) |
                 (dm::is_integer_type<Type>::value        ?   4u : 0u) |
                 (dm::is_real_number<Type>::value         ?   2u : 0u) |
                 (dm::is_floating_point_type<Type>::value ?   1u : 0u) );
    }

    void
    d_tests_number_bases()
    {
        typedef dm::number_base<int, 7>             b7;
        typedef dm::number_base<unsigned char, 16>  b16;
        typedef dm::number_base<std::size_t, 10>    b10;
        typedef dm::number_base<long, 2>            b2;
        typedef dm::number_base<short, 8>           b8;
        // odr-used: the definition below C++17 must exist
        const std::size_t* radix = &b7::radix;

        D_TESTS_EXPECT( (*radix == 7u) &&
                        (b7::max_digit() == 6) &&
                        (!b7::is_valid_digit(-1)) &&
                        (b7::is_valid_digit(0)) &&
                        (b7::is_valid_digit(6)) &&
                        (!b7::is_valid_digit(7)),
                        "number_base<int, 7>");
        D_TESTS_EXPECT( (b16::max_digit() == 15) &&
                        (b16::is_valid_digit(15)) &&
                        (!b16::is_valid_digit(16)) &&
                        (b10::is_valid_digit(9u)) &&
                        (!b10::is_valid_digit(static_cast<std::size_t>(-1))),
                        "number_base over unsigned types");

        // the values the header gave before its port, at C++14 and up, but
        // for the non-static radix
        D_TESTS_EXPECT( (d_tests_traits_of<int>()              == 0x006u) &&
                        (d_tests_traits_of<unsigned>()         == 0x006u) &&
                        (d_tests_traits_of<bool>()             == 0x006u) &&
                        (d_tests_traits_of<char>()             == 0x006u) &&
                        (d_tests_traits_of<float>()            == 0x003u) &&
                        (d_tests_traits_of<double>()           == 0x003u) &&
                        (d_tests_traits_of<long double>()      == 0x003u) &&
                        (d_tests_traits_of<void*>()            == 0x000u),
                        "the traits over the library's types");
        D_TESTS_EXPECT( (d_tests_traits_of<b7>()               == 0x108u) &&
                        (d_tests_traits_of<b16>()              == 0x110u) &&
                        (d_tests_traits_of<b10>()              == 0x120u) &&
                        (d_tests_traits_of<b2>()               == 0x180u) &&
                        (d_tests_traits_of<b8>()               == 0x140u),
                        "the traits over number_base");
        D_TESTS_EXPECT( (d_tests_traits_of<t_user16>()         == 0x110u) &&
                        (d_tests_traits_of<t_user_rational>()  == 0x200u) &&
                        (d_tests_traits_of<t_only_numerator>() == 0x000u) &&
                        (d_tests_traits_of<t_radix_function>() == 0x100u) &&
                        (d_tests_traits_of<t_radix_member>()   == 0x000u),
                        "the traits over a user's types");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        D_TESTS_EXPECT( (re_std::is_same<dm::binary_base<>,
                                         dm::number_base<std::size_t, 2> >::
                             value) &&
                        (re_std::is_same<dm::octal_base<int>,
                                         dm::number_base<int, 8> >::value) &&
                        (re_std::is_same<dm::decimal_base<long>,
                                         dm::number_base<long, 10> >::
                             value) &&
                        (re_std::is_same<dm::hexadecimal_base<char>,
                                         dm::number_base<char, 16> >::
                             value) &&
                        (re_std::is_same<dm::base<short, 7>,
                                         dm::number_base<short, 7> >::value),
                        "the alias templates (C++11)");
#endif

        return;
    }

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // d_tests_variable_templates
    //   from C++14: each variable template is of's constant, for every type.
    template<typename Type>
    bool
    d_tests_variable_templates()
    {
        typedef dk::of<Type> k;

        return ( (d_tests_same(dk::pi<Type>, k::pi())) &&
                 (d_tests_same(dk::two_pi<Type>, k::two_pi())) &&
                 (d_tests_same(dk::half_pi<Type>, k::half_pi())) &&
                 (d_tests_same(dk::quarter_pi<Type>, k::quarter_pi())) &&
                 (d_tests_same(dk::inv_pi<Type>, k::inv_pi())) &&
                 (d_tests_same(dk::tau<Type>, k::tau())) &&
                 (d_tests_same(dk::e<Type>, k::e())) &&
                 (d_tests_same(dk::ln2<Type>, k::ln2())) &&
                 (d_tests_same(dk::ln10<Type>, k::ln10())) &&
                 (d_tests_same(dk::log2e<Type>, k::log2e())) &&
                 (d_tests_same(dk::log10e<Type>, k::log10e())) &&
                 (d_tests_same(dk::sqrt2<Type>, k::sqrt2())) &&
                 (d_tests_same(dk::sqrt3<Type>, k::sqrt3())) &&
                 (d_tests_same(dk::inv_sqrt2<Type>, k::inv_sqrt2())) &&
                 (d_tests_same(dk::euler_gamma<Type>, k::euler_gamma())) &&
                 (d_tests_same(dk::golden_ratio<Type>, k::golden_ratio())) &&
                 (d_tests_same(dk::phi<Type>, k::phi())) &&
                 (d_tests_same(dk::rad_per_deg<Type>, k::rad_per_deg())) &&
                 (d_tests_same(dk::deg_per_rad<Type>, k::deg_per_rad())) );
    }

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    void
    d_tests_by_level()
    {
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        D_TESTS_EXPECT( (d_tests_variable_templates<float>()) &&
                        (d_tests_variable_templates<double>()) &&
                        (d_tests_variable_templates<long double>()) &&
                        (d_tests_same(dk::pi<>, dk::of<double>::pi())),
                        "the variable templates are of's constants (C++14)");
        D_TESTS_EXPECT( (dm::is_rational_v<t_user_rational>) &&
                        (dm::has_number_base_v<t_user16>) &&
                        (dm::is_using_base_v<dm::number_base<int, 7>, 7>) &&
                        (dm::is_hexadecimal_v<t_user16>) &&
                        (!dm::is_binary_v<t_user16>) &&
                        (!dm::is_octal_v<int>) &&
                        (!dm::is_decimal_v<int>) &&
                        (dm::is_integer_type_v<long>) &&
                        (dm::is_real_number_v<float>) &&
                        (dm::is_floating_point_type_v<double>),
                        "the traits' variable templates (C++14)");
#endif

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
        {
            typedef dm::rational<2, -4>              r;
            typedef dm::rational<1, 3>::add<1, 6>    sum;
            typedef dm::rational<1, 3>::subtract<1, 2> difference;
            typedef dm::rational<2, 3>::multiply<3, 4> product;
            typedef dm::rational<2, 3>::divide<4, 9>   quotient;

            D_TESTS_EXPECT( (r::numerator == -1) &&
                            (r::denominator == 2) &&
                            (d_tests_same(r::value, -0.5)) &&
                            (d_tests_same(r()(1, 2.0, 'c'), -0.5)) &&
                            (sum::numerator == 1) &&
                            (sum::denominator == 2) &&
                            (difference::numerator == -1) &&
                            (difference::denominator == 6) &&
                            (product::numerator == 1) &&
                            (product::denominator == 2) &&
                            (quotient::numerator == 3) &&
                            (quotient::denominator == 2) &&
                            (dm::is_rational<r>::value) &&
                            (!dm::has_number_base<r>::value),
                            "rational (C++14)");
        }
#endif

        return;
    }
}  // namespace

// from C++11 the constants are constant expressions, unless a testing build
// strips constexpr
#if ( D_ENV_LANG_IS_CPP11_OR_HIGHER &&                                        \
      !D_QUAL_CONSTEXPR_IS_STRIPPED )
static_assert(djinterp::math::constants::of<double>::pi() > 3.14159,
              "a constant expression from C++11");
static_assert(djinterp::math::constants::of<float>::e() < 2.7183f,
              "of<float>'s constants too");
static_assert(djinterp::math::number_base<int, 16>::max_digit() == 15,
              "number_base's queries too");
static_assert(djinterp::math::number_base<int, 16>::is_valid_digit(15),
              "and is_valid_digit");
#endif

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
static_assert(djinterp::math::constants::pi<long double> ==
                  djinterp::math::constants::of<long double>::pi(),
              "a variable template is of's constant (C++14)");
#endif

static char g_traits_are_constants[
    djinterp::math::is_hexadecimal<
        djinterp::math::number_base<int, 16> >::value ? 1 : -1];


int
main()
{
    d_tests_constants();
    d_tests_number_bases();
    d_tests_by_level();

    (void)g_traits_are_constants;

    std::printf("t_constants_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
