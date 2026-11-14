/*******************************************************************************
* djinterp [c/math]                                                    t_power.c
*
*   Conformance harness for c/math/power.h. cbrt_c must be the correctly
* rounded cube root. For float that is checked exactly: |x| must lie strictly
* between the cubes of the midpoints about the result, each cube taken as an
* exact long double pair -- at one float pattern in 509 across the whole
* range, at every float in the first and last 2^-6 of [1, 8), where the
* reduction's interval begins and ends, and at every subnormal with a single
* bit set and one in 61 of the rest; and the result within an ulp of cbrtf.
*
*   For double, three million arguments -- random patterns of either sign and
* values in [1, 8) -- within three ulps of cbrt (glibc's is up to three from
* the correctly rounded root), how many differ at all printed; and exact
* against values from a 400-bit computation: roots of 2, 3 and 10, the range's
* ends, the perfect cubes, and eight arguments whose root lies within 2e-7 ulp
* of a midpoint, each negated too. For long double, a million random patterns
* within three ulps of cbrtl, and the cubes of a thousand integers exact. The
* sign test the last bit is settled by, at cubes of few bits and a last place
* either side of them, where the approximation cannot decide it and the exact
* sum must. NaN, the zeros, the infinities and the subnormals, every family.
*
*   pow_c must be correctly rounded too. For float, at a million pairs over
* six ranges -- x about 1, any x with x^y anywhere in range, integer powers
* of either sign, x near 1 to a large y, subnormal results, subnormal x --
* against the double library's pow rounded to float wherever that decides,
* and within two ulps of powf. For double, two million pairs within two ulps
* of pow, how many differ printed; and exact at 54 values from rational
* arithmetic or a 400-bit computation: midpoints, exact powers, the ends of
* the range, subnormal midpoints, about 1, eight powers within 1e-6 ulp of a
* midpoint; and 25 such for float. Exact powers at random: squares and
* cubes, reciprocals against their exact remainder, 2^n at every n, 3^n, 5^n
* and 10^n as far as the type holds them. For long double, in whatever
* format, a million pairs within two ulps of powl, a midpoint t^n against
* t^(n-1) t rounded once, (1 - e/2)^y and (1 + e)^y a hair past a midpoint,
* the square root, the reciprocal and the least subnormal. Annex F's special
* values, every family.
*
*   hypot_c must be correctly rounded. For float that is checked exactly: the
* midpoints about the result squared against x^2 + y^2, each exact in long
* double -- at a million pairs over six ranges: any two values, the
* subnormals among them; y/x from 2^-30 to 1; the smaller about where it
* stops mattering; both subnormal; x in the top binade; integers, many
* results exact -- and the result within an ulp of hypotf. For double, two
* million pairs within an ulp of hypot, how many differ printed, and pairs
* of integers below 2^26 against sqrt_c of their squares' exact sum; for long
* double, in whatever format, a million within an ulp of hypotl and the
* integers below 2^(P/2 - 1) the same way. Exact, every family, at values
* from rational arithmetic (tools/edge_values9.py): Pythagorean midpoints,
* to even; roots a hair either side of a midpoint; the overflow threshold;
* subnormal results. The sign test the last bit is settled by, about exact
* squares. Annex F's special values, every family.
*
*   With the argument "exhaustive" it checks every float, all 2^32 patterns,
* for cbrt_c, every positive float to the powers 2.5 and -0.3 for pow_c, and
* every positive float with 1 and with 3/4 of itself for hypot_c (its count
* wants a 64-bit long). Compiled as C++, it checks the overloads; from C++14,
* that cbrt_c, pow_c and hypot_c are constant expressions, for every family,
* with the bits they have at run time. It needs no 64-bit integer -- its
* random numbers come from math_tests_random.h, the same in every build --
* so that ISO-strict C++98 on a 32-bit target, where dj_math_umax has 32
* bits, builds it too.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_power.c -o t_power -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_power.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.11.07
*******************************************************************************/
// std
#include <float.h>   // FLT_MAX, DBL_MAX, LDBL_MAX, LDBL_MANT_DIG
#include <math.h>    // cbrtf ... hypotl, nextafter
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/power.h"  // unit under test
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
// tests
#include "./math_tests_random.h"  // d_tests_seed, d_tests_below ...


// the C library's functions, called through volatile pointers, so that no
// compiler substitutes its own evaluation, or folds a call into a constant
static float       (*volatile g_cbrtf)(float)             = cbrtf;
static double      (*volatile g_cbrt)(double)             = cbrt;
static long double (*volatile g_cbrtl)(long double)       = cbrtl;
static float       (*volatile g_powf)(float, float)       = powf;
static double      (*volatile g_pow)(double, double)      = pow;
static long double (*volatile g_powl)(long double,
                                      long double)        = powl;
static float       (*volatile g_hypotf)(float, float)     = hypotf;
static double      (*volatile g_hypot)(double, double)    = hypot;
static long double (*volatile g_hypotl)(long double,
                                        long double)      = hypotl;

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

// a uniform double in [0, 1), from 53 random bits
static double
d_tests_unit(void)
{
    return (double)d_tests_top(53) * DBL_EPSILON / 2.0;
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

// the distance of _got from _want in units of _want's last place, the
// subnormals' spacing below the normal range; an infinity or NaN is 0 from
// its like and 1e30 from anything else
static double
d_tests_ulps(long double _got, long double _want, int _mant, int _emin)
{
    long double scale = 0.0L;
    long double diff  = 0.0L;
    int         e     = 0;

    if (isnan(_got) || isnan(_want) || isinf(_got) || isinf(_want))
    {
        return d_tests_same_ld(_got, _want) ? 0.0 : 1e30;
    }

    e     = (_want == 0.0L) ? _emin : ilogbl(_want);
    e     = (e < _emin) ? _emin : e;
    scale = ldexpl(1.0L, e - (_mant - 1));
    diff  = (_got > _want) ? (_got - _want) : (_want - _got);

    return (double)(diff / scale);
}

#define D_TESTS_ULPS_F(got, want)                                             \
    d_tests_ulps((got), (want), FLT_MANT_DIG, FLT_MIN_EXP - 1)
#define D_TESTS_ULPS_D(got, want)                                             \
    d_tests_ulps((got), (want), DBL_MANT_DIG, DBL_MIN_EXP - 1)
#define D_TESTS_ULPS_LD(got, want)                                            \
    d_tests_ulps((got), (want), LDBL_MANT_DIG, LDBL_MIN_EXP - 1)

// d_tests_cube_sign
//   the sign of _mid^3 - _a, exactly, for a midpoint _mid of 25 bits near
// the cube root of _a, a float: _mid^2 has 50 bits and is exact in long
// double, _mid^3 is exact as Dekker's pair, its high part within a factor of
// 2 of _a, so that their difference is exact too (Sterbenz's lemma), and the
// one rounding left keeps the sign.
static int
d_tests_cube_sign(long double _a, long double _mid)
{
    const struct dj_internal_math_ld_pair cube =
        d_internal_math_ld_product_exact(_mid, _mid * _mid);
    const long double diff = (cube.high - _a) + cube.low;

    return (diff > 0.0L) ? 1 : ((diff < 0.0L) ? -1 : 0);
}

// d_tests_cbrt_correct_f
//   whether _r is the correctly rounded cube root of _x: |_x| strictly
// between the cubes of the midpoints about |_r| -- half the gap above it,
// and half the gap below it, which is half as wide at a power of 2.
static bool
d_tests_cbrt_correct_f(float _x, float _r)
{
    long double a     = 0.0L;
    long double r     = 0.0L;
    long double above = 0.0L;
    long double below = 0.0L;

    if (isnan(_x))
    {
        return isnan(_r);
    }

    if ( (_x == 0.0f) ||
         isinf(_x) )
    {
        return d_tests_same_f(_x, _r);
    }

    if ( ((_r < 0.0f) != (_x < 0.0f)) ||
         (_r == 0.0f) ||
         isinf(_r) ||
         isnan(_r) )
    {
        return false;
    }

    a     = fabsl((long double)_x);
    r     = fabsl((long double)_r);
    above = ((long double)nextafterf((float)r, FLT_MAX) - r) / 2.0L;
    below = (r - (long double)nextafterf((float)r, 0.0f)) / 2.0L;

    return ( (d_tests_cube_sign(a, r + above) > 0) &&
             (d_tests_cube_sign(a, r - below) < 0) );
}

// d_tests_float_one
//   one float: cbrt_c correctly rounded, by the exact check, and within an
// ulp of cbrtf.
static void
d_tests_float_one(float _x)
{
    const float r = d_math_f_cbrt_c(_x);

    D_TESTS_EXPECT(d_tests_cbrt_correct_f(_x, r),
                   "f cbrt_c correctly rounded", _x);
    D_TESTS_EXPECT(D_TESTS_ULPS_F(r, g_cbrtf(_x)) <= 1.0,
                   "f cbrt_c within an ulp of cbrtf", _x);

    return;
}

static void
d_tests_float(bool _exhaustive)
{
    const uint32_t step = _exhaustive ? 1u : 509u;
    uint32_t       bits = 0u;
    float          x    = 0.0f;
    int            k    = 0;

    // one pattern in 509 across all of them, or every one
    for (bits = 0u; ; bits += step)
    {
        float f = 0.0f;

        memcpy(&f, &bits, sizeof(f));
        d_tests_float_one(f);

        // to the last pattern a step reaches below 2^32
        if (bits > (0xFFFFFFFFu - step))
        {
            break;
        }
    }

    // every float in the first and last 2^-6 of [1, 8), either sign: where
    // the reduction's interval begins and ends
    for (x = 1.0f; x < 1.015625f; x = nextafterf(x, 2.0f))
    {
        d_tests_float_one(x);
        d_tests_float_one(-x);
    }

    for (x = 7.984375f; x < 8.0f; x = nextafterf(x, 9.0f))
    {
        d_tests_float_one(x);
        d_tests_float_one(-x);
    }

    // every subnormal with a single bit set, and one in 61 of the others
    for (k = 0; k < 23; ++k)
    {
        const uint32_t b = (uint32_t)1u << k;
        float          f = 0.0f;

        memcpy(&f, &b, sizeof(f));
        d_tests_float_one(f);
    }

    for (bits = 1u; bits < 0x00800000u; bits += 61u)
    {
        const uint32_t b = (uint32_t)bits;
        float          f = 0.0f;

        memcpy(&f, &b, sizeof(f));
        d_tests_float_one(f);
    }

    return;
}

// d_tests_double
//   cbrt_c within three ulps of cbrt over random arguments; prints how many
// differ at all, and the largest difference.
static void
d_tests_double(void)
{
    double worst = 0.0;
    long   diff  = 0;
    long   i     = 0;

    for (i = 0; i < 3000000; ++i)
    {
        // a random pattern of either sign, or a value in [1, 8)
        double x = 0.0;
        double e = 0.0;

        d_tests_pattern_d(&x, 0);

        if ((i % 3) == 2)
        {
            x = 1.0 + (7.0 * d_tests_unit());
        }

        if (isnan(x))
        {
            continue;
        }

        e = D_TESTS_ULPS_D(d_math_d_cbrt_c(x), g_cbrt(x));
        worst = (e > worst) ? e : worst;
        diff += (e > 0.0) ? 1 : 0;
        D_TESTS_EXPECT(e <= 3.0, "d cbrt_c within three ulps of cbrt", x);
    }

    printf("double: cbrt_c differs from cbrt at %ld of 3000000 arguments, by "
           "%.3g ulps at most\n", diff, worst);

    return;
}

// d_tests_double_edges
//   the correctly rounded roots, from a 400-bit computation (the values are
// tools/edge_values7.py's), each negated too; and the perfect cubes.
static void
d_tests_double_edges(void)
{
    // x and cbrt(x), correctly rounded
    static const double roots[][2] = {
        { 2.0,                     1.2599210498948732      },
        { 3.0,                     1.4422495703074083      },
        { 10.0,                    2.1544346900318838      },
        { 0.10000000000000001,     0.46415888336127792     },
        { 5.0,                     1.7099759466766971      },
        // the largest double, the least normal, the least subnormal
        { 1.7976931348623157e308,  5.6438030941223623e102  },
        { 2.2250738585072014e-308, 2.812644285236262e-103  },
        { 4.9406564584124654e-324, 1.7031839360032603e-108 },
        // about 1, and just below 8
        { 1.0000000000000002,      1.0                     },
        { 0.99999999999999989,     1.0                     },
        { 7.9999999999999991,      2.0                     },
        { 1e-300,                  1e-100                  },
        // the root lies within 2e-7 ulp of a double's midpoint
        { 2.3790087208658548,      1.3349510604765931      },
        { 2.1357600081421264,      1.287807104675273       },
        { 6.6935125715679256,      1.8845949706664253      },
        { 3.9086306433577986,      1.5752211858147342      },
        { 6.6527848156215779,      1.8807648177981431      },
        { 4.2646177547026731,      1.6216608677412441      },
        { 7.1996085267046466,      1.9309437719740978      },
        { 1.970305207245945,       1.2536544166587518      }
    };
    size_t i = 0;
    long   n = 0;

    for (i = 0; i < (sizeof(roots) / sizeof(roots[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_cbrt_c(roots[i][0]),
                                      roots[i][1]),
                       "d cbrt_c correctly rounded", roots[i][0]);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_cbrt_c(-roots[i][0]),
                                      -roots[i][1]),
                       "d cbrt_c correctly rounded, negated", roots[i][0]);
    }

    // n^3 for every n below 2^17 + 2 whose cube a double holds, a stride
    // apart, and the powers of 2: the roots exact
    for (n = 1; n <= 131073; n += 97)
    {
        const double c = (double)n * (double)n * (double)n;

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_cbrt_c(c), (double)n),
                       "d cbrt_c of a perfect cube", c);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_cbrt_c(-c), -(double)n),
                       "d cbrt_c of a perfect cube, negated", c);
    }

    D_TESTS_EXPECT(d_tests_same_d(d_math_d_cbrt_c(2251851353686017.0),
                                  131073.0),
                   "d cbrt_c(131073^3)", 0);

    for (n = -1074; n <= 1023; ++n)
    {
        const double p = ldexp(1.0, (int)n);
        const double r = d_math_d_cbrt_c(p);

        // 2^n's root is 2^(n/3) exactly where 3 divides n
        if ((((n % 3) + 3) % 3) == 0)
        {
            D_TESTS_EXPECT(d_tests_same_d(r, ldexp(1.0, (int)(n / 3))),
                           "d cbrt_c of a power of 8", p);
        }
        else
        {
            D_TESTS_EXPECT(D_TESTS_ULPS_D(r, g_cbrt(p)) <= 3.0,
                           "d cbrt_c of a power of 2", p);
        }
    }

    return;
}

