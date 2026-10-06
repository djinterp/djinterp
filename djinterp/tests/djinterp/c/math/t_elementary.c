/*******************************************************************************
* djinterp [test]                                                 t_elementary.c
*
*   Conformance harness for c/math/elementary.h. sqrt_c must be the C
* library's sqrt, bit for bit: every float in [1, 4) -- every mantissa the
* root's core sees -- and one float in 37 across the whole range; ten million
* random doubles, squares of random doubles and their neighbours (the cases
* nearest a rounding midpoint), every power of two, the subnormals; two
* million random long doubles. acos_c must be within two ulps of acos over
* random values of [-1, 1] and at its edges; the largest error is printed.
*
*   sin_c and cos_c must be within an ulp of sin and cos for float and double
* and two for long double, below 2^20 and at the multiples of pi/2.
*
*   With the argument "exhaustive" it checks every float, all 2^32 patterns.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/math/t_elementary.c -o t_elementary -lm
*
*
* path:      /tests/djinterp/c/math/t_elementary.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
// std
#include <float.h>   // FLT_MAX, DBL_EPSILON, LDBL_MANT_DIG
#include <math.h>    // sqrtf, sqrt, sqrtl, acosf, acos, acosl, nextafter
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strcmp
// djinterp
#include "../../../../inc/djinterp/c/math/elementary.h"  // unit under test
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t, uint64_t


// the C library's functions, called through volatile pointers: clang at -O2
// on a 32-bit x87 target inlines sqrt as fsqrt, which rounds to 64 bits and
// then to 53 -- twice -- and is not the library's correctly rounded root
static float       (*volatile g_sqrtf)(float)             = sqrtf;
static double      (*volatile g_sqrt)(double)             = sqrt;
static long double (*volatile g_sqrtl)(long double)       = sqrtl;
// (and sin and cos, which x87's fsin and fcos approximate at a large
// argument, and clang inlines them too)
static float       (*volatile g_sinf)(float)              = sinf;
static double      (*volatile g_sin)(double)              = sin;
static long double (*volatile g_sinl)(long double)        = sinl;
static float       (*volatile g_cosf)(float)              = cosf;
static double      (*volatile g_cos)(double)              = cos;
static long double (*volatile g_cosl)(long double)        = cosl;

static long     g_checks   = 0;
static long     g_failures = 0;
static uint64_t g_state    = 0x2545F4914F6CDD1Du;

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

static uint64_t
d_tests_random(void)
{
    g_state ^= g_state << 13;
    g_state ^= g_state >> 7;
    g_state ^= g_state << 17;

    return g_state;
}

// bit-for-bit, NaN equal to NaN, -0 distinct from +0
static bool
d_tests_same_f(float _a, float _b)
{
    uint32_t a = 0u;
    uint32_t b = 0u;

    memcpy(&a, &_a, sizeof(a));
    memcpy(&b, &_b, sizeof(b));

    return ((a == b) || (isnan(_a) && isnan(_b)));
}

static bool
d_tests_same_d(double _a, double _b)
{
    uint64_t a = 0u;
    uint64_t b = 0u;

    memcpy(&a, &_a, sizeof(a));
    memcpy(&b, &_b, sizeof(b));

    return ((a == b) || (isnan(_a) && isnan(_b)));
}

static bool
d_tests_same_ld(long double _a, long double _b)
{
    return ( ((_a == _b) && (signbit(_a) == signbit(_b))) ||
             (isnan(_a) && isnan(_b)) );
}

static void
d_tests_sqrt_float(bool _exhaustive)
{
    uint64_t bits = 0u;
    float    x    = 1.0f;

    // every float in [1, 4): every mantissa the unit root sees, both binades
    while (x < 4.0f)
    {
        D_TESTS_EXPECT(d_tests_same_f(d_math_f_sqrt_c(x), g_sqrtf(x)),
                       "f sqrt_c, [1, 4)", x);
        x = nextafterf(x, 5.0f);
    }

    // one pattern in 37 across all of them, or every one
    for (bits = 0u; bits <= 0xFFFFFFFFu; bits += (_exhaustive ? 1u : 37u))
    {
        const uint32_t b = (uint32_t)bits;
        float          f = 0.0f;

        memcpy(&f, &b, sizeof(f));
        D_TESTS_EXPECT(d_tests_same_f(d_math_f_sqrt_c(f), g_sqrtf(f)),
                       "f sqrt_c", f);
    }

    return;
}

static void
d_tests_sqrt_double(void)
{
    long   i  = 0;
    double p2 = DBL_MIN * DBL_EPSILON;

    // random patterns, all signs and exponents
    for (i = 0; i < 10000000; ++i)
    {
        const uint64_t b = d_tests_random();
        double         d = 0.0;

        memcpy(&d, &b, sizeof(d));
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_sqrt_c(d), g_sqrt(d)),
                       "d sqrt_c", d);
    }

    // squares of random doubles in [1, 2), and their neighbours: the values
    // whose roots lie nearest a rounding midpoint
    for (i = 0; i < 2000000; ++i)
    {
        const double c  = 1.0 + ((double)(d_tests_random() >> 11) *
                                 DBL_EPSILON);
        // stored, so the square is a double: clang on x87 would otherwise
        // hand an inlined kernel the product's 64-bit register value
        volatile double square = c * c;
        const double    sq     = square;
        const double up = nextafter(sq, 10.0);
        const double dn = nextafter(sq, 0.0);

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_sqrt_c(sq), g_sqrt(sq)),
                       "d sqrt_c of a square", sq);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_sqrt_c(up), g_sqrt(up)),
                       "d sqrt_c above a square", up);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_sqrt_c(dn), g_sqrt(dn)),
                       "d sqrt_c below a square", dn);
    }

    // every power of two, from the least subnormal up
    while (p2 <= DBL_MAX)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_sqrt_c(p2), g_sqrt(p2)),
                       "d sqrt_c of a power of two", p2);

        if (p2 > DBL_MAX / 2.0)
        {
            break;
        }

        p2 *= 2.0;
    }

    return;
}

static void
d_tests_sqrt_long_double(void)
{
    long i = 0;

    for (i = 0; i < 2000000; ++i)
    {
        const uint64_t    bits = d_tests_random();
        const long double m    = 0.5L + ((long double)(bits >> 1) /
                                         ((long double)((uint64_t)1 << 63) *
                                          2.0L));
        const int         e    = (int)(d_tests_random() % 32760u) - 16380;
        const long double x    = ldexpl(m, e);

        D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_sqrt_c(x), g_sqrtl(x)),
                       "ld sqrt_c", x);
    }

    // the specials
    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_sqrt_c(-0.0L), -0.0L),
                   "ld sqrt_c(-0)", 0);
    D_TESTS_EXPECT(isnan(d_math_ld_sqrt_c(-1.0L)), "ld sqrt_c(-1)", 0);
    D_TESTS_EXPECT(d_math_ld_sqrt_c((long double)HUGE_VAL) ==
                   (long double)HUGE_VAL,
                   "ld sqrt_c(inf)", 0);
    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_sqrt_c(LDBL_MIN * LDBL_EPSILON),
                                   g_sqrtl(LDBL_MIN * LDBL_EPSILON)),
                   "ld sqrt_c of the least subnormal", 0);

    return;
}

// the distance of _got from _want in units of _want's last place
static double
d_tests_ulps(long double _got, long double _want, int _mant)
{
    const long double scale = (_want == 0.0L) ? ldexpl(1.0L, -_mant)
                                              : ldexpl(1.0L,
                                                       ilogbl(_want) -
                                                       (_mant - 1));
    const long double diff  = (_got > _want) ? (_got - _want)
                                             : (_want - _got);

    return (double)(diff / scale);
}

static void
d_tests_acos(void)
{
    double worst_f  = 0.0;
    double worst_d  = 0.0;
    double worst_ld = 0.0;
    long   i        = 0;

    for (i = 0; i < 2000000; ++i)
    {
        const double      u  = ((double)(d_tests_random() >> 11) *
                                DBL_EPSILON) - 1.0;
        const float       uf = (float)u;
        const long double ul = (long double)u *
                               (1.0L - ((long double)(d_tests_random() >> 11) *
                                        LDBL_EPSILON / 4096.0L));
        const double ef  = d_tests_ulps(d_math_f_acos_c(uf), acosf(uf), 24);
        const double ed  = d_tests_ulps(d_math_d_acos_c(u), acos(u), 53);
        const double eld = d_tests_ulps(d_math_ld_acos_c(ul), acosl(ul),
                                        LDBL_MANT_DIG);

        worst_f  = (ef  > worst_f)  ? ef  : worst_f;
        worst_d  = (ed  > worst_d)  ? ed  : worst_d;
        worst_ld = (eld > worst_ld) ? eld : worst_ld;

        D_TESTS_EXPECT(ef  <= 2.0, "f acos_c within two ulps",  uf);
        D_TESTS_EXPECT(ed  <= 2.0, "d acos_c within two ulps",  u);
        D_TESTS_EXPECT(eld <= 2.0, "ld acos_c within two ulps", ul);
    }

    // the edges, and outside the domain
    D_TESTS_EXPECT(d_math_d_acos_c(1.0) == 0.0, "d acos_c(1)", 0);
    D_TESTS_EXPECT(d_tests_ulps(d_math_d_acos_c(-1.0), acos(-1.0), 53) <= 1.0,
                   "d acos_c(-1)", 0);
    D_TESTS_EXPECT(d_tests_ulps(d_math_d_acos_c(0.0), acos(0.0), 53) <= 1.0,
                   "d acos_c(0)", 0);
    D_TESTS_EXPECT(isnan(d_math_d_acos_c(1.0000000000000002)),
                   "d acos_c past 1", 0);
    D_TESTS_EXPECT(isnan(d_math_d_acos_c(nan(""))), "d acos_c(NaN)", 0);

    printf("acos_c's largest error: float %.3g ulps, double %.3g, "
           "long double %.3g\n", worst_f, worst_d, worst_ld);

    return;
}

// d_tests_trig
//   sin_c and cos_c against sin and cos: within an ulp for float and double
// and two for long double, below 2^20, over small, moderate and large
// arguments and the multiples of pi/2 the reduction must not lose; the
// largest error is printed.
static void
d_tests_trig(void)
{
    double worst[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    long   i        = 0;

    for (i = 0; i < 3000000; ++i)
    {
        const double   u     = ((double)(d_tests_random() >> 11) *
                                DBL_EPSILON) - 1.0;
        const int      scale = (int)(i % 21);
        const double   x     = ldexp(u, scale - 1);
        const float    xf    = (float)x;
        const long double xl = (long double)x;
        const double   e[6]  = {
            d_tests_ulps(d_math_f_sin_c(xf), g_sinf(xf), 24),
            d_tests_ulps(d_math_f_cos_c(xf), g_cosf(xf), 24),
            d_tests_ulps(d_math_d_sin_c(x), g_sin(x), 53),
            d_tests_ulps(d_math_d_cos_c(x), g_cos(x), 53),
            d_tests_ulps(d_math_ld_sin_c(xl), g_sinl(xl), LDBL_MANT_DIG),
            d_tests_ulps(d_math_ld_cos_c(xl), g_cosl(xl), LDBL_MANT_DIG)
        };
        int k = 0;

        for (k = 0; k < 6; ++k)
        {
            worst[k] = (e[k] > worst[k]) ? e[k] : worst[k];
        }

        D_TESTS_EXPECT( (e[0] <= 1.0) && (e[1] <= 1.0),
                        "f sin_c and cos_c within an ulp", xf);
        D_TESTS_EXPECT( (e[2] <= 1.0) && (e[3] <= 1.0),
                        "d sin_c and cos_c within an ulp", x);
        D_TESTS_EXPECT( (e[4] <= 2.0) && (e[5] <= 2.0),
                        "ld sin_c and cos_c within two ulps", xl);
    }

    // the multiples of pi/2, where a careless reduction loses everything
    for (i = 1; i < 200000; ++i)
    {
        // stored, so x is a double: clang on x87 hands an inlined kernel
        // the product's 64-bit register value otherwise
        volatile double stored = (double)i * 1.5707963267948966;
        const double    x      = stored;

        const double es = d_tests_ulps(d_math_d_sin_c(x), g_sin(x), 53);
        const double ec = d_tests_ulps(d_math_d_cos_c(x), g_cos(x), 53);

        D_TESTS_EXPECT( (es <= 1.0) &&
                        (ec <= 1.0),
                        "d sin_c and cos_c at a multiple of pi/2", x);
    }

    // the specials
    D_TESTS_EXPECT( (d_math_d_sin_c(0.0) == 0.0) &&
                    (signbit(d_math_d_sin_c(-0.0))) &&
                    (d_math_d_cos_c(0.0) == 1.0) &&
                    (isnan(d_math_d_sin_c((double)HUGE_VAL))) &&
                    (isnan(d_math_d_cos_c(nan("")))),
                    "d sin_c and cos_c at the specials", 0);

    printf("sin_c/cos_c largest error: float %.3g/%.3g, double %.3g/%.3g, "
           "long double %.3g/%.3g ulps\n",
           worst[0], worst[1], worst[2], worst[3], worst[4], worst[5]);

    return;
}

int
main(int _argc, char** _argv)
{
    const bool exhaustive = (_argc > 1) &&
                            (strcmp(_argv[1], "exhaustive") == 0);

    d_tests_sqrt_float(exhaustive);
    d_tests_sqrt_double();
    d_tests_sqrt_long_double();
    d_tests_acos();
    d_tests_trig();

    // the plain forms are the C library's
    D_TESTS_EXPECT(d_math_d_sqrt(2.0) == sqrt(2.0), "d sqrt", 0);
    D_TESTS_EXPECT(d_math_f_acos(0.5f) == acosf(0.5f), "f acos", 0);

    printf("t_elementary: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
