/*******************************************************************************
* djinterp [c/math]                                              t_compensated.c
*
*   Conformance harness for the C core's compensated sums and refined solve
* (S404, S406), all three families, against exact arithmetic. vec.h's dot,
* sum and cross promise what Ogita, Rump and Oishi prove of Dot2 and Sum2:
* the result is the correct rounding of a value within 4 n^2 u^2 S of the
* exact sum, S the sum of the terms' magnitudes and u the unit of the pairs
* the family sums in (2^-53 for float and double, 2^-P for long double) --
* so correctly rounded unless the exact sum lies within that of a midpoint.
* Over 20,000 random cases per family and kernel, of 2 to 8 terms, a quarter
* each with no cancellation, with the terms cancelling to half the
* precision, past it, and to the naive sum's own rounding error, each
* result is checked so, exactly: the sum in integers of 16-bit limbs
* (math_tests_exact.h), the rounding by the oracle there, the midpoint's
* distance in integers. The counts of correctly rounded results, the
* compensated forms' and the _fast forms', are printed. Then the cases a
* careless kernel gets wrong: a sum the naive formula overflows, products
* past the range, a subnormal sum on a midpoint that the low part decides,
* a cross product's signed zero, NaNs and infinities as the formula gives
* them. Then mat.h's products and trace, 2,000 per family, a tenth with a
* row past the range: each entry the compensated dot of its row and column,
* the trace the compensated sum of the diagonal. Then mat.h's refined solve
* on the systems of math_tests_solve_refs.h:
* where cond(A) u lies below 2^-6, every entry the exact solution correctly
* rounded; and on every system, where the family is evaluated in its own
* type, solve_fast's entries as d_mat_*_solve's first solution, through the
* factors the solve keeps.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_compensated.c -o t_compensated -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_compensated.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// the exact reference's integers: 768 bits, for sums of products of the
// x87's 64-bit values over an exponent window of 2^16
#define D_TESTS_EXACT_LIMBS 48

// std
#include <float.h>    // FLT_MAX ... LDBL_MANT_DIG
#include <limits.h>   // INT_MAX
#include <math.h>     // frexpf, frexp, frexpl, nextafterf, nextafter,
                      // nextafterl
#include <stdio.h>    // printf
// djinterp
#include "./math_tests_exact.h"       // d_tests_big_*, d_tests_nearest_*
#include "./math_tests_solve_refs.h"  // g_systems_*
#include "../../../../../inc/djinterp/c/modules/math/mat.h"  // under test


static long          g_checks   = 0;
static long          g_failures = 0;
static unsigned long g_state    = 271828183u;

// d_tests_expect
//   one check: counted, and reported where it fails, the first few times.
static void
d_tests_expect(
    bool        _ok,
    const char* _what,
    int         _line
)
{
    ++g_checks;

    // the first few failures, with what was checked and where
    if (!_ok)
    {
        ++g_failures;

        if (g_failures <= 20)
        {
            printf("  FAIL line %d: %s\n", _line, _what);
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what) d_tests_expect((ok), (what), __LINE__)

// d_tests_next
//   24 random bits, a linear congruence kept to 32 bits so that every
// target draws the same.
static unsigned long
d_tests_next(void)
{
    g_state = ((g_state * 1103515245u) + 12345u) & 0xFFFFFFFFu;

    return (g_state >> 4) & 0xFFFFFFu;
}

// d_tests_mantissa
//   a random value in [1, 2) of 72 random bits.
static long double
d_tests_mantissa(void)
{
    const long double unit = 16777216.0L;
    const long double low  = (long double)d_tests_next() / unit;
    const long double mid  = ((long double)d_tests_next() + low) / unit;

    return 1.0L + (((long double)d_tests_next() + mid) / unit);
}

// d_tests_shl
//   _x * 2^_k, _k >= 0, its sign kept: d_tests_big_shl shifts the
// magnitude.
static struct dj_tests_big
d_tests_shl(
    struct dj_tests_big _x,
    int                 _k
)
{
    const struct dj_tests_big m = d_tests_big_shl(_x, _k);

    return (_x.negative != 0) ? d_tests_big_neg(m) : m;
}

// d_tests_count
//   a test's tally of one kernel: its cases, those correctly rounded, and
// those its _fast form rounded correctly.
struct dj_tests_count
{
    long cases;
    long nearest;
    long fast_nearest;
};

#define D_TESTS_COMPENSATED_FAMILY(T, S, FREXP, NEXT, P, UW, MINV, MAXV,      \
                                   MAXEXP, WIDE)                               \
/* x 2^-e as an integer, exact: e at or below x's lowest bit */                \
static struct dj_tests_big                                                     \
d_tests_fixed_##S(T _x, int _e)                                                \
{                                                                              \
    int                   ex    = 0;                                           \
    const T               mag   = (_x < (T)0) ? -_x : _x;                      \
    const T               f     = FREXP(mag, &ex);                             \
    struct dj_tests_parts parts = d_tests_parts_of((long double)f, ex, P);     \
    struct dj_tests_big   m     = d_tests_big_shl(parts.mantissa,              \
                                                  parts.exponent - _e);        \
    return (_x < (T)0) ? d_tests_big_neg(m) : m;                               \
}                                                                              \
                                                                               \
