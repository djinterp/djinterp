/*******************************************************************************
* djinterp [test]                                                 t_elementary.c
*
*   Conformance harness for c/math/elementary.h. sqrt_c must be the C
* library's sqrt, bit for bit: every float in [1, 4) -- every mantissa the
* root's core sees -- and one float in 37 across the whole range; ten million
* random doubles, squares of random doubles and their neighbours (the cases
* nearest a rounding midpoint), every power of two, the subnormals; two
* million random long doubles.
*
*   asin_c and acos_c: for float, the correctly rounded result -- the long
* double library's, rounded to float -- at one float pattern in 1009, and
* within an ulp of asinf and acosf; for double, within an ulp of asin and
* acos at two million arguments, a third of them uniform in [-1, 1), a third
* within 2^-1 to 2^-52 of +-1 and a third from 2^-1 down to the least
* subnormal, and exact at the edges and where the long double result lands
* on a midpoint; for long double, within an ulp of asinl and two of acosl
* (glibc's is 1.54 ulps off near 1). NaN, the zeros, +-1 and the arguments
* past them for every family; the largest error is printed.
*
*   sin_c and cos_c must be within an ulp of sin and cos for float and double
* and two for long double, below 2^20, at the multiples of pi/2, and in every
* binade from 2^20 to the largest finite value, where the reduction by pi/2
* is Payne and Hanek's; for float they must be the correctly rounded result
* -- the long double library's, rounded to float -- at one pattern in 2003;
* and for double, exact against values from a 400-bit computation at huge
* arguments (the double nearest a multiple of pi/2 among them) and at the
* arguments whose long double result lands on a double's rounding midpoint.
*
*   sqrt_settle_digits, the root's last bit settled in digits of 16 bits --
* sqrt_c's for a double where dj_math_umax has 32 bits -- is called directly,
* so that every build checks it: the library's root from Newton's root and
* from a place and two either side, at every sixteenth float in [1, 4), and
* at 250,000 random doubles there and 50,000 squares and their neighbours;
* and over the whole reach of its loops, from the library's root moved up to
* four places either way, at every sixty-fourth float there, 100,000 random
* doubles and the edges.
*
*   sqrt, the C library's root, must be sqrt_c's as it comes back: taken
* straight into a long double, which keeps what a call leaves in an x87
* register -- glibc's sqrtf leaves 64 bits, which sqrt stores away where
* float is evaluated wider. One positive float pattern in 4093, 500,000
* random doubles and 200,000 random long doubles.
*
*   With the argument "exhaustive" it checks every float, all 2^32 patterns
* (its count wants a 64-bit long). Compiled as C++, it checks the overloads;
* from C++14, that sin_c, cos_c, asin_c, acos_c and sqrt_settle_digits are
* constant expressions, at huge arguments too, with the bits they have at
* run time. It needs no 64-bit integer -- its random numbers come from
* math_tests_random.h, the same in every build -- so that ISO-strict C++98
* on a 32-bit target, where dj_math_umax has 32 bits, builds it too.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_elementary.c -o t_elementary -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_elementary.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.27
*******************************************************************************/
// std
#include <float.h>   // FLT_MAX, DBL_EPSILON, LDBL_MANT_DIG
#include <math.h>    // sqrtf ... cosl, asinf ... acosl, nextafter
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/elementary.h"  // unit under test
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
// tests
#include "./math_tests_random.h"  // d_tests_seed, d_tests_below ...


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
// (and asin and acos, which no compiler then folds into a constant of its
// own computing)
static float       (*volatile g_asinf)(float)             = asinf;
static double      (*volatile g_asin)(double)             = asin;
static long double (*volatile g_asinl)(long double)       = asinl;
static float       (*volatile g_acosf)(float)             = acosf;
static double      (*volatile g_acos)(double)             = acos;
static long double (*volatile g_acosl)(long double)       = acosl;

static long g_checks   = 0;
static long g_failures = 0;

