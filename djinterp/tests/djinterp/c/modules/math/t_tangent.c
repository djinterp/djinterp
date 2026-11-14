/*******************************************************************************
* djinterp [c/math]                                                  t_tangent.c
*
*   Conformance harness for c/math/tangent.h. For float, tan_c and atan_c
* must be the correctly rounded result -- the long double library's, rounded
* to float -- at one float pattern in 1009 across the whole range, at every
* float in [3/4, 3/2) for atan_c, where its reduction's last interval and its
* inversion meet, and at every float about pi/2 and pi for tan_c, where the
* tangent passes its pole and its zero.
*
*   For double, four million random arguments each: tan_c below 2^21 and
* from there to the largest double, atan_c from 2^-60 to 2^60; both within
* an ulp of tan and atan, the library's, and how many differ at all is
* printed. The edges must be exact against values from a 400-bit
* computation: the tangent next to its pole, the arc-tangent at its table's
* boundaries and past 2^60, and the arguments whose long double result lands
* on a double's rounding midpoint, where converting it would round the wrong
* way. For long double, a million arguments each, within two ulps of tanl
* (glibc's own is not always within one) and one of atanl.
*
*   atan2_c, the two-argument arc-tangent: for float, the correctly rounded
* result at a million random pairs, half of them random patterns (NaN, the
* infinities and the subnormals among them) and half within twenty binades
* of each other; for double, within an ulp of atan2 at two million pairs, a
* quarter of them from the whole range, and exact at the quadrants'
* boundaries, at the range's extremes and where the long double result lands
* on a midpoint; for long double, within an ulp of atan2l at half a million.
* Every special case of C99's Annex F, the signs of zero among them, for
* every family.
*
*   With the argument "exhaustive" it checks every float, all 2^32 patterns
* (tan_c and atan_c; its count wants a 64-bit long). Compiled as C++, it
* checks the overloads; from C++14, that all three functions are constant
* expressions, for every family, with the bits they have at run time. It
* needs no 64-bit integer -- its random numbers come from
* math_tests_random.h, the same in every build -- so that ISO-strict C++98
* on a 32-bit target, where dj_math_umax has 32 bits, builds it too.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_tangent.c -o t_tangent -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_tangent.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.09
*                                                            revised: 2026.11.07
*******************************************************************************/
// std
#include <float.h>   // FLT_MAX, DBL_MAX, LDBL_MAX, LDBL_MANT_DIG
#include <math.h>    // tanf ... atanl, atan2f, atan2, atan2l, nextafter
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/tangent.h"  // unit under test
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
// tests
#include "./math_tests_random.h"  // d_tests_seed, d_tests_below ...


// the C library's functions, called through volatile pointers, so that no
// compiler substitutes its own evaluation (an x87's, at a 32-bit target)
static float       (*volatile g_tanf)(float)              = tanf;
static double      (*volatile g_tan)(double)              = tan;
static long double (*volatile g_tanl)(long double)        = tanl;
static float       (*volatile g_atanf)(float)             = atanf;
static double      (*volatile g_atan)(double)             = atan;
static long double (*volatile g_atanl)(long double)       = atanl;
static float       (*volatile g_atan2f)(float, float)     = atan2f;
static double      (*volatile g_atan2)(double, double)    = atan2;
static long double (*volatile g_atan2l)(long double,
                                        long double)      = atan2l;

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

// d_tests_expect_pair
//   d_tests_expect for a function of two arguments: a failure reports both.
static void
d_tests_expect_pair(
    bool        _ok,
    const char* _what,
    long double _y,
    long double _x,
    int         _line
)
{
    ++g_checks;

    if (!_ok)
    {
        ++g_failures;

        if (g_failures <= 40)
        {
            printf("  FAIL line %d: %s (%.21Lg, %.21Lg)\n",
                   _line, _what, _y, _x);
        }
    }

    return;
}

#define D_TESTS_EXPECT_PAIR(ok, what, y, x)                                   \
    d_tests_expect_pair((ok), (what), (long double)(y), (long double)(x),     \
                        __LINE__)

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

// d_tests_float_one
//   one float: tan_c and atan_c against the long double library's result,
// rounded to float, which is the correctly rounded one wherever long double
// has eleven bits more than float; and within an ulp of tanf and atanf.
static void
d_tests_float_one(float _x, bool _tan, bool _atan)
{
    if (_tan)
    {
        const float got = d_math_f_tan_c(_x);

        D_TESTS_EXPECT(d_tests_same_f(got, (float)g_tanl(_x)),
                       "f tan_c correctly rounded", _x);
        D_TESTS_EXPECT(D_TESTS_ULPS_F(got, g_tanf(_x)) <= 1.0,
                       "f tan_c within an ulp of tanf", _x);
    }

    if (_atan)
    {
        const float got = d_math_f_atan_c(_x);

        D_TESTS_EXPECT(d_tests_same_f(got, (float)g_atanl(_x)),
                       "f atan_c correctly rounded", _x);
        D_TESTS_EXPECT(D_TESTS_ULPS_F(got, g_atanf(_x)) <= 1.0,
                       "f atan_c within an ulp of atanf", _x);
    }

    return;
}

