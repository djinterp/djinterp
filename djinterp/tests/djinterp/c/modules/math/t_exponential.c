/*******************************************************************************
* djinterp [c][math]                                             t_exponential.c
*
*   Conformance harness for c/math/exponential.h. For float, exp_c and log_c
* must be the correctly rounded result - the long double library's, rounded
* to float - at one float pattern in 211 across the whole range, at every
* float in [1/2, 2) for log_c, where 1 + f is most structured, and at every
* float within one of the exponential's overflow and underflow thresholds;
* exp2_c, log2_c and log10_c at one pattern in 1009, at every float within
* 2^-7 of 1 for the logarithms and within one of exp2's thresholds, and at
* the powers of 2 and 10, which must be exact.
*
*   For double, exp_c is correctly rounded (S416): exactly e^x at 400 bits,
* rounded, at the 2398 arguments of math_tests_exp_refs.h - random ones over
* the whole range, ones within 2^-66 and 2^-102 of a rounding midpoint, the
* thresholds - and its three phases checked alone at each: a value the first
* or the second decides is the reference, and the third's is at every point.
* How many each decides is printed; the second and the third must each be
* reached. log_c likewise (S424), at the 2,354 arguments of
* math_tests_log_refs.h - random ones over the whole range, subnormals and
* arguments near 1 among them, ones within 2^-66 and 2^-99 of a midpoint, the
* edges - its phases each checked alone. exp_c's common path, 2^-53 < |x| <=
* 708 (S489), must give the three phases' value, bit for bit, at ten million
* random arguments and about its ends; exp_fast (S490) must be within an ulp
* of exp_c there and, where long double holds 64 bits, within 0.511 of an ulp
* of the exact value as expl gives it, and exp_c itself outside the common
* path. log_c's common path (S495) must give the three phases' value, bit for
* bit, and its reduction, from a double's bits, the one by frexp and dd.h's
* exact product, at ten million random arguments and about the reduction's
* seams; log_fast (S500) must be within an ulp of log_c there and, where long
* double holds 64 bits, within 0.52 of an ulp of the exact value as logl gives
* it near 1 and 0.53 elsewhere, within an ulp of log_c about its seams, and
* log_c itself where it hands over. The family's self-test must give 0
* (self_test.h). Ten million random
* arguments each: exp_c over its whole finite range and past both ends, log_c
* over random positive patterns (the subnormals included) and over 1 + d, d
* down to 2^-53. Both must be within an ulp of exp and log, the library's; how
* many differ at all is printed; and the plain exp and log must be exp_c and
* log_c, bit for bit (ruling 12, S478, S496). The edges - the last finite
* exponential, the last nonzero one, eight thousand doubles about each - and
* the specials must be exact, and so must the values a long double's double
* rounding would get wrong, such as exp(2^-53) and log(1 - 2^-53). Where
* double is evaluated wider than double, a 32-bit x87, the double family's
* exp_c and log_c refuse to compile (S418): their checks are skipped, the
* harness says so, and the plain exp and log, and exp_fast and log_fast,
* must be the library's (S479, S490, S497, S501). exp2_c, log2_c and log10_c, three million
* arguments each, must be within three ulps of exp2, log2 and log10 (glibc's
* log10 is up to two from the correctly rounded value); exact against values
* from a 400-bit computation - the ends of exp2's range and the tie at half
* the least subnormal, the logarithms' ends, and six arguments each whose
* long double result lies on a double's midpoint - and at every power of 2,
* and every power of 10 a double holds. For long double, two million random
* arguments each for exp_c and log_c, within an ulp of expl and logl; a
* million each for the others, within three ulps of exp2l, log2l and log10l,
* and the powers exact.
*
*   log1p_c and expm1_c, which complex numbers need: within an ulp of the
* correctly rounded value at every point of math_tests_real_refs.h, in each
* family, and the plain forms, the C library's, within three; the long double
* _c forms within 0.53 of an ulp of the exact value at its points of long
* double's full width, the kernels' bound, as exp_c is at its own, its core
* taking expm1's series; within an ulp of the long double library's value
* rounded to double at 300,000 random doubles each, and rounded to float at
* one float pattern in 4099; within three ulps of log1pl and expm1l at
* 100,000 random long doubles each; Annex F's values; the plain forms the C
* library's, bit for bit, and before C++11 the _c forms.
*
*   With the argument "exhaustive" it checks every float, all 2^32 patterns
* (its count wants a 64-bit long). Compiled as C++ from C++14, it checks too
* that all seven functions are constant expressions, for every family, with
* the bits they have at run time, double exp_c and log_c through each of
* their phases.
* It needs no 64-bit integer - its random
* numbers come from math_tests_random.h, the same in every build - so that
* ISO-strict C++98 on a 32-bit target, where dj_math_umax has 32 bits, builds
* it too.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_exponential.c -o t_exponential -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_exponential.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.08
*                                                            revised: 2026.11.25
*******************************************************************************/
// std
#include <float.h>   // FLT_MAX, DBL_MAX, LDBL_MANT_DIG
#include <math.h>    // expf ... log10l, nextafter
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/decimal.h"      // d_math_decimal_to_f
#include "../../../../../inc/djinterp/c/modules/math/exponential.h"  // unit under test
#include "../../../../../inc/djinterp/c/modules/math/self_test.h"    // d_math_self_test
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
// tests
#include "./math_tests_exp_refs.h"   // g_ref_exp_d_random ... _edges
#include "./math_tests_log_refs.h"   // g_ref_log_d_random ... _edges
#include "./math_tests_random.h"     // d_tests_seed, d_tests_below ...
#include "./math_tests_real_refs.h"  // g_ref_log1p ... g_wide_exp


// the C library's functions, called through volatile pointers, so that no
// compiler substitutes its own evaluation (an x87's, at a 32-bit target)
static float       (*volatile g_expf)(float)             = expf;
static double      (*volatile g_exp)(double)             = exp;
static long double (*volatile g_expl)(long double)       = expl;
static float       (*volatile g_logf)(float)             = logf;
static double      (*volatile g_log)(double)             = log;
static long double (*volatile g_logl)(long double)       = logl;
static float       (*volatile g_exp2f)(float)            = exp2f;
static double      (*volatile g_exp2)(double)            = exp2;
static long double (*volatile g_exp2l)(long double)      = exp2l;
static float       (*volatile g_log2f)(float)            = log2f;
static double      (*volatile g_log2)(double)            = log2;
static long double (*volatile g_log2l)(long double)      = log2l;
static float       (*volatile g_log10f)(float)           = log10f;
static double      (*volatile g_log10)(double)           = log10;
static long double (*volatile g_log10l)(long double)     = log10l;
static long double (*volatile g_log1pl)(long double)     = log1pl;
static long double (*volatile g_expm1l)(long double)     = expm1l;
// the float and double log1p and expm1, which the plain forms are: but
// before C++11, where the plain forms are the _c ones, compared with those
#if !( defined(__cplusplus) &&                                                \
       !D_ENV_LANG_IS_CPP11_OR_HIGHER )
static float       (*volatile g_log1pf)(float)           = log1pf;
static double      (*volatile g_log1p)(double)           = log1p;
static float       (*volatile g_expm1f)(float)           = expm1f;
static double      (*volatile g_expm1)(double)           = expm1;
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

// the number of elements of an array
#define D_TESTS_COUNT(array)                                                   \
    (sizeof(array) / sizeof((array)[0]))

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
//   one float: exp_c and log_c against the long double library's result,
// rounded to float, which is the correctly rounded one wherever long double
// has eleven bits more than float; and within an ulp of expf and logf.
static void
d_tests_float_one(float _x, bool _exp, bool _log)
{
    if (_exp)
    {
        const float got = d_math_f_exp_c(_x);

        D_TESTS_EXPECT(d_tests_same_f(got, (float)g_expl(_x)),
                       "f exp_c correctly rounded", _x);
        D_TESTS_EXPECT(D_TESTS_ULPS_F(got, g_expf(_x)) <= 1.0,
                       "f exp_c within an ulp of expf", _x);
    }

    // the long double library's logarithm of a negative float is NaN too
    if (_log)
    {
        const float got = d_math_f_log_c(_x);

        D_TESTS_EXPECT(d_tests_same_f(got, (float)g_logl(_x)),
                       "f log_c correctly rounded", _x);
        D_TESTS_EXPECT(D_TESTS_ULPS_F(got, g_logf(_x)) <= 1.0,
                       "f log_c within an ulp of logf", _x);
    }

    return;
}

static void
d_tests_float(bool _exhaustive)
{
    const uint32_t step = _exhaustive ? 1u : 211u;
    uint32_t       bits = 0u;
    float          x    = 0.5f;

    // one pattern in 211 across all of them, or every one
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

    // every float in [1/2, 2): log(1 + f) where f is most structured
    for (x = 0.5f; x < 2.0f; x = nextafterf(x, 3.0f))
    {
        d_tests_float_one(x, false, true);
    }

    // every float within one of the overflow threshold (88.72) and of the
    // underflow one (-103.97)
    for (x = 87.75f; x < 89.75f; x = nextafterf(x, 90.0f))
    {
        d_tests_float_one(x, true, false);
    }

    for (x = -105.0f; x < -103.0f; x = nextafterf(x, 0.0f))
    {
        d_tests_float_one(x, true, false);
    }

    return;
}

// d_tests_float_base_one
//   one float: exp2_c, log2_c and log10_c against the long double library's
// results, rounded to float; and within an ulp of exp2f and log2f, two of
// log10f (glibc's is not always within one).
static void
d_tests_float_base_one(float _x, bool _exp2, bool _logs)
{
    if (_exp2)
    {
        const float got = d_math_f_exp2_c(_x);

        D_TESTS_EXPECT(d_tests_same_f(got, (float)g_exp2l(_x)),
                       "f exp2_c correctly rounded", _x);
        D_TESTS_EXPECT(D_TESTS_ULPS_F(got, g_exp2f(_x)) <= 1.0,
                       "f exp2_c within an ulp of exp2f", _x);
    }

    if (_logs)
    {
        const float got2  = d_math_f_log2_c(_x);
        const float got10 = d_math_f_log10_c(_x);

        D_TESTS_EXPECT(d_tests_same_f(got2, (float)g_log2l(_x)),
                       "f log2_c correctly rounded", _x);
        D_TESTS_EXPECT(d_tests_same_f(got10, (float)g_log10l(_x)),
                       "f log10_c correctly rounded", _x);
        D_TESTS_EXPECT( (D_TESTS_ULPS_F(got2, g_log2f(_x)) <= 1.0) &&
                        (D_TESTS_ULPS_F(got10, g_log10f(_x)) <= 2.0),
                        "f log2_c and log10_c within one and two ulps of "
                        "log2f and log10f", _x);
    }

    return;
}

static void
d_tests_float_bases(bool _exhaustive)
{
    const uint32_t step = _exhaustive ? 1u : 1009u;
    uint32_t       bits = 0u;
    float          x    = 0.0f;
    float          p    = 1.0f;
    int            n    = 0;

    // one pattern in 1009 across all of them, or every one
    for (bits = 0u; ; bits += step)
    {
        float f = 0.0f;

        memcpy(&f, &bits, sizeof(f));
        d_tests_float_base_one(f, true, true);

        // to the last pattern a step reaches below 2^32
        if (bits > (0xFFFFFFFFu - step))
        {
            break;
        }
    }

    // every float within 2^-7 of 1, where the logarithms are smallest
    for (x = 0.9921875f; x < 1.0078125f; x = nextafterf(x, 2.0f))
    {
        d_tests_float_base_one(x, false, true);
    }

    // every float within one of 2^x's overflow threshold, 128, and of its
    // underflow one, -150, where half the least subnormal rounds to 0
    for (x = 127.0f; x < 129.0f; x = nextafterf(x, 130.0f))
    {
        d_tests_float_base_one(x, true, false);
    }

    for (x = -151.0f; x < -149.0f; x = nextafterf(x, 0.0f))
    {
        d_tests_float_base_one(x, true, false);
    }

    // the powers of 2, exactly: 2^n and its logarithm n
    for (n = -149; n <= 127; ++n)
    {
        const float q = ldexpf(1.0f, n);

        D_TESTS_EXPECT( (d_tests_same_f(d_math_f_exp2_c((float)n), q)) &&
                        (d_tests_same_f(d_math_f_log2_c(q), (float)n)),
                        "f exp2_c and log2_c at the powers of 2", n);
    }

    // the powers of 10 a float holds, to 10^10, exactly
    for (n = 0; n <= 10; ++n)
    {
        D_TESTS_EXPECT(d_tests_same_f(d_math_f_log10_c(p), (float)n),
                       "f log10_c at the powers of 10", n);
        p = p * 10.0f;
    }

    return;
}