static
void d_tests_expect(
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

// bit-for-bit, NaN equal to NaN, -0 distinct from +0
static
bool d_tests_same_f(
    float _a,
    float _b
)
{
    uint32_t a = 0u;
    uint32_t b = 0u;

    memcpy(&a, &_a, sizeof(a));
    memcpy(&b, &_b, sizeof(b));

    return ((a == b) || (isnan(_a) && isnan(_b)));
}

static
bool d_tests_same_d(
    double _a,
    double _b
)
{
    return ( (memcmp(&_a, &_b, sizeof(_a)) == 0) ||
             (isnan(_a) && isnan(_b)) );
}

static
bool d_tests_same_ld(
    long double _a,
    long double _b
)
{
    // signbit's value is any nonzero for a negative: glibc's gives 1 for a
    // constant it folds and 512 for a long double at run time
    return ( ((_a == _b) && ((signbit(_a) != 0) == (signbit(_b) != 0))) ||
             (isnan(_a) && isnan(_b)) );
}

static
void d_tests_sqrt_float(
    bool _exhaustive
)
{
    const uint32_t step = _exhaustive ? 1u : 37u;
    uint32_t       bits = 0u;
    float          x    = 1.0f;

    // every float in [1, 4): every mantissa the unit root sees, both binades
    while (x < 4.0f)
    {
        D_TESTS_EXPECT(d_tests_same_f(d_math_f_sqrt_c(x), g_sqrtf(x)),
                       "f sqrt_c, [1, 4)", x);
        x = nextafterf(x, 5.0f);
    }

    // one pattern in 37 across all of them, or every one
    for (bits = 0u; ; bits += step)
    {
        float f = 0.0f;

        memcpy(&f, &bits, sizeof(f));
        D_TESTS_EXPECT(d_tests_same_f(d_math_f_sqrt_c(f), g_sqrtf(f)),
                       "f sqrt_c", f);

        // to the last pattern a step reaches below 2^32
        if (bits > (0xFFFFFFFFu - step))
        {
            break;
        }
    }

    return;
}

static
void d_tests_sqrt_double(void)
{
    long   i  = 0;
    double p2 = DBL_MIN * DBL_EPSILON;

    // random patterns, all signs and exponents
    for (i = 0; i < 10000000; ++i)
    {
        double d = 0.0;

        d_tests_pattern_d(&d, 0);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_sqrt_c(d), g_sqrt(d)),
                       "d sqrt_c", d);
    }

    // squares of random doubles in [1, 2), and their neighbours: the values
    // whose roots lie nearest a rounding midpoint
    for (i = 0; i < 2000000; ++i)
    {
        const double c  = 1.0 + ((double)d_tests_top(53) * DBL_EPSILON);
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

static
void d_tests_sqrt_long_double(void)
{
    long i = 0;

    for (i = 0; i < 2000000; ++i)
    {
        // 2^64
        const long double m    = 0.5L + (d_tests_top(63) /
                                         18446744073709551616.0L);
        const int         e    = (int)d_tests_below(32760u) - 16380;
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

// the distance of _got from _want in units of _want's last place, the
// subnormals' spacing below the normal range (the format told by its
// precision); an infinity or NaN is 0 from its like and 1e30 from anything
// else
static
double d_tests_ulps(
    long double _got,
    long double _want,
    int         _mant
)
{
    const int   emin  = (_mant == FLT_MANT_DIG) ? (FLT_MIN_EXP - 1)
                      : (_mant == DBL_MANT_DIG) ? (DBL_MIN_EXP - 1)
                                                : (LDBL_MIN_EXP - 1);
    long double scale = 0.0L;
    long double diff  = 0.0L;
    int         e     = 0;

    if (isnan(_got) || isnan(_want) || isinf(_got) || isinf(_want))
    {
        return d_tests_same_ld(_got, _want) ? 0.0 : 1e30;
    }

    e     = (_want == 0.0L) ? emin : ilogbl(_want);
    e     = (e < emin) ? emin : e;
    scale = ldexpl(1.0L, e - (_mant - 1));
    diff  = (_got > _want) ? (_got - _want) : (_want - _got);

    return (double)(diff / scale);
}

// d_tests_arcs_float
//   asin_c and acos_c for float, at one pattern in 1009 across all of them,
// or every one: the long double library's result rounded to float -- the
// correctly rounded one wherever long double has eleven bits more than
// float -- and within an ulp of asinf and acosf; NaN past +-1.
static
void d_tests_arcs_float(
    bool _exhaustive
)
{
    const uint32_t step = _exhaustive ? 1u : 1009u;
    uint32_t       bits = 0u;

    for (bits = 0u; ; bits += step)
    {
        float f  = 0.0f;
        float as = 0.0f;
        float ac = 0.0f;

        memcpy(&f, &bits, sizeof(f));
        as = d_math_f_asin_c(f);
        ac = d_math_f_acos_c(f);

        D_TESTS_EXPECT(d_tests_same_f(as, (float)g_asinl(f)),
                       "f asin_c correctly rounded", f);
        D_TESTS_EXPECT(d_tests_same_f(ac, (float)g_acosl(f)),
                       "f acos_c correctly rounded", f);
        D_TESTS_EXPECT( (d_tests_ulps(as, g_asinf(f), FLT_MANT_DIG) <= 1.0) &&
                        (d_tests_ulps(ac, g_acosf(f), FLT_MANT_DIG) <= 1.0),
                        "f asin_c and acos_c within an ulp of asinf, acosf",
                        f);

        // to the last pattern a step reaches below 2^32
        if (bits > (0xFFFFFFFFu - step))
        {
            break;
        }
    }

    return;
}

// d_tests_arcs_double
//   asin_c and acos_c within an ulp of asin and acos at two million
// arguments: a third uniform in [-1, 1), a third within 2^-2 to 2^-53 of
// +-1, where the cathetus sqrt(1 - x^2) is small, and a third from 2^-2
// down to the least subnormal, either sign; prints how many differ at all,
// and the largest difference.
static
void d_tests_arcs_double(void)
{
    double worst_s = 0.0;
    double worst_c = 0.0;
    long   diff_s  = 0;
    long   diff_c  = 0;
    long   i       = 0;

    for (i = 0; i < 2000000; ++i)
    {
        const double sign = (d_tests_bit() != 0u) ? -1.0 : 1.0;
        // in [0, 1), 53 random bits
        const double m    = (double)d_tests_top(53) *
                            DBL_EPSILON / 2.0;
        double       x    = 0.0;
        double       e    = 0.0;

        switch (i % 3)
        {
            case 0:
                x = (2.0 * m) - 1.0;

                break;

            case 1:
                x = sign * (1.0 - ldexp(1.0 + m,
                                        -2 - (int)d_tests_below(52u)));

                break;

            default:
                x = sign * ldexp(1.0 + m,
                                 -2 - (int)d_tests_below(1074u));

                break;
        }

        // stored, so x is a double: clang on x87 hands an inlined kernel
        // the 64-bit register value otherwise, and the library the double
        {
            volatile double stored = x;

            x = stored;
        }

        e = d_tests_ulps(d_math_d_asin_c(x), g_asin(x), DBL_MANT_DIG);
        worst_s = (e > worst_s) ? e : worst_s;
        diff_s += (e > 0.0) ? 1 : 0;
        D_TESTS_EXPECT(e <= 1.0, "d asin_c within an ulp of asin", x);

        e = d_tests_ulps(d_math_d_acos_c(x), g_acos(x), DBL_MANT_DIG);
        worst_c = (e > worst_c) ? e : worst_c;
        diff_c += (e > 0.0) ? 1 : 0;
        D_TESTS_EXPECT(e <= 1.0, "d acos_c within an ulp of acos", x);
    }

    printf("double: asin_c differs from asin at %ld of 2000000 arguments, by "
           "%.3g ulps at most; acos_c from acos at %ld, by %.3g\n",
           diff_s, worst_s, diff_c, worst_c);

    return;
}

// d_tests_arcs_edges
//   the correctly rounded values, from a 400-bit computation (the slice's
// tools/edge_values5.py): about sqrt(1/2), where the kernel's two ways of
// reducing meet; pi/6, pi/3 and others; the double below 1, the small and
// the subnormal; and the arguments whose result, rounded to long double,
// lies on a double's midpoint. asin_c is checked with x negated too, which
// negates the result.
static
void d_tests_arcs_edges(void)
{
    // x and asin(x), correctly rounded
    static const double sines[][2] = {
        // about the reduction's middle, sqrt(1/2)
        { 0.7071067811865476,      0.7853981633974484     },
        { 0.7071067811865475,      0.7853981633974482     },
        // pi/6 and a few more
        { 0.5,                     0.5235987755982989     },
        { 0.25,                    0.25268025514207865    },
        { 0.75,                    0.848062078981481      },
        { 0.1,                     0.1001674211615598     },
        { 0.9,                     1.1197695149986342     },
        // the double below 1; the small and the subnormal, their own arc-sine
        { 0.9999999999999999,      1.5707963118937354     },
        { 1e-08,                   1e-08                  },
        { 1e-300,                  1e-300                 },
        { 5e-324,                  5e-324                 },
        // the long double result lies on a double's midpoint
        { 1.432274675006131e-07,   1.4322746750061361e-07 },
        { 1.2894733472766593e-07,  1.289473347276663e-07  },
        { 3.9050447212288085e-08,  3.905044721228809e-08  },
        { 1.974912425181723e-07,   1.974912425181736e-07  },
        { 8.021549786177775e-08,   8.021549786177783e-08  },
        { 2.970267464970435e-07,   2.970267464970479e-07  }
    };
    // x, acos(x) and acos(-x), correctly rounded
    static const double cosines[][3] = {
        // about the reduction's middle, sqrt(1/2)
        { 0.7071067811865476,  0.7853981633974483,      2.356194490192345  },
        { 0.7071067811865475,  0.7853981633974484,      2.356194490192345  },
        // pi/3 and 2 pi/3, and a few more
        { 0.5,                 1.0471975511965979,      2.0943951023931957 },
        { 0.25,                1.318116071652818,       1.8234765819369754 },
        { 0.75,                0.7227342478134157,      2.4188584057763776 },
        { 0.1,                 1.4706289056333368,      1.6709637479564565 },
        { 0.9,                 0.45102681179626236,     2.6905658417935308 },
        // the double below 1, the small and the subnormal: pi/2 less them
        { 0.9999999999999999,  1.4901161193847656e-08,  3.141592638688632  },
        { 1e-08,               1.5707963167948966,      1.5707963367948967 },
        { 1e-300,              1.5707963267948966,      1.5707963267948966 },
        { 5e-324,              1.5707963267948966,      1.5707963267948966 },
        // the long double result lies on a double's midpoint (at x)
        { 0.9999999999998179,  6.034510353600054e-07,   3.1415920501387578 },
        { 0.9999999999988983,  1.4843680859785048e-06,  3.1415911692217073 }
    };
    size_t i = 0;

    for (i = 0; i < (sizeof(sines) / sizeof(sines[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_asin_c(sines[i][0]),
                                      sines[i][1]),
                       "d asin_c correctly rounded at an edge", sines[i][0]);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_asin_c(-sines[i][0]),
                                      -sines[i][1]),
                       "d asin_c odd at an edge", sines[i][0]);
    }

    for (i = 0; i < (sizeof(cosines) / sizeof(cosines[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_acos_c(cosines[i][0]),
                                      cosines[i][1]),
                       "d acos_c correctly rounded at an edge",
                       cosines[i][0]);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_acos_c(-cosines[i][0]),
                                      cosines[i][2]),
                       "d acos_c correctly rounded at a negated edge",
                       cosines[i][0]);
    }

    return;
}

// d_tests_arcs_long_double
//   asin_c within an ulp of asinl and acos_c within two of acosl (glibc's
// acosl is 1.54 ulps off near 1, where acos_c is within half of one) at
// half a million arguments, every bit random, drawn as for double.
static
void d_tests_arcs_long_double(void)
{
    // the exponents from 2^-2 to a long double's least subnormal, and to
    // 2^-P
    const unsigned long span    = (unsigned long)(LDBL_MANT_DIG -
                                                  LDBL_MIN_EXP + 1);
    const unsigned long digits  = (unsigned long)(LDBL_MANT_DIG - 1);
    double              worst_s = 0.0;
    double              worst_c = 0.0;
    long                i       = 0;

    for (i = 0; i < 500000; ++i)
    {
        const long double sign = (d_tests_bit() != 0u) ? -1.0L : 1.0L;
        // in [0, 1), 106 random bits
        const long double m    =
            (d_tests_top(53) * DBL_EPSILON / 2.0L) +
            (d_tests_top(53) * DBL_EPSILON * DBL_EPSILON / 4.0L);
        long double       x    = 0.0L;
        double            e    = 0.0;

        switch (i % 3)
        {
            case 0:
                x = (2.0L * m) - 1.0L;

                break;

            case 1:
                x = sign * (1.0L - ldexpl(1.0L + m,
                                          -2 - (int)d_tests_below(digits)));

                break;

            default:
                x = sign * ldexpl(1.0L + m,
                                  -2 - (int)d_tests_below(span));

                break;
        }

        e = d_tests_ulps(d_math_ld_asin_c(x), g_asinl(x), LDBL_MANT_DIG);
        worst_s = (e > worst_s) ? e : worst_s;
        D_TESTS_EXPECT(e <= 1.0, "ld asin_c within an ulp of asinl", x);

        e = d_tests_ulps(d_math_ld_acos_c(x), g_acosl(x), LDBL_MANT_DIG);
        worst_c = (e > worst_c) ? e : worst_c;
        D_TESTS_EXPECT(e <= 2.0, "ld acos_c within two ulps of acosl", x);
    }

    printf("long double: asin_c within %.3g ulps of asinl, acos_c within "
           "%.3g of acosl\n", worst_s, worst_c);

    return;
}

// d_tests_arcs_specials
//   for every family: NaN gives NaN, as do the infinities and the values
// past +-1; asin_c gives a zero or a subnormal back, sign and all, and
// +-pi/2 at +-1; acos_c gives pi/2 at the zeros and the subnormals, a zero
// at 1 and pi at -1. Float and double against pi and pi/2, rounded; long
// double against asinl and acosl.
static
void d_tests_arcs_specials(void)
{
    // for each kind of argument -- 0 a zero, 1 the least subnormal, 2 one,
    // 3 the next value past one, 4 infinity, 5 NaN -- the kinds of asin's
    // result (odd), acos's and acos's at the argument negated: 0 the
    // argument itself (asin) or a zero (acos), 1 pi/2, 2 pi, 3 NaN
    static const int kinds[6][3] = {
        { 0, 1, 1 },  { 0, 1, 1 },  { 1, 0, 2 },
        { 3, 3, 3 },  { 3, 3, 3 },  { 3, 3, 3 }
    };
    const float       f_arg[6] = { 0.0f, FLT_MIN * FLT_EPSILON, 1.0f,
                                   1.0f + FLT_EPSILON, (float)HUGE_VAL,
                                   nanf("") };
    const float       f_res[4] = { 0.0f, 1.57079637f, 3.14159274f,
                                   nanf("") };
    const double      d_arg[6] = { 0.0, DBL_MIN * DBL_EPSILON, 1.0,
                                   1.0 + DBL_EPSILON, (double)HUGE_VAL,
                                   nan("") };
    const double      d_res[4] = { 0.0, 1.5707963267948966, 3.141592653589793,
                                   nan("") };
    const long double l_arg[6] = { 0.0L, LDBL_MIN * LDBL_EPSILON, 1.0L,
                                   1.0L + LDBL_EPSILON, (long double)HUGE_VAL,
                                   nanl("") };
    size_t i = 0;

    for (i = 0; i < 6; ++i)
    {
        const float       xf = f_arg[i];
        const float       sf = (kinds[i][0] == 0) ? xf : f_res[kinds[i][0]];
        const double      xd = d_arg[i];
        const double      sd = (kinds[i][0] == 0) ? xd : d_res[kinds[i][0]];
        const long double xl = l_arg[i];

        D_TESTS_EXPECT( (d_tests_same_f(d_math_f_asin_c(xf), sf)) &&
                        (d_tests_same_f(d_math_f_asin_c(-xf), -sf)) &&
                        (d_tests_same_f(d_math_f_acos_c(xf),
                                        f_res[kinds[i][1]])) &&
                        (d_tests_same_f(d_math_f_acos_c(-xf),
                                        f_res[kinds[i][2]])),
                        "f asin_c and acos_c's special cases (kind)", i);
        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_asin_c(xd), sd)) &&
                        (d_tests_same_d(d_math_d_asin_c(-xd), -sd)) &&
                        (d_tests_same_d(d_math_d_acos_c(xd),
                                        d_res[kinds[i][1]])) &&
                        (d_tests_same_d(d_math_d_acos_c(-xd),
                                        d_res[kinds[i][2]])),
                        "d asin_c and acos_c's special cases (kind)", i);
        D_TESTS_EXPECT( (d_tests_same_ld(d_math_ld_asin_c(xl), g_asinl(xl))) &&
                        (d_tests_same_ld(d_math_ld_asin_c(-xl),
                                         g_asinl(-xl))) &&
                        (d_tests_same_ld(d_math_ld_acos_c(xl), g_acosl(xl))) &&
                        (d_tests_same_ld(d_math_ld_acos_c(-xl),
                                         g_acosl(-xl))),
                        "ld asin_c and acos_c's special cases (kind)", i);
    }

    // the plain forms are the C library's
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_asin(0.5), g_asin(0.5)),
                   "d asin", 0);
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_acos(0.5f), g_acosf(0.5f)),
                   "f acos", 0);

    return;
}

