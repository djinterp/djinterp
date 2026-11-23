/*******************************************************************************
* djinterp [c][math]                                                      t_dd.c
*
*   Conformance harness for c/math/dd.h, the pairs of doubles, every result
* checked exactly, against integers of up to 768 bits (math_tests_exact.h).
* The exact sums and product at 200,000 random arguments each, their
* exponents far apart, the products across the range: high + low the exact
* sum or product, high its rounding, the pair normalized (high the rounding
* of high + low). The sums, differences and products of pairs at 200,000
* random normalized pairs each, cancelling sums among them: within their
* proven bounds of the exact result - add_d 2u^2 / (1 - 2u), add 3u^2 / (1 -
* 4u), mul_d 3u^2, mul 7u^2, u = 2^-53 - and normalized. The quotients and
* the square root within their bounds, Gappa's (12u^2, 38u^2, 32u^2), by the
* residual, exact: |q y - x| against |x|, and |s^2 - x| against 2x, which
* bounds |s - sqrt(x)| against sqrt(x). The largest error each shows is
* printed, in u^2. The specials: of, value, neg, the square root of the
* zeros and of a negative. Compiled as C++ from C++14, every function is a
* constant expression: the exact sums and product with the run time's bits,
* always, the rest where the build does not fuse a multiply and an add
* (self_test.h's d_math_formulas_fuse): where it does, their bits follow it,
* each still within its bound (S415).
*   Where double is evaluated wider than double, a 32-bit x87, dd.h refuses
* to compile (S418): the harness checks nothing there, and says so. It needs
* no 64-bit integer: its random numbers come from math_tests_random.h, the
* same in every build.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_dd.c -o t_dd -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_dd.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/

// std
#include <math.h>    // frexp, ldexp, ldexpl, floor
#include <stdio.h>   // printf
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/dd.h"         // unit under test
#include "../../../../../inc/djinterp/c/modules/math/self_test.h"  // d_math_formulas_fuse
// tests: 768 bits, room for a product of pairs shifted by 2^146
#define D_TESTS_EXACT_LIMBS  48
#include "./math_tests_exact.h"   // dj_tests_big, d_tests_big_*
#include "./math_tests_random.h"  // d_tests_seed, d_tests_below, d_tests_top


static long g_checks   = 0;
static long g_failures = 0;

// where double is evaluated wider than double, dd.h refuses to compile
#if !D_MATH_DBL_EVAL_WIDE

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


//==============================================================================
// 1.  EXACT VALUES
//==============================================================================
// A double, a sum of doubles or a product of them, exactly: m 2^e, m an
// integer of up to 768 bits.

// dj_tests_dyadic
//   struct: the value m 2^e, exactly.
struct dj_tests_dyadic
{
    struct dj_tests_big m;
    int                 e;
};

// d_tests_shl
//   _x 2^_k, _k >= 0, its sign kept.
static struct dj_tests_big
d_tests_shl(
    struct dj_tests_big _x,
    int                 _k
)
{
    struct dj_tests_big result = d_tests_big_shl(_x, _k);

    result.negative = _x.negative && !d_tests_big_is_zero(result);

    return result;
}

// d_tests_dyadic_of
//   _x exactly: its significand, below 2^53, in two parts below 2^27, so
// that a 32-bit word holds each.
static struct dj_tests_dyadic
d_tests_dyadic_of(
    double _x
)
{
    struct dj_tests_dyadic result;
    int                    e  = 0;
    const double           f  = frexp((_x < 0.0) ? -_x : _x, &e);
    const double           m  = ldexp(f, 53);
    const double           hi = floor(m / 134217728.0);
    const double           lo = m - (hi * 134217728.0);

    result.m = d_tests_big_add(
        d_tests_shl(d_tests_big_of_umax((dj_math_umax)hi, false), 27),
        d_tests_big_of_umax((dj_math_umax)lo, false));
    result.m = (_x < 0.0) ? d_tests_big_neg(result.m) : result.m;
    result.e = e - 53;

    return result;
}

// d_tests_dyadic_add
//   _a + _b, exactly: both at the smaller exponent, then summed.
static struct dj_tests_dyadic
d_tests_dyadic_add(
    struct dj_tests_dyadic _a,
    struct dj_tests_dyadic _b
)
{
    struct dj_tests_dyadic result;

    if (d_tests_big_is_zero(_a.m))
    {
        return _b;
    }

    if (d_tests_big_is_zero(_b.m))
    {
        return _a;
    }

    result.e = (_a.e < _b.e) ? _a.e : _b.e;
    result.m = d_tests_big_add(d_tests_shl(_a.m, _a.e - result.e),
                               d_tests_shl(_b.m, _b.e - result.e));

    return result;
}

// d_tests_dyadic_mul
//   _a _b, exactly.
static struct dj_tests_dyadic
d_tests_dyadic_mul(
    struct dj_tests_dyadic _a,
    struct dj_tests_dyadic _b
)
{
    struct dj_tests_dyadic result;

    result.m = d_tests_big_mul(_a.m, _b.m);
    result.e = _a.e + _b.e;

    return result;
}

// d_tests_dyadic_neg
//   -_a, exactly.
static struct dj_tests_dyadic
d_tests_dyadic_neg(
    struct dj_tests_dyadic _a
)
{
    _a.m = d_tests_big_neg(_a.m);

    return _a;
}

// d_tests_dyadic_pair
//   a pair's value, high + low, exactly.
static struct dj_tests_dyadic
d_tests_dyadic_pair(
    struct dj_math_dd _x
)
{
    return d_tests_dyadic_add(d_tests_dyadic_of(_x.high),
                              d_tests_dyadic_of(_x.low));
}

// d_tests_dyadic_within
//   whether |_err| 2^(106 + 40) <= _bound (2^40 + 1) |_of|: |_err| within
// _bound u^2 of |_of|, to a slack of 2^-40 of the bound, which covers the
// (1 - 2u) and (1 - 4u) of the sums' bounds.
static bool
d_tests_dyadic_within(
    struct dj_tests_dyadic _err,
    struct dj_tests_dyadic _of,
    dj_math_umax           _bound
)
{
    const struct dj_tests_big b =
        d_tests_big_add(d_tests_shl(d_tests_big_of_umax(_bound, false), 40),
                        d_tests_big_of_umax(_bound, false));
    struct dj_tests_big lhs = d_tests_big_abs(_err.m);
    struct dj_tests_big rhs = d_tests_big_mul(b, d_tests_big_abs(_of.m));
    const int           el  = _err.e + 146;
    const int           er  = _of.e;

    // at one exponent, the smaller
    if (el > er)
    {
        lhs = d_tests_shl(lhs, el - er);
    }
    else
    {
        rhs = d_tests_shl(rhs, er - el);
    }

    return d_tests_big_cmp_mag(lhs, rhs) <= 0;
}

// d_tests_dyadic_value
//   _a, near enough to print: its first five limbs, 64 bits or more.
static long double
d_tests_dyadic_value(
    struct dj_tests_dyadic _a
)
{
    long double v   = 0.0L;
    int         top = D_TESTS_LIMBS - 1;
    int         i   = 0;

    while ((top >= 0) && (_a.m.limb[top] == 0UL))
    {
        --top;
    }

    for (i = top; (i >= 0) && (i > (top - 5)); --i)
    {
        v = (v * 65536.0L) + (long double)_a.m.limb[i];
    }

    return ldexpl(_a.m.negative ? -v : v, _a.e + (16 * (i + 1)));
}

// d_tests_in_u2
//   |_err| / |_of| in units of u^2, near enough to print.
static double
d_tests_in_u2(
    struct dj_tests_dyadic _err,
    struct dj_tests_dyadic _of
)
{
    const long double e = d_tests_dyadic_value(_err);
    const long double o = d_tests_dyadic_value(_of);

    return (double)ldexpl(((e < 0.0L) ? -e : e) / ((o < 0.0L) ? -o : o), 106);
}


//==============================================================================
// 2.  ARGUMENTS
//==============================================================================

// d_tests_double
//   a random double of either sign, its exponent _e_low to _e_high, its 52
// fraction bits random.
static double
d_tests_double(
    int _e_low,
    int _e_high
)
{
    const unsigned long n = (unsigned long)(_e_high - _e_low + 1);
    const int           e = _e_low + (int)d_tests_below(n);
    const long double   m = 4503599627370496.0L + d_tests_top(52);
    const double        x = ldexp((double)m, e - 52);

    return (d_tests_bit() != 0u) ? -x : x;
}

// d_tests_pair_at
//   a random normalized pair whose high part is _high: its low part 54 to 61
// binades below, or 0 one time in sixteen.
static struct dj_math_dd
d_tests_pair_at(
    double _high
)
{
    struct dj_math_dd result;
    int               e = 0;

    result.high = _high;
    (void)frexp(_high, &e);
    result.low = (d_tests_below(16u) == 0u)
                     ? 0.0
                     : d_tests_double(e - 55 - (int)d_tests_below(8u),
                                      e - 55);

    return result;
}

// d_tests_pair
//   a random normalized pair, its high part's exponent _e_low to _e_high.
static struct dj_math_dd
d_tests_pair(
    int _e_low,
    int _e_high
)
{
    return d_tests_pair_at(d_tests_double(_e_low, _e_high));
}

// d_tests_normalized
//   whether _x is normalized: high is high + low, rounded.
static bool
d_tests_normalized(
    struct dj_math_dd _x
)
{
    volatile double sum = _x.high + _x.low;

    return sum == _x.high;
}


//==============================================================================
// 3.  THE CHECKS
//==============================================================================

// d_tests_exact_sums
//   two_sum, fast_two_sum and two_product: high + low the exact sum or
// product, high its rounding, the pair normalized.
static void
d_tests_exact_sums(void)
{
    long i = 0;

    for (i = 0; i < 200000; ++i)
    {
        // exponents up to 120 apart, so that the low part is any part
        const double            a  = d_tests_double(-60, 60);
        const double            b  = d_tests_double(-60, 60);
        volatile double         sv = a + b;
        volatile double         pv = 0.0;
        const struct dj_math_dd s  = d_math_dd_two_sum(a, b);
        const struct dj_math_dd f  = ((a < 0.0 ? -a : a) >= (b < 0.0 ? -b : b))
                                         ? d_math_dd_fast_two_sum(a, b)
                                         : d_math_dd_fast_two_sum(b, a);
        // the product across the range: 2^-960 to 2^1000
        const double            c  = d_tests_double(-480, 500);
        const double            d  = d_tests_double(-480, 500);
        const struct dj_math_dd p  = d_math_dd_two_product(c, d);
        const struct dj_tests_dyadic exact_s =
            d_tests_dyadic_add(d_tests_dyadic_of(a), d_tests_dyadic_of(b));
        const struct dj_tests_dyadic exact_p =
            d_tests_dyadic_mul(d_tests_dyadic_of(c), d_tests_dyadic_of(d));

        pv = c * d;
        D_TESTS_EXPECT( (s.high == sv) && d_tests_normalized(s) &&
                        (d_tests_big_cmp(d_tests_dyadic_add(
                             d_tests_dyadic_pair(s),
                             d_tests_dyadic_neg(exact_s)).m,
                             d_tests_big_of(0)) == 0),
                        "two_sum: the exact sum", a);
        D_TESTS_EXPECT( (f.high == sv) && d_tests_normalized(f) &&
                        (d_tests_big_cmp(d_tests_dyadic_add(
                             d_tests_dyadic_pair(f),
                             d_tests_dyadic_neg(exact_s)).m,
                             d_tests_big_of(0)) == 0),
                        "fast_two_sum: the exact sum", a);
        D_TESTS_EXPECT( (p.high == pv) && d_tests_normalized(p) &&
                        (d_tests_big_cmp(d_tests_dyadic_add(
                             d_tests_dyadic_pair(p),
                             d_tests_dyadic_neg(exact_p)).m,
                             d_tests_big_of(0)) == 0),
                        "two_product: the exact product", c);
    }

    return;
}

// dj_tests_worst
//   struct: the largest error an operation shows, in u^2.
struct dj_tests_worst
{
    double add_d;
    double add;
    double mul_d;
    double mul;
    double div_d;
    double div;
    double sqrt;
};

// d_tests_track
//   _worst raised to the error of _got from _exact, and whether it lies
// within _bound u^2 and _got is normalized.
static bool
d_tests_track(
    double*                _worst,
    struct dj_math_dd      _got,
    struct dj_tests_dyadic _exact,
    dj_math_umax           _bound
)
{
    const struct dj_tests_dyadic err =
        d_tests_dyadic_add(d_tests_dyadic_pair(_got),
                           d_tests_dyadic_neg(_exact));
    const double e = d_tests_big_is_zero(err.m) ? 0.0
                                                : d_tests_in_u2(err, _exact);

    *_worst = (e > *_worst) ? e : *_worst;

    return d_tests_normalized(_got) &&
           d_tests_dyadic_within(err, _exact, _bound);
}

// d_tests_sums_products
//   add_d, add, sub, mul_d and mul within their bounds of the exact result;
// one sum in four cancels, the second pair the first's negative but for its
// low part, or a last place.
static void
d_tests_sums_products(
    struct dj_tests_worst* _worst
)
{
    long i = 0;

    for (i = 0; i < 200000; ++i)
    {
        const struct dj_math_dd x = d_tests_pair(-40, 40);
        struct dj_math_dd       y = d_tests_pair(-40, 40);
        const double            z = d_tests_double(-40, 40);
        const struct dj_tests_dyadic ex = d_tests_dyadic_pair(x);
        struct dj_tests_dyadic       ey;
        const struct dj_tests_dyadic ez = d_tests_dyadic_of(z);

        if ((i % 4) == 3)
        {
            y = d_tests_pair_at(((i % 8) == 3) ? -x.high
                                               : -nextafter(x.high, 0.0));
        }

        ey = d_tests_dyadic_pair(y);

        if (!d_tests_big_is_zero(d_tests_dyadic_add(ex, ez).m))
        {
            D_TESTS_EXPECT(d_tests_track(&_worst->add_d,
                                         d_math_dd_add_d(x, z),
                                         d_tests_dyadic_add(ex, ez), 2u),
                           "add_d within 2u^2 / (1 - 2u)", x.high);
        }

        if (!d_tests_big_is_zero(d_tests_dyadic_add(ex, ey).m))
        {
            D_TESTS_EXPECT(d_tests_track(&_worst->add, d_math_dd_add(x, y),
                                         d_tests_dyadic_add(ex, ey), 3u),
                           "add within 3u^2 / (1 - 4u)", x.high);
            D_TESTS_EXPECT(d_tests_track(&_worst->add,
                                         d_math_dd_sub(x, d_math_dd_neg(y)),
                                         d_tests_dyadic_add(ex, ey), 3u),
                           "sub within 3u^2 / (1 - 4u)", x.high);
        }

        D_TESTS_EXPECT(d_tests_track(&_worst->mul_d, d_math_dd_mul_d(x, z),
                                     d_tests_dyadic_mul(ex, ez), 3u),
                       "mul_d within 3u^2", x.high);
        D_TESTS_EXPECT(d_tests_track(&_worst->mul, d_math_dd_mul(x, y),
                                     d_tests_dyadic_mul(ex, ey), 7u),
                       "mul within 7u^2", x.high);
    }

    return;
}

// d_tests_residual
//   _worst raised to |_r| / (_scale |_of|) in u^2, and whether |_r| lies
// within _bound _scale u^2 |_of| and _got is normalized: |q y - x| against
// |x| for a quotient, |s^2 - x| against 2x for a square root.
static bool
d_tests_residual(
    double*                _worst,
    struct dj_math_dd      _got,
    struct dj_tests_dyadic _r,
    struct dj_tests_dyadic _of,
    dj_math_umax           _bound
)
{
    const double e = d_tests_big_is_zero(_r.m) ? 0.0 : d_tests_in_u2(_r, _of);

    *_worst = (e > *_worst) ? e : *_worst;

    return d_tests_normalized(_got) && d_tests_dyadic_within(_r, _of, _bound);
}

// d_tests_quotients
//   div_d and div within 12u^2 and 38u^2, by |q y - x| against |x|, which is
// |q - x/y| against |x/y|; sqrt within 32u^2, by |s^2 - x| against 2x, which
// bounds |s - sqrt(x)| against sqrt(x) (|s^2 - x| = |s - sqrt(x)| (s +
// sqrt(x))).
static void
d_tests_quotients(
    struct dj_tests_worst* _worst
)
{
    long i = 0;

    for (i = 0; i < 200000; ++i)
    {
        const struct dj_math_dd x = d_tests_pair(-40, 40);
        const struct dj_math_dd y = d_tests_pair(-40, 40);
        const double            z = d_tests_double(-40, 40);
        struct dj_math_dd       p = x;
        const struct dj_tests_dyadic ex = d_tests_dyadic_pair(x);
        const struct dj_math_dd qd = d_math_dd_div_d(x, z);
        const struct dj_math_dd q  = d_math_dd_div(x, y);
        struct dj_math_dd       s;
        struct dj_tests_dyadic  ep;

        D_TESTS_EXPECT(d_tests_residual(&_worst->div_d, qd,
            d_tests_dyadic_add(d_tests_dyadic_mul(d_tests_dyadic_pair(qd),
                                                  d_tests_dyadic_of(z)),
                               d_tests_dyadic_neg(ex)), ex, 12u),
            "div_d within 12u^2", x.high);
        D_TESTS_EXPECT(d_tests_residual(&_worst->div, q,
            d_tests_dyadic_add(d_tests_dyadic_mul(d_tests_dyadic_pair(q),
                                                  d_tests_dyadic_pair(y)),
                               d_tests_dyadic_neg(ex)), ex, 38u),
            "div within 38u^2", x.high);

        // the square root of |x|, its exponent even or odd
        p.high = (x.high < 0.0) ? -x.high : x.high;
        p.low  = (x.high < 0.0) ? -x.low : x.low;
        ep     = d_tests_dyadic_pair(p);
        s      = d_math_dd_sqrt(p);
        D_TESTS_EXPECT(d_tests_residual(&_worst->sqrt, s,
            d_tests_dyadic_add(d_tests_dyadic_mul(d_tests_dyadic_pair(s),
                                                  d_tests_dyadic_pair(s)),
                               d_tests_dyadic_neg(ep)),
            d_tests_dyadic_add(ep, ep), 32u),
            "sqrt within 32u^2", p.high);
    }

    return;
}

// d_tests_specials
//   of, value and neg; the square root of the zeros and of a negative.
static void
d_tests_specials(void)
{
    const struct dj_math_dd one   = d_math_dd_of(1.0);
    const struct dj_math_dd zero  = d_math_dd_of(0.0);
    const struct dj_math_dd mzero = d_math_dd_of(-0.0);
    const struct dj_math_dd neg   = d_math_dd_of(-4.0);
    struct dj_math_dd       pair;

    pair.high = 1.0;
    pair.low  = 1.0 / 9007199254740992.0;
    D_TESTS_EXPECT( (one.high == 1.0) && (one.low == 0.0) &&
                    (d_math_dd_value(pair) == 1.0) &&
                    (d_math_dd_neg(pair).high == -1.0) &&
                    (d_math_dd_neg(pair).low == -pair.low),
                    "of, value and neg", 0);
    D_TESTS_EXPECT( (d_math_dd_sqrt(zero).high == 0.0) &&
                    (!signbit(d_math_dd_sqrt(zero).high)) &&
                    (d_math_dd_sqrt(mzero).high == 0.0) &&
                    (signbit(d_math_dd_sqrt(mzero).high)) &&
                    (d_math_dd_sqrt(zero).low == 0.0),
                    "sqrt of the zeros is the zero", 0);
    D_TESTS_EXPECT(d_math_dd_sqrt(neg).high != d_math_dd_sqrt(neg).high,
                   "sqrt of a negative is NaN", 0);

    return;
}

#endif  // !D_MATH_DBL_EVAL_WIDE

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) &&                                             \
      !D_MATH_DBL_EVAL_WIDE )

// from C++14, every function is a constant expression
static constexpr double g_ca = 1.000000000931322574615478515625;
static constexpr double g_cb = 0.999999999068677425384521484375;
static constexpr struct dj_math_dd g_c_prod =
    d_math_dd_two_product(g_ca, g_cb);
static constexpr struct dj_math_dd g_c_sum  = d_math_dd_two_sum(1.0e16, 1.0);
static constexpr struct dj_math_dd g_c_fast = d_math_dd_fast_two_sum(1.0e16,
                                                                     1.0);
static constexpr struct dj_math_dd g_c_x    = { 3.0, 1.0e-16 };
static constexpr struct dj_math_dd g_c_y    = { 7.0, -2.0e-16 };
static constexpr struct dj_math_dd g_c_add  = d_math_dd_add(g_c_x, g_c_y);
static constexpr struct dj_math_dd g_c_mul  = d_math_dd_mul(g_c_x, g_c_y);
static constexpr struct dj_math_dd g_c_div  = d_math_dd_div(g_c_x, g_c_y);
static constexpr struct dj_math_dd g_c_divd = d_math_dd_div_d(g_c_x, 7.0);
static constexpr struct dj_math_dd g_c_sqrt = d_math_dd_sqrt(g_c_x);
static constexpr struct dj_math_dd g_c_muld = d_math_dd_mul_d(g_c_x, 7.0);

// (1 + 2^-30)(1 - 2^-30) = 1 - 2^-60: 1, and -2^-60
static_assert( (g_c_prod.high == 1.0) &&
               (g_c_prod.low == -1.0 / 1152921504606846976.0),
               "two_product: the exact product, a constant expression");
static_assert( (g_c_sum.high == 1.0e16) && (g_c_sum.low == 1.0) &&
               (g_c_fast.high == 1.0e16) && (g_c_fast.low == 1.0),
               "two_sum and fast_two_sum: 10^16 + 1 exactly");
static_assert( (g_c_add.high == 10.0) && (g_c_mul.high == 21.0) &&
               (g_c_muld.high == 21.0),
               "add, mul and mul_d, constant expressions");

// d_tests_constant_evaluation
//   the constants are the run time's results, bit for bit: the exact sums
// and product always, the rest where the build does not fuse (S415).
static void
d_tests_constant_evaluation(void)
{
    volatile double         a  = g_ca;
    volatile double         b  = g_cb;
    volatile double         big = 1.0e16;
    volatile double         one = 1.0;
    volatile double         xh = 3.0;
    volatile double         yh = 7.0;
    struct dj_math_dd       x;
    struct dj_math_dd       y;
    const struct dj_math_dd p  = d_math_dd_two_product(a, b);
    const struct dj_math_dd s  = d_math_dd_two_sum(big, one);

    x.high = xh;
    x.low  = g_c_x.low;
    y.high = yh;
    y.low  = g_c_y.low;
    D_TESTS_EXPECT( (p.high == g_c_prod.high) && (p.low == g_c_prod.low) &&
                    (s.high == g_c_sum.high) && (s.low == g_c_sum.low),
                    "constant evaluation agrees with the run time: the exact "
                    "sums and product", 0);
    D_TESTS_EXPECT( d_math_formulas_fuse() ||
                    ( (d_math_dd_add(x, y).low == g_c_add.low) &&
                      (d_math_dd_mul(x, y).low == g_c_mul.low) &&
                      (d_math_dd_mul_d(x, yh).low == g_c_muld.low) &&
                      (d_math_dd_div(x, y).low == g_c_div.low) &&
                      (d_math_dd_div_d(x, yh).low == g_c_divd.low) &&
                      (d_math_dd_sqrt(x).high == g_c_sqrt.high) &&
                      (d_math_dd_sqrt(x).low == g_c_sqrt.low) ),
                    "constant evaluation agrees with the run time: add, mul, "
                    "mul_d, div, div_d, sqrt", 0);

    return;
}

#endif  // C++14

int
main(void)
{
#if !D_MATH_DBL_EVAL_WIDE
    struct dj_tests_worst worst = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    // the generator's start, the word 0x2545F4914F6CDD1D
    d_tests_seed(0x2545F491u, 0x4F6CDD1Du);
    d_tests_exact_sums();
    d_tests_sums_products(&worst);
    d_tests_quotients(&worst);
    d_tests_specials();
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif
    D_TESTS_EXPECT(!g_reference_overrun, "the reference never overran", 0);
    printf("dd: the largest errors, in u^2 (bounds): add_d %.3f (2), add "
           "%.3f (3), mul_d %.3f (3), mul %.3f (7), div_d %.3f (12), div "
           "%.3f (38), sqrt %.3f (32); this build %s\n", worst.add_d,
           worst.add, worst.mul_d, worst.mul, worst.div_d, worst.div,
           worst.sqrt, d_math_formulas_fuse() ? "fuses" : "does not fuse");
#else
    printf("dd: skipped, double evaluated wider than double: dd.h refuses to "
           "compile here (S418)\n");
#endif  // !D_MATH_DBL_EVAL_WIDE

    printf("t_dd: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