// d_tests_double
//   exp_c and log_c within an ulp of exp and log over random arguments;
// prints how many differ at all, and the largest difference.
static void
d_tests_double(void)
{
    double worst_exp = 0.0;
    double worst_log = 0.0;
    long   diff_exp  = 0;
    long   diff_log  = 0;
    long   i         = 0;

    for (i = 0; i < 10000000; ++i)
    {
        // exp: across [-746, 710], its whole finite range and past both
        // ends, and one in four near 0; stored, so x is a double: clang on
        // x87 hands an inlined kernel the 64-bit register value otherwise,
        // and the library the double
        volatile double stored = ((i % 4) == 3)
                                     ? ldexp((2.0 * d_tests_unit()) - 1.0,
                                             -(int)d_tests_below(60u))
                                     : (-746.0 + (1456.0 * d_tests_unit()));
        const double    x      = stored;
        // log: random positive patterns, every exponent, subnormals
        // included; and one in three 1 + d, d down to 2^-53
        double p = 0.0;
        double e = 0.0;

        d_tests_pattern_d(&p, 1);

        // stored, as x is
        if ((i % 3) == 2)
        {
            volatile double sum = 1.0 + ldexp((2.0 * d_tests_unit()) - 1.0,
                                              -(int)d_tests_below(53u));

            p = sum;
        }

        // where double is evaluated wider, exp_c refuses to compile (S418)
#if !D_MATH_DBL_EVAL_WIDE
        e = D_TESTS_ULPS_D(d_math_d_exp_c(x), g_exp(x));
        worst_exp = (e > worst_exp) ? e : worst_exp;
        diff_exp += (e > 0.0) ? 1 : 0;
        D_TESTS_EXPECT(e <= 1.0, "d exp_c within an ulp of exp", x);
        // and the plain exp is exp_c, bit for bit (ruling 12, S478)
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_exp(x), d_math_d_exp_c(x)),
                       "d exp is exp_c", x);
#else
        // there the plain exp and exp_fast are the library's (S479, S490)
        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_exp(x), g_exp(x))) &&
                        (d_tests_same_d(d_math_d_exp_fast(x), g_exp(x))),
                        "d exp and exp_fast are the library's", x);
#endif  // !D_MATH_DBL_EVAL_WIDE

        // NaN past DBL_MAX: the specials are checked below; log_c, as
        // exp_c, refuses to compile where double is evaluated wider (S418)
#if !D_MATH_DBL_EVAL_WIDE
        if (p <= DBL_MAX)
        {
            e = D_TESTS_ULPS_D(d_math_d_log_c(p), g_log(p));
            worst_log = (e > worst_log) ? e : worst_log;
            diff_log += (e > 0.0) ? 1 : 0;
            D_TESTS_EXPECT(e <= 1.0, "d log_c within an ulp of log", p);
            // and the plain log is log_c, bit for bit (ruling 12, S496)
            D_TESTS_EXPECT(d_tests_same_d(d_math_d_log(p), d_math_d_log_c(p)),
                           "d log is log_c", p);
        }
#else
        // there the plain log and log_fast are the library's (S497, S501)
        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_log(p), g_log(p))) &&
                        (d_tests_same_d(d_math_d_log_fast(p), g_log(p))),
                        "d log and log_fast are the library's", p);
        (void)e;
#endif  // !D_MATH_DBL_EVAL_WIDE
    }

#if !D_MATH_DBL_EVAL_WIDE
    printf("double: exp_c differs from exp at %ld of 10000000 arguments, "
           "by %.3g ulps at most; log_c from log at %ld, by %.3g\n",
           diff_exp, worst_exp, diff_log, worst_log);
#else
    printf("double: exp_c and log_c skipped, double evaluated wider than "
           "double (S418)\n");
    (void)worst_exp;
    (void)diff_exp;
    (void)worst_log;
    (void)diff_log;
#endif  // !D_MATH_DBL_EVAL_WIDE

    return;
}

// where double is evaluated wider than double, a 32-bit x87, the double
// family's exp_c and log_c refuse to compile (S418): their checks are left
// out
#if !D_MATH_DBL_EVAL_WIDE

// d_tests_double_edges
//   the logarithm's correctly rounded values, from a 300-bit computation, at
// its edges and the values whose long double result lies on a double's
// rounding midpoint; and its specials.
static void
d_tests_double_edges(void)
{
    // x and log(x), correctly rounded
    static const double logs[][2] = {
        // the least subnormal, the least normal, the largest double
        { 4.9406564584124654e-324, -744.44007192138122      },
        { 2.2250738585072014e-308, -708.39641853226408      },
        { 1.7976931348623157e308,   709.78271289338397      },
        // 1 - 2^-53, 1 + 2^-52: the midpoint cases of the logarithm
        { 0.99999999999999989,     -1.1102230246251565e-16  },
        { 1.0000000000000002,       2.2204460492503128e-16  },
        { 0.5,                     -0.69314718055994529     },
        { 2.0,                      0.69314718055994529     },
        { 10.0,                     2.3025850929940459      }
    };
    const double inf = (double)HUGE_VAL;
    size_t       i   = 0;

    for (i = 0; i < (sizeof(logs) / sizeof(logs[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_log_c(logs[i][0]), logs[i][1]),
                       "d log_c correctly rounded at an edge", logs[i][0]);
    }

    // the specials: NaN and the negatives to NaN, the zeros to -inf, +inf
    // to +inf, 1 to +0
    D_TESTS_EXPECT( (isnan(d_math_d_log_c(nan("")))) &&
                    (isnan(d_math_d_log_c(-1.0))) &&
                    (isnan(d_math_d_log_c(-inf))) &&
                    (isnan(d_math_d_log_c(-4.9406564584124654e-324))) &&
                    (d_tests_same_d(d_math_d_log_c(0.0), -inf)) &&
                    (d_tests_same_d(d_math_d_log_c(-0.0), -inf)) &&
                    (d_tests_same_d(d_math_d_log_c(inf), inf)) &&
                    (d_tests_same_d(d_math_d_log_c(1.0), 0.0)),
                    "d log_c's specials", 0);

    return;
}

// d_tests_double_exp_edges
//   the exponential's correctly rounded values, from a 300-bit computation:
// the edges of its range, values whose long double result lies on a
// double's rounding midpoint; its specials; and the library's values across
// eight thousand doubles about each threshold.
static void
d_tests_double_exp_edges(void)
{
    // x and exp(x), correctly rounded
    static const double exps[][2] = {
        // the last finite exponential, and the first infinite one
        {  709.78271289338397,     1.7976931348622732e308   },
        // the last nonzero one, the least subnormal
        { -745.13321910194111,     4.9406564584124654e-324  },
        // 1 + 2^-53 + 2^-107 rounded to long double is the midpoint
        // 1 + 2^-53, which rounds to even; the sum does not
        {  1.1102230246251565e-16, 1.0000000000000002       },
        {  2.2204460492503131e-16, 1.0000000000000002       },
        { -1.1102230246251565e-16, 0.99999999999999989      },
        {  1.0,                    2.7182818284590451       },
        { -1.0,                    0.36787944117144233      },
        {  0.5,                    1.6487212707001282       },
        {  100.0,                  2.6881171418161356e43    },
        { -700.0,                  9.8596765437597708e-305  }
    };
    const double inf = (double)HUGE_VAL;
    size_t       i   = 0;
    int          k   = 0;

    for (i = 0; i < (sizeof(exps) / sizeof(exps[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_exp_c(exps[i][0]), exps[i][1]),
                       "d exp_c correctly rounded at an edge", exps[i][0]);
    }

    // the specials: NaN to NaN, +inf to +inf, -inf to +0, either zero to 1
    D_TESTS_EXPECT( (isnan(d_math_d_exp_c(nan("")))) &&
                    (d_tests_same_d(d_math_d_exp_c(inf), inf)) &&
                    (d_tests_same_d(d_math_d_exp_c(-inf), 0.0)) &&
                    (d_tests_same_d(d_math_d_exp_c(0.0), 1.0)) &&
                    (d_tests_same_d(d_math_d_exp_c(-0.0), 1.0)) &&
                    (d_tests_same_d(d_math_d_exp_c(1e300), inf)) &&
                    (d_tests_same_d(d_math_d_exp_c(-1e300), 0.0)),
                    "d exp_c of NaN, the infinities, the zeros, and far past "
                    "either end", 0);

    // one past each end
    D_TESTS_EXPECT(isinf(d_math_d_exp_c(nextafter(exps[0][0], 1000.0))),
                   "d exp_c past the last finite value", 0);
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_exp_c(nextafter(exps[1][0],
                                                           -1000.0)),
                                  0.0),
                   "d exp_c past the last nonzero value", 0);

    // the 8192 doubles about each threshold: the library's, exactly
    for (i = 0; i < 2; ++i)
    {
        double x = exps[i][0];

        for (k = 0; k < 4096; ++k)
        {
            x = nextafter(x, -1000.0);
        }

        for (k = 0; k < 8192; ++k)
        {
            D_TESTS_EXPECT(d_tests_same_d(d_math_d_exp_c(x), g_exp(x)),
                           "d exp_c about a threshold", x);
            x = nextafter(x, 1000.0);
        }
    }

    return;
}

// d_tests_exp_by_phases
//   exp(_x) by the three phases in turn, as exp_c took it before its common
// path (S489): the first's value where it decides, else the second's, else
// the third's; for 2^-53 < |_x| at most 745.14.
static double
d_tests_exp_by_phases(
    double _x
)
{
    const struct dj_internal_math_d_exp_arg a =
        d_internal_math_d_exp_arg_of(_x);
    const struct dj_internal_math_d_exp_result one =
        d_internal_math_d_exp_phase1(a);
    const struct dj_internal_math_d_exp_result two =
        d_internal_math_d_exp_phase2(a);

    if (one.decided)
    {
        return one.value;
    }

    return two.decided ? two.value : d_internal_math_d_exp_phase3(_x, a);
}

// d_tests_double_exp_paths
//   exp_c's common path, 2^-53 < |x| <= 708 (S489), the three phases' value
// bit for bit at ten million random arguments - three in four across
// [-708, 708], one in four of every size from 2^-8 down to 2^-53 - and at
// those about 708 and 2^-53; and exp_fast (S490) within an ulp of exp_c at
// each, and where long double holds 64 bits, within 0.511 of an ulp of the
// exact value by expl's, to 2^-9 of an ulp, expl's own error; how many it
// rounds otherwise than exp_c is printed. Outside the common path exp_fast
// is exp_c, bit for bit.
static void
d_tests_double_exp_paths(void)
{
    const double tiny  = 1.0 / 9007199254740992.0;
    const double inf   = (double)HUGE_VAL;
    double       worst = 0.0;
    long         diff  = 0;
    long         i     = 0;
    int          k     = 0;

    for (i = 0; i < 10000000; ++i)
    {
        // stored, so x is a double, as d_tests_double's are
        volatile double stored = ((i % 4) == 3)
            ? ldexp((2.0 * d_tests_unit()) - 1.0,
                    -8 - (int)d_tests_below(46u))
            : (-708.0 + (1416.0 * d_tests_unit()));
        const double    x      = stored;
        const double    c      = d_math_d_exp_c(x);
        const double    f      = d_math_d_exp_fast(x);

        if ( (x > tiny) || (x < -tiny) )
        {
            D_TESTS_EXPECT(d_tests_same_d(c, d_tests_exp_by_phases(x)),
                           "d exp_c's common path is the three phases'", x);
        }

        D_TESTS_EXPECT(D_TESTS_ULPS_D(f, c) <= 1.0,
                       "d exp_fast within an ulp of exp_c", x);
        diff += d_tests_same_d(f, c) ? 0 : 1;
#if (LDBL_MANT_DIG >= 64)
        {
            const double e = D_TESTS_ULPS_D(f, g_expl((long double)x));

            worst = (e > worst) ? e : worst;
            D_TESTS_EXPECT(e <= 0.511 + (1.0 / 512.0),
                           "d exp_fast within 0.511 of an ulp", x);
        }
#endif
    }

    // about the common path's ends: the three phases' value, and exp_fast
    // within an ulp
    for (i = 0; i < 4; ++i)
    {
        static const double ends[4] = { 708.0, -708.0,
                                        1.0 / 9007199254740992.0,
                                        -1.0 / 9007199254740992.0 };
        double x = ends[i];

        for (k = 0; k < 64; ++k)
        {
            x = nextafter(x, 0.0);
        }

        for (k = 0; k < 129; ++k)
        {
            D_TESTS_EXPECT(d_tests_same_d(d_math_d_exp_c(x),
                                          d_tests_exp_by_phases(x)),
                           "d exp_c about a common path's end", x);
            D_TESTS_EXPECT(D_TESTS_ULPS_D(d_math_d_exp_fast(x),
                                          d_math_d_exp_c(x)) <= 1.0,
                           "d exp_fast about a common path's end", x);
            x = nextafter(x, ((i % 2) == 0) ? 1000.0 : -1000.0);
        }
    }

    // outside the common path, exp_fast is exp_c
    {
        static const double outside[] = {
            0.0, -0.0, 1.0 / 9007199254740992.0, -1.0 / 9007199254740992.0,
            4.9406564584124654e-324, 708.5, -708.5, 709.78271289338397,
            -745.13321910194111, 709.79, -745.14, 1e300, -1e300
        };
        size_t j = 0;

        for (j = 0; j < (sizeof(outside) / sizeof(outside[0])); ++j)
        {
            D_TESTS_EXPECT(d_tests_same_d(d_math_d_exp_fast(outside[j]),
                                          d_math_d_exp_c(outside[j])),
                           "d exp_fast is exp_c outside the common path",
                           outside[j]);
        }

        D_TESTS_EXPECT( (isnan(d_math_d_exp_fast(nan("")))) &&
                        (d_tests_same_d(d_math_d_exp_fast(inf), inf)) &&
                        (d_tests_same_d(d_math_d_exp_fast(-inf), 0.0)),
                        "d exp_fast of NaN and the infinities", 0);
    }

#if (LDBL_MANT_DIG >= 64)
    printf("double: exp_c's common path is the three phases' at 10000000 "
           "arguments; exp_fast rounds otherwise than exp_c at %ld, within "
           "%.4f of an ulp\n", diff, worst);
#else
    printf("double: exp_c's common path is the three phases' at 10000000 "
           "arguments; exp_fast rounds otherwise than exp_c at %ld\n", diff);
    (void)worst;
#endif

    return;
}