// d_tests_trig
//   sin_c and cos_c against sin and cos: within an ulp for float and double
// and two for long double, below 2^20, over small, moderate and large
// arguments and the multiples of pi/2 the reduction must not lose; the
// largest error is printed. (d_tests_trig_far goes on from 2^20.)
static
void d_tests_trig(void)
{
    double worst[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    long   i        = 0;

    for (i = 0; i < 3000000; ++i)
    {
        const double   u     = ((double)d_tests_top(53) * DBL_EPSILON) - 1.0;
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

// d_tests_trig_far
//   sin_c and cos_c from 2^20 to the largest finite value, every family,
// within an ulp of sin and cos (two for long double); every float pattern in
// 2003, the correctly rounded value; and the double edges, exact.
static
void d_tests_trig_far(
    bool _exhaustive
)
{
    // x, sin x, cos x, correctly rounded, from a 400-bit computation (the
    // slice's tools/edge_values.py)
    static const double edges[][3] = {
        // the reduction's earlier failures: wrong digits from 2^20, an
        // infinity from 2^63, NaN past 2^1023
        { 1e15,     0.8582727931702359,   -0.5131937377869703    },
        { 7e18,     0.6744688419011363,   -0.7383033125379704    },
        { 1e19,    -0.9270631660486504,   -0.37490516955071784   },
        { 1e22,    -0.8522008497671888,    0.523214785395139     },
        { 1e100,   -0.3806377310050287,    0.9247242387519338    },
        { 1e300,   -0.8178819121159085,   -0.5753861119575491    },
        // 2^1023, and the largest double
        { 8.98846567431158e+307,     0.563127779850884,
                                    -0.826369834614148     },
        { 1.7976931348623157e+308,   0.004961954789184062,
                                    -0.9999876894265599    },
        // the double nearest a multiple of pi/2, 6381956970095103 2^797,
        // to 25 digits: its 16 lie so near a midpoint that a long double
        // evaluation of the literal (FLT_EVAL_METHOD 2) rounds it to the
        // double below
        { 5.319372648326541416707297e+255, 1.0,
                                    -4.687165924254628e-19 },
        // below 2^20 quarter turns, but within 2^-53 of a multiple of pi/2
        // (29 pi/2, 1856 pi/2): the exact reduction's too
        { 45.553093477052,           1.0,
                                    -6.189806365883577e-19 },
        { 2915.397982531328,         3.9614760741654893e-17,
                                     1.0                   }
    };
    // x and sin x, x and cos x, where the long double result lies on a
    // double's midpoint, so that converting it rounds the wrong way
    static const double sines[][2] = {
        { 3.9050447212288085e-08, 3.905044721228808e-08  },
        { 3.9051328764929894e-08, 3.905132876492989e-08  },
        { 6.525866827134926e-08,  6.525866827134922e-08  },
        { 1.7963834392897238e-07, 1.796383439289714e-07  },
        { 5.1795874429443443e-08, 5.1795874429443423e-08 }
    };
    static const double cosines[][2] = {
        { 1.0538015288394659e-08, 0.9999999999999999     },
        { 1.0536980772906694e-08, 0.9999999999999999     },
        { 4.080801148160136e-08,  0.9999999999999992     },
        { 3.79910611497511e-08,   0.9999999999999992     }
    };
    const uint32_t step     = _exhaustive ? 1u : 2003u;
    double         worst[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    uint32_t       bits     = 0u;
    size_t         j        = 0;
    long           i        = 0;

    for (i = 0; i < 400000; ++i)
    {
        const double      sign = (d_tests_bit() != 0u) ? -1.0 : 1.0;
        const double      m    = 1.0 + ((double)d_tests_top(53) *
                                        DBL_EPSILON / 2.0);
        const float       xf   =
            (float)(sign * ldexp(m, 20 + (int)d_tests_below(108u)));
        const double      x    =
            sign * ldexp(m, 20 + (int)d_tests_below(1004u));
        const long double xl   =
            (long double)sign *
            ldexpl((long double)m + (d_tests_top(53) * LDBL_EPSILON),
                   20 + (int)d_tests_below((unsigned long)(LDBL_MAX_EXP - 20)));
        const double      e[6] = {
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
                        "f sin_c and cos_c within an ulp, past 2^20", xf);
        D_TESTS_EXPECT( (e[2] <= 1.0) && (e[3] <= 1.0),
                        "d sin_c and cos_c within an ulp, past 2^20", x);
        D_TESTS_EXPECT( (e[4] <= 2.0) && (e[5] <= 2.0),
                        "ld sin_c and cos_c within two ulps, past 2^20", xl);
    }

    // every float pattern in 2003, or every one: the long double library's
    // result rounded to float is the correctly rounded one wherever long
    // double has eleven bits more than float
    for (bits = 0u; ; bits += step)
    {
        float f = 0.0f;

        memcpy(&f, &bits, sizeof(f));
        D_TESTS_EXPECT(d_tests_same_f(d_math_f_sin_c(f), (float)g_sinl(f)),
                       "f sin_c correctly rounded", f);
        D_TESTS_EXPECT(d_tests_same_f(d_math_f_cos_c(f), (float)g_cosl(f)),
                       "f cos_c correctly rounded", f);

        // to the last pattern a step reaches below 2^32
        if (bits > (0xFFFFFFFFu - step))
        {
            break;
        }
    }

    // the double edges, both signs
    for (j = 0; j < (sizeof(edges) / sizeof(edges[0])); ++j)
    {
        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_sin_c(edges[j][0]),
                                        edges[j][1])) &&
                        (d_tests_same_d(d_math_d_sin_c(-edges[j][0]),
                                        -edges[j][1])) &&
                        (d_tests_same_d(d_math_d_cos_c(edges[j][0]),
                                        edges[j][2])) &&
                        (d_tests_same_d(d_math_d_cos_c(-edges[j][0]),
                                        edges[j][2])),
                        "d sin_c and cos_c correctly rounded far out",
                        edges[j][0]);
    }

    for (j = 0; j < (sizeof(sines) / sizeof(sines[0])); ++j)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_sin_c(sines[j][0]),
                                      sines[j][1]),
                       "d sin_c correctly rounded at a midpoint",
                       sines[j][0]);
    }

    for (j = 0; j < (sizeof(cosines) / sizeof(cosines[0])); ++j)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_cos_c(cosines[j][0]),
                                      cosines[j][1]),
                       "d cos_c correctly rounded at a midpoint",
                       cosines[j][0]);
    }

    // the largest values of every family
    D_TESTS_EXPECT( (d_tests_ulps(d_math_f_sin_c(FLT_MAX), g_sinf(FLT_MAX),
                                  24) <= 1.0) &&
                    (d_tests_ulps(d_math_ld_sin_c(LDBL_MAX),
                                  g_sinl(LDBL_MAX), LDBL_MANT_DIG) <= 2.0) &&
                    (d_tests_ulps(d_math_ld_cos_c(-LDBL_MAX),
                                  g_cosl(-LDBL_MAX), LDBL_MANT_DIG) <= 2.0),
                    "sin_c and cos_c at the largest values", 0);

    printf("sin_c/cos_c past 2^20, largest error: float %.3g/%.3g, double "
           "%.3g/%.3g, long double %.3g/%.3g ulps\n",
           worst[0], worst[1], worst[2], worst[3], worst[4], worst[5]);

    return;
}

