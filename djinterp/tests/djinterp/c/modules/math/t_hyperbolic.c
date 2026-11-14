/*******************************************************************************
* djinterp [c/math]                                               t_hyperbolic.c
*
*   Conformance harness for c/math/hyperbolic.h. For float, sinh_c, cosh_c
* and tanh_c must be the correctly rounded result -- the long double
* library's, rounded to float -- at one float pattern in 1009 across the
* whole range, and at every float in [7/16, 9/16), where the kernel passes
* from its series to the exponential, and about the overflow; and within two
* ulps of sinhf, coshf and tanhf (glibc's are not always within one).
*
*   For double, three million random arguments, a third below 1/2 down to
* 2^-41, a third from 1/2 to 20 and a third on to 710.5, past the overflow;
* within three ulps of sinh, cosh and tanh (glibc's own are up to two ulps
* off), and how many differ at all is printed. The edges must be exact
* against values from a 400-bit computation: about 1/2, where the kernel
* changes its way, the overflow's edge, where tanh first rounds to 1, and the
* arguments whose long double result lands on a double's rounding midpoint,
* where converting it would round the wrong way; each with x negated too.
* For long double, a million arguments, within three ulps of sinhl and tanhl
* and two of coshl (glibc's are up to 2.47 ulps off; the kernel's within
* 0.6). NaN, the zeros, the infinities, the subnormals and the overflow, for
* every family.
*
*   asinh_c, acosh_c and atanh_c, which complex numbers need: within an ulp of
* the correctly rounded value at every point of math_tests_real_refs.h, in each
* family, and the plain forms, the C library's, within three; the long double
* _c forms within 0.53 of an ulp of the exact value at its points of long
* double's full width, the kernels' bound; within an ulp of the long double
* library's value rounded to double at 300,000 random doubles each -- near the
* functions' edges down to 2^-50, across their middles, far out -- and rounded
* to float at one float pattern in 4099; within four ulps of asinhl, acoshl and
* atanhl at 100,000 random long doubles each (glibc's own are more than two
* off); Annex F's values; the plain forms the C library's, bit for bit, and
* before C++11 the _c forms.
*
*   With the argument "exhaustive" it checks every float, all 2^32 patterns
* (its count wants a 64-bit long). Compiled as C++, it checks the overloads;
* from C++14, that all six functions are constant expressions, for every
* family, with the bits they have at run time. It needs no 64-bit integer --
* its random numbers come from math_tests_random.h, the same in every build
* -- so that ISO-strict C++98 on a 32-bit target, where dj_math_umax has 32
* bits, builds it too.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_hyperbolic.c -o t_hyperbolic -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_hyperbolic.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.09
*                                                            revised: 2026.11.07
*******************************************************************************/
// std
#include <float.h>   // FLT_MAX, DBL_MAX, LDBL_MAX, LDBL_MANT_DIG
#include <math.h>    // sinhf ... tanhl, nextafter
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/decimal.h"     // d_math_decimal_to_f
#include "../../../../../inc/djinterp/c/modules/math/hyperbolic.h"  // unit under test
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
// tests
#include "./math_tests_random.h"     // d_tests_seed, d_tests_below ...
#include "./math_tests_real_refs.h"  // g_ref_asinh, g_ref_acosh, g_ref_atanh


// the C library's functions, called through volatile pointers, so that no
// compiler substitutes its own evaluation, or folds a call into a constant
static float       (*volatile g_sinhf)(float)             = sinhf;
static double      (*volatile g_sinh)(double)             = sinh;
static long double (*volatile g_sinhl)(long double)       = sinhl;
static float       (*volatile g_coshf)(float)             = coshf;
static double      (*volatile g_cosh)(double)             = cosh;
static long double (*volatile g_coshl)(long double)       = coshl;
static float       (*volatile g_tanhf)(float)             = tanhf;
static double      (*volatile g_tanh)(double)             = tanh;
static long double (*volatile g_tanhl)(long double)       = tanhl;
static long double (*volatile g_asinhl)(long double)      = asinhl;
static long double (*volatile g_acoshl)(long double)      = acoshl;
static long double (*volatile g_atanhl)(long double)      = atanhl;
// the float and double asinh, acosh and atanh, which the plain forms are:
// but before C++11, where the plain forms are the _c ones, compared with
// those
#if !( defined(__cplusplus) &&                                                \
       !D_ENV_LANG_IS_CPP11_OR_HIGHER )
static float       (*volatile g_asinhf)(float)            = asinhf;
static double      (*volatile g_asinh)(double)            = asinh;
static float       (*volatile g_acoshf)(float)            = acoshf;
static double      (*volatile g_acosh)(double)            = acosh;
static float       (*volatile g_atanhf)(float)            = atanhf;
static double      (*volatile g_atanh)(double)            = atanh;
#endif

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

// d_tests_float_one
//   one float: sinh_c, cosh_c and tanh_c against the long double library's
// result, rounded to float, which is the correctly rounded one wherever long
// double has eleven bits more than float; and within two ulps of sinhf,
// coshf and tanhf.
static void
d_tests_float_one(float _x)
{
    const float s = d_math_f_sinh_c(_x);
    const float c = d_math_f_cosh_c(_x);
    const float t = d_math_f_tanh_c(_x);

    D_TESTS_EXPECT(d_tests_same_f(s, (float)g_sinhl(_x)),
                   "f sinh_c correctly rounded", _x);
    D_TESTS_EXPECT(d_tests_same_f(c, (float)g_coshl(_x)),
                   "f cosh_c correctly rounded", _x);
    D_TESTS_EXPECT(d_tests_same_f(t, (float)g_tanhl(_x)),
                   "f tanh_c correctly rounded", _x);
    D_TESTS_EXPECT( (D_TESTS_ULPS_F(s, g_sinhf(_x)) <= 2.0) &&
                    (D_TESTS_ULPS_F(c, g_coshf(_x)) <= 2.0) &&
                    (D_TESTS_ULPS_F(t, g_tanhf(_x)) <= 2.0),
                    "f sinh_c, cosh_c, tanh_c within two ulps of the "
                    "library's", _x);

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
        d_tests_float_one(f);

        // to the last pattern a step reaches below 2^32
        if (bits > (0xFFFFFFFFu - step))
        {
            break;
        }
    }

    // every float in [7/16, 9/16): the kernel's series below 1/2, the
    // exponential from it
    for (x = 0.4375f; x < 0.5625f; x = nextafterf(x, 1.0f))
    {
        d_tests_float_one(x);
        d_tests_float_one(-x);
    }

    // every float in [89, 90): sinhf and coshf overflow at 89.415985
    for (x = 89.0f; x < 90.0f; x = nextafterf(x, 100.0f))
    {
        d_tests_float_one(x);
    }

    return;
}

