/*******************************************************************************
* djinterp [c][math]                                                t_circular.c
*
*   Conformance harness for c/math/circular.h, the double family's reduction
* by pi/2 and its wrapping of an angle into one turn (chunk 69, step 55):
*   - wrap_pi and wrap_two_pi exactly the angle at 1,400 bits, rounded, at
*     every argument of math_tests_circular_refs.h - random ones over every
*     binade, the doubles nearest a multiple of pi/2, 242 whose angle lies
*     within 2^-100 of a rounding midpoint, the edges - and each path alone:
*     the fixed path's value is the reference at every argument it takes,
*     and the pair path's wherever it decides; the pair path must decline
*     each angle within 2^-100.5 of a midpoint (S513). How many each path
*     decides is printed;
*   - reduce_half_pi's quadrant the reference's, and its pair within 2^-102
*     of x less that many quarter turns, relatively (S512);
*   - at a million random arguments below 2^40 and two hundred thousand over
*     the whole range, each wrap the fixed path's value, bit for bit; below
*     2^31, the reduction's two paths within 2^-101 of each other;
*   - wrap_180 and wrap_360 against the C library's remainder and fmod,
*     which are exact, at a million random arguments and the edges (S514);
*   - the specials: NaN and the infinities give NaN, a zero's sign is kept,
*     |x| below pi is x itself for wrap_pi and x in [0, 2 pi) for
*     wrap_two_pi, x just below 0 the double nearest 2 pi.
*   Where double is evaluated wider than double, a 32-bit x87, the wraps
* take their fixed path alone, which must give the same bits, and
* reduce_half_pi refuses to compile: its checks, and the pair path's, are
* skipped, and the harness says so.
*
*   Compiled as C++ from C++14, it checks that every function is a constant
* expression with the bits it has at run time, through each path. It needs
* no 64-bit integer - its random numbers come from math_tests_random.h, the
* same in every build - so that ISO-strict C++98 on a 32-bit target builds
* it too.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_circular.c -o t_circular -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_circular.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.27
*                                                            revised: 2026.11.27
*******************************************************************************/
// std
#include <float.h>   // DBL_MAX, DBL_MIN, DBL_EPSILON
#include <math.h>    // remainder, fmod, ldexp, isnan, nextafter
#include <stdio.h>   // printf
#include <string.h>  // memcmp
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/circular.h"  // unit under
                                                                  // test
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
// tests
#include "./math_tests_circular_refs.h"  // g_ref_circular_random ... _edges
#include "./math_tests_random.h"         // d_tests_seed, d_tests_top ...


// the C library's exact remainder and fmod, called through volatile
// pointers, so that no compiler substitutes its own evaluation
static double (*volatile g_remainder)(double, double) = remainder;
static double (*volatile g_fmod)(double, double)      = fmod;

static long g_checks   = 0;
static long g_failures = 0;

static void
d_tests_expect(
    bool        _ok,
    const char* _what,
    double      _value,
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
            printf("  FAIL line %d: %s (%.17g)\n", _line, _what, _value);
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what, value)                                       \
    d_tests_expect((ok), (what), (double)(value), __LINE__)

// the number of elements of an array
#define D_TESTS_COUNT(array)                                                   \
    (sizeof(array) / sizeof((array)[0]))

// bit-for-bit, NaN equal to NaN, -0 distinct from +0
static bool
d_tests_same_d(double _a, double _b)
{
    return ( (memcmp(&_a, &_b, sizeof(_a)) == 0) ||
             (isnan(_a) && isnan(_b)) );
}

// a reference's double, (a 2^48 + b 2^24 + c) 2^e
static double
d_tests_value(
    const struct dj_tests_circular_value* _v
)
{
    const double m = ( (((double)_v->a * 16777216.0) + (double)_v->b) *
                       16777216.0 ) + (double)_v->c;

    return ldexp(m, _v->e);
}

// a random double whose magnitude is 2^u, u uniform in [_lo, _hi), of
// either sign
static double
d_tests_log_uniform(
    double _lo,
    double _hi
)
{
    const double u = _lo + ((_hi - _lo) * ((double)d_tests_top(53) *
                                           (DBL_EPSILON / 2.0)));
    const double m = 1.0 + ((double)d_tests_top(52) * DBL_EPSILON);
    const double x = ldexp(m, (int)floor(u));

    return (d_tests_bit() != 0u) ? -x : x;
}

// pi's double and 2 pi's, each below its value
static const double g_pi_d     = 3.141592653589793;
static const double g_two_pi_d = 6.283185307179586;