// d_tests_newton_f, d_tests_newton_d
//   the root sqrt_unit's Newton iteration gives _m in [1, 4), stored so that
// it is a value of the family.
static
float d_tests_newton_f(
    float _m
)
{
    volatile float root = 0.6f + (0.4f * _m);
    int            i    = 0;

    for (i = 0; i < 6; ++i)
    {
        root = 0.5f * (root + (_m / root));
    }

    return root;
}

static
double d_tests_newton_d(
    double _m
)
{
    volatile double root = 0.6 + (0.4 * _m);
    int             i    = 0;

    for (i = 0; i < 6; ++i)
    {
        root = 0.5 * (root + (_m / root));
    }

    return root;
}

// d_tests_sqrt_digits
//   sqrt_settle_digits -- the last bit settled in digits of 16 bits, which
// sqrt_unit takes where two dj_math_umax limbs do not hold (2^(P+1))^2: a
// double where dj_math_umax has 32 bits -- called directly, so that every
// build checks it: the library's root, bit for bit, settled from Newton's
// root and from it a place and two either side. For float, every sixteenth
// float in [1, 4), and every sixty-fourth from the places either side; for
// double, 250,000 random values there, and 50,000 squares of values in [1,
// 2) and their neighbours, whose roots lie about half a place from those
// values. It draws after the earlier checks, so that their draws are as
// they were.
static
void d_tests_sqrt_digits(void)
{
    float m     = 1.0f;
    long  i     = 0;
    long  count = 0;
    int   k     = 0;

    for (m = 1.0f; m < 4.0f; m = nextafterf(m, 5.0f), ++count)
    {
        const float start = d_tests_newton_f(m);

        if ((count % 16) == 0)
        {
            D_TESTS_EXPECT(d_tests_same_f(
                               d_internal_math_f_sqrt_settle_digits(m, start),
                               g_sqrtf(m)),
                           "f sqrt_settle_digits", m);
        }

        if ((count % 64) == 0)
        {
            const float down = nextafterf(start, 0.0f);
            const float up   = nextafterf(start, 4.0f);

            D_TESTS_EXPECT( (d_tests_same_f(
                                 d_internal_math_f_sqrt_settle_digits(
                                     m, nextafterf(down, 0.0f)),
                                 g_sqrtf(m))) &&
                            (d_tests_same_f(
                                 d_internal_math_f_sqrt_settle_digits(m,
                                                                      down),
                                 g_sqrtf(m))) &&
                            (d_tests_same_f(
                                 d_internal_math_f_sqrt_settle_digits(m, up),
                                 g_sqrtf(m))) &&
                            (d_tests_same_f(
                                 d_internal_math_f_sqrt_settle_digits(
                                     m, nextafterf(up, 4.0f)),
                                 g_sqrtf(m))),
                            "f sqrt_settle_digits from a place and two "
                            "either side", m);
        }
    }

    for (i = 0; i < 300000; ++i)
    {
        // in [0, 1), 53 random bits
        const double    unit   = (double)d_tests_top(53) * DBL_EPSILON / 2.0;
        // the square of a value in [1, 2), stored: its root lies within half
        // a place of that value, and its neighbours' about half a place
        volatile double square = (1.0 + unit) * (1.0 + unit);
        const double    sq     = square;
        // 250,000 random values in [1, 4); then 50,000 squares, with their
        // neighbours
        const double    values[3] = {
            (i >= 250000) ? sq
                          : (((i % 2) == 0) ? (1.0 + unit)
                                            : (2.0 + (2.0 * unit))),
            nextafter(sq, 4.0),
            nextafter(sq, 0.0)
        };
        const int       many   = (i >= 250000) ? 3 : 1;

        for (k = 0; k < many; ++k)
        {
            const double v     = values[k];
            const double start = d_tests_newton_d(v);
            const double down  = nextafter(start, 0.0);
            const double up    = nextafter(start, 4.0);

            // the lower neighbour of 1 lies below the unit range
            if (v < 1.0)
            {
                continue;
            }

            D_TESTS_EXPECT( (d_tests_same_d(
                                 d_internal_math_d_sqrt_settle_digits(v,
                                                                      start),
                                 g_sqrt(v))) &&
                            (d_tests_same_d(
                                 d_internal_math_d_sqrt_settle_digits(
                                     v, nextafter(down, 0.0)),
                                 g_sqrt(v))) &&
                            (d_tests_same_d(
                                 d_internal_math_d_sqrt_settle_digits(v,
                                                                      down),
                                 g_sqrt(v))) &&
                            (d_tests_same_d(
                                 d_internal_math_d_sqrt_settle_digits(v, up),
                                 g_sqrt(v))) &&
                            (d_tests_same_d(
                                 d_internal_math_d_sqrt_settle_digits(
                                     v, nextafter(up, 4.0)),
                                 g_sqrt(v))),
                            "d sqrt_settle_digits from Newton's root, and a "
                            "place and two either side", v);
        }
    }

    return;
}