static void
d_tests_float(bool _exhaustive)
{
    const uint32_t step = _exhaustive ? 1u : 1009u;
    uint32_t       bits = 0u;
    float          x    = 0.0f;

    // one pattern in 1009 across all of them, or every one
    for (bits = 0u; ; bits += step)
    {
        float f = 0.0f;

        memcpy(&f, &bits, sizeof(f));
        d_tests_float_one(f, true, true);

        // to the last pattern a step reaches below 2^32
        if (bits > (0xFFFFFFFFu - step))
        {
            break;
        }
    }

    // every float in [3/4, 3/2): the arc-tangent's last interval, [7/8, 1],
    // and the first past its inversion, (1, 8/7]
    for (x = 0.75f; x < 1.5f; x = nextafterf(x, 2.0f))
    {
        d_tests_float_one(x, false, true);
    }

    // every float about pi/2, the tangent's pole, and about pi, its zero
    for (x = 1.5f; x < 1.65f; x = nextafterf(x, 2.0f))
    {
        d_tests_float_one(x, true, false);
    }

    for (x = 3.1f; x < 3.2f; x = nextafterf(x, 4.0f))
    {
        d_tests_float_one(x, true, false);
    }

    return;
}

// d_tests_double
//   tan_c and atan_c within an ulp of tan and atan over random arguments;
// prints how many differ at all, and the largest difference.
static void
d_tests_double(void)
{
    double worst_tan  = 0.0;
    double worst_atan = 0.0;
    long   diff_tan   = 0;
    long   diff_atan  = 0;
    long   i          = 0;

    for (i = 0; i < 4000000; ++i)
    {
        const double sign = (d_tests_bit() != 0u) ? -1.0 : 1.0;
        // tan: every binade from 2^-3 to 2^20, and from 2^21 to the largest
        const int    t_exp = ((i % 2) == 0)
                                 ? ((int)(i % 24) - 3)
                                 : (21 + (int)d_tests_below(1003u));
        const double t = sign * ldexp(1.0 + d_tests_unit(), t_exp);
        // atan: every binade from 2^-60 to 2^60
        const double a = sign * ldexp(1.0 + d_tests_unit(),
                                      (int)d_tests_below(121u) - 60);
        double       e = 0.0;

        e = D_TESTS_ULPS_D(d_math_d_tan_c(t), g_tan(t));
        worst_tan = (e > worst_tan) ? e : worst_tan;
        diff_tan += (e > 0.0) ? 1 : 0;
        D_TESTS_EXPECT(e <= 1.0, "d tan_c within an ulp of tan", t);

        e = D_TESTS_ULPS_D(d_math_d_atan_c(a), g_atan(a));
        worst_atan = (e > worst_atan) ? e : worst_atan;
        diff_atan += (e > 0.0) ? 1 : 0;
        D_TESTS_EXPECT(e <= 1.0, "d atan_c within an ulp of atan", a);
    }

    printf("double: tan_c differs from tan at %ld of 4000000 arguments, by "
           "%.3g ulps at most; atan_c from atan at %ld, by %.3g\n",
           diff_tan, worst_tan, diff_atan, worst_atan);

    return;
}

