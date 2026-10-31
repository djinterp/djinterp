/*******************************************************************************
* djinterp [test]                                                  t_remainder.c
*
*   Conformance harness for c/math/remainder.h. fmod_c, remainder_c and
* remquo_c must be exact, and so the C library's fmod, remainder and
* remquo, exact by C's definition, bit for bit: two million argument pairs
* for float and for double, in six ranges -- any two values, the subnormals
* among them; |x| up to 2^60 times |y|; |x| and |y| within a factor of 2; a
* subnormal y under a normal x; x in the top binade over y about the least
* subnormal, the longest reductions there are; x a whole multiple of y or a
* last place either side of one, and angles reduced by 2 pi -- against
* fmodf, fmod, remainderf, remainder, remquof and remquo; and for long
* double, in whatever format, two hundred thousand against fmodl,
* remainderl and remquol, the longest reductions at one pair in sixty.
* remquo_c's remainder must be remainder_c's, and its quotient congruent
* mod 8 to the library's, with x / y's sign. For remainder and remquo,
* besides, a hundred thousand ties a family -- x a whole multiple and a
* half of y, exactly -- where the quotient goes to the even integer, and a
* hundred thousand whole multiples, where it is known: remquo_c's must be
* its last three bits. The extremes, and C's special values (Annex F), every
* family.
*
*   With the argument "exhaustive" it checks every positive float against
* four divisors: 2 pi as a float, 1, 0.1 and three times the least
* subnormal (its count wants a 64-bit long). Compiled as C++, it checks the
* overloads; from C++14, that fmod_c, remainder_c and remquo_c are constant
* expressions, for every family -- the longest reduction of a long double
* among the calls -- with the bits they have at run time. It needs no 64-bit
* integer -- its random numbers come from math_tests_random.h, the same in
* every build -- so that ISO-strict C++98 on a 32-bit target, where
* dj_math_umax has 32 bits, builds it too.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_remainder.c -o t_remainder -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_remainder.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.27
*******************************************************************************/
// std
#include <float.h>   // FLT_MAX, DBL_MAX, LDBL_MAX, LDBL_MANT_DIG
#include <math.h>    // fmodf ... remquol, ldexpl, nextafter
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/remainder.h"  // unit under test
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
// tests
#include "./math_tests_random.h"  // d_tests_seed, d_tests_below ...


// the C library's functions, called through volatile pointers, so that no
// compiler substitutes its own evaluation, or folds a call into a constant
static float       (*volatile g_fmodf)(float, float)       = fmodf;
static double      (*volatile g_fmod)(double, double)      = fmod;
static long double (*volatile g_fmodl)(long double,
                                       long double)        = fmodl;
static float       (*volatile g_remainderf)(float, float)  = remainderf;
static double      (*volatile g_remainder)(double, double) = remainder;
static long double (*volatile g_remainderl)(long double,
                                            long double)   = remainderl;
static float       (*volatile g_remquof)(float, float, int*)  = remquof;
static double      (*volatile g_remquo)(double, double, int*) = remquo;
static long double (*volatile g_remquol)(long double,
                                         long double,
                                         int*)                = remquol;

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

// a uniform double in [0, 1), from 53 random bits
static
double d_tests_unit(void)
{
    return (double)d_tests_top(53) * DBL_EPSILON / 2.0;
}

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

// D_TESTS_REMAINDER_SAME
//   remainder_c against the library's remainder, bit for bit -- but where
// both are zero and only the library's has the sign that is not x's. IEEE
// 754 gives a zero remainder x's sign. glibc's double remainder does not
// always where |y| is below 2^-970 (2.39: 1,369 of the two million random
// pairs), so there remainder_c must have x's sign, and the library's zero
// is counted, not failed.
static long g_zero_signs[3] = { 0, 0, 0 };

#define D_TESTS_REMAINDER_SAME(T, S, SAME, LIB, SLOT)                         \
    static bool                                                               \
    d_tests_remainder_##S(T _x, T _y)                                         \
    {                                                                         \
        const T ours = d_math_##S##_remainder_c(_x, _y);                      \
        const T lib  = LIB(_x, _y);                                           \
                                                                              \
        if (SAME(ours, lib))                                                  \
        {                                                                     \
            return true;                                                      \
        }                                                                     \
                                                                              \
        if ( (ours == (T)0) &&                                                \
             (lib == (T)0) &&                                                 \
             (signbit(ours) == signbit(_x)) &&                                \
             (signbit(lib) != signbit(_x)) )                                  \
        {                                                                     \
            ++g_zero_signs[SLOT];                                             \
                                                                              \
            return true;                                                      \
        }                                                                     \
                                                                              \
        return false;                                                         \
    }

D_TESTS_REMAINDER_SAME(float, f, d_tests_same_f, g_remainderf, 0)
D_TESTS_REMAINDER_SAME(double, d, d_tests_same_d, g_remainder, 1)
D_TESTS_REMAINDER_SAME(long double, ld, d_tests_same_ld, g_remainderl, 2)

// D_TESTS_REMQUO_SAME
//   remquo_c against remainder_c and the library's remquo. Its remainder
// must be remainder_c's, bit for bit, and the library's, but for a zero the
// library gives with the sign that is not x's (counted, as above). Its
// quotient must be within -7 and 7, have x / y's sign where it is not 0, and
// be congruent mod 8 to the library's, which C lets be any number of last
// bits from 3: glibc's on x86-64 gives 8 or -8 where remquo_c gives 0, and
// those are counted. Where the result is NaN, C leaves the quotient
// unspecified, and remquo_c's must be 0.
static long g_remquo_zero_signs[3] = { 0, 0, 0 };
static long g_remquo_eights[3]     = { 0, 0, 0 };

#define D_TESTS_REMQUO_SAME(T, S, SAME, LIB, SLOT)                            \
    static bool                                                               \
    d_tests_remquo_##S(T _x, T _y)                                            \
    {                                                                         \
        int     ours_quo = 99;                                                \
        int     lib_quo  = 99;                                                \
        const T ours     = d_math_##S##_remquo_c(_x, _y, &ours_quo);          \
        const T lib      = LIB(_x, _y, &lib_quo);                             \
        const T rem      = d_math_##S##_remainder_c(_x, _y);                  \
        bool    same     = SAME(ours, lib);                                   \
                                                                              \
        if ( (!same) &&                                                       \
             (ours == (T)0) &&                                                \
             (lib == (T)0) &&                                                 \
             (signbit(ours) == signbit(_x)) &&                                \
             (signbit(lib) != signbit(_x)) )                                  \
        {                                                                     \
            ++g_remquo_zero_signs[SLOT];                                      \
            same = true;                                                      \
        }                                                                     \
                                                                              \
        if (!SAME(ours, rem))                                                 \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
                                                                              \
        if (isnan(ours))                                                      \
        {                                                                     \
            return (same && (ours_quo == 0));                                 \
        }                                                                     \
                                                                              \
        if ( (lib_quo == 8) ||                                                \
             (lib_quo == -8) )                                                \
        {                                                                     \
            ++g_remquo_eights[SLOT];                                          \
        }                                                                     \
                                                                              \
        return ( same &&                                                      \
                 (ours_quo >= -7) &&                                          \
                 (ours_quo <= 7) &&                                           \
                 (((ours_quo - lib_quo) % 8) == 0) &&                         \
                 ( (ours_quo == 0) ||                                         \
                   ((ours_quo < 0) == ((_x < (T)0) != (_y < (T)0))) ) &&      \
                 ( (ours_quo == 0) ||                                         \
                   (lib_quo == 0) ||                                          \
                   ((ours_quo < 0) == (lib_quo < 0)) ) );                     \
    }