static void
d_tests_long_double(void)
{
    double worst = 0.0;
    long   i     = 0;
    // the binades p is drawn from, the subnormals' included
    const unsigned long binades =
        (unsigned long)(LDBL_MAX_EXP - LDBL_MIN_EXP + LDBL_MANT_DIG);

    for (i = 0; i < 1000000; ++i)
    {
        // a mantissa with every bit random, in every binade, the
        // subnormals' included, either sign
        const long double m = 0.5L + ((long double)d_tests_unit() / 2.0L) +
                              ((long double)d_tests_unit() * 1e-17L);
        const int         p = (int)d_tests_below(binades) +
                              (LDBL_MIN_EXP - LDBL_MANT_DIG + 1);
        const long double x = ((i % 2) == 0) ? ldexpl(m, p) : -ldexpl(m, p);
        const double      e = D_TESTS_ULPS_LD(d_math_ld_cbrt_c(x),
                                              g_cbrtl(x));

        worst = (e > worst) ? e : worst;
        D_TESTS_EXPECT(e <= 3.0, "ld cbrt_c within three ulps of cbrtl", x);
    }

    // the cubes of the integers to a thousand: exact in any long double
    for (i = 1; i <= 1000; ++i)
    {
        const long double n = (long double)i;

        D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_cbrt_c(n * n * n), n),
                       "ld cbrt_c of a perfect cube", n);
        D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_cbrt_c(-(n * n * n)), -n),
                       "ld cbrt_c of a perfect cube, negated", n);
    }

    // the edges: the largest long double and the least subnormal
    D_TESTS_EXPECT(D_TESTS_ULPS_LD(d_math_ld_cbrt_c(LDBL_MAX),
                                   g_cbrtl(LDBL_MAX)) <= 3.0,
                   "ld cbrt_c(LDBL_MAX)", 0);
    D_TESTS_EXPECT(D_TESTS_ULPS_LD(d_math_ld_cbrt_c(LDBL_MIN * LDBL_EPSILON),
                                   g_cbrtl(LDBL_MIN * LDBL_EPSILON)) <= 3.0,
                   "ld cbrt_c of the least subnormal", 0);

    printf("long double: cbrt_c within %.3g ulps of cbrtl\n", worst);

    return;
}

// d_tests_cube_sign_exact
//   the sign test cbrt_c settles its last bit by: at t + h of few bits, so
// that (t + h)^3 is exact in any long double, against that cube and the
// long doubles either side of it -- 0, -1 and 1 -- where the approximation
// lies within its error bound of 0 and the exact sum decides.
static void
d_tests_cube_sign_exact(void)
{
    int j = 0;

    for (j = 1; j < 400; j += 2)
    {
        // t = 1 + j 2^-10 and h = +-2^-11: t + h has at most 12 bits, its
        // cube at most 36
        const long double t     = 1.0L + ((long double)j / 1024.0L);
        const long double h     = ((j % 4) == 1) ? (1.0L / 2048.0L)
                                                 : (-1.0L / 2048.0L);
        const long double mid   = t + h;
        const long double cube  = (mid * mid) * mid;
        const long double above = d_math_ld_next_up(cube);
        const long double below = d_math_ld_next_down(cube);

        D_TESTS_EXPECT( (d_internal_math_ld_cube_sign(cube, t, h) == 0) &&
                        (d_internal_math_ld_cube_sign(above, t, h) < 0) &&
                        (d_internal_math_ld_cube_sign(below, t, h) > 0) &&
                        (d_internal_math_ld_cube_sign_exact(cube, t, h) ==
                         0) &&
                        (d_internal_math_ld_cube_sign_exact(above, t, h) <
                         0) &&
                        (d_internal_math_ld_cube_sign_exact(below, t, h) >
                         0),
                        "the cube's sign, exactly, about a cube", t);
    }

    return;
}

// d_tests_specials
//   NaN, the zeros, the infinities, the subnormals, every family.
static void
d_tests_specials(void)
{
    const double    inf   = (double)HUGE_VAL;
    volatile double ten   = 10.0;
    volatile float  three = 3.0f;

    D_TESTS_EXPECT( (isnan(d_math_f_cbrt_c(nanf("")))) &&
                    (isnan(d_math_d_cbrt_c(nan("")))) &&
                    (isnan(d_math_ld_cbrt_c(nanl("")))),
                    "cbrt_c(NaN) is NaN", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_f_cbrt_c(0.0f), 0.0f)) &&
                    (d_tests_same_f(d_math_f_cbrt_c(-0.0f), -0.0f)) &&
                    (d_tests_same_d(d_math_d_cbrt_c(0.0), 0.0)) &&
                    (d_tests_same_d(d_math_d_cbrt_c(-0.0), -0.0)) &&
                    (d_tests_same_ld(d_math_ld_cbrt_c(0.0L), 0.0L)) &&
                    (d_tests_same_ld(d_math_ld_cbrt_c(-0.0L), -0.0L)),
                    "cbrt_c of the zeros keeps their sign", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_f_cbrt_c((float)inf),
                                    (float)inf)) &&
                    (d_tests_same_f(d_math_f_cbrt_c(-(float)inf),
                                    -(float)inf)) &&
                    (d_tests_same_d(d_math_d_cbrt_c(inf), inf)) &&
                    (d_tests_same_d(d_math_d_cbrt_c(-inf), -inf)) &&
                    (d_tests_same_ld(d_math_ld_cbrt_c((long double)inf),
                                     (long double)inf)) &&
                    (d_tests_same_ld(d_math_ld_cbrt_c(-(long double)inf),
                                     -(long double)inf)),
                    "cbrt_c of the infinities", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_f_cbrt_c(-27.0f), -3.0f)) &&
                    (d_tests_same_f(d_math_f_cbrt_c(0.125f), 0.5f)) &&
                    (d_tests_same_d(d_math_d_cbrt_c(-8.0), -2.0)) &&
                    (d_tests_same_ld(d_math_ld_cbrt_c(64.0L), 4.0L)),
                    "cbrt_c of small cubes", 0);
    // the least subnormals: 2^-149's root is 2^(-149/3), not a power of 2
    D_TESTS_EXPECT( (d_tests_cbrt_correct_f(FLT_MIN * FLT_EPSILON,
                                            d_math_f_cbrt_c(FLT_MIN *
                                                            FLT_EPSILON))) &&
                    (d_tests_same_d(d_math_d_cbrt_c(4.9406564584124654e-324),
                                    1.7031839360032603e-108)),
                    "cbrt_c of the least subnormal", 0);

    // the plain form is the C library's -- read through volatiles, so that
    // no compiler folds the call into its own correctly rounded constant --
    // but in C++ before C++11, which has no cbrt, where it is cbrt_c
#if ( defined(__cplusplus) &&                                                 \
      !D_ENV_LANG_IS_CPP11_OR_HIGHER )
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_cbrt(ten),
                                    d_math_d_cbrt_c(ten))) &&
                    (d_tests_same_f(d_math_f_cbrt(three),
                                    d_math_f_cbrt_c(three))),
                    "cbrt is cbrt_c before C++11", 0);
#else
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_cbrt(ten), g_cbrt(ten))) &&
                    (d_tests_same_f(d_math_f_cbrt(three), g_cbrtf(three))),
                    "cbrt is the C library's", 0);
#endif

    return;
}

// d_tests_pow_reference_f
//   x^y correctly rounded to float, from the C library's double pow -- within
// a little over half a double's last place of x^y -- where that lies past
// 2^-44 of x^y from a float's midpoint, as it does at all but about one
// argument in a hundred million: then true, the float in *_want.
static bool
d_tests_pow_reference_f(
    float  _x,
    float  _y,
    float* _want
)
{
    // stored, so that the x87 hands back a double, not its register
    volatile double stored = g_pow((double)_x, (double)_y);
    const double    v      = stored;
    const double    a      = fabs(v);
    // the midpoint between FLT_MAX and 2^128, and the margin, 2^-44
    const double    top    = (double)FLT_MAX +
                             ldexp(1.0, FLT_MAX_EXP - FLT_MANT_DIG - 1);
    const double    margin = ldexp(1.0, -44);
    float           f      = 0.0f;
    double          mid    = 0.0;

    if (isnan(v))
    {
        *_want = (float)v;

        return true;
    }

    if (a >= top)
    {
        *_want = (v > 0.0) ? (float)HUGE_VAL : -(float)HUGE_VAL;

        return ( isinf(v) ||
                 ((a - top) > (a * margin)) );
    }

    if (a > (double)FLT_MAX)
    {
        *_want = (v > 0.0) ? FLT_MAX : -FLT_MAX;

        return ((top - a) > (a * margin));
    }

    f = (float)v;

    if ((double)f == v)
    {
        *_want = f;

        return true;
    }

    // the midpoint between f and its neighbour toward v: exact in double
    mid = ((double)f +
           (double)nextafterf(f, (v > (double)f) ? FLT_MAX : -FLT_MAX)) / 2.0;
    *_want = f;

    return (fabs(v - mid) > (a * margin));
}

// d_tests_pow_args
//   an argument pair for range _range of a family whose exponents run from
// _emin to _emax with _mant bits: 0, x in [1/2, 2) and |y| up to 60; 1, any
// normal x and y putting x^y anywhere in range; 2, an integer y from 2 to
// 40 and x of either sign; 3, x within 2^-40 of 1 and y large; 4, x^y
// subnormal; 5, x subnormal and y in (0, 1].
static void
d_tests_pow_args(
    int          _range,
    int          _emin,
    int          _emax,
    int          _mant,
    long double* _x,
    long double* _y
)
{
    const long double sign  = (d_tests_below(2u) == 0u) ? 1.0L : -1.0L;
    const long double u     = (long double)d_tests_unit();
    const long double whole = (long double)(_emax - _emin + _mant);
    long double       lx    = 0.0L;

    switch (_range)
    {
        case 0:
            *_x = 0.5L + (1.5L * u);
            *_y = sign * 60.0L * (long double)d_tests_unit();
            break;

        case 1:
            *_x = ldexpl(1.0L + u,
                         (int)d_tests_below((unsigned long)(_emax - _emin)) +
                         _emin);
            lx  = log2l(*_x);
            lx  = (lx == 0.0L) ? 1.0L : lx;
            *_y = ((long double)(_emin - _mant) +
                   (whole * (long double)d_tests_unit())) / lx;
            break;

        case 2:
            *_y = (long double)(2u + (unsigned)d_tests_below(39u));
            *_x = sign * ldexpl(1.0L + u,
                                (int)(((long double)_emin +
                                       ((long double)(_emax - _emin) *
                                        (long double)d_tests_unit())) /
                                      *_y));
            break;

        case 3:
            *_x = 1.0L + (sign * ldexpl(1.0L + u,
                                        -1 - (int)d_tests_below(40u)));
            lx  = log2l(*_x);
            *_y = ((long double)(_emin - _mant) +
                   (whole * (long double)d_tests_unit())) / lx;
            break;

        case 4:
            *_x = ldexpl(1.0L + u, (int)d_tests_below(200u) - 100);
            *_x = (*_x == 1.0L) ? 3.0L : *_x;
            lx  = log2l(*_x);
            *_y = ((long double)(_emin - _mant - 1) +
                   ((long double)(_mant + 1) * (long double)d_tests_unit())) /
                  lx;
            break;

        default:
            *_x = ldexpl(1.0L + u,
                         _emin - 2 -
                         (int)d_tests_below((unsigned long)(_mant - 2)));
            *_y = (long double)d_tests_unit();
            break;
    }

    return;
}

// d_tests_pow_float
//   pow_c correctly rounded, against the double library's reference wherever
// it decides (d_tests_pow_reference_f), and within two ulps of powf: a
// million argument pairs, in six ranges; with _exhaustive, besides, every
// positive float to the powers 2.5 and -0.3.
static void
d_tests_pow_float(
    bool _exhaustive
)
{
    long     undecided = 0;
    long     i         = 0;
    uint32_t bits      = 0u;

    for (i = 0; i < 1000000; ++i)
    {
        long double lx   = 0.0L;
        long double ly   = 0.0L;
        float       x    = 0.0f;
        float       y    = 0.0f;
        float       got  = 0.0f;
        float       want = 0.0f;

        d_tests_pow_args((int)(i % 6),
                         FLT_MIN_EXP - 1,
                         FLT_MAX_EXP,
                         FLT_MANT_DIG,
                         &lx,
                         &ly);
        x   = (float)lx;
        y   = (float)ly;
        got = d_math_f_pow_c(x, y);

        if (d_tests_pow_reference_f(x, y, &want))
        {
            D_TESTS_EXPECT(d_tests_same_f(got, want),
                           "f pow_c correctly rounded", x);
        }
        else
        {
            ++undecided;
        }

        D_TESTS_EXPECT(D_TESTS_ULPS_F(got, g_powf(x, y)) <= 2.0,
                       "f pow_c within two ulps of powf", x);
    }

    // every positive float, finite, to two powers
    for (bits = (_exhaustive ? 1u : 0x80000000u); bits < 0x7F800000u; ++bits)
    {
        const uint32_t b = (uint32_t)bits;
        float          x = 0.0f;
        float          want = 0.0f;

        memcpy(&x, &b, sizeof(x));

        if (d_tests_pow_reference_f(x, 2.5f, &want))
        {
            D_TESTS_EXPECT(d_tests_same_f(d_math_f_pow_c(x, 2.5f), want),
                           "f pow_c(x, 2.5) correctly rounded", x);
        }
        else
        {
            ++undecided;
        }

        if (d_tests_pow_reference_f(x, -0.3f, &want))
        {
            D_TESTS_EXPECT(d_tests_same_f(d_math_f_pow_c(x, -0.3f), want),
                           "f pow_c(x, -0.3) correctly rounded", x);
        }
        else
        {
            ++undecided;
        }
    }

    printf("float: pow_c, %ld pairs the double library's pow could not "
           "decide\n", undecided);

    return;
}