// d_tests_double_edges
//   the correctly rounded values, from a 400-bit computation (the slice's
// tools/edge_values.py): the tangent about its pole and at huge arguments,
// the arc-tangent at its table's boundaries and far out, and the arguments
// whose result, rounded to long double, lies on a double's midpoint.
static void
d_tests_double_edges(void)
{
    // x and tan(x), correctly rounded
    static const double tans[][2] = {
        // the double nearest pi/2, and 3 pi/2: next to the pole
        { 1.5707963267948966,     1.633123935319537e+16   },
        { 4.71238898038469,       5443746451065123.0      },
        // the double nearest pi/4 lies below it
        { 0.7853981633974483,     0.9999999999999999      },
        { 3.141592653589793,     -1.2246467991473532e-16  },
        { 1.0,                    1.5574077246549023      },
        { -2.0,                   2.185039863261519       },
        { 355.0,                  3.0144353373184265e-05  },
        { 1e6,                   -0.373624453987599       },
        // past 2^20 quarter turns: the exact reduction
        { 1e15,                  -1.672414782127583       },
        { 1e22,                  -1.6287782256068988      },
        { 1e300,                  1.4214488238747245      },
        { 1.7976931348623157e+308, -0.004962015874444895  },
        // the double nearest a multiple of pi/2 (6381956970095103 2^797,
        // to 25 digits: its 16 lie so near a midpoint that a long double
        // evaluation of the literal rounds it to the double below), and two
        // below 2^20 quarter turns but within 2^-53 of one
        { 5.319372648326541416707297e+255, -2.133485385753704e+18 },
        { 45.553093477052,        -1.6155594228467482e+18 },
        { 321307.9594422229,       2.2575397588644696e+16 },
        // the long double result lies on a double's midpoint
        { 1.3538224420202866e-08, 1.3538224420202866e-08  },
        { 1.3540445649613374e-08, 1.3540445649613376e-08  },
        { 5.300200437809995e-08,  5.3002004378100005e-08  },
        { 4.111064862837294e-08,  4.111064862837296e-08   }
    };
    // x and atan(x), correctly rounded
    static const double atans[][2] = {
        // the reduction's boundaries, and the doubles below them
        { 0.125,                  0.12435499454676144     },
        { 0.12499999999999999,    0.12435499454676142     },
        { 0.375,                  0.35877067027057225     },
        { 0.37499999999999994,    0.3587706702705722      },
        { 0.625,                  0.5585993153435624      },
        { 0.6249999999999999,     0.5585993153435623      },
        { 0.875,                  0.7188299996216245      },
        { 0.8749999999999999,     0.7188299996216244      },
        { 1.0,                    0.7853981633974483      },
        { 1.0000000000000002,     0.7853981633974484      },
        // and their reciprocals, past the inversion
        { 1.1428571428571428,     0.851966327173272       },
        { 1.6,                    1.0121970114513341      },
        { 2.6666666666666665,     1.2120256565243244      },
        { 8.0,                    1.446441332248135       },
        { -3.0,                  -1.2490457723982544      },
        // far out: pi/2, rounded
        { 1.152921504606847e+18,  1.5707963267948966      },
        { 1e300,                  1.5707963267948966      },
        { 1.7976931348623157e+308, 1.5707963267948966     },
        { 1e-300,                 1e-300                  },
        // the long double result lies on a double's midpoint
        { 1.3538224420202866e-08, 1.3538224420202866e-08  },
        { 1.3540445649613374e-08, 1.3540445649613372e-08  },
        { 5.300200437809995e-08,  5.30020043780999e-08    },
        { 4.111064862837294e-08,  4.111064862837292e-08   }
    };
    size_t i = 0;

    for (i = 0; i < (sizeof(tans) / sizeof(tans[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_tan_c(tans[i][0]), tans[i][1]),
                       "d tan_c correctly rounded at an edge", tans[i][0]);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_tan_c(-tans[i][0]),
                                      -tans[i][1]),
                       "d tan_c odd at an edge", tans[i][0]);
    }

    for (i = 0; i < (sizeof(atans) / sizeof(atans[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_atan_c(atans[i][0]),
                                      atans[i][1]),
                       "d atan_c correctly rounded at an edge", atans[i][0]);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_atan_c(-atans[i][0]),
                                      -atans[i][1]),
                       "d atan_c odd at an edge", atans[i][0]);
    }

    // the multiples of pi/2, where the reduction must lose nothing: tan_c
    // within an ulp of tan
    for (i = 1; i < 200000; ++i)
    {
        // stored, so x is a double: clang on x87 hands an inlined kernel
        // the product's 64-bit register value otherwise
        volatile double stored = (double)i * 1.5707963267948966;
        const double    x      = stored;

        D_TESTS_EXPECT(D_TESTS_ULPS_D(d_math_d_tan_c(x), g_tan(x)) <= 1.0,
                       "d tan_c at a multiple of pi/2", x);
    }

    return;
}

static void
d_tests_long_double(void)
{
    double worst_tan  = 0.0;
    double worst_atan = 0.0;
    long   i          = 0;

    for (i = 0; i < 1000000; ++i)
    {
        const long double sign = (d_tests_bit() != 0u) ? -1.0L : 1.0L;
        // every bit random: two doubles' worth
        const long double m = 1.0L + (long double)d_tests_unit() +
                              ((long double)d_tests_unit() * LDBL_EPSILON);
        // tan: from 2^-3 to 2^20, and from 2^21 to the largest
        const int         t_exp =
            ((i % 2) == 0)
                ? ((int)(i % 24) - 3)
                : (21 + (int)d_tests_below((unsigned long)(LDBL_MAX_EXP - 22)));
        const long double t = sign * ldexpl(m, t_exp);
        // atan: from 2^-60 to 2^60
        const long double a = sign * ldexpl(m, (int)d_tests_below(121u) - 60);
        double            e = 0.0;

        e = D_TESTS_ULPS_LD(d_math_ld_tan_c(t), g_tanl(t));
        worst_tan = (e > worst_tan) ? e : worst_tan;
        D_TESTS_EXPECT(e <= 2.0, "ld tan_c within two ulps of tanl", t);

        e = D_TESTS_ULPS_LD(d_math_ld_atan_c(a), g_atanl(a));
        worst_atan = (e > worst_atan) ? e : worst_atan;
        D_TESTS_EXPECT(e <= 1.0, "ld atan_c within an ulp of atanl", a);
    }

    // the largest long double, both ways
    D_TESTS_EXPECT(D_TESTS_ULPS_LD(d_math_ld_tan_c(LDBL_MAX),
                                   g_tanl(LDBL_MAX)) <= 2.0,
                   "ld tan_c(LDBL_MAX)", 0);
    D_TESTS_EXPECT(D_TESTS_ULPS_LD(d_math_ld_atan_c(LDBL_MAX),
                                   g_atanl(LDBL_MAX)) <= 1.0,
                   "ld atan_c(LDBL_MAX)", 0);

    printf("long double: tan_c within %.3g ulps of tanl, atan_c within %.3g "
           "of atanl\n", worst_tan, worst_atan);

    return;
}