// d_tests_log_by_phases
//   log(_x) by the three phases in turn, as log_c took them before its slow
// phases went out of line (S495): the first's value where it decides, else
// the second's, else the third's; _x positive and finite, a subnormal lifted
// by 2^54.
static double
d_tests_log_by_phases(
    double _x
)
{
    const struct dj_internal_math_d_ln_arg a =
        (_x < DBL_MIN)
            ? d_internal_math_d_ln_arg_of(_x * 18014398509481984.0, -54)
            : d_internal_math_d_ln_arg_of(_x, 0);
    const struct dj_internal_math_d_exp_result one =
        d_internal_math_d_ln_phase1(a);
    struct dj_internal_math_d_exp_result two = { false, 0.0 };

    if (one.decided)
    {
        return one.value;
    }

    two = d_internal_math_d_ln_phase2(a);

    return two.decided ? two.value : d_internal_math_d_ln_phase3(a);
}

// d_tests_log_reduction_same
//   whether log_c's reduction of _x, from its bits (S495), is the one chunk
// 65's took, made here by other means: m and e by the C library's frexp, i
// the integer nearest 128 (m - 1), and m inv by dd.h's two_product, both
// factors split; e, i, rh and rl each the same, bit for bit. _x positive,
// normal and finite.
static bool
d_tests_log_reduction_same(
    double _x
)
{
    const struct dj_internal_math_d_ln_arg a =
        d_internal_math_d_ln_arg_of(_x, 0);
    int          e = 0;
    const double m = 2.0 * frexp(_x, &e);
    const int    i = (int)(((m - 1.0) * 128.0) + 0.5);
    const struct dj_math_dd p =
        d_math_dd_two_product(m, d_internal_math_d_ln_t(3 * i));
    const struct dj_math_dd r = d_math_dd_fast_two_sum(p.high - 1.0, p.low);

    return ( (a.e == ((e - 1) + ((i >= 53) ? 1 : 0))) &&
             (a.i == i) &&
             (d_tests_same_d(a.rh, r.high)) &&
             (d_tests_same_d(a.rl, r.low)) );
}

// d_tests_double_log_paths
//   log_c's common path (S495), the three phases' value bit for bit, and
// its reduction from the bits the one by frexp and dd.h's exact product, at
// ten million random arguments - one in four a random positive pattern,
// subnormals among them, one 1 + d, d of every size from 2^-1 down to
// 2^-53, one in [1/2, 2] and one in [1, 1e6] - and at the 129 doubles about
// each of the reduction's seams: the least normal, the largest double, 1,
// 2, sqrt(2)'s neighbourhood where i goes from 52 to 53, and 2 - 2^-8, where
// it goes from 127 to 128.
static void
d_tests_double_log_paths(void)
{
    long i = 0;
    int  k = 0;

    for (i = 0; i < 10000000; ++i)
    {
        double x = 0.0;

        if ((i % 4) == 0)
        {
            d_tests_pattern_d(&x, 1);
        }
        else
        {
            // stored, so x is a double, as d_tests_double's are
            volatile double stored = ((i % 4) == 1)
                ? 1.0 + ldexp((2.0 * d_tests_unit()) - 1.0,
                              -1 - (int)d_tests_below(53u))
                : (((i % 4) == 2) ? (0.5 + (1.5 * d_tests_unit()))
                                  : (1.0 + (999999.0 * d_tests_unit())));

            x = stored;
        }

        if ( !(x > 0.0) || (x > DBL_MAX) )
        {
            continue;
        }

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_log_c(x),
                                      d_tests_log_by_phases(x)),
                       "d log_c's common path is the three phases'", x);

        if (x >= DBL_MIN)
        {
            D_TESTS_EXPECT(d_tests_log_reduction_same(x),
                           "d log_c's reduction from the bits", x);
        }
    }

    // about the reduction's seams
    for (i = 0; i < 6; ++i)
    {
        static const double seams[6] = {
            2.2250738585072014e-308, 1.7976931348623157e+308, 1.0, 2.0,
            1.41015625, 1.99609375
        };
        double x = seams[i];

        for (k = 0; k < 64; ++k)
        {
            x = nextafter(x, 0.0);
        }

        for (k = 0; k < 129; ++k)
        {
            if (x <= DBL_MAX)
            {
                D_TESTS_EXPECT(d_tests_same_d(d_math_d_log_c(x),
                                              d_tests_log_by_phases(x)),
                               "d log_c about a seam of its reduction", x);
                D_TESTS_EXPECT( (x < DBL_MIN) ||
                                (d_tests_log_reduction_same(x)),
                                "d log_c's reduction about a seam", x);
            }

            x = nextafter(x, (double)HUGE_VAL);
        }
    }

    printf("double: log_c's common path is the three phases' and its "
           "reduction chunk 65's at 10000000 arguments and about 6 seams\n");

    return;
}

// d_tests_double_log_fast
//   log_fast (S500) within an ulp of log_c at ten million random arguments
// - one in four a random positive pattern, subnormals among them, one 1 +
// d, d of every size from 2^-4 down to 2^-53, the path near 1, one in [1/2,
// 2] and one in [1, 1e6] - and, where long double holds 64 bits, within
// 0.52 of an ulp of the exact value by logl's near 1 and 0.53 elsewhere, to
// 2^-9 of an ulp, logl's own error; how many it rounds otherwise than log_c
// is printed, and each path's largest error. About each seam - the ends of
// the range near 1, 0x1.6p-1 and 0x1.6p0, where k changes, and 0x1.6p1023,
// where it reaches 1024, a row's edge below 1 and above, the least normal
// and the largest double - within an ulp of log_c; at zero, the subnormals,
// the negatives, the infinities and NaN, log_c's value, bit for bit (S501).
static void
d_tests_double_log_fast(void)
{
    const double inf         = (double)HUGE_VAL;
    double       worst_near  = 0.0;
    double       worst_table = 0.0;
    long         diff        = 0;
    long         i           = 0;
    int          k           = 0;

    for (i = 0; i < 10000000; ++i)
    {
        double x = 0.0;
        double f = 0.0;

        if ((i % 4) == 0)
        {
            d_tests_pattern_d(&x, 1);
        }
        else
        {
            // stored, so x is a double, as d_tests_double's are
            volatile double stored = ((i % 4) == 1)
                ? 1.0 + ldexp((2.0 * d_tests_unit()) - 1.0,
                              -4 - (int)d_tests_below(50u))
                : (((i % 4) == 2) ? (0.5 + (1.5 * d_tests_unit()))
                                  : (1.0 + (999999.0 * d_tests_unit())));

            x = stored;
        }

        if ( !(x > 0.0) || (x > DBL_MAX) )
        {
            continue;
        }

        f = d_math_d_log_fast(x);
        D_TESTS_EXPECT(D_TESTS_ULPS_D(f, d_math_d_log_c(x)) <= 1.0,
                       "d log_fast within an ulp of log_c", x);
        diff += d_tests_same_d(f, d_math_d_log_c(x)) ? 0 : 1;
#if (LDBL_MANT_DIG >= 64)
        {
            const double e    = D_TESTS_ULPS_D(f, g_logl((long double)x));
            const bool   near = d_internal_math_d_ln_fast_near(x);

            if (near)
            {
                worst_near = (e > worst_near) ? e : worst_near;
            }
            else
            {
                worst_table = (e > worst_table) ? e : worst_table;
            }

            D_TESTS_EXPECT(e <= ((near ? 0.52 : 0.53) + (1.0 / 512.0)),
                           "d log_fast within its bound", x);
        }
#endif
    }

    // about the seams, 129 doubles each
    for (i = 0; i < 9; ++i)
    {
        static const double m[9] = { 0.9375, 1.064697265625, 0.6875, 1.375,
                                     0.6875, 0.69140625, 1.0703125, 1.0,
                                     1.0 };
        static const int    e[9] = { 0, 0, 0, 0, 1024, 0, 0, -1022, 1024 };
        double x = ldexp(m[i], e[i]);

        // the largest double, from below
        x = (i == 8) ? DBL_MAX : x;

        for (k = 0; k < 64; ++k)
        {
            x = nextafter(x, 0.0);
        }

        for (k = 0; k < 129; ++k)
        {
            if (x <= DBL_MAX)
            {
                D_TESTS_EXPECT(D_TESTS_ULPS_D(d_math_d_log_fast(x),
                                              d_math_d_log_c(x)) <= 1.0,
                               "d log_fast about a seam", x);
            }

            x = nextafter(x, inf);
        }
    }

    // log_c's own value where log_fast hands it over, and at 1
    {
        static const double where[] = {
            0.0, -0.0, 4.9406564584124654e-324, -4.9406564584124654e-324,
            2.2250738585072009e-308, 1.0e-310, -1.0, -1.0e300, 1.0
        };
        size_t j = 0;

        for (j = 0; j < (sizeof(where) / sizeof(where[0])); ++j)
        {
            D_TESTS_EXPECT( (d_tests_same_d(d_math_d_log_fast(where[j]),
                                            d_math_d_log_c(where[j]))) ||
                            ( (isnan(d_math_d_log_fast(where[j]))) &&
                              (isnan(d_math_d_log_c(where[j]))) ),
                            "d log_fast is log_c where it hands over",
                            where[j]);
        }

        D_TESTS_EXPECT( (isnan(d_math_d_log_fast(nan("")))) &&
                        (isnan(d_math_d_log_fast(-inf))) &&
                        (d_tests_same_d(d_math_d_log_fast(inf), inf)) &&
                        (d_tests_same_d(d_math_d_log_fast(0.0), -inf)) &&
                        (d_tests_same_d(d_math_d_log_fast(1.0), 0.0)),
                        "d log_fast of NaN, the infinities, 0 and 1", 0);
    }

#if (LDBL_MANT_DIG >= 64)
    printf("double: log_fast rounds otherwise than log_c at %ld of 10000000 "
           "arguments; within %.4f of an ulp near 1, %.4f elsewhere\n",
           diff, worst_near, worst_table);
#else
    printf("double: log_fast rounds otherwise than log_c at %ld of 10000000 "
           "arguments\n", diff);
    (void)worst_near;
    (void)worst_table;
#endif

    return;
}

// d_tests_exp_value
//   a reference's double, (_m[0] 2^48 + _m[1] 2^24 + _m[2]) 2^_e: exact, as
// the significand lies below 2^53 and the value is a double.
static double
d_tests_exp_value(
    const long* _m,
    int         _e
)
{
    const double m = ( (((double)_m[0] * 16777216.0) + (double)_m[1]) *
                       16777216.0 ) + (double)_m[2];

    return ldexp(m, _e);
}

// d_tests_exp_group
//   one group of math_tests_exp_refs.h: exp_c exactly the reference at each
// point; and, where exp_c reduces - |x| at least 2^-53 - its phases one by
// one: a value the first or the second decides is the reference, and the
// third's is at every point, whether the first two decide or not. Counts in
// _phases[0 .. 2] the points each phase decides, as exp_c takes them, and
// prints them.
static void
d_tests_exp_group(
    const char*                    _name,
    const struct dj_tests_exp_ref* _refs,
    size_t                         _count,
    long*                          _phases
)
{
    const double tiny = 1.0 / 9007199254740992.0;
    size_t       i    = 0;

    _phases[0] = 0;
    _phases[1] = 0;
    _phases[2] = 0;

    for (i = 0; i < _count; ++i)
    {
        // stored, so that no compiler sees x and folds exp_c's work
        volatile double x_v = d_tests_exp_value(_refs[i].x, _refs[i].x_e);
        const double    x   = x_v;
        const double    y   = d_tests_exp_value(_refs[i].y, _refs[i].y_e);

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_exp_c(x), y),
                       "d exp_c correctly rounded at a reference", x);

        if ( (x >= tiny) || (x < -tiny) )
        {
            const struct dj_internal_math_d_exp_arg a =
                d_internal_math_d_exp_arg_of(x);
            const struct dj_internal_math_d_exp_result one =
                d_internal_math_d_exp_phase1(a);
            const struct dj_internal_math_d_exp_result two =
                d_internal_math_d_exp_phase2(a);

            D_TESTS_EXPECT( (!one.decided) ||
                            (d_tests_same_d(one.value, y)),
                            "d exp_c's first phase, where it decides", x);
            D_TESTS_EXPECT( (!two.decided) ||
                            (d_tests_same_d(two.value, y)),
                            "d exp_c's second phase, where it decides", x);
            D_TESTS_EXPECT(d_tests_same_d(d_internal_math_d_exp_phase3(x, a),
                                          y),
                           "d exp_c's third phase, at every point", x);
            ++_phases[one.decided ? 0 : (two.decided ? 1 : 2)];
        }
    }

    printf("double: exp_c at %lu %s references; of those it reduces, the "
           "first phase decides %ld, the second %ld, the third %ld\n",
           (unsigned long)_count, _name, _phases[0], _phases[1], _phases[2]);

    return;
}