// d_tests_double
//   sinh_c, cosh_c and tanh_c within three ulps of sinh, cosh and tanh over
// random arguments, either sign; prints how many differ at all, and the
// largest difference.
static void
d_tests_double(void)
{
    double worst[3] = { 0.0, 0.0, 0.0 };
    long   diff[3]  = { 0, 0, 0 };
    long   i        = 0;

    for (i = 0; i < 3000000; ++i)
    {
        const double sign = (d_tests_bit() != 0u) ? -1.0 : 1.0;
        double       x    = 0.0;
        double       e[3] = { 0.0, 0.0, 0.0 };
        int          k    = 0;

        // below 1/2, the series; to 20; and on past the overflow at 710.48
        switch (i % 3)
        {
            case 0:
                x = sign * ldexp(1.0 + d_tests_unit(),
                                 -2 - (int)d_tests_below(40u));

                break;

            case 1:
                x = sign * (0.5 + (19.5 * d_tests_unit()));

                break;

            default:
                x = sign * (20.0 + (690.5 * d_tests_unit()));

                break;
        }

        // stored, so x is a double: clang on x87 hands an inlined kernel
        // the 64-bit register value otherwise, and the library the double
        {
            volatile double stored = x;

            x = stored;
        }

        e[0] = D_TESTS_ULPS_D(d_math_d_sinh_c(x), g_sinh(x));
        e[1] = D_TESTS_ULPS_D(d_math_d_cosh_c(x), g_cosh(x));
        e[2] = D_TESTS_ULPS_D(d_math_d_tanh_c(x), g_tanh(x));

        for (k = 0; k < 3; ++k)
        {
            worst[k] = (e[k] > worst[k]) ? e[k] : worst[k];
            diff[k] += (e[k] > 0.0) ? 1 : 0;
        }

        D_TESTS_EXPECT( (e[0] <= 3.0) &&
                        (e[1] <= 3.0) &&
                        (e[2] <= 3.0),
                        "d sinh_c, cosh_c, tanh_c within three ulps of the "
                        "library's", x);
    }

    printf("double: sinh_c, cosh_c, tanh_c differ from sinh, cosh, tanh at "
           "%ld, %ld, %ld of 3000000 arguments, by %.3g, %.3g, %.3g ulps at "
           "most\n", diff[0], diff[1], diff[2], worst[0], worst[1], worst[2]);

    return;
}

// d_tests_double_edges
//   the correctly rounded values, from a 400-bit computation (the slice's
// tools/edge_values6.py): about 1/2, where the kernel passes from its series
// to the exponential; the overflow's edge, the largest double whose sinh and
// cosh are finite; where tanh first rounds to 1; and the arguments whose
// result, rounded to long double, lies on a double's midpoint. Each with x
// negated too: sinh and tanh are odd, cosh even.
static void
d_tests_double_edges(void)
{
    // x and sinh(x), correctly rounded
    static const double sines[][2] = {
        // about 1/2
        { 0.5,                      0.5210953054937474     },
        { 0.49999999999999994,      0.5210953054937473     },
        { 0.5000000000000001,       0.5210953054937475     },
        { 0.25,                     0.2526123168081683     },
        { 0.1,                      0.10016675001984403    },
        { 1.0,                      1.1752011936438014     },
        { 2.0,                      3.6268604078470186     },
        { 3.0,                      10.017874927409903     },
        { 10.0,                     11013.232874703393     },
        { 20.0,                     242582597.70489514     },
        { 100.0,                    1.3440585709080678e+43 },
        { 700.0,                    5.0711602736750225e+303 },
        // the largest double whose sinh is finite
        { 710.4758600739439,        1.7976931348621744e+308 },
        { 1e-05,                    1.0000000000166668e-05 },
        { 1e-08,                    1e-08                  },
        // the long double result lies on a double's midpoint
        { 0.002787447878827177,     0.0027874514885111797  },
        { 4.182013110791463,        32.74115335889923      },
        { 2.846765153286729,        8.586958564588608      },
        { 7.930615317672536e-05,    7.930615325985758e-05  },
        { 3.7574489103664192e-06,   3.757448910375261e-06  },
        { 1.7358275190535189e-06,   1.7358275190543907e-06 }
    };
    // x and cosh(x), correctly rounded
    static const double cosines[][2] = {
        // about 1/2
        { 0.5,                      1.1276259652063807     },
        { 0.49999999999999994,      1.1276259652063807     },
        { 0.5000000000000001,       1.127625965206381      },
        { 0.25,                     1.0314130998795732     },
        { 0.1,                      1.0050041680558035     },
        { 1.0,                      1.5430806348152437     },
        { 2.0,                      3.7621956910836314     },
        { 3.0,                      10.067661995777765     },
        { 10.0,                     11013.232920103323     },
        { 20.0,                     242582597.70489514     },
        { 100.0,                    1.3440585709080678e+43 },
        { 700.0,                    5.0711602736750225e+303 },
        // the largest double whose cosh is finite
        { 710.4758600739439,        1.7976931348621744e+308 },
        { 1e-05,                    1.00000000005          },
        { 1e-08,                    1.0                    },
        // the long double result lies on a double's midpoint
        { 0.02839528258993952,      1.000403173125197      },
        { 3.952670136160926,        26.0467149991596       },
        { 0.02089423892253406,      1.0002182925515533     },
        { 1.3486629824097522,       2.0559293162099785     },
        { 3.703860384480165e-06,    1.0000000000068592     },
        { 0.04282937637897789,      1.0009173179515802     }
    };
    // x and tanh(x), correctly rounded
    static const double tangents[][2] = {
        // about 1/2
        { 0.5,                      0.46211715726000974    },
        { 0.49999999999999994,      0.46211715726000974    },
        { 0.5000000000000001,       0.46211715726000985    },
        { 0.25,                     0.24491866240370913    },
        { 0.1,                      0.09966799462495582    },
        { 1.0,                      0.7615941559557649     },
        { 2.0,                      0.9640275800758169     },
        { 3.0,                      0.9950547536867305     },
        { 10.0,                     0.9999999958776927     },
        // the largest double whose tanh rounds below 1, and the next
        { 19.0,                     0.9999999999999999     },
        { 19.061547465398494,       0.9999999999999999     },
        { 19.061547465398498,       1.0                    },
        { 20.0,                     1.0                    },
        { 30.0,                     1.0                    },
        { 1e-05,                    9.999999999666668e-06  },
        { 1e-08,                    1e-08                  },
        // the long double result lies on a double's midpoint
        { 4.080018497905608e-05,    4.0800184956416674e-05 },
        { 5.675345528685314e-07,    5.675345528684704e-07  },
        { 6.984086796085853e-07,    6.984086796084717e-07  },
        { 1.9669532700967804e-06,   1.966953270094244e-06  },
        { 9.299005540890479e-07,    9.299005540887798e-07  },
        { 0.09400735653097297,      0.09373140562276523    }
    };
    size_t i = 0;

    for (i = 0; i < (sizeof(sines) / sizeof(sines[0])); ++i)
    {
        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_sinh_c(sines[i][0]),
                                        sines[i][1])) &&
                        (d_tests_same_d(d_math_d_sinh_c(-sines[i][0]),
                                        -sines[i][1])),
                        "d sinh_c correctly rounded at an edge", sines[i][0]);
    }

    for (i = 0; i < (sizeof(cosines) / sizeof(cosines[0])); ++i)
    {
        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_cosh_c(cosines[i][0]),
                                        cosines[i][1])) &&
                        (d_tests_same_d(d_math_d_cosh_c(-cosines[i][0]),
                                        cosines[i][1])),
                        "d cosh_c correctly rounded at an edge",
                        cosines[i][0]);
    }

    for (i = 0; i < (sizeof(tangents) / sizeof(tangents[0])); ++i)
    {
        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_tanh_c(tangents[i][0]),
                                        tangents[i][1])) &&
                        (d_tests_same_d(d_math_d_tanh_c(-tangents[i][0]),
                                        -tangents[i][1])),
                        "d tanh_c correctly rounded at an edge",
                        tangents[i][0]);
    }

    // past the overflow's edge: the next double up
    {
        const double past = nextafter(710.4758600739439, 800.0);

        D_TESTS_EXPECT( (isinf(d_math_d_sinh_c(past))) &&
                        (isinf(d_math_d_sinh_c(-past))) &&
                        (d_math_d_sinh_c(-past) < 0.0) &&
                        (isinf(d_math_d_cosh_c(-past))),
                        "d sinh_c and cosh_c overflow past the edge", past);
    }

    return;
}