// d_tests_specials
//   NaN, the infinities, the zeros, the subnormals, every family.
static void
d_tests_specials(void)
{
    const double inf  = (double)HUGE_VAL;
    const double tiny = 4.9406564584124654e-324;

    // tan: NaN and the infinities to NaN, either zero to itself, and a
    // subnormal to itself
    D_TESTS_EXPECT( (isnan(d_math_f_tan_c(nanf("")))) &&
                    (isnan(d_math_d_tan_c(nan("")))) &&
                    (isnan(d_math_ld_tan_c(nanl("")))) &&
                    (isnan(d_math_d_tan_c(inf))) &&
                    (isnan(d_math_d_tan_c(-inf))) &&
                    (isnan(d_math_f_tan_c((float)inf))) &&
                    (isnan(d_math_ld_tan_c(-(long double)inf))),
                    "tan_c of NaN and the infinities is NaN", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_tan_c(0.0), 0.0)) &&
                    (d_tests_same_d(d_math_d_tan_c(-0.0), -0.0)) &&
                    (d_tests_same_f(d_math_f_tan_c(-0.0f), -0.0f)) &&
                    (d_tests_same_ld(d_math_ld_tan_c(-0.0L), -0.0L)) &&
                    (d_tests_same_d(d_math_d_tan_c(tiny), tiny)) &&
                    (d_tests_same_d(d_math_d_tan_c(-tiny), -tiny)) &&
                    (d_tests_same_f(d_math_f_tan_c(1e-40f), 1e-40f)),
                    "tan_c of the zeros and the subnormals", 0);

    // atan: NaN to NaN, the infinities to +-pi/2, either zero to itself, a
    // subnormal to itself
    D_TESTS_EXPECT( (isnan(d_math_f_atan_c(nanf("")))) &&
                    (isnan(d_math_d_atan_c(nan("")))) &&
                    (isnan(d_math_ld_atan_c(nanl("")))),
                    "atan_c(NaN) is NaN", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_atan_c(inf),
                                    1.5707963267948966)) &&
                    (d_tests_same_d(d_math_d_atan_c(-inf),
                                    -1.5707963267948966)) &&
                    (d_tests_same_f(d_math_f_atan_c((float)inf),
                                    1.57079637f)) &&
                    (d_tests_same_f(d_math_f_atan_c(-(float)inf),
                                    -1.57079637f)) &&
                    (d_tests_same_ld(d_math_ld_atan_c((long double)inf),
                                     g_atanl((long double)inf))),
                    "atan_c of the infinities is +-pi/2", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_atan_c(0.0), 0.0)) &&
                    (d_tests_same_d(d_math_d_atan_c(-0.0), -0.0)) &&
                    (d_tests_same_f(d_math_f_atan_c(-0.0f), -0.0f)) &&
                    (d_tests_same_ld(d_math_ld_atan_c(-0.0L), -0.0L)) &&
                    (d_tests_same_d(d_math_d_atan_c(tiny), tiny)) &&
                    (d_tests_same_d(d_math_d_atan_c(-tiny), -tiny)) &&
                    (d_tests_same_f(d_math_f_atan_c(-1e-40f), -1e-40f)),
                    "atan_c of the zeros and the subnormals", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_f_atan_c(1.0f), 0.785398185f)) &&
                    (d_tests_same_ld(d_math_ld_atan_c(1.0L),
                                     g_atanl(1.0L))),
                    "atan_c(1) is pi/4, rounded", 0);

    // the plain forms are the C library's
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_tan(1.5), g_tan(1.5)), "d tan", 0);
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_atan(3.0f), g_atanf(3.0f)),
                   "f atan", 0);

    return;
}

// d_tests_atan2_float
//   atan2_c for float against the long double library's atan2l, rounded to
// float -- the correctly rounded result -- and within an ulp of atan2f, at a
// million pairs: every other one two random patterns (NaN, the infinities,
// the zeros and the subnormals among them, and mostly magnitudes so far
// apart that the result is y/x, +-pi/2 or +-pi), the rest within twenty
// binades of each other.
static void
d_tests_atan2_float(void)
{
    long i = 0;

    for (i = 0; i < 1000000; ++i)
    {
        float y = 0.0f;
        float x = 0.0f;
        float got = 0.0f;

        if ((i % 2) == 0)
        {
            const uint32_t by = d_tests_low32();
            const uint32_t bx = d_tests_low32();

            memcpy(&y, &by, sizeof(y));
            memcpy(&x, &bx, sizeof(x));
        }
        else
        {
            const double sy = (d_tests_bit() != 0u) ? -1.0 : 1.0;
            const double sx = (d_tests_bit() != 0u) ? -1.0 : 1.0;
            const int    ex = (int)d_tests_below(61u) - 30;
            const int    ey = ex + (int)d_tests_below(41u) - 20;

            y = (float)(sy * ldexp(1.0 + d_tests_unit(), ey));
            x = (float)(sx * ldexp(1.0 + d_tests_unit(), ex));
        }

        got = d_math_f_atan2_c(y, x);
        D_TESTS_EXPECT_PAIR(d_tests_same_f(got, (float)g_atan2l(y, x)),
                            "f atan2_c correctly rounded", y, x);
        D_TESTS_EXPECT_PAIR(D_TESTS_ULPS_F(got, g_atan2f(y, x)) <= 1.0,
                            "f atan2_c within an ulp of atan2f", y, x);
    }

    return;
}