// d_tests_plain_roots
//   sqrt, the C library's root, against sqrt_c, bit for bit, as it comes
// back: taken straight into a long double, which keeps what the call left in
// the register -- on a 32-bit x87, glibc's sqrtf leaves 64 bits, which sqrt
// stores away (math_common.h's D_MATH_FLT_EVAL_WIDE). For float, one
// positive finite pattern in 4093; for double and long double, 500,000 and
// 200,000 random positive values, every exponent. It draws last, so that
// the earlier draws are as they were.
static
void d_tests_plain_roots(void)
{
    uint32_t bits = 0u;
    long     i    = 0;

    for (bits = 1u; bits < 0x7F800000u; bits += 4093u)
    {
        float f = 0.0f;

        memcpy(&f, &bits, sizeof(f));
        D_TESTS_EXPECT(d_tests_same_ld(d_math_f_sqrt(f), d_math_f_sqrt_c(f)),
                       "f sqrt is sqrt_c's as it comes back", f);
    }

    for (i = 0; i < 500000; ++i)
    {
        double d = 0.0;

        // a positive pattern, every exponent: +inf and NaN, past DBL_MAX,
        // are the specials' checks
        d_tests_pattern_d(&d, 1);

        if (d <= DBL_MAX)
        {
            D_TESTS_EXPECT(d_tests_same_ld(d_math_d_sqrt(d),
                                           d_math_d_sqrt_c(d)),
                           "d sqrt is sqrt_c's as it comes back", d);
        }
    }

    for (i = 0; i < 200000; ++i)
    {
        // 2^64
        const long double m = 0.5L + (d_tests_top(63) /
                                      18446744073709551616.0L);
        const int         e = (int)d_tests_below(32760u) - 16380;
        const long double x = ldexpl(m, e);

        D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_sqrt(x), d_math_ld_sqrt_c(x)),
                       "ld sqrt is sqrt_c's as it comes back", x);
    }

    return;
}