// d_tests_double_exp_refs
//   exp_c at every reference of math_tests_exp_refs.h, its phases each
// reached; and the double family's self-test.
static void
d_tests_double_exp_refs(void)
{
    long random[3] = { 0, 0, 0 };
    long near[3]   = { 0, 0, 0 };
    long hard[3]   = { 0, 0, 0 };
    long edges[3]  = { 0, 0, 0 };

    d_tests_exp_group("random", g_ref_exp_d_random,
                      D_TESTS_COUNT(g_ref_exp_d_random), random);
    d_tests_exp_group("near-midpoint", g_ref_exp_d_near,
                      D_TESTS_COUNT(g_ref_exp_d_near), near);
    d_tests_exp_group("hard", g_ref_exp_d_hard,
                      D_TESTS_COUNT(g_ref_exp_d_hard), hard);
    d_tests_exp_group("edge", g_ref_exp_d_edges,
                      D_TESTS_COUNT(g_ref_exp_d_edges), edges);

    // the second phase decides the near ones, the third the hard ones
    D_TESTS_EXPECT(near[1] > 0, "d exp_c's second phase is reached", 0);
    D_TESTS_EXPECT(hard[2] > 0, "d exp_c's third phase is reached", 0);

    // the build keeps the family's guarantees
    D_TESTS_EXPECT(d_math_self_test() == 0u, "d_math_self_test gives 0",
                   d_math_self_test());
    printf("double: d_math_self_test gives %u; this build %s the plain "
           "formulas' multiplies and adds\n", d_math_self_test(),
           d_math_formulas_fuse() ? "fuses" : "does not fuse");

    return;
}

// d_tests_log_group
//   one group of math_tests_log_refs.h: log_c exactly the reference at each
// point; and its phases one by one: a value the first or the second decides
// is the reference, and the third's is at every point, whether the first two
// decide or not. Counts in _phases[0 .. 2] the points each phase decides, as
// log_c takes them, and prints them.
static void
d_tests_log_group(
    const char*                    _name,
    const struct dj_tests_log_ref* _refs,
    size_t                         _count,
    long*                          _phases
)
{
    size_t i = 0;

    _phases[0] = 0;
    _phases[1] = 0;
    _phases[2] = 0;

    for (i = 0; i < _count; ++i)
    {
        // stored, so that no compiler sees x and folds log_c's work
        volatile double x_v = d_tests_exp_value(_refs[i].x, _refs[i].x_e);
        const double    x   = x_v;
        const double    y   = d_tests_exp_value(_refs[i].y, _refs[i].y_e);
        // reduced as log_c reduces it: a subnormal lifted by 2^54 (S432)
        const struct dj_internal_math_d_ln_arg a =
            (x < DBL_MIN)
                ? d_internal_math_d_ln_arg_of(x * 18014398509481984.0, -54)
                : d_internal_math_d_ln_arg_of(x, 0);
        const struct dj_internal_math_d_exp_result one =
            d_internal_math_d_ln_phase1(a);
        const struct dj_internal_math_d_exp_result two =
            d_internal_math_d_ln_phase2(a);

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_log_c(x), y),
                       "d log_c correctly rounded at a reference", x);
        D_TESTS_EXPECT( (!one.decided) ||
                        (d_tests_same_d(one.value, y)),
                        "d log_c's first phase, where it decides", x);
        D_TESTS_EXPECT( (!two.decided) ||
                        (d_tests_same_d(two.value, y)),
                        "d log_c's second phase, where it decides", x);
        D_TESTS_EXPECT(d_tests_same_d(d_internal_math_d_ln_phase3(a), y),
                       "d log_c's third phase, at every point", x);
        ++_phases[one.decided ? 0 : (two.decided ? 1 : 2)];
    }

    printf("double: log_c at %lu %s references: the first phase decides "
           "%ld, the second %ld, the third %ld\n", (unsigned long)_count,
           _name, _phases[0], _phases[1], _phases[2]);

    return;
}

// d_tests_double_log_refs
//   log_c at every reference of math_tests_log_refs.h, its phases each
// reached.
static void
d_tests_double_log_refs(void)
{
    long random[3] = { 0, 0, 0 };
    long near[3]   = { 0, 0, 0 };
    long hard[3]   = { 0, 0, 0 };
    long edges[3]  = { 0, 0, 0 };

    d_tests_log_group("random", g_ref_log_d_random,
                      D_TESTS_COUNT(g_ref_log_d_random), random);
    d_tests_log_group("near-midpoint", g_ref_log_d_near,
                      D_TESTS_COUNT(g_ref_log_d_near), near);
    d_tests_log_group("hard", g_ref_log_d_hard,
                      D_TESTS_COUNT(g_ref_log_d_hard), hard);
    d_tests_log_group("edge", g_ref_log_d_edges,
                      D_TESTS_COUNT(g_ref_log_d_edges), edges);

    // the second phase decides the near ones, the third the hard ones
    D_TESTS_EXPECT(near[1] > 0, "d log_c's second phase is reached", 0);
    D_TESTS_EXPECT(hard[2] > 0, "d log_c's third phase is reached", 0);

    return;
}

#endif  // !D_MATH_DBL_EVAL_WIDE

// d_tests_double_bases
//   exp2_c, log2_c and log10_c within three ulps of exp2, log2 and log10
// over random arguments; prints how many differ at all, and the largest
// difference.
static void
d_tests_double_bases(void)
{
    double worst[3] = { 0.0, 0.0, 0.0 };
    long   diff[3]  = { 0, 0, 0 };
    long   i        = 0;
    int    k        = 0;

    for (i = 0; i < 3000000; ++i)
    {
        // exp2: across [-1080, 1030], its whole finite range and past both
        // ends, and one in three near 0; stored, so x is a double: clang on
        // x87 hands an inlined kernel the 64-bit register value otherwise,
        // and the library the double
        volatile double stored = ((i % 3) == 2)
                                     ? ldexp((2.0 * d_tests_unit()) - 1.0,
                                             -(int)d_tests_below(60u))
                                     : (-1080.0 + (2110.0 * d_tests_unit()));
        const double    x      = stored;
        // the logarithms: random positive patterns, every exponent,
        // subnormals included; and one in three 1 + d, d down to 2^-53
        double p = 0.0;
        double e[3];

        d_tests_pattern_d(&p, 1);

        // stored, as x is
        if ((i % 3) == 1)
        {
            volatile double sum = 1.0 + ldexp((2.0 * d_tests_unit()) - 1.0,
                                              -(int)d_tests_below(53u));

            p = sum;
        }

        // NaN past DBL_MAX: the specials are checked below
        if (p > DBL_MAX)
        {
            p = 2.0;
        }

        e[0] = D_TESTS_ULPS_D(d_math_d_exp2_c(x), g_exp2(x));
        e[1] = D_TESTS_ULPS_D(d_math_d_log2_c(p), g_log2(p));
        e[2] = D_TESTS_ULPS_D(d_math_d_log10_c(p), g_log10(p));

        for (k = 0; k < 3; ++k)
        {
            worst[k] = (e[k] > worst[k]) ? e[k] : worst[k];
            diff[k] += (e[k] > 0.0) ? 1 : 0;
        }

        D_TESTS_EXPECT(e[0] <= 3.0, "d exp2_c within three ulps of exp2", x);
        D_TESTS_EXPECT( (e[1] <= 3.0) &&
                        (e[2] <= 3.0),
                        "d log2_c and log10_c within three ulps of log2 and "
                        "log10", p);
    }

    printf("double: exp2_c, log2_c, log10_c differ from exp2, log2, log10 at "
           "%ld, %ld, %ld of 3000000 arguments, by %.3g, %.3g, %.3g ulps at "
           "most\n", diff[0], diff[1], diff[2], worst[0], worst[1], worst[2]);

    return;
}

// d_tests_double_base_edges
//   the correctly rounded values, from a 400-bit computation (the values are
// tools/edge_values7.py's); the ends of exp2's range; and the powers of 2
// and of 10, exactly.
static void
d_tests_double_base_edges(void)
{
    // x and 2^x, correctly rounded
    static const double exp2s[][2] = {
        // the range's ends: the last finite value, the least subnormal,
        // and past half of it, where it is 0
        {  1023.9999999999999,     1.7976931348621742e308   },
        {  1023.0,                 8.9884656743115795e307   },
        { -1022.0,                 2.2250738585072014e-308  },
        { -1074.0,                 4.9406564584124654e-324  },
        { -1074.5,                 4.9406564584124654e-324  },
        { -1075.0000000000002,     0.0                      },
        { -1075.5,                 0.0                      },
        {  0.5,                    1.4142135623730951       },
        { -0.5,                    0.70710678118654757      },
        {  0.33333333333333331,    1.2599210498948732       },
        {  10.5,                   1448.1546878700494       },
        {  0.10000000000000001,    1.0717734625362931       },
        {  1e-10,                  1.0000000000693148       },
        { -1e-10,                  0.99999999993068533      },
        {  2.2204460492503131e-16, 1.0000000000000002       },
        // the long double result lies on a double's midpoint
        { -34.437491269100562,     4.2981557880427738e-11   },
        {  0.0027729589604474384,  1.0019239170432919       },
        { -56.973019700139872,     7.0698813723776454e-18   },
        { -0.0022494405604828842,  0.99844202152815698      },
        { -0.31179993394662309,    0.80563600505349353      },
        { -3.2952443002645695e-07, 0.99999977159109654      }
    };
    // x and log2(x), correctly rounded
    static const double log2s[][2] = {
        { 3.0,                     1.5849625007211561       },
        { 10.0,                    3.3219280948873622       },
        { 0.10000000000000001,    -3.3219280948873622       },
        { 1.7976931348623157e308,  1024.0                   },
        { 1.0000000000000002,      3.2034265038149171e-16   },
        { 0.99999999999999989,    -1.6017132519074588e-16   },
        { 1e-300,                 -996.57842846620872       },
        { 7.0,                     2.8073549220576042       },
        // the long double result lies on a double's midpoint
        { 121.72691382087567,      6.9275043729710548       },
        { 0.99999792143923438,    -2.998732425266986e-06    },
        { 1.0001680645881046,      0.0002424455751202983    },
        { 36.881936678953615,      5.204842508891141        },
        { 0.99339026874096381,    -0.0095674806951759808    },
        { 1.0125049317173203,      0.017928935105206236     }
    };
    // x and log10(x), correctly rounded
    static const double log10s[][2] = {
        { 2.0,                     0.3010299956639812       },
        { 0.5,                    -0.3010299956639812       },
        { 1.0000000000000001e-05, -5.0                      },
        { 9.9999999999999992e22,   23.0                     },
        { 1.7976931348623157e308,  308.25471555991675       },
        { 2.2250738585072014e-308, -307.65265556858878      },
        { 4.9406564584124654e-324, -323.30621534311581      },
        { 1.0000000000000002,      9.6432746655328696e-17   },
        { 0.99999999999999989,    -4.821637332766436e-17    },
        { 3.0,                     0.47712125471966244      },
        { 7.0,                     0.84509804001425681      },
        // the long double result lies on a double's midpoint
        { 1.0000000022098703,      9.5973447020239622e-10   },
        { 1.0145696546530816,      0.0062818686123704481    },
        { 10.922782565370685,      1.0383332884462846       },
        { 18.03354547675962,       1.2560811193801136       },
        { 41.862912932693618,      1.6218294444178729       },
        { 5.2855375375858714e-05, -4.2769108384916388       }
    };
    size_t i = 0;
    int    n = 0;
    double p = 1.0;

    for (i = 0; i < (sizeof(exp2s) / sizeof(exp2s[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_exp2_c(exp2s[i][0]),
                                      exp2s[i][1]),
                       "d exp2_c correctly rounded at an edge", exp2s[i][0]);
    }

    for (i = 0; i < (sizeof(log2s) / sizeof(log2s[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_log2_c(log2s[i][0]),
                                      log2s[i][1]),
                       "d log2_c correctly rounded at an edge", log2s[i][0]);
    }

    for (i = 0; i < (sizeof(log10s) / sizeof(log10s[0])); ++i)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_log10_c(log10s[i][0]),
                                      log10s[i][1]),
                       "d log10_c correctly rounded at an edge",
                       log10s[i][0]);
    }

    // past the last finite value, and at half the least subnormal: the tie
    // goes to the even 0
    D_TESTS_EXPECT( (isinf(d_math_d_exp2_c(1024.0))) &&
                    (d_tests_same_d(d_math_d_exp2_c(-1075.0), 0.0)),
                    "d exp2_c past either end", 0);

    // every power of 2: 2^n and its logarithm n, exactly
    for (n = -1074; n <= 1023; ++n)
    {
        const double q = ldexp(1.0, n);

        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_exp2_c((double)n), q)) &&
                        (d_tests_same_d(d_math_d_log2_c(q), (double)n)),
                        "d exp2_c and log2_c at the powers of 2", n);
    }

    // every power of 10 a double holds, to 10^22, exactly
    for (n = 0; n <= 22; ++n)
    {
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_log10_c(p), (double)n),
                       "d log10_c at the powers of 10", n);
        p = p * 10.0;
    }

    return;
}

