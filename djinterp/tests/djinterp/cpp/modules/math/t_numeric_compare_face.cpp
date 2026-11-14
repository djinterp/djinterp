/*******************************************************************************
* djinterp [cpp/math]                                 t_numeric_compare_face.cpp
*
*   Tests for math/numeric_compare.hpp at every level from C++98:
*   - numeric_text from a C string, a pointer and a length, a std::string,
*     and from C++17 a std::string_view;
*   - integer_value against the platform's widest unsigned integer, for
*     every operation on random numbers that fit it, and on numbers of up to
*     200 digits against identities and known products;
*   - rational_value from its parts, and over 0;
*   - compare and its predicates on a table of numbers in ascending groups,
*     each group one value written every way the syntax allows, with
*     exponents of up to 40 digits; on texts that are not numbers; on
*     random literals whose values are exact rationals in the C core's
*     family, against literal.h and value.h's exact comparison; on random
*     rationals of up to 60 digits, written as fractions, mixed numbers and
*     decimals, equal to themselves and ordered against their neighbours;
*     and on a sample sorted by compare, which must agree with itself both
*     ways round.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc   \
*         tests/djinterp/cpp/modules/math/t_numeric_compare_face.cpp           \
*         -o t_compare -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_numeric_compare_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.07
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/numeric_compare.hpp"  // tested
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/literal.h"  // the C core's exact
                                                    // reading, a reference
// std
#include <cstdio>   // std::printf, std::sprintf
#include <cstring>  // std::strlen
#include <string>   // std::string


namespace
{
    namespace dm = djinterp::math;

    long g_checks   = 0;
    long g_failures = 0;

    // d_tests_expect
    //   counts a check, and reports the first few failures with what was
    // checked, and where.
    void
    d_tests_expect(
        bool        _ok,
        const char* _what,
        const char* _a,
        const char* _b,
        int         _line
    )
    {
        ++g_checks;

        // report the first few failures
        if (!_ok)
        {
            ++g_failures;

            if (g_failures <= 40)
            {
                std::printf("  FAIL line %d: %s [%.60s] [%.60s]\n", _line,
                            _what, _a, _b);
            }
        }

        return;
    }

    #define D_TESTS_EXPECT(ok, what, a, b)                                    \
        d_tests_expect((ok), (what), (a), (b), __LINE__)

    // d_tests_random32
    //   32 random bits: xorshift, in an unsigned long.
    unsigned long g_state = 2718281828UL;

    unsigned long
    d_tests_random32()
    {
        unsigned long x = g_state;

        x       = (x ^ (x << 13)) & 0xFFFFFFFFUL;
        x       = x ^ (x >> 17);
        x       = (x ^ (x << 5)) & 0xFFFFFFFFUL;
        g_state = x;

        return x;
    }

    // d_tests_below
    //   a random number from 0 to _n - 1.
    unsigned long
    d_tests_below(
        unsigned long _n
    )
    {
        return d_tests_random32() % _n;
    }

    // d_tests_umax_bits
    //   how many bits dj_math_umax has.
    int
    d_tests_umax_bits()
    {
        dj_math_umax all   = static_cast<dj_math_umax>(0u) - 1u;
        int          count = 0;

        // one bit at a time
        while (all != 0u)
        {
            all = all >> 1;
            ++count;
        }

        return count;
    }

    // d_tests_random_umax
    //   a random number of up to _bits bits, the length itself random.
    dj_math_umax
    d_tests_random_umax(
        int _bits
    )
    {
        const int    length = static_cast<int>(d_tests_below(
                                 static_cast<unsigned long>(_bits) + 1UL));
        dj_math_umax result = 0u;
        int          i      = 0;

        // sixteen bits at a time
        for (i = 0; i < length; i += 16)
        {
            result = (result << 8) << 8;
            result = result | static_cast<dj_math_umax>(d_tests_random32() &
                                                        0xFFFFUL);
        }

        // cut to the length
        if (length < d_tests_umax_bits())
        {
            result = result & ((static_cast<dj_math_umax>(1u) << length) - 1u);
        }

        return result;
    }

    // d_tests_text_of
    //   _x's decimal digits.
    std::string
    d_tests_text_of(
        dj_math_umax _x
    )
    {
        std::string result;
        dj_math_umax rest = _x;

        // each digit, the least significant first
        do
        {
            result.insert(result.begin(),
                          static_cast<char>('0' + static_cast<int>(rest %
                                                                   10u)));
            rest = rest / 10u;
        } while (rest != 0u);

        return result;
    }

    // d_tests_digits
    //   _count random decimal digits, the first not 0 unless _count is 1.
    std::string
    d_tests_digits(
        unsigned long _count
    )
    {
        std::string   result;
        unsigned long i = 0;

        // each digit
        for (i = 0; i < _count; ++i)
        {
            const unsigned long low = ( (i == 0) &&
                                        (_count > 1) ) ? 1UL : 0UL;

            result.push_back(static_cast<char>('0' + static_cast<int>(
                                 low + d_tests_below(10UL - low))));
        }

        return result;
    }


    //==========================================================================
    // 1.  TEXT
    //==========================================================================

    // d_tests_text
    //   numeric_text from each source: what it points to and how long.
    void
    d_tests_text()
    {
        const char*       c_text = "12/34";
        const std::string text("-5.5e1");
        const char*       none   = D_NULLPTR;

        D_TESTS_EXPECT( (dm::numeric_text(c_text).data() == c_text) &&
                        (dm::numeric_text(c_text).size() == 5u),
                        "a C string", c_text, "" );
        D_TESTS_EXPECT( (dm::numeric_text(c_text, 2u).size() == 2u) &&
                        (dm::equal(dm::numeric_text(c_text, 2u), "12")),
                        "a pointer and a length, short of the NUL", c_text,
                        "12" );
        D_TESTS_EXPECT( (dm::numeric_text(text).data() == text.data()) &&
                        (dm::numeric_text(text).size() == 6u) &&
                        (dm::equal(text, "-55")),
                        "a std::string", text.c_str(), "-55" );
        D_TESTS_EXPECT( (dm::numeric_text(none).size() == 0u) &&
                        (!dm::parse_number(none).is_number()),
                        "a null C string: empty, not a number", "", "" );
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        {
            const std::string_view view("1 1/2xyz", 5u);

            D_TESTS_EXPECT( (dm::numeric_text(view).size() == 5u) &&
                            (dm::equal(view, "1.5")),
                            "a std::string_view", "1 1/2", "1.5" );
        }
#endif

        return;
    }


    //==========================================================================
    // 2.  INTEGERS
    //==========================================================================

    // d_tests_integer_texts
    //   integer_value from text and from dj_math_umax, and its queries.
    void
    d_tests_integer_texts()
    {
        const dj_math_umax most = static_cast<dj_math_umax>(0u) - 1u;

        struct row
        {
            const char* text;
            const char* digits;
        };

        const row rows[] = {
            { "0", "0" },     { "000", "0" },     { "", "0" },
            { "7", "7" },     { "0007", "7" },    { "12a", "12" },
            { "+5", "0" },    { "-5", "0" },      { " 5", "0" },
            { "x", "0" },     { "100", "100" },   { "1.5", "1" },
            { "123456789012345678901234567890",
              "123456789012345678901234567890" }
        };
        const unsigned count = sizeof(rows) / sizeof(rows[0]);
        unsigned       i     = 0;

        // each text
        for (i = 0; i < count; ++i)
        {
            const dm::integer_value x((dm::numeric_text(rows[i].text)));

            D_TESTS_EXPECT( (x.to_string() == rows[i].digits) &&
                            (x.digits() == std::strlen(rows[i].digits)) &&
                            (x.is_zero() ==
                             (std::string(rows[i].digits) == "0")),
                            "integer_value from text", rows[i].text,
                            rows[i].digits );
        }

        D_TESTS_EXPECT( (dm::integer_value().is_zero()) &&
                        (dm::integer_value().to_string() == "0") &&
                        (dm::integer_value().digits() == 1u),
                        "integer_value(): 0", "", "" );
        D_TESTS_EXPECT( (dm::integer_value(most).to_string() ==
                         d_tests_text_of(most)),
                        "integer_value of the widest unsigned integer's most",
                        d_tests_text_of(most).c_str(), "" );

        return;
    }

    // d_tests_integer_native
    //   every operation on random numbers whose results fit dj_math_umax,
    // against the platform's arithmetic.
    void
    d_tests_integer_native(
        long _count
    )
    {
        const int bits = d_tests_umax_bits();
        long      i    = 0;

        // each pair
        for (i = 0; i < _count; ++i)
        {
            const dj_math_umax a = d_tests_random_umax(bits / 2);
            const dj_math_umax b = d_tests_random_umax(
                                      (d_tests_below(4UL) == 0UL) ? bits / 2
                                                                  : 8);
            const dm::integer_value x(a);
            const dm::integer_value y(b);
            const std::string       ta = d_tests_text_of(a);
            const std::string       tb = d_tests_text_of(b);
            dm::integer_value       z(x);
            const int               want = (a < b) ? -1 : ((a > b) ? 1 : 0);

            z += y;

            D_TESTS_EXPECT( (x.to_string() == ta) &&
                            (x.digits() == ta.size()),
                            "integer_value(dj_math_umax)", ta.c_str(), "" );
            D_TESTS_EXPECT( ((x + y).to_string() == d_tests_text_of(a + b)) &&
                            (z == (x + y)),
                            "a sum", ta.c_str(), tb.c_str() );
            D_TESTS_EXPECT( (x - y).to_string() ==
                                ((a >= b) ? d_tests_text_of(a - b)
                                          : std::string("0")),
                            "a difference, 0 below 0", ta.c_str(),
                            tb.c_str() );
            D_TESTS_EXPECT( (x * y).to_string() == d_tests_text_of(a * b),
                            "a product", ta.c_str(), tb.c_str() );
            D_TESTS_EXPECT( (x.compare(y) == want)      &&
                            ((x < y) == (a < b))        &&
                            ((x > y) == (a > b))        &&
                            ((x == y) == (a == b))      &&
                            ((x != y) == (a != b))      &&
                            ((x <= y) == (a <= b))      &&
                            ((x >= y) == (a >= b)),
                            "the comparisons", ta.c_str(), tb.c_str() );
        }

        return;
    }

    // d_tests_integer_big
    //   numbers of up to 200 digits: the identities of a commutative ring
    // with cancellation, known products, and carries through runs of 9s.
    void
    d_tests_integer_big(
        long _count
    )
    {
        const dm::integer_value one(static_cast<dj_math_umax>(1u));
        const dm::integer_value two(static_cast<dj_math_umax>(2u));
        const dm::integer_value three(static_cast<dj_math_umax>(3u));
        dm::integer_value       power2(one);
        dm::integer_value       power3(one);
        long                    i = 0;

        // each triple
        for (i = 0; i < _count; ++i)
        {
            const std::string       ta = d_tests_digits(1UL +
                                                        d_tests_below(200UL));
            const std::string       tb = d_tests_digits(1UL +
                                                        d_tests_below(200UL));
            const std::string       tc = d_tests_digits(1UL +
                                                        d_tests_below(60UL));
            const dm::integer_value a((dm::numeric_text(ta)));
            const dm::integer_value b((dm::numeric_text(tb)));
            const dm::integer_value c((dm::numeric_text(tc)));

            D_TESTS_EXPECT( (a.to_string() == ta) &&
                            (a.digits() == ta.size()),
                            "digits read and written back", ta.c_str(), "" );
            D_TESTS_EXPECT( (((a + b) - b) == a) &&
                            ((a + b) == (b + a)) &&
                            ((a - (a + b)).is_zero()) &&
                            ((a - a).is_zero()),
                            "sums and differences", ta.c_str(), tb.c_str() );
            D_TESTS_EXPECT( ((a * b) == (b * a)) &&
                            (((a * b) * c) == (a * (b * c))) &&
                            (((a + b) * c) == ((a * c) + (b * c))) &&
                            ((a * one) == a) &&
                            ((a * dm::integer_value()).is_zero()),
                            "products", ta.c_str(), tb.c_str() );
            D_TESTS_EXPECT( ((a < (a + one)) && ((a + one) > a) &&
                             (a.compare(a) == 0) && (a <= a) && (a >= a)) &&
                            ((a.compare(b) == -b.compare(a))),
                            "orders", ta.c_str(), tb.c_str() );
        }

        // 2^200 and 3^150, by repeated products, and their product
        for (i = 0; i < 200; ++i)
        {
            power2 = power2 * two;
        }

        for (i = 0; i < 150; ++i)
        {
            power3 = power3 * three;
        }

        D_TESTS_EXPECT( power2.to_string() ==
                            "16069380442589902755419620923411626025222029937"
                            "82792835301376",
                        "2^200", "", "" );
        D_TESTS_EXPECT( power3.to_string() ==
                            "36998848503512697292470078245169664418647310038"
                            "9722973815184405301748249",
                        "3^150", "", "" );
        D_TESTS_EXPECT( (power2 * power3).to_string() ==
                            "59454857254069362884986028750765908201998441185"
                            "87451146656264515637316427910686512707369042187"
                            "47229713754019390778814623215595290624",
                        "2^200 3^150", "", "" );

        // (10^n - 1)^2 = 10^(2n) - 2 10^n + 1: 9s, an 8, 0s and a 1
        for (i = 1; i <= 120; i += 7)
        {
            const std::string       nines(static_cast<std::size_t>(i), '9');
            const dm::integer_value x((dm::numeric_text(nines)));
            std::string             want(static_cast<std::size_t>(i - 1),
                                         '9');

            want += "8";
            want += std::string(static_cast<std::size_t>(i - 1), '0');
            want += "1";

            D_TESTS_EXPECT( ((x * x).to_string() == want) &&
                            ((x + one).to_string() ==
                             "1" + std::string(static_cast<std::size_t>(i),
                                               '0')) &&
                            (((x + one) - one) == x),
                            "the carries of 10^n - 1", nines.c_str(), "" );
        }

        return;
    }


    //==========================================================================
    // 3.  RATIONALS
    //==========================================================================

    // d_tests_rationals
    //   rational_value from its parts against cross products in dj_math_umax,
    // and its queries; over 0, not a number.
    void
    d_tests_rationals(
        long _count
    )
    {
        const int                bits = d_tests_umax_bits();
        const dm::rational_value nan(dm::integer_value(
                                         static_cast<dj_math_umax>(1u)),
                                     dm::integer_value(),
                                     false);
        const dm::rational_value zero(dm::integer_value(),
                                      dm::integer_value(
                                          static_cast<dj_math_umax>(5u)),
                                      true);
        long                     i = 0;

        D_TESTS_EXPECT( (dm::rational_value().is_zero()) &&
                        (dm::rational_value().is_number()) &&
                        (!dm::rational_value().is_negative()),
                        "rational_value(): 0", "", "" );
        D_TESTS_EXPECT( (!nan.is_number()) && (!nan.is_zero()) &&
                        (!nan.is_negative()) && (nan.compare(nan) == 2) &&
                        (nan.compare(zero) == 2) && (zero.compare(nan) == 2),
                        "over 0: not a number, unordered", "", "" );
        D_TESTS_EXPECT( (zero.is_zero()) && (!zero.is_negative()) &&
                        (zero.compare(dm::rational_value()) == 0),
                        "-0/5: 0, with no sign", "", "" );

        // each pair, cross products within the widest unsigned integer
        for (i = 0; i < _count; ++i)
        {
            const dj_math_umax n1 = d_tests_random_umax(bits / 2 - 1);
            const dj_math_umax d1 = d_tests_random_umax(bits / 2 - 1) + 1u;
            const dj_math_umax n2 = d_tests_random_umax(bits / 2 - 1);
            const dj_math_umax d2 = d_tests_random_umax(bits / 2 - 1) + 1u;
            const bool         s1 = (d_tests_below(2UL) == 0UL);
            const bool         s2 = (d_tests_below(2UL) == 0UL);
            const dm::rational_value r1((dm::integer_value(n1)),
                                        dm::integer_value(d1), s1);
            const dm::rational_value r2((dm::integer_value(n2)),
                                        dm::integer_value(d2), s2);
            const dj_math_umax left  = n1 * d2;
            const dj_math_umax right = n2 * d1;
            const int          sign1 = (n1 == 0u) ? 0 : ((s1) ? -1 : 1);
            const int          sign2 = (n2 == 0u) ? 0 : ((s2) ? -1 : 1);
            const int          m     = (left < right) ? -1
                                     : ((left > right) ? 1 : 0);
            const int          want  = (sign1 != sign2)
                                           ? ((sign1 < sign2) ? -1 : 1)
                                           : ((sign1 < 0) ? -m
                                              : ((sign1 == 0) ? 0 : m));

            D_TESTS_EXPECT( (r1.compare(r2) == want) &&
                            (r2.compare(r1) == -want) &&
                            (r1.is_zero() == (n1 == 0u)) &&
                            (r1.is_negative() == (sign1 < 0)),
                            "a rational's order and queries",
                            d_tests_text_of(n1).c_str(),
                            d_tests_text_of(n2).c_str() );
        }

        return;
    }


    //==========================================================================
    // 4.  COMPARING TEXTS
    //==========================================================================

    // d_tests_check_pair
    //   compare, both ways round, and every predicate, for texts whose order
    // is _want: -1, 0, 1, or 2 for unordered.
    void
    d_tests_check_pair(
        const char* _a,
        const char* _b,
        int         _want,
        int         _line
    )
    {
        const int  order   = dm::compare(_a, _b);
        const int  reverse = dm::compare(_b, _a);
        const bool ok      =
            ( (order == _want) &&
              (reverse == ((_want == 2) ? 2 : -_want)) &&
              (dm::equal(_a, _b) == (_want == 0)) &&
              (dm::not_equal(_a, _b) == (_want != 0)) &&
              (dm::less_than(_a, _b) == (_want == -1)) &&
              (dm::greater_than(_a, _b) == (_want == 1)) &&
              (dm::less_than_or_equal(_a, _b) ==
               ( (_want == -1) || (_want == 0) )) &&
              (dm::greater_than_or_equal(_a, _b) ==
               ( (_want == 1) || (_want == 0) )) &&
              (dm::parse_number(_a).compare(dm::parse_number(_b)) ==
               order) );

        d_tests_expect(ok, "compare and the predicates", _a, _b, _line);

        return;
    }

    // the table: groups in ascending order, each one value written in
    // several ways, ending with an empty text
    const char* const g_groups[] = {
        "-1e1000000000000000000000000000000000000000",
        "-10e999999999999999999999999999999999999999",
        "", "-2e100000000000000000000",
        "-0.2e100000000000000000001",
        "", "-1e100000000000000000000", "-10e99999999999999999999",
        "", "-123456789012345678901234567890",
        "-123456789012345678901234567890.000",
        "-1.23456789012345678901234567890e29",
        "-123456789012345678901234567890/1",
        "-246913578024691357802469135780/2",
        "-123456789012345678901234567889 2/2",
        "", "-1.5", "-1 1/2", "-3/2", "-15e-1", "-0.15e1", "-1  500/1000",
        "-000001.500000", "-1 500000000000000000000/1000000000000000000000",
        "", "-1/3", "-0 1/3", "-2/6", "-1000/3000", "-00001/0003",
        "", "-0.3333333333333333333333333333333333333333",
        "", "-1e-100000000000000000000", "-0.01e-99999999999999999998",
        "", "0", "-0", "+0", "0.0", ".0", "0.", "0e999999999999999999999",
        "-0e-5", "0/7", "-0/7", "0 0/3", "-0 0/9", "000", "-0.000e+00",
        "", "1e-100000000000000000000", "0.1e-99999999999999999999",
        "10e-100000000000000000001",
        "", "1e-30", "0.000000000000000000000000000001",
        "1/1000000000000000000000000000000", "0.0001e-26",
        "", "0.3333333333333333333333333333333333333333",
        "", "1/3", "2/6", "0 1/3", "00001/00003", "100/300",
        "1000000000000000000000000000000/3000000000000000000000000000000",
        "", "0.5", ".5", "1/2", "5e-1", "50e-2", "0 1/2", "500/1000",
        "0.50000000000000000000000000000000000000000000000000", "5E-1",
        "+.5e0",
        "", "0.999999999999999999999999999999999999999999",
        "", "1", "1.0", "1.", "1e0", "+1", "10e-1", "1/1", "7/7", "0 7/7",
        "1 0/5", ("100000000000000000000000000000000000000000/"
                  "100000000000000000000000000000000000000000"),
        "0001", "1E+0",
        "", "1.0000000000000000000000000000000000000001",
        "", "2.3333333333333333333333333333",
        "", "7/3", "2 1/3", "14/6", "2  100/300",
        "", "9223372036854775807", "9.223372036854775807e18",
        "9223372036854775806 1/1", "18446744073709551614/2",
        "", "9223372036854775808", "9223372036854775807 1/1",
        "", "18446744073709551616", "1.8446744073709551616e19",
        "", "340282366920938463463374607431768211456",
        "340282366920938463463374607431768211456000/1000",
        "", "1e4932", "10e4931",
        "", "1e5000",
        "", "1e100000000000000000000", "10e99999999999999999999",
        "100000000000000000000e99999999999999999980",
        "", "2e100000000000000000000", "0.2e100000000000000000001",
        "", "1e1000000000000000000000000000000000000000",
        "0.1e1000000000000000000000000000000000000001",
        ""
    };

    // d_tests_table
    //   every pair of texts in the table: equal within a group, and ordered
    // as their groups are.
    void
    d_tests_table()
    {
        const int count = static_cast<int>(sizeof(g_groups) /
                                           sizeof(g_groups[0]));
        int       group_a = 0;
        int       i       = 0;

        // each text, and its group
        for (i = 0; i < count; ++i)
        {
            int group_b = 0;
            int j       = 0;

            // the end of a group
            if (g_groups[i][0] == '\0')
            {
                ++group_a;
                continue;
            }

            // each text against it
            for (j = 0; j < count; ++j)
            {
                // the end of a group
                if (g_groups[j][0] == '\0')
                {
                    ++group_b;
                    continue;
                }

                d_tests_check_pair(g_groups[i], g_groups[j],
                                   (group_a < group_b) ? -1
                                   : ((group_a > group_b) ? 1 : 0),
                                   __LINE__);
            }
        }

        return;
    }

    // d_tests_not_numbers
    //   texts that are not one literal, and fractions over 0: not numbers,
    // unordered with everything, themselves included.
    void
    d_tests_not_numbers()
    {
        const char* const texts[] = {
            "", "abc", "1/0", "0/0", "1 1/0", "-5/00", "1 0/0", "1.5/2",
            " 1", "1 ", "1e", "1e+", "--1", "+-1", "1/2/3", "0x10", "inf",
            "nan", "1,5", "1 1", "1/ 2", "1 /2", "1\t1/2", ".", "-", "e5",
            "1/-2", "1 -1/2", "1e5/2", "1/2e3", "1..2"
        };
        const char* const numbers[] = { "0", "1", "-1/2", "1e1000" };
        const unsigned    count = sizeof(texts) / sizeof(texts[0]);
        unsigned          i     = 0;

        // each text
        for (i = 0; i < count; ++i)
        {
            unsigned j = 0;

            D_TESTS_EXPECT( !dm::parse_number(texts[i]).is_number(),
                            "not a number", texts[i], "" );
            d_tests_check_pair(texts[i], texts[i], 2, __LINE__);

            // against numbers, and the next text
            for (j = 0; j < 4u; ++j)
            {
                d_tests_check_pair(texts[i], numbers[j], 2, __LINE__);
            }

            d_tests_check_pair(texts[i], texts[(i + 1u) % count], 2,
                               __LINE__);
        }

        return;
    }

    // d_tests_literal_text
    //   a random literal whose value is mostly an exact rational in the C
    // core's family: an integer, a decimal, scientific notation, a fraction
    // or a mixed number, of at most _digits digits in a term.
    std::string
    d_tests_literal_text(
        unsigned long _digits
    )
    {
        const unsigned long form = d_tests_below(5UL);
        std::string         text;
        char                exponent[16];

        // a sign, sometimes
        if (d_tests_below(3UL) == 0UL)
        {
            text = (d_tests_below(2UL) == 0UL) ? "-" : "+";
        }

        // the form
        if (form == 0UL)
        {
            text += d_tests_digits(1UL + d_tests_below(_digits));
        }
        else if (form == 1UL)
        {
            const std::string   digits = d_tests_digits(1UL +
                                                        d_tests_below(_digits));
            const unsigned long point  = d_tests_below(digits.size() + 1UL);

            text += digits.substr(0u, point) + "." + digits.substr(point);
        }
        else if (form == 2UL)
        {
            std::sprintf(exponent, "e%d",
                         static_cast<int>(d_tests_below(9UL)) - 4);
            text += d_tests_digits(1UL + d_tests_below(_digits / 2UL)) +
                    exponent;
        }
        else
        {
            // a mixed number's whole part
            if (form == 4UL)
            {
                text += d_tests_digits(1UL + d_tests_below(3UL)) + " ";
            }

            // the terms, one after the other, so that every compiler draws
            // the same random numbers
            const std::string num = d_tests_digits(1UL + d_tests_below(
                                                             _digits / 2UL));
            const std::string den = d_tests_digits(1UL + d_tests_below(
                                                             _digits / 2UL));

            text += num + "/" + den;
        }

        return text;
    }

    // d_tests_exact_of
    //   whether literal.h reads _text as an exact number, into *_out.
    bool
    d_tests_exact_of(
        const std::string&    _text,
        struct dj_math_value* _out
    )
    {
        return ( (d_math_literal_to_value(_out, _text.data(), _text.size())) &&
                 ( (_out->kind == D_MATH_VALUE_INTEGER) ||
                   (_out->kind == D_MATH_VALUE_RATIONAL) ) );
    }

    // d_tests_against_core
    //   random literals that the C core reads exactly: compare against
    // value.h's exact comparison of literal.h's values.
    void
    d_tests_against_core(
        long _count
    )
    {
        const unsigned long digits =
            (D_INTERNAL_RATIONAL_WORD_BITS >= 64u) ? 18UL : 9UL;
        long                i      = 0;
        long                exact  = 0;

        // each pair
        for (i = 0; i < _count; ++i)
        {
            const std::string    a  = d_tests_literal_text(digits);
            const std::string    b  = (d_tests_below(8UL) == 0UL)
                                         ? a
                                         : d_tests_literal_text(digits);
            struct dj_math_value va = d_math_value_none();
            struct dj_math_value vb = d_math_value_none();

            // both exact in the family
            if ( (d_tests_exact_of(a, &va)) &&
                 (d_tests_exact_of(b, &vb)) )
            {
                ++exact;
                d_tests_check_pair(a.c_str(), b.c_str(),
                                   static_cast<int>(
                                       d_math_value_cmp(va, vb)),
                                   __LINE__);
            }
        }

        D_TESTS_EXPECT( exact > (_count / 2L),
                        "most random literals exact in the family", "", "" );

        return;
    }

    // d_tests_rewrites
    //   random rationals of up to 60 digits, each written several ways,
    // which must be equal, and next to their neighbours: a numerator one
    // more is above, one less below.
    void
    d_tests_rewrites(
        long _count
    )
    {
        long i = 0;

        // each rational
        for (i = 0; i < _count; ++i)
        {
            const std::string       tw = "1" + d_tests_digits(
                                                       1UL +
                                                       d_tests_below(19UL));
            const std::string       tr = d_tests_digits(1UL +
                                                        d_tests_below(30UL));
            const std::string       tq = "1" + d_tests_digits(30UL);
            const dm::integer_value w((dm::numeric_text(tw)));
            const dm::integer_value r((dm::numeric_text(tr)));
            const dm::integer_value q((dm::numeric_text(tq)));
            const dm::integer_value p = (w * q) + r;
            const dm::integer_value g(
                d_tests_random_umax(30) + 1u);
            const std::string zeros(static_cast<std::size_t>(
                                        d_tests_below(40UL)), '0');
            const std::string sign = (d_tests_below(2UL) == 0UL) ? "-" : "";
            const std::string fraction = sign + p.to_string() + "/" + tq;
            const std::string mixed    = sign + tw + " " + tr + "/" + tq;
            const std::string scaled   = sign + p.to_string() + zeros + "/" +
                                         tq + zeros;
            const std::string factored = sign + (p * g).to_string() + "/" +
                                         (q * g).to_string();
            const std::string above    = sign + (p + dm::integer_value(
                                         static_cast<dj_math_umax>(1u)))
                                         .to_string() + "/" + tq;
            const std::string below    = sign + (p - dm::integer_value(
                                         static_cast<dj_math_umax>(1u)))
                                         .to_string() + "/" + tq;
            const int         up       = (sign == "-") ? -1 : 1;

            d_tests_check_pair(fraction.c_str(), mixed.c_str(), 0, __LINE__);
            d_tests_check_pair(fraction.c_str(), scaled.c_str(), 0,
                               __LINE__);
            d_tests_check_pair(mixed.c_str(), factored.c_str(), 0, __LINE__);
            d_tests_check_pair(above.c_str(), fraction.c_str(), up,
                               __LINE__);
            d_tests_check_pair(below.c_str(), mixed.c_str(), -up, __LINE__);

            // a decimal: p / 10^30 as digits and a point, in three ways
            {
                const std::string digits = p.to_string();
                const std::string padded =
                    std::string((digits.size() < 31u)
                                    ? (31u - digits.size()) : 0u, '0') +
                    digits;
                const std::size_t point = padded.size() - 30u;
                const std::string plain = sign + padded.substr(0u, point) +
                                          "." + padded.substr(point) + zeros;
                const std::string over  = sign + p.to_string() + "/1" +
                                          std::string(30u, '0');
                const std::string sci   = sign + p.to_string() + zeros +
                                          "e-" + d_tests_text_of(
                                              static_cast<dj_math_umax>(
                                                  30u + zeros.size()));

                d_tests_check_pair(plain.c_str(), over.c_str(), 0, __LINE__);
                d_tests_check_pair(sci.c_str(), plain.c_str(), 0, __LINE__);
                d_tests_check_pair((plain + "1").c_str(), over.c_str(), up,
                                   __LINE__);
            }
        }

        return;
    }

    // d_tests_sorted
    //   a sample of literals of every kind, sorted by compare: every pair
    // agrees both ways round, and the order is consistent with the sort.
    void
    d_tests_sorted(
        int _count
    )
    {
        std::string texts[200];
        int         n = (_count < 200) ? _count : 200;
        int         i = 0;
        int         j = 0;

        // the sample: random numbers, some with long exponents
        for (i = 0; i < n; ++i)
        {
            texts[i] = d_tests_literal_text(30UL);

            // a fraction over 0 is not a number: another
            while (!dm::parse_number(texts[i]).is_number())
            {
                texts[i] = d_tests_literal_text(30UL);
            }

            // a long exponent, sometimes
            if ( (d_tests_below(4UL) == 0UL) &&
                 (texts[i].find('/') == std::string::npos) )
            {
                texts[i] += (texts[i].find('e') == std::string::npos)
                                ? "e" : "";
                texts[i] += d_tests_digits(1UL + d_tests_below(25UL));
            }
        }

        // insertion sort, by compare
        for (i = 1; i < n; ++i)
        {
            const std::string key = texts[i];

            j = i;

            // move the larger ones up
            while ( (j > 0) &&
                    (dm::compare(texts[j - 1], key) == 1) )
            {
                texts[j] = texts[j - 1];
                --j;
            }

            texts[j] = key;
        }

        // every pair: the order the sort gave, and the same both ways
        for (i = 0; i < n; ++i)
        {
            for (j = 0; j < n; ++j)
            {
                const int order = dm::compare(texts[i], texts[j]);

                D_TESTS_EXPECT( (order == -dm::compare(texts[j], texts[i])) &&
                                ( (i < j) ? (order <= 0)
                                  : ((i > j) ? (order >= 0)
                                             : (order == 0)) ),
                                "a sorted sample's order", texts[i].c_str(),
                                texts[j].c_str() );
            }
        }

        return;
    }

    // d_tests_long_texts
    //   texts of thousands of digits, and fractions whose cross products
    // are thousands of digits long.
    void
    d_tests_long_texts()
    {
        const std::string threes(5000u, '3');
        const std::string third = "0." + threes;
        const std::string tenth = "1" + std::string(20000u, '0') + "/3";
        const std::string near  = std::string(20000u, '3') + ".4";
        const std::string some(1000u, '3');
        const std::string big   = some + "/1" + some;
        const std::string next  = some + "/1" + some.substr(1u) + "4";

        d_tests_check_pair(third.c_str(), "1/3", -1, __LINE__);
        d_tests_check_pair((third + "4").c_str(), "1/3", 1, __LINE__);
        d_tests_check_pair(tenth.c_str(), near.c_str(), -1, __LINE__);
        d_tests_check_pair(tenth.c_str(),
                           (std::string(20000u, '3') + ".3").c_str(), 1,
                           __LINE__);
        d_tests_check_pair(big.c_str(), (some + "0/1" + some + "0").c_str(),
                           0, __LINE__);
        d_tests_check_pair(big.c_str(), "0.25", -1, __LINE__);
        d_tests_check_pair(big.c_str(), next.c_str(), 1, __LINE__);

        return;
    }
}


int
main()
{
    d_tests_text();
    d_tests_integer_texts();
    d_tests_integer_native(3000L);
    d_tests_integer_big(300L);
    d_tests_rationals(3000L);
    d_tests_table();
    d_tests_not_numbers();
    d_tests_against_core(4000L);
    d_tests_rewrites(300L);
    d_tests_sorted(120);
    d_tests_long_texts();

    std::printf("t_numeric_compare_face: %ld checks, %ld failures\n",
                g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
