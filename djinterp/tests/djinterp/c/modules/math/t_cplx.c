/*******************************************************************************
* djinterp [c/math]                                                     t_cplx.c
*
*   Conformance harness for c/math/cplx.h, all three families and both forms.
* First, against references: every function at the points of
* math_tests_cplx_refs.h, and the five C99 has only for real arguments --
* exp2, log2, log10, log1p and expm1 -- at those of
* math_tests_cplx_five_refs.h, each exact in float, the values computed by
* mpmath at 400 bits or as many more as each needs: each part
* within its bound, in steps (float_order.h), of the part correctly rounded
* to the family; the bounds are g_bounds below, and each function's largest
* distance is printed. Then, for each family, its own points at the ends of
* its range, against the same bounds: parts near its largest value and in
* its subnormals, and exp, sinh, cosh and tanh past its overflow and
* underflow. Then, for double and long double, every other shared point
* widened to the family's precision, against the same bounds: a point exact
* in float makes every product of two parts exact in double, so that only
* these round the arithmetic's products and give the kernels' exact steps
* their low parts; and for log, at each of those precisions, |z| just below
* 1 with both parts just below 1/sqrt(2), where the larger part's square
* less 1 is no longer exact in one value. At every reference point, the
* product's and the quotient's float and double parts are measured too, in
* ulps of the exact value: rounded once, each is within half an ulp of it
* and 2^-8 more. Then the whole powers, z^n by repeated squaring in pairs,
* at the points of math_tests_cplx_pow_refs.h, where one part of z^n
* cancels, at exponents from 2 to 2^30 of both signs: a part at least |n|
* 2^(4 - P) of the larger within a step of the part correctly rounded, a
* smaller one within a step or within |n| 2^(3 - 2P) of the larger, P long
* double's precision; the plain form's the _c form's, bit for bit; and at
* exact points on and off the axes, z^n for n from -6 to 6 the bits of the
* family's own products and quotient it stands for, each zero's sign among
* them (S382).
*
*   Then C99's Annex G (G.6), as C17 has it (its DR 471 for cacosh and ctanh):
* every value its text gives at the zeros, the infinities and the NaNs, with the
* symmetries applied to each entry -- conj for every function, and the odd and
* even functions' own -- and the functions G.6 defines through others (casin,
* catan, ccos, csin, ctan) equal to their definitions, bit for bit, at every
* pair of special parts and a few finite ones; and the five's values at the same
* parts, those of the functions each is a form of: clog's for log2, log10 and
* log1p, at 1 + z for log1p, and cexp's for exp2 and expm1, less 1 for expm1.
* Then exact values: the cuts' sides chosen by a zero's sign, the powers that
* are exact, the operations with a real operand (G.5), the infinities of
* products and quotients, the modulus past overflow, the argument's quadrants,
* the five's exact parts, a quotient's zero the formula's (S382). Then the real
* axis: each of the five, in both forms, the real function's value in the same
* form and the zero of y, bit for bit; and expm1's real part on the imaginary
* axis -0 where it underflows. In C, where the compiler has _Complex, the
* conversions to and from it; in C++, the one-name overloads; from C++14, every
* function a constant expression, for every family, with the bits it has at run
* time, pow_c's at a whole exponent of each sign among them.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_cplx.c -o t_cplx -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_cplx.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.27
*                                                            revised: 2026.11.07
*******************************************************************************/
// std
#include <float.h>   // the families' digits and least exponents
#include <stdio.h>   // printf
#include <string.h>  // strlen, memcpy
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/cplx.h"     // unit under test
#include "../../../../../inc/djinterp/c/modules/math/decimal.h"  // d_math_decimal_to_f
// tests
#include "./math_tests_cplx_refs.h"       // g_ref_exp ... g_ref_pow
#include "./math_tests_cplx_five_refs.h"  // g_ref_exp2 ... g_wide_ld_expm1
#include "./math_tests_cplx_pow_refs.h"   // g_power_f ... g_power_ld


#define D_TESTS_COUNT(array)                                                   \
    (sizeof(array) / sizeof((array)[0]))

// a distance no bound allows: a NaN against a number, or past the count's
// reach
#define D_TESTS_FAR  1000000ul

// how many failures are reported in full
#ifndef D_TESTS_SHOW
    #define D_TESTS_SHOW  40
#endif

static long g_checks   = 0;
static long g_failures = 0;

