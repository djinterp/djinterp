/*******************************************************************************
* djinterp [test]                                            t_interval_face.cpp
*
*   Tests for the interval faces (math/interval/), at every level from C++98.
* The compile-time half -- static members as constants, the conversion types,
* the traits (with impostors: non-static members, int-valued flags), the
* families, and from C++14 the operations themselves as constant expressions
* -- runs wherever the unit compiles, which the C++ ladder does at every rung.
* The runtime half needs running, and checks what the faces add to the C core:
*
*     - the generic path agrees with the C families: an interval over an
*       enumeration (the generic path) against the same interval over int
*       (the _imax family), every operation, every bound kind, steps 0 to 3;
*     - iteration by index reaches every member, at the ends of the value
*       type too: uint8_t [0, 255], signed char [-128, 127], bool, a stride;
*     - normalize<FloatType> is exactly the former formula, (v - lo) / (hi -
*       lo) computed in FloatType;
*     - intersects, member-wise, against overlaps' ranges: strides that meet
*       by the Chinese remainder theorem and strides that never do;
*     - text, runtime_open_interval (an invalid one included), the extremes
*       of int, and the overlap of an empty interval;
*     - from C++20, floating-point bounds through the C core's floating
*       families: every representable value a member, an open bound moved
*       to its neighbour, a stride, iteration over every float.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/interval/t_interval_face.cpp -o t_interval_face
*
*
* path:      /tests/djinterp/math/interval/t_interval_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "../../../../inc/djinterp/math/interval/interval.hpp"  // under test
// std
#include <climits>  // INT_MIN, INT_MAX, CHAR_BIT
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <string>   // std::string


NS_DJINTERP
NS_MATH
NS_INTERNAL

    // the probe types: an enumeration (the generic path), an interval-like
    // type of the user's, and two impostors
    enum t_colour
    {
        t_red = -2,
        t_green,
        t_blue,
        t_cyan,
        t_magenta,
        t_yellow,
        t_black,
        t_white
    };

    struct t_user_interval
    {
        typedef int         value_type;
        typedef std::size_t size_type;

        static const int  lower_bound   = 1;
        static const int  upper_bound   = 4;
        static const bool is_left_open  = false;
        static const bool is_right_open = true;
        static const int  step          = 0;
    };

    struct t_not_static
    {
        int  lower_bound;
        int  upper_bound;
        bool is_left_open;
        bool is_right_open;
    };

    struct t_int_flags
    {
        static const int lower_bound   = 0;
        static const int upper_bound   = 1;
        static const int is_left_open  = 0;
        static const int is_right_open = 0;
    };

    typedef interval<int, 2, 8, true, true>        t_open;
    typedef interval<int, -4, 9, false, false, 3>  t_stepped;
    typedef interval<int, 0, INT_MAX>              t_wide;

    // the families
    D_STATIC_ASSERT((interval_family_of<int>::value == 1),
                    "int reaches the signed family");
    D_STATIC_ASSERT((interval_family_of<unsigned char>::value == 2),
                    "unsigned char reaches the unsigned family");
    D_STATIC_ASSERT((interval_family_of<bool>::value == 2),
                    "bool reaches the unsigned family");
    D_STATIC_ASSERT((interval_family_of<float>::value == 3),
                    "float reaches the float family");
    D_STATIC_ASSERT((interval_family_of<double>::value == 4),
                    "double reaches the double family");
    D_STATIC_ASSERT((interval_family_of<long double>::value == 5),
                    "long double reaches the long double family");
    D_STATIC_ASSERT((interval_family_of<t_colour>::value == 0),
                    "an enumeration takes the generic path");

    // static members are constants, and the conversions name the right types
    D_STATIC_ASSERT((t_open::as_closed::lower_bound == 3),
                    "as_closed moves an open bound inward");
    D_STATIC_ASSERT((re_std::is_same<t_open::to_closed_interval,
                                     closed_interval<int, 3, 7> >::value),
                    "to_closed_interval");
    D_STATIC_ASSERT((re_std::is_same<
                         interval_cast_to_open<
                             closed_interval<int, 3, 7> >::type,
                         open_interval<int, 2, 8> >::value),
                    "interval_cast_to_open");
    D_STATIC_ASSERT((re_std::is_same<
                         t_open::rebind_step<2>::type,
                         interval<int, 2, 8, true, true, 2> >::value),
                    "rebind_step");
    D_STATIC_ASSERT((re_std::is_same<t_wide::as_open,
                                     interval<int, -1, INT_MIN, true, true>
                                    >::value),
                    "a bound at INT_MAX wraps; only using as_open fails");

    // the traits, impostors included
    D_STATIC_ASSERT((is_interval<t_open>::value),
                    "an interval is an interval");
    D_STATIC_ASSERT((is_interval<t_user_interval>::value),
                    "a user type with the structure is an interval");
    D_STATIC_ASSERT((!is_interval<int>::value),
                    "int is not an interval");
    D_STATIC_ASSERT((!is_interval<t_not_static>::value),
                    "non-static members are not the structure");
    D_STATIC_ASSERT((!is_interval<t_int_flags>::value),
                    "int-valued openness flags are not the structure");
    D_STATIC_ASSERT((is_discrete_interval<t_stepped>::value),
                    "a positive step is discrete");
    D_STATIC_ASSERT((is_continuous_interval<t_open>::value),
                    "a zero step is continuous");
    D_STATIC_ASSERT((is_open<t_open>::value),
                    "both ends open");
    D_STATIC_ASSERT((is_half_open<t_user_interval>::value),
                    "one end open");
    D_STATIC_ASSERT((is_closed<t_stepped>::value),
                    "both ends closed");
    D_STATIC_ASSERT((is_left_open<t_open>::value),
                    "left open");
    D_STATIC_ASSERT((is_right_closed<t_stepped>::value),
                    "right closed");
    D_STATIC_ASSERT((is_empty_interval<
                         interval<int, 4, 4, false, true> >::value),
                    "[4, 4) is empty");
    D_STATIC_ASSERT((is_degenerate_interval<interval<int, 4, 4> >::value),
                    "[4, 4] is degenerate");
    D_STATIC_ASSERT((intervals_same_type<t_open, t_stepped>::value),
                    "same value type");
    D_STATIC_ASSERT((!intervals_same_boundary_type<t_open, t_stepped>::value),
                    "different boundary kinds");
    D_STATIC_ASSERT((re_std::is_same<interval_value_type<int>::type,
                                     void>::value),
                    "no value_type is void");

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // the operations are constant expressions from C++14, through the kernels
    static_assert(t_stepped::size() == 5, "constexpr size");
    static_assert(t_stepped::contains(5) && !t_stepped::contains(6),
                  "constexpr contains");
    static_assert(t_stepped::clamp_nearest(7) == 8, "constexpr clamp_nearest");
    static_assert(t_open::first() == 3 && t_open::last() == 7,
                  "constexpr first, last");
    static_assert(t_wide::size() == 2147483648ul, "constexpr wide size");
    static_assert(is_interval_v<t_open> && !is_open_v<t_stepped>, "_v");
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_END  // internal
NS_END  // math
NS_END  // djinterp


namespace
{
    using djinterp::math::closed_interval;
    using djinterp::math::discrete_interval;
    using djinterp::math::interval;
    using djinterp::math::open_interval;
    using djinterp::math::runtime_open_interval;
    using djinterp::math::internal::t_colour;

    long g_checks   = 0;
    long g_failures = 0;

    void
    d_tests_expect(
        bool        _ok,
        const char* _what,
        long        _detail,
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
                std::printf("  FAIL line %d: %s (%ld)\n",
                            _line,
                            _what,
                            _detail);
            }
        }

        return;
    }

    #define D_TESTS_EXPECT(ok, what, detail)                                  \
        d_tests_expect((ok), (what), static_cast<long>(detail), __LINE__)

    // d_tests_same
    //   whether two values are equal once each is rounded to its type: on
    // x87 (FLT_EVAL_METHOD 2) clang can keep one side's excess precision in
    // a register, so a store through volatile makes the comparison the one
    // the paths are meant to agree on.
    template<typename Float>
    bool
    d_tests_same(
        Float _left,
        Float _right
    )
    {
        volatile Float left  = _left;
        volatile Float right = _right;

        return (left == right);
    }

    // d_tests_generic_parity
    //   one bound kind and step: the enumeration's interval (the generic
    // path) against int's (the signed family), every operation.
    template<bool LeftOpen,
             bool RightOpen,
             int  Step>
    void
    d_tests_generic_parity()
    {
        typedef interval<t_colour,
                         djinterp::math::internal::t_red,
                         djinterp::math::internal::t_white,
                         LeftOpen,
                         RightOpen,
                         static_cast<t_colour>(Step)>     generic_t;
        typedef interval<int, -2, 5, LeftOpen, RightOpen, Step> c_t;

        D_TESTS_EXPECT(generic_t::size() == c_t::size(), "size", Step);
        D_TESTS_EXPECT(generic_t::is_empty() == c_t::is_empty(),
                       "is_empty", Step);
        D_TESTS_EXPECT(generic_t::to_string() == c_t::to_string(),
                       "to_string", Step);

        // first and last are defined only where there are members
        if (!c_t::is_empty())
        {
            D_TESTS_EXPECT(static_cast<int>(generic_t::first()) ==
                           c_t::first(),
                           "first", Step);
            D_TESTS_EXPECT(static_cast<int>(generic_t::last()) ==
                           c_t::last(),
                           "last", Step);
        }

        // every value in a window around the bounds
        for (int v = -4; v <= 7; ++v)
        {
            const t_colour g = static_cast<t_colour>(v);

            D_TESTS_EXPECT(generic_t::contains(g) == c_t::contains(v),
                           "contains", v);
            D_TESTS_EXPECT(generic_t::contains_in_range(g) ==
                           c_t::contains_in_range(v),
                           "contains_in_range", v);
            D_TESTS_EXPECT(generic_t::index_of(g) == c_t::index_of(v),
                           "index_of", v);
            D_TESTS_EXPECT(d_tests_same(generic_t::normalize(g),
                                        c_t::normalize(v)),
                           "normalize", v);
            D_TESTS_EXPECT(d_tests_same(generic_t::normalize_effective(g),
                                        c_t::normalize_effective(v)),
                           "normalize_effective", v);
            D_TESTS_EXPECT(d_tests_same(generic_t::normalize_discrete(g),
                                        c_t::normalize_discrete(v)),
                           "normalize_discrete", v);

            // clamping is defined where there are members to clamp to
            if (!c_t::is_empty())
            {
                D_TESTS_EXPECT(static_cast<int>(generic_t::clamp(g)) ==
                               c_t::clamp(v),
                               "clamp", v);
                D_TESTS_EXPECT(static_cast<int>(generic_t::clamp_nearest(g)) ==
                               c_t::clamp_nearest(v),
                               "clamp_nearest", v);
            }
        }

        // the members, by index and by iteration
        typename generic_t::iterator gi = generic_t::begin();
        typename c_t::iterator       ci = c_t::begin();

        for (; ci != c_t::end(); ++ci, ++gi)
        {
            D_TESTS_EXPECT(static_cast<int>(*gi) == *ci,
                           "iteration", *ci);
        }

        D_TESTS_EXPECT(gi == generic_t::end(), "iteration ends", Step);

        // overlap with a closed and an open neighbour
        D_TESTS_EXPECT(
            generic_t::overlaps(interval<t_colour,
                                         djinterp::math::internal::t_black,
                                         djinterp::math::internal::t_white>())
            == c_t::overlaps(interval<int, 4, 5>()),
            "overlaps", Step);
        D_TESTS_EXPECT(
            generic_t::overlaps(interval<t_colour,
                                         djinterp::math::internal::t_white,
                                         djinterp::math::internal::t_white,
                                         true,
                                         false>())
            == c_t::overlaps(interval<int, 5, 5, true, false>()),
            "overlaps an empty one", Step);
        D_TESTS_EXPECT(
            generic_t::intersects(
                interval<t_colour,
                         djinterp::math::internal::t_green,
                         djinterp::math::internal::t_white,
                         false,
                         false,
                         djinterp::math::internal::t_magenta>())
            == c_t::intersects(interval<int, -1, 5, false, false, 2>()),
            "intersects a stride", Step);
        D_TESTS_EXPECT(
            generic_t::intersects(interval<t_colour,
                                           djinterp::math::internal::t_black,
                                           djinterp::math::internal::t_white>())
            == c_t::intersects(interval<int, 4, 5>()),
            "intersects a continuous one", Step);

        return;
    }

    // d_tests_iterates
    //   an interval's iteration against its count and its at().
    template<typename Interval>
    long
    d_tests_iterates(
        const char* _what
    )
    {
        long                        count = 0;
        typename Interval::iterator it    = Interval::begin();

        for (; it != Interval::end(); ++it)
        {
            D_TESTS_EXPECT(*it == Interval::at(it.index()), _what, count);
            ++count;
        }

        D_TESTS_EXPECT(static_cast<std::size_t>(count) == Interval::size(),
                       _what, count);

        return count;
    }

    void
    d_tests_iteration()
    {
        typedef closed_interval<unsigned char, 0, 255>              all_u8;
        typedef closed_interval<signed char, -128, 127>             all_s8;
        typedef closed_interval<bool, false, true>                  all_bool;
        typedef discrete_interval<unsigned char, 0, 255, 5>         every_5;
        typedef interval<int, -3, 7, true, false, 4>                stride_4;

        D_TESTS_EXPECT(d_tests_iterates<all_u8>("uint8_t [0, 255]") == 256,
                       "uint8_t [0, 255] has 256 members", 0);
        D_TESTS_EXPECT(d_tests_iterates<all_s8>("signed char [-128, 127]") ==
                       256,
                       "signed char [-128, 127] has 256 members", 0);
        D_TESTS_EXPECT(d_tests_iterates<all_bool>("bool") == 2,
                       "bool [false, true] has 2 members", 0);
        D_TESTS_EXPECT(d_tests_iterates<every_5>("uint8_t [0:5:255]") == 52,
                       "uint8_t [0:5:255] has 52 members", 0);
        D_TESTS_EXPECT(d_tests_iterates<stride_4>("(-3:4:7]") == 3,
                       "(-3:4:7] has 3 members", 0);
        D_TESTS_EXPECT( (all_bool::at(0) == false) &&
                        (all_bool::at(1) == true),
                        "bool's members in order", 0);
        D_TESTS_EXPECT(every_5::last() == 255, "uint8_t [0:5:255] ends on 255",
                       every_5::last());

        return;
    }

    void
    d_tests_normalize_formula()
    {
        typedef closed_interval<int, -3, 7> ci;

        // the former formula, computed in the requested type
        for (int v = -6; v <= 10; ++v)
        {
            const float  want_f = static_cast<float>(v + 3) /
                                  static_cast<float>(10);
            const double want_d = static_cast<double>(v + 3) /
                                  static_cast<double>(10);

            D_TESTS_EXPECT(d_tests_same(ci::normalize<float>(v), want_f),
                           "normalize<float> is (v - lo) / (hi - lo)", v);
            D_TESTS_EXPECT(d_tests_same(ci::normalize(v), want_d),
                           "normalize is (v - lo) / (hi - lo)", v);
        }

        return;
    }

    void
    d_tests_text_and_runtime()
    {
        typedef interval<t_colour,
                         djinterp::math::internal::t_red,
                         djinterp::math::internal::t_white,
                         false,
                         true> colours;

        // a template-id's commas would split a macro argument, so each
        // condition is parenthesized
        D_TESTS_EXPECT((closed_interval<int, -3, 7>::to_string() == "[-3, 7]"),
                       "closed text", 0);
        D_TESTS_EXPECT((open_interval<int, 2, 8>::to_string() == "(2, 8)"),
                       "open text", 0);
        D_TESTS_EXPECT((discrete_interval<int, -4, 9, 3>::to_string() ==
                        "[-4:3:9]"),
                       "discrete text", 0);
        D_TESTS_EXPECT( (interval<unsigned, 0, 5, false, true>::to_string() ==
                         "[0, 5)"),
                        "half-open text", 0);
        D_TESTS_EXPECT(colours::to_string() == "[-2, 5)",
                       "the generic path's text", 0);

        // a runtime interval, valid and not
        const runtime_open_interval<int, 3> good(9);
        const runtime_open_interval<int, 3> bad(2);
        int                                 sum = 0;

        for (runtime_open_interval<int, 3>::iterator it = good.begin();
             it != good.end();
             ++it)
        {
            sum += *it;
        }

        D_TESTS_EXPECT( (good.is_valid()) &&
                        (good.size() == 5u) &&
                        (sum == 4 + 5 + 6 + 7 + 8),
                        "runtime_open_interval (3, 9)", sum);
        D_TESTS_EXPECT( (!bad.is_valid()) &&
                        (bad.size() == 0u) &&
                        (!bad.contains(3)) &&
                        (bad.begin() == bad.end()),
                        "an invalid runtime_open_interval is empty", 0);
        D_TESTS_EXPECT(good.to_string() == "(3, 9)", "runtime text", 0);
        D_TESTS_EXPECT( (good.clamp(1) == 4) &&
                        (good.clamp(20) == 8),
                        "runtime clamp", 0);
        D_TESTS_EXPECT( (good.overlaps(good)) &&
                        (!good.overlaps(bad)),
                        "runtime overlaps", 0);

        return;
    }

    void
    d_tests_intersects()
    {
        // x = 0 mod 6 and x = 4 mod 10 meet at 24; of different parity, never
        D_TESTS_EXPECT((discrete_interval<int, 0, 1000, 6>::intersects(
                            discrete_interval<int, 4, 1000, 10>())),
                       "strides meeting at 24", 0);
        D_TESTS_EXPECT((!discrete_interval<int, 0, 23, 6>::intersects(
                            discrete_interval<int, 4, 1000, 10>())),
                       "strides meeting only past a range", 0);
        D_TESTS_EXPECT((!discrete_interval<int, 0, 1000, 6>::intersects(
                            discrete_interval<int, 1, 1000, 10>())),
                       "strides of different parity", 0);
        D_TESTS_EXPECT((discrete_interval<int, 0, 1000, 6>::overlaps(
                            discrete_interval<int, 1, 1000, 10>())),
                       "overlaps still compares ranges", 0);

        // (0, 2) holds 1 and (1, 3) holds 2: they overlap on the real line
        // and share no member
        D_TESTS_EXPECT((open_interval<int, 0, 2>::overlaps(
                            open_interval<int, 1, 3>())),
                       "open intervals overlap on the real line", 0);
        D_TESTS_EXPECT((!open_interval<int, 0, 2>::intersects(
                            open_interval<int, 1, 3>())),
                       "and share no member", 0);
        D_TESTS_EXPECT((closed_interval<int, 0, 5>::intersects(
                            closed_interval<int, 5, 9>())),
                       "closed intervals sharing an end", 0);

        // the unified template against each of the others
        D_TESTS_EXPECT((interval<int, 0, 100, false, false, 7>::intersects(
                            interval<int, 3, 100, false, false, 5>())),
                       "unified strides meet (28)", 0);
        D_TESTS_EXPECT((!interval<int, 0, 100, false, false, 4>::intersects(
                            discrete_interval<int, 1, 100, 2>())),
                       "unified against discrete, different parity", 0);
        D_TESTS_EXPECT((interval<int, 0, 10, true, true>::intersects(
                            open_interval<int, 8, 12>())),
                       "unified against open (9)", 0);
        D_TESTS_EXPECT((!interval<int, 4, 4, false, true>::intersects(
                            closed_interval<int, 0, 9>())),
                       "an empty interval shares nothing", 0);

        // runtime intervals with one lower bound
        D_TESTS_EXPECT( (runtime_open_interval<int, 3>(9).intersects(
                             runtime_open_interval<int, 3>(5))) &&
                        (!runtime_open_interval<int, 3>(9).intersects(
                             runtime_open_interval<int, 3>(4))),
                        "runtime intervals", 0);

        return;
    }

    void
    d_tests_extremes()
    {
        typedef closed_interval<int, INT_MIN, INT_MAX> all_int;
        typedef interval<int, 0, INT_MAX>              non_negative;

        // the count of every int needs one bit more than int has
        if (sizeof(std::size_t) * CHAR_BIT > sizeof(int) * CHAR_BIT)
        {
            D_TESTS_EXPECT(all_int::size() ==
                           static_cast<std::size_t>(UINT_MAX) + 1u,
                           "every int counted", 0);
        }

        D_TESTS_EXPECT( (all_int::contains(INT_MIN)) &&
                        (all_int::contains(INT_MAX)),
                        "every int contained", 0);
        D_TESTS_EXPECT( (all_int::normalize(INT_MIN) == 0.0) &&
                        (all_int::normalize(INT_MAX) == 1.0),
                        "every int normalized", 0);
        D_TESTS_EXPECT( (non_negative::clamp(INT_MIN) == 0) &&
                        (non_negative::clamp(INT_MAX) == INT_MAX) &&
                        (non_negative::last() == INT_MAX),
                        "a bound at INT_MAX", 0);
        D_TESTS_EXPECT((!interval<int, 4, 4, false, true>::overlaps(
                            closed_interval<int, 3, 5>())),
                       "an empty interval overlaps nothing", 0);

        return;
    }

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    void
    d_tests_floating()
    {
        typedef interval<double, 0.5, 2.5>                    reals;
        typedef interval<double, 0.5, 2.5, true, true>        open_reals;
        typedef interval<double, 0.0, 2.0, false, false, 0.5> halves;
        typedef closed_interval<float, 1.0f, 1.0000005f>      five_floats;

        // a continuous interval holds every double between its bounds
        D_TESTS_EXPECT( (reals::contains(1.0)) &&
                        (!reals::contains(3.0)),
                        "a floating-point interval's contains", 0);
        D_TESTS_EXPECT(reals::size() ==
                       static_cast<std::size_t>(d_math_d_steps(0.5, 2.5)) + 1u,
                       "its size is the doubles it holds", 0);
        D_TESTS_EXPECT(reals::to_string() == "[0.5, 2.5]",
                       "its text, in the fewest digits", 0);
        D_TESTS_EXPECT(reals::normalize(1.5) == 0.5, "its normalize", 0);

        // an open bound moves to the neighbouring double
        D_TESTS_EXPECT( (open_reals::first() == d_math_d_next_up(0.5)) &&
                        (open_reals::last() == d_math_d_next_down(2.5)),
                        "an open interval's first and last", 0);
        D_TESTS_EXPECT( (open_reals::clamp(0.0) == d_math_d_next_up(0.5)) &&
                        (!open_reals::contains(0.5)) &&
                        (open_reals::contains(d_math_d_next_up(0.5))),
                        "clamp lands inside an open bound", 0);
        static_assert(open_reals::as_closed::lower_bound ==
                          d_math_d_next_up(0.5),
                      "as_closed moves an open bound to its neighbour");

        // a stride: 0, 0.5, 1, 1.5, 2
        D_TESTS_EXPECT( (halves::size() == 5u) &&
                        (halves::at(3) == 1.5) &&
                        (halves::contains(1.5)) &&
                        (!halves::contains(1.25)),
                        "a floating-point stride", 0);
        D_TESTS_EXPECT(halves::intersects(
                           interval<double, 0.25, 3.0, false, false, 0.75>()),
                       "strides sharing 1.0", 0);
        D_TESTS_EXPECT(!halves::intersects(
                           interval<double, 0.25, 3.0, false, false, 0.5>()),
                       "strides a quarter apart", 0);

        // every float of a short interval, in order
        {
            float  previous = 0.0f;
            long   count    = 0;
            bool   in_order = true;

            for (five_floats::iterator it = five_floats::begin();
                 it != five_floats::end();
                 ++it, ++count)
            {
                in_order = in_order &&
                           ( (count == 0) ||
                             (*it == d_math_f_next_up(previous)) );
                previous = *it;
            }

            D_TESTS_EXPECT( (count == 5) &&
                            (in_order),
                            "iteration visits every float", count);
        }

        return;
    }
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
}  // namespace


int
main()
{
    d_tests_generic_parity<false, false, 0>();
    d_tests_generic_parity<false, false, 1>();
    d_tests_generic_parity<false, false, 2>();
    d_tests_generic_parity<false, false, 3>();
    d_tests_generic_parity<false, true,  0>();
    d_tests_generic_parity<false, true,  1>();
    d_tests_generic_parity<false, true,  2>();
    d_tests_generic_parity<false, true,  3>();
    d_tests_generic_parity<true,  false, 0>();
    d_tests_generic_parity<true,  false, 1>();
    d_tests_generic_parity<true,  false, 2>();
    d_tests_generic_parity<true,  false, 3>();
    d_tests_generic_parity<true,  true,  0>();
    d_tests_generic_parity<true,  true,  1>();
    d_tests_generic_parity<true,  true,  2>();
    d_tests_generic_parity<true,  true,  3>();
    d_tests_iteration();
    d_tests_normalize_formula();
    d_tests_text_and_runtime();
    d_tests_intersects();
    d_tests_extremes();
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    d_tests_floating();
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    std::printf("t_interval_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
