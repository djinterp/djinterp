/*******************************************************************************
* djinterp [test]                                                t_float_order.c
*
*   Conformance harness for c/math/float_order.h, all three families: next_up
* and next_down against the C library's nextafter -- over windows of every
* consecutive float around zero, the least normal value, 1 and the largest
* finite value, over a million random values of each type, and at the
* special values -- and steps and advance against stepping with nextafter,
* near and across zero and across binades, and against each other.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -Wall -Wextra -Werror -pedantic-errors -Iinc                 \
*        tests/djinterp/c/math/t_float_order.c -o t_float_order -lm
*
*
* path:      /tests/djinterp/c/math/t_float_order.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
// std
#include <float.h>   // FLT_MIN ... LDBL_MAX
#include <math.h>    // nextafterf, nextafter, nextafterl, ldexpl, isnan
#include <stdio.h>   // printf
#include <string.h>  // memcpy
// djinterp
#include "../../../../inc/djinterp/c/math/float_order.h"  // unit under test
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t, uint64_t
                                                     // where they exist


static long g_checks   = 0;
static long g_failures = 0;

static void
d_tests_expect(
    bool        _ok,
    const char* _what,
    long double _value,
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
            printf("  FAIL line %d: %s (%.21Lg)\n", _line, _what, _value);
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what, value)                                       \
    d_tests_expect((ok), (what), (long double)(value), __LINE__)

#if defined(UINT64_MAX)
static uint64_t g_state = 0x9E3779B97F4A7C15u;

static uint64_t
d_tests_random(void)
{
    g_state ^= g_state << 13;
    g_state ^= g_state >> 7;
    g_state ^= g_state << 17;

    return g_state;
}
#endif  // UINT64_MAX

// the same value, ±0 being one: what the order promises
static bool d_tests_same_f(float _a, float _b)
{
    return ((_a == _b) || (isnan(_a) && isnan(_b)));
}

static bool d_tests_same_d(double _a, double _b)
{
    return ((_a == _b) || (isnan(_a) && isnan(_b)));
}

static bool d_tests_same_ld(long double _a, long double _b)
{
    return ((_a == _b) || (isnan(_a) && isnan(_b)));
}

static void
d_tests_one_f(float _x)
{
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_next_up(_x),
                                  nextafterf(_x, (float)HUGE_VAL)),
                   "f next_up", _x);
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_next_down(_x),
                                  nextafterf(_x, -(float)HUGE_VAL)),
                   "f next_down", _x);

    return;
}

static void
d_tests_one_d(double _x)
{
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_next_up(_x),
                                  nextafter(_x, HUGE_VAL)),
                   "d next_up", _x);
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_next_down(_x),
                                  nextafter(_x, -HUGE_VAL)),
                   "d next_down", _x);

    return;
}

static void
d_tests_one_ld(long double _x)
{
    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_next_up(_x),
                                   nextafterl(_x, (long double)HUGE_VAL)),
                   "ld next_up", _x);
    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_next_down(_x),
                                   nextafterl(_x, -(long double)HUGE_VAL)),
                   "ld next_down", _x);

    return;
}

// d_tests_window_f
//   every float from _start for _count steps: each neighbour, and the
// distance from _start to each, both ways.
static void
d_tests_window_f(float _start, long _count)
{
    float x = _start;
    long  i = 0;

    for (i = 0; i < _count; ++i)
    {
        d_tests_one_f(x);

        // steps and advance agree with stepping, every 97th finite value
        if ( ((i % 97) == 0) &&
             (x <= FLT_MAX) )
        {
            D_TESTS_EXPECT(d_math_f_steps(_start, x) == (d_math_umax)i,
                           "f steps over a window", x);
            D_TESTS_EXPECT(d_tests_same_f(d_math_f_advance(_start,
                                                           (d_math_umax)i),
                                          x),
                           "f advance over a window", x);
        }

        x = nextafterf(x, (float)HUGE_VAL);
    }

    return;
}

static void
d_tests_window_d(double _start, long _count)
{
    double x = _start;
    long   i = 0;

    for (i = 0; i < _count; ++i)
    {
        d_tests_one_d(x);

        // (a 32-bit d_math_umax cannot count across a double binade)
        if ( ((i % 97) == 0) &&
             (x <= DBL_MAX) &&
             (sizeof(d_math_umax) >= 8u) )
        {
            D_TESTS_EXPECT(d_math_d_steps(_start, x) == (d_math_umax)i,
                           "d steps over a window", x);
            D_TESTS_EXPECT(d_tests_same_d(d_math_d_advance(_start,
                                                           (d_math_umax)i),
                                          x),
                           "d advance over a window", x);
        }

        x = nextafter(x, HUGE_VAL);
    }

    return;
}

static void
d_tests_window_ld(long double _start, long _count)
{
    long double x = _start;
    long        i = 0;

    for (i = 0; i < _count; ++i)
    {
        d_tests_one_ld(x);

        if ( ((i % 97) == 0) &&
             (x <= LDBL_MAX) &&
             (sizeof(d_math_umax) >= 8u) )
        {
            D_TESTS_EXPECT(d_math_ld_steps(_start, x) == (d_math_umax)i,
                           "ld steps over a window", x);
            D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_advance(_start,
                                                             (d_math_umax)i),
                                           x),
                           "ld advance over a window", x);
        }

        x = nextafterl(x, (long double)HUGE_VAL);
    }

    return;
}

#if defined(UINT64_MAX)
// d_tests_random_values
//   for random values: the neighbours, and advance(a, steps(a, b)) == b and
// steps(a, advance(a, n)) == n wherever the count does not saturate.
static void
d_tests_random_values(void)
{
    long i = 0;

    for (i = 0; i < 1000000; ++i)
    {
        const uint64_t bits  = d_tests_random();
        const uint32_t bits32 = (uint32_t)(bits >> 32);
        float          f      = 0.0f;
        double         d      = 0.0;

        memcpy(&f, &bits32, sizeof(f));
        memcpy(&d, &bits, sizeof(d));

        d_tests_one_f(f);
        d_tests_one_d(d);

        // a pair of finite doubles of one sign, and a distance between them
        if ( (!isnan(d)) &&
             (d < DBL_MAX) &&
             (d > -DBL_MAX) )
        {
            const d_math_umax n = d_tests_random() >> 20;
            const double      e = d_math_d_advance(d, n);

            // the value n steps on, unless that passed the largest finite one
            if (e <= DBL_MAX)
            {
                D_TESTS_EXPECT(d_math_d_steps(d, e) == n,
                               "d steps(a, advance(a, n)) == n", d);
                D_TESTS_EXPECT(d_tests_same_d(
                                   d_math_d_advance(d, d_math_d_steps(d, e)),
                                   e),
                               "d advance(a, steps(a, b)) == b", d);
            }
        }

        // a pair of finite floats, and every distance between them counts
        if ( (!isnan(f)) &&
             (f < FLT_MAX) &&
             (f > -FLT_MAX) )
        {
            const d_math_umax n = d_tests_random() >> 34;
            const float       e = d_math_f_advance(f, n);

            if (e <= FLT_MAX)
            {
                D_TESTS_EXPECT(d_math_f_steps(f, e) == n,
                               "f steps(a, advance(a, n)) == n", f);
            }
        }
    }

    // long double: random mantissa and exponent across the whole range
    for (i = 0; i < 300000; ++i)
    {
        const uint64_t    bits = d_tests_random();
        const long double m    = 0.5L + ((long double)(bits >> 12) /
                                         (long double)((uint64_t)1 << 53));
        const int         e    = (int)(d_tests_random() % 32760u) - 16380;
        const long double x    = ((bits & 1u) ? -1.0L : 1.0L) * ldexpl(m, e);

        d_tests_one_ld(x);

        // a distance that stays within d_math_umax
        {
            const d_math_umax n = d_tests_random() >> 4;
            const long double y = d_math_ld_advance(x, n);

            if (y <= LDBL_MAX)
            {
                D_TESTS_EXPECT(d_math_ld_steps(x, y) == n,
                               "ld steps(a, advance(a, n)) == n", x);
            }
        }
    }

    return;
}
#endif  // UINT64_MAX

static void
d_tests_specials(void)
{
    const float       fs[]  = { 0.0f, -0.0f, FLT_MIN, -FLT_MIN, FLT_MAX,
                                -FLT_MAX, 1.0f, -1.0f, 2.0f, 0.5f,
                                FLT_MIN * FLT_EPSILON, (float)HUGE_VAL,
                                -(float)HUGE_VAL };
    const double      ds[]  = { 0.0, -0.0, DBL_MIN, -DBL_MIN, DBL_MAX,
                                -DBL_MAX, 1.0, -1.0, 2.0, 0.5,
                                DBL_MIN * DBL_EPSILON, HUGE_VAL, -HUGE_VAL };
    const long double lds[] = { 0.0L, -0.0L, LDBL_MIN, -LDBL_MIN, LDBL_MAX,
                                -LDBL_MAX, 1.0L, -1.0L, 2.0L, 0.5L,
                                LDBL_MIN * LDBL_EPSILON,
                                (long double)HUGE_VAL,
                                -(long double)HUGE_VAL };
    size_t i = 0;

    for (i = 0; i < sizeof(fs) / sizeof(fs[0]); ++i)
    {
        d_tests_one_f(fs[i]);
    }

    for (i = 0; i < sizeof(ds) / sizeof(ds[0]); ++i)
    {
        d_tests_one_d(ds[i]);
    }

    for (i = 0; i < sizeof(lds) / sizeof(lds[0]); ++i)
    {
        d_tests_one_ld(lds[i]);
    }

    // NaN is its own neighbour
    D_TESTS_EXPECT(isnan(d_math_d_next_up(nan(""))), "d NaN next_up", 0);

    // a d_math_umax of 32 bits (ISO strict C++98 on a 32-bit target) holds
    // no double binade's count: within one the count is exact, across one
    // it saturates
    if (sizeof(d_math_umax) < 8u)
    {
        D_TESTS_EXPECT(d_math_d_steps(1.0, 1.0 + (1000.0 * DBL_EPSILON)) ==
                       (d_math_umax)1000u,
                       "narrow d steps within a binade", 0);
        D_TESTS_EXPECT(d_math_d_steps(1.0, 2.0) == D_MATH_UMAX_MAX,
                       "narrow d steps across a binade saturate", 0);
        D_TESTS_EXPECT(d_math_d_advance(2.0 - DBL_EPSILON, 3u) ==
                       2.0 + (4.0 * DBL_EPSILON),
                       "narrow d advance across a binade edge", 0);
        D_TESTS_EXPECT(d_math_d_advance(-1.0, 5u) ==
                       -1.0 + (5.0 * DBL_EPSILON / 2.0),
                       "narrow d advance below zero", 0);
        D_TESTS_EXPECT(d_math_f_steps(1.0f, 2.0f) == ((d_math_umax)1 << 23),
                       "narrow f steps over a binade", 0);

        return;
    }

    // whole ranges, counted: every finite float from -FLT_MAX to FLT_MAX is
    // 2 * (2^31 - 2^23) - 1 values, so 2^32 - 2^24 - 2 steps
    D_TESTS_EXPECT(d_math_f_steps(-FLT_MAX, FLT_MAX) ==
                   (d_math_umax)4278190078u,
                   "f steps over every finite float", 0);
    D_TESTS_EXPECT(d_math_f_advance(-FLT_MAX, (d_math_umax)4278190078u) ==
                   FLT_MAX,
                   "f advance over every finite float", 0);
    // (the shifts are reduced so a narrow d_math_umax still compiles them)
    D_TESTS_EXPECT(d_math_d_steps(1.0, 2.0) ==
                   ((d_math_umax)1 << (52 % (sizeof(d_math_umax) * 8u))),
                   "d steps over a binade", 0);
    D_TESTS_EXPECT( (LDBL_MANT_DIG != 64) ||
                    (d_math_ld_steps(1.0L, 2.0L) ==
                     ((d_math_umax)1 << ((LDBL_MANT_DIG - 1) %
                                         (sizeof(d_math_umax) * 8u)))),
                    "ld steps over a binade", 0);
    D_TESTS_EXPECT(d_math_ld_steps(1.0L, 4.0L) == D_MATH_UMAX_MAX,
                   "ld steps over two binades saturate", 0);
    D_TESTS_EXPECT(d_math_d_advance(DBL_MAX, 1u) == HUGE_VAL,
                   "d advance past the largest finite value", 0);

    // the infinities are the order's ends
    D_TESTS_EXPECT(d_math_f_steps(FLT_MAX, (float)HUGE_VAL) == 1u,
                   "f one step from the largest float to +inf", 0);
    D_TESTS_EXPECT(d_math_d_advance(-HUGE_VAL, 1u) == -DBL_MAX,
                   "d one step up from -inf", 0);
    D_TESTS_EXPECT(d_math_d_advance(-HUGE_VAL, 2u) ==
                   d_math_d_next_up(-DBL_MAX),
                   "d two steps up from -inf", 0);
    D_TESTS_EXPECT(d_math_f_steps(-(float)HUGE_VAL, (float)HUGE_VAL) ==
                   (d_math_umax)4278190078u + 2u,
                   "f every float and both infinities", 0);

    return;
}

int
main(void)
{
    d_tests_specials();

    // every consecutive float in four windows: across zero and the
    // subnormals, the least normal value, 1, and up to the largest
    d_tests_window_f(-(FLT_MIN * FLT_EPSILON) * 200000.0f, 400000);
    d_tests_window_f(FLT_MIN - ((FLT_MIN * FLT_EPSILON) * 200000.0f), 400000);
    d_tests_window_f(0.99f, 400000);
    d_tests_window_f(FLT_MAX * 0.9999f, 200000);
    d_tests_window_d(-(DBL_MIN * DBL_EPSILON) * 100000.0, 200000);
    d_tests_window_d(DBL_MIN - ((DBL_MIN * DBL_EPSILON) * 100000.0), 200000);
    d_tests_window_d(0.9999999999999, 200000);
    d_tests_window_d(DBL_MAX * 0.99999999999999, 200000);
    d_tests_window_ld(-(LDBL_MIN * LDBL_EPSILON) * 50000.0L, 100000);
    d_tests_window_ld(0.9999999999999999L, 100000);

#if defined(UINT64_MAX)
    d_tests_random_values();
#endif  // UINT64_MAX

    printf("t_float_order: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