static void
d_tests_long_double(void)
{
    // the largest argument drawn: short of the overflow at LDBL_MAX_EXP ln 2
    const long double top      = (long double)(LDBL_MAX_EXP - 2) * 0.6931L;
    double            worst[3] = { 0.0, 0.0, 0.0 };
    long              i        = 0;

    for (i = 0; i < 1000000; ++i)
    {
        const long double sign = (d_tests_bit() != 0u) ? -1.0L : 1.0L;
        // in [0, 1), every bit random: two doubles' worth
        const long double m    = (long double)d_tests_unit() +
                                 ((long double)d_tests_unit() * LDBL_EPSILON);
        long double       x    = 0.0L;
        double            e[3] = { 0.0, 0.0, 0.0 };
        int               k    = 0;

        switch (i % 3)
        {
            case 0:
                x = sign * ldexpl(1.0L + m,
                                  -2 - (int)d_tests_below(60u));

                break;

            case 1:
                x = sign * (0.5L + (19.5L * m));

                break;

            default:
                x = sign * (20.0L + ((top - 20.0L) * m));

                break;
        }

        e[0] = D_TESTS_ULPS_LD(d_math_ld_sinh_c(x), g_sinhl(x));
        e[1] = D_TESTS_ULPS_LD(d_math_ld_cosh_c(x), g_coshl(x));
        e[2] = D_TESTS_ULPS_LD(d_math_ld_tanh_c(x), g_tanhl(x));

        for (k = 0; k < 3; ++k)
        {
            worst[k] = (e[k] > worst[k]) ? e[k] : worst[k];
        }

        D_TESTS_EXPECT( (e[0] <= 3.0) &&
                        (e[1] <= 2.0) &&
                        (e[2] <= 3.0),
                        "ld sinh_c, cosh_c, tanh_c within three, two, three "
                        "ulps of the library's", x);
    }

    printf("long double: sinh_c, cosh_c, tanh_c within %.3g, %.3g, %.3g ulps "
           "of sinhl, coshl, tanhl\n", worst[0], worst[1], worst[2]);

    return;
}