static void
d_tests_expect(
    bool        _ok,
    const char* _what,
    int         _line
)
{
    ++g_checks;

    // report the first few failures with what was checked, and where
    if (!_ok)
    {
        ++g_failures;

        if (g_failures <= D_TESTS_SHOW)
        {
            printf("  FAIL line %d: %s\n", _line, _what);
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what)                                               \
    d_tests_expect((ok), (what), __LINE__)


//==============================================================================
// 1.  THE OPERATIONS, FOR EVERY FAMILY
//==============================================================================
// Each check names an operation and a family; the family's function is
// called on parts given as long double values -- each exact in every family,
// as the references' points are -- and its result widened, exactly, to long
// double, read through volatile so that no x87 register's extra bits remain.

enum dj_tests_op
{
    D_TESTS_EXP = 0,
    D_TESTS_LOG,
    D_TESTS_SQRT,
    D_TESTS_SIN,
    D_TESTS_COS,
    D_TESTS_TAN,
    D_TESTS_ASIN,
    D_TESTS_ACOS,
    D_TESTS_ATAN,
    D_TESTS_SINH,
    D_TESTS_COSH,
    D_TESTS_TANH,
    D_TESTS_ASINH,
    D_TESTS_ACOSH,
    D_TESTS_ATANH,
    D_TESTS_EXP2,
    D_TESTS_LOG2,
    D_TESTS_LOG10,
    D_TESTS_LOG1P,
    D_TESTS_EXPM1,
    D_TESTS_POW,
    D_TESTS_ABS,       // the modulus, as the real part
    D_TESTS_ARG,       // the argument, as the real part
    D_TESTS_MUL,
    D_TESTS_DIV,
    D_TESTS_ADD,
    D_TESTS_SUB,
    D_TESTS_ADD_REAL,  // z + x, x the third part given
    D_TESTS_SUB_REAL,  // z - x
    D_TESTS_REAL_SUB,  // x - z
    D_TESTS_MUL_REAL,  // z x
    D_TESTS_DIV_REAL,  // z / x
    D_TESTS_REAL_DIV,  // x / z
    D_TESTS_CONJ,
    D_TESTS_PROJ,
    D_TESTS_NEG,
    D_TESTS_OPS
};

static const char* const g_op_names[D_TESTS_OPS] =
{
    "exp",   "log",   "sqrt",  "sin",   "cos",   "tan",   "asin",  "acos",
    "atan",  "sinh",  "cosh",  "tanh",  "asinh", "acosh", "atanh", "exp2",
    "log2",  "log10", "log1p", "expm1", "pow",
    "abs",   "arg",   "mul",   "div",   "add",   "sub",   "add_real",
    "sub_real", "real_sub", "mul_real", "div_real", "real_div", "conj",
    "proj",  "neg"
};

static const char* const g_family_names[3] = { "float", "double",
                                               "long double" };

// dj_tests_result
//   struct: a result's two parts, widened to long double.
struct dj_tests_result
{
    long double part[2];
};

// D_TESTS_CPLX_CALL
//   stamps d_tests_call_<S>: the operation _op of the family, its _c form
// where _pure, on _in[0] + _in[1] i and, for the binary operations,
// _in[2] + _in[3] i or the real _in[2]. The functions are reached through
// tables of pointers, each compiled once on its own: called directly, every
// kernel would be inlined into one function, which compilers optimize in
// time far past the sum of its parts'.
#define D_TESTS_CPLX_CALL(T, S)                                                \
typedef struct dj_cplx_##S (*d_tests_unary_##S)(struct dj_cplx_##S);           \
typedef struct dj_cplx_##S (*d_tests_binary_##S)(struct dj_cplx_##S,           \
                                                 struct dj_cplx_##S);          \
typedef struct dj_cplx_##S (*d_tests_mixed_##S)(struct dj_cplx_##S, T);        \
typedef struct dj_cplx_##S (*d_tests_left_##S)(T, struct dj_cplx_##S);         \
typedef T (*d_tests_real_##S)(struct dj_cplx_##S);                             \
                                                                               \
static const d_tests_unary_##S g_pure_##S[D_TESTS_POW] =                       \
{                                                                              \
    d_cplx_##S##_exp_c,   d_cplx_##S##_log_c,   d_cplx_##S##_sqrt_c,           \
    d_cplx_##S##_sin_c,   d_cplx_##S##_cos_c,   d_cplx_##S##_tan_c,            \
    d_cplx_##S##_asin_c,  d_cplx_##S##_acos_c,  d_cplx_##S##_atan_c,           \
    d_cplx_##S##_sinh_c,  d_cplx_##S##_cosh_c,  d_cplx_##S##_tanh_c,           \
    d_cplx_##S##_asinh_c, d_cplx_##S##_acosh_c, d_cplx_##S##_atanh_c,          \
    d_cplx_##S##_exp2_c,  d_cplx_##S##_log2_c,  d_cplx_##S##_log10_c,          \
    d_cplx_##S##_log1p_c, d_cplx_##S##_expm1_c                                 \
};                                                                             \
                                                                               \
static const d_tests_unary_##S g_plain_##S[D_TESTS_POW] =                      \
{                                                                              \
    d_cplx_##S##_exp,   d_cplx_##S##_log,   d_cplx_##S##_sqrt,                 \
    d_cplx_##S##_sin,   d_cplx_##S##_cos,   d_cplx_##S##_tan,                  \
    d_cplx_##S##_asin,  d_cplx_##S##_acos,  d_cplx_##S##_atan,                 \
    d_cplx_##S##_sinh,  d_cplx_##S##_cosh,  d_cplx_##S##_tanh,                 \
    d_cplx_##S##_asinh, d_cplx_##S##_acosh, d_cplx_##S##_atanh,                \
    d_cplx_##S##_exp2,  d_cplx_##S##_log2,  d_cplx_##S##_log10,                \
    d_cplx_##S##_log1p, d_cplx_##S##_expm1                                     \
};                                                                             \
                                                                               \
/* pow_c, pow, mul, div, add, sub */                                          \
static const d_tests_binary_##S g_binary_##S[6] =                              \
{                                                                              \
    d_cplx_##S##_pow_c, d_cplx_##S##_pow, d_cplx_##S##_mul,                    \
    d_cplx_##S##_div,   d_cplx_##S##_add, d_cplx_##S##_sub                     \
};                                                                             \
                                                                               \
/* add_real, sub_real, mul_real, div_real; real_sub, real_div */              \
static const d_tests_mixed_##S g_mixed_##S[4] =                                \
{                                                                              \
    d_cplx_##S##_add_real, d_cplx_##S##_sub_real,                              \
    d_cplx_##S##_mul_real, d_cplx_##S##_div_real                               \
};                                                                             \
                                                                               \
static const d_tests_left_##S g_left_##S[2] =                                  \
{                                                                              \
    d_cplx_##S##_real_sub, d_cplx_##S##_real_div                               \
};                                                                             \
                                                                               \
/* abs_c, abs, arg_c, arg */                                                  \
static const d_tests_real_##S g_real_##S[4] =                                  \
{                                                                              \
    d_cplx_##S##_abs_c, d_cplx_##S##_abs,                                      \
    d_cplx_##S##_arg_c, d_cplx_##S##_arg                                       \
};                                                                             \
                                                                               \
/* conj, proj, neg */                                                         \
static const d_tests_unary_##S g_other_##S[3] =                                \
{                                                                              \
    d_cplx_##S##_conj, d_cplx_##S##_proj, d_cplx_##S##_neg                     \
};                                                                             \
                                                                               \
static struct dj_tests_result                                                  \
d_tests_call_##S(                                                              \
    int                _op,                                                    \
    bool               _pure,                                                  \
    const long double* _in                                                     \
)                                                                              \
{                                                                              \
    const struct dj_cplx_##S z = d_cplx_##S##_make((T)_in[0], (T)_in[1]);      \
    const struct dj_cplx_##S w = d_cplx_##S##_make((T)_in[2], (T)_in[3]);      \
    const T                  x = (T)_in[2];                                    \
    volatile struct dj_cplx_##S r = { { (T)0, (T)0 } };                        \
    volatile T              v = (T)0;                                          \
    struct dj_tests_result  result = { { 0.0L, 0.0L } };                       \
    struct dj_cplx_##S      out = z;                                           \
                                                                               \
    if (_op < D_TESTS_POW)                                                     \
    {                                                                          \
        out = _pure ? g_pure_##S[_op](z) : g_plain_##S[_op](z);                \
    }                                                                          \
    else if (_op == D_TESTS_POW)                                               \
    {                                                                          \
        out = g_binary_##S[_pure ? 0 : 1](z, w);                               \
    }                                                                          \
    else if (_op <= D_TESTS_ARG)                                               \
    {                                                                          \
        v   = g_real_##S[((_op == D_TESTS_ARG) ? 2 : 0) + (_pure ? 0 : 1)](z); \
        out = d_cplx_##S##_make(v, (T)0);                                      \
    }                                                                          \
    else if (_op <= D_TESTS_SUB)                                               \
    {                                                                          \
        out = g_binary_##S[2 + (_op - D_TESTS_MUL)](z, w);                     \
    }                                                                          \
    else if ( (_op == D_TESTS_REAL_SUB) ||                                     \
              (_op == D_TESTS_REAL_DIV) )                                      \
    {                                                                          \
        /* by the operation: gcc at -Og resolves a constant index to a */      \
        /* direct call of a function it is told to inline, and stops */        \
        out = g_left_##S[(_op == D_TESTS_REAL_DIV) ? 1 : 0](x, z);             \
    }                                                                          \
    else if (_op <= D_TESTS_DIV_REAL)                                          \
    {                                                                          \
        const int index = (_op == D_TESTS_ADD_REAL) ? 0                        \
                        : (_op == D_TESTS_SUB_REAL) ? 1                        \
                        : (_op == D_TESTS_MUL_REAL) ? 2 : 3;                   \
                                                                               \
        out = g_mixed_##S[index](z, x);                                        \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        out = g_other_##S[_op - D_TESTS_CONJ](z);                              \
    }                                                                          \
                                                                               \
    r.part[0]        = out.part[0];                                            \
    r.part[1]        = out.part[1];                                            \
    result.part[0]   = (long double)r.part[0];                                 \
    result.part[1]   = (long double)r.part[1];                                 \
                                                                               \
    return result;                                                             \
}

D_TESTS_CPLX_CALL(float, f)
D_TESTS_CPLX_CALL(double, d)
D_TESTS_CPLX_CALL(long double, ld)

// d_tests_call
//   the operation _op of the family _family (0 float, 1 double, 2 long
// double), as d_tests_call_<S>.
static struct dj_tests_result
d_tests_call(
    int                _family,
    int                _op,
    bool               _pure,
    const long double* _in
)
{
    if (_family == 0)
    {
        return d_tests_call_f(_op, _pure, _in);
    }

    if (_family == 1)
    {
        return d_tests_call_d(_op, _pure, _in);
    }

    return d_tests_call_ld(_op, _pure, _in);
}

// d_tests_text
//   the decimal _text rounded to the family, by decimal.h, widened.
static long double
d_tests_text(
    int         _family,
    const char* _text
)
{
    const size_t length = strlen(_text);

    if (_family == 0)
    {
        float value = 0.0f;

        (void)d_math_decimal_to_f(&value, _text, length);

        return (long double)value;
    }

    if (_family == 1)
    {
        double value = 0.0;

        (void)d_math_decimal_to_d(&value, _text, length);

        return (long double)value;
    }

    {
        long double value = 0.0L;

        (void)d_math_decimal_to_ld(&value, _text, length);

        return value;
    }
}

// d_tests_clamp
//   a count of steps as an unsigned long, D_TESTS_FAR past it.
static unsigned long
d_tests_clamp(
    dj_math_umax _steps
)
{
    return (_steps >= (dj_math_umax)D_TESTS_FAR) ? D_TESTS_FAR
                                                 : (unsigned long)_steps;
}

// d_tests_steps
//   how many of the family's values lie between _a and _b, both the family's
// values widened: 0 for two NaNs, D_TESTS_FAR for a NaN and a number.
static unsigned long
d_tests_steps(
    int         _family,
    long double _a,
    long double _b
)
{
    if ( (_a != _a) ||
         (_b != _b) )
    {
        return ( (_a != _a) &&
                 (_b != _b) ) ? 0ul : D_TESTS_FAR;
    }

    if (_family == 0)
    {
        const float a = (float)_a;
        const float b = (float)_b;

        return d_tests_clamp((a <= b) ? d_math_f_steps(a, b)
                                      : d_math_f_steps(b, a));
    }

    if (_family == 1)
    {
        const double a = (double)_a;
        const double b = (double)_b;

        return d_tests_clamp((a <= b) ? d_math_d_steps(a, b)
                                      : d_math_d_steps(b, a));
    }

    return d_tests_clamp((_a <= _b) ? d_math_ld_steps(_a, _b)
                                    : d_math_ld_steps(_b, _a));
}

// d_tests_point
//   m 2^e, exactly: a point's part as the references give it, e past long
// double's normal exponents among them -- in steps of 2^8000, which pow2
// reaches, so that only an exact last product can be subnormal.
static long double
d_tests_point(
    long _m,
    int  _e
)
{
    long double value = (long double)_m;
    int         e     = _e;

    while (e > 8000)
    {
        value = value * d_internal_math_ld_pow2(8000);
        e     = e - 8000;
    }

    while (e < -8000)
    {
        value = value * d_internal_math_ld_pow2(-8000);
        e     = e + 8000;
    }

    return value * d_internal_math_ld_pow2(e);
}

// d_tests_same
//   whether two widened parts are the same value: the same bits but for a
// NaN's, any NaN being the same as any other.
static bool
d_tests_same(
    long double _a,
    long double _b
)
{
    if ( (_a != _a) ||
         (_b != _b) )
    {
        return ( (_a != _a) &&
                 (_b != _b) );
    }

    return ( (_a == _b) &&
             (d_internal_math_ld_negative(_a) ==
              d_internal_math_ld_negative(_b)) );
}


//==============================================================================
// 2.  AGAINST THE REFERENCES
//==============================================================================
// The bounds, in steps of each part, for float, double and long double, the
// _c forms' first and the plain forms' after. A _c form is the long double
// kernel rounded once to the family: within 1 step for float and double,
// where long double carries 11 bits past double's, and its own bound for
// long double. A plain form is the same algorithm in the family's type on
// the C library's real functions: their error, a few steps, and the kernel's;
// the plain pow's general case is the _c kernel's (cplx.h, S266). The five
// are the kernel rounded once in both forms, off the real axis, where the
// references lie: within a step of the part correctly rounded, the kernel's
// pairs carrying it past twice the family's precision (cplx.h, S342).
// Where long double is no wider than double, double's _c bound is long
// double's (d_tests_bound).

// dj_tests_bounds
//   struct: an operation's bounds.
struct dj_tests_bounds
{
    unsigned pure[3];
    unsigned plain[3];
};

static const struct dj_tests_bounds g_bounds[D_TESTS_ABS + 1] =
{
    { { 1u, 1u, 3u }, { 4u, 4u, 4u } },  // exp
    { { 1u, 1u, 2u }, { 3u, 3u, 3u } },  // log
    { { 1u, 1u, 2u }, { 3u, 3u, 3u } },  // sqrt
    { { 1u, 1u, 3u }, { 4u, 4u, 4u } },  // sin
    { { 1u, 1u, 3u }, { 4u, 4u, 4u } },  // cos
    { { 1u, 1u, 4u }, { 5u, 5u, 6u } },  // tan
    { { 1u, 1u, 2u }, { 4u, 4u, 4u } },  // asin
    { { 1u, 1u, 3u }, { 4u, 4u, 4u } },  // acos
    { { 1u, 1u, 2u }, { 4u, 4u, 4u } },  // atan
    { { 1u, 1u, 2u }, { 4u, 4u, 4u } },  // sinh
    { { 1u, 1u, 3u }, { 4u, 4u, 4u } },  // cosh
    { { 1u, 1u, 4u }, { 5u, 5u, 6u } },  // tanh
    { { 1u, 1u, 3u }, { 4u, 4u, 4u } },  // asinh
    { { 1u, 1u, 2u }, { 4u, 4u, 4u } },  // acosh
    { { 1u, 1u, 2u }, { 4u, 4u, 4u } },  // atanh
    { { 1u, 1u, 1u }, { 1u, 1u, 1u } },  // exp2
    { { 1u, 1u, 1u }, { 1u, 1u, 1u } },  // log2
    { { 1u, 1u, 1u }, { 1u, 1u, 1u } },  // log10
    { { 1u, 1u, 1u }, { 1u, 1u, 1u } },  // log1p
    { { 1u, 1u, 1u }, { 1u, 1u, 1u } },  // expm1
    { { 1u, 1u, 2u }, { 2u, 2u, 3u } },  // pow
    { { 1u, 1u, 1u }, { 2u, 2u, 2u } }   // abs and arg
};

// d_tests_bound
//   the bound of the operation _op (pow and before, abs for arg) for the
// family, in the form _pure.
static unsigned long
d_tests_bound(
    int  _op,
    int  _family,
    bool _pure
)
{
    const int index = (_op > D_TESTS_ABS) ? D_TESTS_ABS : _op;

    if (!_pure)
    {
        return (unsigned long)g_bounds[index].plain[_family];
    }

    // long double no wider than double: double's kernel is long double's
    if ( (_family == 1) &&
         (LDBL_MANT_DIG < (DBL_MANT_DIG + 8)) )
    {
        return (unsigned long)g_bounds[index].pure[2];
    }

    return (unsigned long)g_bounds[index].pure[_family];
}

// The product and the quotient are each rounded once (cplx.h, S281): but
// where the parts' products cancel nearly to nothing, a float or double part
// is the family's value nearest the exact one, or, where the exact one lies
// within a small fraction of an ulp of a midpoint, the value beside it.
// Each reference point measures it in ulps of the exact value, against half
// an ulp and 2^-8 more; the exact value is the reference rounded to long
// double, within 2^-12 of a double's ulp where long double has 64 bits. A
// part whose exact value lies past the family's normal range, where it
// overflows or its scaled quotient rounds again, is not measured, and a long
// double less than 8 bits wider than double measures float's alone.

// half an ulp, and 2^-8 of one for the long double reference and the
// roundings before the last
#define D_TESTS_HALF_BOUND  0.50390625L

// the product's and the quotient's largest errors, float's and double's
static long double g_half_worst[2][2] = { { 0.0L, 0.0L }, { 0.0L, 0.0L } };

// d_tests_ulps
//   |_got - _exact| in ulps of the family (0 float, 1 double) at _exact's
// binade; 0 for an exact value past the family's normal range, which is not
// measured, and D_TESTS_FAR for a NaN against it.
static long double
d_tests_ulps(
    int         _family,
    long double _got,
    long double _exact
)
{
    const int digits = (_family == 0) ? FLT_MANT_DIG : DBL_MANT_DIG;
    const long double least = (_family == 0) ? (long double)FLT_MIN
                                             : (long double)DBL_MIN;
    const long double most  = (_family == 0) ? (long double)FLT_MAX
                                             : (long double)DBL_MAX;
    const long double size  = (_exact < 0.0L) ? -_exact : _exact;
    const long double gap   = (_got > _exact) ? (_got - _exact)
                                              : (_exact - _got);

    if ( (size < least) ||
         (size > most) )
    {
        return 0.0L;
    }

    if (_got != _got)
    {
        return (long double)D_TESTS_FAR;
    }

    return gap / (d_internal_math_ld_binade_of(size).power /
                  d_internal_math_ld_pow2(digits - 1));
}

// d_tests_check_half
//   the product's or the quotient's parts in float or double, _got, against
// D_TESTS_HALF_BOUND of the exact parts _re and _im; the largest kept.
static void
d_tests_check_half(
    int                           _op,
    int                           _family,
    const long double*            _in,
    const struct dj_tests_result* _got,
    const char*                   _re,
    const char*                   _im
)
{
    const int         which = (_op == D_TESTS_MUL) ? 0 : 1;
    const long double re    = d_tests_text(2, _re);
    const long double im    = d_tests_text(2, _im);
    const long double other = d_tests_ulps(_family, _got->part[1], im);
    long double       error = d_tests_ulps(_family, _got->part[0], re);
    bool              ok    = false;

    if (other > error)
    {
        error = other;
    }

    if (error > g_half_worst[which][_family])
    {
        g_half_worst[which][_family] = error;
    }

    ok = (error <= D_TESTS_HALF_BOUND);
    D_TESTS_EXPECT(ok, "a part within half an ulp and 2^-8 of the exact");

    if ( (!ok) &&
         (g_failures <= D_TESTS_SHOW) )
    {
        printf("  %s %s at (%.21Lg, %.21Lg) (%.21Lg, %.21Lg): %.6Lf ulps:"
               " (%.21Lg, %.21Lg), want (%s, %s)\n",
               g_op_names[_op],
               g_family_names[_family],
               _in[0],
               _in[1],
               _in[2],
               _in[3],
               error,
               _got->part[0],
               _got->part[1],
               _re,
               _im);
    }

    return;
}

// d_tests_report
//   one point's failure, once the distance passes the bound.
static void
d_tests_report(
    int                           _op,
    int                           _family,
    bool                          _pure,
    const long double*            _in,
    const struct dj_tests_result* _got,
    const char*                   _re,
    const char*                   _im,
    unsigned long                 _steps
)
{
    if (g_failures <= D_TESTS_SHOW)
    {
        printf("  %s %s%s at (%.21Lg, %.21Lg) (%.21Lg, %.21Lg): %lu steps:"
               " (%.21Lg, %.21Lg), want (%s, %s)\n",
               g_op_names[_op],
               g_family_names[_family],
               _pure ? " _c" : "",
               _in[0],
               _in[1],
               _in[2],
               _in[3],
               _steps,
               _got->part[0],
               _got->part[1],
               _re,
               _im);
    }

    return;
}

// d_tests_check_ref
//   one point of one operation, both forms, the families _first to _last:
// each part's distance against the bound; the largest kept in _worst, family
// by family, the _c form's first. The product's and the quotient's float
// and double parts, which have one form, measured once against
// D_TESTS_HALF_BOUND as well (d_tests_check_half).
static void
d_tests_check_ref(
    int                _op,
    const long double* _in,
    const char*        _re,
    const char*        _im,
    unsigned long*     _worst,
    int                _first,
    int                _last
)
{
    int family = 0;

    for (family = _first; family <= _last; ++family)
    {
        const long double re = d_tests_text(family, _re);
        const long double im = d_tests_text(family, _im);
        int               form = 0;

        for (form = 0; form < 2; ++form)
        {
            const bool pure = (form == 0);
            const struct dj_tests_result got =
                d_tests_call(family, _op, pure, _in);
            unsigned long steps = d_tests_steps(family, got.part[0], re);
            unsigned long other = 0ul;

            // abs and arg give one real value each
            if (_op == D_TESTS_ABS)
            {
                other = 0ul;
            }
            else if (_op == D_TESTS_ARG)
            {
                steps = d_tests_steps(family, got.part[0], im);
            }
            else
            {
                other = d_tests_steps(family, got.part[1], im);
            }

            if (other > steps)
            {
                steps = other;
            }

            // one form, measured once, where long double measures it
            if ( ( (_op == D_TESTS_MUL) ||
                   (_op == D_TESTS_DIV) )       &&
                 (pure)                         &&
                 ( (family == 0) ||
                   ( (family == 1) &&
                     (LDBL_MANT_DIG >= (DBL_MANT_DIG + 8)) ) ) )
            {
                d_tests_check_half(_op, family, _in, &got, _re, _im);
            }

            if (steps > _worst[(2 * family) + form])
            {
                _worst[(2 * family) + form] = steps;
            }

            {
                const bool ok = (steps <= d_tests_bound(_op, family, pure));

                D_TESTS_EXPECT(ok, "a part within its bound of the reference");

                if (!ok)
                {
                    d_tests_report(_op, family, pure, _in, &got, _re, _im,
                                   steps);
                }
            }
        }
    }

    return;
}

// d_tests_print_worst
//   the largest distances of one operation, the _c form's and the plain
// form's, for each family.
static void
d_tests_print_worst(
    const char*          _name,
    const unsigned long* _worst
)
{
    printf("  %-6s float %lu %lu  double %lu %lu  long double %lu %lu\n",
           _name,
           _worst[0],
           _worst[1],
           _worst[2],
           _worst[3],
           _worst[4],
           _worst[5]);

    return;
}

// the unary references, in the operations' order
static const struct dj_tests_cplx_ref* const g_unary_refs[D_TESTS_POW] =
{
    g_ref_exp,   g_ref_log,   g_ref_sqrt,  g_ref_sin,   g_ref_cos,
    g_ref_tan,   g_ref_asin,  g_ref_acos,  g_ref_atan,  g_ref_sinh,
    g_ref_cosh,  g_ref_tanh,  g_ref_asinh, g_ref_acosh, g_ref_atanh,
    g_ref_exp2,  g_ref_log2,  g_ref_log10, g_ref_log1p, g_ref_expm1
};

static const size_t g_unary_counts[D_TESTS_POW] =
{
    D_TESTS_COUNT(g_ref_exp),   D_TESTS_COUNT(g_ref_log),
    D_TESTS_COUNT(g_ref_sqrt),  D_TESTS_COUNT(g_ref_sin),
    D_TESTS_COUNT(g_ref_cos),   D_TESTS_COUNT(g_ref_tan),
    D_TESTS_COUNT(g_ref_asin),  D_TESTS_COUNT(g_ref_acos),
    D_TESTS_COUNT(g_ref_atan),  D_TESTS_COUNT(g_ref_sinh),
    D_TESTS_COUNT(g_ref_cosh),  D_TESTS_COUNT(g_ref_tanh),
    D_TESTS_COUNT(g_ref_asinh), D_TESTS_COUNT(g_ref_acosh),
    D_TESTS_COUNT(g_ref_atanh), D_TESTS_COUNT(g_ref_exp2),
    D_TESTS_COUNT(g_ref_log2),  D_TESTS_COUNT(g_ref_log10),
    D_TESTS_COUNT(g_ref_log1p), D_TESTS_COUNT(g_ref_expm1)
};

// d_tests_references
//   every reference, every operation and family, both forms.
static void
d_tests_references(void)
{
    int    op = 0;
    size_t i  = 0u;

    printf("  the largest distances, in steps, _c then plain:\n");

    for (op = 0; op < D_TESTS_POW; ++op)
    {
        unsigned long worst[6] = { 0ul, 0ul, 0ul, 0ul, 0ul, 0ul };

        for (i = 0u; i < g_unary_counts[op]; ++i)
        {
            const struct dj_tests_cplx_ref* r = &g_unary_refs[op][i];
            long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

            in[0] = d_tests_point(r->xm, r->xe);
            in[1] = d_tests_point(r->ym, r->ye);
            d_tests_check_ref(op, in, r->re, r->im, worst, 0, 2);
        }

        d_tests_print_worst(g_op_names[op], worst);
    }

    {
        unsigned long abs_worst[6] = { 0ul, 0ul, 0ul, 0ul, 0ul, 0ul };
        unsigned long arg_worst[6] = { 0ul, 0ul, 0ul, 0ul, 0ul, 0ul };

        for (i = 0u; i < D_TESTS_COUNT(g_ref_abs_arg); ++i)
        {
            const struct dj_tests_cplx_ref* r = &g_ref_abs_arg[i];
            long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

            in[0] = d_tests_point(r->xm, r->xe);
            in[1] = d_tests_point(r->ym, r->ye);
            d_tests_check_ref(D_TESTS_ABS, in, r->re, r->im, abs_worst,
                              0, 2);
            d_tests_check_ref(D_TESTS_ARG, in, r->re, r->im, arg_worst,
                              0, 2);
        }

        d_tests_print_worst("abs", abs_worst);
        d_tests_print_worst("arg", arg_worst);
    }

    {
        const struct dj_tests_cplx_ref2* const tables[3] =
            { g_ref_mul, g_ref_div, g_ref_pow };
        const size_t counts[3] = { D_TESTS_COUNT(g_ref_mul),
                                   D_TESTS_COUNT(g_ref_div),
                                   D_TESTS_COUNT(g_ref_pow) };
        const int    ops[3]    = { D_TESTS_MUL, D_TESTS_DIV, D_TESTS_POW };
        int          k = 0;

        for (k = 0; k < 3; ++k)
        {
            unsigned long worst[6] = { 0ul, 0ul, 0ul, 0ul, 0ul, 0ul };

            for (i = 0u; i < counts[k]; ++i)
            {
                const struct dj_tests_cplx_ref2* r = &tables[k][i];
                long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

                in[0] = d_tests_point(r->xm, r->xe);
                in[1] = d_tests_point(r->ym, r->ye);
                in[2] = d_tests_point(r->um, r->ue);
                in[3] = d_tests_point(r->vm, r->ve);
                d_tests_check_ref(ops[k], in, r->re, r->im, worst, 0, 2);
            }

            d_tests_print_worst(g_op_names[ops[k]], worst);
        }
    }

    {
        // pow on the axes: the turns and the general case's axis angles
        unsigned long worst[6] = { 0ul, 0ul, 0ul, 0ul, 0ul, 0ul };

        for (i = 0u; i < D_TESTS_COUNT(g_ref_pow_axes); ++i)
        {
            const struct dj_tests_cplx_ref2* r = &g_ref_pow_axes[i];
            long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

            in[0] = d_tests_point(r->xm, r->xe);
            in[1] = d_tests_point(r->ym, r->ye);
            in[2] = d_tests_point(r->um, r->ue);
            in[3] = d_tests_point(r->vm, r->ve);
            d_tests_check_ref(D_TESTS_POW, in, r->re, r->im, worst, 0, 2);
        }

        d_tests_print_worst("pow_ax", worst);
    }

    return;
}


// dj_tests_edge
//   struct: one family's points at the ends of its range, for one function.
struct dj_tests_edge
{
    int                             family;
    int                             op;
    const struct dj_tests_cplx_ref* refs;
    size_t                          count;
};

#define D_TESTS_EDGE(F, S, OP, NAME)                                           \
    { (F), (OP), g_edge_##S##_##NAME, D_TESTS_COUNT(g_edge_##S##_##NAME) }

#define D_TESTS_EDGES(F, S)                                                    \
    D_TESTS_EDGE(F, S, D_TESTS_EXP, exp),                                      \
    D_TESTS_EDGE(F, S, D_TESTS_LOG, log),                                      \
    D_TESTS_EDGE(F, S, D_TESTS_SQRT, sqrt),                                    \
    D_TESTS_EDGE(F, S, D_TESTS_ACOS, acos),                                    \
    D_TESTS_EDGE(F, S, D_TESTS_SINH, sinh),                                    \
    D_TESTS_EDGE(F, S, D_TESTS_COSH, cosh),                                    \
    D_TESTS_EDGE(F, S, D_TESTS_TANH, tanh),                                    \
    D_TESTS_EDGE(F, S, D_TESTS_ASINH, asinh),                                  \
    D_TESTS_EDGE(F, S, D_TESTS_ATANH, atanh),                                  \
    D_TESTS_EDGE(F, S, D_TESTS_EXP2, exp2),                                    \
    D_TESTS_EDGE(F, S, D_TESTS_LOG2, log2),                                    \
    D_TESTS_EDGE(F, S, D_TESTS_LOG10, log10),                                  \
    D_TESTS_EDGE(F, S, D_TESTS_LOG1P, log1p),                                  \
    D_TESTS_EDGE(F, S, D_TESTS_EXPM1, expm1),                                  \
    D_TESTS_EDGE(F, S, D_TESTS_ABS, abs_arg),                                  \
    D_TESTS_EDGE(F, S, D_TESTS_ARG, abs_arg)

static const struct dj_tests_edge g_edges[] =
{
    D_TESTS_EDGES(0, f),
    D_TESTS_EDGES(1, d),
    D_TESTS_EDGES(2, ld)
};

// d_tests_edges
//   each family's own points at the ends of its range -- near its largest
// value and in its subnormals, where exp, sinh, cosh, tanh, exp2 and expm1
// pass its overflow and underflow, and exp2's y ln 2 far out -- against the
// same bounds; the largest distances printed, family by family.
static void
d_tests_edges(void)
{
    size_t k = 0u;

    printf("  at the ends of each family's range, _c then plain:\n");

    for (k = 0u; k < D_TESTS_COUNT(g_edges); ++k)
    {
        const struct dj_tests_edge* e = &g_edges[k];
        unsigned long worst[6] = { 0ul, 0ul, 0ul, 0ul, 0ul, 0ul };
        size_t        i        = 0u;

        for (i = 0u; i < e->count; ++i)
        {
            long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

            in[0] = d_tests_point(e->refs[i].xm, e->refs[i].xe);
            in[1] = d_tests_point(e->refs[i].ym, e->refs[i].ye);
            d_tests_check_ref(e->op, in, e->refs[i].re, e->refs[i].im, worst,
                              e->family, e->family);
        }

        printf("    %-11s %-6s %lu %lu\n",
               g_family_names[e->family],
               g_op_names[e->op],
               worst[2 * e->family],
               worst[(2 * e->family) + 1]);
    }

    return;
}


// d_tests_full_point
//   a full-width part, (_m[0] 2^48 + _m[1] 2^24 + _m[2]) 2^_e, exactly in
// long double: each limb's term is exact, and each sum a prefix of the
// part's bits.
static long double
d_tests_full_point(
    const long* _m,
    int         _e
)
{
    const long double high = d_tests_point(_m[0], _e + 48);
    const long double mid  = d_tests_point(_m[1], _e + 24);
    const long double low  = d_tests_point(_m[2], _e);

    return (high + mid) + low;
}

// D_TESTS_WIDE_UNARY
//   the full-width tables of one family's unary operations, in the
// operations' order, and their counts.
#define D_TESTS_WIDE_UNARY(S)                                                  \
static const struct dj_tests_cplx_wide* const g_wide_unary_##S[D_TESTS_POW] =  \
{                                                                              \
    g_wide_##S##_exp,   g_wide_##S##_log,   g_wide_##S##_sqrt,                 \
    g_wide_##S##_sin,   g_wide_##S##_cos,   g_wide_##S##_tan,                  \
    g_wide_##S##_asin,  g_wide_##S##_acos,  g_wide_##S##_atan,                 \
    g_wide_##S##_sinh,  g_wide_##S##_cosh,  g_wide_##S##_tanh,                 \
    g_wide_##S##_asinh, g_wide_##S##_acosh, g_wide_##S##_atanh,                \
    g_wide_##S##_exp2,  g_wide_##S##_log2,  g_wide_##S##_log10,                \
    g_wide_##S##_log1p, g_wide_##S##_expm1                                     \
};                                                                             \
                                                                               \
static const size_t g_wide_unary_counts_##S[D_TESTS_POW] =                     \
{                                                                              \
    D_TESTS_COUNT(g_wide_##S##_exp),   D_TESTS_COUNT(g_wide_##S##_log),        \
    D_TESTS_COUNT(g_wide_##S##_sqrt),  D_TESTS_COUNT(g_wide_##S##_sin),        \
    D_TESTS_COUNT(g_wide_##S##_cos),   D_TESTS_COUNT(g_wide_##S##_tan),        \
    D_TESTS_COUNT(g_wide_##S##_asin),  D_TESTS_COUNT(g_wide_##S##_acos),       \
    D_TESTS_COUNT(g_wide_##S##_atan),  D_TESTS_COUNT(g_wide_##S##_sinh),       \
    D_TESTS_COUNT(g_wide_##S##_cosh),  D_TESTS_COUNT(g_wide_##S##_tanh),       \
    D_TESTS_COUNT(g_wide_##S##_asinh), D_TESTS_COUNT(g_wide_##S##_acosh),      \
    D_TESTS_COUNT(g_wide_##S##_atanh), D_TESTS_COUNT(g_wide_##S##_exp2),       \
    D_TESTS_COUNT(g_wide_##S##_log2),  D_TESTS_COUNT(g_wide_##S##_log10),      \
    D_TESTS_COUNT(g_wide_##S##_log1p), D_TESTS_COUNT(g_wide_##S##_expm1)       \
};

D_TESTS_WIDE_UNARY(d)
D_TESTS_WIDE_UNARY(ld)

// d_tests_full_width_family
//   one family's full-width tables, every operation in both forms, each
// operation's largest distances printed.
static void
d_tests_full_width_family(
    int                                      _family,
    const struct dj_tests_cplx_wide* const*  _unary,
    const size_t*                            _counts,
    const struct dj_tests_cplx_wide*         _abs_arg,
    size_t                                   _abs_arg_count,
    const struct dj_tests_cplx_wide2* const* _binary,
    const size_t*                            _binary_counts
)
{
    static const int binary_ops[3] = { D_TESTS_MUL, D_TESTS_DIV,
                                       D_TESTS_POW };
    unsigned long worst[D_TESTS_ARG + 3][6];
    size_t        i  = 0u;
    int           op = 0;
    int           k  = 0;

    memset(worst, 0, sizeof(worst));

    for (op = 0; op < D_TESTS_POW; ++op)
    {
        for (i = 0u; i < _counts[op]; ++i)
        {
            const struct dj_tests_cplx_wide* r = &_unary[op][i];
            long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

            in[0] = d_tests_full_point(r->xm, r->xe);
            in[1] = d_tests_full_point(r->ym, r->ye);
            d_tests_check_ref(op, in, r->re, r->im, worst[op], _family,
                              _family);
        }
    }

    for (i = 0u; i < _abs_arg_count; ++i)
    {
        const struct dj_tests_cplx_wide* r = &_abs_arg[i];
        long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

        in[0] = d_tests_full_point(r->xm, r->xe);
        in[1] = d_tests_full_point(r->ym, r->ye);
        d_tests_check_ref(D_TESTS_ABS, in, r->re, r->im, worst[D_TESTS_ABS],
                          _family, _family);
        d_tests_check_ref(D_TESTS_ARG, in, r->re, r->im, worst[D_TESTS_ARG],
                          _family, _family);
    }

    for (k = 0; k < 3; ++k)
    {
        // mul and div after arg, pow in its own place
        unsigned long* w = (binary_ops[k] == D_TESTS_POW)
                         ? worst[D_TESTS_POW]
                         : worst[D_TESTS_ARG + 1 + k];

        for (i = 0u; i < _binary_counts[k]; ++i)
        {
            const struct dj_tests_cplx_wide2* r = &_binary[k][i];
            long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

            in[0] = d_tests_full_point(r->xm, r->xe);
            in[1] = d_tests_full_point(r->ym, r->ye);
            in[2] = d_tests_full_point(r->um, r->ue);
            in[3] = d_tests_full_point(r->vm, r->ve);
            d_tests_check_ref(binary_ops[k], in, r->re, r->im, w, _family,
                              _family);
        }
    }

    for (op = 0; op <= (D_TESTS_ARG + 2); ++op)
    {
        const char* name = (op == (D_TESTS_ARG + 1)) ? "mul"
                         : (op == (D_TESTS_ARG + 2)) ? "div"
                                                     : g_op_names[op];

        printf("    %-11s %-6s %lu %lu\n",
               g_family_names[_family],
               name,
               worst[op][2 * _family],
               worst[op][(2 * _family) + 1]);
    }

    return;
}

// d_tests_full_width
//   the full-width tables of double and of long double, long double's
// where it is wider than double; then the product's and the quotient's
// largest errors in ulps of the exact value, at every point so far
// (d_tests_check_half).
static void
d_tests_full_width(void)
{
    const struct dj_tests_cplx_wide2* const binary_d[3] =
        { g_wide_d_mul, g_wide_d_div, g_wide_d_pow };
    const size_t binary_counts_d[3] = { D_TESTS_COUNT(g_wide_d_mul),
                                        D_TESTS_COUNT(g_wide_d_div),
                                        D_TESTS_COUNT(g_wide_d_pow) };
    const struct dj_tests_cplx_wide2* const binary_ld[3] =
        { g_wide_ld_mul, g_wide_ld_div, g_wide_ld_pow };
    const size_t binary_counts_ld[3] = { D_TESTS_COUNT(g_wide_ld_mul),
                                         D_TESTS_COUNT(g_wide_ld_div),
                                         D_TESTS_COUNT(g_wide_ld_pow) };

    printf("  at each family's full width, _c then plain:\n");
    d_tests_full_width_family(1, g_wide_unary_d, g_wide_unary_counts_d,
                        g_wide_d_abs_arg, D_TESTS_COUNT(g_wide_d_abs_arg),
                        binary_d, binary_counts_d);

    // a long double of double's width takes no point of 64 bits exactly
    if (LDBL_MANT_DIG >= 64)
    {
        d_tests_full_width_family(2, g_wide_unary_ld, g_wide_unary_counts_ld,
                            g_wide_ld_abs_arg,
                            D_TESTS_COUNT(g_wide_ld_abs_arg),
                            binary_ld, binary_counts_ld);
    }

    printf("  the product's and the quotient's largest errors, in ulps of"
           " the exact value:\n");
    printf("    mul    float %.6Lf  double %.6Lf\n",
           g_half_worst[0][0],
           g_half_worst[0][1]);
    printf("    div    float %.6Lf  double %.6Lf\n",
           g_half_worst[1][0],
           g_half_worst[1][1]);

    return;
}

// The whole powers, z^n by repeated squaring carried in pairs (cplx.h,
// S370), at the points of math_tests_cplx_pow_refs.h, where one part of z^n
// cancels to some 2^-c of the modulus: each part that is at least |n|
// 2^(4 - P) of the larger part, P long double's precision, within a step of
// the part correctly rounded; each smaller part, which may cancel past what
// long double's pairs carry to a step, within a step or within |n|
// 2^(3 - 2P) of the larger part; and the plain form's parts the _c form's,
// bit for bit. Then, at exact points on and off the axes, whose powers and
// inverses are exact, z^n the bits of the products and the quotient it
// stands for, each zero's sign among them: the family's own d_cplx_<S>_mul
// from n's highest bit down, and its d_cplx_<S>_div of 1 by that for n < 0
// (S370, S382).

// d_tests_power_scale
//   |_n| times the larger of the exact parts _re and _im, times 2^_k: a
// whole power's threshold, k = 4 - P, and its allowance, k = 3 - 2P.
static long double
d_tests_power_scale(
    long        _n,
    long double _re,
    long double _im,
    int         _k
)
{
    const long double a     = (_re < 0.0L) ? -_re : _re;
    const long double b     = (_im < 0.0L) ? -_im : _im;
    const long double count = (_n < 0) ? -(long double)_n : (long double)_n;

    return (count * ((a > b) ? a : b)) * d_internal_math_ld_pow2(_k);
}

// dj_tests_power_tally
//   struct: one form's whole powers in one family: the largest distance in
// steps of a part above the threshold; the parts below it; and the largest
// error of those, as a share of the allowance.
struct dj_tests_power_tally
{
    unsigned long within;
    unsigned long below;
    long double   share;
};

// d_tests_power_part
//   one part of a whole power against its reference: where the exact part,
// _exact, is at least _threshold, within a step of the part correctly
// rounded, _want; otherwise within a step too or within _allowance of the
// exact part; counted in _tally, and the steps into *_steps where larger.
static bool
d_tests_power_part(
    int                          _family,
    long double                  _got,
    long double                  _want,
    long double                  _exact,
    long double                  _threshold,
    long double                  _allowance,
    unsigned long*               _steps,
    struct dj_tests_power_tally* _tally
)
{
    const unsigned long steps = d_tests_steps(_family, _got, _want);
    const long double   size  = (_exact < 0.0L) ? -_exact : _exact;
    const long double   error = (_got > _exact) ? (_got - _exact)
                                                : (_exact - _got);

    if (steps > *_steps)
    {
        *_steps = steps;
    }

    // at least the threshold: within a step
    if (size >= _threshold)
    {
        if (steps > _tally->within)
        {
            _tally->within = steps;
        }

        return (steps <= 1ul);
    }

    ++_tally->below;

    // below it, within a step too, or, past it, within the allowance, its
    // share kept: the family's own rounding is within the step
    if (steps <= 1ul)
    {
        return true;
    }

    if ( (_got == _got) &&
         (_allowance > 0.0L) &&
         ((error / _allowance) > _tally->share) )
    {
        _tally->share = error / _allowance;
    }

    return ( (_got == _got) &&
             (error <= _allowance) );
}

// d_tests_check_powers
//   one family's whole powers, _rows: each row's parts, in both forms,
// within a step of the reference above the threshold and within the
// allowance below it; the plain form's parts the _c form's, bit for bit;
// each form's tally printed.
static void
d_tests_check_powers(
    int                               _family,
    const struct dj_tests_cplx_power* _rows,
    size_t                            _count
)
{
    struct dj_tests_power_tally tally[2] = { { 0ul, 0ul, 0.0L },
                                             { 0ul, 0ul, 0.0L } };
    size_t                      i        = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const struct dj_tests_cplx_power* r = &_rows[i];
        const long double re    = d_tests_text(_family, r->re);
        const long double im    = d_tests_text(_family, r->im);
        const long double re_x  = d_tests_text(2, r->re);
        const long double im_x  = d_tests_text(2, r->im);
        const long double least = d_tests_power_scale(r->n, re_x, im_x,
                                                      4 - LDBL_MANT_DIG);
        const long double allow = d_tests_power_scale(r->n, re_x, im_x,
                                                      3 - (2 * LDBL_MANT_DIG));
        struct dj_tests_result got[2] = { { { 0.0L, 0.0L } },
                                          { { 0.0L, 0.0L } } };
        long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
        int         form  = 0;

        in[0] = d_tests_full_point(r->xm, r->xe);
        in[1] = d_tests_full_point(r->ym, r->ye);
        in[2] = (long double)r->n;

        for (form = 0; form < 2; ++form)
        {
            const bool pure = (form == 0);
            unsigned long here = 0ul;
            bool          ok   = true;

            got[form] = d_tests_call(_family, D_TESTS_POW, pure, in);
            ok = d_tests_power_part(_family, got[form].part[0], re, re_x,
                                    least, allow, &here, &tally[form]);
            ok = d_tests_power_part(_family, got[form].part[1], im, im_x,
                                    least, allow, &here, &tally[form]) && ok;

            D_TESTS_EXPECT(ok, "a whole power's part within a step, or, "
                               "below the threshold, within the allowance");

            if (!ok)
            {
                d_tests_report(D_TESTS_POW, _family, pure, in, &got[form],
                               r->re, r->im, here);
            }
        }

        D_TESTS_EXPECT( (d_tests_same(got[0].part[0], got[1].part[0])) &&
                        (d_tests_same(got[0].part[1], got[1].part[1])),
                        "a whole power's plain form the _c form's, bit for "
                        "bit" );
    }

    printf("    %-11s %3lu rows: at most %lu %lu steps; below the threshold"
           " %lu %lu parts, at most %.3Lf %.3Lf of the allowance\n",
           g_family_names[_family],
           (unsigned long)_count,
           tally[0].within,
           tally[1].within,
           tally[0].below,
           tally[1].below,
           tally[0].share,
           tally[1].share);

    return;
}

// d_tests_power_by_products
//   _z^_n in the family as the products and the quotient it stands for:
// the family's own d_cplx_<S>_mul from _n's highest bit down, the power
// squared and then times _z where the bit is set, as cplx.h's kernel takes
// the bits, and for _n < 0 the family's d_cplx_<S>_div of 1 by that power.
static struct dj_tests_result
d_tests_power_by_products(
    int                _family,
    const long double* _z,
    int                _n
)
{
    const unsigned         count = (_n < 0) ? (unsigned)(-_n) : (unsigned)_n;
    struct dj_tests_result m     = { { 0.0L, 0.0L } };
    long double            in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    unsigned               bit   = 1u;

    m.part[0] = _z[0];
    m.part[1] = _z[1];

    // the highest bit
    while (bit <= (count / 2u))
    {
        bit = bit * 2u;
    }

    // squared, then times z where the bit is set
    for (bit = bit / 2u; bit > 0u; bit = bit / 2u)
    {
        in[0] = m.part[0];
        in[1] = m.part[1];
        in[2] = m.part[0];
        in[3] = m.part[1];
        m     = d_tests_call(_family, D_TESTS_MUL, true, in);

        if ((count & bit) != 0u)
        {
            in[0] = m.part[0];
            in[1] = m.part[1];
            in[2] = _z[0];
            in[3] = _z[1];
            m     = d_tests_call(_family, D_TESTS_MUL, true, in);
        }
    }

    // a negative exponent: 1 over the power
    if (_n < 0)
    {
        in[0] = 1.0L;
        in[1] = 0.0L;
        in[2] = m.part[0];
        in[3] = m.part[1];
        m     = d_tests_call(_family, D_TESTS_DIV, true, in);
    }

    return m;
}

// d_tests_power_zeros
//   z^n at exact points whose powers and inverses are exact, on the axes
// with zeros of either sign and off them, n from -6 to 6, in every family
// and both forms: the bits of d_tests_power_by_products, each zero's sign
// among them -- the products' for n > 0 (S370), the quotient's, the
// formula's, for n < 0 (S382).
static void
d_tests_power_zeros(void)
{
    static const long double points[][2] =
    {
        { -2.0L, 0.0L },  { -2.0L, -0.0L }, { 2.0L, 0.0L },   { 2.0L, -0.0L },
        { 0.0L, 2.0L },   { 0.0L, -2.0L },  { -0.0L, 2.0L },  { -0.0L, -2.0L },
        { 1.0L, 1.0L },   { 1.0L, -1.0L },  { -1.0L, 1.0L },  { -1.0L, -1.0L },
        { -0.5L, 0.5L },  { 2.0L, -2.0L },  { -1.0L, -0.0L }, { -0.0L, 1.0L }
    };
    long failed[2] = { 0L, 0L };  // n > 0, n < 0
    long zeros[2]  = { 0L, 0L };
    int  family    = 0;

    for (family = 0; family < 3; ++family)
    {
        size_t k = 0u;

        for (k = 0u; k < D_TESTS_COUNT(points); ++k)
        {
            int n = 0;

            for (n = -6; n <= 6; ++n)
            {
                const int              side = (n < 0) ? 1 : 0;
                struct dj_tests_result want = { { 0.0L, 0.0L } };
                long double            in[4];
                int                    form = 0;

                // no z^0 here: 1 for every z, among the exact values
                if (n == 0)
                {
                    continue;
                }

                want  = d_tests_power_by_products(family, points[k], n);
                in[0] = points[k][0];
                in[1] = points[k][1];
                in[2] = (long double)n;
                in[3] = 0.0L;
                zeros[side] += ( ((want.part[0] == 0.0L) ? 1L : 0L) +
                                 ((want.part[1] == 0.0L) ? 1L : 0L) );

                for (form = 0; form < 2; ++form)
                {
                    const struct dj_tests_result got =
                        d_tests_call(family, D_TESTS_POW, (form == 0), in);

                    failed[side] +=
                        ( (d_tests_same(got.part[0], want.part[0])) &&
                          (d_tests_same(got.part[1], want.part[1])) )
                            ? 0L : 1L;
                }
            }
        }
    }

    D_TESTS_EXPECT( (failed[0] == 0L) &&
                    (zeros[0] > 0L),
                    "z^n, n from 1 to 6, at exact points: the bits of the "
                    "family's repeated squaring by d_cplx_<S>_mul, each "
                    "zero's sign the products' (S370)" );
    D_TESTS_EXPECT( (failed[1] == 0L) &&
                    (zeros[1] > 0L),
                    "z^n, n from -6 to -1, at exact points: the bits of 1 "
                    "over that by d_cplx_<S>_div, each zero's sign the "
                    "quotient's, the formula's (S382)" );
    printf("    zeros' signs at exact points: %ld and %ld powers off, of "
           "%ld and %ld zero parts\n",
           failed[0],
           failed[1],
           zeros[0],
           zeros[1]);

    return;
}

// d_tests_whole_powers
//   the whole powers of every family, long double's where it holds the
// points' 64 bits; then their zeros' signs at exact points.
static void
d_tests_whole_powers(void)
{
    printf("  whole powers where a part cancels, _c then plain:\n");
    d_tests_check_powers(0, g_power_f, D_TESTS_COUNT(g_power_f));
    d_tests_check_powers(1, g_power_d, D_TESTS_COUNT(g_power_d));

    // a long double of double's width takes no point of 64 bits exactly
    if (LDBL_MANT_DIG >= 64)
    {
        d_tests_check_powers(2, g_power_ld, D_TESTS_COUNT(g_power_ld));
    }

    d_tests_power_zeros();

    return;
}


//==============================================================================
// 3.  ANNEX G
//==============================================================================
// Each entry is a function, its argument's parts and its result's, as
// letters. The argument's: 0 +0, o -0, 1 and N +-1, 2 2.5, m -2.5, I +inf, i
// -inf, n NaN. The result's: those, and z a zero of either sign, J an
// infinity of either sign, P and p +-pi, H and h +-pi/2, K pi/2 of either
// sign, Q and q +-pi/4, T and t +-3pi/4; and for log2 and log10 those over ln
// 2 and ln 10, A, B, C and D +pi, pi/2, pi/4 and 3pi/4 over ln 2, E, F, G
// and U over ln 10, their lower cases the negatives -- the constants within
// a step of the family's nearest. Each entry is checked as written and as
// each symmetry G.6 states makes it: f(conj z) = conj f(z) for all, f(-z) =
// -f(z) for the odd and f(-z) = f(z) for the even. The five C99 has only for
// real arguments take the values of the functions each is a form of: exp2
// cexp's, cis(2.5 ln 2) having cis(2.5)'s signs; log2 and log10 clog's, the
// angle over ln b; log1p clog's at 1 + z; expm1 cexp's less 1.

// dj_tests_special
//   struct: one of G.6's values.
struct dj_tests_special
{
    int         op;
    const char* in;   // the argument's parts
    const char* out;  // the result's
};

static const struct dj_tests_special g_specials[] =
{
    // G.6.1.1 cacos
    { D_TESTS_ACOS,  "00", "Ho" }, { D_TESTS_ACOS,  "o0", "Ho" },
    { D_TESTS_ACOS,  "0n", "Hn" }, { D_TESTS_ACOS,  "on", "Hn" },
    { D_TESTS_ACOS,  "0I", "Hi" }, { D_TESTS_ACOS,  "oI", "Hi" },
    { D_TESTS_ACOS,  "2I", "Hi" }, { D_TESTS_ACOS,  "mI", "Hi" },
    { D_TESTS_ACOS,  "2n", "nn" }, { D_TESTS_ACOS,  "mn", "nn" },
    { D_TESTS_ACOS,  "i0", "Pi" }, { D_TESTS_ACOS,  "i2", "Pi" },
    { D_TESTS_ACOS,  "I0", "0i" }, { D_TESTS_ACOS,  "I2", "0i" },
    { D_TESTS_ACOS,  "iI", "Ti" }, { D_TESTS_ACOS,  "II", "Qi" },
    { D_TESTS_ACOS,  "In", "nJ" }, { D_TESTS_ACOS,  "in", "nJ" },
    { D_TESTS_ACOS,  "n0", "nn" }, { D_TESTS_ACOS,  "n2", "nn" },
    { D_TESTS_ACOS,  "nI", "ni" }, { D_TESTS_ACOS,  "nn", "nn" },
    // G.6.2.1 cacosh, with C17's 0 + i NaN (DR 471)
    { D_TESTS_ACOSH, "00", "0H" }, { D_TESTS_ACOSH, "o0", "0H" },
    { D_TESTS_ACOSH, "0I", "IH" }, { D_TESTS_ACOSH, "oI", "IH" },
    { D_TESTS_ACOSH, "2I", "IH" }, { D_TESTS_ACOSH, "mI", "IH" },
    { D_TESTS_ACOSH, "0n", "nK" }, { D_TESTS_ACOSH, "on", "nK" },
    { D_TESTS_ACOSH, "2n", "nn" }, { D_TESTS_ACOSH, "mn", "nn" },
    { D_TESTS_ACOSH, "i0", "IP" }, { D_TESTS_ACOSH, "i2", "IP" },
    { D_TESTS_ACOSH, "I0", "I0" }, { D_TESTS_ACOSH, "I2", "I0" },
    { D_TESTS_ACOSH, "iI", "IT" }, { D_TESTS_ACOSH, "II", "IQ" },
    { D_TESTS_ACOSH, "In", "In" }, { D_TESTS_ACOSH, "in", "In" },
    { D_TESTS_ACOSH, "n0", "nn" }, { D_TESTS_ACOSH, "n2", "nn" },
    { D_TESTS_ACOSH, "nI", "In" }, { D_TESTS_ACOSH, "nn", "nn" },
    // G.6.2.2 casinh
    { D_TESTS_ASINH, "00", "00" }, { D_TESTS_ASINH, "0I", "IH" },
    { D_TESTS_ASINH, "2I", "IH" }, { D_TESTS_ASINH, "0n", "nn" },
    { D_TESTS_ASINH, "2n", "nn" }, { D_TESTS_ASINH, "I0", "I0" },
    { D_TESTS_ASINH, "I2", "I0" }, { D_TESTS_ASINH, "II", "IQ" },
    { D_TESTS_ASINH, "In", "In" }, { D_TESTS_ASINH, "n0", "n0" },
    { D_TESTS_ASINH, "n2", "nn" }, { D_TESTS_ASINH, "nI", "Jn" },
    { D_TESTS_ASINH, "nn", "nn" },
    // G.6.2.3 catanh
    { D_TESTS_ATANH, "00", "00" }, { D_TESTS_ATANH, "0n", "0n" },
    { D_TESTS_ATANH, "10", "I0" }, { D_TESTS_ATANH, "0I", "0H" },
    { D_TESTS_ATANH, "2I", "0H" }, { D_TESTS_ATANH, "2n", "nn" },
    { D_TESTS_ATANH, "I0", "0H" }, { D_TESTS_ATANH, "I2", "0H" },
    { D_TESTS_ATANH, "II", "0H" }, { D_TESTS_ATANH, "In", "0n" },
    { D_TESTS_ATANH, "n0", "nn" }, { D_TESTS_ATANH, "n2", "nn" },
    { D_TESTS_ATANH, "nI", "zH" }, { D_TESTS_ATANH, "nn", "nn" },
    // G.6.2.4 ccosh: cis(2.5) is (-, +)
    { D_TESTS_COSH,  "00", "10" }, { D_TESTS_COSH,  "0I", "nz" },
    { D_TESTS_COSH,  "0n", "nz" }, { D_TESTS_COSH,  "2I", "nn" },
    { D_TESTS_COSH,  "2n", "nn" }, { D_TESTS_COSH,  "I0", "I0" },
    { D_TESTS_COSH,  "I2", "iI" }, { D_TESTS_COSH,  "II", "Jn" },
    { D_TESTS_COSH,  "In", "In" }, { D_TESTS_COSH,  "n0", "nz" },
    { D_TESTS_COSH,  "n2", "nn" }, { D_TESTS_COSH,  "nI", "nn" },
    { D_TESTS_COSH,  "nn", "nn" },
    // G.6.2.5 csinh
    { D_TESTS_SINH,  "00", "00" }, { D_TESTS_SINH,  "0I", "zn" },
    { D_TESTS_SINH,  "0n", "zn" }, { D_TESTS_SINH,  "2I", "nn" },
    { D_TESTS_SINH,  "2n", "nn" }, { D_TESTS_SINH,  "I0", "I0" },
    { D_TESTS_SINH,  "I2", "iI" }, { D_TESTS_SINH,  "II", "Jn" },
    { D_TESTS_SINH,  "In", "Jn" }, { D_TESTS_SINH,  "n0", "n0" },
    { D_TESTS_SINH,  "n2", "nn" }, { D_TESTS_SINH,  "nI", "nn" },
    { D_TESTS_SINH,  "nn", "nn" },
    // G.6.2.6 ctanh, with C17's 0 + i inf and 0 + i NaN (DR 471): sin(5)
    // is negative
    { D_TESTS_TANH,  "00", "00" }, { D_TESTS_TANH,  "0I", "0n" },
    { D_TESTS_TANH,  "2I", "nn" }, { D_TESTS_TANH,  "0n", "0n" },
    { D_TESTS_TANH,  "2n", "nn" }, { D_TESTS_TANH,  "I0", "10" },
    { D_TESTS_TANH,  "I2", "1o" }, { D_TESTS_TANH,  "II", "1z" },
    { D_TESTS_TANH,  "In", "1z" }, { D_TESTS_TANH,  "n0", "n0" },
    { D_TESTS_TANH,  "n2", "nn" }, { D_TESTS_TANH,  "nI", "nn" },
    { D_TESTS_TANH,  "nn", "nn" },
    // G.6.3.1 cexp
    { D_TESTS_EXP,   "00", "10" }, { D_TESTS_EXP,   "o0", "10" },
    { D_TESTS_EXP,   "0I", "nn" }, { D_TESTS_EXP,   "2I", "nn" },
    { D_TESTS_EXP,   "mI", "nn" }, { D_TESTS_EXP,   "0n", "nn" },
    { D_TESTS_EXP,   "2n", "nn" }, { D_TESTS_EXP,   "mn", "nn" },
    { D_TESTS_EXP,   "I0", "I0" }, { D_TESTS_EXP,   "i0", "00" },
    { D_TESTS_EXP,   "i2", "o0" }, { D_TESTS_EXP,   "I2", "iI" },
    { D_TESTS_EXP,   "iI", "zz" }, { D_TESTS_EXP,   "II", "Jn" },
    { D_TESTS_EXP,   "in", "zz" }, { D_TESTS_EXP,   "In", "Jn" },
    { D_TESTS_EXP,   "n0", "n0" }, { D_TESTS_EXP,   "n2", "nn" },
    { D_TESTS_EXP,   "nI", "nn" }, { D_TESTS_EXP,   "nn", "nn" },
    // G.6.3.2 clog
    { D_TESTS_LOG,   "o0", "iP" }, { D_TESTS_LOG,   "00", "i0" },
    { D_TESTS_LOG,   "0I", "IH" }, { D_TESTS_LOG,   "oI", "IH" },
    { D_TESTS_LOG,   "2I", "IH" }, { D_TESTS_LOG,   "mI", "IH" },
    { D_TESTS_LOG,   "0n", "nn" }, { D_TESTS_LOG,   "on", "nn" },
    { D_TESTS_LOG,   "2n", "nn" }, { D_TESTS_LOG,   "mn", "nn" },
    { D_TESTS_LOG,   "i0", "IP" }, { D_TESTS_LOG,   "i2", "IP" },
    { D_TESTS_LOG,   "I0", "I0" }, { D_TESTS_LOG,   "I2", "I0" },
    { D_TESTS_LOG,   "iI", "IT" }, { D_TESTS_LOG,   "II", "IQ" },
    { D_TESTS_LOG,   "In", "In" }, { D_TESTS_LOG,   "in", "In" },
    { D_TESTS_LOG,   "n0", "nn" }, { D_TESTS_LOG,   "n2", "nn" },
    { D_TESTS_LOG,   "nI", "In" }, { D_TESTS_LOG,   "nn", "nn" },
    // G.6.4.2 csqrt
    { D_TESTS_SQRT,  "00", "00" }, { D_TESTS_SQRT,  "o0", "00" },
    { D_TESTS_SQRT,  "0I", "II" }, { D_TESTS_SQRT,  "oI", "II" },
    { D_TESTS_SQRT,  "2I", "II" }, { D_TESTS_SQRT,  "mI", "II" },
    { D_TESTS_SQRT,  "II", "II" }, { D_TESTS_SQRT,  "iI", "II" },
    { D_TESTS_SQRT,  "nI", "II" }, { D_TESTS_SQRT,  "0n", "nn" },
    { D_TESTS_SQRT,  "on", "nn" }, { D_TESTS_SQRT,  "2n", "nn" },
    { D_TESTS_SQRT,  "mn", "nn" }, { D_TESTS_SQRT,  "i0", "0I" },
    { D_TESTS_SQRT,  "i2", "0I" }, { D_TESTS_SQRT,  "I0", "I0" },
    { D_TESTS_SQRT,  "I2", "I0" }, { D_TESTS_SQRT,  "in", "nJ" },
    { D_TESTS_SQRT,  "In", "In" }, { D_TESTS_SQRT,  "n0", "nn" },
    { D_TESTS_SQRT,  "n2", "nn" }, { D_TESTS_SQRT,  "nn", "nn" },
    // exp2: cexp's
    { D_TESTS_EXP2,  "00", "10" }, { D_TESTS_EXP2,  "o0", "10" },
    { D_TESTS_EXP2,  "0I", "nn" }, { D_TESTS_EXP2,  "2I", "nn" },
    { D_TESTS_EXP2,  "mI", "nn" }, { D_TESTS_EXP2,  "0n", "nn" },
    { D_TESTS_EXP2,  "2n", "nn" }, { D_TESTS_EXP2,  "mn", "nn" },
    { D_TESTS_EXP2,  "I0", "I0" }, { D_TESTS_EXP2,  "i0", "00" },
    { D_TESTS_EXP2,  "i2", "o0" }, { D_TESTS_EXP2,  "I2", "iI" },
    { D_TESTS_EXP2,  "iI", "zz" }, { D_TESTS_EXP2,  "II", "Jn" },
    { D_TESTS_EXP2,  "in", "zz" }, { D_TESTS_EXP2,  "In", "Jn" },
    { D_TESTS_EXP2,  "n0", "n0" }, { D_TESTS_EXP2,  "n2", "nn" },
    { D_TESTS_EXP2,  "nI", "nn" }, { D_TESTS_EXP2,  "nn", "nn" },
    // expm1: cexp's less 1, expm1(-0) -0
    { D_TESTS_EXPM1, "00", "00" }, { D_TESTS_EXPM1, "o0", "o0" },
    { D_TESTS_EXPM1, "0I", "nn" }, { D_TESTS_EXPM1, "2I", "nn" },
    { D_TESTS_EXPM1, "mI", "nn" }, { D_TESTS_EXPM1, "0n", "nn" },
    { D_TESTS_EXPM1, "2n", "nn" }, { D_TESTS_EXPM1, "mn", "nn" },
    { D_TESTS_EXPM1, "I0", "I0" }, { D_TESTS_EXPM1, "i0", "N0" },
    { D_TESTS_EXPM1, "i2", "N0" }, { D_TESTS_EXPM1, "I2", "iI" },
    { D_TESTS_EXPM1, "iI", "Nz" }, { D_TESTS_EXPM1, "II", "Jn" },
    { D_TESTS_EXPM1, "in", "Nz" }, { D_TESTS_EXPM1, "In", "Jn" },
    { D_TESTS_EXPM1, "n0", "n0" }, { D_TESTS_EXPM1, "n2", "nn" },
    { D_TESTS_EXPM1, "nI", "nn" }, { D_TESTS_EXPM1, "nn", "nn" },
    // log2: clog's, the angle over ln 2
    { D_TESTS_LOG2,  "o0", "iA" }, { D_TESTS_LOG2,  "00", "i0" },
    { D_TESTS_LOG2,  "0I", "IB" }, { D_TESTS_LOG2,  "oI", "IB" },
    { D_TESTS_LOG2,  "2I", "IB" }, { D_TESTS_LOG2,  "mI", "IB" },
    { D_TESTS_LOG2,  "0n", "nn" }, { D_TESTS_LOG2,  "on", "nn" },
    { D_TESTS_LOG2,  "2n", "nn" }, { D_TESTS_LOG2,  "mn", "nn" },
    { D_TESTS_LOG2,  "i0", "IA" }, { D_TESTS_LOG2,  "i2", "IA" },
    { D_TESTS_LOG2,  "I0", "I0" }, { D_TESTS_LOG2,  "I2", "I0" },
    { D_TESTS_LOG2,  "iI", "ID" }, { D_TESTS_LOG2,  "II", "IC" },
    { D_TESTS_LOG2,  "In", "In" }, { D_TESTS_LOG2,  "in", "In" },
    { D_TESTS_LOG2,  "n0", "nn" }, { D_TESTS_LOG2,  "n2", "nn" },
    { D_TESTS_LOG2,  "nI", "In" }, { D_TESTS_LOG2,  "nn", "nn" },
    // log10: clog's, the angle over ln 10
    { D_TESTS_LOG10, "o0", "iE" }, { D_TESTS_LOG10, "00", "i0" },
    { D_TESTS_LOG10, "0I", "IF" }, { D_TESTS_LOG10, "oI", "IF" },
    { D_TESTS_LOG10, "2I", "IF" }, { D_TESTS_LOG10, "mI", "IF" },
    { D_TESTS_LOG10, "0n", "nn" }, { D_TESTS_LOG10, "on", "nn" },
    { D_TESTS_LOG10, "2n", "nn" }, { D_TESTS_LOG10, "mn", "nn" },
    { D_TESTS_LOG10, "i0", "IE" }, { D_TESTS_LOG10, "i2", "IE" },
    { D_TESTS_LOG10, "I0", "I0" }, { D_TESTS_LOG10, "I2", "I0" },
    { D_TESTS_LOG10, "iI", "IU" }, { D_TESTS_LOG10, "II", "IG" },
    { D_TESTS_LOG10, "In", "In" }, { D_TESTS_LOG10, "in", "In" },
    { D_TESTS_LOG10, "n0", "nn" }, { D_TESTS_LOG10, "n2", "nn" },
    { D_TESTS_LOG10, "nI", "In" }, { D_TESTS_LOG10, "nn", "nn" },
    // log1p: clog's at 1 + z, and -1 the logarithm of a zero
    { D_TESTS_LOG1P, "00", "00" }, { D_TESTS_LOG1P, "o0", "o0" },
    { D_TESTS_LOG1P, "N0", "i0" }, { D_TESTS_LOG1P, "0I", "IH" },
    { D_TESTS_LOG1P, "oI", "IH" }, { D_TESTS_LOG1P, "2I", "IH" },
    { D_TESTS_LOG1P, "mI", "IH" }, { D_TESTS_LOG1P, "0n", "nn" },
    { D_TESTS_LOG1P, "2n", "nn" }, { D_TESTS_LOG1P, "mn", "nn" },
    { D_TESTS_LOG1P, "i0", "IP" }, { D_TESTS_LOG1P, "i2", "IP" },
    { D_TESTS_LOG1P, "I0", "I0" }, { D_TESTS_LOG1P, "I2", "I0" },
    { D_TESTS_LOG1P, "iI", "IT" }, { D_TESTS_LOG1P, "II", "IQ" },
    { D_TESTS_LOG1P, "In", "In" }, { D_TESTS_LOG1P, "in", "In" },
    { D_TESTS_LOG1P, "n0", "nn" }, { D_TESTS_LOG1P, "n2", "nn" },
    { D_TESTS_LOG1P, "nI", "In" }, { D_TESTS_LOG1P, "nn", "nn" }
};

// d_tests_part
//   an argument's part from its letter.
static long double
d_tests_part(
    char _code
)
{
    const long double inf = d_internal_math_ld_infinity();

    switch (_code)
    {
    case 'o':
        return -0.0L;
    case '1':
        return 1.0L;
    case 'N':
        return -1.0L;
    case '2':
        return 2.5L;
    case 'm':
        return -2.5L;
    case 'I':
        return inf;
    case 'i':
        return -inf;
    case 'n':
        return d_internal_math_ld_nan();
    default:
        return 0.0L;
    }
}

// d_tests_negated
//   the letter of a part's negation.
static char
d_tests_negated(
    char _code
)
{
    static const char from[] = "0o12mIiNPpHhQqTtAaBbCcDdEeFfGgUu";
    static const char to[]   = "o0Nm2iI1pPhHqQtTaAbBcCdDeEfFgGuU";
    size_t            k      = 0u;

    for (k = 0u; from[k] != '\0'; ++k)
    {
        if (from[k] == _code)
        {
            return to[k];
        }
    }

    // n, z, J and K are their own
    return _code;
}

// d_tests_constant
//   whether _got is the constant of the letter _code, within a step of the
// family's nearest; _sign_free, either sign.
static bool
d_tests_constant(
    int         _family,
    long double _got,
    char        _code,
    bool        _sign_free
)
{
    const char* digits = "3.14159265358979323846264338327950288419716940";
    const char* half   = "1.57079632679489661923132169163975144209858470";
    const char* fourth = "0.785398163397448309615660845819875721049292350";
    const char* three  = "2.35619449019234492884698253745962716314787705";
    // the four over ln 2, A to D, and over ln 10, E, F, G and U
    const char* const over[8] =
    {
        "4.53236014182719380962768294571666681017186147",
        "2.26618007091359690481384147285833340508593073",
        "1.13309003545679845240692073642916670254296537",
        "3.39927010637039535722076220928750010762889610",
        "1.36437635384184134748578362543135577021012748",
        "0.682188176920920673742891812715677885105063742",
        "0.341094088460460336871445906357838942552531871",
        "1.02328226538138101061433771907351682765759561"
    };
    static const char over_codes[] = "ABCDEFGUabcdefgu";
    const char* text   = digits;
    bool        minus  = false;
    long double value  = 0.0L;
    long double got    = _got;
    size_t      k      = 0u;

    // the constants over ln 2 and ln 10, their lower cases negative
    for (k = 0u; over_codes[k] != '\0'; ++k)
    {
        if (over_codes[k] == _code)
        {
            text  = over[k % 8u];
            minus = (k >= 8u);
        }
    }

    switch (_code)
    {
    case 'p':
        minus = true;
        break;
    case 'H':
    case 'K':
        text = half;
        break;
    case 'h':
        text  = half;
        minus = true;
        break;
    case 'Q':
        text = fourth;
        break;
    case 'q':
        text  = fourth;
        minus = true;
        break;
    case 'T':
        text = three;
        break;
    case 't':
        text  = three;
        minus = true;
        break;
    default:
        break;
    }

    value = d_tests_text(_family, text);

    if (_sign_free)
    {
        got = (got < 0.0L) ? -got : got;
    }
    else if (minus)
    {
        value = -value;
    }

    return (d_tests_steps(_family, got, value) <= 1ul);
}

// d_tests_matches
//   whether a result's part _got is what the letter _code names.
static bool
d_tests_matches(
    int         _family,
    long double _got,
    char        _code
)
{
    const long double inf = d_internal_math_ld_infinity();

    switch (_code)
    {
    case 'n':
        return (_got != _got);
    case '0':
        return ( (_got == 0.0L) &&
                 (!d_internal_math_ld_negative(_got)) );
    case 'o':
        return ( (_got == 0.0L) &&
                 (d_internal_math_ld_negative(_got)) );
    case 'z':
        return (_got == 0.0L);
    case 'I':
        return (_got == inf);
    case 'i':
        return (_got == -inf);
    case 'J':
        return ( (_got == inf) ||
                 (_got == -inf) );
    case '1':
        return (_got == 1.0L);
    case 'N':
        return (_got == -1.0L);
    case 'K':
        return d_tests_constant(_family, _got, _code, true);
    default:
        return d_tests_constant(_family, _got, _code, false);
    }
}

// d_tests_special_one
//   one entry, as given: every family, both forms.
static void
d_tests_special_one(
    int         _op,
    const char* _in,
    const char* _out
)
{
    long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    int         family = 0;

    in[0] = d_tests_part(_in[0]);
    in[1] = d_tests_part(_in[1]);

    for (family = 0; family < 3; ++family)
    {
        int form = 0;

        for (form = 0; form < 2; ++form)
        {
            const struct dj_tests_result got =
                d_tests_call(family, _op, (form == 0), in);
            const bool ok = ( (d_tests_matches(family, got.part[0],
                                               _out[0])) &&
                              (d_tests_matches(family, got.part[1],
                                               _out[1])) );

            D_TESTS_EXPECT(ok, "Annex G's value");

            if ( (!ok) &&
                 (g_failures <= D_TESTS_SHOW) )
            {
                printf("  c%s(%c, %c) %s%s: (%Lg, %Lg), want %c %c\n",
                       g_op_names[_op],
                       _in[0],
                       _in[1],
                       g_family_names[family],
                       (form == 0) ? " _c" : "",
                       got.part[0],
                       got.part[1],
                       _out[0],
                       _out[1]);
            }
        }
    }

    return;
}

// d_tests_annex_g
//   every entry, as written and by its symmetries.
static void
d_tests_annex_g(void)
{
    size_t k = 0u;

    for (k = 0u; k < D_TESTS_COUNT(g_specials); ++k)
    {
        const struct dj_tests_special* s = &g_specials[k];
        const bool odd  = ( (s->op == D_TESTS_ASINH) ||
                            (s->op == D_TESTS_ATANH) ||
                            (s->op == D_TESTS_SINH) ||
                            (s->op == D_TESTS_TANH) );
        const bool even = (s->op == D_TESTS_COSH);
        char       in[3]  = { '\0', '\0', '\0' };
        char       out[3] = { '\0', '\0', '\0' };

        d_tests_special_one(s->op, s->in, s->out);

        // f(conj z) = conj f(z)
        in[0]  = s->in[0];
        in[1]  = d_tests_negated(s->in[1]);
        out[0] = s->out[0];
        out[1] = d_tests_negated(s->out[1]);
        d_tests_special_one(s->op, in, out);

        if ( (odd) ||
             (even) )
        {
            // f(-z) = -f(z), or f(z); and of conj z
            in[0]  = d_tests_negated(s->in[0]);
            in[1]  = d_tests_negated(s->in[1]);
            out[0] = odd ? d_tests_negated(s->out[0]) : s->out[0];
            out[1] = odd ? d_tests_negated(s->out[1]) : s->out[1];
            d_tests_special_one(s->op, in, out);

            in[1]  = s->in[1];
            out[1] = odd ? s->out[1] : d_tests_negated(s->out[1]);
            d_tests_special_one(s->op, in, out);
        }
    }

    return;
}

// d_tests_alike
//   d_tests_same, or where _loose, the same but for the sign of a zero or an
// infinity; and finite values within _steps of the family's of each other,
// which only a plain form evaluated wider than its type is allowed (below).
static bool
d_tests_alike(
    int           _family,
    long double   _a,
    long double   _b,
    bool          _loose,
    unsigned long _steps
)
{
    const long double inf = d_internal_math_ld_infinity();

    if ( (_loose) &&
         ( (_a == 0.0L) ||
           (_a == inf) ||
           (_a == -inf) ) )
    {
        return ( (_a == _b) ||
                 (_a == -_b) );
    }

    if ( (_steps > 0ul) &&
         (_a != 0.0L) &&
         (_a == _a) &&
         (_a != inf) &&
         (_a != -inf) )
    {
        return (d_tests_steps(_family, _a, _b) <= _steps);
    }

    return d_tests_same(_a, _b);
}

// d_tests_wide
//   whether the family's plain forms are evaluated wider than its type, as
// float and double are on a 32-bit x87 (math_common.h's section 4): there a
// result the C library returns, or a sum held in a register, keeps the
// wider type's bits as the compiler places it, so that the same algorithm,
// inlined twice, may round differently -- its bound holds, not its bits.
static bool
d_tests_wide(
    int _family
)
{
    return ( ( (_family == 0) &&
               (D_MATH_FLT_EVAL_WIDE) ) ||
             ( (_family == 1) &&
               (D_MATH_DBL_EVAL_WIDE) ) );
}

// d_tests_definitions
//   G.6's functions by others, bit for bit: casin z = -i casinh(iz), catan
// z = -i catanh(iz), ccos z = ccosh(iz), csin z = -i csinh(iz), ctan z = -i
// ctanh(iz), at every pair of the special parts and of some finite ones --
// a plain form evaluated wider than its type within its bound (d_tests_wide).
static void
d_tests_definitions(void)
{
    static const char codes[] = "0o12mIin";
    const long double finite[6] = { 0.75L, -1.25L, 3.0517578125e-05L,
                                    20.0L, -700.0L, 0.5L };
    const int         ops[5]      = { D_TESTS_ASIN, D_TESTS_ATAN,
                                      D_TESTS_COS, D_TESTS_SIN,
                                      D_TESTS_TAN };
    const int         partners[5] = { D_TESTS_ASINH, D_TESTS_ATANH,
                                      D_TESTS_COSH, D_TESTS_SINH,
                                      D_TESTS_TANH };
    long double       parts[14];
    size_t            a = 0u;
    size_t            b = 0u;
    int               k = 0;

    for (a = 0u; a < 8u; ++a)
    {
        parts[a] = d_tests_part(codes[a]);
    }

    for (a = 0u; a < 6u; ++a)
    {
        parts[8u + a] = finite[a];
    }

    for (a = 0u; a < 14u; ++a)
    {
        for (b = 0u; b < 14u; ++b)
        {
            for (k = 0; k < 5; ++k)
            {
                int family = 0;

                for (family = 0; family < 3; ++family)
                {
                    int form = 0;

                    for (form = 0; form < 2; ++form)
                    {
                        const bool        pure  = (form == 0);
                        long double       in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
                        long double       iz[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
                        struct dj_tests_result f;
                        struct dj_tests_result h;
                        bool              ok    = false;

                        in[0] = parts[a];
                        in[1] = parts[b];
                        // i z = -y + i x
                        iz[0] = -parts[b];
                        iz[1] = parts[a];
                        f     = d_tests_call(family, ops[k], pure, in);
                        h     = d_tests_call(family, partners[k], pure, iz);

                        // -i w = w.im - i w.re; ccos's is w itself. Where
                        // a NaN is among the parts, G.6 leaves the signs of
                        // the zeros and infinities beside it free
                        const bool loose =
                            ( (in[0] != in[0]) ||
                              (in[1] != in[1]) ||
                              (h.part[0] != h.part[0]) ||
                              (h.part[1] != h.part[1]) );

                        const unsigned long slack =
                            ( (!pure) &&
                              (d_tests_wide(family)) )
                                ? d_tests_bound(ops[k], family, false)
                                : 0ul;

                        if (ops[k] == D_TESTS_COS)
                        {
                            ok = ( (d_tests_alike(family, f.part[0],
                                                  h.part[0], loose, slack)) &&
                                   (d_tests_alike(family, f.part[1],
                                                  h.part[1], loose, slack)) );
                        }
                        else
                        {
                            ok = ( (d_tests_alike(family, f.part[0],
                                                  h.part[1], loose, slack)) &&
                                   (d_tests_alike(family, f.part[1],
                                                  -h.part[0], loose,
                                                  slack)) );
                        }

                        D_TESTS_EXPECT(ok, "G.6's definition by another");

                        if ( (!ok) &&
                             (g_failures <= D_TESTS_SHOW) )
                        {
                            printf("  c%s(%Lg, %Lg) %s%s: (%Lg, %Lg) by"
                                   " c%s's (%Lg, %Lg)\n",
                                   g_op_names[ops[k]],
                                   in[0],
                                   in[1],
                                   g_family_names[family],
                                   pure ? " _c" : "",
                                   f.part[0],
                                   f.part[1],
                                   g_op_names[partners[k]],
                                   h.part[0],
                                   h.part[1]);
                        }
                    }
                }
            }
        }
    }

    return;
}


//==============================================================================
// 4.  EXACT VALUES
//==============================================================================
// Each entry an operation, its parts, and its result's: each part within
// the given steps of the value written, its sign that value's, a zero's
// among them; NaN for a NaN.

// dj_tests_exact
//   struct: one exact value.
struct dj_tests_exact
{
    int         op;
    long double in[4];
    long double out[2];
    unsigned    steps;
    bool        signs;  // whether a zero part's sign is checked too
};

static const struct dj_tests_exact g_exact[] =
{
    // the square root's cut, its side the zero's sign: sqrt(-4 -+ 0i)
    { D_TESTS_SQRT, { -4.0L, -0.0L, 0.0L, 0.0L }, { 0.0L, -2.0L }, 0u, true },
    { D_TESTS_SQRT, { -4.0L, 0.0L, 0.0L, 0.0L },  { 0.0L, 2.0L },  0u, true },
    { D_TESTS_SQRT, { 0.0L, 2.0L, 0.0L, 0.0L },   { 1.0L, 1.0L },  0u, true },
    { D_TESTS_SQRT, { 3.0L, -4.0L, 0.0L, 0.0L },  { 2.0L, -1.0L }, 0u, true },
    { D_TESTS_SQRT, { -3.0L, 4.0L, 0.0L, 0.0L },  { 1.0L, 2.0L },  0u, true },
    // the logarithm's: log(-1 -+ 0i) = 0 -+ pi i, log 1 = 0
    { D_TESTS_LOG,  { 1.0L, 0.0L, 0.0L, 0.0L },   { 0.0L, 0.0L },  0u, true },
    { D_TESTS_LOG,  { 1.0L, -0.0L, 0.0L, 0.0L },  { 0.0L, -0.0L }, 0u, true },
    { D_TESTS_LOG,  { 0.0L, 1.0L, 0.0L, 0.0L },
                    { 0.0L, 1.57079632679489661923132169163975144L },
                    1u, true },
    { D_TESTS_LOG,  { -1.0L, 0.0L, 0.0L, 0.0L },
                    { 0.0L, 3.14159265358979323846264338327950288L },
                    1u, true },
    { D_TESTS_LOG,  { -1.0L, -0.0L, 0.0L, 0.0L },
                    { 0.0L, -3.14159265358979323846264338327950288L },
                    1u, true },
    // the zeros of the functions, kept
    { D_TESTS_SIN,  { 0.0L, -0.0L, 0.0L, 0.0L },  { 0.0L, -0.0L }, 0u, true },
    { D_TESTS_TAN,  { -0.0L, 0.0L, 0.0L, 0.0L },  { -0.0L, 0.0L }, 0u, true },
    { D_TESTS_ATAN, { 0.0L, -0.0L, 0.0L, 0.0L },  { 0.0L, -0.0L }, 0u, true },
    { D_TESTS_ASIN, { -0.0L, -0.0L, 0.0L, 0.0L }, { -0.0L, -0.0L }, 0u, true },
    { D_TESTS_COS,  { 0.0L, 0.0L, 0.0L, 0.0L },   { 1.0L, -0.0L }, 0u, true },
    { D_TESTS_EXP,  { 0.0L, -0.0L, 0.0L, 0.0L },  { 1.0L, -0.0L }, 0u, true },
    // the powers: z^0 = 1 for every z, NaNs too; whole powers by squaring,
    // each zero the products' and the quotient's (S382); a negative or
    // imaginary base to a real power by turns
    { D_TESTS_POW,  { 2.5L, -3.0L, 0.0L, 0.0L },  { 1.0L, 0.0L },  0u, true },
    { D_TESTS_POW,  { 1.0L, 1.0L, 10.0L, 0.0L },  { 0.0L, 32.0L }, 0u, true },
    { D_TESTS_POW,  { 1.0L, 1.0L, -2.0L, 0.0L },  { 0.0L, -0.5L }, 0u, true },
    { D_TESTS_POW,  { 0.0L, 1.0L, 3.0L, 0.0L },   { -0.0L, -1.0L }, 0u, true },
    { D_TESTS_POW,  { -2.0L, 0.0L, -1.0L, 0.0L }, { -0.5L, -0.0L }, 0u, true },
    { D_TESTS_POW,  { 0.0L, 2.0L, -3.0L, 0.0L },  { -0.0L, 0.125L }, 0u, true },
    { D_TESTS_POW,  { 3.0L, 4.0L, 2.0L, 0.0L },   { -7.0L, 24.0L }, 0u, true },
    { D_TESTS_POW,  { -4.0L, 0.0L, 0.5L, 0.0L },  { 0.0L, 2.0L },  0u, true },
    { D_TESTS_POW,  { -4.0L, -0.0L, 0.5L, 0.0L }, { 0.0L, -2.0L }, 0u, true },
    { D_TESTS_POW,  { -8.0L, 0.0L, 0.5L, 0.0L },
                    { 0.0L, 2.82842712474619009760337744841939616L },
                    1u, true },
    { D_TESTS_POW,  { 0.0L, 4.0L, 0.5L, 0.0L },
                    { 1.41421356237309504880168872420969808L,
                      1.41421356237309504880168872420969808L }, 1u, true },
    { D_TESTS_POW,  { 4.0L, 0.0L, 0.5L, 0.0L },   { 2.0L, 0.0L },  0u, true },
    { D_TESTS_POW,  { 0.0L, 0.0L, 2.0L, 3.0L },   { 0.0L, 0.0L },  0u, true },
    { D_TESTS_POW,  { 0.0L, 1.0L, 0.0L, 1.0L },
                    { 0.207879576350761908546955619834978770L, 0.0L },
                    1u, true },
    // the modulus, exact (past the squares' overflow and in the subnormals
    // in d_tests_infinities)
    { D_TESTS_ABS,  { 3.0L, -4.0L, 0.0L, 0.0L },  { 5.0L, 0.0L },  0u, true },
    { D_TESTS_ABS,  { -0.0L, -0.0L, 0.0L, 0.0L }, { 0.0L, 0.0L },  0u, true },
    // the argument's quadrants and its zeros' signs
    { D_TESTS_ARG,  { -1.0L, 0.0L, 0.0L, 0.0L },
                    { 3.14159265358979323846264338327950288L, 0.0L },
                    1u, true },
    { D_TESTS_ARG,  { -1.0L, -0.0L, 0.0L, 0.0L },
                    { -3.14159265358979323846264338327950288L, 0.0L },
                    1u, true },
    { D_TESTS_ARG,  { -0.0L, 0.0L, 0.0L, 0.0L },
                    { 3.14159265358979323846264338327950288L, 0.0L },
                    1u, true },
    { D_TESTS_ARG,  { 0.0L, -0.0L, 0.0L, 0.0L },  { -0.0L, 0.0L }, 0u, true },
    { D_TESTS_ARG,  { 0.0L, 0.0L, 0.0L, 0.0L },   { 0.0L, 0.0L },  0u, true },
    { D_TESTS_ARG,  { 1.0L, 1.0L, 0.0L, 0.0L },
                    { 0.785398163397448309615660845819875721L, 0.0L },
                    1u, true },
    // the arithmetic: exact where the parts are, a real operand's zero kept
    { D_TESTS_MUL,  { 1.0L, 2.0L, 3.0L, -1.0L },  { 5.0L, 5.0L },  0u, true },
    { D_TESTS_MUL,  { 1.0L, 0.0L, 1.0L, -0.0L },  { 1.0L, 0.0L },  0u, true },
    { D_TESTS_DIV,  { 1.0L, 2.0L, 3.0L, 4.0L },
                    { 0.44L, 0.08L }, 1u, true },
    { D_TESTS_DIV,  { 5.0L, 5.0L, 3.0L, -1.0L },  { 1.0L, 2.0L },  0u, true },
    // a quotient's zero the formula's, of its sign (S382)
    { D_TESTS_DIV,  { 1.0L, 0.0L, -2.0L, 0.0L },  { -0.5L, -0.0L }, 0u, true },
    { D_TESTS_DIV,  { 1.0L, 0.0L, -2.0L, -0.0L }, { -0.5L, 0.0L },  0u, true },
    { D_TESTS_DIV,  { 1.0L, -0.0L, 2.0L, 0.0L },  { 0.5L, -0.0L },  0u, true },
    { D_TESTS_DIV,  { 0.0L, -1.0L, 0.0L, 2.0L },  { -0.5L, -0.0L }, 0u, true },
    { D_TESTS_ADD,  { 1.0L, -0.0L, 2.0L, -0.0L }, { 3.0L, -0.0L }, 0u, true },
    { D_TESTS_SUB,  { 1.0L, 0.0L, 2.0L, 0.0L },   { -1.0L, 0.0L }, 0u, true },
    { D_TESTS_ADD_REAL, { 1.0L, -0.0L, 2.0L, 0.0L },
                        { 3.0L, -0.0L }, 0u, true },
    { D_TESTS_SUB_REAL, { 1.0L, -0.0L, 2.0L, 0.0L },
                        { -1.0L, -0.0L }, 0u, true },
    { D_TESTS_REAL_SUB, { 1.0L, 0.0L, 2.0L, 0.0L },
                        { 1.0L, -0.0L }, 0u, true },
    { D_TESTS_MUL_REAL, { 3.0L, -0.0L, 2.0L, 0.0L },
                        { 6.0L, -0.0L }, 0u, true },
    { D_TESTS_DIV_REAL, { 6.0L, 4.0L, 2.0L, 0.0L },
                        { 3.0L, 2.0L }, 0u, true },
    { D_TESTS_REAL_DIV, { 0.0L, 1.0L, 1.0L, 0.0L },
                        { 0.0L, -1.0L }, 0u, true },
    { D_TESTS_REAL_DIV, { 3.0L, 4.0L, 25.0L, 0.0L },
                        { 3.0L, -4.0L }, 0u, true },
    { D_TESTS_REAL_DIV, { -2.0L, 0.0L, 1.0L, 0.0L },
                        { -0.5L, -0.0L }, 0u, true },
    { D_TESTS_CONJ, { 1.0L, 0.0L, 0.0L, 0.0L },   { 1.0L, -0.0L }, 0u, true },
    { D_TESTS_NEG,  { 0.0L, -0.0L, 0.0L, 0.0L },  { -0.0L, 0.0L }, 0u, true },
    { D_TESTS_PROJ, { 2.0L, 3.0L, 0.0L, 0.0L },   { 2.0L, 3.0L },  0u, true },
    // the five: a power of the base its exponent, exactly, on either axis
    // and on the cut, the angle over ln b within a step
    { D_TESTS_LOG2,  { 8.0L, -0.0L, 0.0L, 0.0L },  { 3.0L, -0.0L }, 0u, true },
    { D_TESTS_LOG2,  { -8.0L, 0.0L, 0.0L, 0.0L },
                     { 3.0L, 4.53236014182719380962768294571666681L },
                     1u, true },
    { D_TESTS_LOG2,  { -8.0L, -0.0L, 0.0L, 0.0L },
                     { 3.0L, -4.53236014182719380962768294571666681L },
                     1u, true },
    { D_TESTS_LOG2,  { 0.0L, 0.25L, 0.0L, 0.0L },
                     { -2.0L, 2.26618007091359690481384147285833341L },
                     1u, true },
    { D_TESTS_LOG2,  { 1.0L, 1.0L, 0.0L, 0.0L },
                     { 0.5L, 1.13309003545679845240692073642916670L },
                     1u, true },
    { D_TESTS_LOG10, { -1000.0L, 0.0L, 0.0L, 0.0L },
                     { 3.0L, 1.36437635384184134748578362543135577L },
                     1u, true },
    { D_TESTS_LOG10, { 0.0L, -100.0L, 0.0L, 0.0L },
                     { 2.0L, -0.682188176920920673742891812715677885L },
                     1u, true },
    { D_TESTS_LOG10, { 1.0L, 0.0L, 0.0L, 0.0L },   { 0.0L, 0.0L },  0u, true },
    { D_TESTS_EXP2,  { 3.0L, 0.0L, 0.0L, 0.0L },   { 8.0L, 0.0L },  0u, true },
    { D_TESTS_EXP2,  { -1.0L, -0.0L, 0.0L, 0.0L }, { 0.5L, -0.0L }, 0u, true },
    { D_TESTS_LOG1P, { -0.0L, -0.0L, 0.0L, 0.0L }, { -0.0L, -0.0L }, 0u, true },
    { D_TESTS_LOG1P, { -2.0L, 0.0L, 0.0L, 0.0L },
                     { 0.0L, 3.14159265358979323846264338327950288L },
                     1u, true },
    { D_TESTS_LOG1P, { -2.0L, -0.0L, 0.0L, 0.0L },
                     { 0.0L, -3.14159265358979323846264338327950288L },
                     1u, true },
    { D_TESTS_LOG1P, { -1.0L, 1.0L, 0.0L, 0.0L },
                     { 0.0L, 1.57079632679489661923132169163975144L },
                     1u, true },
    { D_TESTS_LOG1P, { -1.0L, -2.0L, 0.0L, 0.0L },
                     { 0.693147180559945309417232121458176568L,
                       -1.57079632679489661923132169163975144L }, 1u, true },
    { D_TESTS_EXPM1, { 0.0L, -0.0L, 0.0L, 0.0L },  { 0.0L, -0.0L }, 0u, true },
    { D_TESTS_EXPM1, { -0.0L, 0.0L, 0.0L, 0.0L },  { -0.0L, 0.0L }, 0u, true }
};

// d_tests_exact_values
//   every entry, every family, both forms where there are two.
static void
d_tests_exact_values(void)
{
    size_t k = 0u;

    for (k = 0u; k < D_TESTS_COUNT(g_exact); ++k)
    {
        const struct dj_tests_exact* e = &g_exact[k];
        int                          family = 0;

        for (family = 0; family < 3; ++family)
        {
            int form = 0;

            for (form = 0; form < 2; ++form)
            {
                // each value rounded to the family, by way of its text's
                // nearest: the values above are exact in float, or the
                // nearest long double to a constant, rounded once more
                long double in[4];
                long double want[2];
                struct dj_tests_result got;
                bool        ok = true;
                int         j  = 0;

                for (j = 0; j < 4; ++j)
                {
                    in[j] = e->in[j];
                }

                for (j = 0; j < 2; ++j)
                {
                    want[j] = (family == 0) ? (long double)(float)e->out[j]
                            : (family == 1) ? (long double)(double)e->out[j]
                                            : e->out[j];
                }

                got = d_tests_call(family, e->op, (form == 0), in);

                for (j = 0; j < 2; ++j)
                {
                    const unsigned long steps =
                        d_tests_steps(family, got.part[j], want[j]);

                    if ( (steps > (unsigned long)e->steps) ||
                         ( (e->signs) &&
                           (want[j] == 0.0L) &&
                           (d_internal_math_ld_negative(got.part[j]) !=
                            d_internal_math_ld_negative(want[j])) ) )
                    {
                        ok = false;
                    }
                }

                D_TESTS_EXPECT(ok, "an exact value");

                if ( (!ok) &&
                     (g_failures <= D_TESTS_SHOW) )
                {
                    printf("  %s(%Lg, %Lg; %Lg, %Lg) %s%s: (%.21Lg, %.21Lg),"
                           " want (%.21Lg, %.21Lg)\n",
                           g_op_names[e->op],
                           in[0],
                           in[1],
                           in[2],
                           in[3],
                           g_family_names[family],
                           (form == 0) ? " _c" : "",
                           got.part[0],
                           got.part[1],
                           want[0],
                           want[1]);
                }
            }
        }
    }

    return;
}

// D_TESTS_REAL_OF
//   stamps d_tests_real_of_<S>: the real function of the family in the form
// _pure of one of the five, at _x rounded to the family, widened.
#define D_TESTS_REAL_OF(T, S)                                                  \
static long double                                                             \
d_tests_real_of_##S(                                                           \
    int         _op,                                                           \
    bool        _pure,                                                         \
    long double _x                                                             \
)                                                                              \
{                                                                              \
    const T    x = (T)_x;                                                      \
    volatile T r = (T)0;                                                       \
                                                                               \
    switch (_op)                                                               \
    {                                                                          \
    case D_TESTS_EXP2:                                                         \
        r = _pure ? d_math_##S##_exp2_c(x) : d_math_##S##_exp2(x);             \
        break;                                                                 \
    case D_TESTS_LOG2:                                                         \
        r = _pure ? d_math_##S##_log2_c(x) : d_math_##S##_log2(x);             \
        break;                                                                 \
    case D_TESTS_LOG10:                                                        \
        r = _pure ? d_math_##S##_log10_c(x) : d_math_##S##_log10(x);           \
        break;                                                                 \
    case D_TESTS_LOG1P:                                                        \
        r = _pure ? d_math_##S##_log1p_c(x) : d_math_##S##_log1p(x);           \
        break;                                                                 \
    default:                                                                   \
        r = _pure ? d_math_##S##_expm1_c(x) : d_math_##S##_expm1(x);           \
        break;                                                                 \
    }                                                                          \
                                                                               \
    return (long double)r;                                                     \
}

D_TESTS_REAL_OF(float, f)
D_TESTS_REAL_OF(double, d)
D_TESTS_REAL_OF(long double, ld)

// d_tests_real_of
//   the real function of one of the five, as d_tests_real_of_<S>.
static long double
d_tests_real_of(
    int         _family,
    int         _op,
    bool        _pure,
    long double _x
)
{
    if (_family == 0)
    {
        return d_tests_real_of_f(_op, _pure, _x);
    }

    if (_family == 1)
    {
        return d_tests_real_of_d(_op, _pure, _x);
    }

    return d_tests_real_of_ld(_op, _pure, _x);
}

// d_tests_real_axis
//   each of the five on the real axis, where its value is real: in every
// family and both forms, the real function's value in the same form, bit
// for bit, and the zero of y, of either sign -- at values of x from the
// subnormals to past the overflow, and at -inf, +inf and NaN where the real
// function takes them.
static void
d_tests_real_axis(void)
{
    static const long double xs[] =
    {
        0.0L, -0.0L, 1.0L, -1.0L, 0.5L, -0.75L, 3.0L, 8.0L, 0.1L, -0.3L,
        1.0e-30L, -2.5e-20L, 7.25L, 100.0L, 1000.0L, 1.0e10L, -40.0L,
        88.5L, 130.0L, 709.5L, 1100.0L, 11356.0L, 16400.0L, -16500.0L,
        1.0e-310L, 3.0e-4940L
    };
    const long double inf = d_internal_math_ld_infinity();
    const long double odd[3] = { inf, -inf, d_internal_math_ld_nan() };
    size_t i = 0u;
    int    op = 0;

    for (op = D_TESTS_EXP2; op <= D_TESTS_EXPM1; ++op)
    {
        for (i = 0u; i < (D_TESTS_COUNT(xs) + 3u); ++i)
        {
            const long double x = (i < D_TESTS_COUNT(xs))
                                      ? xs[i]
                                      : odd[i - D_TESTS_COUNT(xs)];
            int sign = 0;

            for (sign = 0; sign < 2; ++sign)
            {
                long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
                int         family = 0;

                in[0] = x;
                in[1] = (sign == 0) ? 0.0L : -0.0L;

                for (family = 0; family < 3; ++family)
                {
                    int form = 0;

                    // where the value is real: x positive for log2 and
                    // log10, past -1 for log1p, every x for exp2 and expm1
                    if ( ( ( (op == D_TESTS_LOG2) ||
                             (op == D_TESTS_LOG10) ) &&
                           (!(x > 0.0L)) ) ||
                         ( (op == D_TESTS_LOG1P) &&
                           (!(x > -1.0L)) ) )
                    {
                        continue;
                    }

                    for (form = 0; form < 2; ++form)
                    {
                        const bool pure = (form == 0);
                        const struct dj_tests_result got =
                            d_tests_call(family, op, pure, in);
                        const long double want =
                            d_tests_real_of(family, op, pure, x);
                        const bool ok =
                            ( (d_tests_same(got.part[0], want)) &&
                              (d_tests_same(got.part[1], in[1])) );

                        D_TESTS_EXPECT(ok, "the real function's value on the"
                                           " real axis");

                        if ( (!ok) &&
                             (g_failures <= D_TESTS_SHOW) )
                        {
                            printf("  %s(%Lg, %Lg) %s%s: (%.21Lg, %Lg), the"
                                   " real function's %.21Lg\n",
                                   g_op_names[op],
                                   in[0],
                                   in[1],
                                   g_family_names[family],
                                   pure ? " _c" : "",
                                   got.part[0],
                                   got.part[1],
                                   want);
                        }
                    }
                }
            }
        }
    }

    return;
}

// d_tests_underflow
//   expm1 on the imaginary axis where its real part, -2 sin^2(y/2),
// underflows: -0, the zero of a negative value, in long double where its
// range reaches 2^-8250, a quad's or an x87's.
static void
d_tests_underflow(void)
{
    long double in[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    int         form  = 0;

    // long double of double's range: no such y
    if (LDBL_MAX_EXP < 16384)
    {
        return;
    }

    in[1] = d_internal_math_ld_pow2(-8250);

    for (form = 0; form < 2; ++form)
    {
        const struct dj_tests_result got =
            d_tests_call(2, D_TESTS_EXPM1, (form == 0), in);
        const bool ok = ( (got.part[0] == 0.0L) &&
                          (d_internal_math_ld_negative(got.part[0])) &&
                          (got.part[1] == in[1]) );

        D_TESTS_EXPECT(ok, "expm1(i 2^-8250): -0 + i 2^-8250");
    }

    return;
}

// d_tests_infinities
//   G.5's infinities: a product of an infinity and a nonzero, a quotient of
// a nonzero by 0 or of an infinity by a finite value, an infinity whatever
// its other part; a finite value over an infinity, 0. And the projection
// and the modulus of an infinity with a NaN.
static void
d_tests_infinities(void)
{
    const long double inf = d_internal_math_ld_infinity();
    const long double nan = d_internal_math_ld_nan();
    int               family = 0;

    for (family = 0; family < 3; ++family)
    {
        const long double in_mul[4]  = { inf, nan, 2.0L, 3.0L };
        const long double in_mul2[4] = { 1.0L, 1.0L, nan, -inf };
        const long double in_div0[4] = { 1.0L, 1.0L, 0.0L, 0.0L };
        const long double in_divi[4] = { inf, nan, 2.0L, 3.0L };
        const long double in_divz[4] = { 1.0L, 1.0L, inf, inf };
        const long double in_rdiv[4] = { 0.0L, 0.0L, 1.0L, 0.0L };
        const long double in_proj[4] = { nan, -inf, 0.0L, 0.0L };
        const long double in_proj2[4] = { -inf, 3.0L, 0.0L, 0.0L };
        const long double in_abs[4]  = {  nan, -inf, 0.0L, 0.0L };
        const struct dj_tests_result mul  =
            d_tests_call(family, D_TESTS_MUL, true, in_mul);
        const struct dj_tests_result mul2 =
            d_tests_call(family, D_TESTS_MUL, true, in_mul2);
        const struct dj_tests_result div0 =
            d_tests_call(family, D_TESTS_DIV, true, in_div0);
        const struct dj_tests_result divi =
            d_tests_call(family, D_TESTS_DIV, true, in_divi);
        const struct dj_tests_result divz =
            d_tests_call(family, D_TESTS_DIV, true, in_divz);
        const struct dj_tests_result rdiv =
            d_tests_call(family, D_TESTS_REAL_DIV, true, in_rdiv);
        const struct dj_tests_result proj =
            d_tests_call(family, D_TESTS_PROJ, true, in_proj);
        const struct dj_tests_result proj2 =
            d_tests_call(family, D_TESTS_PROJ, true, in_proj2);
        const struct dj_tests_result abs_c =
            d_tests_call(family, D_TESTS_ABS, true, in_abs);
        const struct dj_tests_result abs_p =
            d_tests_call(family, D_TESTS_ABS, false, in_abs);

        D_TESTS_EXPECT( ( (d_tests_matches(family, mul.part[0], 'J')) ||
                          (d_tests_matches(family, mul.part[1], 'J')) ),
                        "an infinity times a nonzero is an infinity" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, mul2.part[0], 'J')) ||
                          (d_tests_matches(family, mul2.part[1], 'J')) ),
                        "a nonzero times an infinity is an infinity" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, div0.part[0], 'J')) ||
                          (d_tests_matches(family, div0.part[1], 'J')) ),
                        "a nonzero over 0 is an infinity" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, divi.part[0], 'J')) ||
                          (d_tests_matches(family, divi.part[1], 'J')) ),
                        "an infinity over a finite value is an infinity" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, divz.part[0], 'z')) &&
                          (d_tests_matches(family, divz.part[1], 'z')) ),
                        "a finite value over an infinity is 0" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, rdiv.part[0], 'J')) ||
                          (d_tests_matches(family, rdiv.part[1], 'J')) ),
                        "a real nonzero over 0 is an infinity" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, proj.part[0], 'I')) &&
                          (d_tests_matches(family, proj.part[1], 'o')) ),
                        "proj of an infinity with a NaN" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, proj2.part[0], 'I')) &&
                          (d_tests_matches(family, proj2.part[1], '0')) ),
                        "proj of -inf + 3i" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, abs_c.part[0], 'I')) &&
                          (d_tests_matches(family, abs_p.part[0], 'I')) ),
                        "the modulus of an infinity with a NaN is +inf" );

        {
            // |3/4 + i| 2^127 and |3 + 4i| 2^-149, exactly: past the
            // squares' overflow, and in float's subnormals
            const long double big[4] =
                { 0.75L * d_internal_math_ld_pow2(127),
                  d_internal_math_ld_pow2(127), 0.0L, 0.0L };
            const long double tiny[4] =
                { 3.0L * d_internal_math_ld_pow2(-149),
                  4.0L * d_internal_math_ld_pow2(-149), 0.0L, 0.0L };
            int form = 0;

            for (form = 0; form < 2; ++form)
            {
                const struct dj_tests_result b =
                    d_tests_call(family, D_TESTS_ABS, (form == 0), big);
                const struct dj_tests_result t =
                    d_tests_call(family, D_TESTS_ABS, (form == 0), tiny);

                D_TESTS_EXPECT( ( (b.part[0] ==
                                   1.25L * d_internal_math_ld_pow2(127)) &&
                                  (t.part[0] ==
                                   5.0L * d_internal_math_ld_pow2(-149)) ),
                                "the modulus past overflow and in the"
                                " subnormals" );
            }
        }

        {
            // whole powers: (2^20 (1 + i))^-7 = 2^-144 (1 + i), a float's
            // subnormal, where z^7 itself is past float's range; (1 + i)^200
            // = 2^100; and (1 + i)^-260 = -2^-130, again a float's
            // subnormal, where (1 + i)^260 is past float's range -- the
            // power rescaled as it is formed
            const long double scaled[4] =
                { d_internal_math_ld_pow2(20), d_internal_math_ld_pow2(20),
                  -7.0L, 0.0L };
            const long double unit[4]  = { 1.0L, 1.0L, 200.0L, 0.0L };
            const long double small[4] = { 1.0L, 1.0L, -260.0L, 0.0L };
            // and (1 + i)^(-2 EMAX) = 2^-EMAX, the family's subnormal, where
            // the eighth and later squares pass its range: the power
            // rescaled after each square, not only after each product
            const int         emax[3]  = { FLT_MAX_EXP, DBL_MAX_EXP,
                                           LDBL_MAX_EXP };
            const long double far[4]   =
                { 1.0L, 1.0L, -2.0L * (long double)emax[family], 0.0L };
            int form = 0;

            for (form = 0; form < 2; ++form)
            {
                const struct dj_tests_result p =
                    d_tests_call(family, D_TESTS_POW, (form == 0), scaled);
                const struct dj_tests_result q =
                    d_tests_call(family, D_TESTS_POW, (form == 0), unit);
                const struct dj_tests_result r =
                    d_tests_call(family, D_TESTS_POW, (form == 0), small);
                const struct dj_tests_result f =
                    d_tests_call(family, D_TESTS_POW, (form == 0), far);

                D_TESTS_EXPECT( ( (p.part[0] ==
                                   d_internal_math_ld_pow2(-144)) &&
                                  (p.part[1] ==
                                   d_internal_math_ld_pow2(-144)) &&
                                  (q.part[0] ==
                                   d_internal_math_ld_pow2(100)) &&
                                  (q.part[1] == 0.0L) &&
                                  (r.part[0] ==
                                   -d_internal_math_ld_pow2(-130)) &&
                                  (r.part[1] == 0.0L) &&
                                  (f.part[0] ==
                                   d_tests_point(1L, -emax[family])) &&
                                  (f.part[1] == 0.0L) ),
                                "whole powers, rescaled as they are formed" );
            }
        }
    }

    {
        // equality is the parts': a NaN equals nothing, -0 equals +0
        const long double nan = d_internal_math_ld_nan();

        D_TESTS_EXPECT( (!d_cplx_d_equal(d_cplx_d_make((double)nan, 0.0),
                                         d_cplx_d_make((double)nan, 0.0))) &&
                        (d_cplx_f_equal(d_cplx_f_make(0.0f, -0.0f),
                                        d_cplx_f_make(-0.0f, 0.0f))) &&
                        (!d_cplx_ld_equal(d_cplx_ld_make(1.0L, 2.0L),
                                          d_cplx_ld_make(1.0L, -2.0L))),
                        "equality of the parts" );
        D_TESTS_EXPECT( (d_cplx_d_real(d_cplx_d_make(1.5, -2.5)) == 1.5) &&
                        (d_cplx_d_imag(d_cplx_d_make(1.5, -2.5)) == -2.5),
                        "the parts read back" );
    }

    return;
}