// d_tests_fixed_takes
//   whether the wraps' fixed path is defined at _x for the form _upper:
// outside the turn the function returns itself, and past -2^-60.
static bool
d_tests_fixed_takes(
    double _x,
    bool   _upper
)
{
    if (_upper)
    {
        return ( (_x > g_two_pi_d) ||
                 (_x <= -(1.0 / 1152921504606846976.0)) );
    }

    return ( (_x > g_pi_d) || (_x < -g_pi_d) );
}

// d_tests_group
//   one group of math_tests_circular_refs.h: both wraps exactly the
// reference, the fixed path alone at every argument it takes, the pair path
// wherever it decides, declining where the reference says it must; and
// reduce_half_pi's quadrant and pair. Prints how many each path decides.
static void
d_tests_group(
    const char*                         _name,
    const struct dj_tests_circular_ref* _refs,
    size_t                              _count
)
{
    long   pair  = 0;
    long   fixed = 0;
    size_t i     = 0;

    for (i = 0; i < _count; ++i)
    {
        // stored, so that no compiler sees x and folds the work
        volatile double x_v = d_tests_value(&_refs[i].x);
        const double    x   = x_v;
        const double    p   = d_tests_value(&_refs[i].p);
        const double    t   = d_tests_value(&_refs[i].t);
        int             form = 0;

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_wrap_pi(x), p),
                       "wrap_pi correctly rounded at a reference", x);
        D_TESTS_EXPECT(d_tests_same_d(d_math_d_wrap_two_pi(x), t),
                       "wrap_two_pi correctly rounded at a reference", x);

        for (form = 0; form < 2; ++form)
        {
            const bool   upper = (form == 1);
            const double want  = upper ? t : p;

            if (!d_tests_fixed_takes(x, upper))
            {
                continue;
            }

            D_TESTS_EXPECT(d_tests_same_d(
                               d_internal_math_d_wrap_fixed(x, upper), want),
                           "the wraps' fixed path, at every argument it "
                           "takes", x);
#if !D_MATH_DBL_EVAL_WIDE
            {
                const struct dj_internal_math_d_wrap_result one =
                    d_internal_math_d_wrap_pair(x, upper);

                D_TESTS_EXPECT( (!one.decided) ||
                                (d_tests_same_d(one.value, want)),
                                "the wraps' pair path, where it decides", x);
                D_TESTS_EXPECT( ((_refs[i].fixed & (upper ? 2 : 1)) == 0) ||
                                (!one.decided),
                                "the wraps' pair path declines an angle "
                                "within 2^-100.5 of a midpoint", x);
                ++(*(one.decided ? &pair : &fixed));
            }
#else
            ++fixed;
#endif  // !D_MATH_DBL_EVAL_WIDE
        }

#if !D_MATH_DBL_EVAL_WIDE
        {
            const double rh = d_tests_value(&_refs[i].rh);
            const double rl = d_tests_value(&_refs[i].rl);
            int          q  = -1;
            const struct dj_math_dd r = d_math_d_reduce_half_pi(x, &q);
            // the pairs' difference, each part's exact where they agree
            const double d  = (r.high - rh) + (r.low - rl);
            const double ad = (d < 0.0) ? -d : d;
            const double ar = (rh < 0.0) ? -rh : rh;

            D_TESTS_EXPECT(q == _refs[i].q,
                           "reduce_half_pi's quadrant at a reference", x);
            D_TESTS_EXPECT(ad <= (ar *
                                  (1.0 / 5070602400912917605986812821504.0)),
                           "reduce_half_pi within 2^-102 at a reference", x);
        }
#endif  // !D_MATH_DBL_EVAL_WIDE
    }

    printf("double: the wraps at %lu %s references; of the wraps the fixed "
           "path takes, the pair path decides %ld, the fixed path %ld\n",
           (unsigned long)_count, _name, pair, fixed);

    return;
}

// d_tests_refs
//   every group of math_tests_circular_refs.h.
static void
d_tests_refs(void)
{
    d_tests_group("random", g_ref_circular_random,
                  D_TESTS_COUNT(g_ref_circular_random));
    d_tests_group("near", g_ref_circular_near,
                  D_TESTS_COUNT(g_ref_circular_near));
    d_tests_group("hard", g_ref_circular_hard,
                  D_TESTS_COUNT(g_ref_circular_hard));
    d_tests_group("edge", g_ref_circular_edges,
                  D_TESTS_COUNT(g_ref_circular_edges));

    return;
}