// d_tests_specials
//   NaN, the zeros, the infinities, the subnormals and the overflow, every
// family: sinh and tanh keep a zero's sign and a subnormal, cosh is 1 there;
// sinh is +-inf and cosh +inf at the infinities and past the largest finite
// result, tanh +-1.
static void
d_tests_specials(void)
{
    const double      inf   = (double)HUGE_VAL;
    const double      tiny  = 4.9406564584124654e-324;
    const long double inf_l = (long double)HUGE_VAL;

    D_TESTS_EXPECT( (isnan(d_math_f_sinh_c(nanf("")))) &&
                    (isnan(d_math_d_cosh_c(nan("")))) &&
                    (isnan(d_math_ld_tanh_c(nanl("")))) &&
                    (isnan(d_math_d_sinh_c(nan("")))) &&
                    (isnan(d_math_f_cosh_c(nanf("")))) &&
                    (isnan(d_math_d_tanh_c(nan("")))),
                    "NaN gives NaN", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_sinh_c(-0.0), -0.0)) &&
                    (d_tests_same_d(d_math_d_sinh_c(0.0), 0.0)) &&
                    (d_tests_same_f(d_math_f_tanh_c(-0.0f), -0.0f)) &&
                    (d_tests_same_ld(d_math_ld_tanh_c(-0.0L), -0.0L)) &&
                    (d_tests_same_d(d_math_d_cosh_c(-0.0), 1.0)) &&
                    (d_tests_same_f(d_math_f_cosh_c(0.0f), 1.0f)) &&
                    (d_tests_same_ld(d_math_ld_sinh_c(-0.0L), -0.0L)),
                    "the zeros", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_sinh_c(tiny), tiny)) &&
                    (d_tests_same_d(d_math_d_tanh_c(-tiny), -tiny)) &&
                    (d_tests_same_d(d_math_d_cosh_c(tiny), 1.0)) &&
                    (d_tests_same_f(d_math_f_sinh_c(-1e-40f), -1e-40f)) &&
                    (d_tests_same_d(d_math_d_sinh_c(1e-200), 1e-200)),
                    "the subnormals and the tiny", 0);
    D_TESTS_EXPECT( (isnan(d_math_ld_cosh_c(nanl("")))) &&
                    (d_tests_same_ld(d_math_ld_sinh_c(12000.0L), inf_l)) &&
                    (d_tests_same_ld(d_math_ld_sinh_c(-12000.0L), -inf_l)) &&
                    (d_tests_same_ld(d_math_ld_sinh_c(11357.25L), inf_l)) &&
                    (d_tests_same_ld(d_math_ld_cosh_c(11357.25L), inf_l)),
                    "long double: NaN, and past the overflow", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_sinh_c(inf), inf)) &&
                    (d_tests_same_d(d_math_d_sinh_c(-inf), -inf)) &&
                    (d_tests_same_d(d_math_d_cosh_c(-inf), inf)) &&
                    (d_tests_same_d(d_math_d_tanh_c(inf), 1.0)) &&
                    (d_tests_same_d(d_math_d_tanh_c(-inf), -1.0)) &&
                    (d_tests_same_f(d_math_f_cosh_c(-(float)inf),
                                    (float)inf)) &&
                    (d_tests_same_ld(d_math_ld_sinh_c(-inf_l), -inf_l)) &&
                    (d_tests_same_ld(d_math_ld_tanh_c(inf_l), 1.0L)),
                    "the infinities", 0);
    D_TESTS_EXPECT( (isinf(d_math_d_sinh_c(1000.0))) &&
                    (d_math_d_sinh_c(-1000.0) < 0.0) &&
                    (isinf(d_math_f_cosh_c(-100.0f))) &&
                    (d_tests_same_f(d_math_f_sinh_c(89.41598510742188f),
                                    (float)g_sinhl(89.41598510742188f))) &&
                    (isinf(d_math_f_sinh_c(89.41599273681641f))) &&
                    (d_tests_same_ld(d_math_ld_cosh_c(LDBL_MAX),
                                     inf_l)) &&
                    (d_tests_same_d(d_math_d_tanh_c(1e300), 1.0)),
                    "past the largest finite result", 0);

    // the plain forms are the C library's
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_sinh(1.5), g_sinh(1.5)),
                   "d sinh", 0);
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_cosh(1.5f), g_coshf(1.5f)),
                   "f cosh", 0);
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_tanh(-0.75), g_tanh(-0.75)),
                   "d tanh", 0);

    return;
}


// 1.  The inverse functions
//------------------------------------------------------------------------------
// asinh, acosh and atanh, which complex numbers need: against the references
// (every family, both forms), at random arguments against the long double
// library, and at Annex F's specials.

#define D_TESTS_COUNT(array)                                                   \
    (sizeof(array) / sizeof((array)[0]))

// d_tests_ref_point
//   a reference's point, m 2^e, exactly.
static long double
d_tests_ref_point(
    long _m,
    int  _e
)
{
    return (long double)_m * ldexpl(1.0L, _e);
}

// d_tests_inverse_one
//   the function _k (0 asinh, 1 acosh, 2 atanh) of the family _family at
// _x, its _c form where _pure, widened.
static long double
d_tests_inverse_one(
    int         _k,
    int         _family,
    bool        _pure,
    long double _x
)
{
    volatile float       f = 0.0f;
    volatile double      d = 0.0;
    volatile long double l = 0.0L;

    if (_family == 0)
    {
        const float x = (float)_x;

        f = (_k == 0) ? (_pure ? d_math_f_asinh_c(x) : d_math_f_asinh(x))
          : (_k == 1) ? (_pure ? d_math_f_acosh_c(x) : d_math_f_acosh(x))
                      : (_pure ? d_math_f_atanh_c(x) : d_math_f_atanh(x));

        return (long double)f;
    }

    if (_family == 1)
    {
        const double x = (double)_x;

        d = (_k == 0) ? (_pure ? d_math_d_asinh_c(x) : d_math_d_asinh(x))
          : (_k == 1) ? (_pure ? d_math_d_acosh_c(x) : d_math_d_acosh(x))
                      : (_pure ? d_math_d_atanh_c(x) : d_math_d_atanh(x));

        return (long double)d;
    }

    l = (_k == 0) ? (_pure ? d_math_ld_asinh_c(_x) : d_math_ld_asinh(_x))
      : (_k == 1) ? (_pure ? d_math_ld_acosh_c(_x) : d_math_ld_acosh(_x))
                  : (_pure ? d_math_ld_atanh_c(_x) : d_math_ld_atanh(_x));

    return l;
}

