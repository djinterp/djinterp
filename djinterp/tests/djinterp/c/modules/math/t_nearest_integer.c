/*******************************************************************************
* djinterp [c/math]                                          t_nearest_integer.c
*
*   Conformance harness for c/math/nearest_integer.h. floor_c, ceil_c,
* trunc_c and round_c must be exact, and so the C library's floor, ceil,
* trunc and round, exact by C's definition, bit for bit, a zero's sign
* included: for float and for double, two million random patterns and two
* million values below 2^(P+1), where fractions are; every value within
* 2,000 places of each edge that matters -- a zero, the halves and
* integers near it, the least normal value, 2^(P-2) where the spacing
* becomes a half, 2^(P-1) where it becomes 1, 2^32, 2^63 and 2^64 where a
* conversion's reach ends, the largest value -- either sign; and for long
* double, in whatever format, two hundred thousand values and the same
* windows. C's special values (Annex F), every family; the plain forms, the
* library's.
*
*   With the argument "exhaustive" it checks every float, all 2^32 patterns,
* besides (its count wants a 64-bit long). Compiled as C++, it checks the
* overloads; from C++14, that the four are constant expressions, every
* family -- far past 2^63 among them -- with the bits they have at run time.
* It needs no 64-bit integer, so that ISO-strict C++98 on a 32-bit target,
* where d_math_umax has 32 bits, builds it too.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/math/t_nearest_integer.c -o t_nearest_integer -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_nearest_integer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.27
*******************************************************************************/
// std
#include <float.h>   // FLT_MANT_DIG ... LDBL_MAX
#include <math.h>    // floorf ... roundl, ldexpl, nextafter, isnan, signbit
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/nearest_integer.h"  // unit under
                                                                         // test
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


// the C library's functions, called through volatile pointers, so that no
// compiler substitutes its own evaluation, or folds a call into a constant
static float       (*volatile g_floorf)(float)             = floorf;
static float       (*volatile g_ceilf)(float)              = ceilf;
static float       (*volatile g_truncf)(float)             = truncf;
static float       (*volatile g_roundf)(float)             = roundf;
static double      (*volatile g_floor)(double)             = floor;
static double      (*volatile g_ceil)(double)              = ceil;
static double      (*volatile g_trunc)(double)             = trunc;
static double      (*volatile g_round)(double)             = round;
static long double (*volatile g_floorl)(long double)       = floorl;
static long double (*volatile g_ceill)(long double)        = ceill;
static long double (*volatile g_truncl)(long double)       = truncl;
static long double (*volatile g_roundl)(long double)       = roundl;

static long     g_checks   = 0;
static long     g_failures = 0;
static uint32_t g_state    = 0x9E3779B9u;

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

// xorshift on 32 bits, which every target has
static uint32_t
d_tests_random(void)
{
    g_state ^= g_state << 13;
    g_state ^= g_state >> 17;
    g_state ^= g_state << 5;

    return g_state;
}

// a uniform long double in [1, 2), from 128 random bits rounded to the
// format's
static long double
d_tests_unit(void)
{
    const long double word = 4294967296.0L;   // 2^32
    long double       u    = 0.0L;
    int               k    = 0;

    for (k = 0; k < 4; ++k)
    {
        u = (u + (long double)d_tests_random()) / word;
    }

    return 1.0L + u;
}

// bit-for-bit, NaN equal to NaN, -0 distinct from +0
static bool
d_tests_same_f(float _a, float _b)
{
    return ( (memcmp(&_a, &_b, sizeof(_a)) == 0) ||
             (isnan(_a) && isnan(_b)) );
}

static bool
d_tests_same_d(double _a, double _b)
{
    return ( (memcmp(&_a, &_b, sizeof(_a)) == 0) ||
             (isnan(_a) && isnan(_b)) );
}

static bool
d_tests_same_ld(long double _a, long double _b)
{
    // signbit's value is any nonzero for a negative: glibc's gives 1 for a
    // constant it folds and 512 for a long double at run time
    return ( ((_a == _b) && ((signbit(_a) != 0) == (signbit(_b) != 0))) ||
             (isnan(_a) && isnan(_b)) );
}