// d_tests_wraps_random
//   each wrap the fixed path's value, bit for bit, at _count random
// arguments of magnitude 2^-2 to 2^_top; prints how many wraps the fixed
// path takes and how many of those the pair path decided.
static void
d_tests_wraps_random(
    long   _count,
    double _top
)
{
    long taken = 0;
    long pair  = 0;
    long i     = 0;

    for (i = 0; i < _count; ++i)
    {
        volatile double x_v = d_tests_log_uniform(-2.0, _top);
        const double    x   = x_v;
        int             form = 0;

        for (form = 0; form < 2; ++form)
        {
            const bool   upper = (form == 1);
            const double got   = upper ? d_math_d_wrap_two_pi(x)
                                       : d_math_d_wrap_pi(x);

            if (!d_tests_fixed_takes(x, upper))
            {
                D_TESTS_EXPECT(d_tests_same_d(got, x),
                               "a wrap is x itself inside its turn", x);
                continue;
            }

            D_TESTS_EXPECT(d_tests_same_d(
                               got, d_internal_math_d_wrap_fixed(x, upper)),
                           "a wrap is its fixed path's value", x);
            ++taken;
#if !D_MATH_DBL_EVAL_WIDE
            pair += d_internal_math_d_wrap_pair(x, upper).decided ? 1 : 0;
#endif  // !D_MATH_DBL_EVAL_WIDE
        }
    }

    printf("double: the wraps at %ld random arguments to 2^%.0f, each the "
           "fixed path's value; of the %ld wraps it takes, the pair path "
           "decided %ld\n", _count, _top, taken, pair);

    return;
}

#if !D_MATH_DBL_EVAL_WIDE
// d_tests_reduce_paths
//   below 2^31, the reduction's pair path and its far path within 2^-101 of
// each other, relatively, at _count random arguments from 1/2; where the
// two quadrants differ, x 2/pi lying at a half, each r is about pi/4 and
// they differ by pi/2.
static void
d_tests_reduce_paths(
    long _count
)
{
    long apart = 0;
    long i     = 0;

    for (i = 0; i < _count; ++i)
    {
        volatile double x_v = d_tests_log_uniform(-1.0, 31.0);
        const double    x   = x_v;
        const struct dj_internal_math_d_angle a =
            d_internal_math_d_reduce_pair(x);
        const struct dj_internal_math_d_angle b =
            d_internal_math_d_reduce_far(x);
        const double ar = (b.r.high < 0.0) ? -b.r.high : b.r.high;

        if (a.quadrant == b.quadrant)
        {
            const double d  = (a.r.high - b.r.high) + (a.r.low - b.r.low);
            const double ad = (d < 0.0) ? -d : d;

            D_TESTS_EXPECT(ad <= (ar * (1.0 /
                                        2535301200456458802993406410752.0)),
                           "the reduction's two paths within 2^-101", x);
        }
        else
        {
            ++apart;
            D_TESTS_EXPECT( (ar > 0.78) && (ar < 0.79),
                            "the reduction's paths apart only at a half", x);
        }
    }

    printf("double: the reduction's two paths at %ld random arguments below "
           "2^31, within 2^-101 of each other; %ld at a half, a quadrant "
           "apart\n", _count, apart);

    return;
}
#endif  // !D_MATH_DBL_EVAL_WIDE

// d_tests_degrees_one
//   wrap_180 and wrap_360 at _x against remainder and fmod, which are exact.
static void
d_tests_degrees_one(
    double _x
)
{
    const double r = g_remainder(_x, 360.0);
    const double f = g_fmod(_x, 360.0);
    const double w = (f < 0.0) ? (f + 360.0) : f;

    D_TESTS_EXPECT(d_tests_same_d(d_math_d_wrap_180(_x),
                                  (r == -180.0) ? 180.0 : r),
                   "wrap_180 is the remainder by 360, -180 taken as 180",
                   _x);
    D_TESTS_EXPECT(d_tests_same_d(d_math_d_wrap_360(_x),
                                  (w == 360.0) ? 0.0 : w),
                   "wrap_360 is the angle in [0, 360), rounded once", _x);

    return;
}