// d_tests_inverse_refs
//   each reference, each family: the _c form within an ulp of the correctly
// rounded value -- the long double kernel's error, below 0.52 ulp of long
// double, rounded once more -- and the plain form, the C library's (before
// C++11, _c), within 3; the largest distances printed.
static void
d_tests_inverse_refs(void)
{
    const struct dj_tests_real_ref* const tables[3] = { g_ref_asinh,
                                                        g_ref_acosh,
                                                        g_ref_atanh };
    const size_t      counts[3] = { D_TESTS_COUNT(g_ref_asinh),
                                    D_TESTS_COUNT(g_ref_acosh),
                                    D_TESTS_COUNT(g_ref_atanh) };
    const char* const names[3]  = { "asinh", "acosh", "atanh" };
    int               k         = 0;

    for (k = 0; k < 3; ++k)
    {
        double worst[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
        size_t i        = 0u;

        for (i = 0u; i < counts[k]; ++i)
        {
            const struct dj_tests_real_ref* r = &tables[k][i];
            const long double x      = d_tests_ref_point(r->m, r->e);
            const size_t      length = strlen(r->value);
            float             wf     = 0.0f;
            double            wd     = 0.0;
            long double       wl     = 0.0L;
            double            u[6];
            int               j      = 0;

            (void)d_math_decimal_to_f(&wf, r->value, length);
            (void)d_math_decimal_to_d(&wd, r->value, length);
            (void)d_math_decimal_to_ld(&wl, r->value, length);

            u[0] = D_TESTS_ULPS_F(d_tests_inverse_one(k, 0, true, x), wf);
            u[1] = D_TESTS_ULPS_F(d_tests_inverse_one(k, 0, false, x), wf);
            u[2] = D_TESTS_ULPS_D(d_tests_inverse_one(k, 1, true, x), wd);
            u[3] = D_TESTS_ULPS_D(d_tests_inverse_one(k, 1, false, x), wd);
            u[4] = D_TESTS_ULPS_LD(d_tests_inverse_one(k, 2, true, x), wl);
            u[5] = D_TESTS_ULPS_LD(d_tests_inverse_one(k, 2, false, x), wl);

            for (j = 0; j < 6; ++j)
            {
                worst[j] = (u[j] > worst[j]) ? u[j] : worst[j];
            }

            D_TESTS_EXPECT( (u[0] <= 1.0) &&
                            (u[2] <= 1.0) &&
                            (u[4] <= 1.0),
                            "asinh_c, acosh_c and atanh_c within an ulp of"
                            " the reference", x );
            D_TESTS_EXPECT( (u[1] <= 3.0) &&
                            (u[3] <= 3.0) &&
                            (u[5] <= 3.0),
                            "asinh, acosh and atanh within 3 ulps of the"
                            " reference", x );
        }

        printf("  %s against the references, in ulps, _c then plain: float"
               " %.3f %.3f, double %.3f %.3f, long double %.3f %.3f\n",
               names[k],
               worst[0],
               worst[1],
               worst[2],
               worst[3],
               worst[4],
               worst[5]);
    }

    return;
}

// the long double kernels' bound, in fractions of an ulp of the exact value
#define D_TESTS_KERNEL_BOUND  0.53

// d_tests_wide_point
//   a full-width point, (_m[0] 2^48 + _m[1] 2^24 + _m[2]) 2^_e, exactly: each
// limb's term is exact, and each sum a prefix of the point's bits.
static long double
d_tests_wide_point(
    const long* _m,
    int         _e
)
{
    return (d_tests_ref_point(_m[0], _e + 48) +
            d_tests_ref_point(_m[1], _e + 24)) +
           d_tests_ref_point(_m[2], _e);
}

// d_tests_inverse_wide
//   each full-width reference: asinh_c, acosh_c and atanh_c in long double
// within D_TESTS_KERNEL_BOUND of an ulp of the exact value -- their distance
// from the nearest long double, exact, less the exact value's offset from it
// -- and the plain forms, the C library's, within 3 of the nearest; the
// largest distances printed. A long double narrower than 64 bits takes no
// such point exactly, and is left out.
static void
d_tests_inverse_wide(void)
{
    const struct dj_tests_real_wide* const tables[3] = { g_wide_asinh,
                                                         g_wide_acosh,
                                                         g_wide_atanh };
    const size_t      counts[3] = { D_TESTS_COUNT(g_wide_asinh),
                                    D_TESTS_COUNT(g_wide_acosh),
                                    D_TESTS_COUNT(g_wide_atanh) };
    const char* const names[3]  = { "asinh", "acosh", "atanh" };
    int               k         = 0;

    if (LDBL_MANT_DIG < 64)
    {
        return;
    }

    for (k = 0; k < 3; ++k)
    {
        double worst_c     = 0.0;
        double worst_plain = 0.0;
        size_t i           = 0u;

        for (i = 0u; i < counts[k]; ++i)
        {
            const struct dj_tests_real_wide* r = &tables[k][i];
            const long double x = d_tests_wide_point(r->m, r->e);
            long double       wl     = 0.0L;
            double            offset = 0.0;
            long double       unit   = 0.0L;
            long double       got    = 0.0L;
            double            error  = 0.0;
            double            plain  = 0.0;

            (void)d_math_decimal_to_ld(&wl, r->value, strlen(r->value));
            (void)d_math_decimal_to_d(&offset, r->offset, strlen(r->offset));
            unit  = ldexpl(1.0L, ilogbl(wl) - (LDBL_MANT_DIG - 1));
            got   = d_tests_inverse_one(k, 2, true, x);
            error = fabs((double)((got - wl) / unit) - offset);
            plain = D_TESTS_ULPS_LD(d_tests_inverse_one(k, 2, false, x), wl);
            worst_c     = (error > worst_c) ? error : worst_c;
            worst_plain = (plain > worst_plain) ? plain : worst_plain;

            D_TESTS_EXPECT( (error <= D_TESTS_KERNEL_BOUND),
                            "ld asinh_c, acosh_c and atanh_c within the"
                            " kernel's bound of the exact value", x );
            D_TESTS_EXPECT( (plain <= 3.0),
                            "ld asinh, acosh and atanh within 3 ulps of the"
                            " reference", x );
        }

        printf("  %s at long double's full width, in ulps of the exact value:"
               " _c %.4f, plain %.3f\n",
               names[k],
               worst_c,
               worst_plain);
    }

    return;
}

// d_tests_inverse_argument
//   a random argument of the function _k, as a long double: a third near its
// edge -- 0 for asinh and atanh, 1 for acosh -- down to 2^-(_depth), a third
// across its middle, a third far out (asinh and acosh to 2^(_far), atanh
// within 2^-(_depth) of +-1); _u in [0, 1).
static long double
d_tests_inverse_argument(
    int         _k,
    long double _u,
    int         _depth,
    int         _far
)
{
    const unsigned long kind = d_tests_below(3u);
    const long double   s    = (d_tests_bit() != 0u) ? -1.0L : 1.0L;
    const int           near = 2 + (int)d_tests_below((unsigned long)_depth);
    const int           out  = 2 + (int)d_tests_below((unsigned long)_far);

    if (_k == 1)
    {
        return (kind == 0u) ? (1.0L + ldexpl(1.0L + _u, -near))
             : (kind == 1u) ? (1.0L + (15.0L * _u))
                            : ldexpl(1.0L + _u, out);
    }

    if (kind == 0u)
    {
        return s * ldexpl(1.0L + _u, -near);
    }

    if (kind == 1u)
    {
        return (_k == 0) ? ((8.0L * _u) - 4.0L) : ((1.5L * _u) - 0.75L);
    }

    return (_k == 0) ? (s * ldexpl(1.0L + _u, out))
                     : (s * (1.0L - ldexpl(1.0L + _u, -near)));
}

// d_tests_inverse_random
//   double: 300,000 arguments each, d_tests_inverse_argument's, each _c
// result within an ulp of the long double library's rounded to double, and
// the plain the C library's, bit for bit (before C++11, _c's). Float: one
// pattern in 4099, within an ulp of the long double library's rounded to
// float, and the plain the C library's, bit for bit, as double's. Long
// double: 100,000 each, within 4 ulps of the library's: the kernel's own
// error lies below 0.52 of an ulp, the library's above 2. The library is
// reached through the volatile pointers, as everywhere here.
static void
d_tests_inverse_random(void)
{
    long double (*const library[3])(long double) = { g_asinhl, g_acoshl,
                                                     g_atanhl };
    double      worst[9] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    long        i        = 0;
    int         k        = 0;
    uint32_t    pattern  = 0u;

    for (k = 0; k < 3; ++k)
    {
        for (i = 0; i < 300000L; ++i)
        {
            volatile double x =
                (double)d_tests_inverse_argument(k, (long double)d_tests_unit(),
                                                 50, 1000);
            const double    got  = (double)d_tests_inverse_one(k, 1, true, x);
            const double    plain = (double)d_tests_inverse_one(k, 1, false, x);
            const double    d    = D_TESTS_ULPS_D(got,
                                       (double)library[k]((long double)x));
            volatile double lib  = 0.0;

            worst[k] = (d > worst[k]) ? d : worst[k];
            D_TESTS_EXPECT( (d <= 1.0), "asinh_c, acosh_c or atanh_c within"
                            " an ulp", x );

#if ( defined(__cplusplus) &&                                                 \
      !D_ENV_LANG_IS_CPP11_OR_HIGHER )
            lib = got;
#else
            lib = (k == 0) ? g_asinh(x) : (k == 1) ? g_acosh(x) : g_atanh(x);
#endif
            D_TESTS_EXPECT( (d_tests_same_d(plain, lib)),
                            "asinh, acosh and atanh are the C library's"
                            " (before C++11, the _c forms)", x );
        }
    }

    for (pattern = 0u; pattern < 0xFFFFF000u; pattern += 4099u)
    {
        float x = 0.0f;

        memcpy(&x, &pattern, sizeof(x));

        for (k = 0; k < 3; ++k)
        {
            const float    got   = (float)d_tests_inverse_one(k, 0, true, x);
            const float    plain = (float)d_tests_inverse_one(k, 0, false,
                                                              x);
            const double   d     = D_TESTS_ULPS_F(got,
                                     (float)library[k]((long double)x));
            volatile float lib   = 0.0f;

            worst[3 + k] = (d > worst[3 + k]) ? d : worst[3 + k];
            D_TESTS_EXPECT( (d <= 1.0), "an inverse's _c of a float", x );

#if ( defined(__cplusplus) &&                                                 \
      !D_ENV_LANG_IS_CPP11_OR_HIGHER )
            lib = got;
#else
            lib = (k == 0) ? g_asinhf(x) : (k == 1) ? g_acoshf(x) : g_atanhf(x);
#endif
            D_TESTS_EXPECT( (d_tests_same_f(plain, lib)),
                            "an inverse's plain form of a float the C"
                            " library's (before C++11, the _c form)", x );
        }
    }

    for (k = 0; k < 3; ++k)
    {
        for (i = 0; i < 100000L; ++i)
        {
            const long double u = (long double)d_tests_unit() +
                                  (d_tests_top(11) * ldexpl(1.0L, -64));
            volatile long double x =
                d_tests_inverse_argument(k, u, 62, 16000);
            const long double got = d_tests_inverse_one(k, 2, true, x);
            const double      d   = D_TESTS_ULPS_LD(got, library[k](x));

            worst[6 + k] = (d > worst[6 + k]) ? d : worst[6 + k];
            D_TESTS_EXPECT( (d <= 4.0), "an inverse's _c within 4 ulps of"
                            " the long double library", x );
        }
    }

    printf("  asinh_c, acosh_c and atanh_c against the long double library,"
           " in ulps: double %.3f %.3f %.3f, float %.3f %.3f %.3f, long"
           " double %.3f %.3f %.3f\n",
           worst[0],
           worst[1],
           worst[2],
           worst[3],
           worst[4],
           worst[5],
           worst[6],
           worst[7],
           worst[8]);

    return;
}