// d_tests_one_*
//   the four at one value, against the library's, bit for bit.
static void
d_tests_one_f(float _x)
{
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_floor_c(_x), g_floorf(_x)),
                   "f floor_c is floorf", _x);
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_ceil_c(_x), g_ceilf(_x)),
                   "f ceil_c is ceilf", _x);
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_trunc_c(_x), g_truncf(_x)),
                   "f trunc_c is truncf", _x);
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_round_c(_x), g_roundf(_x)),
                   "f round_c is roundf", _x);

    return;
}

static void
d_tests_one_d(double _x)
{
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_floor_c(_x), g_floor(_x)),
                   "d floor_c is floor", _x);
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_ceil_c(_x), g_ceil(_x)),
                   "d ceil_c is ceil", _x);
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_trunc_c(_x), g_trunc(_x)),
                   "d trunc_c is trunc", _x);
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_round_c(_x), g_round(_x)),
                   "d round_c is round", _x);

    return;
}

static void
d_tests_one_ld(long double _x)
{
    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_floor_c(_x), g_floorl(_x)),
                   "ld floor_c is floorl", _x);
    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_ceil_c(_x), g_ceill(_x)),
                   "ld ceil_c is ceill", _x);
    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_trunc_c(_x), g_truncl(_x)),
                   "ld trunc_c is truncl", _x);
    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_round_c(_x), g_roundl(_x)),
                   "ld round_c is roundl", _x);

    return;
}

// d_tests_edges
//   the edges a family's windows are taken about, P its precision: a zero,
// the halves and integers near it, 2^(P-2), 2^(P-1), 2^32, 2^63 and 2^64;
// the least normal and the largest values are the caller's.
static const long double g_edges[] = {
    0.0L, 0.5L, 1.0L, 1.5L, 2.0L, 2.5L, 3.5L, 100.5L,
    4294967296.0L,             // 2^32
    9223372036854775808.0L,    // 2^63
    18446744073709551616.0L    // 2^64
};

// d_tests_float
//   two million random patterns; two million values below 2^25, any sign;
// every float within 2,000 places of each edge, either sign; with
// _exhaustive, every float.
static void
d_tests_float(
    bool _exhaustive
)
{
    long     i    = 0;
    uint32_t bits = 0u;
    size_t   e    = 0u;

    for (i = 0; i < 2000000; ++i)
    {
        const uint32_t b = d_tests_random();
        float          x = 0.0f;

        memcpy(&x, &b, sizeof(x));
        d_tests_one_f(x);
        d_tests_one_f((float)ldexpl(d_tests_unit(),
                                    (int)(d_tests_random() % 28u) - 3) *
                      (((d_tests_random() % 2u) == 0u) ? 1.0f : -1.0f));
    }

    for (e = 0u; e < (sizeof(g_edges) / sizeof(g_edges[0])) + 3u; ++e)
    {
        const float edge = (e == sizeof(g_edges) / sizeof(g_edges[0]))
                               ? FLT_MIN
                           : (e == sizeof(g_edges) / sizeof(g_edges[0]) + 1u)
                               ? (float)ldexpl(1.0L, FLT_MANT_DIG - 2)
                           : (e == sizeof(g_edges) / sizeof(g_edges[0]) + 2u)
                               ? (float)ldexpl(1.0L, FLT_MANT_DIG - 1)
                               : (float)g_edges[e];
        float up   = edge;
        float down = edge;

        for (i = 0; i < 2000; ++i)
        {
            d_tests_one_f(up);
            d_tests_one_f(-up);
            d_tests_one_f(down);
            d_tests_one_f(-down);
            up   = nextafterf(up, FLT_MAX);
            down = nextafterf(down, -FLT_MAX);
        }
    }

    // the top: the largest values, and the infinity past them
    {
        float x = FLT_MAX;

        for (i = 0; i < 2000; ++i)
        {
            d_tests_one_f(x);
            d_tests_one_f(-x);
            x = nextafterf(x, 0.0f);
        }

        d_tests_one_f((float)HUGE_VAL);
        d_tests_one_f(-(float)HUGE_VAL);
    }

    if (_exhaustive)
    {
        do
        {
            float x = 0.0f;

            memcpy(&x, &bits, sizeof(x));
            d_tests_one_f(x);
            ++bits;
        } while (bits != 0u);
    }

    return;
}