// d_tests_degrees
//   the degree wraps at a million random arguments, random patterns among
// them, and at the edges.
static void
d_tests_degrees(void)
{
    static const double edges[] =
    {
        0.0, -0.0, 180.0, -180.0, 360.0, -360.0, 540.0, -540.0, 720.0,
        -1.0e-300, 1.0e-300, -4.9406564584124654e-324, 179.99999999999997,
        -179.99999999999997, 359.99999999999994, -359.99999999999994,
        1.0e22, -1.0e22, 1.7976931348623157e308, -1.7976931348623157e308
    };
    size_t i = 0;
    long   k = 0;

    for (i = 0; i < D_TESTS_COUNT(edges); ++i)
    {
        d_tests_degrees_one(edges[i]);
    }

    for (k = 0; k < 1000000; ++k)
    {
        volatile double x_v = 0.0;

        if ((k % 4) == 0)
        {
            double pattern = 0.0;

            d_tests_pattern_d(&pattern, 0);
            x_v = pattern;

            if (!( (x_v <= DBL_MAX) && (x_v >= -DBL_MAX) ))
            {
                continue;
            }
        }
        else
        {
            x_v = d_tests_log_uniform(-60.0, ((k % 4) == 1) ? 12.0 : 1023.0);
        }

        d_tests_degrees_one(x_v);
    }

    printf("double: wrap_180 and wrap_360 at a million random arguments and "
           "%lu edges, against remainder and fmod\n",
           (unsigned long)D_TESTS_COUNT(edges));

    return;
}

// d_tests_specials
//   NaN and the infinities; the zeros; the turns' ends; x just below 0.
static void
d_tests_specials(void)
{
    volatile double inf_v = DBL_MAX;
    const double    inf   = inf_v * 2.0;
    const double    nan_  = inf - inf;
    const double    least = 4.9406564584124654e-324;
    const double    above = nextafter(g_pi_d, 4.0);
    const double    above2 = nextafter(g_two_pi_d, 7.0);

    D_TESTS_EXPECT( (isnan(d_math_d_wrap_pi(nan_))) &&
                    (isnan(d_math_d_wrap_pi(inf))) &&
                    (isnan(d_math_d_wrap_pi(-inf))) &&
                    (isnan(d_math_d_wrap_two_pi(nan_))) &&
                    (isnan(d_math_d_wrap_two_pi(inf))) &&
                    (isnan(d_math_d_wrap_two_pi(-inf))) &&
                    (isnan(d_math_d_wrap_180(nan_))) &&
                    (isnan(d_math_d_wrap_180(inf))) &&
                    (isnan(d_math_d_wrap_360(-inf))),
                    "NaN and the infinities give NaN", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_wrap_pi(0.0), 0.0)) &&
                    (d_tests_same_d(d_math_d_wrap_pi(-0.0), -0.0)) &&
                    (d_tests_same_d(d_math_d_wrap_two_pi(0.0), 0.0)) &&
                    (d_tests_same_d(d_math_d_wrap_two_pi(-0.0), -0.0)) &&
                    (d_tests_same_d(d_math_d_wrap_180(-0.0), -0.0)) &&
                    (d_tests_same_d(d_math_d_wrap_360(0.0), 0.0)),
                    "a zero's sign is kept", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_wrap_pi(g_pi_d), g_pi_d)) &&
                    (d_tests_same_d(d_math_d_wrap_pi(-g_pi_d), -g_pi_d)) &&
                    (d_math_d_wrap_pi(above) < 0.0) &&
                    (d_math_d_wrap_pi(-above) > 0.0) &&
                    (d_tests_same_d(d_math_d_wrap_two_pi(g_two_pi_d),
                                    g_two_pi_d)) &&
                    (d_math_d_wrap_two_pi(above2) < 1.0e-15) &&
                    (d_tests_same_d(d_math_d_wrap_two_pi(-least),
                                    g_two_pi_d)) &&
                    (d_tests_same_d(d_math_d_wrap_two_pi(-1.0e-300),
                                    g_two_pi_d)),
                    "the turns' ends, and x just below 0", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_wrap_180(-180.0), 180.0)) &&
                    (d_tests_same_d(d_math_d_wrap_180(540.0), 180.0)) &&
                    (d_tests_same_d(d_math_d_wrap_180(-540.0), 180.0)) &&
                    (d_tests_same_d(d_math_d_wrap_360(-1.0e-300), 0.0)) &&
                    (d_tests_same_d(d_math_d_wrap_360(-360.0), -0.0)) &&
                    (d_tests_same_d(d_math_d_wrap_360(370.0), 10.0)),
                    "the degree wraps' ends", 0);
#if !D_MATH_DBL_EVAL_WIDE
    {
        int q1 = -1;
        int q2 = -1;
        int q3 = -1;
        const struct dj_math_dd a = d_math_d_reduce_half_pi(nan_, &q1);
        const struct dj_math_dd b = d_math_d_reduce_half_pi(-inf, &q2);
        const struct dj_math_dd c = d_math_d_reduce_half_pi(0.75, &q3);

        D_TESTS_EXPECT( (isnan(a.high)) && (q1 == 0) &&
                        (isnan(b.high)) && (q2 == 0) &&
                        (d_tests_same_d(c.high, 0.75)) &&
                        (d_tests_same_d(c.low, 0.0)) && (q3 == 0),
                        "reduce_half_pi: NaN for NaN and an infinity, x "
                        "itself within pi/4", 0);
    }