// d_tests_atan2_double
//   atan2_c within an ulp of atan2 at two million random pairs, any signs:
// three in four within thirty binades of each other, x from 2^-60 to 2^60,
// and every fourth from the whole range, the subnormals included. Prints
// how many differ at all, and the largest difference.
static void
d_tests_atan2_double(void)
{
    double worst = 0.0;
    long   diff  = 0;
    long   i     = 0;

    for (i = 0; i < 2000000; ++i)
    {
        const double sy   = (d_tests_bit() != 0u) ? -1.0 : 1.0;
        const double sx   = (d_tests_bit() != 0u) ? -1.0 : 1.0;
        const bool   wide = ((i % 4) == 3);
        const int    ex   = wide ? ((int)d_tests_below(2098u) - 1074)
                                 : ((int)d_tests_below(121u) - 60);
        const int    ey   = wide ? ((int)d_tests_below(2098u) - 1074)
                                 : (ex + (int)d_tests_below(61u) - 30);
        const double y    = sy * ldexp(1.0 + d_tests_unit(), ey);
        const double x    = sx * ldexp(1.0 + d_tests_unit(), ex);
        const double e    = D_TESTS_ULPS_D(d_math_d_atan2_c(y, x),
                                           g_atan2(y, x));

        worst = (e > worst) ? e : worst;
        diff += (e > 0.0) ? 1 : 0;
        D_TESTS_EXPECT_PAIR(e <= 1.0, "d atan2_c within an ulp of atan2",
                            y, x);
    }

    printf("double: atan2_c differs from atan2 at %ld of 2000000 pairs, by "
           "%.3g ulps at most\n", diff, worst);

    return;
}

// d_tests_atan2_edges
//   the correctly rounded values, from a 400-bit computation (the slice's
// tools/edge_values5.py): the quadrants' boundaries and the octants', the
// range's extremes, where the quotient is subnormal or vanishes, and the
// pairs whose result, rounded to long double, lies on a double's midpoint;
// each with y negated too, which negates the result.
static void
d_tests_atan2_edges(void)
{
    // y, x and atan2(y, x), correctly rounded
    static const double edges[][3] = {
        // the octants' boundaries, pi/4 and 3 pi/4, and about them
        { 1.0,                  1.0,                  0.7853981633974483     },
        { 1.0,                 -1.0,                  2.356194490192345      },
        { 0.1,                  0.1000000000000001,   0.7853981633974478     },
        { 1.0,                  2.0,                  0.4636476090008061     },
        { 2.0,                  1.0,                  1.1071487177940904     },
        { 3.0,                  4.0,                  0.6435011087932844     },
        { 5.0,                 -12.0,                 2.746801533890032      },
        // y/x the double nearest sqrt(3): pi/3, rounded
        { 1.0,                  0.5773502691896257,   1.0471975511965979     },
        // next to the negative x axis: a little less than pi
        { 1e-10,               -1.0,                  3.141592653489793      },
        // the range's extremes, where the quotient vanishes, overflows or is
        // subnormal; 5e-324 is the least subnormal
        { 1e-300,               1e+300,               0.0                    },
        { 1e+300,               1e-300,               1.5707963267948966     },
        { DBL_MAX,              DBL_MAX,              0.7853981633974483     },
        { DBL_MAX,             -DBL_MAX,              2.356194490192345      },
        { 5e-324,               1.0,                  5e-324                 },
        { 1.0,                  5e-324,               1.5707963267948966     },
        { 5e-324,              -1.0,                  3.141592653589793      },
        { 5e-324,               5e-324,               0.7853981633974483     },
        { 5e-324,              -5e-324,               2.356194490192345      },
        { 5e-324,               DBL_MAX,              0.0                    },
        { DBL_MIN,              3.0,                  7.41691286169067e-309  },
        // the long double result lies on a double's midpoint
        { 7.220715161495508e-09,  1.9147291546910938, 3.771141805515797e-09  },
        { 4.172281616093229e-08,  1.5559283912167312, 2.6815383276286367e-08 },
        { 1.3243945896326575e-07, 1.3167433966445126, 1.0058106940256076e-07 },
        { 7.627730920295016e-09,  1.3045627461096219, 5.846963622901172e-09  },
        { 2.8493342539993373e-07, 1.843893807571157,  1.5452811014928023e-07 },
        { 6.664565369844864e-08,  1.2666312368085826, 5.261646149385173e-08  }
    };
    size_t i = 0;

    for (i = 0; i < (sizeof(edges) / sizeof(edges[0])); ++i)
    {
        D_TESTS_EXPECT_PAIR(
            d_tests_same_d(d_math_d_atan2_c(edges[i][0], edges[i][1]),
                           edges[i][2]),
            "d atan2_c correctly rounded at an edge", edges[i][0],
            edges[i][1]);
        D_TESTS_EXPECT_PAIR(
            d_tests_same_d(d_math_d_atan2_c(-edges[i][0], edges[i][1]),
                           -edges[i][2]),
            "d atan2_c odd in y at an edge", edges[i][0], edges[i][1]);
    }

    return;
}