static void
d_tests_long_double(void)
{
    double worst_exp = 0.0;
    double worst_log = 0.0;
    long   i         = 0;
    // the binades p is drawn from, the subnormals' included
    const unsigned long binades =
        (unsigned long)(LDBL_MAX_EXP - LDBL_MIN_EXP + LDBL_MANT_DIG);
    // the range's ends: ln of the largest long double, and of the least
    // subnormal's half (LDBL_MAX_EXP - 1 binades, and that many more and
    // the precision's below 1)
    const long double top    = 0.6931471805599453L * (long double)LDBL_MAX_EXP;
    const long double bottom = -0.6931471805599453L *
                               (long double)(LDBL_MANT_DIG - LDBL_MIN_EXP + 1);

    for (i = 0; i < 2000000; ++i)
    {
        // exp: across the whole finite range, past both ends a little, the
        // last bits random too
        const long double x = (bottom - 2.0L) +
                              ((top - bottom + 4.0L) *
                               (long double)d_tests_unit()) +
                              ((long double)d_tests_unit() * 1e-12L);
        // log: a mantissa with every bit random...
        const long double m = 0.5L + ((long double)d_tests_unit() / 2.0L) +
                              ((long double)d_tests_unit() * 1e-17L);
        // ...in every binade, the subnormals' included
        const int         p = (int)d_tests_below(binades) +
                              (LDBL_MIN_EXP - LDBL_MANT_DIG + 1);
        // or one in three 1 + d, d down to 2^-P
        const int         q = (int)d_tests_below((unsigned long)LDBL_MANT_DIG);
        long double       y = ldexpl(m, p);
        double            e = 0.0;

        if ((i % 3) == 2)
        {
            y = 1.0L + ldexpl((2.0L * (long double)d_tests_unit()) - 1.0L,
                              -q);
        }

        e = D_TESTS_ULPS_LD(d_math_ld_exp_c(x), g_expl(x));
        worst_exp = (e > worst_exp) ? e : worst_exp;
        D_TESTS_EXPECT(e <= 1.0, "ld exp_c within an ulp of expl", x);

        e = D_TESTS_ULPS_LD(d_math_ld_log_c(y), g_logl(y));
        worst_log = (e > worst_log) ? e : worst_log;
        D_TESTS_EXPECT(e <= 1.0, "ld log_c within an ulp of logl", y);
    }

    // the edges: the largest long double and the least subnormal
    D_TESTS_EXPECT(D_TESTS_ULPS_LD(d_math_ld_log_c(LDBL_MAX),
                                   g_logl(LDBL_MAX)) <= 1.0,
                   "ld log_c(LDBL_MAX)", 0);
    D_TESTS_EXPECT(D_TESTS_ULPS_LD(d_math_ld_log_c(LDBL_MIN * LDBL_EPSILON),
                                   g_logl(LDBL_MIN * LDBL_EPSILON)) <= 1.0,
                   "ld log_c of the least subnormal", 0);

    printf("long double: exp_c within %.3g ulps of expl, log_c within %.3g "
           "of logl\n", worst_exp, worst_log);

    return;
}

// d_tests_long_double_bases
//   exp2_c, log2_c and log10_c within three ulps of exp2l, log2l and log10l
// over random arguments; the powers of 2 and of 10 exactly.
static void
d_tests_long_double_bases(void)
{
    double      worst[3] = { 0.0, 0.0, 0.0 };
    long        i        = 0;
    int         n        = 0;
    long double p        = 1.0L;
    long double five     = 1.0L;
    // the binades q is drawn from, the subnormals' included, and the
    // precision
    const unsigned long binades =
        (unsigned long)(LDBL_MAX_EXP - LDBL_MIN_EXP + LDBL_MANT_DIG);
    const unsigned long digits  = (unsigned long)LDBL_MANT_DIG;
    // 2^x's range's ends, a little past both
    const long double top    = (long double)LDBL_MAX_EXP + 2.0L;
    const long double bottom = (long double)(LDBL_MIN_EXP - LDBL_MANT_DIG) -
                               2.0L;

    for (i = 0; i < 1000000; ++i)
    {
        // exp2: across the whole finite range, the last bits random too,
        // or one in three near 0
        const long double x = ((i % 3) == 2)
                                  ? ldexpl((2.0L *
                                            (long double)d_tests_unit()) -
                                           1.0L,
                                           -(int)d_tests_below(70u))
                                  : (bottom + ((top - bottom) *
                                               (long double)d_tests_unit()) +
                                     ((long double)d_tests_unit() * 1e-12L));
        // the logarithms: a mantissa with every bit random, in every
        // binade, the subnormals' included; or one in three 1 + d
        const long double m = 0.5L + ((long double)d_tests_unit() / 2.0L) +
                              ((long double)d_tests_unit() * 1e-17L);
        const int         q = (int)d_tests_below(binades) +
                              (LDBL_MIN_EXP - LDBL_MANT_DIG + 1);
        const long double y = ((i % 3) == 1)
                                  ? (1.0L +
                                     ldexpl((2.0L *
                                             (long double)d_tests_unit()) -
                                            1.0L,
                                            -(int)d_tests_below(digits)))
                                  : ldexpl(m, q);
        double            e[3];
        int               k = 0;

        e[0] = D_TESTS_ULPS_LD(d_math_ld_exp2_c(x), g_exp2l(x));
        e[1] = D_TESTS_ULPS_LD(d_math_ld_log2_c(y), g_log2l(y));
        e[2] = D_TESTS_ULPS_LD(d_math_ld_log10_c(y), g_log10l(y));

        for (k = 0; k < 3; ++k)
        {
            worst[k] = (e[k] > worst[k]) ? e[k] : worst[k];
        }

        D_TESTS_EXPECT(e[0] <= 3.0, "ld exp2_c within three ulps of exp2l",
                       x);
        D_TESTS_EXPECT( (e[1] <= 3.0) &&
                        (e[2] <= 3.0),
                        "ld log2_c and log10_c within three ulps of log2l "
                        "and log10l", y);
    }

    // the powers of 2, a stride apart across the whole range: 2^n and its
    // logarithm n, exactly
    for (n = LDBL_MIN_EXP - LDBL_MANT_DIG; n < LDBL_MAX_EXP; n += 7)
    {
        const long double r = ldexpl(1.0L, n);

        D_TESTS_EXPECT( (d_tests_same_ld(d_math_ld_exp2_c((long double)n),
                                         r)) &&
                        (d_tests_same_ld(d_math_ld_log2_c(r),
                                         (long double)n)),
                        "ld exp2_c and log2_c at the powers of 2", n);
    }

    // every power of 10 the long double holds - while 5^n has at most its
    // precision - exactly
    for (n = 0; five < ldexpl(1.0L, LDBL_MANT_DIG); ++n)
    {
        D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_log10_c(p), (long double)n),
                       "ld log10_c at the powers of 10", n);
        p    = p * 10.0L;
        five = five * 5.0L;
    }

    printf("long double: exp2_c, log2_c, log10_c within %.3g, %.3g, %.3g ulps "
           "of exp2l, log2l, log10l\n", worst[0], worst[1], worst[2]);

    return;
}

// d_tests_specials
//   NaN, the infinities, the zeros and the domain's edges, every family.
static void
d_tests_specials(void)
{
    const double    inf          = (double)HUGE_VAL;
    volatile double one_and_half = 1.5;
    volatile double three        = 3.0;
    volatile float  three_f      = 3.0f;

    // exp: NaN to NaN, +inf to +inf, -inf to +0, either zero to 1 (the
    // double family's with its edges)
    D_TESTS_EXPECT( (isnan(d_math_f_exp_c(nanf("")))) &&
                    (isnan(d_math_ld_exp_c(nanl("")))),
                    "exp_c(NaN) is NaN", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_f_exp_c((float)inf),
                                    (float)inf)) &&
                    (d_tests_same_f(d_math_f_exp_c(-(float)inf), 0.0f)) &&
                    (d_tests_same_ld(d_math_ld_exp_c((long double)inf),
                                     (long double)inf)) &&
                    (d_tests_same_ld(d_math_ld_exp_c(-(long double)inf),
                                     0.0L)),
                    "exp_c of the infinities", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_f_exp_c(-0.0f), 1.0f)) &&
                    (d_tests_same_ld(d_math_ld_exp_c(-0.0L), 1.0L)),
                    "exp_c of the zeros", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_f_exp_c(1e30f), (float)inf)) &&
                    (d_tests_same_ld(d_math_ld_exp_c(1e4000L),
                                     (long double)inf)),
                    "exp_c far past either end", 0);

    // log: NaN and the negatives to NaN, the zeros to -inf, +inf to +inf,
    // 1 to +0; the double family's with its edges (S418)
    D_TESTS_EXPECT( (isnan(d_math_f_log_c(-1.0f))) &&
                    (isnan(d_math_ld_log_c(-1.0L))),
                    "log_c of NaN and of a negative is NaN", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_f_log_c(0.0f), -(float)inf)) &&
                    (d_tests_same_ld(d_math_ld_log_c(-0.0L),
                                     -(long double)inf)),
                    "log_c of the zeros is -inf", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_f_log_c((float)inf),
                                    (float)inf)) &&
                    (d_tests_same_ld(d_math_ld_log_c((long double)inf),
                                     (long double)inf)),
                    "log_c(+inf) is +inf", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_f_log_c(1.0f), 0.0f)) &&
                    (d_tests_same_ld(d_math_ld_log_c(1.0L), 0.0L)),
                    "log_c(1) is +0", 0);

    // exp2: NaN to NaN, +inf to +inf, -inf to +0, either zero to 1, far
    // past either end to +inf and +0
    D_TESTS_EXPECT( (isnan(d_math_f_exp2_c(nanf("")))) &&
                    (isnan(d_math_d_exp2_c(nan("")))) &&
                    (isnan(d_math_ld_exp2_c(nanl("")))),
                    "exp2_c(NaN) is NaN", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_exp2_c(inf), inf)) &&
                    (d_tests_same_d(d_math_d_exp2_c(-inf), 0.0)) &&
                    (d_tests_same_f(d_math_f_exp2_c((float)inf),
                                    (float)inf)) &&
                    (d_tests_same_f(d_math_f_exp2_c(-(float)inf), 0.0f)) &&
                    (d_tests_same_ld(d_math_ld_exp2_c((long double)inf),
                                     (long double)inf)) &&
                    (d_tests_same_ld(d_math_ld_exp2_c(-(long double)inf),
                                     0.0L)),
                    "exp2_c of the infinities", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_exp2_c(0.0), 1.0)) &&
                    (d_tests_same_d(d_math_d_exp2_c(-0.0), 1.0)) &&
                    (d_tests_same_f(d_math_f_exp2_c(-0.0f), 1.0f)) &&
                    (d_tests_same_ld(d_math_ld_exp2_c(-0.0L), 1.0L)) &&
                    (d_tests_same_d(d_math_d_exp2_c(1e300), inf)) &&
                    (d_tests_same_d(d_math_d_exp2_c(-1e300), 0.0)) &&
                    (d_tests_same_f(d_math_f_exp2_c(1e30f), (float)inf)) &&
                    (d_tests_same_ld(d_math_ld_exp2_c(1e4000L),
                                     (long double)inf)),
                    "exp2_c of the zeros, and far past either end", 0);

    // log2 and log10: NaN and the negatives to NaN, the zeros to -inf, +inf
    // to +inf, 1 to +0
    D_TESTS_EXPECT( (isnan(d_math_d_log2_c(nan("")))) &&
                    (isnan(d_math_d_log2_c(-1.0))) &&
                    (isnan(d_math_d_log10_c(-inf))) &&
                    (isnan(d_math_d_log10_c(-4.9406564584124654e-324))) &&
                    (isnan(d_math_f_log2_c(-1.0f))) &&
                    (isnan(d_math_f_log10_c(-1.0f))) &&
                    (isnan(d_math_ld_log2_c(-1.0L))) &&
                    (isnan(d_math_ld_log10_c(-1.0L))) &&
                    (isnan(d_math_ld_log10_c(nanl("")))),
                    "log2_c and log10_c of NaN and of a negative are NaN",
                    0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_log2_c(0.0), -inf)) &&
                    (d_tests_same_d(d_math_d_log10_c(-0.0), -inf)) &&
                    (d_tests_same_f(d_math_f_log2_c(-0.0f), -(float)inf)) &&
                    (d_tests_same_f(d_math_f_log10_c(0.0f), -(float)inf)) &&
                    (d_tests_same_ld(d_math_ld_log2_c(0.0L),
                                     -(long double)inf)) &&
                    (d_tests_same_ld(d_math_ld_log10_c(-0.0L),
                                     -(long double)inf)),
                    "log2_c and log10_c of the zeros are -inf", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_log2_c(inf), inf)) &&
                    (d_tests_same_d(d_math_d_log10_c(inf), inf)) &&
                    (d_tests_same_f(d_math_f_log10_c((float)inf),
                                    (float)inf)) &&
                    (d_tests_same_ld(d_math_ld_log2_c((long double)inf),
                                     (long double)inf)),
                    "log2_c and log10_c of +inf are +inf", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_log2_c(1.0), 0.0)) &&
                    (d_tests_same_d(d_math_d_log10_c(1.0), 0.0)) &&
                    (d_tests_same_f(d_math_f_log2_c(1.0f), 0.0f)) &&
                    (d_tests_same_f(d_math_f_log10_c(1.0f), 0.0f)) &&
                    (d_tests_same_ld(d_math_ld_log2_c(1.0L), 0.0L)) &&
                    (d_tests_same_ld(d_math_ld_log10_c(1.0L), 0.0L)),
                    "log2_c(1) and log10_c(1) are +0", 0);

    // the plain forms are the C library's: read through volatiles, so that
    // no compiler folds a call into its own correctly rounded constant; but
    // the plain double exp, which is exp_c wherever double is evaluated in
    // double (ruling 12, S478), and the C library's elsewhere (S479)