// d_tests_paths
//   the paths no reference reaches, each family's, both forms where there
// are two: a product of NaNs, and of an infinity whose formula keeps a part;
// a product whose operands leave the safe range, a part of each 0, and one
// with a subnormal operand, each exact; a quotient with a zero real part, of
// a NaN over 0, and of NaNs; a whole power far past either end; pow of a
// NaN, of 0 to a negative or a complex power, and of an infinity, C99's
// formula; and acos(1 +- 0i).
static void
d_tests_paths(void)
{
    const long double inf = d_internal_math_ld_infinity();
    const long double nan = d_internal_math_ld_nan();
    // a part past each family's safe range, and a subnormal of each
    const int         big_e[3]  = { 100, 600, 9000 };
    const int         tiny_e[3] = { -145, -1070, -16440 };
    const int         lift_e[3] = { 100, 1000, 16000 };
    int               family    = 0;

    for (family = 0; family < 3; ++family)
    {
        const long double big  = d_tests_point(1L, big_e[family]);
        const long double tiny = d_tests_point(1L, tiny_e[family]);
        const long double lift = d_tests_point(1L, lift_e[family]);
        const long double in_nans[4]  = { nan, nan, 1.0L, 1.0L };
        const long double in_infx[4]  = { inf, 1.0L, 1.0L, 0.0L };
        const long double in_far[4]   = { 0.0L, big, 0.0L, 1.0L / big };
        const long double in_sub[4]   = { tiny, 0.0L, lift, 0.0L };
        const long double in_imag[4]  = { 0.0L, 2.0L, 1.0L, 1.0L };
        const long double in_nan0[4]  = { nan, 1.0L, 0.0L, 0.0L };
        const struct dj_tests_result mul_nans =
            d_tests_call(family, D_TESTS_MUL, true, in_nans);
        const struct dj_tests_result mul_infx =
            d_tests_call(family, D_TESTS_MUL, true, in_infx);
        const struct dj_tests_result mul_far =
            d_tests_call(family, D_TESTS_MUL, true, in_far);
        const struct dj_tests_result mul_sub =
            d_tests_call(family, D_TESTS_MUL, true, in_sub);
        const struct dj_tests_result div_imag =
            d_tests_call(family, D_TESTS_DIV, true, in_imag);
        const struct dj_tests_result div_nan0 =
            d_tests_call(family, D_TESTS_DIV, true, in_nan0);
        const struct dj_tests_result div_nans =
            d_tests_call(family, D_TESTS_DIV, true, in_nans);
        int form = 0;

        D_TESTS_EXPECT( ( (d_tests_matches(family, mul_nans.part[0], 'n')) &&
                          (d_tests_matches(family, mul_nans.part[1], 'n')) ),
                        "a product of NaNs is NaNs" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, mul_infx.part[0], 'I')) &&
                          (d_tests_matches(family, mul_infx.part[1], 'n')) ),
                        "(inf + i) 1 is the formula's inf + i NaN" );
        D_TESTS_EXPECT( ( (mul_far.part[0] == -1.0L) &&
                          (d_tests_matches(family, mul_far.part[1], '0')) ),
                        "(i 2^E)(i 2^-E) past the safe range is -1 + 0i" );
        D_TESTS_EXPECT( ( (mul_sub.part[0] ==
                           d_tests_point(1L, tiny_e[family] +
                                             lift_e[family])) &&
                          (d_tests_matches(family, mul_sub.part[1], 'z')) ),
                        "a subnormal times a large value, exactly" );
        D_TESTS_EXPECT( ( (div_imag.part[0] == 1.0L) &&
                          (div_imag.part[1] == 1.0L) ),
                        "2i / (1 + i) is 1 + i" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, div_nan0.part[0], 'n')) &&
                          (d_tests_matches(family, div_nan0.part[1], 'I')) ),
                        "(NaN + i) / 0 is NaN + i inf" );
        D_TESTS_EXPECT( ( (d_tests_matches(family, div_nans.part[0], 'n')) &&
                          (d_tests_matches(family, div_nans.part[1], 'n')) ),
                        "a quotient of NaNs is NaNs" );

        for (form = 0; form < 2; ++form)
        {
            const bool        pure = (form == 0);
            const long double in_up[4]    = { 1.0L, 1.0L, 1048576.0L, 0.0L };
            const long double in_down[4]  = { 1.0L, 1.0L, -1048576.0L, 0.0L };
            const long double in_small[4] = { 0.25L, 0.25L, 1048576.0L, 0.0L };
            const long double in_pnan[4]  = { nan, 1.0L, 2.0L, 0.0L };
            const long double in_zneg[4]  = { 0.0L, 0.0L, -2.0L, 0.0L };
            const long double in_zcpx[4]  = { 0.0L, 0.0L, -1.0L, 1.0L };
            const long double in_zimg[4]  = { 0.0L, 0.0L, 0.0L, 1.0L };
            const long double in_pinf[4]  = { inf, 0.0L, 1.0L, 1.0L };
            const long double in_one[4]   = { 1.0L, 0.0L, 0.0L, 0.0L };
            const long double in_onem[4]  = { 1.0L, -0.0L, 0.0L, 0.0L };
            const struct dj_tests_result up =
                d_tests_call(family, D_TESTS_POW, pure, in_up);
            const struct dj_tests_result down =
                d_tests_call(family, D_TESTS_POW, pure, in_down);
            const struct dj_tests_result small =
                d_tests_call(family, D_TESTS_POW, pure, in_small);
            const struct dj_tests_result pnan =
                d_tests_call(family, D_TESTS_POW, pure, in_pnan);
            const struct dj_tests_result zneg =
                d_tests_call(family, D_TESTS_POW, pure, in_zneg);
            const struct dj_tests_result zcpx =
                d_tests_call(family, D_TESTS_POW, pure, in_zcpx);
            const struct dj_tests_result zimg =
                d_tests_call(family, D_TESTS_POW, pure, in_zimg);
            const struct dj_tests_result pinf =
                d_tests_call(family, D_TESTS_POW, pure, in_pinf);
            const struct dj_tests_result one =
                d_tests_call(family, D_TESTS_ACOS, pure, in_one);
            const struct dj_tests_result onem =
                d_tests_call(family, D_TESTS_ACOS, pure, in_onem);

            D_TESTS_EXPECT( ( (d_tests_matches(family, up.part[0], 'I')) &&
                              (d_tests_matches(family, up.part[1], 'z')) &&
                              (d_tests_matches(family, down.part[0], 'z')) &&
                              (d_tests_matches(family, down.part[1], 'z')) &&
                              (d_tests_matches(family, small.part[0], 'z')) &&
                              (d_tests_matches(family, small.part[1], 'z')) ),
                            "(1 + i)^(+-2^20) and ((1 + i)/4)^(2^20), past"
                            " either end" );
            D_TESTS_EXPECT( ( (d_tests_matches(family, pnan.part[0], 'n')) &&
                              (d_tests_matches(family, pnan.part[1], 'n')) ),
                            "pow of a NaN is NaNs" );
            D_TESTS_EXPECT( ( (d_tests_matches(family, zneg.part[0], 'I')) &&
                              (d_tests_matches(family, zneg.part[1], '0')) &&
                              (d_tests_matches(family, zcpx.part[0], 'n')) &&
                              (d_tests_matches(family, zcpx.part[1], 'n')) &&
                              (d_tests_matches(family, zimg.part[0], 'n')) &&
                              (d_tests_matches(family, zimg.part[1], 'n')) ),
                            "0 to a negative power is +inf, to a complex one"
                            " NaNs" );
            D_TESTS_EXPECT( ( (d_tests_matches(family, pinf.part[0], 'J')) &&
                              (d_tests_matches(family, pinf.part[1], 'n')) ),
                            "inf^(1 + i) is exp(+inf + i inf), +-inf + i NaN" );
            D_TESTS_EXPECT( ( (d_tests_matches(family, one.part[0], '0')) &&
                              (d_tests_matches(family, one.part[1], 'o')) &&
                              (d_tests_matches(family, onem.part[0], '0')) &&
                              (d_tests_matches(family, onem.part[1], '0')) ),
                            "acos(1 + i0) is 0 - i0, acos(1 - i0) 0 + i0" );
        }
    }

    return;
}