D_TESTS_REMQUO_SAME(float, f, d_tests_same_f, g_remquof, 0)
D_TESTS_REMQUO_SAME(double, d, d_tests_same_d, g_remquo, 1)
D_TESTS_REMQUO_SAME(long double, ld, d_tests_same_ld, g_remquol, 2)

// d_tests_args
//   an argument pair for range _range of a family whose exponents run from
// _emin to _emax with _mant bits: 0, any two values, the subnormals among
// them; 1, |x| up to 2^60 times |y|; 2, |x| and |y| within a factor of 2;
// 3, a subnormal y under a normal x; 4, x in the top binade over y within
// 2^_mant of the least subnormal: the longest reductions; 5, x a whole
// multiple of y, or a last place either side, or an angle and 2 pi. Either
// sign each.
static void
d_tests_args(
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

    switch (_range)
    {
        case 0:
            *_x = ldexpl(u, _emin - _mant +
                            (int)d_tests_below((unsigned long)(span + _mant)));
            *_y = ldexpl(v, _emin - _mant +
                            (int)d_tests_below((unsigned long)(span + _mant)));
            break;

        case 1:
            ex  = _emin + 61 + (int)d_tests_below((unsigned long)(span - 62));
            *_x = ldexpl(u, ex);
            *_y = ldexpl(v, ex - (int)d_tests_below(61u));
            break;

        case 2:
            ex  = _emin + (int)d_tests_below((unsigned long)(span - 1));
            *_x = ldexpl(u, ex);
            *_y = ldexpl(v, ex);
            break;

        case 3:
            *_x = ldexpl(u, _emin + (int)d_tests_below(200u));
            *_y = ldexpl(v, _emin - 2 -
                            (int)d_tests_below((unsigned long)(_mant - 2)));
            break;

        case 4:
            *_x = ldexpl(u, _emax - 1);
            *_y = ldexpl(v, _emin - _mant +
                            (int)d_tests_below((unsigned long)_mant));
            break;

        default:
            if (d_tests_below(2u) == 0u)
            {
                // an angle, to some 2^20 turns, and 2 pi
                *_y = 6.283185307179586476925286766559L;
                *_x = ldexpl(u, (int)d_tests_below(24u) - 2);
            }
            else
            {
                // n y, for n below 2^20, then a last place either side
                *_y = ldexpl(v, (int)d_tests_below(200u) - 100);
                *_x = *_y *
                      (long double)(1u + (unsigned)d_tests_below(1048575u));
            }
            break;
    }

    if (d_tests_below(2u) == 0u)
    {
        *_x = -*_x;
    }

    if (d_tests_below(2u) == 0u)
    {
        *_y = -*_y;
    }

    return;
}

// d_tests_float
//   fmod_c, remainder_c and remquo_c against fmodf, remainderf and remquof,
// bit for bit: two million argument pairs, each rounded to float, and in
// range 5 the neighbours of x too; with _exhaustive, besides, every positive
// float against four divisors.
static
void d_tests_float(
    bool _exhaustive
)
{
    static const float divisors[4] = {
        6.28318548f, 1.0f, 0.100000001f, 4.20389539e-45f
    };
    long     i    = 0;
    uint32_t bits = 0u;
    int      k    = 0;

    for (i = 0; i < 2000000; ++i)
    {
        long double lx = 0.0L;
        long double ly = 0.0L;
        float       x  = 0.0f;
        float       y  = 0.0f;

        d_tests_args((int)(i % 6),
                     FLT_MIN_EXP,
                     FLT_MAX_EXP,
                     FLT_MANT_DIG,
                     &lx,
                     &ly);
        x = (float)lx;
        y = (float)ly;

        D_TESTS_EXPECT(d_tests_same_f(d_math_f_fmod_c(x, y), g_fmodf(x, y)),
                       "f fmod_c is fmodf", x);
        D_TESTS_EXPECT(d_tests_remainder_f(x, y),
                       "f remainder_c is remainderf", x);
        D_TESTS_EXPECT(d_tests_remquo_f(x, y), "f remquo_c is remquof", x);

        if ((i % 6) == 5)
        {
            const float up   = nextafterf(x, FLT_MAX);
            const float down = nextafterf(x, -FLT_MAX);

            D_TESTS_EXPECT( (d_tests_same_f(d_math_f_fmod_c(up, y),
                                            g_fmodf(up, y))) &&
                            (d_tests_same_f(d_math_f_fmod_c(down, y),
                                            g_fmodf(down, y))),
                            "f fmod_c is fmodf a last place either side", x);
            D_TESTS_EXPECT( (d_tests_remainder_f(up, y)) &&
                            (d_tests_remainder_f(down, y)),
                            "f remainder_c is remainderf a last place either "
                            "side", x);
            D_TESTS_EXPECT( (d_tests_remquo_f(up, y)) &&
                            (d_tests_remquo_f(down, y)),
                            "f remquo_c is remquof a last place either side",
                            x);
        }
    }

    for (bits = (_exhaustive ? 1u : 0x80000000u); bits < 0x7F800000u; ++bits)
    {
        const uint32_t b = (uint32_t)bits;
        float          x = 0.0f;

        memcpy(&x, &b, sizeof(x));

        for (k = 0; k < 4; ++k)
        {
            D_TESTS_EXPECT(d_tests_same_f(d_math_f_fmod_c(x, divisors[k]),
                                          g_fmodf(x, divisors[k])),
                           "f fmod_c is fmodf, every float", x);
            D_TESTS_EXPECT(d_tests_remainder_f(x, divisors[k]),
                           "f remainder_c is remainderf, every float", x);
            D_TESTS_EXPECT(d_tests_remquo_f(x, divisors[k]),
                           "f remquo_c is remquof, every float", x);
        }
    }

    return;
}