// d_tests_atan2_long_double
//   atan2_c within an ulp of atan2l at half a million random pairs, every
// bit random: three in four within thirty binades of each other, every
// fourth from the whole range, the subnormals included.
static void
d_tests_atan2_long_double(void)
{
    // every exponent a long double's least subnormal and largest value span
    const unsigned long span  = (unsigned long)(LDBL_MAX_EXP - LDBL_MIN_EXP +
                                                LDBL_MANT_DIG);
    double              worst = 0.0;
    long                i     = 0;

    for (i = 0; i < 500000; ++i)
    {
        const long double sy   = (d_tests_bit() != 0u) ? -1.0L : 1.0L;
        const long double sx   = (d_tests_bit() != 0u) ? -1.0L : 1.0L;
        const long double my   = 1.0L + (long double)d_tests_unit() +
                                 ((long double)d_tests_unit() * LDBL_EPSILON);
        const long double mx   = 1.0L + (long double)d_tests_unit() +
                                 ((long double)d_tests_unit() * LDBL_EPSILON);
        const bool        wide = ((i % 4) == 3);
        const int         ex   =
            wide ? ((int)d_tests_below(span) +
                    (LDBL_MIN_EXP - LDBL_MANT_DIG))
                 : ((int)d_tests_below(121u) - 60);
        const int         ey   =
            wide ? ((int)d_tests_below(span) +
                    (LDBL_MIN_EXP - LDBL_MANT_DIG))
                 : (ex + (int)d_tests_below(61u) - 30);
        const long double y    = sy * ldexpl(my, ey);
        const long double x    = sx * ldexpl(mx, ex);
        const double      e    = D_TESTS_ULPS_LD(d_math_ld_atan2_c(y, x),
                                                 g_atan2l(y, x));

        worst = (e > worst) ? e : worst;
        D_TESTS_EXPECT_PAIR(e <= 1.0, "ld atan2_c within an ulp of atan2l",
                            y, x);
    }

    printf("long double: atan2_c within %.3g ulps of atan2l\n", worst);

    return;
}

// d_tests_atan2_specials
//   C99's Annex F, for every family: NaN in either argument gives NaN; a
// zero y gives a zero toward a positive x and pi toward a negative one, the
// sign of a zero x deciding; a zero x gives pi/2; a finite y gives a zero
// toward +infinity and pi toward -infinity; an infinite y gives pi/2, pi/4
// or 3 pi/4; and the result has y's sign, so that every row below is
// checked with y negated too. Float and double against pi's multiples,
// rounded; long double against atan2l.
static void
d_tests_atan2_specials(void)
{
    // y, x, x's sign and the result, as kinds: an argument 0 for a zero,
    // 1 the least subnormal, 2 one, 3 the largest finite value, 4 infinity,
    // 5 NaN; a result 0 for a zero, 1 pi/4, 2 pi/2, 3 3 pi/4, 4 pi, 5 NaN
    static const int rows[][4] = {
        { 0, 0,  1, 0 },  { 0, 0, -1, 4 },  { 0, 1, -1, 4 },
        { 0, 2, -1, 4 },  { 0, 4, -1, 4 },  { 0, 1,  1, 0 },
        { 0, 2,  1, 0 },  { 0, 4,  1, 0 },  { 1, 0,  1, 2 },
        { 1, 0, -1, 2 },  { 2, 0,  1, 2 },  { 4, 0, -1, 2 },
        { 2, 4, -1, 4 },  { 3, 4, -1, 4 },  { 2, 4,  1, 0 },
        { 3, 4,  1, 0 },  { 4, 2,  1, 2 },  { 4, 3, -1, 2 },
        { 4, 4, -1, 3 },  { 4, 4,  1, 1 },  { 5, 2,  1, 5 },
        { 2, 5,  1, 5 },  { 5, 5,  1, 5 },  { 4, 5,  1, 5 },
        { 5, 4, -1, 5 },  { 0, 5,  1, 5 },  { 5, 0, -1, 5 }
    };
    const float       f_arg[6] = { 0.0f, FLT_MIN * FLT_EPSILON, 1.0f, FLT_MAX,
                                   (float)HUGE_VAL, nanf("") };
    const float       f_res[6] = { 0.0f, 0.785398185f, 1.57079637f,
                                   2.35619450f, 3.14159274f, nanf("") };
    const double      d_arg[6] = { 0.0, DBL_MIN * DBL_EPSILON, 1.0, DBL_MAX,
                                   (double)HUGE_VAL, nan("") };
    const double      d_res[6] = { 0.0, 0.7853981633974483,
                                   1.5707963267948966, 2.356194490192345,
                                   3.141592653589793, nan("") };
    const long double l_arg[6] = { 0.0L, LDBL_MIN * LDBL_EPSILON, 1.0L,
                                   LDBL_MAX, (long double)HUGE_VAL,
                                   nanl("") };
    size_t i = 0;

    for (i = 0; i < (sizeof(rows) / sizeof(rows[0])); ++i)
    {
        const float       yf = f_arg[rows[i][0]];
        const float       xf = (float)rows[i][2] * f_arg[rows[i][1]];
        const float       rf = f_res[rows[i][3]];
        const double      yd = d_arg[rows[i][0]];
        const double      xd = (double)rows[i][2] * d_arg[rows[i][1]];
        const double      rd = d_res[rows[i][3]];
        const long double yl = l_arg[rows[i][0]];
        const long double xl = (long double)rows[i][2] * l_arg[rows[i][1]];

        D_TESTS_EXPECT( (d_tests_same_f(d_math_f_atan2_c(yf, xf), rf)) &&
                        (d_tests_same_f(d_math_f_atan2_c(-yf, xf), -rf)),
                        "f atan2_c's special cases (row)", i);
        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_atan2_c(yd, xd), rd)) &&
                        (d_tests_same_d(d_math_d_atan2_c(-yd, xd), -rd)),
                        "d atan2_c's special cases (row)", i);
        D_TESTS_EXPECT( (d_tests_same_ld(d_math_ld_atan2_c(yl, xl),
                                         g_atan2l(yl, xl))) &&
                        (d_tests_same_ld(d_math_ld_atan2_c(-yl, xl),
                                         g_atan2l(-yl, xl))),
                        "ld atan2_c's special cases (row)", i);
    }

    // the plain forms are the C library's
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_atan2(1.5, -2.0),
                                  g_atan2(1.5, -2.0)),
                   "d atan2", 0);
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_atan2(-1.5f, 2.0f),
                                  g_atan2f(-1.5f, 2.0f)),
                   "f atan2", 0);

    return;
}