// d_tests_pow_double
//   pow_c within two ulps of pow -- glibc's is within a little over half an
// ulp -- at two million argument pairs in six ranges; how many differ at all
// printed.
static void
d_tests_pow_double(void)
{
    double worst = 0.0;
    long   diff  = 0;
    long   i     = 0;

    for (i = 0; i < 2000000; ++i)
    {
        long double     lx  = 0.0L;
        long double     ly  = 0.0L;
        double          x   = 0.0;
        double          y   = 0.0;
        volatile double lib = 0.0;
        double          e   = 0.0;

        d_tests_pow_args((int)(i % 6),
                         DBL_MIN_EXP - 1,
                         DBL_MAX_EXP,
                         DBL_MANT_DIG,
                         &lx,
                         &ly);
        x   = (double)lx;
        y   = (double)ly;
        lib = g_pow(x, y);
        e   = D_TESTS_ULPS_D(d_math_d_pow_c(x, y), lib);
        worst = (e > worst) ? e : worst;
        diff += (e > 0.0) ? 1 : 0;
        D_TESTS_EXPECT(e <= 2.0, "d pow_c within two ulps of pow", x);
    }

    printf("double: pow_c differs from pow at %ld of 2000000 pairs, by %.3g "
           "ulps at most\n", diff, worst);

    return;
}

// d_tests_pow_edges
//   the correctly rounded powers, from rational arithmetic or a 400-bit
// computation (tools/edge_values8.py): the midpoints, the exact powers, the
// powers of 2 at both ends of the range, the subnormal midpoints, about 1,
// and arguments whose power lies within a millionth of an ulp of a midpoint.
static void
d_tests_pow_edges(void)
{
    // x, y and x^y, correctly rounded
    static const double powers_d[][3] = {
        // 5^23, a midpoint: to even
        { 25.0, 11.5, 11920928955078124.0 },
        // the same
        { 5.0, 23.0, 11920928955078124.0 },
        // a midpoint
        { 3.0, 34.0, 16677181699666568.0 },
        // a midpoint, negative
        { -7.0, 19.0, -11398895185373144.0 },
        // (3 2^25 + 1)^2, a midpoint
        { 100663297.0, 2.0, 10133099362910208.0 },
        // exact, 53 bits
        { 3.0, 33.0, 5559060566555523.0 },
        // exact
        { 10.0, 22.0, 1e+22 },
        // a midpoint: 1e23's double
        { 10.0, 23.0, 9.9999999999999992e+22 },
        // a root's power, exact
        { 9.0, 1.5, 27.0 },
        // 11^7, exact
        { 121.0, 3.5, 19487171.0 },
        // 27, below 1 by the logarithm
        { 81.0, 0.75, 27.0 },
        // a power of 2
        { 4.0, 2.5, 32.0 },
        // the least subnormal
        { 2.0, -1074.0, 4.9406564584124654e-324 },
        // half of it: to even, 0
        { 2.0, -1075.0, 0.0 },
        // the same
        { 4.0, -537.5, 0.0 },
        // past half: the least subnormal
        { 2.0, -1074.5, 4.9406564584124654e-324 },
        // the least subnormal
        { 0.5, 1074.0, 4.9406564584124654e-324 },
        // the largest power of 2
        { 2.0, 1023.0, 8.9884656743115795e+307 },
        // past the largest finite
        { 2.0, 1024.0, (double)HUGE_VAL },
        // the same
        { 0.5, -1024.0, (double)HUGE_VAL },
        // 243 2^-1075, a subnormal midpoint
        { 5.6973406474558792e-65, 5.0, 6.0276008792632078e-322 },
        // the largest
        { 1.7976931348623157e+308, 1.0, 1.7976931348623157e+308 },
        { 1.7976931348623157e+308, 0.5, 1.3407807929942596e+154 },
        // 2^512 squared: 2^1024, past the largest
        { 1.3407807929942597e+154, 2.0, (double)HUGE_VAL },
        // a last place below: finite
        { 1.3407807929942596e+154, 2.0, 1.7976931348623155e+308 },
        // 0.1, the quotient
        { 10.0, -1.0, 0.10000000000000001 },
        // 1/3
        { 3.0, -1.0, 0.33333333333333331 },
        // a midpoint and 3/8 ulp^2 past it
        { 0.99999999999999989, 1.5, 0.99999999999999989 },
        { 0.99999999999999989, -3.0, 1.0000000000000004 },
        { 0.99999999999999989, -1.0, 1.0000000000000002 },
        { 0.99999999999999989, 0.5, 0.99999999999999989 },
        { 1.0000000000000002, 1.5, 1.0000000000000004 },
        { 1.0000000000000002, 2.5, 1.0000000000000007 },
        // the superellipse's exponent
        { 1.6074999999999999, 2.5, 3.2762531686877052 },
        // and its reciprocal
        { 2.5, 0.62208398133748055, 1.7682843567468598 },
        { 10.0, 0.29999999999999999, 1.9952623149688795 },
        { 2.0, 0.33333333333333331, 1.2599210498948732 },
        { 0.10000000000000001, 3.0, 0.0010000000000000002 },
        { 10000000000.0, -2.5, 1e-25 },
        { 1.0000001000000001, 10000000.0, 2.7182816941320818 },
        { 0.99999990000000005, -100000000.0, 22026.47669210544 },
        { 7.0, -0.5, 0.37796447300922725 },
        // the square root
        { 2.0, 0.5, 1.4142135623730951 },
        { 1e-300, 0.25, 9.9999999999999996e-76 },
        // the least subnormal's root
        { 4.9406564584124654e-324, 0.5, 2.2227587494850775e-162 },
        { 4.9406564584124654e-324, -0.25, 6.7073942738914618e+80 },
        // within 1e-6 ulp of a midpoint
        { 3.2872603504190834, 14.742884433600651, 41650999.685074024 },
        { 3.4335804236560614, -11.711821334570608, 5.3140327889760181e-07 },
        { 2.547087121883111, -7.794650306609241, 0.0006839597886535055 },
        { 3.5306592152953917, -6.8445927026692015, 0.0001778894464384582 },
        { 3.4020016261311423, 27.762209270780446, 578262372537558.75 },
        { 0.88520038205915319, 307.33829650045618, 5.2948457784630077e-17 },
        { 3.5308708669032796, -22.063598201653889, 8.1612247859371078e-13 },
        { 3.6830642603995267, 2.7261032827596994, 34.957874696834217 },
    };
    static const float powers_f[][3] = {
        // a midpoint: to even
        { 4097.0f, 2.0f, 16785408.0f },
        // a midpoint
        { 11.0f, 7.0f, 19487172.0f },
        // a midpoint
        { 17.0f, 6.0f, 24137568.0f },
        // a midpoint, negative
        { -11.0f, 7.0f, -19487172.0f },
        // 11^7, by the root
        { 121.0f, 3.5f, 19487172.0f },
        // exact
        { 3.0f, 15.0f, 14348907.0f },
        // exact
        { 10.0f, 10.0f, 1e+10f },
        { 10.0f, 11.0f, 9.9999998e+10f },
        // the least subnormal
        { 2.0f, -149.0f, 1.40129846e-45f },
        // half of it: 0
        { 2.0f, -150.0f, 0.0f },
        // the least subnormal
        { 2.0f, -149.5f, 1.40129846e-45f },
        { 2.0f, 127.0f, 1.70141183e+38f },
        // past the largest finite
        { 2.0f, 128.0f, (float)HUGE_VAL },
        // 243 2^-150, a subnormal midpoint
        { 2.79396772e-09f, 5.0f, 1.70958413e-43f },
        // a midpoint and past it
        { 0.99999994f, 1.5f, 0.99999994f },
        { 0.99999994f, -3.0f, 1.00000024f },
        { 1.00000012f, 2.5f, 1.00000036f },
        { 1.60749996f, 2.5f, 3.27625299f },
        { 10.0f, 0.300000012f, 1.99526238f },
        { 2.0f, 0.333333343f, 1.25992107f },
        // within 3e-7 ulp of a midpoint
        { 3.29267168f, 4.16435766f, 142.973907f },
        { 3.07511044f, -1.68910742f, 0.149951503f },
        { 2.90792942f, 4.43097305f, 113.273315f },
        { 2.82459402f, 4.89495707f, 161.216858f },
        { 1.21035171f, -24.1453762f, 0.00995562598f },
    };
    size_t i = 0;

    for (i = 0; i < (sizeof(powers_d) / sizeof(powers_d[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_pow_c(powers_d[i][0],
                                                     powers_d[i][1]),
                                      powers_d[i][2]),
                       "d pow_c correctly rounded", powers_d[i][0]);
    }

    for (i = 0; i < (sizeof(powers_f) / sizeof(powers_f[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_f(d_math_f_pow_c(powers_f[i][0],
                                                     powers_f[i][1]),
                                      powers_f[i][2]),
                       "f pow_c correctly rounded", powers_f[i][0]);
    }

    return;
}

// d_tests_reciprocal_d
//   whether _q is 1/_r correctly rounded, _r normal: 1/r - q = (1 - q r)/r
// within half the gap about q, q r exact as Dekker's pair -- 1 - q r is a
// double's remainder, exact where q is the rounded quotient -- and the
// comparison in long double, which cannot misjudge: 1/r never lies on a
// midpoint, nor nearer one than 2^-54 of its last place.
static bool
d_tests_reciprocal_d(
    double _r,
    double _q
)
{
    const struct dj_internal_math_ld_pair p =
        d_internal_math_ld_product_exact((long double)_q, (long double)_r);
    const long double rest  = (1.0L - p.high) - p.low;
    const long double above = ( (long double)nextafter(_q, DBL_MAX) -
                                (long double)_q ) / 2.0L;
    const long double below = ( (long double)_q -
                                (long double)nextafter(_q, 0.0) ) / 2.0L;

    if (_q == 0.0)
    {
        return false;
    }

    return ( (rest < (above * (long double)_r)) &&
             (rest > -(below * (long double)_r)) );
}

// d_tests_pow_exact
//   exact powers, every family, at many more arguments than a table holds:
// x^2, x^3 and x^-1 where the result is exact or on a midpoint, against the
// product or quotient rounded once by the hardware; 2^n at every n; and the
// powers of 3, 5 and 10 the type holds.
static void
d_tests_pow_exact(void)
{
    long i = 0;
    int  n = 0;

    for (i = 0; i < 200000; ++i)
    {
        // x of at most 13 bits: x^2 and x^3 exact in a double; and a float
        // of 13 bits, whose square has 25 or 26 -- a midpoint, or rounded
        // -- against the exact square rounded once
        const double   x  = ldexp((double)(1u + (unsigned)d_tests_below(8191u)),
                                  (int)d_tests_below(200u) - 100);
        const uint32_t m  = 4097u + (uint32_t)d_tests_below(4095u);
        const float    xf = (float)m;
        volatile float square = (float)((double)m * (double)m);
        // any double, for its reciprocal
        double         r  = 0.0;

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_pow_c(x, 2.0), x * x),
                       "d pow_c(x, 2) exact", x);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_pow_c(-x, 3.0), -(x * x * x)),
                       "d pow_c(-x, 3) exact", x);
        D_TESTS_EXPECT(d_tests_same_f(d_math_f_pow_c(xf, 2.0f), square),
                       "f pow_c(x, 2) correctly rounded", xf);

        d_tests_pattern_d(&r, 0);
        r = fabs(r);

        // r from DBL_MIN to 2^1022: 1/r normal, so that Dekker's product
        // is exact in any long double
        if ( !isnan(r) &&
             (r >= DBL_MIN) &&
             (r <= (1.0 / DBL_MIN)) )
        {
            D_TESTS_EXPECT(d_tests_reciprocal_d(r, d_math_d_pow_c(r, -1.0)),
                           "d pow_c(x, -1) the quotient, rounded once", r);
        }
    }

    // 2^n, every n a double reaches, and past both ends
    for (n = -1080; n <= 1030; ++n)
    {
        const double want = (n >= 1024) ? (double)HUGE_VAL
                          : ((n < -1074) ? 0.0 : ldexp(1.0, n));

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_pow_c(2.0, (double)n), want),
                       "d pow_c(2, n)", n);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_pow_c(0.5, (double)-n), want),
                       "d pow_c(1/2, -n)", n);
    }

    for (n = -155; n <= 130; ++n)
    {
        const float want = (n >= 128) ? (float)HUGE_VAL
                         : ((n < -149) ? 0.0f : (float)ldexp(1.0, n));

        D_TESTS_EXPECT(d_tests_same_f(d_math_f_pow_c(2.0f, (float)n), want),
                       "f pow_c(2, n)", n);
    }

    // 3^n to 3^33 and 10^n to 10^22, exact in a double and in any long
    // double; 5^n to 5^10 and 10^n to 10^10 in a float
    {
        double      p3  = 1.0;
        double      p10 = 1.0;
        long double l3  = 1.0L;
        long double l10 = 1.0L;
        float       f5  = 1.0f;
        float       f10 = 1.0f;

        for (n = 1; n <= 33; ++n)
        {
            p3 *= 3.0;
            l3 *= 3.0L;
            D_TESTS_EXPECT(d_tests_same_d(d_math_d_pow_c(3.0, (double)n), p3),
                           "d pow_c(3, n) exact", n);
            D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_pow_c(3.0L,
                                                           (long double)n),
                                           l3),
                           "ld pow_c(3, n) exact", n);
        }

        for (n = 1; n <= 22; ++n)
        {
            p10 *= 10.0;
            l10 *= 10.0L;
            D_TESTS_EXPECT(d_tests_same_d(d_math_d_pow_c(10.0, (double)n),
                                          p10),
                           "d pow_c(10, n) exact", n);
            D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_pow_c(10.0L,
                                                           (long double)n),
                                           l10),
                           "ld pow_c(10, n) exact", n);
        }

        for (n = 1; n <= 10; ++n)
        {
            f5  *= 5.0f;
            f10 *= 10.0f;
            D_TESTS_EXPECT(d_tests_same_f(d_math_f_pow_c(5.0f, (float)n), f5),
                           "f pow_c(5, n) exact", n);
            D_TESTS_EXPECT(d_tests_same_f(d_math_f_pow_c(10.0f, (float)n),
                                          f10),
                           "f pow_c(10, n) exact", n);
        }
    }

    return;
}