// d_tests_double
//   fmod_c, remainder_c and remquo_c against fmod, remainder and remquo,
// bit for bit, at two million argument pairs.
static
void d_tests_double(void)
{
    long i = 0;

    for (i = 0; i < 2000000; ++i)
    {
        long double lx = 0.0L;
        long double ly = 0.0L;
        double      x  = 0.0;
        double      y  = 0.0;

        d_tests_args((int)(i % 6),
                     DBL_MIN_EXP,
                     DBL_MAX_EXP,
                     DBL_MANT_DIG,
                     &lx,
                     &ly);
        x = (double)lx;
        y = (double)ly;

        D_TESTS_EXPECT(d_tests_same_d(d_math_d_fmod_c(x, y), g_fmod(x, y)),
                       "d fmod_c is fmod", x);
        D_TESTS_EXPECT(d_tests_remainder_d(x, y),
                       "d remainder_c is remainder", x);
        D_TESTS_EXPECT(d_tests_remquo_d(x, y), "d remquo_c is remquo", x);

        if ((i % 6) == 5)
        {
            const double up   = nextafter(x, DBL_MAX);
            const double down = nextafter(x, -DBL_MAX);

            D_TESTS_EXPECT( (d_tests_same_d(d_math_d_fmod_c(up, y),
                                            g_fmod(up, y))) &&
                            (d_tests_same_d(d_math_d_fmod_c(down, y),
                                            g_fmod(down, y))),
                            "d fmod_c is fmod a last place either side", x);
            D_TESTS_EXPECT( (d_tests_remainder_d(up, y)) &&
                            (d_tests_remainder_d(down, y)),
                            "d remainder_c is remainder a last place either "
                            "side", x);
            D_TESTS_EXPECT( (d_tests_remquo_d(up, y)) &&
                            (d_tests_remquo_d(down, y)),
                            "d remquo_c is remquo a last place either side",
                            x);
        }
    }

    return;
}

// d_tests_long_double
//   fmod_c, remainder_c and remquo_c against fmodl, remainderl and remquol,
// bit for bit, at two hundred thousand argument pairs, in whatever format
// long double has: the longest reductions -- x in the top binade over y
// about the least subnormal, some 33,000 halvings on an x87 -- at one pair
// in sixty, the others in range 1's place.
static
void d_tests_long_double(void)
{
    long i = 0;

    for (i = 0; i < 200000; ++i)
    {
        long double x     = 0.0L;
        long double y     = 0.0L;
        int         range = (int)(i % 6);

        if ( (range == 4) &&
             ((i % 60) != 4) )
        {
            range = 1;
        }

        d_tests_args(range,
                     LDBL_MIN_EXP,
                     LDBL_MAX_EXP,
                     LDBL_MANT_DIG,
                     &x,
                     &y);

        D_TESTS_EXPECT(d_tests_same_ld(d_math_ld_fmod_c(x, y),
                                       g_fmodl(x, y)),
                       "ld fmod_c is fmodl", x);
        D_TESTS_EXPECT(d_tests_remainder_ld(x, y),
                       "ld remainder_c is remainderl", x);
        D_TESTS_EXPECT(d_tests_remquo_ld(x, y), "ld remquo_c is remquol", x);
    }

    return;
}

// d_tests_ties
//   remainder_c and remquo_c against the library's where the quotient is a
// whole number and a half -- x = (k + 1/2) y, exactly, both signs -- so that
// the result is y / 2 or -y / 2 by the quotient's parity, and remquo_c's
// quotient the last three bits of the even one of k and k + 1: a hundred
// thousand ties a family, y's mantissa short enough that (2k + 1) y fits the
// family.
static
void d_tests_ties(void)
{
    long i = 0;

    for (i = 0; i < 100000; ++i)
    {
        const unsigned    k  = (unsigned)d_tests_below(4096u);
        const unsigned    kd = (unsigned)d_tests_below(1048576u);
        const int         n  = (int)((k + (k % 2u)) % 8u);
        const int         nd = (int)((kd + (kd % 2u)) % 8u);
        const long double m  = 1.0L + ((long double)d_tests_below(1024u) /
                                       1024.0L);
        const long double md = 1.0L + ((long double)d_tests_below(1048576u) /
                                       1048576.0L);
        const float       fy = (float)ldexpl(m, (int)d_tests_below(120u) - 60);
        const double      dy = (double)ldexpl(md,
                                              (int)d_tests_below(1000u) - 500);
        const long double ly = ldexpl(md, (int)d_tests_below(1000u) - 500);
        const float       fx = ((float)(2u * k + 1u) * fy) / 2.0f;
        const double      dx = ((double)(2u * kd + 1u) * dy) / 2.0;
        const long double lx = ((long double)(2u * kd + 1u) * ly) / 2.0L;
        int               fq = 99;
        int               dq = 99;
        int               lq = 99;

        D_TESTS_EXPECT( (d_tests_remainder_f(fx, fy)) &&
                        (d_tests_remainder_f(-fx, fy)),
                        "f remainder_c is remainderf at a tie", fx);
        D_TESTS_EXPECT( (d_tests_remainder_d(dx, dy)) &&
                        (d_tests_remainder_d(-dx, -dy)),
                        "d remainder_c is remainder at a tie", dx);
        D_TESTS_EXPECT( (d_tests_remainder_ld(lx, ly)) &&
                        (d_tests_remainder_ld(-lx, ly)),
                        "ld remainder_c is remainderl at a tie", lx);
        D_TESTS_EXPECT( (d_tests_remquo_f(fx, fy)) &&
                        (d_tests_remquo_f(-fx, fy)) &&
                        (d_tests_remquo_d(dx, dy)) &&
                        (d_tests_remquo_d(-dx, -dy)) &&
                        (d_tests_remquo_ld(lx, ly)) &&
                        (d_tests_remquo_ld(-lx, ly)),
                        "remquo_c is the library's at a tie", fx);

        (void)d_math_f_remquo_c(-fx, fy, &fq);
        (void)d_math_d_remquo_c(-dx, -dy, &dq);
        (void)d_math_ld_remquo_c(-lx, ly, &lq);

        D_TESTS_EXPECT( (fq == -n) &&
                        (dq == nd) &&
                        (lq == -nd),
                        "remquo_c's quotient at a tie is the even one's", fx);
    }

    return;
}