/* the exponent of x's lowest bit, or INT_MAX for 0 */                         \
static int                                                                     \
d_tests_low_##S(T _x)                                                          \
{                                                                              \
    int ex = 0;                                                                \
    if (_x == (T)0)                                                            \
    {                                                                          \
        return INT_MAX;                                                        \
    }                                                                          \
    (void)FREXP((_x < (T)0) ? -_x : _x, &ex);                                  \
    return ex - P;                                                             \
}                                                                              \
                                                                               \
/* a random value: a sign, 72 random bits and an exponent in [-8, 8] */        \
static T                                                                       \
d_tests_entry_##S(void)                                                        \
{                                                                              \
    const long double m = d_tests_mantissa();                                  \
    const int         e = (int)(d_tests_next() % 17u) - 8;                     \
    const T           v = (T)ldexpl(m, e);                                     \
    return ((d_tests_next() & 1u) != 0u) ? -v : v;                             \
}                                                                              \
                                                                               \
/* whether _r is the correct rounding of a value within 4 _n^2 2^-2UW s of */  \
/* x -- x = _x 2^_e, s = _s 2^_e: the oracle's nearest value to x, or x    */  \
/* within that of the midpoint between _r and its neighbour toward x;      */  \
/* *_nearest whether it is the nearest. A 0 is x's nearest where x is 0,   */  \
/* and else within where x is, the cases' sums lying far above the least   */  \
/* subnormal                                                               */  \
static bool                                                                    \
d_tests_within_##S(T _r, struct dj_tests_big _x, struct dj_tests_big _s,       \
                   int _e, size_t _n, bool* _nearest)                          \
{                                                                              \
    const T lo = NEXT(_r, -MAXV);                                              \
    const T hi = NEXT(_r, MAXV);                                               \
    int     e  = _e;                                                           \
    struct dj_tests_big x, s, r, nb, twice, gap, bound, unit;                  \
    bound = d_tests_big_mul(_s, d_tests_big_of((dj_math_imax)(8u * _n * _n))); \
    if (_r == (T)0)                                                            \
    {                                                                          \
        *_nearest = d_tests_big_is_zero(_x);                                   \
        return d_tests_big_cmp(                                                \
                   d_tests_big_shl(d_tests_big_abs(_x), (2 * UW) + 1),         \
                   bound) <= 0;                                                \
    }                                                                          \
    e = (d_tests_low_##S(_r) - 1 < e) ? d_tests_low_##S(_r) - 1 : e;           \
    e = (d_tests_low_##S(lo) - 1 < e) ? d_tests_low_##S(lo) - 1 : e;           \
    e = (d_tests_low_##S(hi) - 1 < e) ? d_tests_low_##S(hi) - 1 : e;           \
    x = d_tests_shl(_x, _e - e);                                               \
    s = d_tests_big_shl(_s, _e - e);                                           \
    r = d_tests_fixed_##S(_r, e);                                              \
    unit = d_tests_big_shl(d_tests_big_of(1), (e < 0) ? -e : 0);               \
    *_nearest = d_tests_nearest_##S(x.negative != 0,                           \
                                    d_tests_big_shl(d_tests_big_abs(x),        \
                                                    (e > 0) ? e : 0),          \
                                    unit, _r);                                 \
    if (*_nearest)                                                             \
    {                                                                          \
        return true;                                                           \
    }                                                                          \
    nb    = d_tests_fixed_##S((d_tests_big_cmp(x, r) > 0) ? hi : lo, e);       \
    twice = d_tests_big_add(x, x);                                             \
    gap   = d_tests_big_abs(d_tests_big_add(twice,                             \
                                d_tests_big_neg(d_tests_big_add(r, nb))));     \
    bound = d_tests_big_mul(s, d_tests_big_of((dj_math_imax)(8u * _n * _n)));  \
    return d_tests_big_cmp(d_tests_big_shl(gap, 2 * UW), bound) <= 0;          \
}                                                                              \
                                                                               \
/* a case's last term: a cancelling one at level 1 to 3, cancelling to */      \
/* half the precision, past it, and to the naive sum's rounding error */       \
static T                                                                       \
d_tests_cancel_##S(T _s, int _level)                                           \
{                                                                              \
    const long double d = (_level == 1) ? ldexpl(d_tests_mantissa(), -(P / 2)) \
                        : (_level == 2) ? ldexpl(d_tests_mantissa(), -(P + 2)) \
                                        : 0.0L;                                \
    return (T)(-((long double)_s + ((long double)_s * d)));                    \
}                                                                              \
                                                                               \
/* dot: 20,000 cases of 2 to 8 terms */                                        \
static void                                                                    \
d_tests_dot_##S(struct dj_tests_count* _count)                                 \
{                                                                              \
    long trial = 0;                                                            \
    for (trial = 0; trial < 20000; ++trial)                                    \
    {                                                                          \
        const size_t n     = (size_t)(trial % 7) + 2u;                         \
        const int    level = (int)((trial / 7) % 4);                           \
        T            a[8], b[8];                                               \
        int          ea = INT_MAX, eb = INT_MAX;                               \
        size_t       i  = 0u;                                                  \
        bool         near = false, fast_near = false, ok = false;              \
        struct dj_tests_big x = d_tests_big_of(0), s = d_tests_big_of(0);      \
        for (i = 0u; i + 1u < n; ++i)                                          \
        {                                                                      \
            a[i] = d_tests_entry_##S();                                        \
            b[i] = d_tests_entry_##S();                                        \
        }                                                                      \
        a[n - 1u] = (level == 0) ? d_tests_entry_##S()                         \
                    : d_tests_cancel_##S(d_vec_##S##_dot_fast(a, b, n - 1u),   \
                                         level);                               \
        b[n - 1u] = (level == 0) ? d_tests_entry_##S() : (T)1;                 \
        for (i = 0u; i < n; ++i)                                               \
        {                                                                      \
            ea = (d_tests_low_##S(a[i]) < ea) ? d_tests_low_##S(a[i]) : ea;    \
            eb = (d_tests_low_##S(b[i]) < eb) ? d_tests_low_##S(b[i]) : eb;    \
        }                                                                      \
        for (i = 0u; i < n; ++i)                                               \
        {                                                                      \
            const struct dj_tests_big p =                                      \
                d_tests_big_mul(d_tests_fixed_##S(a[i], ea),                   \
                                d_tests_fixed_##S(b[i], eb));                  \
            x = d_tests_big_add(x, p);                                         \
            s = d_tests_big_add(s, d_tests_big_abs(p));                        \
        }                                                                      \
        ok = d_tests_within_##S(d_vec_##S##_dot(a, b, n), x, s, ea + eb, n,    \
                                &near);                                        \
        D_TESTS_EXPECT(ok, #S " dot: Dot2's rounding of a near value");        \
        (void)d_tests_within_##S(d_vec_##S##_dot_fast(a, b, n), x, s, ea + eb, \
                                 n, &fast_near);                               \
        _count->cases        += 1;                                             \
        _count->nearest      += near ? 1 : 0;                                  \
        _count->fast_nearest += fast_near ? 1 : 0;                             \
    }                                                                          \
    return;                                                                    \
}                                                                              \
                                                                               \
/* sum: 20,000 cases of 2 to 8 terms */                                        \
static void                                                                    \
d_tests_sum_##S(struct dj_tests_count* _count)                                 \
{                                                                              \
    long trial = 0;                                                            \
    for (trial = 0; trial < 20000; ++trial)                                    \
    {                                                                          \
        const size_t n     = (size_t)(trial % 7) + 2u;                         \
        const int    level = (int)((trial / 7) % 4);                           \
        T            a[8];                                                     \
        int          ea = INT_MAX;                                             \
        size_t       i  = 0u;                                                  \
        bool         near = false, fast_near = false, ok = false;              \
        struct dj_tests_big x = d_tests_big_of(0), s = d_tests_big_of(0);      \
        for (i = 0u; i + 1u < n; ++i)                                          \
        {                                                                      \
            a[i] = d_tests_entry_##S();                                        \
        }                                                                      \
        a[n - 1u] = (level == 0) ? d_tests_entry_##S()                         \
                    : d_tests_cancel_##S(d_vec_##S##_sum_fast(a, n - 1u),      \
                                         level);                               \
        for (i = 0u; i < n; ++i)                                               \
        {                                                                      \
            ea = (d_tests_low_##S(a[i]) < ea) ? d_tests_low_##S(a[i]) : ea;    \
        }                                                                      \
        for (i = 0u; i < n; ++i)                                               \
        {                                                                      \
            const struct dj_tests_big t = d_tests_fixed_##S(a[i], ea);         \
            x = d_tests_big_add(x, t);                                         \
            s = d_tests_big_add(s, d_tests_big_abs(t));                        \
        }                                                                      \
        ok = d_tests_within_##S(d_vec_##S##_sum(a, n), x, s, ea, n, &near);    \
        D_TESTS_EXPECT(ok, #S " sum: Sum2's rounding of a near value");        \
        (void)d_tests_within_##S(d_vec_##S##_sum_fast(a, n), x, s, ea, n,      \
                                 &fast_near);                                  \
        _count->cases        += 1;                                             \
        _count->nearest      += near ? 1 : 0;                                  \
        _count->fast_nearest += fast_near ? 1 : 0;                             \
    }                                                                          \
    return;                                                                    \
}                                                                              \
                                                                               \
/* cross: 20,000 pairs of 3-vectors, nearly parallel past level 0 */           \
static void                                                                    \
d_tests_cross_##S(struct dj_tests_count* _count)                               \
{                                                                              \
    long trial = 0;                                                            \
    for (trial = 0; trial < 20000; ++trial)                                    \
    {                                                                          \
        const int level = (int)(trial % 4);                                    \
        const int shift = (level == 0) ? 0 : (level == 1) ? (P / 2)            \
                        : (level == 2) ? P : (P + 8);                          \
        T         a[3], b[3], c[3], cf[3];                                     \
        size_t    k = 0u;                                                      \
        for (k = 0u; k < 3u; ++k)                                              \
        {                                                                      \
            const T e = d_tests_entry_##S();                                   \
            a[k] = d_tests_entry_##S();                                        \
            b[k] = (level == 0) ? e : (T)((long double)a[k] * 3.0L +           \
                                          ldexpl((long double)e, -shift));     \
        }                                                                      \
        d_vec_##S##_cross(c, a, b);                                            \
        d_vec_##S##_cross_fast(cf, a, b);                                      \
        for (k = 0u; k < 3u; ++k)                                              \
        {                                                                      \
            const size_t i = (k + 1u) % 3u, j = (k + 2u) % 3u;                 \
            const int    ea = (d_tests_low_##S(a[i]) < d_tests_low_##S(a[j]))  \
                                  ? d_tests_low_##S(a[i])                      \
                                  : d_tests_low_##S(a[j]);                     \
            const int    eb = (d_tests_low_##S(b[i]) < d_tests_low_##S(b[j]))  \
                                  ? d_tests_low_##S(b[i])                      \
                                  : d_tests_low_##S(b[j]);                     \
            const struct dj_tests_big p =                                      \
                d_tests_big_mul(d_tests_fixed_##S(a[i], ea),                   \
                                d_tests_fixed_##S(b[j], eb));                  \
            const struct dj_tests_big q =                                      \
                d_tests_big_mul(d_tests_fixed_##S(a[j], ea),                   \
                                d_tests_fixed_##S(b[i], eb));                  \
            const struct dj_tests_big x =                                      \
                d_tests_big_add(p, d_tests_big_neg(q));                        \
            const struct dj_tests_big s =                                      \
                d_tests_big_add(d_tests_big_abs(p), d_tests_big_abs(q));       \
            bool near = false, fast_near = false;                              \
            D_TESTS_EXPECT(d_tests_within_##S(c[k], x, s, ea + eb, 2u, &near), \
                           #S " cross: Dot2's rounding of a near value");      \
            (void)d_tests_within_##S(cf[k], x, s, ea + eb, 2u, &fast_near);    \
            _count->cases        += 1;                                         \
            _count->nearest      += near ? 1 : 0;                              \
            _count->fast_nearest += fast_near ? 1 : 0;                         \
        }                                                                      \
    }                                                                          \
    return;                                                                    \
}                                                                              \
                                                                               \
/* the cases a careless kernel gets wrong */                                   \
static void                                                                    \
d_tests_edges_##S(void)                                                        \
{                                                                              \
    const T one  = (T)1;                                                       \
    const T tiny = MINV * (T)((T)1 / d_internal_math_##S##_pow2(P - 1));       \
    const T big  = d_internal_math_##S##_pow2((MAXEXP / 2) + 4);               \
    T       out[3], want[3];                                                   \
    /* a sum the naive formula overflows */                                    \
    {                                                                          \
        const T a[3] = { MAXV, MAXV, -MAXV };                                  \
        const T b[3] = { one, one, one };                                      \
        D_TESTS_EXPECT( (d_vec_##S##_dot(a, b, 3u) == MAXV) &&                 \
                        (d_vec_##S##_sum(a, 3u) == MAXV) &&                    \
                        (d_vec_##S##_dot_fast(a, b, 3u) > MAXV),               \
                        #S " a sum the formula overflows: MAX");               \
    }                                                                          \
    /* products past the range, cancelling exactly: +0, the formula NaN */     \
    {                                                                          \
        const T a[2] = { big * (T)3, big };                                    \
        const T b[2] = { big, -(big * (T)3) };                                 \
        const T d    = d_vec_##S##_dot(a, b, 2u);                              \
        const T f    = d_vec_##S##_dot_fast(a, b, 2u);                         \
        D_TESTS_EXPECT( (d == (T)0) && (!signbit(d)) && (f != f),              \
                        #S " products past the range, cancelling: +0");        \
    }                                                                          \
    /* products far below the range: 1.5 tiny - tiny 2^-30, a subnormal */     \
    /* below the midpoint between tiny and 2 tiny, which the low part's  */    \
    /* sign takes to tiny; the formula's rounding takes it to 2 tiny     */    \
    {                                                                          \
        const T p20  = d_internal_math_##S##_pow2(20);                         \
        const T p10  = d_internal_math_##S##_pow2(10);                         \
        const T a[2] = { (T)1.5 / p20, -((T)1 / (p20 * p20)) };                \
        const T b[2] = { tiny * p20, tiny * p10 };                             \
        D_TESTS_EXPECT( (d_vec_##S##_dot(a, b, 2u) == tiny) &&                 \
                        (d_vec_##S##_dot_fast(a, b, 2u) == tiny * (T)2),       \
                        #S " a subnormal sum below the midpoint: tiny");       \
    }                                                                          \
    /* a cross product's signed zero: the formula's */                         \
    {                                                                          \
        const T a[3] = { (T)2, -(T)0, (T)0 };                                  \
        const T b[3] = { (T)3, one, one };                                     \
        d_vec_##S##_cross(out, a, b);                                          \
        d_vec_##S##_cross_fast(want, a, b);                                    \
        D_TESTS_EXPECT( (out[0] == want[0]) &&                                 \
                        ((signbit(out[0]) != 0) ==                             \
                         (signbit(want[0]) != 0)) &&                           \
                        (signbit(out[0]) != 0) &&                              \
                        (out[1] == want[1]) && (out[2] == want[2]),            \
                        #S " a cross product's -0, as the formula gives it");  \
    }                                                                          \
    /* NaNs and infinities: the formula's */                                   \
    {                                                                          \
        const T inf  = MAXV * (T)2;                                            \
        const T a[3] = { inf, one, (T)2 };                                     \
        const T b[3] = { one, (T)0, inf };                                     \
        const T z[3] = { (T)0, one, one };                                     \
        const T d    = d_vec_##S##_dot(a, b, 3u);                              \
        D_TESTS_EXPECT( (d == d_vec_##S##_dot_fast(a, b, 3u)) &&               \
                        (d_vec_##S##_dot(a, z, 3u) !=                          \
                         d_vec_##S##_dot(a, z, 3u)) &&                         \
                        (d_vec_##S##_sum(a, 3u) == inf) &&                     \
                        (d_mat_##S##_trace(a, 1u) == inf),                     \
                        #S " NaNs and infinities as the formula gives them");  \
    }                                                                          \
    return;                                                                    \
}                                                                              \
                                                                               \
/* mat.h's products and trace: each entry the compensated dot of its row */  \
/* and column, the trace the compensated sum of the diagonal -- on 2,000   */  \
/* random products of orders 1 to 5, a row of each tenth past the range    */  \
static void                                                                    \
d_tests_products_##S(void)                                                     \
{                                                                              \
    long trial = 0;                                                            \
    for (trial = 0; trial < 2000; ++trial)                                     \
    {                                                                          \
        const size_t r = (size_t)(trial % 5) + 1u;                             \
        const size_t c = (size_t)((trial / 5) % 5) + 1u;                       \
        const size_t q = (size_t)((trial / 25) % 5) + 1u;                      \
        const T      far = d_internal_math_##S##_pow2((MAXEXP / 2) + 8);       \
        T            a[25], b[25], v[5], out[25], w[5], col[5], diag[5];       \
        size_t       i = 0u, j = 0u;                                           \
        bool         same = true;                                              \
        for (i = 0u; i < 25u; ++i)                                             \
        {                                                                      \
            a[i] = d_tests_entry_##S();                                        \
            b[i] = d_tests_entry_##S();                                        \
        }                                                                      \
        for (i = 0u; i < 5u; ++i)                                              \
        {                                                                      \
            v[i] = d_tests_entry_##S();                                        \
        }                                                                      \
        if ((trial % 10) == 0)                                                 \
        {                                                                      \
            for (j = 0u; j < c; ++j)                                           \
            {                                                                  \
                a[j] = a[j] * far;                                             \
            }                                                                  \
        }                                                                      \
        d_mat_##S##_multiply(out, a, b, r, c, q);                              \
        for (i = 0u; i < r; ++i)                                               \
        {                                                                      \
            for (j = 0u; j < q; ++j)                                           \
            {                                                                  \
                size_t k = 0u;                                                 \
                for (k = 0u; k < c; ++k)                                       \
                {                                                              \
                    col[k] = b[(k * q) + j];                                   \
                }                                                              \
                same = same && (out[(i * q) + j] ==                            \
                                d_vec_##S##_dot(a + (i * c), col, c));         \
            }                                                                  \
        }                                                                      \
        D_TESTS_EXPECT(same, #S " multiply: each entry its row's and "         \
                             "column's dot");                                  \
        d_mat_##S##_multiply_vector(w, a, v, r, c);                            \
        same = true;                                                           \
        for (i = 0u; i < r; ++i)                                               \
        {                                                                      \
            same = same && (w[i] == d_vec_##S##_dot(a + (i * c), v, c));       \
        }                                                                      \
        D_TESTS_EXPECT(same, #S " multiply_vector: each entry a row's dot");   \
        for (i = 0u; i < r; ++i)                                               \
        {                                                                      \
            diag[i] = a[(i * r) + i];                                          \
        }                                                                      \
        D_TESTS_EXPECT(d_mat_##S##_trace(a, r) == d_vec_##S##_sum(diag, r),    \
                       #S " trace: the diagonal's sum");                       \
    }                                                                          \
    return;                                                                    \
}                                                                              \
                                                                               \
/* a reference's value, (m[0] 2^48 + m[1] 2^24 + m[2]) 2^e, exactly */        \
static T                                                                       \
d_tests_value_##S(const struct dj_tests_solve_value* _v)                       \
{                                                                              \
    const long double m = ((long double)_v->m[0] * 281474976710656.0L) +       \
                          ((long double)_v->m[1] * 16777216.0L) +              \
                          (long double)_v->m[2];                               \
    return (T)ldexpl(m, _v->e);                                                \
}                                                                              \
                                                                               \
/* the refined solve on the references: each well-conditioned system's */     \
/* solution exactly rounded; every system's first solution solve_fast's */     \
static void                                                                    \
d_tests_solve_##S(long* _well, long* _exact, long* _fast_exact,                \
                  long* _entries)                                              \
{                                                                              \
    size_t k = 0u;                                                             \
    for (k = 0u; k < sizeof(g_systems_##S) / sizeof(g_systems_##S[0]); ++k)   \
    {                                                                          \
        const struct dj_tests_system_##S* sys = &g_systems_##S[k];             \
        const size_t n = sys->n;                                               \
        T            x[D_TESTS_SOLVE_NMAX], xf[D_TESTS_SOLVE_NMAX];            \
        T            scratch[(D_TESTS_SOLVE_NMAX * D_TESTS_SOLVE_NMAX) +       \
                             (2 * D_TESTS_SOLVE_NMAX)];                        \
        T            sf[D_TESTS_SOLVE_NMAX * D_TESTS_SOLVE_NMAX];              \
        T            a[D_TESTS_SOLVE_NMAX * D_TESTS_SOLVE_NMAX];               \
        T            b[D_TESTS_SOLVE_NMAX], want[D_TESTS_SOLVE_NMAX];          \
        size_t       pivots[D_TESTS_SOLVE_NMAX];                               \
        size_t       i = 0u;                                                   \
        bool         exact = true, fast_exact = true;                          \
        for (i = 0u; i < n * n; ++i)                                           \
        {                                                                      \
            a[i] = d_tests_value_##S(&sys->a[i]);                              \
        }                                                                      \
        for (i = 0u; i < n; ++i)                                               \
        {                                                                      \
            b[i]    = d_tests_value_##S(&sys->b[i]);                           \
            want[i] = d_tests_value_##S(&sys->x[i]);                           \
        }                                                                      \
        D_TESTS_EXPECT( (d_mat_##S##_solve(x, scratch, pivots, a, b, n, 1u)) &&\
                        (d_mat_##S##_solve_fast(xf, sf, a, b, n, 1u)),         \
                        #S " solve: a system the references hold, solved");    \
        for (i = 0u; i < n; ++i)                                               \
        {                                                                      \
            exact      = exact && (x[i] == want[i]);                           \
            fast_exact = fast_exact && (xf[i] == want[i]);                     \
            *_exact      += (x[i] == want[i]) ? 1 : 0;                         \
            *_fast_exact += (xf[i] == want[i]) ? 1 : 0;                        \
            *_entries    += 1;                                                 \
        }                                                                      \
        if (sys->well)                                                         \
        {                                                                      \
            *_well += 1;                                                       \
            D_TESTS_EXPECT(exact, #S " solve: the exact solution rounded");    \
        }                                                                      \
        /* the kept factors give solve_fast's first solution, where   */       \
        /* the family is evaluated in its own type: wider, a value's   */      \
        /* bits turn on where the compiler spills it                   */      \
        for (i = 0u; i < n; ++i)                                               \
        {                                                                      \
            x[i] = b[i];                                                       \
        }                                                                      \
        d_internal_mat_##S##_lu_solve(x, 1u, scratch, pivots, n);              \
        D_TESTS_EXPECT( (WIDE) ||                                              \
                        (d_vec_##S##_equal(x, xf, n)),                         \
                       #S " solve: the factors kept give solve_fast's bits");  \
    }                                                                          \
    return;                                                                    \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_family_##S(void)                                                       \
{                                                                              \
    struct dj_tests_count dot = { 0, 0, 0 }, sum = { 0, 0, 0 };                \
    struct dj_tests_count cross = { 0, 0, 0 };                                 \
    long well = 0, exact = 0, fast_exact = 0, entries = 0;                     \
    d_tests_dot_##S(&dot);                                                     \
    d_tests_sum_##S(&sum);                                                     \
    d_tests_cross_##S(&cross);                                                 \
    d_tests_edges_##S();                                                       \
    d_tests_products_##S();                                                    \
    d_tests_solve_##S(&well, &exact, &fast_exact, &entries);                   \
    printf("%-2s dot:   %ld cases, %ld correctly rounded; dot_fast %ld\n",     \
           #S, dot.cases, dot.nearest, dot.fast_nearest);                      \
    printf("%-2s sum:   %ld cases, %ld correctly rounded; sum_fast %ld\n",     \
           #S, sum.cases, sum.nearest, sum.fast_nearest);                      \
    printf("%-2s cross: %ld parts, %ld correctly rounded; cross_fast %ld\n",   \
           #S, cross.cases, cross.nearest, cross.fast_nearest);                \
    printf("%-2s solve: %ld entries of %ld systems (%ld well conditioned), "   \
           "%ld exact; solve_fast %ld\n",                                      \
           #S, entries,                                                        \
           (long)(sizeof(g_systems_##S) / sizeof(g_systems_##S[0])), well,     \
           exact, fast_exact);                                                 \
    return;                                                                    \
}

D_TESTS_COMPENSATED_FAMILY(float, f, frexpf, nextafterf, FLT_MANT_DIG, 53,
                           FLT_MIN, FLT_MAX, FLT_MAX_EXP, D_MATH_FLT_EVAL_WIDE)
D_TESTS_COMPENSATED_FAMILY(double, d, frexp, nextafter, DBL_MANT_DIG, 53,
                           DBL_MIN, DBL_MAX, DBL_MAX_EXP, D_MATH_DBL_EVAL_WIDE)
#if (LDBL_MANT_DIG == 64)
D_TESTS_COMPENSATED_FAMILY(long double, ld, frexpl, nextafterl, LDBL_MANT_DIG,
                           LDBL_MANT_DIG, LDBL_MIN, LDBL_MAX, LDBL_MAX_EXP,
                           D_MATH_LDBL_EVAL_WIDE)
#endif  // (LDBL_MANT_DIG == 64)

int
main(void)
{
    d_tests_family_f();
    d_tests_family_d();
#if (LDBL_MANT_DIG == 64)
    d_tests_family_ld();
#endif  // (LDBL_MANT_DIG == 64)

    D_TESTS_EXPECT(!g_reference_overrun,
                   "the exact reference never passed its limbs");
    printf("t_compensated: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
