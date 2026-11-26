/*******************************************************************************
* djinterp [c][math]                                                  t_scalar.c
*
*   Conformance harness for c/math/scalar.h, the scalar helpers of the
* owner's ruling 13 (2026.11.24). Each helper is exact, so each is checked to
* the bit, every family:
*   - abs_c, copysign_c and is_finite_c against the C library's fabs,
*     copysign and isfinite, and ldexp_c and frexp_c against ldexp and
*     frexp, the exponent too: at a million random patterns of float and of
*     double, among them every binade from the subnormals to the largest,
*     and for ldexp every scale from below the least subnormal to past
*     overflow, where a careless scaling rounds twice; long double at two
*     hundred thousand values, in whatever format;
*   - min, max, min_num and max_num against a reference written from IEEE
*     754-2019's words, signbit and isnan, and where glibc has them (2.35
*     on), against C23's fminimum, fmaximum, fminimum_num and fmaximum_num,
*     NaN for NaN and a zero's sign included; clamp against max then min,
*     and its edges;
*   - lerp's promises: _a at _t = 0 and _b at _t = 1 exactly, _a where _a
*     and _b are equal, inside [_a, _b] for _t in [0, 1], and monotonic in
*     _t, at a million random triples; and a NaN in any giving NaN;
*   - C's special values, each family; the plain forms, the library's.
*
*   Compiled as C++, it checks the overloads; from C++14, that the helpers
* are constant expressions with the run time's bits, a zero's sign among
* them; from C++20, lerp against std::lerp where the target has no fused
* multiply-add, which nothing keeps <cmath>'s lerp from using. It needs no
* 64-bit integer, so that ISO-strict C++98 on a 32-bit target builds it
* too.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_scalar.c -o t_scalar -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_scalar.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
// glibc declares C23's fminimum and its kin under its own extensions
#if ( (defined(__linux__)) &&                                                 \
      (!defined(_GNU_SOURCE)) )
    #define _GNU_SOURCE 1
#endif

// std
#include <float.h>   // FLT_MIN ... LDBL_MAX
#include <math.h>    // fabsf ... frexpl, isnan, isfinite, signbit, nextafter
#include <stdio.h>   // printf
#include <string.h>  // memcpy, memcmp
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 202002L) )
    #include <cmath>  // std::lerp
#endif
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/scalar.h"  // unit under
                                                                // test
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t

// C23's minimum and maximum, in glibc from 2.35
#if ( (defined(__GLIBC__)) &&                                                 \
      ( (__GLIBC__ > 2) ||                                                     \
        ( (__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 35) ) ) )
    #define D_TESTS_HAVE_C23_MINIMUM 1
#else
    #define D_TESTS_HAVE_C23_MINIMUM 0
#endif


// the C library's functions, called through volatile pointers, so that no
// compiler substitutes its own evaluation, or folds a call into a constant
static float       (*volatile g_fabsf)(float)                   = fabsf;
static double      (*volatile g_fabs)(double)                   = fabs;
static long double (*volatile g_fabsl)(long double)             = fabsl;
static float       (*volatile g_copysignf)(float, float)        = copysignf;
static double      (*volatile g_copysign)(double, double)       = copysign;
static long double (*volatile g_copysignl)(long double,
                                           long double)         = copysignl;
static float       (*volatile g_ldexpf)(float, int)             = ldexpf;
static double      (*volatile g_ldexp)(double, int)             = ldexp;
static long double (*volatile g_ldexpl)(long double, int)       = ldexpl;
static float       (*volatile g_frexpf)(float, int*)            = frexpf;
static double      (*volatile g_frexp)(double, int*)            = frexp;
static long double (*volatile g_frexpl)(long double, int*)      = frexpl;
#if D_TESTS_HAVE_C23_MINIMUM
static double      (*volatile g_fminimum)(double, double)       = fminimum;
static double      (*volatile g_fmaximum)(double, double)       = fmaximum;
static double      (*volatile g_fminimum_num)(double, double)   = fminimum_num;
static double      (*volatile g_fmaximum_num)(double, double)   = fmaximum_num;
static float       (*volatile g_fminimumf)(float, float)        = fminimumf;
static float       (*volatile g_fmaximum_numf)(float, float)    = fmaximum_numf;
#endif

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

// random patterns: every bit of a float or a double drawn, NaN and the
// infinities among them now and then
static float
d_tests_pattern_f(void)
{
    const uint32_t bits = d_tests_random();
    float          x    = 0.0f;

    memcpy(&x, &bits, sizeof(x));

    return x;
}

static double
d_tests_pattern_d(void)
{
    uint32_t words[2];
    double   x = 0.0;

    words[0] = d_tests_random();
    words[1] = d_tests_random();
    memcpy(&x, words, sizeof(x));

    return x;
}

// a long double of random sign, fraction and binade, the subnormals among
// them, in whatever format
static long double
d_tests_value_ld(void)
{
    const int span = LDBL_MAX_EXP - LDBL_MIN_EXP + LDBL_MANT_DIG;
    const int e    = (int)(d_tests_random() % (uint32_t)span) +
                     LDBL_MIN_EXP - LDBL_MANT_DIG;
    long double x  = ldexpl(d_tests_unit(), e - 1);

    return ((d_tests_random() & 1u) != 0u) ? -x : x;
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

// the reference minimum and maximum, from IEEE 754-2019's words: a NaN in
// either gives NaN; else the lesser, -0 less than +0. _num: a NaN passed
// over for the other number.
static double
d_tests_ref_min(double _x, double _y, bool _num)
{
    if (isnan(_x) || isnan(_y))
    {
        if (_num)
        {
            return isnan(_x) ? _y : _x;
        }

        return isnan(_x) ? _x : _y;
    }

    if (_x == _y)
    {
        return (signbit(_x) != 0) ? _x : _y;
    }

    return (_x < _y) ? _x : _y;
}

static double
d_tests_ref_max(double _x, double _y, bool _num)
{
    if (isnan(_x) || isnan(_y))
    {
        if (_num)
        {
            return isnan(_x) ? _y : _x;
        }

        return isnan(_x) ? _x : _y;
    }

    if (_x == _y)
    {
        return (signbit(_x) != 0) ? _y : _x;
    }

    return (_x > _y) ? _x : _y;
}

// d_tests_sign_and_scale_*
//   abs, copysign, is_finite, ldexp and frexp at one value, against the C
// library's, bit for bit, both forms; ldexp at the scale _n.
static void
d_tests_sign_and_scale_f(float _x, float _y, int _n)
{
    int   e1 = 0;
    int   e2 = 0;
    float m1 = 0.0f;
    float m2 = 0.0f;

    D_TESTS_EXPECT(d_tests_same_f(d_math_f_abs_c(_x), g_fabsf(_x)) &&
                   d_tests_same_f(d_math_f_abs(_x), g_fabsf(_x)),
                   "f abs is fabs", _x);
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_copysign_c(_x, _y),
                                  g_copysignf(_x, _y)) &&
                   d_tests_same_f(d_math_f_copysign(_x, _y),
                                  g_copysignf(_x, _y)),
                   "f copysign is copysign", _x);
    D_TESTS_EXPECT( (d_math_f_is_finite_c(_x) == (isfinite(_x) != 0)) &&
                    (d_math_f_is_finite(_x) == (isfinite(_x) != 0)),
                    "f is_finite is isfinite", _x);
    D_TESTS_EXPECT(d_tests_same_f(d_math_f_ldexp_c(_x, _n), g_ldexpf(_x, _n)) &&
                   d_tests_same_f(d_math_f_ldexp(_x, _n), g_ldexpf(_x, _n)),
                   "f ldexp is ldexp", _x);
    m1 = d_math_f_frexp_c(_x, &e1);
    m2 = g_frexpf(_x, &e2);
    D_TESTS_EXPECT(d_tests_same_f(m1, m2) &&
                   ( (e1 == e2) || (!isfinite(_x)) ),
                   "f frexp is frexp", _x);

    return;
}

static void
d_tests_sign_and_scale_d(double _x, double _y, int _n)
{
    int    e1 = 0;
    int    e2 = 0;
    double m1 = 0.0;
    double m2 = 0.0;

    D_TESTS_EXPECT(d_tests_same_d(d_math_d_abs_c(_x), g_fabs(_x)) &&
                   d_tests_same_d(d_math_d_abs(_x), g_fabs(_x)),
                   "d abs is fabs", _x);
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_copysign_c(_x, _y),
                                  g_copysign(_x, _y)) &&
                   d_tests_same_d(d_math_d_copysign(_x, _y),
                                  g_copysign(_x, _y)),
                   "d copysign is copysign", _x);
    D_TESTS_EXPECT( (d_math_d_is_finite_c(_x) == (isfinite(_x) != 0)) &&
                    (d_math_d_is_finite(_x) == (isfinite(_x) != 0)),
                    "d is_finite is isfinite", _x);
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_ldexp_c(_x, _n), g_ldexp(_x, _n)) &&
                   d_tests_same_d(d_math_d_ldexp(_x, _n), g_ldexp(_x, _n)),
                   "d ldexp is ldexp", _x);
    m1 = d_math_d_frexp_c(_x, &e1);
    m2 = g_frexp(_x, &e2);
    D_TESTS_EXPECT(d_tests_same_d(m1, m2) &&
                   ( (e1 == e2) || (!isfinite(_x)) ),
                   "d frexp is frexp", _x);

    return;
}

static void
d_tests_sign_and_scale_ld(long double _x, long double _y, int _n)
{
    int         e1 = 0;
    int         e2 = 0;
    long double m1 = 0.0L;
    long double m2 = 0.0L;

    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_abs_c(_x), g_fabsl(_x)) &&
                   d_tests_same_ld(d_math_ld_abs(_x), g_fabsl(_x)),
                   "ld abs is fabs", _x);
    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_copysign_c(_x, _y),
                                   g_copysignl(_x, _y)) &&
                   d_tests_same_ld(d_math_ld_copysign(_x, _y),
                                   g_copysignl(_x, _y)),
                   "ld copysign is copysign", _x);
    D_TESTS_EXPECT( (d_math_ld_is_finite_c(_x) == (isfinite(_x) != 0)) &&
                    (d_math_ld_is_finite(_x) == (isfinite(_x) != 0)),
                    "ld is_finite is isfinite", _x);
    D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_ldexp_c(_x, _n),
                                   g_ldexpl(_x, _n)) &&
                   d_tests_same_ld(d_math_ld_ldexp(_x, _n), g_ldexpl(_x, _n)),
                   "ld ldexp is ldexp", _x);
    m1 = d_math_ld_frexp_c(_x, &e1);
    m2 = g_frexpl(_x, &e2);
    D_TESTS_EXPECT(d_tests_same_ld(m1, m2) &&
                   ( (e1 == e2) || (!isfinite(_x)) ),
                   "ld frexp is frexp", _x);

    return;
}

// a scale for ldexp: most near the edges where rounding happens - from
// below the least subnormal to past overflow - the rest anywhere
static int
d_tests_scale(int _emin, int _emax, int _p)
{
    const uint32_t r    = d_tests_random();
    const int      wide = 2 * (_emax - _emin + _p) + 8;

    switch (r % 4u)
    {
        case 0u:
            return (int)((r >> 2) % (uint32_t)wide) - (wide / 2);
        case 1u:
            // the subnormal edge, either way
            return _emin - _p - 4 + (int)((r >> 2) % (uint32_t)(2 * _p + 8)) -
                   (_emax - _emin);
        case 2u:
            return (int)((r >> 2) % (uint32_t)(2 * _p + 8)) - _p - 4;
        default:
            return -(int)((r >> 2) % (uint32_t)wide);
    }
}

// d_tests_extremes_d
//   min, max, min_num, max_num and clamp at one pair, against the
// reference, and against C23's where glibc has them.
static void
d_tests_extremes_d(double _x, double _y)
{
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_min(_x, _y),
                                  d_tests_ref_min(_x, _y, false)) &&
                   d_tests_same_d(d_math_d_max(_x, _y),
                                  d_tests_ref_max(_x, _y, false)),
                   "d min and max are IEEE 754-2019's", _x);
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_min_num(_x, _y),
                                  d_tests_ref_min(_x, _y, true)) &&
                   d_tests_same_d(d_math_d_max_num(_x, _y),
                                  d_tests_ref_max(_x, _y, true)),
                   "d min_num and max_num are IEEE 754-2019's", _x);
#if D_TESTS_HAVE_C23_MINIMUM
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_min(_x, _y), g_fminimum(_x, _y)) &&
                   d_tests_same_d(d_math_d_max(_x, _y), g_fmaximum(_x, _y)) &&
                   d_tests_same_d(d_math_d_min_num(_x, _y),
                                  g_fminimum_num(_x, _y)) &&
                   d_tests_same_d(d_math_d_max_num(_x, _y),
                                  g_fmaximum_num(_x, _y)),
                   "d min, max, min_num and max_num are C23's", _x);
#endif
    {
        // clamp: max then min, a NaN kept, a _lo above _hi giving _hi
        const double lo = (_x < _y) ? _x : _y;
        const double hi = (_x < _y) ? _y : _x;
        const double z  = d_tests_pattern_d();

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_clamp(z, lo, hi),
                                      d_tests_ref_min(
                                          d_tests_ref_max(z, lo, false),
                                          hi, false)),
                       "d clamp is max then min", z);
    }

    return;
}

// d_tests_lerp_d
//   lerp's promises at one triple: the ends exactly, an equal pair, inside
// [_a, _b] for _t in [0, 1], and monotonic in _t.
static void
d_tests_lerp_d(double _a, double _b, double _t)
{
    const double v = d_math_d_lerp(_a, _b, _t);

    D_TESTS_EXPECT(d_tests_same_d(d_math_d_lerp(_a, _b, 0.0), _a) &&
                   d_tests_same_d(d_math_d_lerp(_a, _b, 1.0), _b),
                   "d lerp is _a at 0 and _b at 1", _a);
    D_TESTS_EXPECT(d_math_d_lerp(_a, _a, _t) == _a,
                   "d lerp of an equal pair is it", _a);

    if ( (_t >= 0.0) &&
         (_t <= 1.0) )
    {
        const double lo = (_a < _b) ? _a : _b;
        const double hi = (_a < _b) ? _b : _a;
        const double u  = nextafter(_t, 2.0);

        D_TESTS_EXPECT( (v >= lo) && (v <= hi), "d lerp stays in [_a, _b]",
                        _t);
        D_TESTS_EXPECT( (_a < _b) ? (d_math_d_lerp(_a, _b, u) >= v)
                                  : (d_math_d_lerp(_a, _b, u) <= v),
                        "d lerp is monotonic", _t);
    }

    // std::lerp, where the target has no fused multiply-add: no pragma
    // keeps <cmath>'s text from fusing, as scalar.h's is kept
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 202002L) &&                                              \
      (defined(__cpp_lib_interpolate)) &&                                      \
      (!defined(__FMA__)) &&                                                   \
      (!defined(__FP_FAST_FMA)) )
    D_TESTS_EXPECT(d_tests_same_d(v, std::lerp(_a, _b, _t)),
                   "d lerp is std::lerp", _t);
#endif

    return;
}

static void
d_tests_random_values(void)
{
    long i = 0;

    for (i = 0; i < 1000000L; ++i)
    {
        const float  xf = d_tests_pattern_f();
        const float  yf = d_tests_pattern_f();
        const double xd = d_tests_pattern_d();
        const double yd = d_tests_pattern_d();

        d_tests_sign_and_scale_f(xf, yf,
                                 d_tests_scale(FLT_MIN_EXP, FLT_MAX_EXP,
                                               FLT_MANT_DIG));
        d_tests_sign_and_scale_d(xd, yd,
                                 d_tests_scale(DBL_MIN_EXP, DBL_MAX_EXP,
                                               DBL_MANT_DIG));
        d_tests_extremes_d(xd, yd);

#if D_TESTS_HAVE_C23_MINIMUM
        D_TESTS_EXPECT(d_tests_same_f(d_math_f_min(xf, yf),
                                      g_fminimumf(xf, yf)) &&
                       d_tests_same_f(d_math_f_max_num(xf, yf),
                                      g_fmaximum_numf(xf, yf)),
                       "f min and max_num are C23's", xf);
#endif
    }

    // lerp over finite ends of every size and a _t mostly in [0, 1]
    for (i = 0; i < 1000000L; ++i)
    {
        const double a = (double)(d_tests_unit() - 1.5L) *
                         ldexp(1.0, (int)(d_tests_random() % 200u) - 100);
        const double b = ((d_tests_random() % 8u) == 0u)
                         ? a
                         : (double)(d_tests_unit() - 1.5L) *
                           ldexp(1.0, (int)(d_tests_random() % 200u) - 100);
        const double t = ((d_tests_random() % 4u) == 0u)
                         ? (double)(4.0L * (d_tests_unit() - 1.5L))
                         : (double)(d_tests_unit() - 1.0L);

        d_tests_lerp_d(a, b, t);
    }

    for (i = 0; i < 200000L; ++i)
    {
        d_tests_sign_and_scale_ld(d_tests_value_ld(), d_tests_value_ld(),
                                  d_tests_scale(LDBL_MIN_EXP, LDBL_MAX_EXP,
                                                LDBL_MANT_DIG));
    }

    return;
}

// d_tests_specials
//   C's special values - the zeros, the infinities, NaN, the least
// subnormal, the least normal, the largest - each family, each helper.
static void
d_tests_specials(void)
{
    const double nan_d  = (double)NAN;
    const double inf_d  = (double)HUGE_VAL;
    const double tiny_d = DBL_MIN * DBL_EPSILON;
    const double list[] = { 0.0, -0.0, 1.0, -1.0, 0.5, -2.5, DBL_MIN,
                            -DBL_MIN, tiny_d, -tiny_d, DBL_MAX, -DBL_MAX,
                            inf_d, -inf_d, nan_d, -nan_d };
    const int    count  = (int)(sizeof(list) / sizeof(list[0]));
    int          i      = 0;
    int          j      = 0;
    int          e      = 7;

    // every pair, every helper, both forms
    for (i = 0; i < count; ++i)
    {
        for (j = 0; j < count; ++j)
        {
            d_tests_sign_and_scale_d(list[i], list[j], 0);
            d_tests_sign_and_scale_d(list[i], list[j], 1100);
            d_tests_sign_and_scale_d(list[i], list[j], -1100);
            d_tests_extremes_d(list[i], list[j]);
            d_tests_sign_and_scale_f((float)list[i], (float)list[j], -150);
            d_tests_sign_and_scale_ld((long double)list[i],
                                      (long double)list[j], -20000);
        }
    }

    // the zeros: -0 below +0, either order, every family
    D_TESTS_EXPECT( (signbit(d_math_d_min(0.0, -0.0)) != 0) &&
                    (signbit(d_math_d_min(-0.0, 0.0)) != 0) &&
                    (signbit(d_math_d_max(0.0, -0.0)) == 0) &&
                    (signbit(d_math_d_max(-0.0, 0.0)) == 0) &&
                    (signbit(d_math_f_min(0.0f, -0.0f)) != 0) &&
                    (signbit(d_math_ld_max(-0.0L, 0.0L)) == 0) &&
                    (signbit(d_math_ld_min_num(0.0L, -0.0L)) != 0),
                    "-0 is less than +0", 0);

    // a NaN: kept by min, max and clamp, passed over by min_num and max_num
    D_TESTS_EXPECT( (isnan(d_math_d_min(nan_d, 1.0))) &&
                    (isnan(d_math_d_max(1.0, nan_d))) &&
                    (d_math_d_min_num(nan_d, 1.0) == 1.0) &&
                    (d_math_d_max_num(1.0, nan_d) == 1.0) &&
                    (isnan(d_math_d_min_num(nan_d, nan_d))) &&
                    (isnan(d_math_d_clamp(nan_d, 0.0, 1.0))) &&
                    (isnan(d_math_d_clamp(0.5, nan_d, 1.0))) &&
                    (isnan(d_math_f_clamp(0.5f, 0.0f, (float)NAN))),
                    "a NaN is kept, or passed over by the _num pair", 0);

    // clamp's edges: a zero's sign, and a _lo above _hi giving _hi
    D_TESTS_EXPECT( (signbit(d_math_d_clamp(-0.0, 0.0, 1.0)) == 0) &&
                    (signbit(d_math_d_clamp(0.0, -1.0, -0.0)) != 0) &&
                    (d_math_d_clamp(0.5, 2.0, 1.0) == 1.0) &&
                    (d_math_d_clamp(-inf_d, -1.0, 1.0) == -1.0) &&
                    (d_math_ld_clamp(3.0L, 1.0L, 2.0L) == 2.0L),
                    "clamp's edges", 0);

    // lerp: a NaN in any gives NaN; some exact values (a constant cast, so
    // that where double is evaluated wider it is compared as a double)
    D_TESTS_EXPECT( (isnan(d_math_d_lerp(nan_d, 1.0, 0.5))) &&
                    (isnan(d_math_d_lerp(0.0, nan_d, 0.5))) &&
                    (isnan(d_math_d_lerp(0.0, 1.0, nan_d))) &&
                    (isnan(d_math_d_lerp(1.0, 2.0, nan_d))) &&
                    (d_math_d_lerp(1.0, 3.0, 0.5) == 2.0) &&
                    (d_math_d_lerp(0.1, 0.3, 1.0) == (double)0.3) &&
                    (d_math_d_lerp(-1.0, 1.0, 0.25) == -0.5) &&
                    (d_math_d_lerp(1.0, 2.0, 2.0) == 3.0) &&
                    (d_math_f_lerp(2.0f, 4.0f, 0.5f) == 3.0f) &&
                    (d_math_ld_lerp(2.0L, 4.0L, 0.25L) == 2.5L),
                    "lerp's NaN and exact values", 0);

    // abs and copysign read a NaN's sign where the compiler lets a constant
    // expression see it (GCC and Clang)
#if defined(__GNUC__)
    D_TESTS_EXPECT( (signbit(d_math_d_abs_c(-nan_d)) == 0) &&
                    (signbit(d_math_d_copysign_c(nan_d, -1.0)) != 0) &&
                    (signbit(d_math_d_copysign_c(2.0, -nan_d)) != 0) &&
                    (signbit(d_math_f_abs_c(-(float)NAN)) == 0),
                    "a NaN's sign", 0);
#endif

    // frexp's exponent at a zero, an infinity and NaN: 0
    (void)d_math_d_frexp_c(inf_d, &e);
    D_TESTS_EXPECT(e == 0, "frexp_c(inf) gives exponent 0", 0);
    e = 7;
    (void)d_math_d_frexp_c(0.0, &e);
    D_TESTS_EXPECT(e == 0, "frexp_c(0) gives exponent 0", 0);

    // ldexp: the edges of the subnormals, where two roundings would err
    D_TESTS_EXPECT( (d_math_d_ldexp_c(1.0, -1074) == tiny_d) &&
                    (d_math_d_ldexp_c(1.0, -1075) == 0.0) &&
                    (d_math_d_ldexp_c(1.5, -1075) == tiny_d) &&
                    (d_math_d_ldexp_c(1.0000000000000002, -1075) == tiny_d) &&
                    (d_math_d_ldexp_c(DBL_MAX, -2098) == tiny_d) &&
                    (d_math_d_ldexp_c(tiny_d, 2097) == ldexp(1.0, 1023)) &&
                    (isinf(d_math_d_ldexp_c(1.0, 1024))) &&
                    (d_math_f_ldexp_c(1.0f, -149) == FLT_MIN * FLT_EPSILON),
                    "ldexp at the subnormal edge and overflow", 0);

    return;
}

#if defined(__cplusplus)

// d_tests_overloads
//   the C++ names are each family's functions.
static void
d_tests_overloads(void)
{
    int e1 = 0;
    int e2 = 0;

    D_TESTS_EXPECT( (d_tests_same_f(d_math_min(1.0f, -2.0f),
                                    d_math_f_min(1.0f, -2.0f))) &&
                    (d_tests_same_d(d_math_max_num(1.0, 2.0),
                                    d_math_d_max_num(1.0, 2.0))) &&
                    (d_tests_same_ld(d_math_clamp(5.0L, 0.0L, 1.0L),
                                     d_math_ld_clamp(5.0L, 0.0L, 1.0L))) &&
                    (d_tests_same_d(d_math_lerp(1.0, 2.0, 0.25),
                                    d_math_d_lerp(1.0, 2.0, 0.25))) &&
                    (d_tests_same_d(d_math_abs(-3.0), d_math_d_abs(-3.0))) &&
                    (d_tests_same_f(d_math_copysign_c(2.0f, -1.0f),
                                    d_math_f_copysign_c(2.0f, -1.0f))) &&
                    (d_math_is_finite(1.0L)) &&
                    (d_tests_same_d(d_math_ldexp(3.0, 4),
                                    d_math_d_ldexp(3.0, 4))) &&
                    (d_tests_same_d(d_math_frexp_c(12.0, &e1),
                                    d_math_d_frexp(12.0, &e2))) &&
                    (e1 == e2),
                    "the C++ overloads are the families' functions", 0);

    return;
}

#endif  // __cplusplus

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// from C++14, the helpers are constant expressions, every family
static constexpr double      g_cmin_d   = d_math_d_min(0.0, -0.0);
static constexpr double      g_cmax_d   = d_math_d_max(-0.0, 0.0);
static constexpr double      g_clerp_d  = d_math_d_lerp(0.1, 0.3, 0.7);
static constexpr double      g_cldexp_d = d_math_d_ldexp_c(1.5, -1075);
static constexpr float       g_cclamp_f = d_math_f_clamp(9.0f, -1.0f, 1.0f);
static constexpr long double g_cabs_ld  = d_math_ld_abs_c(-2.5L);

// frexp_c's exponent, in a constant expression
constexpr int
d_tests_exponent_of(double _x)
{
    int e = 0;

    d_math_d_frexp_c(_x, &e);

    return e;
}

static_assert(d_math_d_min(1.0, 2.0) == 1.0, "min is a constant");
static_assert(d_math_d_max_num(1.0, (double)NAN) == 1.0,
              "max_num passes a NaN over, as a constant");
static_assert(d_math_d_clamp(5.0, 0.0, 1.0) == 1.0, "clamp is a constant");
static_assert(d_math_d_lerp(1.0, 3.0, 0.5) == 2.0, "lerp is a constant");
static_assert(d_math_d_ldexp_c(1.0, -1074) == DBL_MIN * DBL_EPSILON,
              "ldexp_c reaches the least subnormal, as a constant");
static_assert(d_tests_exponent_of(12.0) == 4, "frexp_c is a constant");
static_assert(d_tests_exponent_of(DBL_MIN * DBL_EPSILON) == -1073,
              "frexp_c of the least subnormal");
static_assert(!d_math_d_is_finite_c((double)HUGE_VAL),
              "is_finite_c is a constant");
static_assert(d_math_min(2.0f, 1.0f) == 1.0f, "the C++ overloads");
#if defined(__GNUC__)
static_assert(d_math_d_copysign_c(1.0, d_math_d_min(0.0, -0.0)) == -1.0,
              "min of the zeros is -0, as a constant");
static_assert(d_math_d_copysign_c(1.0, d_math_d_max(-0.0, 0.0)) == 1.0,
              "max of the zeros is +0, as a constant");
#endif

// d_tests_constant_evaluation
//   the constants are the run time's results, bit for bit - a zero's sign
// included: the arguments are read through volatiles, so the calls below
// are not folded.
static void
d_tests_constant_evaluation(void)
{
    volatile double      z   = 0.0;
    volatile double      a   = 0.1;
    volatile double      t   = 0.7;
    volatile double      x   = 1.5;
    volatile float       big = 9.0f;
    volatile long double neg = -2.5L;

    D_TESTS_EXPECT( (d_tests_same_d(g_cmin_d, d_math_d_min(z, -z))) &&
                    (d_tests_same_d(g_cmax_d, d_math_d_max(-z, z))) &&
                    (d_tests_same_d(g_clerp_d, d_math_d_lerp(a, 0.3, t))) &&
                    (d_tests_same_d(g_cldexp_d, d_math_d_ldexp_c(x, -1075))) &&
                    (d_tests_same_f(g_cclamp_f,
                                    d_math_f_clamp(big, -1.0f, 1.0f))) &&
                    (d_tests_same_ld(g_cabs_ld, d_math_ld_abs_c(neg))),
                    "constant evaluation agrees with the run time", 0);

    return;
}

#endif  // C++14

int
main(void)
{
    d_tests_random_values();
    d_tests_specials();
#if defined(__cplusplus)
    d_tests_overloads();
#endif
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    printf("t_scalar: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