// d_tests_inverse_specials
//   Annex F's values, every family, both forms: asinh of the zeros and the
// infinities their own; acosh of 1 +0, below 1 and of -inf NaN, of +inf
// +inf; atanh of the zeros their own, of +-1 +-inf, past them and of the
// infinities NaN; a subnormal its own for asinh and atanh; NaN to NaN.
static void
d_tests_inverse_specials(void)
{
    const long double inf  = (long double)HUGE_VAL;
    const long double nanv = (long double)nan("");
    // float's least subnormal, 2^-149, built exactly: a literal 1.0e-45f is
    // read at long double's precision where float is evaluated wider (g++
    // before C++11 on the x87), and is then no float at all
    const long double minf = ldexpl(1.0L, -149);
    int               family = 0;

    for (family = 0; family < 3; ++family)
    {
        int form = 0;

        for (form = 0; form < 2; ++form)
        {
            const bool pure = (form == 0);

#define D_TESTS_INV(k, x)                                                     \
    d_tests_inverse_one((k), family, pure, (long double)(x))

            D_TESTS_EXPECT( (d_tests_same_ld(D_TESTS_INV(0, 0.0L), 0.0L)) &&
                            (d_tests_same_ld(D_TESTS_INV(0, -0.0L), -0.0L)) &&
                            (d_tests_same_ld(D_TESTS_INV(0, inf), inf)) &&
                            (d_tests_same_ld(D_TESTS_INV(0, -inf), -inf)) &&
                            (d_tests_same_ld(D_TESTS_INV(0, -minf), -minf)) &&
                            (isnan(D_TESTS_INV(0, nanv))),
                            "asinh at the zeros, the infinities, a subnormal"
                            " and NaN", family );
            D_TESTS_EXPECT( (d_tests_same_ld(D_TESTS_INV(1, 1.0L), 0.0L)) &&
                            (d_tests_same_ld(D_TESTS_INV(1, inf), inf)) &&
                            (isnan(D_TESTS_INV(1, 0.5L))) &&
                            (isnan(D_TESTS_INV(1, -1.0L))) &&
                            (isnan(D_TESTS_INV(1, -inf))) &&
                            (isnan(D_TESTS_INV(1, nanv))),
                            "acosh at 1, below it, the infinities and NaN",
                            family );
            D_TESTS_EXPECT( (d_tests_same_ld(D_TESTS_INV(2, 0.0L), 0.0L)) &&
                            (d_tests_same_ld(D_TESTS_INV(2, -0.0L), -0.0L)) &&
                            (d_tests_same_ld(D_TESTS_INV(2, 1.0L), inf)) &&
                            (d_tests_same_ld(D_TESTS_INV(2, -1.0L), -inf)) &&
                            (d_tests_same_ld(D_TESTS_INV(2, minf), minf)) &&
                            (isnan(D_TESTS_INV(2, 1.5L))) &&
                            (isnan(D_TESTS_INV(2, -2.0L))) &&
                            (isnan(D_TESTS_INV(2, inf))) &&
                            (isnan(D_TESTS_INV(2, nanv))),
                            "atanh at the zeros, +-1, past them, a subnormal"
                            " and NaN", family );

#undef D_TESTS_INV
        }
    }

    return;
}