//==============================================================================
// 5.  C99'S _Complex (C ONLY)
//==============================================================================

#if ( (!defined(__cplusplus))          &&                                     \
      (!defined(__STDC_NO_COMPLEX__)) &&                                     \
      (!defined(D_ENV_COMPILER_MSVC)) )

// d_tests_complex
//   each family's struct to _Complex and back, its parts in place.
static void
d_tests_complex(void)
{
    const struct dj_cplx_f  f = d_cplx_f_make(1.5f, -0.0f);
    const struct dj_cplx_d  d = d_cplx_d_make(-2.25, 3.0);
    const struct dj_cplx_ld l = d_cplx_ld_make(0.125L, -7.0L);
    const float _Complex        fc = d_cplx_f_to_complex(f);
    const double _Complex       dc = d_cplx_d_to_complex(d);
    const long double _Complex  lc = d_cplx_ld_to_complex(l);
    float       fp[2] = { 0.0f, 0.0f };
    double      dp[2] = { 0.0, 0.0 };
    long double lp[2] = { 0.0L, 0.0L };

    memcpy(fp, &fc, sizeof(fp));
    memcpy(dp, &dc, sizeof(dp));
    memcpy(lp, &lc, sizeof(lp));

    D_TESTS_EXPECT( (fp[0] == 1.5f) &&
                    (d_internal_math_f_negative(fp[1])) &&
                    (dp[0] == -2.25) &&
                    (dp[1] == 3.0) &&
                    (lp[0] == 0.125L) &&
                    (lp[1] == -7.0L),
                    "to _Complex, the real part first" );
    D_TESTS_EXPECT( (d_tests_same(d_cplx_f_from_complex(fc).part[1],
                                  -0.0L)) &&
                    (d_cplx_d_equal(d_cplx_d_from_complex(dc), d)) &&
                    (d_cplx_ld_equal(d_cplx_ld_from_complex(lc), l)),
                    "from _Complex, the parts kept" );

    return;
}