#if !D_MATH_DBL_EVAL_WIDE
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_exp(one_and_half),
                                  d_math_d_exp_c(one_and_half)),
                   "d exp is exp_c", 0);
#else
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_exp(one_and_half),
                                  g_exp(one_and_half)),
                   "d exp is the C library's where double is evaluated "
                   "wider", 0);
#endif  // !D_MATH_DBL_EVAL_WIDE
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_log(three_f), g_logf(three_f)),
                   "f log", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_log10(three), g_log10(three))) &&
                    (d_tests_same_f(d_math_f_log10(three_f),
                                    g_log10f(three_f))),
                    "log10 is the C library's", 0);

    // and exp2 and log2 too, but in C++ before C++11, which has neither,
    // where they are exp2_c and log2_c
#if ( defined(__cplusplus) &&                                                 \
      !D_ENV_LANG_IS_CPP11_OR_HIGHER )
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_exp2(one_and_half),
                                    d_math_d_exp2_c(one_and_half))) &&
                    (d_tests_same_d(d_math_d_log2(three),
                                    d_math_d_log2_c(three))),
                    "exp2 and log2 are exp2_c and log2_c before C++11", 0);
#else
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_exp2(one_and_half),
                                    g_exp2(one_and_half))) &&
                    (d_tests_same_d(d_math_d_log2(three), g_log2(three))),
                    "exp2 and log2 are the C library's", 0);
#endif

    return;
}


// 1.  log1p and expm1
//------------------------------------------------------------------------------
// The two functions complex numbers need: against the references (every
// family, both forms), at random arguments against the long double library,
// and at Annex F's specials.

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