// d_tests_pow_long_double
//   pow_c within two ulps of powl at a million argument pairs in six
// ranges; and, in whatever format long double has, the results that lie on
// a midpoint or a hair past one: t^n with P + 1 bits, from t^(n-1) t
// rounded once by the multiplication; (1 - e/2)^y and (1 + e)^y, e long
// double's epsilon, whose second-order term decides; and the square root,
// the reciprocal, the least subnormal and half of it.
static void
d_tests_pow_long_double(void)
{
    const long double e     = LDBL_EPSILON;
    const long double below = 1.0L - (e / 2.0L);
    const long double above = 1.0L + e;
    double            worst = 0.0;
    long              i     = 0;

    for (i = 0; i < 1000000; ++i)
    {
        long double x  = 0.0L;
        long double y  = 0.0L;
        double      er = 0.0;

        d_tests_pow_args((int)(i % 6),
                         LDBL_MIN_EXP - 1,
                         LDBL_MAX_EXP,
                         LDBL_MANT_DIG,
                         &x,
                         &y);
        er    = D_TESTS_ULPS_LD(d_math_ld_pow_c(x, y), g_powl(x, y));
        worst = (er > worst) ? er : worst;
        D_TESTS_EXPECT(er <= 2.0, "ld pow_c within two ulps of powl", x);
    }

    printf("long double: pow_c within %.3g ulps of powl\n", worst);

    // a midpoint: 3^34, 3^41 or 5^49 in a double's, an x87's or a quad's
    // precision, against 3^33 3, 3^40 3 or 5^48 5 rounded once
    {
        const long double t = (LDBL_MANT_DIG == 113) ? 5.0L : 3.0L;
        const int         n = (LDBL_MANT_DIG == 53)  ? 34
                            : ((LDBL_MANT_DIG == 64) ? 41
                            : ((LDBL_MANT_DIG == 113) ? 49 : 0));
        volatile long double power = 1.0L;
        int                  j     = 0;

        if (n > 0)
        {
            for (j = 1; j < n; ++j)
            {
                power = power * t;
            }

            power = power * t;
            D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_pow_c(t, (long double)n),
                                           power),
                           "ld pow_c on a midpoint, to even", n);
            D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_pow_c(-t,
                                                           (long double)n),
                                           ((n % 2) == 1) ? -power : power),
                           "ld pow_c on a midpoint, negative", n);
        }
    }

    // a hair past a midpoint: (1 - e/2)^1.5 = 1 - 1.5 (e/2) + (3/8)(e/2)^2,
    // past the midpoint between 1 - e/2 and 1 - e, so 1 - e/2; and so on
    D_TESTS_EXPECT( (d_tests_same_ld(d_math_ld_pow_c(below, 1.5L), below)) &&
                    (d_tests_same_ld(d_math_ld_pow_c(below, -3.0L),
                                     1.0L + (2.0L * e))) &&
                    (d_tests_same_ld(d_math_ld_pow_c(below, -1.0L), above)) &&
                    (d_tests_same_ld(d_math_ld_pow_c(below, 0.5L), below)) &&
                    (d_tests_same_ld(d_math_ld_pow_c(above, 1.5L),
                                     1.0L + (2.0L * e))) &&
                    (d_tests_same_ld(d_math_ld_pow_c(above, 2.5L),
                                     1.0L + (3.0L * e))),
                    "ld pow_c a hair past a midpoint", 0);

    // the square root and the reciprocal, each rounded once
    for (i = 1; i < 2000; ++i)
    {
        const long double x = (long double)i * 1.0009765625L;

        D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_pow_c(x, 0.5L),
                                       d_math_ld_sqrt_c(x)),
                       "ld pow_c(x, 1/2) is sqrt_c", x);
        D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_pow_c(x, -1.0L), 1.0L / x),
                       "ld pow_c(x, -1) the quotient", x);
    }

    // the least subnormal, as a power of 2 and as a power of 1/2; half of
    // it, a midpoint, to the even 0
    D_TESTS_EXPECT( (d_tests_same_ld(
                         d_math_ld_pow_c(2.0L, (long double)(LDBL_MIN_EXP -
                                                             LDBL_MANT_DIG)),
                         LDBL_MIN * LDBL_EPSILON)) &&
                    (d_tests_same_ld(
                         d_math_ld_pow_c(0.5L, (long double)(LDBL_MANT_DIG -
                                                             LDBL_MIN_EXP)),
                         LDBL_MIN * LDBL_EPSILON)) &&
                    (d_tests_same_ld(
                         d_math_ld_pow_c(2.0L, (long double)(LDBL_MIN_EXP -
                                                             LDBL_MANT_DIG -
                                                             1)),
                         0.0L)),
                    "ld pow_c at the least subnormal", 0);

    return;
}