#if defined(__cplusplus)

// d_tests_overloads
//   compiled as C++: one name for every family, chosen by argument type.
static void
d_tests_overloads(void)
{
    D_TESTS_EXPECT( (d_tests_same_f(d_math_tan_c(0.25f),
                                    d_math_f_tan_c(0.25f))) &&
                    (d_tests_same_d(d_math_tan_c(0.25),
                                    d_math_d_tan_c(0.25))) &&
                    (d_tests_same_ld(d_math_tan_c(0.25L),
                                     d_math_ld_tan_c(0.25L))) &&
                    (d_tests_same_f(d_math_atan_c(3.0f),
                                    d_math_f_atan_c(3.0f))) &&
                    (d_tests_same_d(d_math_atan_c(3.0),
                                    d_math_d_atan_c(3.0))) &&
                    (d_tests_same_ld(d_math_atan_c(3.0L),
                                     d_math_ld_atan_c(3.0L))) &&
                    (d_tests_same_d(d_math_tan(0.25), g_tan(0.25))) &&
                    (d_tests_same_d(d_math_atan(3.0), g_atan(3.0))),
                    "the C++ overloads are the families' functions", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_atan2_c(1.0f, -3.0f),
                                    d_math_f_atan2_c(1.0f, -3.0f))) &&
                    (d_tests_same_d(d_math_atan2_c(1.0, -3.0),
                                    d_math_d_atan2_c(1.0, -3.0))) &&
                    (d_tests_same_ld(d_math_atan2_c(1.0L, -3.0L),
                                     d_math_ld_atan2_c(1.0L, -3.0L))) &&
                    (d_tests_same_f(d_math_atan2(1.0f, -3.0f),
                                    g_atan2f(1.0f, -3.0f))) &&
                    (d_tests_same_d(d_math_atan2(1.0, -3.0),
                                    g_atan2(1.0, -3.0))),
                    "the C++ overloads of atan2", 0);

    return;
}

#endif  // __cplusplus

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// from C++14, all three are constant expressions: every family, the edges
// and the specials among them
static constexpr float       g_ctan_f    = d_math_f_tan_c(1.0f);
static constexpr double      g_ctan_d    = d_math_d_tan_c(1.0);
static constexpr long double g_ctan_ld   = d_math_ld_tan_c(1.0L);
static constexpr float       g_catan_f   = d_math_f_atan_c(10.0f);
static constexpr double      g_catan_d   = d_math_d_atan_c(10.0);
static constexpr long double g_catan_ld  = d_math_ld_atan_c(10.0L);
static constexpr double      g_ctan_pole = d_math_d_tan_c(1.5707963267948966);
static constexpr double      g_ctan_far  = d_math_d_tan_c(1e300);
static constexpr double      g_ctan_max  =
    d_math_d_tan_c(1.7976931348623157e+308);
static constexpr long double g_ctan_ldmax = d_math_ld_tan_c(LDBL_MAX);
static constexpr double      g_ctan_mid  =
    d_math_d_tan_c(1.3538224420202866e-08);
static constexpr double      g_catan_mid =
    d_math_d_atan_c(1.3540445649613374e-08);
static constexpr double      g_catan_inf = d_math_d_atan_c((double)HUGE_VAL);
static constexpr double      g_catan_sub =
    d_math_d_atan_c(4.9406564584124654e-324);
static constexpr float       g_catan2_f  = d_math_f_atan2_c(1.0f, -2.0f);
static constexpr double      g_catan2_d  = d_math_d_atan2_c(1.0, -2.0);
static constexpr long double g_catan2_ld = d_math_ld_atan2_c(1.0L, -2.0L);
static constexpr double      g_catan2_mid =
    d_math_d_atan2_c(7.220715161495508e-09, 1.9147291546910938);
static constexpr double      g_catan2_sub = d_math_d_atan2_c(DBL_MIN, 3.0);
static constexpr double      g_catan2_inf =
    d_math_d_atan2_c(-(double)HUGE_VAL, -(double)HUGE_VAL);

#if ( (defined(D_ENV_COMPILER_GCC)) ||                                         \
      (defined(D_ENV_COMPILER_CLANG)) )

// a zero argument's sign is read by a builtin, a constant expression for
// GCC and Clang; elsewhere atan2_c of a zero is evaluated at run time
static constexpr double      g_catan2_zero = d_math_d_atan2_c(-0.0, -0.0);

static_assert(d_math_d_atan2_c(0.0, -0.0) == (double)3.141592653589793,
              "atan2_c(+0, -0) is pi");
static_assert(d_math_d_atan2_c(-1.0, 0.0) == (double)-1.5707963267948966,
              "atan2_c(-1, +0) is -pi/2");

#endif  // GCC or Clang

static_assert(d_math_d_tan_c(0.0) == 0.0, "tan_c(0) is 0");
static_assert(d_math_d_atan_c(0.0) == 0.0, "atan_c(0) is 0");
static_assert(d_math_d_atan_c(1.0) == (double)0.7853981633974483,
              "atan_c(1)");