#endif  // C99's _Complex


//==============================================================================
// 6.  C++
//==============================================================================

#if defined(__cplusplus)

// d_tests_overloads
//   one name for every family: the family's function, bit for bit.
static void
d_tests_overloads(void)
{
    const struct dj_cplx_f  f = d_cplx_f_make(0.75f, -1.25f);
    const struct dj_cplx_d  d = d_cplx_d_make(0.75, -1.25);
    const struct dj_cplx_ld l = d_cplx_ld_make(0.75L, -1.25L);

    D_TESTS_EXPECT( (d_cplx_f_equal(d_cplx_exp(f), d_cplx_f_exp(f))) &&
                    (d_cplx_d_equal(d_cplx_exp_c(d), d_cplx_d_exp_c(d))) &&
                    (d_cplx_ld_equal(d_cplx_log(l), d_cplx_ld_log(l))) &&
                    (d_cplx_f_equal(d_cplx_atanh_c(f),
                                    d_cplx_f_atanh_c(f))) &&
                    (d_cplx_d_equal(d_cplx_mul(d, d), d_cplx_d_mul(d, d))) &&
                    (d_cplx_ld_equal(d_cplx_pow_c(l, l),
                                     d_cplx_ld_pow_c(l, l))) &&
                    (d_cplx_abs(f) == d_cplx_f_abs(f)) &&
                    (d_cplx_arg_c(l) == d_cplx_ld_arg_c(l)) &&
                    (d_cplx_f_equal(d_cplx_exp2(f), d_cplx_f_exp2(f))) &&
                    (d_cplx_d_equal(d_cplx_log2_c(d),
                                    d_cplx_d_log2_c(d))) &&
                    (d_cplx_ld_equal(d_cplx_log10(l), d_cplx_ld_log10(l))) &&
                    (d_cplx_f_equal(d_cplx_log1p_c(f),
                                    d_cplx_f_log1p_c(f))) &&
                    (d_cplx_d_equal(d_cplx_expm1(d), d_cplx_d_expm1(d))),
                    "the C++ overloads are the families' functions" );

    return;
}