// d_tests_double
//   two million random patterns; two million values below 2^54, any sign;
// every double within 2,000 places of each edge, either sign.
static void
d_tests_double(void)
{
    long   i = 0;
    size_t e = 0u;

    for (i = 0; i < 2000000; ++i)
    {
        const uint32_t b[2] = { d_tests_random(), d_tests_random() };
        double         x    = 0.0;

        memcpy(&x, b, sizeof(x));
        d_tests_one_d(x);
        d_tests_one_d((double)ldexpl(d_tests_unit(),
                                     (int)(d_tests_random() % 57u) - 3) *
                      (((d_tests_random() % 2u) == 0u) ? 1.0 : -1.0));
    }

    for (e = 0u; e < (sizeof(g_edges) / sizeof(g_edges[0])) + 3u; ++e)
    {
        const double edge = (e == sizeof(g_edges) / sizeof(g_edges[0]))
                                ? DBL_MIN
                            : (e == sizeof(g_edges) / sizeof(g_edges[0]) + 1u)
                                ? (double)ldexpl(1.0L, DBL_MANT_DIG - 2)
                            : (e == sizeof(g_edges) / sizeof(g_edges[0]) + 2u)
                                ? (double)ldexpl(1.0L, DBL_MANT_DIG - 1)
                                : (double)g_edges[e];
        double up   = edge;
        double down = edge;

        for (i = 0; i < 2000; ++i)
        {
            d_tests_one_d(up);
            d_tests_one_d(-up);
            d_tests_one_d(down);
            d_tests_one_d(-down);
            up   = nextafter(up, DBL_MAX);
            down = nextafter(down, -DBL_MAX);
        }
    }

    {
        double x = DBL_MAX;

        for (i = 0; i < 2000; ++i)
        {
            d_tests_one_d(x);
            d_tests_one_d(-x);
            x = nextafter(x, 0.0);
        }

        d_tests_one_d(HUGE_VAL);
        d_tests_one_d(-HUGE_VAL);
    }

    return;
}

// d_tests_long_double
//   in whatever format long double has: two hundred thousand values, most
// below 2^(P+2) and one in eight of any exponent; every long double within
// 2,000 places of each edge, either sign.
static void
d_tests_long_double(void)
{
    long   i = 0;
    size_t e = 0u;

    for (i = 0; i < 200000; ++i)
    {
        const int exponent =
            ((i % 8) == 0)
                ? (LDBL_MIN_EXP - LDBL_MANT_DIG +
                   (int)(d_tests_random() %
                         (uint32_t)(LDBL_MAX_EXP - LDBL_MIN_EXP +
                                    LDBL_MANT_DIG)))
                : ((int)(d_tests_random() % (uint32_t)(LDBL_MANT_DIG + 5)) -
                   3);
        const long double x = ldexpl(d_tests_unit(), exponent);

        d_tests_one_ld(x);
        d_tests_one_ld(-x);
    }

    for (e = 0u; e < (sizeof(g_edges) / sizeof(g_edges[0])) + 3u; ++e)
    {
        const long double edge =
            (e == sizeof(g_edges) / sizeof(g_edges[0]))
                ? LDBL_MIN
            : (e == sizeof(g_edges) / sizeof(g_edges[0]) + 1u)
                ? ldexpl(1.0L, LDBL_MANT_DIG - 2)
            : (e == sizeof(g_edges) / sizeof(g_edges[0]) + 2u)
                ? ldexpl(1.0L, LDBL_MANT_DIG - 1)
                : g_edges[e];
        long double up   = edge;
        long double down = edge;

        for (i = 0; i < 2000; ++i)
        {
            d_tests_one_ld(up);
            d_tests_one_ld(-up);
            d_tests_one_ld(down);
            d_tests_one_ld(-down);
            up   = nextafterl(up, LDBL_MAX);
            down = nextafterl(down, -LDBL_MAX);
        }
    }

    {
        long double x = LDBL_MAX;

        for (i = 0; i < 2000; ++i)
        {
            d_tests_one_ld(x);
            d_tests_one_ld(-x);
            x = nextafterl(x, 0.0L);
        }

        d_tests_one_ld((long double)HUGE_VAL);
        d_tests_one_ld(-(long double)HUGE_VAL);
    }

    return;
}