#if defined(__cplusplus)

// d_tests_overloads
//   compiled as C++: one name for every family, chosen by argument type.
static void
d_tests_overloads(void)
{
    D_TESTS_EXPECT( (d_tests_same_f(d_math_sinh_c(0.75f),
                                    d_math_f_sinh_c(0.75f))) &&
                    (d_tests_same_d(d_math_sinh_c(0.75),
                                    d_math_d_sinh_c(0.75))) &&
                    (d_tests_same_ld(d_math_sinh_c(0.75L),
                                     d_math_ld_sinh_c(0.75L))) &&
                    (d_tests_same_f(d_math_cosh_c(3.0f),
                                    d_math_f_cosh_c(3.0f))) &&
                    (d_tests_same_d(d_math_cosh_c(3.0),
                                    d_math_d_cosh_c(3.0))) &&
                    (d_tests_same_ld(d_math_cosh_c(3.0L),
                                     d_math_ld_cosh_c(3.0L))) &&
                    (d_tests_same_f(d_math_tanh_c(-0.25f),
                                    d_math_f_tanh_c(-0.25f))) &&
                    (d_tests_same_d(d_math_tanh_c(-0.25),
                                    d_math_d_tanh_c(-0.25))) &&
                    (d_tests_same_ld(d_math_tanh_c(-0.25L),
                                     d_math_ld_tanh_c(-0.25L))) &&
                    (d_tests_same_d(d_math_sinh(0.75), g_sinh(0.75))) &&
                    (d_tests_same_d(d_math_cosh(3.0), g_cosh(3.0))) &&
                    (d_tests_same_f(d_math_tanh(-0.25f), g_tanhf(-0.25f))),
                    "the C++ overloads are the families' functions", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_asinh_c(0.75f),
                                    d_math_f_asinh_c(0.75f))) &&
                    (d_tests_same_d(d_math_acosh_c(3.0),
                                    d_math_d_acosh_c(3.0))) &&
                    (d_tests_same_ld(d_math_atanh_c(-0.25L),
                                     d_math_ld_atanh_c(-0.25L))) &&
                    (d_tests_same_d(d_math_asinh(0.75),
                                    d_math_d_asinh(0.75))) &&
                    (d_tests_same_f(d_math_acosh(3.0f),
                                    d_math_f_acosh(3.0f))) &&
                    (d_tests_same_ld(d_math_atanh(-0.25L),
                                     d_math_ld_atanh(-0.25L))),
                    "the C++ overloads of asinh, acosh and atanh", 0);

    return;
}

#endif  // __cplusplus

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// from C++14, all three are constant expressions: every family, the edges and
// the specials among them
static constexpr float       g_csinh_f   = d_math_f_sinh_c(0.75f);
static constexpr double      g_csinh_d   =
    d_math_d_sinh_c(0.49999999999999994);
static constexpr long double g_csinh_ld  = d_math_ld_sinh_c(20.0L);
static constexpr double      g_csinh_top = d_math_d_sinh_c(710.4758600739439);
static constexpr double      g_csinh_mid =
    d_math_d_sinh_c(0.002787447878827177);
static constexpr float       g_ccosh_f   = d_math_f_cosh_c(-89.0f);
static constexpr double      g_ccosh_d   = d_math_d_cosh_c(0.5);
static constexpr long double g_ccosh_ld  = d_math_ld_cosh_c(11000.0L);
static constexpr double      g_ccosh_inf = d_math_d_cosh_c(1000.0);
static constexpr float       g_ctanh_f   = d_math_f_tanh_c(-0.5f);
static constexpr double      g_ctanh_d   =
    d_math_d_tanh_c(19.061547465398494);
static constexpr long double g_ctanh_ld  = d_math_ld_tanh_c(3.0L);
static constexpr double      g_ctanh_mid =
    d_math_d_tanh_c(0.09400735653097297);