static_assert(d_math_d_tan_c(1e22) == (double)-1.6287782256068988,
              "tan_c(1e22)");
static_assert(d_math_d_tan_c((double)HUGE_VAL) !=
              d_math_d_tan_c((double)HUGE_VAL), "tan_c(inf) is NaN");
static_assert(d_math_atan_c(2.0f) > (float)1.107f,
              "the C++ overloads, float");
static_assert(d_math_d_atan2_c(1.0, 1.0) == (double)0.7853981633974483,
              "atan2_c(1, 1) is pi/4");
static_assert(d_math_d_atan2_c(1e-10, -1.0) == (double)3.141592653489793,
              "atan2_c(1e-10, -1)");
static_assert(d_math_d_atan2_c((double)HUGE_VAL, -(double)HUGE_VAL) ==
              (double)2.356194490192345, "atan2_c(inf, -inf) is 3 pi/4");
static_assert(d_math_atan2_c(2.0f, 1.0f) > (float)1.107f,
              "the C++ overloads of atan2, float");

// d_tests_constant_evaluation
//   the constants are the run time's results, bit for bit: the arguments
// are read through volatiles, so the calls below are not folded.
static void
d_tests_constant_evaluation(void)
{
    volatile float       one_f  = 1.0f;
    volatile double      one_d  = 1.0;
    volatile long double one_ld = 1.0L;
    volatile float       ten_f  = 10.0f;
    volatile double      ten_d  = 10.0;
    volatile long double ten_ld = 10.0L;
    volatile double      pole   = 1.5707963267948966;
    volatile double      far    = 1e300;
    volatile double      top    = 1.7976931348623157e+308;
    volatile long double ldtop  = LDBL_MAX;
    volatile double      mid_t  = 1.3538224420202866e-08;
    volatile double      mid_a  = 1.3540445649613374e-08;
    volatile double      inf    = (double)HUGE_VAL;
    volatile double      least  = 4.9406564584124654e-324;
    volatile float       mtwo_f = -2.0f;
    volatile double      mtwo_d = -2.0;
    volatile long double mtwo_l = -2.0L;
    volatile double      mid_y  = 7.220715161495508e-09;
    volatile double      mid_x  = 1.9147291546910938;
    volatile double      dmin   = DBL_MIN;
    volatile double      three  = 3.0;

    D_TESTS_EXPECT( (d_tests_same_f(g_ctan_f, d_math_f_tan_c(one_f))) &&
                    (d_tests_same_d(g_ctan_d, d_math_d_tan_c(one_d))) &&
                    (d_tests_same_ld(g_ctan_ld, d_math_ld_tan_c(one_ld))) &&
                    (d_tests_same_f(g_catan_f, d_math_f_atan_c(ten_f))) &&
                    (d_tests_same_d(g_catan_d, d_math_d_atan_c(ten_d))) &&
                    (d_tests_same_ld(g_catan_ld,
                                     d_math_ld_atan_c(ten_ld))) &&
                    (d_tests_same_d(g_ctan_pole, d_math_d_tan_c(pole))) &&
                    (d_tests_same_d(g_ctan_far, d_math_d_tan_c(far))) &&
                    (d_tests_same_d(g_ctan_max, d_math_d_tan_c(top))) &&
                    (d_tests_same_ld(g_ctan_ldmax,
                                     d_math_ld_tan_c(ldtop))) &&
                    (d_tests_same_d(g_ctan_mid, d_math_d_tan_c(mid_t))) &&
                    (d_tests_same_d(g_catan_mid, d_math_d_atan_c(mid_a))) &&
                    (d_tests_same_d(g_catan_inf, d_math_d_atan_c(inf))) &&
                    (d_tests_same_d(g_catan_sub,
                                    d_math_d_atan_c(least))),
                    "constant evaluation agrees with the run time", 0);
    D_TESTS_EXPECT( (d_tests_same_f(g_catan2_f,
                                    d_math_f_atan2_c(one_f, mtwo_f))) &&
                    (d_tests_same_d(g_catan2_d,
                                    d_math_d_atan2_c(one_d, mtwo_d))) &&
                    (d_tests_same_ld(g_catan2_ld,
                                     d_math_ld_atan2_c(one_ld, mtwo_l))) &&
                    (d_tests_same_d(g_catan2_mid,
                                    d_math_d_atan2_c(mid_y, mid_x))) &&
                    (d_tests_same_d(g_catan2_sub,
                                    d_math_d_atan2_c(dmin, three))) &&
                    (d_tests_same_d(g_catan2_inf,
                                    d_math_d_atan2_c(-inf, -inf))),
                    "constant evaluation agrees with the run time, atan2",
                    0);
#if ( (defined(D_ENV_COMPILER_GCC)) ||                                         \
      (defined(D_ENV_COMPILER_CLANG)) )
    {
        volatile double zero = -0.0;

        D_TESTS_EXPECT(d_tests_same_d(g_catan2_zero,
                                      d_math_d_atan2_c(zero, zero)),
                       "constant evaluation agrees, atan2_c(-0, -0)", 0);
    }
#endif

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
    d_tests_specials();
    d_tests_atan2_float();
    d_tests_atan2_double();
    d_tests_atan2_edges();
    d_tests_atan2_long_double();
    d_tests_atan2_specials();
#if defined(__cplusplus)
    d_tests_overloads();
#endif
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    printf("t_tangent: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