// d_tests_log1p_expm1_refs
//   each reference, each family: the _c form within an ulp of the correctly
// rounded value - the long double kernel's error, below 0.53 ulp of long
// double (d_tests_kernels_wide), rounded once more - and the plain
// form, the C library's (before C++11, _c), within 3; the largest distances
// printed.
static void
d_tests_log1p_expm1_refs(void)
{
    const struct dj_tests_real_ref* const tables[2] = { g_ref_log1p,
                                                        g_ref_expm1 };
    const size_t      counts[2] = { D_TESTS_COUNT(g_ref_log1p),
                                    D_TESTS_COUNT(g_ref_expm1) };
    const char* const names[2]  = { "log1p", "expm1" };
    int               k         = 0;

    for (k = 0; k < 2; ++k)
    {
        double worst[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
        size_t i        = 0u;

        for (i = 0u; i < counts[k]; ++i)
        {
            const struct dj_tests_real_ref* r = &tables[k][i];
            const long double    x      = d_tests_ref_point(r->m, r->e);
            const size_t         length = strlen(r->value);
            const float          xf     = (float)x;
            const double         xd     = (double)x;
            float                wf     = 0.0f;
            double               wd     = 0.0;
            long double          wl     = 0.0L;
            volatile float       fc     = 0.0f;
            volatile float       fp     = 0.0f;
            volatile double      dc     = 0.0;
            volatile double      dp     = 0.0;
            volatile long double lc     = 0.0L;
            volatile long double lp     = 0.0L;
            double               u[6];
            int                  j      = 0;

            (void)d_math_decimal_to_f(&wf, r->value, length);
            (void)d_math_decimal_to_d(&wd, r->value, length);
            (void)d_math_decimal_to_ld(&wl, r->value, length);

            if (k == 0)
            {
                fc = d_math_f_log1p_c(xf);
                fp = d_math_f_log1p(xf);
                dc = d_math_d_log1p_c(xd);
                dp = d_math_d_log1p(xd);
                lc = d_math_ld_log1p_c(x);
                lp = d_math_ld_log1p(x);
            }
            else
            {
                fc = d_math_f_expm1_c(xf);
                fp = d_math_f_expm1(xf);
                dc = d_math_d_expm1_c(xd);
                dp = d_math_d_expm1(xd);
                lc = d_math_ld_expm1_c(x);
                lp = d_math_ld_expm1(x);
            }

            u[0] = D_TESTS_ULPS_F(fc, wf);
            u[1] = D_TESTS_ULPS_F(fp, wf);
            u[2] = D_TESTS_ULPS_D(dc, wd);
            u[3] = D_TESTS_ULPS_D(dp, wd);
            u[4] = D_TESTS_ULPS_LD(lc, wl);
            u[5] = D_TESTS_ULPS_LD(lp, wl);

            for (j = 0; j < 6; ++j)
            {
                worst[j] = (u[j] > worst[j]) ? u[j] : worst[j];
            }

            D_TESTS_EXPECT( (u[0] <= 1.0) &&
                            (u[2] <= 1.0) &&
                            (u[4] <= 1.0),
                            "log1p_c and expm1_c within an ulp of the"
                            " reference", x );
            D_TESTS_EXPECT( (u[1] <= 3.0) &&
                            (u[3] <= 3.0) &&
                            (u[5] <= 3.0),
                            "log1p and expm1 within 3 ulps of the reference",
                            x );
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

// d_tests_kernels_wide
//   each full-width reference: log1p_c, expm1_c and exp_c in long double
// within D_TESTS_KERNEL_BOUND of an ulp of the exact value - their distance
// from the nearest long double, exact, less the exact value's offset from it
// - and the plain forms, the C library's, within 3 of the nearest; the
// largest distances printed. exp's core takes expm1's series, and is held to
// the same bound. A long double narrower than 64 bits takes no such point
// exactly, and is left out.
static void
d_tests_kernels_wide(void)
{
    const struct dj_tests_real_wide* const tables[3] = { g_wide_log1p,
                                                         g_wide_expm1,
                                                         g_wide_exp };
    const size_t      counts[3] = { D_TESTS_COUNT(g_wide_log1p),
                                    D_TESTS_COUNT(g_wide_expm1),
                                    D_TESTS_COUNT(g_wide_exp) };
    const char* const names[3]  = { "log1p", "expm1", "exp" };
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
            const long double    x = d_tests_wide_point(r->m, r->e);
            long double          wl     = 0.0L;
            double               offset = 0.0;
            long double          unit   = 0.0L;
            volatile long double lc     = 0.0L;
            volatile long double lp     = 0.0L;
            double               error  = 0.0;
            double               plain  = 0.0;

            (void)d_math_decimal_to_ld(&wl, r->value, strlen(r->value));
            (void)d_math_decimal_to_d(&offset, r->offset, strlen(r->offset));
            unit = ldexpl(1.0L, ilogbl(wl) - (LDBL_MANT_DIG - 1));

            if (k == 0)
            {
                lc = d_math_ld_log1p_c(x);
                lp = d_math_ld_log1p(x);
            }
            else if (k == 1)
            {
                lc = d_math_ld_expm1_c(x);
                lp = d_math_ld_expm1(x);
            }
            else
            {
                lc = d_math_ld_exp_c(x);
                lp = d_math_ld_exp(x);
            }

            error = fabs((double)((lc - wl) / unit) - offset);
            plain = D_TESTS_ULPS_LD(lp, wl);
            worst_c     = (error > worst_c) ? error : worst_c;
            worst_plain = (plain > worst_plain) ? plain : worst_plain;

            D_TESTS_EXPECT( (error <= D_TESTS_KERNEL_BOUND),
                            "ld log1p_c, expm1_c and exp_c within the"
                            " kernel's bound of the exact value", x );
            D_TESTS_EXPECT( (plain <= 3.0),
                            "ld log1p, expm1 and exp within 3 ulps of the"
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

// d_tests_log1p_expm1_random
//   double: 300,000 arguments each, a third near 0 down to 2^-60, a third
// across the kernels' changes of way, a third far out (log1p to 2^1000,
// expm1 past its overflow); each _c result within an ulp of the long double
// library's rounded to double, and the plain the C library's, bit for bit
// (before C++11, _c's). Float: one pattern in 4099, within an ulp of the
// long double library's rounded to float, and the plain the C library's,
// bit for bit, as double's. Long double: 100,000 each, within 3 ulps of the
// library's.
static void
d_tests_log1p_expm1_random(void)
{
    double   worst[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    long     i        = 0;
    uint32_t pattern  = 0u;

    for (i = 0; i < 300000L; ++i)
    {
        const unsigned long kind = d_tests_below(3u);
        const double        u    = d_tests_unit();
        const double        s    = (d_tests_bit() != 0u) ? -1.0 : 1.0;
        volatile double     xl   = 0.0;
        volatile double     xe   = 0.0;
        volatile double     got  = 0.0;
        double              d    = 0.0;

        if (kind == 0u)
        {
            xl = s * ldexp(1.0 + u, -2 - (int)d_tests_below(59u));
            xe = xl;
        }
        else if (kind == 1u)
        {
            xl = (1.5 * u) - 0.75;
            xe = (2.0 * u) - 1.0;
        }
        else
        {
            xl = ldexp(1.0 + u, (int)d_tests_below(1000u));
            xe = (770.0 * u) - 60.0;
        }

        got = d_math_d_log1p_c(xl);
        d   = D_TESTS_ULPS_D(got, (double)g_log1pl((long double)xl));
        worst[0] = (d > worst[0]) ? d : worst[0];
        D_TESTS_EXPECT( (d <= 1.0), "log1p_c within an ulp", xl );

        got = d_math_d_expm1_c(xe);
        d   = D_TESTS_ULPS_D(got, (double)g_expm1l((long double)xe));
        worst[1] = (d > worst[1]) ? d : worst[1];
        D_TESTS_EXPECT( (d <= 1.0), "expm1_c within an ulp", xe );

#if ( defined(__cplusplus) &&                                                 \
      !D_ENV_LANG_IS_CPP11_OR_HIGHER )
        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_log1p(xl),
                                        d_math_d_log1p_c(xl))) &&
                        (d_tests_same_d(d_math_d_expm1(xe),
                                        d_math_d_expm1_c(xe))),
                        "log1p and expm1 are log1p_c and expm1_c before"
                        " C++11", xl );
#else
        D_TESTS_EXPECT( (d_tests_same_d(d_math_d_log1p(xl), g_log1p(xl))) &&
                        (d_tests_same_d(d_math_d_expm1(xe), g_expm1(xe))),
                        "log1p and expm1 are the C library's", xl );
#endif
    }

    for (pattern = 0u; pattern < 0xFFFFF000u; pattern += 4099u)
    {
        float           x    = 0.0f;
        volatile float  got  = 0.0f;
        double          d    = 0.0;

        memcpy(&x, &pattern, sizeof(x));

        got = d_math_f_log1p_c(x);
        d   = D_TESTS_ULPS_F(got, (float)g_log1pl((long double)x));
        worst[2] = (d > worst[2]) ? d : worst[2];
        D_TESTS_EXPECT( (d <= 1.0), "log1p_c of a float", x );

        got = d_math_f_expm1_c(x);
        d   = D_TESTS_ULPS_F(got, (float)g_expm1l((long double)x));
        worst[3] = (d > worst[3]) ? d : worst[3];
        D_TESTS_EXPECT( (d <= 1.0), "expm1_c of a float", x );

#if ( defined(__cplusplus) &&                                                 \
      !D_ENV_LANG_IS_CPP11_OR_HIGHER )
        D_TESTS_EXPECT( (d_tests_same_f(d_math_f_log1p(x),
                                        d_math_f_log1p_c(x))) &&
                        (d_tests_same_f(d_math_f_expm1(x),
                                        d_math_f_expm1_c(x))),
                        "log1p and expm1 of a float are log1p_c and"
                        " expm1_c before C++11", x );
#else
        D_TESTS_EXPECT( (d_tests_same_f(d_math_f_log1p(x), g_log1pf(x))) &&
                        (d_tests_same_f(d_math_f_expm1(x), g_expm1f(x))),
                        "log1p and expm1 of a float are the C library's",
                        x );
#endif
    }

    for (i = 0; i < 100000L; ++i)
    {
        const unsigned long  kind = d_tests_below(3u);
        const long double    u    = (long double)d_tests_unit() +
                                    (d_tests_top(11) * ldexpl(1.0L, -64));
        const long double    s    = (d_tests_bit() != 0u) ? -1.0L : 1.0L;
        volatile long double xl   = 0.0L;
        volatile long double xe   = 0.0L;
        volatile long double got  = 0.0L;
        double               d    = 0.0;

        if (kind == 0u)
        {
            xl = s * ldexpl(1.0L + u, -2 - (int)d_tests_below(70u));
            xe = xl;
        }
        else if (kind == 1u)
        {
            xl = (1.5L * u) - 0.75L;
            xe = (2.0L * u) - 1.0L;
        }
        else
        {
            xl = ldexpl(1.0L + u, (int)d_tests_below(16000u));
            xe = (11500.0L * u) - 60.0L;
        }

        got = d_math_ld_log1p_c(xl);
        d   = D_TESTS_ULPS_LD(got, g_log1pl(xl));
        worst[4] = (d > worst[4]) ? d : worst[4];
        D_TESTS_EXPECT( (d <= 3.0), "log1p_c within 3 ulps of log1pl", xl );

        got = d_math_ld_expm1_c(xe);
        d   = D_TESTS_ULPS_LD(got, g_expm1l(xe));
        worst[5] = (d > worst[5]) ? d : worst[5];
        D_TESTS_EXPECT( (d <= 3.0), "expm1_c within 3 ulps of expm1l", xe );
    }

    printf("  log1p_c and expm1_c against the long double library, in ulps:"
           " double %.3f %.3f, float %.3f %.3f, long double %.3f %.3f\n",
           worst[0],
           worst[1],
           worst[2],
           worst[3],
           worst[4],
           worst[5]);

    return;
}

// d_tests_log1p_expm1_specials
//   Annex F's values, every family, both forms: log1p of the zeros their
// own, of -1 -inf, below -1 and of -inf NaN, of +inf +inf; expm1 of the
// zeros their own, of -inf -1, of +inf and past the overflow +inf, far below
// 0 -1; a subnormal its own for both; NaN to NaN.
static void
d_tests_log1p_expm1_specials(void)
{
    const long double inf  = (long double)HUGE_VAL;
    volatile float    minf = 1.0e-45f;
    volatile double   mind = 4.9406564584124654e-324;
    int               form = 0;

    for (form = 0; form < 2; ++form)
    {
        const bool pure = (form == 0);

#define D_TESTS_LOG1P(S, T, x)                                                \
    (pure ? d_math_##S##_log1p_c((T)(x)) : d_math_##S##_log1p((T)(x)))
#define D_TESTS_EXPM1(S, T, x)                                                \
    (pure ? d_math_##S##_expm1_c((T)(x)) : d_math_##S##_expm1((T)(x)))

        D_TESTS_EXPECT( (d_tests_same_f(D_TESTS_LOG1P(f, float, 0.0f), 0.0f)) &&
                        (d_tests_same_f(D_TESTS_LOG1P(f, float, -0.0f),
                                        -0.0f)) &&
                        (d_tests_same_d(D_TESTS_LOG1P(d, double, 0.0), 0.0)) &&
                        (d_tests_same_d(D_TESTS_LOG1P(d, double, -0.0),
                                        -0.0)) &&
                        (d_tests_same_ld(D_TESTS_LOG1P(ld, long double, -0.0L),
                                         -0.0L)),
                        "log1p of the zeros", form );
        D_TESTS_EXPECT( (d_tests_same_f(D_TESTS_LOG1P(f, float, -1.0f),
                                        -(float)inf)) &&
                        (d_tests_same_d(D_TESTS_LOG1P(d, double, -1.0),
                                        -(double)inf)) &&
                        (d_tests_same_ld(D_TESTS_LOG1P(ld, long double, -1.0L),
                                         -inf)) &&
                        (isnan(D_TESTS_LOG1P(f, float, -2.0f))) &&
                        (isnan(D_TESTS_LOG1P(d, double, -inf))) &&
                        (isnan(D_TESTS_LOG1P(ld, long double, -1.5L))) &&
                        (isnan(D_TESTS_LOG1P(d, double, nan("")))) &&
                        (d_tests_same_d(D_TESTS_LOG1P(d, double, inf),
                                        (double)inf)) &&
                        (d_tests_same_f(D_TESTS_LOG1P(f, float, minf), minf)) &&
                        (d_tests_same_d(D_TESTS_LOG1P(d, double, mind), mind)),
                        "log1p at -1, below it, the infinities, NaN and the"
                        " least subnormal", form );
        D_TESTS_EXPECT( (d_tests_same_f(D_TESTS_EXPM1(f, float, -0.0f),
                                        -0.0f)) &&
                        (d_tests_same_d(D_TESTS_EXPM1(d, double, 0.0), 0.0)) &&
                        (d_tests_same_ld(D_TESTS_EXPM1(ld, long double, -0.0L),
                                         -0.0L)) &&
                        (d_tests_same_f(D_TESTS_EXPM1(f, float, -inf),
                                        -1.0f)) &&
                        (d_tests_same_d(D_TESTS_EXPM1(d, double, -inf),
                                        -1.0)) &&
                        (d_tests_same_ld(D_TESTS_EXPM1(ld, long double, -inf),
                                         -1.0L)) &&
                        (d_tests_same_d(D_TESTS_EXPM1(d, double, -800.0),
                                        -1.0)) &&
                        (d_tests_same_f(D_TESTS_EXPM1(f, float, 89.0f),
                                        (float)inf)) &&
                        (d_tests_same_d(D_TESTS_EXPM1(d, double, 710.0),
                                        (double)inf)) &&
                        (d_tests_same_ld(D_TESTS_EXPM1(ld, long double,
                                                       11357.0L), inf)) &&
                        (d_tests_same_d(D_TESTS_EXPM1(d, double, inf),
                                        (double)inf)) &&
                        (isnan(D_TESTS_EXPM1(f, float, nanf("")))) &&
                        (d_tests_same_f(D_TESTS_EXPM1(f, float, minf), minf)) &&
                        (d_tests_same_d(D_TESTS_EXPM1(d, double, -mind),
                                        -mind)),
                        "expm1 at the zeros, the infinities, its overflow,"
                        " NaN and the least subnormal", form );

#undef D_TESTS_LOG1P
#undef D_TESTS_EXPM1
    }

    return;
}

#if defined(__cplusplus)

// d_tests_overloads
//   compiled as C++: one name for every family, chosen by argument type.
static void
d_tests_overloads(void)
{
    // the double family's exp_c and log_c where they build (S418)
#if !D_MATH_DBL_EVAL_WIDE
    D_TESTS_EXPECT( (d_tests_same_d(d_math_exp_c(0.25),
                                    d_math_d_exp_c(0.25))) &&
                    (d_tests_same_d(d_math_log_c(3.0),
                                    d_math_d_log_c(3.0))),
                    "the overloads: d exp_c and log_c", 0);
#endif  // !D_MATH_DBL_EVAL_WIDE
    D_TESTS_EXPECT( (d_tests_same_f(d_math_exp_c(0.25f),
                                    d_math_f_exp_c(0.25f))) &&
                    (d_tests_same_ld(d_math_exp_c(0.25L),
                                     d_math_ld_exp_c(0.25L))) &&
                    (d_tests_same_f(d_math_log_c(3.0f),
                                    d_math_f_log_c(3.0f))) &&
                    (d_tests_same_ld(d_math_log_c(3.0L),
                                     d_math_ld_log_c(3.0L))) &&
                    (d_tests_same_d(d_math_exp(0.25), d_math_d_exp(0.25))) &&
                    (d_tests_same_d(d_math_log(3.0), g_log(3.0))),
                    "the C++ overloads are the families' functions", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_exp2_c(0.25f),
                                    d_math_f_exp2_c(0.25f))) &&
                    (d_tests_same_d(d_math_exp2_c(0.25),
                                    d_math_d_exp2_c(0.25))) &&
                    (d_tests_same_ld(d_math_exp2_c(0.25L),
                                     d_math_ld_exp2_c(0.25L))) &&
                    (d_tests_same_f(d_math_log2_c(3.0f),
                                    d_math_f_log2_c(3.0f))) &&
                    (d_tests_same_d(d_math_log2_c(3.0),
                                    d_math_d_log2_c(3.0))) &&
                    (d_tests_same_ld(d_math_log2_c(3.0L),
                                     d_math_ld_log2_c(3.0L))) &&
                    (d_tests_same_f(d_math_log10_c(3.0f),
                                    d_math_f_log10_c(3.0f))) &&
                    (d_tests_same_d(d_math_log10_c(3.0),
                                    d_math_d_log10_c(3.0))) &&
                    (d_tests_same_ld(d_math_log10_c(3.0L),
                                     d_math_ld_log10_c(3.0L))) &&
                    (d_tests_same_d(d_math_exp2(0.25), d_math_d_exp2(0.25))) &&
                    (d_tests_same_d(d_math_log2(3.0), d_math_d_log2(3.0))) &&
                    (d_tests_same_ld(d_math_log10(3.0L),
                                     d_math_ld_log10(3.0L))),
                    "the C++ overloads of exp2, log2 and log10", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_log1p_c(0.25f),
                                    d_math_f_log1p_c(0.25f))) &&
                    (d_tests_same_d(d_math_log1p_c(0.25),
                                    d_math_d_log1p_c(0.25))) &&
                    (d_tests_same_ld(d_math_log1p_c(0.25L),
                                     d_math_ld_log1p_c(0.25L))) &&
                    (d_tests_same_f(d_math_expm1_c(0.25f),
                                    d_math_f_expm1_c(0.25f))) &&
                    (d_tests_same_d(d_math_expm1_c(0.25),
                                    d_math_d_expm1_c(0.25))) &&
                    (d_tests_same_ld(d_math_expm1_c(0.25L),
                                     d_math_ld_expm1_c(0.25L))) &&
                    (d_tests_same_d(d_math_log1p(0.25),
                                    d_math_d_log1p(0.25))) &&
                    (d_tests_same_f(d_math_expm1(0.25f),
                                    d_math_f_expm1(0.25f))),
                    "the C++ overloads of log1p and expm1", 0);

    return;
}

#endif  // __cplusplus

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// from C++14, both are constant expressions: every family, the edges and
// the specials among them
static constexpr float       g_cexp_f    = d_math_f_exp_c(1.0f);
static constexpr long double g_cexp_ld   = d_math_ld_exp_c(1.0L);
static constexpr float       g_clog_f    = d_math_f_log_c(10.0f);
static constexpr long double g_clog_ld   = d_math_ld_log_c(10.0L);
static constexpr long double g_cexp_ldsub = d_math_ld_exp_c(-11390.0L);
static constexpr float       g_cexp2_f   = d_math_f_exp2_c(-0.5f);
static constexpr double      g_cexp2_top = d_math_d_exp2_c(1023.9999999999999);
static constexpr double      g_cexp2_sub = d_math_d_exp2_c(-1074.5);
static constexpr double      g_cexp2_mid =
    d_math_d_exp2_c(-34.437491269100562);
static constexpr long double g_cexp2_ld  = d_math_ld_exp2_c(0.1L);
static constexpr float       g_clog2_f   = d_math_f_log2_c(3.0f);
static constexpr double      g_clog2_mid =
    d_math_d_log2_c(121.72691382087567);
static constexpr long double g_clog2_ld  = d_math_ld_log2_c(10.0L);
static constexpr float       g_clog10_f  = d_math_f_log10_c(7.0f);
static constexpr double      g_clog10_mid =
    d_math_d_log10_c(10.922782565370685);
static constexpr double      g_clog10_sub =
    d_math_d_log10_c(4.9406564584124654e-324);
static constexpr long double g_clog10_ld = d_math_ld_log10_c(2.0L);

static_assert(d_math_d_log_c(1.0) == 0.0, "log_c(1) is 0");
static_assert(d_math_d_log_c(0.0) < -DBL_MAX, "log_c(0) is -inf");
static_assert(d_math_d_log_c(-1.0) != d_math_d_log_c(-1.0),
              "log_c(-1) is NaN");
static_assert(d_math_exp_c(2.0f) > 7.38f, "the C++ overloads, float");
static_assert(d_math_d_exp2_c(10.0) == 1024.0, "exp2_c(10) is 1024");
static_assert(d_math_d_exp2_c(-1075.0) == 0.0,
              "exp2_c at half the least subnormal is +0, the tie's even");