static_assert(d_math_d_sinh_c(0.0) == 0.0, "sinh_c(0) is 0");
static_assert(d_math_d_cosh_c(0.0) == 1.0, "cosh_c(0) is 1");
static_assert(d_math_d_sinh_c(1.0) == (double)1.1752011936438014,
              "sinh_c(1)");
static_assert(d_math_d_cosh_c(710.4758600739439) ==
              (double)1.7976931348621744e+308, "cosh_c at the overflow");
static_assert(d_math_d_tanh_c(19.061547465398498) == 1.0,
              "tanh_c rounds to 1");
static_assert(d_math_tanh_c(2.0f) > (float)0.964f,
              "the C++ overloads, float");

// and the inverse functions
static constexpr float       g_casinh_f  = d_math_f_asinh_c(-0.75f);
static constexpr double      g_casinh_d  = d_math_d_asinh_c(1.0e200);
static constexpr long double g_casinh_ld = d_math_ld_asinh_c(1.0e-3L);
static constexpr float       g_cacosh_f  = d_math_f_acosh_c(1.5f);
static constexpr double      g_cacosh_d  = d_math_d_acosh_c(1.0000001);
static constexpr long double g_cacosh_ld = d_math_ld_acosh_c(1.0e100L);
static constexpr float       g_catanh_f  = d_math_f_atanh_c(0.5f);
static constexpr double      g_catanh_d  = d_math_d_atanh_c(-0.9999999);
static constexpr long double g_catanh_ld = d_math_ld_atanh_c(1.0e-7L);

static_assert(d_math_d_asinh_c(0.0) == 0.0, "asinh_c(0) is 0");
static_assert(d_math_d_acosh_c(1.0) == 0.0, "acosh_c(1) is 0");
static_assert(d_math_d_atanh_c(1.0) > DBL_MAX, "atanh_c(1) is +inf");
static_assert(d_math_d_acosh_c(0.5) != d_math_d_acosh_c(0.5),
              "acosh_c(1/2) is NaN");

// d_tests_constant_evaluation
//   the constants are the run time's results, bit for bit: the arguments
// are read through volatiles, so the calls below are not folded.
static void
d_tests_constant_evaluation(void)
{
    volatile float       f_sinh = 0.75f;
    volatile double      d_sinh = 0.49999999999999994;
    volatile long double l_sinh = 20.0L;
    volatile double      d_top  = 710.4758600739439;
    volatile double      d_midp = 0.002787447878827177;
    volatile float       f_cosh = -89.0f;
    volatile double      d_cosh = 0.5;
    volatile long double l_cosh = 11000.0L;
    volatile double      d_huge = 1000.0;
    volatile float       f_tanh = -0.5f;
    volatile double      d_tanh = 19.061547465398494;
    volatile long double l_tanh = 3.0L;
    volatile double      d_midt = 0.09400735653097297;

    D_TESTS_EXPECT( (d_tests_same_f(g_csinh_f, d_math_f_sinh_c(f_sinh))) &&
                    (d_tests_same_d(g_csinh_d, d_math_d_sinh_c(d_sinh))) &&
                    (d_tests_same_ld(g_csinh_ld,
                                     d_math_ld_sinh_c(l_sinh))) &&
                    (d_tests_same_d(g_csinh_top, d_math_d_sinh_c(d_top))) &&
                    (d_tests_same_d(g_csinh_mid, d_math_d_sinh_c(d_midp))) &&
                    (d_tests_same_f(g_ccosh_f, d_math_f_cosh_c(f_cosh))) &&
                    (d_tests_same_d(g_ccosh_d, d_math_d_cosh_c(d_cosh))) &&
                    (d_tests_same_ld(g_ccosh_ld,
                                     d_math_ld_cosh_c(l_cosh))) &&
                    (d_tests_same_d(g_ccosh_inf, d_math_d_cosh_c(d_huge))) &&
                    (d_tests_same_f(g_ctanh_f, d_math_f_tanh_c(f_tanh))) &&
                    (d_tests_same_d(g_ctanh_d, d_math_d_tanh_c(d_tanh))) &&
                    (d_tests_same_ld(g_ctanh_ld,
                                     d_math_ld_tanh_c(l_tanh))) &&
                    (d_tests_same_d(g_ctanh_mid, d_math_d_tanh_c(d_midt))),
                    "constant evaluation agrees with the run time", 0);

    {
        volatile float       as_f  = -0.75f;
        volatile double      as_d  = 1.0e200;
        volatile long double as_ld = 1.0e-3L;
        volatile float       ac_f  = 1.5f;
        volatile double      ac_d  = 1.0000001;
        volatile long double ac_ld = 1.0e100L;
        volatile float       at_f  = 0.5f;
        volatile double      at_d  = -0.9999999;
        volatile long double at_ld = 1.0e-7L;

        D_TESTS_EXPECT( (d_tests_same_f(g_casinh_f, d_math_f_asinh_c(as_f))) &&
                        (d_tests_same_d(g_casinh_d, d_math_d_asinh_c(as_d))) &&
                        (d_tests_same_ld(g_casinh_ld,
                                         d_math_ld_asinh_c(as_ld))) &&
                        (d_tests_same_f(g_cacosh_f, d_math_f_acosh_c(ac_f))) &&
                        (d_tests_same_d(g_cacosh_d, d_math_d_acosh_c(ac_d))) &&
                        (d_tests_same_ld(g_cacosh_ld,
                                         d_math_ld_acosh_c(ac_ld))) &&
                        (d_tests_same_f(g_catanh_f, d_math_f_atanh_c(at_f))) &&
                        (d_tests_same_d(g_catanh_d, d_math_d_atanh_c(at_d))) &&
                        (d_tests_same_ld(g_catanh_ld,
                                         d_math_ld_atanh_c(at_ld))),
                        "constant evaluation agrees with the run time: the"
                        " inverse functions", 0);
    }

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
    d_tests_inverse_refs();
    d_tests_inverse_wide();
    d_tests_inverse_random();
    d_tests_inverse_specials();
#if defined(__cplusplus)
    d_tests_overloads();
#endif
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    printf("t_hyperbolic: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