// D_TESTS_NEAREST_SPECIALS
//   Annex F's special values for one family, and the values each function
// is about: the zeros and the infinities are their own, and NaN gives NaN;
// a result that is a zero keeps the value's sign; a half goes away from
// zero, and the largest value below one stays at 0; the last half below
// 2^(P-1), which is 1 / EPS, and the integers past it. LEAST is the least
// subnormal.
#define D_TESTS_NEAREST_SPECIALS(T, S, SAME, EPS, LEAST)                      \
    {                                                                         \
        const T inf  = (T)HUGE_VAL;                                           \
        const T qnan = (T)nan("");                                            \
        const T half = (T)0.5;                                                \
        const T top  = (T)1 / (EPS);                                          \
                                                                              \
        D_TESTS_EXPECT( SAME(d_math_##S##_floor_c((T)0), (T)0) &&             \
                        SAME(d_math_##S##_floor_c(-(T)0), -(T)0) &&           \
                        SAME(d_math_##S##_ceil_c(-(T)0), -(T)0) &&            \
                        SAME(d_math_##S##_trunc_c(-(T)0), -(T)0) &&           \
                        SAME(d_math_##S##_round_c(-(T)0), -(T)0) &&           \
                        SAME(d_math_##S##_floor_c(inf), inf) &&               \
                        SAME(d_math_##S##_ceil_c(-inf), -inf) &&              \
                        SAME(d_math_##S##_trunc_c(-inf), -inf) &&             \
                        SAME(d_math_##S##_round_c(inf), inf) &&               \
                        isnan(d_math_##S##_floor_c(qnan)) &&                  \
                        isnan(d_math_##S##_ceil_c(qnan)) &&                   \
                        isnan(d_math_##S##_trunc_c(qnan)) &&                  \
                        isnan(d_math_##S##_round_c(qnan)),                    \
                        #S ": the zeros, the infinities, NaN", 0);            \
        D_TESTS_EXPECT( SAME(d_math_##S##_floor_c(-half), -(T)1) &&           \
                        SAME(d_math_##S##_ceil_c(-half), -(T)0) &&            \
                        SAME(d_math_##S##_trunc_c(-half), -(T)0) &&           \
                        SAME(d_math_##S##_round_c(-half), -(T)1) &&           \
                        SAME(d_math_##S##_round_c(half), (T)1) &&             \
                        SAME(d_math_##S##_round_c(-(T)0.25), -(T)0) &&        \
                        SAME(d_math_##S##_round_c(half - (EPS) / (T)4),       \
                             (T)0) &&                                         \
                        SAME(d_math_##S##_round_c((T)2.5), (T)3) &&           \
                        SAME(d_math_##S##_round_c(-(T)2.5), -(T)3) &&         \
                        SAME(d_math_##S##_ceil_c((T)1.25), (T)2) &&           \
                        SAME(d_math_##S##_floor_c(-(T)1.25), -(T)2) &&        \
                        SAME(d_math_##S##_floor_c(LEAST), (T)0) &&            \
                        SAME(d_math_##S##_ceil_c(LEAST), (T)1) &&             \
                        SAME(d_math_##S##_floor_c(-LEAST), -(T)1) &&          \
                        SAME(d_math_##S##_ceil_c(-LEAST), -(T)0) &&           \
                        SAME(d_math_##S##_round_c(-LEAST), -(T)0),            \
                        #S ": halves, quarters, the least values", 0);        \
        D_TESTS_EXPECT( SAME(d_math_##S##_floor_c(top - half),                \
                             top - (T)1) &&                                   \
                        SAME(d_math_##S##_ceil_c(top - half), top) &&         \
                        SAME(d_math_##S##_round_c(top - half), top) &&        \
                        SAME(d_math_##S##_trunc_c(-(top - half)),             \
                             -(top - (T)1)) &&                                \
                        SAME(d_math_##S##_round_c(-(top - half)), -top) &&    \
                        SAME(d_math_##S##_floor_c(top + (T)1),                \
                             top + (T)1) &&                                   \
                        SAME(d_math_##S##_round_c(-(top + (T)3)),             \
                             -(top + (T)3)),                                  \
                        #S ": the last half below 2^(P-1), past it", 0);      \
    }

// d_tests_specials
//   every family's special values; the plain forms, the library's at every
// level, C++98 included -- there trunc and round are the _c forms, which
// agree with it.
static void
d_tests_specials(void)
{
    volatile double v = -2.5;
    volatile float  w = 7.5f;

    D_TESTS_NEAREST_SPECIALS(float, f, d_tests_same_f, FLT_EPSILON,
                             FLT_MIN * FLT_EPSILON)
    D_TESTS_NEAREST_SPECIALS(double, d, d_tests_same_d, DBL_EPSILON,
                             DBL_MIN * DBL_EPSILON)
    D_TESTS_NEAREST_SPECIALS(long double, ld, d_tests_same_ld, LDBL_EPSILON,
                             LDBL_MIN * LDBL_EPSILON)

    // the N5 cases: past 2^63, where a long long would have wrapped. Each
    // value is stored in its family's type first: on the x87 at C99 a
    // constant carries long double's precision (FLT_EVAL_METHOD 2), and
    // 1e300 as a long double is not 1e300 as a double
    {
        const double      n5   = 9.3e18;
        const double      big  = 1e300;
        const long double lbig = 1e19L;
        const float       fbig = 1e30f;

        D_TESTS_EXPECT( (d_math_d_floor_c(n5) == n5) &&
                        (d_math_d_floor_c(big) == big) &&
                        (d_math_d_ceil_c(-big) == -big) &&
                        (d_math_d_round_c(n5) == n5) &&
                        (d_math_ld_trunc_c(lbig) == lbig) &&
                        (d_math_f_floor_c(fbig) == fbig),
                        "past 2^63 a value is its own", 0);
    }

    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_floor(v), g_floor(v))) &&
                    (d_tests_same_d(d_math_d_ceil(v), g_ceil(v))) &&
                    (d_tests_same_d(d_math_d_trunc(v), g_trunc(v))) &&
                    (d_tests_same_d(d_math_d_round(v), g_round(v))) &&
                    (d_tests_same_f(d_math_f_round(w), g_roundf(w))) &&
                    (d_tests_same_ld(d_math_ld_floor((long double)v),
                                     g_floorl((long double)v))),
                    "the plain forms are the C library's", 0);

    return;
}

#if defined(__cplusplus)

// d_tests_overloads
//   compiled as C++: one name for every family, chosen by argument type.
static void
d_tests_overloads(void)
{
    D_TESTS_EXPECT( (d_tests_same_f(d_math_floor_c(-7.5f),
                                    d_math_f_floor_c(-7.5f))) &&
                    (d_tests_same_d(d_math_ceil_c(-7.5),
                                    d_math_d_ceil_c(-7.5))) &&
                    (d_tests_same_ld(d_math_trunc_c(-7.5L),
                                     d_math_ld_trunc_c(-7.5L))) &&
                    (d_tests_same_d(d_math_round_c(-7.5),
                                    d_math_d_round_c(-7.5))) &&
                    (d_tests_same_d(d_math_floor(7.5), d_math_d_floor(7.5))) &&
                    (d_tests_same_f(d_math_ceil(7.5f), d_math_f_ceil(7.5f))) &&
                    (d_tests_same_ld(d_math_trunc(7.5L),
                                     d_math_ld_trunc(7.5L))) &&
                    (d_tests_same_d(d_math_round(7.5), d_math_d_round(7.5))),
                    "the C++ overloads are the families' functions", 0);

    return;
}

#endif  // __cplusplus

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// from C++14, the four are constant expressions: every family, a zero's
// sign, a half each way, and far past 2^63
static constexpr double      g_cfloor_d  = d_math_d_floor_c(-0.5);
static constexpr double      g_cceil_d   = d_math_d_ceil_c(-0.5);
static constexpr double      g_ctrunc_d  = d_math_d_trunc_c(-2.75);
static constexpr double      g_cround_d  = d_math_d_round_c(-2.5);
static constexpr float       g_cround_f  = d_math_f_round_c(0.49999997f);
static constexpr long double g_cfloor_ld = d_math_ld_floor_c(-1e19L);
static constexpr long double g_cround_ld = d_math_ld_round_c(2.5L);
static constexpr double      g_cbig_d    = d_math_d_floor_c(9.3e18);

static_assert(d_math_d_floor_c(-0.5) == -1.0, "floor_c(-0.5) is -1");
static_assert(d_math_d_ceil_c(-0.5) == 0.0, "ceil_c(-0.5) is -0");
static_assert(d_math_d_round_c(2.5) == 3.0, "round_c(2.5) is 3");
static_assert(d_math_d_round_c(0.49999999999999994) == 0.0,
              "round_c of the largest double below a half is 0");
static_assert(d_math_d_floor_c(9.3e18) == 9.3e18,
              "floor_c(9.3e18) is 9.3e18, where a long long wrapped");
static_assert(d_math_d_floor_c(1e300) == 1e300, "floor_c(1e300)");
static_assert(d_math_d_trunc_c(4503599627370495.5) == 4503599627370495.0,
              "trunc_c of the last half below 2^52");
static_assert(d_math_round_c(-1.5f) == -2.0f, "the C++ overloads");
static_assert(d_math_d_floor_c((double)NAN) != d_math_d_floor_c((double)NAN),
              "floor_c(NaN) is NaN, as a constant");
static_assert(d_math_d_ceil_c(-(double)HUGE_VAL) == -(double)HUGE_VAL,
              "ceil_c(-inf) is -inf, as a constant");

// d_tests_constant_evaluation
//   the constants are the run time's results, bit for bit -- a zero's sign
// included: the arguments are read through volatiles, so the calls below
// are not folded.
static void
d_tests_constant_evaluation(void)
{
    volatile double      half    = -0.5;
    volatile double      t       = -2.75;
    volatile double      r       = -2.5;
    volatile float       f       = 0.49999997f;
    volatile long double big     = -1e19L;
    volatile long double lhalf   = 2.5L;
    volatile double      n5      = 9.3e18;

    D_TESTS_EXPECT( (d_tests_same_d(g_cfloor_d, d_math_d_floor_c(half))) &&
                    (d_tests_same_d(g_cceil_d, d_math_d_ceil_c(half))) &&
                    (d_tests_same_d(g_ctrunc_d, d_math_d_trunc_c(t))) &&
                    (d_tests_same_d(g_cround_d, d_math_d_round_c(r))) &&
                    (d_tests_same_f(g_cround_f, d_math_f_round_c(f))) &&
                    (d_tests_same_ld(g_cfloor_ld, d_math_ld_floor_c(big))) &&
                    (d_tests_same_ld(g_cround_ld,
                                     d_math_ld_round_c(lhalf))) &&
                    (d_tests_same_d(g_cbig_d, d_math_d_floor_c(n5))),
                    "constant evaluation agrees with the run time", 0);

    return;
}

#endif  // C++14

int
main(int _argc, char** _argv)
{
    const bool exhaustive = (_argc > 1) &&
                            (strcmp(_argv[1], "exhaustive") == 0);

    d_tests_float(exhaustive);
    d_tests_double();
    d_tests_long_double();
    d_tests_specials();
#if defined(__cplusplus)
    d_tests_overloads();
#endif
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    printf("t_nearest_integer: %ld checks, %ld failures\n", g_checks,
           g_failures);

    return (g_failures == 0) ? 0 : 1;
}