// d_tests_multiples
//   remquo_c where the quotient is known: x = n y exactly, n a whole number
// from 1, either sign each -- a zero with x's sign, and n's last three bits
// with the sign of x / y -- and against the library's: a hundred thousand a
// family, y the subnormals among it, its mantissa short enough that n y
// fits the family.
static
void d_tests_multiples(void)
{
    long i = 0;

    for (i = 0; i < 100000; ++i)
    {
        const unsigned    n    = 1u + (unsigned)d_tests_below(4095u);
        const unsigned    nd   = 1u + (unsigned)d_tests_below(1048575u);
        const long double m    = 1.0L + ((long double)d_tests_below(1024u) /
                                         1024.0L);
        const long double md   = 1.0L + ((long double)d_tests_below(1048576u) /
                                         1048576.0L);
        const bool        negx = (d_tests_below(2u) == 0u);
        const bool        negy = (d_tests_below(2u) == 0u);
        const int         sign = (negx != negy) ? -1 : 1;
        const float       fy   = (float)ldexpl(m,
                                               (int)d_tests_below(200u) - 140);
        const double      dy   = (double)ldexpl(md,
                                                (int)d_tests_below(2000u) -
                                                1060);
        const long double ly   = ldexpl(md, (int)d_tests_below(2000u) - 1060);
        const float       fx   = (float)n * fy;
        const double      dx   = (double)nd * dy;
        const long double lx   = (long double)nd * ly;
        const float       fxs  = negx ? -fx : fx;
        const float       fys  = negy ? -fy : fy;
        const double      dxs  = negx ? -dx : dx;
        const double      dys  = negy ? -dy : dy;
        const long double lxs  = negx ? -lx : lx;
        const long double lys  = negy ? -ly : ly;
        int               fq   = 99;
        int               dq   = 99;
        int               lq   = 99;
        const float       fr   = d_math_f_remquo_c(fxs, fys, &fq);
        const double      dr   = d_math_d_remquo_c(dxs, dys, &dq);
        const long double lr   = d_math_ld_remquo_c(lxs, lys, &lq);

        D_TESTS_EXPECT( (d_tests_same_f(fr, negx ? -0.0f : 0.0f)) &&
                        (fq == (sign * (int)(n % 8u))) &&
                        (d_tests_same_d(dr, negx ? -0.0 : 0.0)) &&
                        (dq == (sign * (int)(nd % 8u))) &&
                        (d_tests_same_ld(lr, negx ? -0.0L : 0.0L)) &&
                        (lq == (sign * (int)(nd % 8u))),
                        "remquo_c at a whole multiple: a zero with x's sign "
                        "and the quotient's last three bits", fx);
        D_TESTS_EXPECT( (d_tests_remquo_f(fxs, fys)) &&
                        (d_tests_remquo_d(dxs, dys)) &&
                        (d_tests_remquo_ld(lxs, lys)),
                        "remquo_c is the library's at a whole multiple", fx);
    }

    return;
}