#endif  // !D_MATH_DBL_EVAL_WIDE

    return;
}

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
// the wraps as constant expressions: by the pair path, by the far pair
// path, by the fixed path (a hard argument, which the pair path declines),
// and in degrees
static constexpr double g_cwrap_pair  = d_math_d_wrap_pi(1000.0);
static constexpr double g_cwrap_far   = d_math_d_wrap_two_pi(-1.0e22);
static constexpr double g_cwrap_hard  =
    d_math_d_wrap_pi(1297464926182694.5);
static constexpr double g_cwrap_180   = d_math_d_wrap_180(-5400.5);
static constexpr double g_cwrap_360   = d_math_d_wrap_360(-1.0e-300);

#if !D_MATH_DBL_EVAL_WIDE
// dj_tests_reduced
//   struct: reduce_half_pi's results, as one value a constant can hold.
struct dj_tests_reduced
{
    double high;
    double low;
    int    quadrant;
};

static constexpr dj_tests_reduced
d_tests_reduce_c(
    double _x
)
{
    int               q = 0;
    const dj_math_dd  r = d_math_d_reduce_half_pi(_x, &q);
    dj_tests_reduced  result = { 0.0, 0.0, 0 };

    result.high     = r.high;
    result.low      = r.low;
    result.quadrant = q;

    return result;
}

static constexpr dj_tests_reduced g_creduce_pair = d_tests_reduce_c(-7.5);
static constexpr dj_tests_reduced g_creduce_far  = d_tests_reduce_c(1.0e300);
#endif  // !D_MATH_DBL_EVAL_WIDE

// d_tests_constant_evaluation
//   the constants are the run time's results, bit for bit: the arguments
// are read through volatiles, so the calls below are not folded.
static void
d_tests_constant_evaluation(void)
{
    volatile double pair = 1000.0;
    volatile double far  = -1.0e22;
    volatile double hard = 1297464926182694.5;
    volatile double deg  = -5400.5;
    volatile double tiny = -1.0e-300;

    D_TESTS_EXPECT( (d_tests_same_d(g_cwrap_pair, d_math_d_wrap_pi(pair))) &&
                    (d_tests_same_d(g_cwrap_far, d_math_d_wrap_two_pi(far))) &&
                    (d_tests_same_d(g_cwrap_hard, d_math_d_wrap_pi(hard))) &&
                    (d_tests_same_d(g_cwrap_180, d_math_d_wrap_180(deg))) &&
                    (d_tests_same_d(g_cwrap_360, d_math_d_wrap_360(tiny))),
                    "constant evaluation agrees with the run time: the "
                    "wraps, through each path", 0);
#if !D_MATH_DBL_EVAL_WIDE
    {
        volatile double a = -7.5;
        volatile double b = 1.0e300;
        const dj_tests_reduced ra = d_tests_reduce_c(a);
        const dj_tests_reduced rb = d_tests_reduce_c(b);

        D_TESTS_EXPECT( (d_tests_same_d(g_creduce_pair.high, ra.high)) &&
                        (d_tests_same_d(g_creduce_pair.low, ra.low)) &&
                        (g_creduce_pair.quadrant == ra.quadrant) &&
                        (d_tests_same_d(g_creduce_far.high, rb.high)) &&
                        (d_tests_same_d(g_creduce_far.low, rb.low)) &&
                        (g_creduce_far.quadrant == rb.quadrant),
                        "constant evaluation agrees with the run time: "
                        "reduce_half_pi, by each path", 0);
    }
#endif  // !D_MATH_DBL_EVAL_WIDE

    return;
}
#endif  // C++14

int
main(void)
{
    // the generator's start, the word 0x2545F4914F6CDD1D
    d_tests_seed(0x2545F491u, 0x4F6CDD1Du);
#if D_MATH_DBL_EVAL_WIDE
    printf("double evaluated wider than double: the wraps take their fixed "
           "path alone; reduce_half_pi and the pair path not checked\n");
#endif  // D_MATH_DBL_EVAL_WIDE
    d_tests_refs();
    d_tests_wraps_random(1000000L, 40.0);
    d_tests_wraps_random(200000L, 1023.99);
#if !D_MATH_DBL_EVAL_WIDE
    d_tests_reduce_paths(1000000L);
#endif  // !D_MATH_DBL_EVAL_WIDE
    d_tests_degrees();
    d_tests_specials();
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    printf("t_circular: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
