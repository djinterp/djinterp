/*******************************************************************************
* djinterp [cpp][math]                                         t_values_face.cpp
*
*   Tests for math/values.hpp, compile-time sampling, at every level from
* C++98 (step 30, 5b). Below C++11 the header is empty. From C++11: sample,
* a constant expression wherever its expression's evaluation is, and at run
* time the C core's bits for a transcendental. From C++14: sample_over, at
* each of an interval's members, past an open lower bound where it took the
* lower bound itself.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++14 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/t_values_face.cpp                    \
*         -o t_values -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_values_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.22
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/values.hpp"  // the
                                                              // tested header
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/elementary.h"  // sin_c
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


    //==========================================================================
    // 1.  SAMPLE, A CONSTANT EXPRESSION FROM C++11
    //==========================================================================

    // each initialized by a constant expression from C++11
    D_CONSTEXPR std::array<double, 4> g_squares =
        dm::sample<4>(dv::x * dv::x, 0.0, 0.5);
    D_CONSTEXPR std::array<double, 3> g_unit_step =
        dm::sample<3>(dv::x + 1.0, 2.0);
    D_CONSTEXPR std::array<double, 5> g_descending =
        dm::sample<5>(dm::abs(dv::x), 2.0, -1.0);
    D_CONSTEXPR std::array<double, 1> g_single =
        dm::sample<1>(dm::constant(7.0) * dv::x, 3.0, 100.0);

    // d_tests_sample
    //   sample's values, folded at compile time; a transcendental's, at run
    // time, the C core's.
    void
    d_tests_sample()
    {
        const double at[4] = { 0.25, 1.0, 1.75, 2.5 };
        const std::array<double, 4> sines =
            dm::sample<4>(dm::sin(dv::x), 0.25, 0.75);
        bool same = true;

        D_TESTS_EXPECT( (g_squares[0] == 0.0) &&
                        (g_squares[1] == 0.25) &&
                        (g_squares[2] == 1.0) &&
                        (g_squares[3] == 2.25) &&
                        (g_squares.size() == 4),
                        "sample<4>(x * x, 0, 0.5): x^2 at 0, 0.5, 1 and 1.5, "
                        "a constant expression from C++11");
        D_TESTS_EXPECT( (g_unit_step[0] == 3.0) &&
                        (g_unit_step[1] == 4.0) &&
                        (g_unit_step[2] == 5.0),
                        "the step 1 by default");
        D_TESTS_EXPECT( (g_descending[0] == 2.0) &&
                        (g_descending[1] == 1.0) &&
                        (g_descending[2] == 0.0) &&
                        (g_descending[3] == 1.0) &&
                        (g_descending[4] == 2.0) &&
                        (g_single[0] == 21.0) &&
                        (g_single.size() == 1),
                        "a negative step; one point, the step unused");

        for (std::size_t i = 0; i < 4; ++i)
        {
            same = ( (same) &&
                     (d_tests_same_bits(sines[i], d_math_sin_c(at[i]))) );
        }

        D_TESTS_EXPECT(same,
                       "sample of sin at run time the C core's d_math_sin_c, "
                       "bit for bit (N3)");

        return;
    }


    //==========================================================================
    // 2.  SAMPLE_OVER, AT AN INTERVAL'S MEMBERS, FROM C++14
    //==========================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    typedef dm::interval<int, 0, 4>                     closed_type;
    typedef dm::interval<int, 0, 4, true, false>        left_open_type;
    typedef dm::interval<int, 0, 4, false, true>        right_open_type;
    typedef dm::interval<int, 0, 6, false, false, 2>    stepped_type;
    typedef dm::interval<int, 0, 6, true, false, 2>     open_stepped_type;
    typedef dm::interval<int, -3, 3, true, true, 1>     open_type;

    D_CONSTEXPR std::array<double, closed_type::size()> g_closed =
        dm::sample_over<closed_type>(dv::x * dv::x);
    D_CONSTEXPR std::array<double, left_open_type::size()> g_left_open =
        dm::sample_over<left_open_type>(dv::x);
    D_CONSTEXPR std::array<double, right_open_type::size()> g_right_open =
        dm::sample_over<right_open_type>(dv::x);
    D_CONSTEXPR std::array<double, stepped_type::size()> g_stepped =
        dm::sample_over<stepped_type>(dv::x + 0.5);
    D_CONSTEXPR std::array<double, open_stepped_type::size()> g_open_stepped =
        dm::sample_over<open_stepped_type>(dv::x);
    D_CONSTEXPR std::array<double, open_type::size()> g_open =
        dm::sample_over<open_type>(dv::x);

    // d_tests_sample_over
    //   sample_over's values, folded at compile time: each interval's
    // members, by at().
    void
    d_tests_sample_over()
    {
        bool members = true;

        for (std::size_t i = 0; i < g_open.size(); ++i)
        {
            members = ( (members) &&
                        (g_open[i] ==
                             static_cast<double>(open_type::at(i))) );
        }

        D_TESTS_EXPECT( (g_closed.size() == 5) &&
                        (g_closed[0] == 0.0) && (g_closed[1] == 1.0) &&
                        (g_closed[2] == 4.0) && (g_closed[3] == 9.0) &&
                        (g_closed[4] == 16.0),
                        "x^2 over [0, 4]: its five members, a constant "
                        "expression from C++14");
        D_TESTS_EXPECT( (g_left_open.size() == 4) &&
                        (g_left_open[0] == 1.0) && (g_left_open[1] == 2.0) &&
                        (g_left_open[2] == 3.0) && (g_left_open[3] == 4.0) &&
                        (g_right_open.size() == 4) &&
                        (g_right_open[0] == 0.0) &&
                        (g_right_open[3] == 3.0),
                        "over (0, 4]: 1 to 4, where it took 0 to 3, 0 not a "
                        "member; over [0, 4): 0 to 3");
        D_TESTS_EXPECT( (g_stepped.size() == 4) &&
                        (g_stepped[0] == 0.5) && (g_stepped[1] == 2.5) &&
                        (g_stepped[2] == 4.5) && (g_stepped[3] == 6.5) &&
                        (g_open_stepped.size() == 3) &&
                        (g_open_stepped[0] == 1.0) &&
                        (g_open_stepped[1] == 3.0) &&
                        (g_open_stepped[2] == 5.0),
                        "over [0, 6] by 2: 0, 2, 4, 6; over (0, 6] by 2: 1, "
                        "3, 5, where it took 0, 2, 4");
        D_TESTS_EXPECT( (g_open.size() == 5) &&
                        (g_open[0] == -2.0) &&
                        (g_open[4] == 2.0) &&
                        (members),
                        "over (-3, 3): -2 to 2, each the interval's at(i)");

        return;
    }
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    D_TESTS_EXPECT(DJINTERP_MATH_VALUES_HPP == 1,
                   "the header reads at every level, empty below C++11");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_sample();
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    d_tests_sample_over();
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

    std::printf("t_values_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