// d_tests_moved_f, d_tests_moved_d
//   _root moved _places places, up where _places is above 0 and down where it
// is below, stored so that it is a value of the family.
static
float d_tests_moved_f(
    float _root,
    int   _places
)
{
    volatile float moved = _root;
    int            i     = 0;

    for (i = 0; i < ((_places < 0) ? -_places : _places); ++i)
    {
        moved = nextafterf(moved, (_places < 0) ? 0.0f : 8.0f);
    }

    return moved;
}

static
double d_tests_moved_d(
    double _root,
    int    _places
)
{
    volatile double moved = _root;
    int             i     = 0;

    for (i = 0; i < ((_places < 0) ? -_places : _places); ++i)
    {
        moved = nextafter(moved, (_places < 0) ? 0.0 : 8.0);
    }

    return moved;
}

// d_tests_reach_f, d_tests_reach_d
//   whether sqrt_settle_digits gives the library's root of _m from that root
// moved 0 to 4 places either way: the whole reach of its loops, four steps
// up and four down.
static
bool d_tests_reach_f(
    float _m
)
{
    volatile float stored = g_sqrtf(_m);
    const float    root   = stored;
    bool           all    = true;
    int            k      = 0;

    for (k = -4; k <= 4; ++k)
    {
        if (!d_tests_same_f(d_internal_math_f_sqrt_settle_digits(
                                _m, d_tests_moved_f(root, k)),
                            root))
        {
            all = false;
        }
    }

    return all;
}

static
bool d_tests_reach_d(
    double _m
)
{
    volatile double stored = g_sqrt(_m);
    const double    root   = stored;
    bool            all    = true;
    int             k      = 0;

    for (k = -4; k <= 4; ++k)
    {
        if (!d_tests_same_d(d_internal_math_d_sqrt_settle_digits(
                                _m, d_tests_moved_d(root, k)),
                            root))
        {
            all = false;
        }
    }

    return all;
}