// D_TESTS_FMOD_SPECIALS
//   Annex F's special values for one family, and the extremes: x a zero
// keeps its sign; y a zero, or x infinite, gives NaN; y infinite gives x;
// NaN gives NaN; the result has x's sign, a zero's included; the largest x
// over the least y, the longest reduction; the least values.
#define D_TESTS_FMOD_SPECIALS(T, S, SAME, LIB, MAXV, MINV, LEAST)             \
    {                                                                         \
        const T inf  = (T)HUGE_VAL;                                           \
        const T qnan = (T)nan("");                                            \
                                                                              \
        D_TESTS_EXPECT( SAME(d_math_##S##_fmod_c((T)0, (T)3), (T)0) &&        \
                        SAME(d_math_##S##_fmod_c(-(T)0, (T)3), -(T)0) &&      \
                        SAME(d_math_##S##_fmod_c(-(T)0, inf), -(T)0) &&       \
                        SAME(d_math_##S##_fmod_c((T)5, inf), (T)5) &&         \
                        SAME(d_math_##S##_fmod_c(-(T)5, -inf), -(T)5) &&      \
                        SAME(d_math_##S##_fmod_c(-MAXV, inf), -MAXV),         \
                        #S " fmod_c: a zero x, an infinite y", 0);            \
        D_TESTS_EXPECT( isnan(d_math_##S##_fmod_c((T)5, (T)0)) &&             \
                        isnan(d_math_##S##_fmod_c((T)5, -(T)0)) &&            \
                        isnan(d_math_##S##_fmod_c(inf, (T)3)) &&              \
                        isnan(d_math_##S##_fmod_c(-inf, inf)) &&              \
                        isnan(d_math_##S##_fmod_c((T)0, (T)0)) &&             \
                        isnan(d_math_##S##_fmod_c(qnan, (T)3)) &&             \
                        isnan(d_math_##S##_fmod_c((T)3, qnan)),               \
                        #S " fmod_c: NaN", 0);                                \
        D_TESTS_EXPECT( SAME(d_math_##S##_fmod_c((T)7, (T)3), (T)1) &&        \
                        SAME(d_math_##S##_fmod_c(-(T)7, (T)3), -(T)1) &&      \
                        SAME(d_math_##S##_fmod_c((T)7, -(T)3), (T)1) &&       \
                        SAME(d_math_##S##_fmod_c(-(T)6, (T)3), -(T)0) &&      \
                        SAME(d_math_##S##_fmod_c((T)6, -(T)3), (T)0) &&       \
                        SAME(d_math_##S##_fmod_c((T)5.5, (T)2), (T)1.5) &&    \
                        SAME(d_math_##S##_fmod_c((T)2, (T)5.5), (T)2),        \
                        #S " fmod_c: small", 0);                              \
        D_TESTS_EXPECT( SAME(d_math_##S##_fmod_c(MAXV, LEAST),                \
                             LIB(MAXV, LEAST)) &&                             \
                        SAME(d_math_##S##_fmod_c(-MAXV, LEAST * (T)3),        \
                             LIB(-MAXV, LEAST * (T)3)) &&                     \
                        SAME(d_math_##S##_fmod_c(MAXV, MINV),                 \
                             LIB(MAXV, MINV)) &&                              \
                        SAME(d_math_##S##_fmod_c(MAXV, (T)3),                 \
                             LIB(MAXV, (T)3)) &&                              \
                        SAME(d_math_##S##_fmod_c(LEAST * (T)3, LEAST * (T)2), \
                             LEAST) &&                                        \
                        SAME(d_math_##S##_fmod_c(MINV, LEAST * (T)3),         \
                             LIB(MINV, LEAST * (T)3)) &&                      \
                        SAME(d_math_##S##_fmod_c(LEAST, MAXV), LEAST) &&      \
                        SAME(d_math_##S##_fmod_c(MAXV, MAXV), (T)0),          \
                        #S " fmod_c: the extremes", 0);                       \
    }

// D_TESTS_REMAINDER_SPECIALS
//   Annex F's special values for remainder in one family, and the ties and
// extremes: x a zero keeps its sign, and so does a zero result; y a zero,
// or x infinite, gives NaN; y infinite gives x; NaN gives NaN; a quotient
// a whole number and a half goes to the even one; the largest x over the
// least y and over itself; the least values, where y / 2 is not exact.
#define D_TESTS_REMAINDER_SPECIALS(T, S, SAME, MAXV, MINV, LEAST)             \
    {                                                                         \
        const T inf  = (T)HUGE_VAL;                                           \
        const T qnan = (T)nan("");                                            \
                                                                              \
        D_TESTS_EXPECT( SAME(d_math_##S##_remainder_c((T)0, (T)3), (T)0) &&   \
                        SAME(d_math_##S##_remainder_c(-(T)0, (T)3), -(T)0) && \
                        SAME(d_math_##S##_remainder_c(-(T)0, inf), -(T)0) &&  \
                        SAME(d_math_##S##_remainder_c((T)5, inf), (T)5) &&    \
                        SAME(d_math_##S##_remainder_c(-(T)5, -inf),           \
                             -(T)5) &&                                        \
                        SAME(d_math_##S##_remainder_c(-MAXV, inf), -MAXV),    \
                        #S " remainder_c: a zero x, an infinite y", 0);       \
        D_TESTS_EXPECT( isnan(d_math_##S##_remainder_c((T)5, (T)0)) &&        \
                        isnan(d_math_##S##_remainder_c((T)5, -(T)0)) &&       \
                        isnan(d_math_##S##_remainder_c(inf, (T)3)) &&         \
                        isnan(d_math_##S##_remainder_c(-inf, inf)) &&         \
                        isnan(d_math_##S##_remainder_c((T)0, (T)0)) &&        \
                        isnan(d_math_##S##_remainder_c(qnan, (T)3)) &&        \
                        isnan(d_math_##S##_remainder_c((T)3, qnan)),          \
                        #S " remainder_c: NaN", 0);                           \
        D_TESTS_EXPECT( SAME(d_math_##S##_remainder_c((T)5, (T)2), (T)1) &&   \
                        SAME(d_math_##S##_remainder_c((T)7, (T)2), -(T)1) &&  \
                        SAME(d_math_##S##_remainder_c(-(T)5, (T)2), -(T)1) && \
                        SAME(d_math_##S##_remainder_c((T)3, (T)2), -(T)1) &&  \
                        SAME(d_math_##S##_remainder_c((T)1, -(T)2), (T)1) &&  \
                        SAME(d_math_##S##_remainder_c((T)2, (T)2), (T)0) &&   \
                        SAME(d_math_##S##_remainder_c(-(T)2, (T)2), -(T)0) && \
                        SAME(d_math_##S##_remainder_c((T)6, (T)4), -(T)2) &&  \
                        SAME(d_math_##S##_remainder_c((T)10, (T)4), (T)2) &&  \
                        SAME(d_math_##S##_remainder_c((T)7, (T)3), (T)1) &&   \
                        SAME(d_math_##S##_remainder_c((T)8, (T)3), -(T)1),    \
                        #S " remainder_c: small, the ties to even", 0);       \
        D_TESTS_EXPECT( d_tests_remainder_##S(MAXV, LEAST) &&                 \
                        d_tests_remainder_##S(-MAXV, LEAST * (T)3) &&         \
                        d_tests_remainder_##S(MAXV, MINV) &&                  \
                        d_tests_remainder_##S(MAXV, MAXV * (T)0.75) &&        \
                        SAME(d_math_##S##_remainder_c(MAXV, MAXV), (T)0) &&   \
                        SAME(d_math_##S##_remainder_c(-MAXV, LEAST),          \
                             -(T)0) &&                                        \
                        SAME(d_math_##S##_remainder_c(LEAST * (T)3,           \
                                                      LEAST * (T)2),          \
                             -LEAST) &&                                       \
                        SAME(d_math_##S##_remainder_c(LEAST * (T)5,           \
                                                      LEAST * (T)2),          \
                             LEAST) &&                                        \
                        SAME(d_math_##S##_remainder_c(MINV * (T)3,            \
                                                      MINV * (T)2), -MINV) && \
                        SAME(d_math_##S##_remainder_c(LEAST, MAXV), LEAST),   \
                        #S " remainder_c: the extremes", 0);                  \
    }

// D_TESTS_REMQUO_IS
//   remquo_c at (_x, _y) gives _r, bit for bit (NaN for NaN), and stores the
// quotient _q.
#define D_TESTS_REMQUO_IS(T, S, SAME)                                         \
    static bool                                                               \
    d_tests_remquo_is_##S(T _x, T _y, T _r, int _q)                           \
    {                                                                         \
        int     quo    = 99;                                                  \
        const T result = d_math_##S##_remquo_c(_x, _y, &quo);                 \
                                                                              \
        return ( (SAME(result, _r)) &&                                        \
                 (quo == _q) );                                               \
    }

D_TESTS_REMQUO_IS(float, f, d_tests_same_f)
D_TESTS_REMQUO_IS(double, d, d_tests_same_d)
D_TESTS_REMQUO_IS(long double, ld, d_tests_same_ld)

// D_TESTS_REMQUO_SPECIALS
//   Annex F's special values for remquo in one family, the quotient with
// each -- 0 where the result is NaN, or y infinite -- and the ties, the
// last three bits and the extremes: a quotient 8 gives 0; the largest x
// over the least y, over itself, and over y where 8y, 4y or 2y would
// overflow, against the library's; the least values, where y / 2 is not
// exact.
#define D_TESTS_REMQUO_SPECIALS(T, S, MAXV, MINV, LEAST)                      \
    {                                                                         \
        const T inf  = (T)HUGE_VAL;                                           \
        const T qnan = (T)nan("");                                            \
                                                                              \
        D_TESTS_EXPECT( d_tests_remquo_is_##S((T)0, (T)3, (T)0, 0) &&         \
                        d_tests_remquo_is_##S(-(T)0, (T)3, -(T)0, 0) &&       \
                        d_tests_remquo_is_##S(-(T)0, inf, -(T)0, 0) &&        \
                        d_tests_remquo_is_##S((T)5, inf, (T)5, 0) &&          \
                        d_tests_remquo_is_##S(-(T)5, -inf, -(T)5, 0) &&       \
                        d_tests_remquo_is_##S(-MAXV, inf, -MAXV, 0),          \
                        #S " remquo_c: a zero x, an infinite y", 0);          \
        D_TESTS_EXPECT( d_tests_remquo_is_##S((T)5, (T)0, qnan, 0) &&         \
                        d_tests_remquo_is_##S((T)5, -(T)0, qnan, 0) &&        \
                        d_tests_remquo_is_##S(inf, (T)3, qnan, 0) &&          \
                        d_tests_remquo_is_##S(-inf, inf, qnan, 0) &&          \
                        d_tests_remquo_is_##S((T)0, (T)0, qnan, 0) &&         \
                        d_tests_remquo_is_##S(qnan, (T)3, qnan, 0) &&         \
                        d_tests_remquo_is_##S((T)3, qnan, qnan, 0),           \
                        #S " remquo_c: NaN, the quotient 0", 0);              \
        D_TESTS_EXPECT( d_tests_remquo_is_##S((T)5, (T)2, (T)1, 2) &&         \
                        d_tests_remquo_is_##S((T)7, (T)2, -(T)1, 4) &&        \
                        d_tests_remquo_is_##S(-(T)7, (T)2, (T)1, -4) &&       \
                        d_tests_remquo_is_##S((T)3, (T)2, -(T)1, 2) &&        \
                        d_tests_remquo_is_##S((T)1, -(T)2, (T)1, 0) &&        \
                        d_tests_remquo_is_##S((T)2, (T)2, (T)0, 1) &&         \
                        d_tests_remquo_is_##S(-(T)2, (T)2, -(T)0, -1) &&      \
                        d_tests_remquo_is_##S((T)6, (T)4, -(T)2, 2) &&        \
                        d_tests_remquo_is_##S((T)10, -(T)4, (T)2, -2) &&      \
                        d_tests_remquo_is_##S(-(T)13, (T)2, -(T)1, -6) &&     \
                        d_tests_remquo_is_##S((T)63, -(T)9, (T)0, -7) &&      \
                        d_tests_remquo_is_##S((T)9, (T)1, (T)0, 1),           \
                        #S " remquo_c: small, the ties to even", 0);          \
        D_TESTS_EXPECT( d_tests_remquo_is_##S((T)15, (T)2, -(T)1, 0) &&       \
                        d_tests_remquo_is_##S((T)17, (T)2, (T)1, 0) &&        \
                        d_tests_remquo_is_##S((T)23, (T)2, -(T)1, 4) &&       \
                        d_tests_remquo_is_##S(-(T)64, (T)8, -(T)0, 0) &&      \
                        d_tests_remquo_is_##S((T)7.5, -(T)1, -(T)0.5, 0),     \
                        #S " remquo_c: the last three bits, 8 as 0", 0);      \
        D_TESTS_EXPECT( d_tests_remquo_##S(MAXV, LEAST) &&                    \
                        d_tests_remquo_##S(-MAXV, LEAST * (T)3) &&            \
                        d_tests_remquo_##S(MAXV, MINV) &&                     \
                        d_tests_remquo_##S(MAXV, MAXV * (T)0.75) &&           \
                        d_tests_remquo_##S(MAXV, MAXV / (T)3) &&              \
                        d_tests_remquo_##S(MAXV, MAXV / (T)5) &&              \
                        d_tests_remquo_##S(-MAXV, MAXV / (T)7) &&             \
                        d_tests_remquo_##S(MAXV, -MAXV / (T)9) &&             \
                        d_tests_remquo_is_##S(MAXV, MAXV, (T)0, 1) &&         \
                        d_tests_remquo_is_##S(-MAXV, LEAST, -(T)0, 0) &&      \
                        d_tests_remquo_is_##S(LEAST * (T)3, LEAST * (T)2,     \
                                              -LEAST, 2) &&                   \
                        d_tests_remquo_is_##S(LEAST * (T)5, LEAST * (T)2,     \
                                              LEAST, 2) &&                    \
                        d_tests_remquo_is_##S(MINV * (T)3, MINV * (T)2,       \
                                              -MINV, 2) &&                    \
                        d_tests_remquo_is_##S(LEAST, MAXV, LEAST, 0),         \
                        #S " remquo_c: the extremes", 0);                     \
    }

// d_tests_specials
//   every family's special values and extremes; the plain forms, the
// library's at every level -- before C++11, remainder_c and remquo_c, which
// agree -- and a null _quo, which stores nothing.
static
void d_tests_specials(void)
{
    volatile double ten     = 10.0;
    volatile double third   = 0.3;
    volatile float  two     = 2.0f;
    int             q_ours  = 99;
    int             q_lib   = 99;
    int             qf_ours = 99;
    int             qf_lib  = 99;
    int             ql_ours = 99;
    int             ql_lib  = 99;

    D_TESTS_FMOD_SPECIALS(float, f, d_tests_same_f, g_fmodf, FLT_MAX,
                          FLT_MIN, FLT_MIN * FLT_EPSILON)
    D_TESTS_FMOD_SPECIALS(double, d, d_tests_same_d, g_fmod, DBL_MAX,
                          DBL_MIN, DBL_MIN * DBL_EPSILON)
    D_TESTS_FMOD_SPECIALS(long double, ld, d_tests_same_ld, g_fmodl, LDBL_MAX,
                          LDBL_MIN, LDBL_MIN * LDBL_EPSILON)
    D_TESTS_REMAINDER_SPECIALS(float, f, d_tests_same_f, FLT_MAX, FLT_MIN,
                               FLT_MIN * FLT_EPSILON)
    D_TESTS_REMAINDER_SPECIALS(double, d, d_tests_same_d, DBL_MAX, DBL_MIN,
                               DBL_MIN * DBL_EPSILON)
    D_TESTS_REMAINDER_SPECIALS(long double, ld, d_tests_same_ld, LDBL_MAX,
                               LDBL_MIN, LDBL_MIN * LDBL_EPSILON)
    D_TESTS_REMQUO_SPECIALS(float, f, FLT_MAX, FLT_MIN, FLT_MIN * FLT_EPSILON)
    D_TESTS_REMQUO_SPECIALS(double, d, DBL_MAX, DBL_MIN, DBL_MIN * DBL_EPSILON)
    D_TESTS_REMQUO_SPECIALS(long double, ld, LDBL_MAX, LDBL_MIN,
                            LDBL_MIN * LDBL_EPSILON)

    // the plain form is the C library's (std::fmod in C++, which C++98
    // has), read through volatiles so that no compiler folds the call
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_fmod(ten, third),
                                    g_fmod(ten, third))) &&
                    (d_tests_same_f(d_math_f_fmod(two, (float)third),
                                    g_fmodf(two, (float)third))),
                    "fmod is the C library's", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_remainder(ten, third),
                                    g_remainder(ten, third))) &&
                    (d_tests_same_f(d_math_f_remainder(two, (float)third),
                                    g_remainderf(two, (float)third))) &&
                    (d_tests_same_ld(d_math_ld_remainder((long double)ten,
                                                         (long double)third),
                                     g_remainderl((long double)ten,
                                                  (long double)third))),
                    "remainder is the C library's", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_remquo(ten, third, &q_ours),
                                    g_remquo(ten, third, &q_lib))) &&
                    (((q_ours - q_lib) % 8) == 0) &&
                    (d_tests_same_f(d_math_f_remquo(two, (float)third,
                                                    &qf_ours),
                                    g_remquof(two, (float)third, &qf_lib))) &&
                    (((qf_ours - qf_lib) % 8) == 0) &&
                    (d_tests_same_ld(d_math_ld_remquo((long double)ten,
                                                      (long double)third,
                                                      &ql_ours),
                                     g_remquol((long double)ten,
                                               (long double)third,
                                               &ql_lib))) &&
                    (((ql_ours - ql_lib) % 8) == 0),
                    "remquo is the C library's", 0);
    D_TESTS_EXPECT( (d_tests_same_d(d_math_d_remquo(ten, third, NULL),
                                    d_math_d_remainder_c(ten, third))) &&
                    (d_tests_same_d(d_math_d_remquo_c(ten, third, NULL),
                                    d_math_d_remainder_c(ten, third))) &&
                    (d_tests_same_f(d_math_f_remquo_c(two, (float)third,
                                                      NULL),
                                    d_math_f_remainder_c(two,
                                                         (float)third))),
                    "remquo and remquo_c with a null _quo: the remainder", 0);

    return;
}

#if defined(__cplusplus)

// d_tests_overloads
//   compiled as C++: one name for every family, chosen by argument type.
static
void d_tests_overloads(void)
{
    int q1 = 99;
    int q2 = 99;
    int q3 = 99;
    int q4 = 99;
    int q5 = 99;
    int q6 = 99;
    int q7 = 99;
    int q8 = 99;

    D_TESTS_EXPECT( (d_tests_same_f(d_math_fmod_c(7.5f, 2.0f),
                                    d_math_f_fmod_c(7.5f, 2.0f))) &&
                    (d_tests_same_d(d_math_fmod_c(7.5, 2.0),
                                    d_math_d_fmod_c(7.5, 2.0))) &&
                    (d_tests_same_ld(d_math_fmod_c(7.5L, 2.0L),
                                     d_math_ld_fmod_c(7.5L, 2.0L))) &&
                    (d_tests_same_d(d_math_fmod(7.5, 2.0),
                                    d_math_d_fmod(7.5, 2.0))) &&
                    (d_tests_same_ld(d_math_fmod(7.5L, 2.0L),
                                     d_math_ld_fmod(7.5L, 2.0L))) &&
                    (d_tests_same_f(d_math_remainder_c(7.5f, 2.0f),
                                    d_math_f_remainder_c(7.5f, 2.0f))) &&
                    (d_tests_same_d(d_math_remainder_c(7.5, 2.0),
                                    d_math_d_remainder_c(7.5, 2.0))) &&
                    (d_tests_same_ld(d_math_remainder_c(7.5L, 2.0L),
                                     d_math_ld_remainder_c(7.5L, 2.0L))) &&
                    (d_tests_same_d(d_math_remainder(7.5, 2.0),
                                    d_math_d_remainder(7.5, 2.0))),
                    "the C++ overloads are the families' functions", 0);
    D_TESTS_EXPECT( (d_tests_same_f(d_math_remquo_c(7.5f, 2.0f, &q1),
                                    d_math_f_remquo_c(7.5f, 2.0f, &q2))) &&
                    (q1 == q2) &&
                    (d_tests_same_d(d_math_remquo_c(-7.5, 2.0, &q3),
                                    d_math_d_remquo_c(-7.5, 2.0, &q4))) &&
                    (q3 == q4) &&
                    (d_tests_same_ld(d_math_remquo_c(7.5L, -2.0L, &q5),
                                     d_math_ld_remquo_c(7.5L, -2.0L, &q6))) &&
                    (q5 == q6) &&
                    (d_tests_same_d(d_math_remquo(23.0, 2.0, &q7),
                                    d_math_d_remquo(23.0, 2.0, &q8))) &&
                    (q7 == q8),
                    "the C++ overloads of remquo are the families'", 0);

    return;
}

#endif  // __cplusplus

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// from C++14, fmod_c is a constant expression: every family, an angle, the
// zeros, and the longest reduction a long double has
static constexpr double      g_cfmod_d     = d_math_d_fmod_c(100.0,
                                                             6.283185307179586);
static constexpr float       g_cfmod_f     = d_math_f_fmod_c(1e30f, 3.0f);
static constexpr long double g_cfmod_ld    = d_math_ld_fmod_c(1e300L, 7.0L);
static constexpr double      g_cfmod_neg   = d_math_d_fmod_c(-6.0, 3.0);
static constexpr double      g_cfmod_sub   =
    d_math_d_fmod_c(1.7976931348623157e308, 4.9406564584124654e-324 * 3.0);
static constexpr long double g_cfmod_ldtop =
    d_math_ld_fmod_c(LDBL_MAX, LDBL_MIN * LDBL_EPSILON * 3.0L);

static_assert(d_math_d_fmod_c(7.0, 3.0) == 1.0, "fmod_c(7, 3) is 1");
static_assert(d_math_d_fmod_c(-7.0, 3.0) == -1.0, "fmod_c(-7, 3) is -1");
static_assert(d_math_d_fmod_c(5.5, 2.0) == 1.5, "fmod_c(5.5, 2) is 1.5");
static_assert(d_math_d_fmod_c(1e300, 1e-300) == 4.891554850853602e-301,
              "fmod_c(1e300, 1e-300), exactly");
static_assert(d_math_fmod_c(9.0f, 4.0f) == 1.0f, "the C++ overloads");
static_assert(d_math_d_fmod_c((double)NAN, 3.0) !=
              d_math_d_fmod_c((double)NAN, 3.0),
              "fmod_c(NaN, 3) is NaN, as a constant");
static_assert(d_math_d_fmod_c(3.0, (double)NAN) !=
              d_math_d_fmod_c(3.0, (double)NAN),
              "fmod_c(3, NaN) is NaN, as a constant");
static_assert(d_math_d_fmod_c(-5.0, (double)HUGE_VAL) == -5.0,
              "fmod_c(-5, inf) is -5, as a constant");

// and remainder_c: the ties to even, a zero's sign, the longest reduction
static constexpr double      g_crem_d     = d_math_d_remainder_c(7.0, 2.0);
static constexpr double      g_crem_zero  = d_math_d_remainder_c(-2.0, 2.0);
static constexpr float       g_crem_f     = d_math_f_remainder_c(1e30f, 3.0f);
static constexpr long double g_crem_ldtop =
    d_math_ld_remainder_c(LDBL_MAX, LDBL_MIN * LDBL_EPSILON * 3.0L);

static_assert(d_math_d_remainder_c(5.0, 2.0) == 1.0,
              "remainder_c(5, 2) is 1: 2.5 goes to 2");
static_assert(d_math_d_remainder_c(7.0, 2.0) == -1.0,
              "remainder_c(7, 2) is -1: 3.5 goes to 4");
static_assert(d_math_d_remainder_c(6.0, 4.0) == -2.0,
              "remainder_c(6, 4) is -2: 1.5 goes to 2");
static_assert(d_math_d_remainder_c(100.0, 6.283185307179586) ==
              d_math_d_fmod_c(100.0, 6.283185307179586) -
              6.283185307179586,
              "remainder_c(100, 2 pi), a step past fmod_c's");
static_assert(d_math_remainder_c(9.0f, 4.0f) == 1.0f, "the C++ overloads");
static_assert(d_math_d_remainder_c((double)NAN, 3.0) !=
              d_math_d_remainder_c((double)NAN, 3.0),
              "remainder_c(NaN, 3) is NaN, as a constant");
static_assert(d_math_d_remainder_c(-5.0, (double)HUGE_VAL) == -5.0,
              "remainder_c(-5, inf) is -5, as a constant");

// and remquo_c, whose quotient a constant expression reads through an int of
// its own: the ties to even, the last three bits, a null _quo, the longest
// reduction
#define D_TESTS_CQUO(T, S)                                                    \
    static constexpr int                                                      \
    d_tests_cquo_##S(T _x, T _y)                                              \
    {                                                                         \
        int quo = 99;                                                         \
                                                                              \
        (void)d_math_##S##_remquo_c(_x, _y, &quo);                            \
                                                                              \
        return quo;                                                           \
    }

D_TESTS_CQUO(float, f)
D_TESTS_CQUO(double, d)
D_TESTS_CQUO(long double, ld)

static constexpr int         g_cquo_d     = d_tests_cquo_d(7.0, 2.0);
static constexpr int         g_cquo_neg   = d_tests_cquo_d(-13.0, 2.0);
static constexpr int         g_cquo_f     = d_tests_cquo_f(1e30f, 3.0f);
static constexpr int         g_cquo_ldtop =
    d_tests_cquo_ld(LDBL_MAX, LDBL_MIN * LDBL_EPSILON * 3.0L);
static constexpr long double g_crq_ldtop  =
    d_math_ld_remquo_c(LDBL_MAX, LDBL_MIN * LDBL_EPSILON * 3.0L, NULL);

static_assert(d_tests_cquo_d(7.0, 2.0) == 4,
              "remquo_c(7, 2): 3.5 goes to 4");
static_assert(d_math_d_remquo_c(7.0, 2.0, NULL) == -1.0,
              "remquo_c(7, 2) is -1, with a null _quo");
static_assert(d_tests_cquo_d(-13.0, 2.0) == -6,
              "remquo_c(-13, 2): -6.5 goes to -6");
static_assert(d_tests_cquo_d(15.0, 2.0) == 0,
              "remquo_c(15, 2): 7.5 goes to 8, whose last three bits are 0");
static_assert(d_tests_cquo_d(63.0, -9.0) == -7,
              "remquo_c(63, -9): -7");
static_assert(d_tests_cquo_d(-5.0, (double)HUGE_VAL) == 0,
              "remquo_c(-5, inf): the quotient 0");
static_assert(d_tests_cquo_d((double)NAN, 3.0) == 0,
              "remquo_c(NaN, 3): the quotient 0");
static_assert(d_math_remquo_c(9.0f, 4.0f, NULL) == 1.0f, "the C++ overloads");

// d_tests_constant_evaluation
//   the constants are the run time's results, bit for bit: the arguments
// are read through volatiles, so the calls below are not folded.
static
void d_tests_constant_evaluation(void)
{
    volatile double      hundred  = 100.0;
    volatile float       big      = 1e30f;
    volatile long double huge     = 1e300L;
    volatile double      six      = -6.0;
    volatile double      top      = 1.7976931348623157e308;
    volatile long double ldtop    = LDBL_MAX;
    volatile double      seven    = 7.0;
    volatile double      two_d    = 2.0;
    volatile double      thirteen = 13.0;

    D_TESTS_EXPECT( (d_tests_same_d(g_cfmod_d,
                                    d_math_d_fmod_c(hundred,
                                                    6.283185307179586))) &&
                    (d_tests_same_f(g_cfmod_f, d_math_f_fmod_c(big, 3.0f))) &&
                    (d_tests_same_ld(g_cfmod_ld,
                                     d_math_ld_fmod_c(huge, 7.0L))) &&
                    (d_tests_same_d(g_cfmod_neg, d_math_d_fmod_c(six, 3.0))) &&
                    (d_tests_same_d(g_cfmod_sub,
                                    d_math_d_fmod_c(top,
                                                    4.9406564584124654e-324 *
                                                    3.0))) &&
                    (d_tests_same_ld(g_cfmod_ldtop,
                                     d_math_ld_fmod_c(ldtop,
                                                      LDBL_MIN *
                                                      LDBL_EPSILON * 3.0L))),
                    "constant evaluation agrees with the run time", 0);
    D_TESTS_EXPECT( (d_tests_same_d(g_crem_d,
                                    d_math_d_remainder_c(seven, 2.0))) &&
                    (d_tests_same_d(g_crem_zero,
                                    d_math_d_remainder_c(-two_d, 2.0))) &&
                    (d_tests_same_f(g_crem_f,
                                    d_math_f_remainder_c(big, 3.0f))) &&
                    (d_tests_same_ld(g_crem_ldtop,
                                     d_math_ld_remainder_c(ldtop,
                                                           LDBL_MIN *
                                                           LDBL_EPSILON *
                                                           3.0L))),
                    "remainder_c's constants agree with the run time", 0);
    D_TESTS_EXPECT( (g_cquo_d == d_tests_cquo_d(seven, 2.0)) &&
                    (g_cquo_neg == d_tests_cquo_d(-thirteen, 2.0)) &&
                    (g_cquo_f == d_tests_cquo_f(big, 3.0f)) &&
                    (g_cquo_ldtop == d_tests_cquo_ld(ldtop,
                                                     LDBL_MIN *
                                                     LDBL_EPSILON * 3.0L)) &&
                    (d_tests_same_ld(g_crq_ldtop,
                                     d_math_ld_remquo_c(ldtop,
                                                        LDBL_MIN *
                                                        LDBL_EPSILON * 3.0L,
                                                        NULL))),
                    "remquo_c's constants agree with the run time", 0);

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

    // the generator's start, the word 0x9E3779B97F4A7C15
    d_tests_seed(0x9E3779B9u, 0x7F4A7C15u);
    d_tests_float(exhaustive);
    d_tests_double();
    d_tests_long_double();
    d_tests_ties();
    d_tests_multiples();
    d_tests_specials();
#if defined(__cplusplus)
    d_tests_overloads();
#endif
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    // the library's zeros with the sign that is not x's, which remainder_c
    // does not copy (D_TESTS_REMAINDER_SAME)
    printf("  remainder: the library's zero with the other sign at %ld "
           "float, %ld double, %ld long double results\n",
           g_zero_signs[0], g_zero_signs[1], g_zero_signs[2]);
    printf("  remquo: the library's zero with the other sign at %ld, %ld "
           "and %ld; its quotient 8 or -8, remquo_c's 0, at %ld, %ld and "
           "%ld\n",
           g_remquo_zero_signs[0], g_remquo_zero_signs[1],
           g_remquo_zero_signs[2], g_remquo_eights[0], g_remquo_eights[1],
           g_remquo_eights[2]);
    printf("t_remainder: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