#endif  // __cplusplus

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// D_TESTS_CPLX_CONSTANT
//   stamps, for one family, every function's _c form at one point as a
// constant expression, pow_c's at a whole exponent of each sign too, and
// d_tests_constant_<S>, which compares each with the run time's result at
// the same point, read through volatiles.
#define D_TESTS_CPLX_CONSTANT(T, S, X, Y, U, V)                                \
static constexpr struct dj_cplx_##S g_cz_##S = d_cplx_##S##_make(X, Y);        \
static constexpr struct dj_cplx_##S g_cw_##S = d_cplx_##S##_make(U, V);        \
static constexpr struct dj_cplx_##S g_cr_##S[] =                               \
{                                                                              \
    d_cplx_##S##_exp_c(g_cz_##S),   d_cplx_##S##_log_c(g_cz_##S),              \
    d_cplx_##S##_sqrt_c(g_cz_##S),  d_cplx_##S##_sin_c(g_cz_##S),              \
    d_cplx_##S##_cos_c(g_cz_##S),   d_cplx_##S##_tan_c(g_cz_##S),              \
    d_cplx_##S##_asin_c(g_cz_##S),  d_cplx_##S##_acos_c(g_cz_##S),             \
    d_cplx_##S##_atan_c(g_cz_##S),  d_cplx_##S##_sinh_c(g_cz_##S),             \
    d_cplx_##S##_cosh_c(g_cz_##S),  d_cplx_##S##_tanh_c(g_cz_##S),             \
    d_cplx_##S##_asinh_c(g_cz_##S), d_cplx_##S##_acosh_c(g_cz_##S),            \
    d_cplx_##S##_atanh_c(g_cz_##S), d_cplx_##S##_exp2_c(g_cz_##S),             \
    d_cplx_##S##_log2_c(g_cz_##S),  d_cplx_##S##_log10_c(g_cz_##S),            \
    d_cplx_##S##_log1p_c(g_cz_##S), d_cplx_##S##_expm1_c(g_cz_##S),            \
    d_cplx_##S##_pow_c(g_cz_##S, g_cw_##S),                                    \
    d_cplx_##S##_pow_c(g_cz_##S, d_cplx_##S##_make((T)7, (T)0)),               \
    d_cplx_##S##_pow_c(g_cz_##S, d_cplx_##S##_make((T)-3, (T)0)),              \
    d_cplx_##S##_mul(g_cz_##S, g_cw_##S),                                      \
    d_cplx_##S##_div(g_cz_##S, g_cw_##S),                                      \
    d_cplx_##S##_make(d_cplx_##S##_abs_c(g_cz_##S),                            \
                      d_cplx_##S##_arg_c(g_cz_##S))                            \
};                                                                             \
                                                                               \