// d_tests_sqrt_reach
//   sqrt_settle_digits over the whole reach of its loops: from the library's
// root moved up to four places either way it must give the root -- its
// residual stays small enough there for the low digits it is taken in to
// keep its sign. Every sixty-fourth float in [1, 4) and 100,000 random
// doubles there; and, for both families, the edges: 1 and the value above
// it, 2 and the value below it, 2.25, the value below 4, and starts of 2 and
// past it, which the settle takes as the largest root below 2. It draws
// last, so that the earlier draws are as they were.
static
void d_tests_sqrt_reach(void)
{
    const float  edges_f[6]  = { 1.0f, 1.0000001f, 1.9999999f, 2.0f, 2.25f,
                                 3.9999998f };
    const double edges_d[6]  = { 1.0, 1.0000000000000002, 1.9999999999999998,
                                 2.0, 2.25, 3.9999999999999996 };
    const float  starts_f[3] = { 2.0f, 2.0000002f, 3.0f };
    const double starts_d[3] = { 2.0, 2.0000000000000004, 3.0 };
    float        m           = 1.0f;
    long         i           = 0;
    long         count       = 0;
    int          k           = 0;

    for (m = 1.0f; m < 4.0f; m = nextafterf(m, 5.0f), ++count)
    {
        if ((count % 64) == 0)
        {
            D_TESTS_EXPECT(d_tests_reach_f(m),
                           "f sqrt_settle_digits from the root, and up to four "
                           "places either side", m);
        }
    }

    for (i = 0; i < 100000; ++i)
    {
        // in [0, 1), 53 random bits; [1, 4), stored
        const double    unit   = (double)d_tests_top(53) * DBL_EPSILON / 2.0;
        volatile double stored = ((i % 2) == 0) ? (1.0 + unit)
                                                : (2.0 + (2.0 * unit));
        const double    v      = stored;

        D_TESTS_EXPECT(d_tests_reach_d(v),
                       "d sqrt_settle_digits from the root, and up to four "
                       "places either side", v);
    }

    for (k = 0; k < 6; ++k)
    {
        D_TESTS_EXPECT( (d_tests_reach_f(edges_f[k])) &&
                        (d_tests_reach_d(edges_d[k])),
                        "sqrt_settle_digits at the edges", edges_d[k]);
    }

    // below 4, from 2 and past it: the largest root below 2
    for (k = 0; k < 3; ++k)
    {
        D_TESTS_EXPECT( (d_tests_same_f(d_internal_math_f_sqrt_settle_digits(
                                            edges_f[5], starts_f[k]),
                                        g_sqrtf(edges_f[5]))) &&
                        (d_tests_same_d(d_internal_math_d_sqrt_settle_digits(
                                            edges_d[5], starts_d[k]),
                                        g_sqrt(edges_d[5]))),
                        "sqrt_settle_digits from 2 and past it", starts_d[k]);
    }

    return;
}

#if defined(__cplusplus)

// d_tests_overloads
//   compiled as C++: one name for every family, chosen by argument type.
static
void d_tests_overloads(void)
{
    D_TESTS_EXPECT( (d_tests_same_f(d_math_sqrt_c(2.0f),
                                    d_math_f_sqrt_c(2.0f))) &&
                    (d_tests_same_d(d_math_sqrt_c(2.0),
                                    d_math_d_sqrt_c(2.0))) &&
                    (d_tests_same_ld(d_math_sqrt_c(2.0L),
                                     d_math_ld_sqrt_c(2.0L))) &&
                    (d_tests_same_f(d_math_sin_c(0.5f),
                                    d_math_f_sin_c(0.5f))) &&
                    (d_tests_same_d(d_math_cos_c(0.5),
                                    d_math_d_cos_c(0.5))) &&
                    (d_tests_same_ld(d_math_sin_c(0.5L),
                                     d_math_ld_sin_c(0.5L))) &&
                    (d_tests_same_d(d_math_sqrt(2.0), g_sqrt(2.0))) &&
                    (d_tests_same_d(d_math_cos(0.5), g_cos(0.5))),
                    "the C++ overloads are the families' functions", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_asin_c(0.25f),
                                    d_math_f_asin_c(0.25f))) &&
                    (d_tests_same_d(d_math_asin_c(0.25),
                                    d_math_d_asin_c(0.25))) &&
                    (d_tests_same_ld(d_math_asin_c(0.25L),
                                     d_math_ld_asin_c(0.25L))) &&
                    (d_tests_same_f(d_math_acos_c(-0.25f),
                                    d_math_f_acos_c(-0.25f))) &&
                    (d_tests_same_d(d_math_acos_c(-0.25),
                                    d_math_d_acos_c(-0.25))) &&
                    (d_tests_same_ld(d_math_acos_c(-0.25L),
                                     d_math_ld_acos_c(-0.25L))) &&
                    (d_tests_same_d(d_math_asin(0.25), g_asin(0.25))) &&
                    (d_tests_same_f(d_math_acos(-0.25f), g_acosf(-0.25f))),
                    "the C++ overloads of asin and acos", 0);

    return;
}

#endif  // __cplusplus

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// from C++14, sin_c, cos_c, asin_c and acos_c are constant expressions at
// every finite argument, the largest of each family among them
static constexpr float       g_csin_f     = d_math_f_sin_c(FLT_MAX);
static constexpr double      g_csin_d     = d_math_d_sin_c(1e22);
static constexpr double      g_ccos_d     = d_math_d_cos_c(1e300);
static constexpr double      g_csin_dmax  = d_math_d_sin_c(DBL_MAX);
static constexpr double      g_csin_mid   =
    d_math_d_sin_c(3.9050447212288085e-08);
static constexpr long double g_csin_ld    = d_math_ld_sin_c(1e22L);
static constexpr long double g_ccos_ldmax = d_math_ld_cos_c(LDBL_MAX);
static constexpr float       g_casin_f    = d_math_f_asin_c(0.25f);
static constexpr double      g_casin_d    = d_math_d_asin_c(0.9);
static constexpr long double g_casin_ld   = d_math_ld_asin_c(-0.5L);
static constexpr double      g_casin_mid  =
    d_math_d_asin_c(1.432274675006131e-07);
static constexpr double      g_casin_sub  = d_math_d_asin_c(-5e-324);
static constexpr float       g_cacos_f    = d_math_f_acos_c(-0.75f);
static constexpr double      g_cacos_d    =
    d_math_d_acos_c(-0.9999999999999999);
static constexpr long double g_cacos_ld   = d_math_ld_acos_c(0.5L);
static constexpr double      g_cacos_mid  =
    d_math_d_acos_c(0.9999999999998179);

static_assert(d_math_d_sin_c(1e22) == (double)-0.8522008497671888,
              "sin_c(1e22)");