static_assert(d_math_d_exp2_c(1024.0) > DBL_MAX, "exp2_c overflows to +inf");
static_assert(d_math_d_log2_c(1024.0) == 10.0, "log2_c(1024) is 10");
static_assert(d_math_d_log10_c(1e22) == 22.0, "log10_c(1e22) is 22");
static_assert(d_math_d_log10_c(1.0) == 0.0, "log10_c(1) is 0");
static_assert(d_math_d_log2_c(0.0) < -DBL_MAX, "log2_c(0) is -inf");
static_assert(d_math_log10_c(1000.0f) == 3.0f, "the C++ overloads, float");

// and log1p and expm1
static constexpr float       g_clog1p_f  = d_math_f_log1p_c(0.75f);
static constexpr double      g_clog1p_d  = d_math_d_log1p_c(-0.5);
static constexpr long double g_clog1p_ld = d_math_ld_log1p_c(1.0e-10L);
static constexpr double      g_clog1p_far = d_math_d_log1p_c(1.0e300);
static constexpr float       g_cexpm1_f  = d_math_f_expm1_c(-0.25f);
static constexpr double      g_cexpm1_d  = d_math_d_expm1_c(1.0e-5);
static constexpr long double g_cexpm1_ld = d_math_ld_expm1_c(20.0L);
static constexpr double      g_cexpm1_far = d_math_d_expm1_c(-30.0);

static_assert(d_math_d_log1p_c(0.0) == 0.0, "log1p_c(0) is 0");
static_assert(d_math_d_log1p_c(-1.0) < -DBL_MAX, "log1p_c(-1) is -inf");
static_assert(d_math_d_expm1_c(0.0) == 0.0, "expm1_c(0) is 0");
static_assert(d_math_d_expm1_c(-1000.0) == -1.0, "expm1_c far below is -1");
static_assert(d_math_d_expm1_c(710.0) > DBL_MAX, "expm1_c overflows");

// the double family's exp_c, correctly rounded by three phases, where it
// builds: where double is evaluated wider, it refuses to (S418)
#if !D_MATH_DBL_EVAL_WIDE

static constexpr double g_cexp_d    = d_math_d_exp_c(1.0);
static constexpr double g_cexp_top  = d_math_d_exp_c(709.78271289338397);
static constexpr double g_cexp_sub  = d_math_d_exp_c(-745.13321910194111);
static constexpr double g_cexp_mid  = d_math_d_exp_c(1.1102230246251565e-16);
// math_tests_exp_refs.h's: near, which the second phase decides, the second
// with a subnormal result; hard, which only the third decides
static constexpr double g_cexp_two  = d_math_d_exp_c(8.517062039666136);
static constexpr double g_cexp_two_sub = d_math_d_exp_c(-710.7099386321499);
static constexpr double g_cexp_three = d_math_d_exp_c(7.541752774995959e-10);
static constexpr double g_cexp_three_below =
    d_math_d_exp_c(-8.959423965116919e-09);

static_assert(d_math_d_exp_c(0.0) == 1.0, "exp_c(0) is 1");
static_assert(d_math_d_exp_c(710.0) > DBL_MAX, "exp_c overflows to +inf");
static_assert(d_math_d_exp_c(-746.0) == 0.0, "exp_c underflows to +0");
static_assert(g_cexp_two == 4999.344284249569,
              "exp_c where its second phase decides");
static_assert(g_cexp_two_sub == 2.2008752078632e-309,
              "exp_c where its second phase decides a subnormal result");
static_assert(g_cexp_three == 1.0000000007541754,
              "exp_c at the hardest argument known, its third phase's");
static_assert(g_cexp_three_below == 0.9999999910405761,
              "exp_c below 1 where only its third phase decides");

// and log_c (S424): math_tests_log_refs.h's near, which the second phase
// decides, and hard, which only the third does
static constexpr double g_clog_d    = d_math_d_log_c(10.0);
static constexpr double g_clog_mid  = d_math_d_log_c(0.99999999999999989);
static constexpr double g_clog_sub  =
    d_math_d_log_c(4.9406564584124654e-324);
static constexpr double g_clog_two  = d_math_d_log_c(556902.1755923942);
static constexpr double g_clog_two_small =
    d_math_d_log_c(1.1076818399960603e-199);
static constexpr double g_clog_three = d_math_d_log_c(1.0000000000000013);
static constexpr double g_clog_three_below =
    d_math_d_log_c(0.9999999999999998);

static_assert(d_math_d_log_c(1.0) == 0.0, "log_c(1) is +0");
static_assert(d_math_d_log_c(0.0) < -DBL_MAX, "log_c(0) is -inf");
static_assert(g_clog_two == 13.230144876182438,
              "log_c where its second phase decides");
static_assert(g_clog_two_small == -458.1121641067429,
              "log_c where its second phase decides, far below 1");
static_assert(g_clog_three == 1.332267629550187e-15,
              "log_c above 1 where only its third phase decides");
static_assert(g_clog_three_below == -2.2204460492503136e-16,
              "log_c below 1 where only its third phase decides");

// d_tests_constant_evaluation_exp_d
//   the double exp_c's constants are the run time's results, bit for bit,
// through each of its phases: the arguments are read through volatiles, so
// the calls below are not folded.
static void
d_tests_constant_evaluation_exp_d(void)
{
    volatile double one_d    = 1.0;
    volatile double top      = 709.78271289338397;
    volatile double sub      = -745.13321910194111;
    volatile double mid      = 1.1102230246251565e-16;
    volatile double two      = 8.517062039666136;
    volatile double two_sub  = -710.7099386321499;
    volatile double hard     = 7.541752774995959e-10;
    volatile double hard_neg = -8.959423965116919e-09;

    D_TESTS_EXPECT( (d_tests_same_d(g_cexp_d, d_math_d_exp_c(one_d))) &&
                    (d_tests_same_d(g_cexp_top, d_math_d_exp_c(top))) &&
                    (d_tests_same_d(g_cexp_sub, d_math_d_exp_c(sub))) &&
                    (d_tests_same_d(g_cexp_mid, d_math_d_exp_c(mid))) &&
                    (d_tests_same_d(g_cexp_two, d_math_d_exp_c(two))) &&
                    (d_tests_same_d(g_cexp_two_sub,
                                    d_math_d_exp_c(two_sub))) &&
                    (d_tests_same_d(g_cexp_three, d_math_d_exp_c(hard))) &&
                    (d_tests_same_d(g_cexp_three_below,
                                    d_math_d_exp_c(hard_neg))),
                    "constant evaluation agrees with the run time: d "
                    "exp_c, through each of its phases", 0);

    return;
}

// d_tests_constant_evaluation_log_d
//   the double log_c's constants are the run time's results, bit for bit,
// through each of its phases, as exp_c's are.
static void
d_tests_constant_evaluation_log_d(void)
{
    volatile double ten_d   = 10.0;
    volatile double below   = 0.99999999999999989;
    volatile double least   = 4.9406564584124654e-324;
    volatile double two     = 556902.1755923942;
    volatile double small   = 1.1076818399960603e-199;
    volatile double hard    = 1.0000000000000013;
    volatile double hard_lo = 0.9999999999999998;

    D_TESTS_EXPECT( (d_tests_same_d(g_clog_d, d_math_d_log_c(ten_d))) &&
                    (d_tests_same_d(g_clog_mid, d_math_d_log_c(below))) &&
                    (d_tests_same_d(g_clog_sub, d_math_d_log_c(least))) &&
                    (d_tests_same_d(g_clog_two, d_math_d_log_c(two))) &&
                    (d_tests_same_d(g_clog_two_small,
                                    d_math_d_log_c(small))) &&
                    (d_tests_same_d(g_clog_three, d_math_d_log_c(hard))) &&
                    (d_tests_same_d(g_clog_three_below,
                                    d_math_d_log_c(hard_lo))),
                    "constant evaluation agrees with the run time: d "
                    "log_c, through each of its phases", 0);

    return;
}

#endif  // !D_MATH_DBL_EVAL_WIDE

// d_tests_constant_evaluation
//   the constants are the run time's results, bit for bit: the arguments
// are read through volatiles, so the calls below are not folded.
static void
d_tests_constant_evaluation(void)
{
    volatile float       one_f  = 1.0f;
    volatile long double one_ld = 1.0L;
    volatile float       ten_f  = 10.0f;
    volatile long double ten_ld = 10.0L;
    volatile double      least  = 4.9406564584124654e-324;
    volatile long double ldsub  = -11390.0L;
    volatile float       mhalf  = -0.5f;
    volatile double      etop   = 1023.9999999999999;
    volatile double      esub   = -1074.5;
    volatile double      emid   = -34.437491269100562;
    volatile long double tenth  = 0.1L;
    volatile float       three  = 3.0f;
    volatile double      l2mid  = 121.72691382087567;
    volatile long double ten_l  = 10.0L;
    volatile float       seven  = 7.0f;
    volatile double      l10mid = 10.922782565370685;
    volatile long double two_l  = 2.0L;

#if !D_MATH_DBL_EVAL_WIDE
    d_tests_constant_evaluation_exp_d();
    d_tests_constant_evaluation_log_d();
#endif  // !D_MATH_DBL_EVAL_WIDE

    D_TESTS_EXPECT( (d_tests_same_f(g_cexp_f, d_math_f_exp_c(one_f))) &&
                    (d_tests_same_ld(g_cexp_ld, d_math_ld_exp_c(one_ld))) &&
                    (d_tests_same_f(g_clog_f, d_math_f_log_c(ten_f))) &&
                    (d_tests_same_ld(g_clog_ld, d_math_ld_log_c(ten_ld))) &&
                    (d_tests_same_ld(g_cexp_ldsub,
                                     d_math_ld_exp_c(ldsub))),
                    "constant evaluation agrees with the run time", 0);
    D_TESTS_EXPECT( (d_tests_same_f(g_cexp2_f, d_math_f_exp2_c(mhalf))) &&
                    (d_tests_same_d(g_cexp2_top, d_math_d_exp2_c(etop))) &&
                    (d_tests_same_d(g_cexp2_sub, d_math_d_exp2_c(esub))) &&
                    (d_tests_same_d(g_cexp2_mid, d_math_d_exp2_c(emid))) &&
                    (d_tests_same_ld(g_cexp2_ld, d_math_ld_exp2_c(tenth))) &&
                    (d_tests_same_f(g_clog2_f, d_math_f_log2_c(three))) &&
                    (d_tests_same_d(g_clog2_mid, d_math_d_log2_c(l2mid))) &&
                    (d_tests_same_ld(g_clog2_ld, d_math_ld_log2_c(ten_l))) &&
                    (d_tests_same_f(g_clog10_f, d_math_f_log10_c(seven))) &&
                    (d_tests_same_d(g_clog10_mid,
                                    d_math_d_log10_c(l10mid))) &&
                    (d_tests_same_d(g_clog10_sub, d_math_d_log10_c(least))) &&
                    (d_tests_same_ld(g_clog10_ld, d_math_ld_log10_c(two_l))),
                    "constant evaluation agrees with the run time: exp2_c, "
                    "log2_c and log10_c", 0);

    {
        volatile float       lp_f  = 0.75f;
        volatile double      lp_d  = -0.5;
        volatile long double lp_ld = 1.0e-10L;
        volatile double      lp_far = 1.0e300;
        volatile float       em_f  = -0.25f;
        volatile double      em_d  = 1.0e-5;
        volatile long double em_ld = 20.0L;
        volatile double      em_far = -30.0;

        D_TESTS_EXPECT( (d_tests_same_f(g_clog1p_f, d_math_f_log1p_c(lp_f))) &&
                        (d_tests_same_d(g_clog1p_d, d_math_d_log1p_c(lp_d))) &&
                        (d_tests_same_ld(g_clog1p_ld,
                                         d_math_ld_log1p_c(lp_ld))) &&
                        (d_tests_same_d(g_clog1p_far,
                                        d_math_d_log1p_c(lp_far))) &&
                        (d_tests_same_f(g_cexpm1_f, d_math_f_expm1_c(em_f))) &&
                        (d_tests_same_d(g_cexpm1_d, d_math_d_expm1_c(em_d))) &&
                        (d_tests_same_ld(g_cexpm1_ld,
                                         d_math_ld_expm1_c(em_ld))) &&
                        (d_tests_same_d(g_cexpm1_far,
                                        d_math_d_expm1_c(em_far))),
                        "constant evaluation agrees with the run time: log1p_c"
                        " and expm1_c", 0);
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
    d_tests_float_bases(exhaustive);
    d_tests_double();
#if !D_MATH_DBL_EVAL_WIDE
    d_tests_double_edges();
    d_tests_double_exp_edges();
    d_tests_double_exp_paths();
    d_tests_double_log_paths();
    d_tests_double_log_fast();
    d_tests_double_exp_refs();
    d_tests_double_log_refs();
#endif  // !D_MATH_DBL_EVAL_WIDE
    d_tests_double_bases();
    d_tests_double_base_edges();
    d_tests_long_double();
    d_tests_long_double_bases();
    d_tests_specials();
    d_tests_log1p_expm1_refs();
    d_tests_kernels_wide();
    d_tests_log1p_expm1_random();
    d_tests_log1p_expm1_specials();
#if defined(__cplusplus)
    d_tests_overloads();
#endif
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    printf("t_exponential: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