static void                                                                    \
d_tests_constant_##S(void)                                                     \
{                                                                              \
    volatile T               x = X;                                            \
    volatile T               y = Y;                                            \
    volatile T               u = U;                                            \
    volatile T               v = V;                                            \
    const struct dj_cplx_##S z = d_cplx_##S##_make(x, y);                      \
    const struct dj_cplx_##S w = d_cplx_##S##_make(u, v);                      \
    const struct dj_cplx_##S r[] =                                             \
    {                                                                          \
        d_cplx_##S##_exp_c(z),   d_cplx_##S##_log_c(z),                        \
        d_cplx_##S##_sqrt_c(z),  d_cplx_##S##_sin_c(z),                        \
        d_cplx_##S##_cos_c(z),   d_cplx_##S##_tan_c(z),                        \
        d_cplx_##S##_asin_c(z),  d_cplx_##S##_acos_c(z),                       \
        d_cplx_##S##_atan_c(z),  d_cplx_##S##_sinh_c(z),                       \
        d_cplx_##S##_cosh_c(z),  d_cplx_##S##_tanh_c(z),                       \
        d_cplx_##S##_asinh_c(z), d_cplx_##S##_acosh_c(z),                      \
        d_cplx_##S##_atanh_c(z), d_cplx_##S##_exp2_c(z),                       \
        d_cplx_##S##_log2_c(z),  d_cplx_##S##_log10_c(z),                      \
        d_cplx_##S##_log1p_c(z), d_cplx_##S##_expm1_c(z),                      \
        d_cplx_##S##_pow_c(z, w),                                              \
        d_cplx_##S##_pow_c(z, d_cplx_##S##_make((T)7, (T)0)),                  \
        d_cplx_##S##_pow_c(z, d_cplx_##S##_make((T)-3, (T)0)),                 \
        d_cplx_##S##_mul(z, w),  d_cplx_##S##_div(z, w),                       \
        d_cplx_##S##_make(d_cplx_##S##_abs_c(z), d_cplx_##S##_arg_c(z))        \
    };                                                                         \
    bool   ok = true;                                                          \
    size_t k  = 0u;                                                            \
                                                                               \
    for (k = 0u; k < D_TESTS_COUNT(r); ++k)                                    \
    {                                                                          \
        volatile T a0 = r[k].part[0];                                          \
        volatile T a1 = r[k].part[1];                                          \
                                                                               \
        if ( (!d_tests_same((long double)a0,                                   \
                            (long double)g_cr_##S[k].part[0])) ||              \
             (!d_tests_same((long double)a1,                                   \
                            (long double)g_cr_##S[k].part[1])) )               \
        {                                                                      \
            ok = false;                                                        \
            printf("  constant %s %lu: (%Lg, %Lg) at run time (%Lg, %Lg)\n",   \
                   #S,                                                         \
                   (unsigned long)k,                                           \
                   (long double)g_cr_##S[k].part[0],                           \
                   (long double)g_cr_##S[k].part[1],                           \
                   (long double)a0,                                            \
                   (long double)a1);                                           \
        }                                                                      \
    }                                                                          \
                                                                               \
    D_TESTS_EXPECT(ok, "constant evaluation agrees with the run time");        \
                                                                               \
    return;                                                                    \
}

D_TESTS_CPLX_CONSTANT(float, f, 0.75f, -1.25f, 1.5f, 0.5f)
D_TESTS_CPLX_CONSTANT(double, d, -2.5, 0.375, 0.25, -3.0)
D_TESTS_CPLX_CONSTANT(long double, ld, 1.0e-3L, 7.0L, -1.75L, 2.0L)

#endif  // C++14

int
main(void)
{
    d_tests_references();
    d_tests_edges();
    d_tests_full_width();
    d_tests_whole_powers();
    d_tests_annex_g();
    d_tests_definitions();
    d_tests_exact_values();
    d_tests_real_axis();
    d_tests_underflow();
    d_tests_infinities();
    d_tests_paths();
#if ( (!defined(__cplusplus))          &&                                     \
      (!defined(__STDC_NO_COMPLEX__)) &&                                     \
      (!defined(D_ENV_COMPILER_MSVC)) )
    d_tests_complex();
#endif
#if defined(__cplusplus)
    d_tests_overloads();
#endif
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_f();
    d_tests_constant_d();
    d_tests_constant_ld();
#endif

    printf("t_cplx: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