// D_TESTS_POW_SPECIALS
//   Annex F's special values for one family: x^+-0 and 1^y, NaN in either,
// the zeros, the infinities, -1, a negative x to an integer and to a
// fraction, |x| about 1 to +-inf.
#define D_TESTS_POW_SPECIALS(T, S, SAME, MAXV)                                \
    {                                                                         \
        const T inf  = (T)HUGE_VAL;                                           \
        const T qnan = (T)nan("");                                            \
                                                                              \
        D_TESTS_EXPECT( SAME(d_math_##S##_pow_c(qnan, (T)0), (T)1) &&         \
                        SAME(d_math_##S##_pow_c(qnan, -(T)0), (T)1) &&        \
                        SAME(d_math_##S##_pow_c((T)1, qnan), (T)1) &&         \
                        SAME(d_math_##S##_pow_c((T)1, inf), (T)1) &&          \
                        SAME(d_math_##S##_pow_c((T)1, -inf), (T)1) &&         \
                        SAME(d_math_##S##_pow_c(inf, (T)0), (T)1) &&          \
                        SAME(d_math_##S##_pow_c(-inf, -(T)0), (T)1),          \
                        #S " pow_c: x^0 and 1^y are 1", 0);                   \
        D_TESTS_EXPECT( isnan(d_math_##S##_pow_c(qnan, (T)2)) &&              \
                        isnan(d_math_##S##_pow_c((T)2, qnan)) &&              \
                        isnan(d_math_##S##_pow_c(-(T)1, qnan)) &&             \
                        isnan(d_math_##S##_pow_c(-(T)2, (T)0.5)) &&           \
                        isnan(d_math_##S##_pow_c(-(T)3, (T)2.25)),            \
                        #S " pow_c: NaN, and a negative x to a fraction", 0); \
        D_TESTS_EXPECT( SAME(d_math_##S##_pow_c((T)0, -(T)3), inf) &&         \
                        SAME(d_math_##S##_pow_c(-(T)0, -(T)3), -inf) &&       \
                        SAME(d_math_##S##_pow_c((T)0, -(T)2), inf) &&         \
                        SAME(d_math_##S##_pow_c(-(T)0, -(T)2), inf) &&        \
                        SAME(d_math_##S##_pow_c(-(T)0, -(T)0.5), inf) &&      \
                        SAME(d_math_##S##_pow_c(-(T)0, -inf), inf) &&         \
                        SAME(d_math_##S##_pow_c((T)0, (T)3), (T)0) &&         \
                        SAME(d_math_##S##_pow_c(-(T)0, (T)3), -(T)0) &&       \
                        SAME(d_math_##S##_pow_c(-(T)0, (T)2), (T)0) &&        \
                        SAME(d_math_##S##_pow_c(-(T)0, (T)0.5), (T)0) &&      \
                        SAME(d_math_##S##_pow_c(-(T)0, inf), (T)0) &&         \
                        SAME(d_math_##S##_pow_c(-(T)0, MAXV), (T)0),          \
                        #S " pow_c: the zeros", 0);                           \
        D_TESTS_EXPECT( SAME(d_math_##S##_pow_c(-(T)1, inf), (T)1) &&         \
                        SAME(d_math_##S##_pow_c(-(T)1, -inf), (T)1) &&        \
                        SAME(d_math_##S##_pow_c(-(T)1, (T)3), -(T)1) &&       \
                        SAME(d_math_##S##_pow_c(-(T)1, MAXV), (T)1) &&        \
                        SAME(d_math_##S##_pow_c((T)0.5, inf), (T)0) &&        \
                        SAME(d_math_##S##_pow_c((T)0.5, -inf), inf) &&        \
                        SAME(d_math_##S##_pow_c(-(T)2, inf), inf) &&          \
                        SAME(d_math_##S##_pow_c(-(T)2, -inf), (T)0),          \
                        #S " pow_c: |x| about 1 to the infinities", 0);       \
        D_TESTS_EXPECT( SAME(d_math_##S##_pow_c(inf, (T)-0.5), (T)0) &&       \
                        SAME(d_math_##S##_pow_c(inf, (T)3), inf) &&           \
                        SAME(d_math_##S##_pow_c(-inf, -(T)3), -(T)0) &&       \
                        SAME(d_math_##S##_pow_c(-inf, -(T)2), (T)0) &&        \
                        SAME(d_math_##S##_pow_c(-inf, -(T)0.5), (T)0) &&      \
                        SAME(d_math_##S##_pow_c(-inf, (T)3), -inf) &&         \
                        SAME(d_math_##S##_pow_c(-inf, (T)2), inf) &&          \
                        SAME(d_math_##S##_pow_c(-inf, (T)0.5), inf),          \
                        #S " pow_c: the infinities", 0);                      \
        D_TESTS_EXPECT( SAME(d_math_##S##_pow_c(-(T)2, (T)3), -(T)8) &&       \
                        SAME(d_math_##S##_pow_c(-(T)2, -(T)3), -(T)0.125) &&  \
                        SAME(d_math_##S##_pow_c(-(T)3, (T)2), (T)9) &&        \
                        SAME(d_math_##S##_pow_c((T)7, (T)1), (T)7) &&         \
                        SAME(d_math_##S##_pow_c(-(T)7, (T)1), -(T)7) &&       \
                        SAME(d_math_##S##_pow_c((T)16, (T)0.25), (T)2) &&     \
                        SAME(d_math_##S##_pow_c((T)16, (T)0.75), (T)8) &&     \
                        SAME(d_math_##S##_pow_c((T)8, -(T)1), (T)0.125) &&    \
                        SAME(d_math_##S##_pow_c(MAXV, (T)2), inf) &&          \
                        SAME(d_math_##S##_pow_c(MAXV, -(T)2), (T)0) &&        \
                        SAME(d_math_##S##_pow_c(-MAXV, (T)3), -inf),          \
                        #S " pow_c: small and large", 0);                     \
    }

// d_tests_pow_specials
//   every family's special values; the plain form, the library's.
static void
d_tests_pow_specials(void)
{
    volatile double ten   = 10.0;
    volatile double third = 0.3;
    volatile float  two   = 2.0f;

    D_TESTS_POW_SPECIALS(float, f, d_tests_same_f, FLT_MAX)
    D_TESTS_POW_SPECIALS(double, d, d_tests_same_d, DBL_MAX)
    D_TESTS_POW_SPECIALS(long double, ld, d_tests_same_ld, LDBL_MAX)

    // the plain form is the C library's (std::pow in C++, which C++98 has),
    // read through volatiles so that no compiler folds the call
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_pow(ten, third),
                                    g_pow(ten, third))) &&
                    (d_tests_same_f(d_math_f_pow(two, (float)third),
                                    g_powf(two, (float)third))),
                    "pow is the C library's", 0);

    return;
}

// d_tests_hypot_args
//   an argument pair for range _range of a family whose exponents run from
// _emin to _emax with _mant bits: 0, x and y of any size, the subnormals
// among them; 1, y/x in [2^-30, 1], x anywhere; 2, y/x from 2^-(P/2) to
// 2^-(P + 4), where the smaller stops mattering; 3, both subnormal; 4, x in
// the top binade; 5, integers below 2^(P/2 + 2) in a random binade -- many
// exact results among them. Either sign, either order.
static void
d_tests_hypot_args(
    int          _range,
    int          _emin,
    int          _emax,
    int          _mant,
    long double* _x,
    long double* _y
)
{
    const int         span = _emax - _emin;
    const long double u    = 1.0L + (long double)d_tests_unit();
    const long double v    = 1.0L + (long double)d_tests_unit();
    int               ex   = 0;
    long double       t    = 0.0L;

    switch (_range)
    {
        case 0:
            *_x = ldexpl(u, _emin - _mant +
                            (int)d_tests_below((unsigned long)(span + _mant)));
            *_y = ldexpl(v, _emin - _mant +
                            (int)d_tests_below((unsigned long)(span + _mant)));
            break;

        case 1:
            ex  = _emin + (int)d_tests_below((unsigned long)(span - 1));
            *_x = ldexpl(u, ex);
            *_y = ldexpl(v, ex - (int)d_tests_below(31u));
            break;

        case 2:
            ex  = _emin + _mant + 8 +
                  (int)d_tests_below((unsigned long)(span - _mant - 10));
            *_x = ldexpl(u, ex);
            *_y = ldexpl(v, ex - (_mant / 2) -
                            (int)d_tests_below(
                                (unsigned long)((_mant / 2) + 5)));
            break;

        case 3:
            *_x = ldexpl(u, _emin - 2 -
                            (int)d_tests_below((unsigned long)_mant));
            *_y = ldexpl(v, _emin - 2 -
                            (int)d_tests_below((unsigned long)_mant));
            break;

        case 4:
            *_x = ldexpl(u, _emax - 1);
            *_y = ldexpl(v, _emax - 1 -
                            (int)d_tests_below((unsigned long)(_mant + 4)));
            break;

        default:
            ex  = _emin - 4 +
                  (int)d_tests_below((unsigned long)(span - (_mant / 2) - 2));
            *_x = ldexpl(d_tests_top((_mant / 2) + 2), ex);
            *_y = ldexpl(d_tests_top((_mant / 2) + 2), ex);
            break;
    }

    if (d_tests_below(2u) == 0u)
    {
        *_x = -*_x;
    }

    if (d_tests_below(2u) == 0u)
    {
        t   = *_x;
        *_x = *_y;
        *_y = t;
    }

    return;
}

// d_tests_hypot_sign_f
//   the sign of _m^2 - (_a^2 + _b^2), exactly, for floats _a and _b and _m
// of 25 bits at most, a float's midpoint: a^2 and b^2 have 48 bits and m^2
// 50, each exact in any long double; a^2 + b^2 as a two-sum pair s + s_lo,
// exact; m^2 - s exact where the two lie within a factor of 2 of each other
// (Sterbenz's lemma), and past s_lo by far where they do not; so the one
// rounding left keeps the sign.
static int
d_tests_hypot_sign_f(
    long double _a,
    long double _b,
    long double _m
)
{
    const long double a2   = _a * _a;
    const long double b2   = _b * _b;
    const long double s    = a2 + b2;
    const long double bv   = s - a2;
    const long double s_lo = (a2 - (s - bv)) + (b2 - bv);
    const long double diff = ((_m * _m) - s) - s_lo;

    return (diff > 0.0L) ? 1 : ((diff < 0.0L) ? -1 : 0);
}

// d_tests_hypot_correct_f
//   whether _r is hypot(_x, _y) correctly rounded: the midpoints about _r --
// half the gap above it and below it, which is half as wide at a power of 2
// -- the one above past the root, or on it with _r even, and the one below
// short of it, or on it with _r even. +inf where the root lies on or past
// the midpoint above FLT_MAX; and C's special values.
static bool
d_tests_hypot_correct_f(
    float _x,
    float _y,
    float _r
)
{
    const long double a     = fabsl((long double)_x);
    const long double b     = fabsl((long double)_y);
    const long double r     = (long double)_r;
    // half FLT_MAX's last place: the midpoint above it is FLT_MAX + top
    const long double top   = ldexpl(1.0L, FLT_MAX_EXP - FLT_MANT_DIG - 1);
    long double       above = 0.0L;
    long double       below = 0.0L;
    uint32_t          bits  = 0u;
    bool              even  = false;

    if ( isinf(_x) ||
         isinf(_y) )
    {
        return d_tests_same_f(_r, (float)HUGE_VAL);
    }

    if ( isnan(_x) ||
         isnan(_y) )
    {
        return isnan(_r);
    }

    if ( (a == 0.0L) ||
         (b == 0.0L) )
    {
        return d_tests_same_f(_r, (float)(a + b));
    }

    if (isinf(_r))
    {
        // FLT_MAX is odd: on its midpoint above, the root goes to +inf
        return (_r > 0.0f) &&
               (d_tests_hypot_sign_f(a, b, (long double)FLT_MAX + top) <= 0);
    }

    if ( isnan(_r) ||
         (_r <= 0.0f) )
    {
        return false;
    }

    memcpy(&bits, &_r, sizeof(bits));
    even  = ((bits % 2u) == 0u);
    below = (r - (long double)nextafterf(_r, 0.0f)) / 2.0L;
    above = (_r == FLT_MAX) ? top
                            : (((long double)nextafterf(_r, FLT_MAX) - r) /
                               2.0L);

    {
        const int up   = d_tests_hypot_sign_f(a, b, r + above);
        const int down = d_tests_hypot_sign_f(a, b, r - below);

        return ( ((up > 0) || ((up == 0) && even)) &&
                 ((down < 0) || ((down == 0) && even)) );
    }
}

// d_tests_hypot_float
//   hypot_c correctly rounded, by the exact check, and within an ulp of
// hypotf: a million argument pairs in six ranges; with _exhaustive,
// besides, every positive float with 1 and with the float nearest 3/4 of
// it.
static void
d_tests_hypot_float(
    bool _exhaustive
)
{
    long     differ = 0;
    long     i      = 0;
    uint32_t bits   = 0u;

    for (i = 0; i < 1000000; ++i)
    {
        long double lx = 0.0L;
        long double ly = 0.0L;
        float       x  = 0.0f;
        float       y  = 0.0f;
        float       r  = 0.0f;

        d_tests_hypot_args((int)(i % 6),
                           FLT_MIN_EXP,
                           FLT_MAX_EXP,
                           FLT_MANT_DIG,
                           &lx,
                           &ly);
        x = (float)lx;
        y = (float)ly;
        r = d_math_f_hypot_c(x, y);

        D_TESTS_EXPECT(d_tests_hypot_correct_f(x, y, r),
                       "f hypot_c correctly rounded", x);
        D_TESTS_EXPECT(D_TESTS_ULPS_F(r, g_hypotf(x, y)) <= 1.0,
                       "f hypot_c within an ulp of hypotf", x);
        differ += d_tests_same_f(r, g_hypotf(x, y)) ? 0 : 1;
    }

    for (bits = (_exhaustive ? 1u : 0x80000000u); bits < 0x7F800000u; ++bits)
    {
        const uint32_t b = (uint32_t)bits;
        float          x = 0.0f;

        memcpy(&x, &b, sizeof(x));
        D_TESTS_EXPECT(d_tests_hypot_correct_f(x,
                                               1.0f,
                                               d_math_f_hypot_c(x, 1.0f)),
                       "f hypot_c(x, 1) correctly rounded", x);
        D_TESTS_EXPECT(d_tests_hypot_correct_f(x,
                                               x * 0.75f,
                                               d_math_f_hypot_c(x,
                                                                x * 0.75f)),
                       "f hypot_c(x, 3x/4) correctly rounded", x);
    }

    printf("float: hypot_c differs from hypotf at %ld of 1000000 pairs\n",
           differ);

    return;
}

// d_tests_hypot_double
//   hypot_c within an ulp of hypot -- glibc's is within one -- at two million
// argument pairs in six ranges, how many differ at all printed; and, at
// pairs of integers below 2^26 in a random binade, whose squares' sum is
// exact in a double, the square root of that sum: sqrt_c's, correctly
// rounded.
static void
d_tests_hypot_double(void)
{
    long differ = 0;
    long i      = 0;

    for (i = 0; i < 2000000; ++i)
    {
        long double lx = 0.0L;
        long double ly = 0.0L;
        double      x  = 0.0;
        double      y  = 0.0;
        double      r  = 0.0;

        d_tests_hypot_args((int)(i % 6),
                           DBL_MIN_EXP,
                           DBL_MAX_EXP,
                           DBL_MANT_DIG,
                           &lx,
                           &ly);
        x = (double)lx;
        y = (double)ly;
        r = d_math_d_hypot_c(x, y);

        D_TESTS_EXPECT(D_TESTS_ULPS_D(r, g_hypot(x, y)) <= 1.0,
                       "d hypot_c within an ulp of hypot", x);
        differ += d_tests_same_d(r, g_hypot(x, y)) ? 0 : 1;
    }

    for (i = 0; i < 300000; ++i)
    {
        const double a = (double)d_tests_top(26);
        const double b = (double)d_tests_top(26);
        const int    e = (int)d_tests_below(1800u) - 900;

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_hypot_c(ldexp(a, e),
                                                       ldexp(b, e)),
                                      ldexp(d_math_d_sqrt_c((a * a) + (b * b)),
                                            e)),
                       "d hypot_c of integers, the root of the exact sum", a);
    }

    printf("double: hypot_c differs from hypot at %ld of 2000000 pairs\n",
           differ);

    return;
}

// D_TESTS_HYPOT_TABLE
//   one table's rows, { a, ea, b, eb, r, er }: hypot(a 2^ea, b 2^eb) is
// r 2^er, or +inf where r is -1; each row with its arguments swapped and
// negated too.
#define D_TESTS_HYPOT_TABLE(T, S, SAME, LDEXP, ROWS, WHAT)                    \
    {                                                                         \
        size_t k = 0;                                                         \
                                                                              \
        for (k = 0; k < (sizeof(ROWS) / sizeof(ROWS[0])); ++k)                \
        {                                                                     \
            const T x    = (T)LDEXP(ROWS[k][0], (int)ROWS[k][1]);             \
            const T y    = (T)LDEXP(ROWS[k][2], (int)ROWS[k][3]);             \
            const T want = (ROWS[k][4] < 0) ? (T)HUGE_VAL                     \
                         : (T)LDEXP(ROWS[k][4], (int)ROWS[k][5]);             \
                                                                              \
            D_TESTS_EXPECT( SAME(d_math_##S##_hypot_c(x, y), want) &&         \
                            SAME(d_math_##S##_hypot_c(y, x), want) &&         \
                            SAME(d_math_##S##_hypot_c(-x, y), want) &&        \
                            SAME(d_math_##S##_hypot_c(y, -x), want),          \
                            WHAT, (long double)k);                            \
        }                                                                     \
    }

// d_tests_hypot_edges
//   the tables, from rational arithmetic (tools/edge_values9.py): a
// Pythagorean triple's hypotenuse of P + 1 bits on a midpoint, to even, in
// place and scaled to small and large binades; roots within 1e-3 of a last
// place of a midpoint for float, 1e-8 for double and closer for long
// double, either side; the overflow threshold, either side; the smaller
// about 2^-(P/2) and 2^-(P + 2) of the larger; subnormal results and
// results about the least normal value; hypot(x, x) at the ends. Long
// double's table is its format's: a double's, an x87's or a quad's.
static void
d_tests_hypot_edges(void)
{
    static const double floats[][6] = {
        { 16776945.0, 0, 94231.0, 3, 1049617.0, 4 },
        { 16776945.0, -63, 94231.0, -60, 1049617.0, -59 },
        { 16776945.0, 102, 94231.0, 105, 1049617.0, 106 },
        { 16776963.0, 0, 264321.0, 2, 4202561.0, 2 },
        { 16776963.0, -63, 264321.0, -61, 4202561.0, -61 },
        { 16776963.0, 102, 264321.0, 104, 4202561.0, 104 },
        { 16776837.0, 0, 323821.0, 2, 4206691.0, 2 },
        { 16776837.0, -63, 323821.0, -61, 4206691.0, -61 },
        { 16776837.0, 102, 323821.0, 104, 4206691.0, 104 },
        { 16776511.0, 0, 187575.0, 3, 526359.0, 5 },
        { 16776511.0, -63, 187575.0, -60, 526359.0, -58 },
        { 16776511.0, 102, 187575.0, 105, 526359.0, 107 },
        { 16777215.0, 0, 11585.0, 1, 8388615.0, 1 },
        { 16777215.0, 0, 23171.0, 0, 1048577.0, 4 },
        { 16777211.0, 0, 25905.0, 0, 8388615.0, 1 },
        { 16777211.0, 0, 12953.0, 1, 1048577.0, 4 },
        { 16777215.0, 0, 51159.0, 0, 4194323.0, 2 },
        { 16777215.0, 0, 6395.0, 3, 8388647.0, 1 },
        { 16777215.0, 104, 16777213.0, 92, 16777215.0, 104 },
        { 16777215.0, 104, 8388607.0, 93, 16777215.0, 104 },
        { 16777215.0, 104, 16777215.0, 92, 16777215.0, 104 },
        { 16777215.0, 104, 1.0, 116, -1.0, 0 },
        { 1.0, 0, 1.0, -11, 8388609.0, -23 },
        { 16777215.0, -24, 3.0, -13, 1.0, 0 },
        { 1.0, 0, 1.0, -12, 1.0, 0 },
        { 16777215.0, -24, 3.0, -14, 16777215.0, -24 },
        { 1.0, 0, 1.0, -13, 1.0, 0 },
        { 16777215.0, -24, 3.0, -15, 16777215.0, -24 },
        { 1.0, 0, 1.0, -25, 1.0, 0 },
        { 16777215.0, -24, 3.0, -27, 16777215.0, -24 },
        { 1.0, 0, 1.0, -26, 1.0, 0 },
        { 16777215.0, -24, 3.0, -28, 16777215.0, -24 },
        { 1.0, 0, 1.0, -27, 1.0, 0 },
        { 16777215.0, -24, 3.0, -29, 16777215.0, -24 },
        { 3.0, -149, 1.0, -147, 5.0, -149 },
        { 1.0, -149, 1.0, -149, 1.0, -149 },
        { 5.0, -149, 3.0, -147, 13.0, -149 },
        { 8388607.0, -149, 8388605.0, -149, 741455.0, -145 },
        { 1.0, -127, 1.0, -127, 2965821.0, -148 },
        { 12345.0, -149, 6789.0, -149, 14089.0, -149 },
        { 1.0, -126, 1.0, -126, 11863283.0, -149 },
        { 16777215.0, -149, 1.0, -149, 16777215.0, -149 },
        { 16777215.0, 104, 16777215.0, 104, -1.0, 0 },
        { 1.0, -149, 1.0, -149, 1.0, -149 },
        { 1.0, 0, 1.0, 0, 11863283.0, -23 },
    };
    static const double doubles[][6] = {
        { 9007199254710947.0, 0, 400836614451.0, 2, 2251799849353641.0, 2 },
        { 9007199254710947.0, -511, 400836614451.0, -509,
          2251799849353641.0, -509 },
        { 9007199254710947.0, 969, 400836614451.0, 971,
          2251799849353641.0, 971 },
        { 9007199254729689.0, 0, 47927664835.0, 6, 1125899972127611.0, 3 },
        { 9007199254729689.0, -511, 47927664835.0, -505,
          1125899972127611.0, -508 },
        { 9007199254729689.0, 969, 47927664835.0, 975,
          1125899972127611.0, 972 },
        { 9007199254677655.0, 0, 503881103379.0, 3, 1125900019587249.0, 3 },
        { 9007199254677655.0, -511, 503881103379.0, -508,
          1125900019587249.0, -508 },
        { 9007199254677655.0, 969, 503881103379.0, 972,
          1125900019587249.0, 972 },
        { 9007199254701765.0, 0, 1201228646733.0, 2, 2251800134074739.0, 2 },
        { 9007199254701765.0, -511, 1201228646733.0, -509,
          2251800134074739.0, -509 },
        { 9007199254701765.0, 969, 1201228646733.0, 971,
          2251800134074739.0, 971 },
        { 9007199254740991.0, 0, 1.0, 29, 4503599627370503.0, 1 },
        { 9007199254740991.0, 0, 536870913.0, 0, 562949953421313.0, 4 },
        { 9007199254740987.0, 0, 600239927.0, 0, 4503599627370503.0, 1 },
        { 9007199254740987.0, 0, 75029991.0, 3, 562949953421313.0, 4 },
        { 9007199254740991.0, 0, 1185378877.0, 0, 2251799813685267.0, 2 },
        { 9007199254740991.0, 0, 592689439.0, 1, 4503599627370535.0, 1 },
        { 9007199254740991.0, 971, 3184525836262885.0, 946,
          9007199254740991.0, 971 },
        { 9007199254740991.0, 971, 6369051672525771.0, 945,
          9007199254740991.0, 971 },
        { 9007199254740991.0, 971, 1592262918131443.0, 947,
          9007199254740991.0, 971 },
        { 9007199254740991.0, 971, 6369051672525773.0, 945, -1.0, 0 },
        { 9007199254740991.0, 971, 3184525836262887.0, 946, -1.0, 0 },
        { 1.0, 0, 1.0, -25, 2251799813685249.0, -51 },
        { 9007199254740991.0, -53, 3.0, -27, 4503599627370497.0, -52 },
        { 1.0, 0, 1.0, -26, 1.0, 0 },
        { 9007199254740991.0, -53, 3.0, -28, 1.0, 0 },
        { 1.0, 0, 1.0, -27, 1.0, 0 },
        { 9007199254740991.0, -53, 3.0, -29, 9007199254740991.0, -53 },
        { 1.0, 0, 1.0, -54, 1.0, 0 },
        { 9007199254740991.0, -53, 3.0, -56, 9007199254740991.0, -53 },
        { 1.0, 0, 1.0, -55, 1.0, 0 },
        { 9007199254740991.0, -53, 3.0, -57, 9007199254740991.0, -53 },
        { 1.0, 0, 1.0, -56, 1.0, 0 },
        { 9007199254740991.0, -53, 3.0, -58, 9007199254740991.0, -53 },
        { 3.0, -1074, 1.0, -1072, 5.0, -1074 },
        { 1.0, -1074, 1.0, -1074, 1.0, -1074 },
        { 5.0, -1074, 3.0, -1072, 13.0, -1074 },
        { 4503599627370495.0, -1074, 4503599627370493.0, -1074,
          3184525836262885.0, -1073 },
        { 1.0, -1023, 1.0, -1023, 1592262918131443.0, -1073 },
        { 12345.0, -1074, 6789.0, -1074, 14089.0, -1074 },
        { 1.0, -1022, 1.0, -1022, 6369051672525773.0, -1074 },
        { 9007199254740991.0, -1074, 1.0, -1074, 9007199254740991.0, -1074 },
        { 9007199254740991.0, 971, 9007199254740991.0, 971, -1.0, 0 },
        { 1.0, -1074, 1.0, -1074, 1.0, -1074 },
        { 1.0, 0, 1.0, 0, 6369051672525773.0, -52 },
    };
#if (LDBL_MANT_DIG == 53)
    static const double longs[][6] = {
        { 9007199254710947.0, 0, 400836614451.0, 2, 2251799849353641.0, 2 },
        { 9007199254710947.0, -511, 400836614451.0, -509,
          2251799849353641.0, -509 },
        { 9007199254710947.0, 969, 400836614451.0, 971,
          2251799849353641.0, 971 },
        { 9007199254729689.0, 0, 47927664835.0, 6, 1125899972127611.0, 3 },
        { 9007199254729689.0, -511, 47927664835.0, -505,
          1125899972127611.0, -508 },
        { 9007199254729689.0, 969, 47927664835.0, 975,
          1125899972127611.0, 972 },
        { 9007199254677655.0, 0, 503881103379.0, 3, 1125900019587249.0, 3 },
        { 9007199254677655.0, -511, 503881103379.0, -508,
          1125900019587249.0, -508 },
        { 9007199254677655.0, 969, 503881103379.0, 972,
          1125900019587249.0, 972 },
        { 9007199254701765.0, 0, 1201228646733.0, 2, 2251800134074739.0, 2 },
        { 9007199254701765.0, -511, 1201228646733.0, -509,
          2251800134074739.0, -509 },
        { 9007199254701765.0, 969, 1201228646733.0, 971,
          2251800134074739.0, 971 },
        { 9007199254740991.0, 0, 1.0, 29, 4503599627370503.0, 1 },
        { 9007199254740991.0, 0, 536870913.0, 0, 562949953421313.0, 4 },
        { 9007199254740987.0, 0, 600239927.0, 0, 4503599627370503.0, 1 },
        { 9007199254740987.0, 0, 75029991.0, 3, 562949953421313.0, 4 },
        { 9007199254740991.0, 0, 1185378877.0, 0, 2251799813685267.0, 2 },
        { 9007199254740991.0, 0, 592689439.0, 1, 4503599627370535.0, 1 },
        { 9007199254740991.0, 971, 3184525836262885.0, 946,
          9007199254740991.0, 971 },
        { 9007199254740991.0, 971, 6369051672525771.0, 945,
          9007199254740991.0, 971 },
        { 9007199254740991.0, 971, 1592262918131443.0, 947,
          9007199254740991.0, 971 },
        { 9007199254740991.0, 971, 6369051672525773.0, 945, -1.0, 0 },
        { 9007199254740991.0, 971, 3184525836262887.0, 946, -1.0, 0 },
        { 1.0, 0, 1.0, -25, 2251799813685249.0, -51 },
        { 9007199254740991.0, -53, 3.0, -27, 4503599627370497.0, -52 },
        { 1.0, 0, 1.0, -26, 1.0, 0 },
        { 9007199254740991.0, -53, 3.0, -28, 1.0, 0 },
        { 1.0, 0, 1.0, -27, 1.0, 0 },
        { 9007199254740991.0, -53, 3.0, -29, 9007199254740991.0, -53 },
        { 1.0, 0, 1.0, -54, 1.0, 0 },
        { 9007199254740991.0, -53, 3.0, -56, 9007199254740991.0, -53 },
        { 1.0, 0, 1.0, -55, 1.0, 0 },
        { 9007199254740991.0, -53, 3.0, -57, 9007199254740991.0, -53 },
        { 1.0, 0, 1.0, -56, 1.0, 0 },
        { 9007199254740991.0, -53, 3.0, -58, 9007199254740991.0, -53 },
        { 3.0, -1074, 1.0, -1072, 5.0, -1074 },
        { 1.0, -1074, 1.0, -1074, 1.0, -1074 },
        { 5.0, -1074, 3.0, -1072, 13.0, -1074 },
        { 4503599627370495.0, -1074, 4503599627370493.0, -1074,
          3184525836262885.0, -1073 },
        { 1.0, -1023, 1.0, -1023, 1592262918131443.0, -1073 },
        { 12345.0, -1074, 6789.0, -1074, 14089.0, -1074 },
        { 1.0, -1022, 1.0, -1022, 6369051672525773.0, -1074 },
        { 9007199254740991.0, -1074, 1.0, -1074, 9007199254740991.0, -1074 },
        { 9007199254740991.0, 971, 9007199254740991.0, 971, -1.0, 0 },
        { 1.0, -1074, 1.0, -1074, 1.0, -1074 },
        { 1.0, 0, 1.0, 0, 6369051672525773.0, -52 },
    };
#elif (LDBL_MANT_DIG == 64)
    static const long double longs[][6] = {
        { 18446744073709533085.0L, 0, 199033079510277.0L, 2,
          4611686022722359833.0L, 2 },
        { 18446744073709533085.0L, -8191, 199033079510277.0L, -8189,
          4611686022722359833.0L, -8189 },
        { 18446744073709533085.0L, 16318, 199033079510277.0L, 16320,
          4611686022722359833.0L, 16320 },
        { 18446744073709289475.0L, 0, 281477124325377.0L, 2,
          4611686027017388033.0L, 2 },
        { 18446744073709289475.0L, -8191, 281477124325377.0L, -8189,
          4611686027017388033.0L, -8189 },
        { 18446744073709289475.0L, 16318, 281477124325377.0L, 16320,
          4611686027017388033.0L, 16320 },
        { 18446744073709474501.0L, 0, 344735550254235.0L, 2,
          4611686031312309075.0L, 2 },
        { 18446744073709474501.0L, -8191, 344735550254235.0L, -8189,
          4611686031312309075.0L, -8189 },
        { 18446744073709474501.0L, 16318, 344735550254235.0L, 16320,
          4611686031312309075.0L, 16320 },
        { 18446744073709106775.0L, 0, 199034153391125.0L, 3,
          2305843017803684153.0L, 3 },
        { 18446744073709106775.0L, -8191, 199034153391125.0L, -8188,
          2305843017803684153.0L, -8188 },
        { 18446744073709106775.0L, 16318, 199034153391125.0L, 16321,
          2305843017803684153.0L, 16321 },
        { 18446744073709551615.0L, 0, 24296003999.0L, 0,
          9223372036854775815.0L, 1 },
        { 18446744073709551615.0L, 0, 759250125.0L, 5,
          1152921504606846977.0L, 4 },
        { 18446744073709551611.0L, 0, 13581879131.0L, 1,
          9223372036854775815.0L, 1 },
        { 18446744073709551611.0L, 0, 27163758263.0L, 0,
          1152921504606846977.0L, 4 },
        { 18446744073709551615.0L, 0, 53644124333.0L, 0,
          4611686018427387923.0L, 2 },
        { 18446744073709551615.0L, 0, 26822062167.0L, 1,
          9223372036854775847.0L, 1 },
        { 18446744073709551615.0L, 16320, 18446744073709551613.0L, 16288,
          18446744073709551615.0L, 16320 },
        { 18446744073709551615.0L, 16320, 9223372036854775807.0L, 16289,
          18446744073709551615.0L, 16320 },
        { 18446744073709551615.0L, 16320, 18446744073709551615.0L, 16288,
          18446744073709551615.0L, 16320 },
        { 18446744073709551615.0L, 16320, 1.0L, 16352, -1.0L, 0 },
        { 1.0L, 0, 1.0L, -31, 9223372036854775809.0L, -63 },
        { 18446744073709551615.0L, -64, 3.0L, -33, 1.0L, 0 },
        { 1.0L, 0, 1.0L, -32, 1.0L, 0 },
        { 18446744073709551615.0L, -64, 3.0L, -34,
          18446744073709551615.0L, -64 },
        { 1.0L, 0, 1.0L, -33, 1.0L, 0 },
        { 18446744073709551615.0L, -64, 3.0L, -35,
          18446744073709551615.0L, -64 },
        { 1.0L, 0, 1.0L, -65, 1.0L, 0 },
        { 18446744073709551615.0L, -64, 3.0L, -67,
          18446744073709551615.0L, -64 },
        { 1.0L, 0, 1.0L, -66, 1.0L, 0 },
        { 18446744073709551615.0L, -64, 3.0L, -68,
          18446744073709551615.0L, -64 },
        { 1.0L, 0, 1.0L, -67, 1.0L, 0 },
        { 18446744073709551615.0L, -64, 3.0L, -69,
          18446744073709551615.0L, -64 },
        { 3.0L, -16445, 1.0L, -16443, 5.0L, -16445 },
        { 1.0L, -16445, 1.0L, -16445, 1.0L, -16445 },
        { 5.0L, -16445, 3.0L, -16443, 13.0L, -16445 },
        { 9223372036854775807.0L, -16445, 9223372036854775805.0L, -16445,
          6521908912666391105.0L, -16444 },
        { 1.0L, -16383, 1.0L, -16383, 3260954456333195553.0L, -16444 },
        { 12345.0L, -16445, 6789.0L, -16445, 14089.0L, -16445 },
        { 1.0L, -16382, 1.0L, -16382, 3260954456333195553.0L, -16443 },
        { 18446744073709551615.0L, -16445, 1.0L, -16445,
          18446744073709551615.0L, -16445 },
        { 18446744073709551615.0L, 16320, 18446744073709551615.0L, 16320,
          -1.0L, 0 },
        { 1.0L, -16445, 1.0L, -16445, 1.0L, -16445 },
        { 1.0L, 0, 1.0L, 0, 3260954456333195553.0L, -61 },
    };
#elif (LDBL_MANT_DIG == 113)
    static const long double longs[][6] = {
        { 10384593717069655257060991796861355.0L, 0,
          22608407016843931292719177.0L, 2,
          2596148429267413912707244191994183.0L, 2 },
        { 10384593717069655257060991796861355.0L, -8191,
          22608407016843931292719177.0L, -8189,
          2596148429267413912707244191994183.0L, -8189 },
        { 10384593717069655257060991796861355.0L, 16269,
          22608407016843931292719177.0L, 16271,
          2596148429267413912707244191994183.0L, 16271 },
        { 10384593717069655257060991569774869.0L, 0,
          32253064665027618246186105.0L, 2,
          2596148429267414014612071009178167.0L, 2 },
        { 10384593717069655257060991569774869.0L, -8191,
          32253064665027618246186105.0L, -8189,
          2596148429267414014612071009178167.0L, -8189 },
        { 10384593717069655257060991569774869.0L, 16269,
          32253064665027618246186105.0L, 16271,
          2596148429267414014612071009178167.0L, 16271 },
        { 10384593717069655257060991155469887.0L, 0,
          19807702373191062126936373.0L, 3,
          324518553658426764564612234145847.0L, 5 },
        { 10384593717069655257060991155469887.0L, -8191,
          19807702373191062126936373.0L, -8188,
          324518553658426764564612234145847.0L, -8186 },
        { 10384593717069655257060991155469887.0L, 16269,
          19807702373191062126936373.0L, 16272,
          324518553658426764564612234145847.0L, 16274 },
        { 10384593717069655257060990296087529.0L, 0,
          11452348032468015806311795.0L, 4,
          1298074214633707109210862424212365.0L, 3 },
        { 10384593717069655257060990296087529.0L, -8191,
          11452348032468015806311795.0L, -8187,
          1298074214633707109210862424212365.0L, -8188 },
        { 10384593717069655257060990296087529.0L, 16269,
          11452348032468015806311795.0L, 16273,
          1298074214633707109210862424212365.0L, 16272 },
        { 10384593717069655257060992658440191.0L, 0, 1.0L, 59,
          5192296858534827628530496329220103.0L, 1 },
        { 10384593717069655257060992658440191.0L, 0,
          576460752303423489.0L, 0, 649037107316853453566312041152513.0L, 4 },
        { 10384593717069655257060992658440187.0L, 0, 20140709820486303.0L, 5,
          5192296858534827628530496329220103.0L, 1 },
        { 10384593717069655257060992658440187.0L, 0,
          644502714255561697.0L, 0, 649037107316853453566312041152513.0L, 4 },
        { 10384593717069655257060992658440191.0L, 0, 9943678736654855.0L, 7,
          2596148429267413814265248164610067.0L, 2 },
        { 10384593717069655257060992658440191.0L, 0,
          1272790878291821441.0L, 0,
          5192296858534827628530496329220135.0L, 1 },
        { 10384593717069655257060992658440191.0L, 16271,
          7343016637207168931428032607349395.0L, 16215,
          10384593717069655257060992658440191.0L, 16271 },
        { 10384593717069655257060992658440191.0L, 16271,
          1835754159301792232857008151837349.0L, 16217,
          10384593717069655257060992658440191.0L, 16271 },
        { 10384593717069655257060992658440191.0L, 16271,
          7343016637207168931428032607349397.0L, 16215,
          10384593717069655257060992658440191.0L, 16271 },
        { 10384593717069655257060992658440191.0L, 16271,
          3671508318603584465714016303674699.0L, 16216, -1.0L, 0 },
        { 10384593717069655257060992658440191.0L, 16271,
          7343016637207168931428032607349399.0L, 16215, -1.0L, 0 },
        { 1.0L, 0, 1.0L, -55, 2596148429267413814265248164610049.0L, -111 },
        { 10384593717069655257060992658440191.0L, -113, 3.0L, -57,
          5192296858534827628530496329220097.0L, -112 },
        { 1.0L, 0, 1.0L, -56, 1.0L, 0 },
        { 10384593717069655257060992658440191.0L, -113, 3.0L, -58, 1.0L, 0 },
        { 1.0L, 0, 1.0L, -57, 1.0L, 0 },
        { 10384593717069655257060992658440191.0L, -113, 3.0L, -59,
          10384593717069655257060992658440191.0L, -113 },
        { 1.0L, 0, 1.0L, -114, 1.0L, 0 },
        { 10384593717069655257060992658440191.0L, -113, 3.0L, -116,
          10384593717069655257060992658440191.0L, -113 },
        { 1.0L, 0, 1.0L, -115, 1.0L, 0 },
        { 10384593717069655257060992658440191.0L, -113, 3.0L, -117,
          10384593717069655257060992658440191.0L, -113 },
        { 1.0L, 0, 1.0L, -116, 1.0L, 0 },
        { 10384593717069655257060992658440191.0L, -113, 3.0L, -118,
          10384593717069655257060992658440191.0L, -113 },
        { 3.0L, -16494, 1.0L, -16492, 5.0L, -16494 },
        { 1.0L, -16494, 1.0L, -16494, 1.0L, -16494 },
        { 5.0L, -16494, 3.0L, -16492, 13.0L, -16494 },
        { 5192296858534827628530496329220095.0L, -16494,
          5192296858534827628530496329220093.0L, -16494,
          7343016637207168931428032607349395.0L, -16494 },
        { 1.0L, -16383, 1.0L, -16383,
          3671508318603584465714016303674699.0L, -16494 },
        { 12345.0L, -16494, 6789.0L, -16494, 14089.0L, -16494 },
        { 1.0L, -16382, 1.0L, -16382,
          7343016637207168931428032607349397.0L, -16494 },
        { 10384593717069655257060992658440191.0L, -16494, 1.0L, -16494,
          10384593717069655257060992658440191.0L, -16494 },
        { 10384593717069655257060992658440191.0L, 16271,
          10384593717069655257060992658440191.0L, 16271, -1.0L, 0 },
        { 1.0L, -16494, 1.0L, -16494, 1.0L, -16494 },
        { 1.0L, 0, 1.0L, 0, 7343016637207168931428032607349397.0L, -112 },
    };
#endif

    D_TESTS_HYPOT_TABLE(float, f, d_tests_same_f, ldexp, floats,
                        "f hypot_c at an edge value")
    D_TESTS_HYPOT_TABLE(double, d, d_tests_same_d, ldexp, doubles,
                        "d hypot_c at an edge value")
#if ( (LDBL_MANT_DIG == 53) ||                                                \
      (LDBL_MANT_DIG == 64) ||                                                \
      (LDBL_MANT_DIG == 113) )
    D_TESTS_HYPOT_TABLE(long double, ld, d_tests_same_ld, ldexpl, longs,
                        "ld hypot_c at an edge value")
#endif

    return;
}

// d_tests_hypot_long_double
//   hypot_c within an ulp of hypotl at a million argument pairs in six
// ranges, how many differ printed; and at pairs of integers below 2^(P/2 -
// 1), whose squares' sum is exact in any long double, sqrt_c of that sum.
static void
d_tests_hypot_long_double(void)
{
    const int half   = (LDBL_MANT_DIG / 2) - 1;
    long      differ = 0;
    long      i      = 0;

    for (i = 0; i < 1000000; ++i)
    {
        long double x = 0.0L;
        long double y = 0.0L;
        long double r = 0.0L;

        d_tests_hypot_args((int)(i % 6),
                           LDBL_MIN_EXP,
                           LDBL_MAX_EXP,
                           LDBL_MANT_DIG,
                           &x,
                           &y);
        r = d_math_ld_hypot_c(x, y);

        D_TESTS_EXPECT(D_TESTS_ULPS_LD(r, g_hypotl(x, y)) <= 1.0,
                       "ld hypot_c within an ulp of hypotl", x);
        differ += d_tests_same_ld(r, g_hypotl(x, y)) ? 0 : 1;
    }

    for (i = 0; i < 100000; ++i)
    {
        const long double a =
            floorl(ldexpl((long double)d_tests_unit(), half));
        const long double b =
            floorl(ldexpl((long double)d_tests_unit(), half));
        const int         e = (int)d_tests_below(2000u) - 1000;

        D_TESTS_EXPECT(d_tests_same_ld(
                           d_math_ld_hypot_c(ldexpl(a, e), ldexpl(b, e)),
                           ldexpl(d_math_ld_sqrt_c((a * a) + (b * b)), e)),
                       "ld hypot_c of integers, the root of the exact sum",
                       a);
    }

    printf("long double: hypot_c differs from hypotl at %ld of 1000000 "
           "pairs\n", differ);

    return;
}

// d_tests_hypot_sign
//   the sign test the last bit is settled by, about squares of few bits:
// for (a, b) a Pythagorean pair's legs scaled so that a lies in [1, 2), and
// t + h their hypotenuse, a^2 + b^2 is (t + h)^2 exactly -- 0 -- and with b
// a last place either way, -1 or 1: where the approximation lies within
// its bound and the exact sum decides.
static void
d_tests_hypot_sign(void)
{
    // legs and hypotenuse, each over 2^4: 4-3-5, 12-5-13, 15-8-17,
    // 21-20-29, 24-7-25
    static const long double legs[][3] = {
        { 16.0L, 12.0L, 20.0L }, { 24.0L, 10.0L, 26.0L },
        { 30.0L, 16.0L, 34.0L }, { 21.0L, 20.0L, 29.0L },
        { 24.0L, 7.0L, 25.0L }
    };
    size_t k = 0;
    int    j = 0;

    for (k = 0; k < (sizeof(legs) / sizeof(legs[0])); ++k)
    {
        const long double a = legs[k][0] / 16.0L;
        const long double b = legs[k][1] / 16.0L;
        const long double m = legs[k][2] / 16.0L;
        // the squares as Dekker's pairs, b's and its neighbours'
        const struct dj_internal_math_ld_pair a2 =
            d_internal_math_ld_product_exact(a, a);
        const struct dj_internal_math_ld_pair b2 =
            d_internal_math_ld_product_exact(b, b);
        const struct dj_internal_math_ld_pair up =
            d_internal_math_ld_product_exact(d_math_ld_next_up(b),
                                             d_math_ld_next_up(b));
        const struct dj_internal_math_ld_pair down =
            d_internal_math_ld_product_exact(d_math_ld_next_down(b),
                                             d_math_ld_next_down(b));

        for (j = -1; j <= 1; j += 2)
        {
            const long double h = (long double)j / 2048.0L;
            const long double t = m - h;

            D_TESTS_EXPECT( (d_internal_math_ld_hypot_sign(a2, b2, t, h) ==
                             0) &&
                            (d_internal_math_ld_hypot_sign(a2, up, t, h) <
                             0) &&
                            (d_internal_math_ld_hypot_sign(a2, down, t, h) >
                             0) &&
                            (d_internal_math_ld_hypot_sign_exact(a2, b2, t,
                                                                 h) == 0) &&
                            (d_internal_math_ld_hypot_sign_exact(a2, up, t,
                                                                 h) < 0) &&
                            (d_internal_math_ld_hypot_sign_exact(a2, down, t,
                                                                 h) > 0),
                            "the square's sign, exactly, about a square", m);
        }
    }

    return;
}

// D_TESTS_HYPOT_SPECIALS
//   Annex F's special values for one family: an infinity gives +inf, NaN or
// not; NaN otherwise NaN; hypot(x, +-0) is |x|, +0 for the zeros; the
// result the same for either sign and order; the largest and the least.
#define D_TESTS_HYPOT_SPECIALS(T, S, SAME, MAXV, LEAST)                       \
    {                                                                         \
        const T inf  = (T)HUGE_VAL;                                           \
        const T qnan = (T)nan("");                                            \
                                                                              \
        D_TESTS_EXPECT( SAME(d_math_##S##_hypot_c(inf, qnan), inf) &&         \
                        SAME(d_math_##S##_hypot_c(qnan, -inf), inf) &&        \
                        SAME(d_math_##S##_hypot_c(-inf, (T)1), inf) &&        \
                        SAME(d_math_##S##_hypot_c((T)0, -inf), inf) &&        \
                        SAME(d_math_##S##_hypot_c(inf, inf), inf),            \
                        #S " hypot_c: an infinity gives +inf", 0);            \
        D_TESTS_EXPECT( isnan(d_math_##S##_hypot_c(qnan, (T)1)) &&            \
                        isnan(d_math_##S##_hypot_c((T)0, qnan)) &&            \
                        isnan(d_math_##S##_hypot_c(qnan, qnan)),              \
                        #S " hypot_c: NaN gives NaN", 0);                     \
        D_TESTS_EXPECT( SAME(d_math_##S##_hypot_c(-(T)3, (T)0), (T)3) &&      \
                        SAME(d_math_##S##_hypot_c((T)0, -(T)3), (T)3) &&      \
                        SAME(d_math_##S##_hypot_c(-(T)0, -(T)0), (T)0) &&     \
                        SAME(d_math_##S##_hypot_c((T)0, -(T)0), (T)0) &&      \
                        SAME(d_math_##S##_hypot_c(-LEAST, -(T)0), LEAST) &&   \
                        SAME(d_math_##S##_hypot_c(-MAXV, (T)0), MAXV),        \
                        #S " hypot_c: x and a zero is |x|", 0);               \
        D_TESTS_EXPECT( SAME(d_math_##S##_hypot_c((T)3, (T)4), (T)5) &&       \
                        SAME(d_math_##S##_hypot_c(-(T)4, -(T)3), (T)5) &&     \
                        SAME(d_math_##S##_hypot_c((T)5, -(T)12), (T)13) &&    \
                        SAME(d_math_##S##_hypot_c(MAXV, MAXV), inf) &&        \
                        SAME(d_math_##S##_hypot_c(MAXV, (T)1), MAXV) &&       \
                        SAME(d_math_##S##_hypot_c(-LEAST, LEAST), LEAST) &&   \
                        SAME(d_math_##S##_hypot_c((T)1, LEAST), (T)1),        \
                        #S " hypot_c: small and large", 0);                   \
    }

// d_tests_hypot_specials
//   every family's special values; the plain form, the library's, but in
// C++ before C++11, which has no hypot, where it is hypot_c.
static void
d_tests_hypot_specials(void)
{
    volatile double ten   = 10.0;
    volatile double third = 0.3;
    volatile float  two   = 2.0f;

    D_TESTS_HYPOT_SPECIALS(float, f, d_tests_same_f, FLT_MAX,
                           FLT_MIN * FLT_EPSILON)
    D_TESTS_HYPOT_SPECIALS(double, d, d_tests_same_d, DBL_MAX,
                           DBL_MIN * DBL_EPSILON)
    D_TESTS_HYPOT_SPECIALS(long double, ld, d_tests_same_ld, LDBL_MAX,
                           LDBL_MIN * LDBL_EPSILON)

#if ( defined(__cplusplus) &&                                                 \
      !D_ENV_LANG_IS_CPP11_OR_HIGHER )
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_hypot(ten, third),
                                    d_math_d_hypot_c(ten, third))) &&
                    (d_tests_same_f(d_math_f_hypot(two, (float)third),
                                    d_math_f_hypot_c(two, (float)third))),
                    "hypot is hypot_c before C++11", 0);
#else
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_hypot(ten, third),
                                    g_hypot(ten, third))) &&
                    (d_tests_same_f(d_math_f_hypot(two, (float)third),
                                    g_hypotf(two, (float)third))),
                    "hypot is the C library's", 0);
#endif

    return;
}

#if defined(__cplusplus)

// d_tests_overloads
//   compiled as C++: one name for every family, chosen by argument type.
static void
d_tests_overloads(void)
{
    D_TESTS_EXPECT( (d_tests_same_f(d_math_cbrt_c(3.0f),
                                    d_math_f_cbrt_c(3.0f))) &&
                    (d_tests_same_d(d_math_cbrt_c(3.0),
                                    d_math_d_cbrt_c(3.0))) &&
                    (d_tests_same_ld(d_math_cbrt_c(3.0L),
                                     d_math_ld_cbrt_c(3.0L))) &&
                    (d_tests_same_d(d_math_cbrt(3.0), d_math_d_cbrt(3.0))) &&
                    (d_tests_same_ld(d_math_cbrt(3.0L),
                                     d_math_ld_cbrt(3.0L))),
                    "the C++ overloads are the families' functions", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_pow_c(3.0f, 2.5f),
                                    d_math_f_pow_c(3.0f, 2.5f))) &&
                    (d_tests_same_d(d_math_pow_c(3.0, 2.5),
                                    d_math_d_pow_c(3.0, 2.5))) &&
                    (d_tests_same_ld(d_math_pow_c(3.0L, 2.5L),
                                     d_math_ld_pow_c(3.0L, 2.5L))) &&
                    (d_tests_same_d(d_math_pow(3.0, 2.5),
                                    d_math_d_pow(3.0, 2.5))) &&
                    (d_tests_same_ld(d_math_pow(3.0L, 2.5L),
                                     d_math_ld_pow(3.0L, 2.5L))),
                    "the C++ overloads of pow are the families' functions", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_hypot_c(3.0f, 2.5f),
                                    d_math_f_hypot_c(3.0f, 2.5f))) &&
                    (d_tests_same_d(d_math_hypot_c(3.0, 2.5),
                                    d_math_d_hypot_c(3.0, 2.5))) &&
                    (d_tests_same_ld(d_math_hypot_c(3.0L, 2.5L),
                                     d_math_ld_hypot_c(3.0L, 2.5L))) &&
                    (d_tests_same_d(d_math_hypot(3.0, 2.5),
                                    d_math_d_hypot(3.0, 2.5))) &&
                    (d_tests_same_ld(d_math_hypot(3.0L, 2.5L),
                                     d_math_ld_hypot(3.0L, 2.5L))),
                    "the C++ overloads of hypot are the families' functions",
                    0);

    return;
}

#endif  // __cplusplus

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// from C++14, cbrt_c is a constant expression: every family, the edges and
// the specials among them
static constexpr float       g_ccbrt_f   = d_math_f_cbrt_c(3.0f);
static constexpr double      g_ccbrt_d   = d_math_d_cbrt_c(10.0);
static constexpr long double g_ccbrt_ld  = d_math_ld_cbrt_c(2.0L);
static constexpr double      g_ccbrt_top =
    d_math_d_cbrt_c(1.7976931348623157e308);
static constexpr double      g_ccbrt_sub =
    d_math_d_cbrt_c(4.9406564584124654e-324);
static constexpr double      g_ccbrt_mid = d_math_d_cbrt_c(2.3790087208658548);
static constexpr float       g_ccbrt_fsub =
    d_math_f_cbrt_c(1.40129846e-45f);
static constexpr long double g_ccbrt_ldtop = d_math_ld_cbrt_c(LDBL_MAX);

static_assert(d_math_d_cbrt_c(27.0) == 3.0, "cbrt_c(27) is 3");
static_assert(d_math_d_cbrt_c(-0.125) == -0.5, "cbrt_c(-1/8) is -1/2");
static_assert(d_math_d_cbrt_c(2.0) == (double)1.2599210498948732,
              "cbrt_c(2), correctly rounded");
static_assert(d_math_d_cbrt_c(1e-300) == (double)1e-100,
              "cbrt_c(1e-300), correctly rounded");
static_assert(d_math_d_cbrt_c(8.0) == 2.0, "cbrt_c(8) is 2");
static_assert(d_math_cbrt_c(64.0f) == 4.0f, "the C++ overloads, float");

// from C++14, pow_c is a constant expression: every family, its exact
// routes, its midpoints, the zeros and infinities among them
static constexpr double      g_cpow_mid  = d_math_d_pow_c(25.0, 11.5);
static constexpr float       g_cpow_f    = d_math_f_pow_c(4097.0f, 2.0f);
static constexpr long double g_cpow_ld   = d_math_ld_pow_c(3.0L, 2.5L);
static constexpr double      g_cpow_any  = d_math_d_pow_c(1.6075, 2.5);
static constexpr double      g_cpow_sub  = d_math_d_pow_c(2.0, -1074.5);
static constexpr double      g_cpow_rec  = d_math_d_pow_c(10.0, -1.0);
static constexpr double      g_cpow_neg  = d_math_d_pow_c(-7.0, 19.0);
static constexpr double      g_cpow_root = d_math_d_pow_c(1e-300, 0.25);

static_assert(d_math_d_pow_c(2.0, 10.0) == 1024.0, "pow_c(2, 10) is 1024");
static_assert(d_math_d_pow_c(25.0, 11.5) == 11920928955078124.0,
              "pow_c(25, 11.5), 5^23, a midpoint, to even");
static_assert(d_math_f_pow_c(4097.0f, 2.0f) == 16785408.0f,
              "pow_c(4097, 2), a midpoint, to even");
static_assert(d_math_d_pow_c(-2.0, -3.0) == -0.125, "pow_c(-2, -3)");
static_assert(d_math_d_pow_c(-0.0, -3.0) == -(double)HUGE_VAL,
              "pow_c(-0, -3) is -inf");
static_assert(d_math_d_pow_c(2.0, -1075.0) == 0.0,
              "pow_c(2, -1075), half the least subnormal, is 0");
static_assert(d_math_d_pow_c(-8.0, 0.5) != d_math_d_pow_c(-8.0, 0.5),
              "pow_c(-8, 1/2) is NaN");
static_assert(d_math_pow_c(3.0f, 2.0f) == 9.0f, "the C++ overloads, pow");
static_assert(d_math_d_pow_c((double)NAN, 2.0) !=
              d_math_d_pow_c((double)NAN, 2.0),
              "pow_c(NaN, 2) is NaN, as a constant");
static_assert(d_math_d_pow_c(2.0, (double)NAN) !=
              d_math_d_pow_c(2.0, (double)NAN),
              "pow_c(2, NaN) is NaN, as a constant");

// from C++14, hypot_c is a constant expression: every family, a midpoint,
// the overflow, the subnormals, a negligible leg
static constexpr double      g_chyp_d    = d_math_d_hypot_c(1.0, 1.0);
static constexpr float       g_chyp_f    = d_math_f_hypot_c(16777215.0f,
                                                            8192.0f);
static constexpr long double g_chyp_ld   = d_math_ld_hypot_c(2.0L, 3.0L);
static constexpr double      g_chyp_mid  =
    d_math_d_hypot_c(9007199254710947.0, 1603346457804.0);
static constexpr double      g_chyp_top  =
    d_math_d_hypot_c(1.7976931348623157e308, 1.7976931348623157e308);
static constexpr double      g_chyp_sub  =
    d_math_d_hypot_c(4.9406564584124654e-324, 1.4821969375237396e-323);
static constexpr double      g_chyp_tiny = d_math_d_hypot_c(1.0, 1e-10);
static constexpr long double g_chyp_ldtop =
    d_math_ld_hypot_c(LDBL_MAX, LDBL_MAX / 2.0L);

static_assert(d_math_d_hypot_c(3.0, 4.0) == 5.0, "hypot_c(3, 4) is 5");
static_assert(d_math_d_hypot_c(9007199254710947.0, 1603346457804.0) ==
              9007199397414564.0,
              "hypot_c on a midpoint, to even");
static_assert(d_math_f_hypot_c(16777215.0f, 8192.0f) == 16777216.0f,
              "hypot_c(2^24 - 1, 2^13), a midpoint, to even");
static_assert(d_math_d_hypot_c(-0.0, -0.0) == 0.0, "hypot_c of the zeros");
static_assert(d_math_d_hypot_c(1.0, 1.4901161193847656e-8) == 1.0,
              "hypot_c(1, 2^-26), a hair short of a midpoint, is 1");
static_assert(d_math_d_hypot_c(1.7976931348623157e308, 1.0) ==
              1.7976931348623157e308,
              "hypot_c(DBL_MAX, 1) is DBL_MAX");
static_assert(d_math_hypot_c(5.0f, 12.0f) == 13.0f,
              "the C++ overloads, hypot");
static_assert(d_math_d_hypot_c(1.0, (double)NAN) !=
              d_math_d_hypot_c(1.0, (double)NAN),
              "hypot_c(1, NaN) is NaN, as a constant");
static_assert(d_math_d_hypot_c((double)NAN, -(double)HUGE_VAL) ==
              (double)HUGE_VAL,
              "hypot_c(NaN, -inf) is +inf, as a constant");

// d_tests_constant_evaluation
//   the constants are the run time's results, bit for bit: the arguments
// are read through volatiles, so the calls below are not folded.
static void
d_tests_constant_evaluation(void)
{
    volatile float       three_f = 3.0f;
    volatile double      ten_d   = 10.0;
    volatile long double two_ld  = 2.0L;
    volatile double      top     = 1.7976931348623157e308;
    volatile double      least   = 4.9406564584124654e-324;
    volatile double      mid     = 2.3790087208658548;
    volatile float       fleast  = 1.40129846e-45f;
    volatile long double ldtop   = LDBL_MAX;
    volatile double      p25     = 25.0;
    volatile double      p11     = 11.5;
    volatile float       f4097   = 4097.0f;
    volatile long double ld3     = 3.0L;
    volatile double      super   = 1.6075;
    volatile double      two     = 2.0;
    volatile double      ten     = 10.0;
    volatile double      seven   = -7.0;
    volatile double      tiny    = 1e-300;
    volatile double      one     = 1.0;
    volatile float       f24     = 16777215.0f;
    volatile long double ld2     = 2.0L;
    volatile double      leg     = 9007199254710947.0;

    D_TESTS_EXPECT( (d_tests_same_f(g_ccbrt_f, d_math_f_cbrt_c(three_f))) &&
                    (d_tests_same_d(g_ccbrt_d, d_math_d_cbrt_c(ten_d))) &&
                    (d_tests_same_ld(g_ccbrt_ld, d_math_ld_cbrt_c(two_ld))) &&
                    (d_tests_same_d(g_ccbrt_top, d_math_d_cbrt_c(top))) &&
                    (d_tests_same_d(g_ccbrt_sub, d_math_d_cbrt_c(least))) &&
                    (d_tests_same_d(g_ccbrt_mid, d_math_d_cbrt_c(mid))) &&
                    (d_tests_same_f(g_ccbrt_fsub,
                                    d_math_f_cbrt_c(fleast))) &&
                    (d_tests_same_ld(g_ccbrt_ldtop,
                                     d_math_ld_cbrt_c(ldtop))),
                    "constant evaluation agrees with the run time", 0);
    D_TESTS_EXPECT( (d_tests_same_d(g_cpow_mid, d_math_d_pow_c(p25, p11))) &&
                    (d_tests_same_f(g_cpow_f, d_math_f_pow_c(f4097, 2.0f))) &&
                    (d_tests_same_ld(g_cpow_ld, d_math_ld_pow_c(ld3, 2.5L))) &&
                    (d_tests_same_d(g_cpow_any, d_math_d_pow_c(super, 2.5))) &&
                    (d_tests_same_d(g_cpow_sub,
                                    d_math_d_pow_c(two, -1074.5))) &&
                    (d_tests_same_d(g_cpow_rec, d_math_d_pow_c(ten, -1.0))) &&
                    (d_tests_same_d(g_cpow_neg, d_math_d_pow_c(seven, 19.0))) &&
                    (d_tests_same_d(g_cpow_root,
                                    d_math_d_pow_c(tiny, 0.25))),
                    "pow_c: constant evaluation agrees with the run time", 0);
    D_TESTS_EXPECT( (d_tests_same_d(g_chyp_d, d_math_d_hypot_c(one, one))) &&
                    (d_tests_same_f(g_chyp_f,
                                    d_math_f_hypot_c(f24, 8192.0f))) &&
                    (d_tests_same_ld(g_chyp_ld,
                                     d_math_ld_hypot_c(ld2, 3.0L))) &&
                    (d_tests_same_d(g_chyp_mid,
                                    d_math_d_hypot_c(leg, 1603346457804.0))) &&
                    (d_tests_same_d(g_chyp_top, d_math_d_hypot_c(top, top))) &&
                    (d_tests_same_d(g_chyp_sub,
                                    d_math_d_hypot_c(least,
                                                     3.0 * least))) &&
                    (d_tests_same_d(g_chyp_tiny,
                                    d_math_d_hypot_c(one, 1e-10))) &&
                    (d_tests_same_ld(g_chyp_ldtop,
                                     d_math_ld_hypot_c(ldtop,
                                                       ldtop / 2.0L))),
                    "hypot_c: constant evaluation agrees with the run time",
                    0);

    return;
}

#endif  // C++14

int
main(int _argc, char** _argv)
{
    const bool exhaustive = (_argc > 1) &&
                            (strcmp(_argv[1], "exhaustive") == 0);

    // the generator's start, the word 0x2545F4914F6CDD1D
    d_tests_seed(0x2545F491u, 0x4F6CDD1Du);
    d_tests_float(exhaustive);
    d_tests_double();
    d_tests_double_edges();
    d_tests_long_double();
    d_tests_cube_sign_exact();
    d_tests_specials();
    d_tests_pow_float(exhaustive);
    d_tests_pow_double();
    d_tests_pow_edges();
    d_tests_pow_exact();
    d_tests_pow_long_double();
    d_tests_pow_specials();
    d_tests_hypot_float(exhaustive);
    d_tests_hypot_double();
    d_tests_hypot_edges();
    d_tests_hypot_long_double();
    d_tests_hypot_sign();
    d_tests_hypot_specials();
#if defined(__cplusplus)
    d_tests_overloads();
#endif
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    printf("t_power: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