static_assert(d_math_d_cos_c(1e22) == (double)0.523214785395139,
              "cos_c(1e22)");
static_assert(d_math_d_sin_c(0.0) == 0.0, "sin_c(0) is 0");
static_assert(d_math_d_cos_c(0.0) == 1.0, "cos_c(0) is 1");
static_assert(d_math_d_asin_c(0.5) == (double)0.5235987755982989,
              "asin_c(1/2) is pi/6");
static_assert(d_math_d_asin_c(-1.0) == (double)-1.5707963267948966,
              "asin_c(-1) is -pi/2");
static_assert(d_math_d_acos_c(0.5) == (double)1.0471975511965979,
              "acos_c(1/2) is pi/3");
static_assert(d_math_d_acos_c(-1.0) == (double)3.141592653589793,
              "acos_c(-1) is pi");
static_assert(d_math_d_acos_c(1.0) == 0.0, "acos_c(1) is 0");
static_assert(d_math_d_asin_c(2.0) != d_math_d_asin_c(2.0),
              "asin_c(2) is NaN");
static_assert(d_math_asin_c(0.5f) > (float)0.5235f,
              "the C++ overloads of asin, float");

// and the last bit of a root settled in digits of 16 bits, which a double
// takes where dj_math_umax has 32 bits: from a place either side
static constexpr double      g_csettle_d  =
    d_internal_math_d_sqrt_settle_digits(2.0, 1.414213562373095);
static constexpr float       g_csettle_f  =
    d_internal_math_f_sqrt_settle_digits(3.0f, 1.7320509f);

static_assert(d_internal_math_d_sqrt_settle_digits(2.0, 1.414213562373095) ==
              (double)1.4142135623730951,
              "sqrt_settle_digits: the root of 2, from a place below");
static_assert(d_internal_math_d_sqrt_settle_digits(2.25,
                                                   1.4999999999999998) ==
              1.5,
              "sqrt_settle_digits: the root of 2.25 is 1.5");
static_assert(d_internal_math_f_sqrt_settle_digits(3.0f, 1.7320509f) ==
              (float)1.73205078f,
              "sqrt_settle_digits: the root of 3, float, from a place above");

// d_tests_constant_evaluation
//   the constants are the run time's results, bit for bit: the arguments
// are read through volatiles, so the calls below are not folded.
static
void d_tests_constant_evaluation(void)
{
    volatile float       fmax  = FLT_MAX;
    volatile double      e22   = 1e22;
    volatile double      e300  = 1e300;
    volatile double      dmax  = DBL_MAX;
    volatile double      mid   = 3.9050447212288085e-08;
    volatile long double le22  = 1e22L;
    volatile long double ldmax = LDBL_MAX;
    volatile float       qf    = 0.25f;
    volatile double      ninth = 0.9;
    volatile long double halfl = 0.5L;
    volatile double      mid_s = 1.432274675006131e-07;
    volatile double      least = 5e-324;
    volatile float       tq_f  = 0.75f;
    volatile double      below = 0.9999999999999999;
    volatile double      mid_c = 0.9999999999998179;
    volatile double      two   = 2.0;
    volatile float       three = 3.0f;

    D_TESTS_EXPECT( (d_tests_same_f(g_csin_f, d_math_f_sin_c(fmax))) &&
                    (d_tests_same_d(g_csin_d, d_math_d_sin_c(e22))) &&
                    (d_tests_same_d(g_ccos_d, d_math_d_cos_c(e300))) &&
                    (d_tests_same_d(g_csin_dmax, d_math_d_sin_c(dmax))) &&
                    (d_tests_same_d(g_csin_mid, d_math_d_sin_c(mid))) &&
                    (d_tests_same_ld(g_csin_ld, d_math_ld_sin_c(le22))) &&
                    (d_tests_same_ld(g_ccos_ldmax,
                                     d_math_ld_cos_c(ldmax))),
                    "constant evaluation agrees with the run time", 0);
    D_TESTS_EXPECT( (d_tests_same_f(g_casin_f, d_math_f_asin_c(qf))) &&
                    (d_tests_same_d(g_casin_d, d_math_d_asin_c(ninth))) &&
                    (d_tests_same_ld(g_casin_ld, d_math_ld_asin_c(-halfl))) &&
                    (d_tests_same_d(g_casin_mid, d_math_d_asin_c(mid_s))) &&
                    (d_tests_same_d(g_casin_sub, d_math_d_asin_c(-least))) &&
                    (d_tests_same_f(g_cacos_f, d_math_f_acos_c(-tq_f))) &&
                    (d_tests_same_d(g_cacos_d, d_math_d_acos_c(-below))) &&
                    (d_tests_same_ld(g_cacos_ld, d_math_ld_acos_c(halfl))) &&
                    (d_tests_same_d(g_cacos_mid, d_math_d_acos_c(mid_c))),
                    "constant evaluation agrees with the run time, asin and "
                    "acos", 0);
    D_TESTS_EXPECT( (d_tests_same_d(g_csettle_d,
                                    d_internal_math_d_sqrt_settle_digits(
                                        two, 1.414213562373095))) &&
                    (d_tests_same_f(g_csettle_f,
                                    d_internal_math_f_sqrt_settle_digits(
                                        three, 1.7320509f))),
                    "sqrt_settle_digits's constants agree with the run time",
                    0);

    return;
}

#endif  // C++14

int main(
    int    _argc,
    char** _argv
)
{
    const bool exhaustive = (_argc > 1) &&
                            (strcmp(_argv[1], "exhaustive") == 0);

    // the generator's start, the word 0x2545F4914F6CDD1D
    d_tests_seed(0x2545F491u, 0x4F6CDD1Du);
    d_tests_sqrt_float(exhaustive);
    d_tests_sqrt_double();
    d_tests_sqrt_long_double();
    d_tests_arcs_float(exhaustive);
    d_tests_arcs_double();
    d_tests_arcs_edges();
    d_tests_arcs_long_double();
    d_tests_arcs_specials();
    d_tests_trig();
    d_tests_trig_far(exhaustive);
    d_tests_sqrt_digits();
    d_tests_plain_roots();
    d_tests_sqrt_reach();
#if defined(__cplusplus)
    d_tests_overloads();
#endif
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    // the plain forms are the C library's
    D_TESTS_EXPECT(d_math_d_sqrt(2.0) == sqrt(2.0), "d sqrt", 0);

    printf("t_elementary: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
