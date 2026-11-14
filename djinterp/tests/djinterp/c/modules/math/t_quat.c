/*******************************************************************************
* djinterp [c/math]                                                     t_quat.c
*
*   Conformance harness for c/math/quat.h, all three families.
*
*   The accurate functions against math_tests_quat_refs.h's references,
* computed by mpmath: each part's distance in steps (float_order.h) from the
* correctly rounded one, its largest for each function, family and form
* printed and held to the bound quat.h states -- for slerp, whose parts are
* sums that may cancel, the distance in steps of the largest part. The _c
* and plain forms of what takes a root alone give the same bits.
*
*   The _fast forms against the former formulas written out here in the
* same order, bit for bit. Identities: q q*, q q^-1 and the quotients, the
* rotation and its matrix, q, -q and 2^k q the same rotation, Shepperd's
* method inverting the matrix, two vectors' quaternion turning one into the
* other, exp and log, the square root squared, powers as products; the
* product of parts of 0, 1/2, 1 and 2, every step exact, the formula's, a
* zero's sign too; from_matrix4 from_matrix3's. For a vector part along im
* alone, a real q's too, exp, log, sqrt and pow are cplx.h's functions bit
* for bit, in both forms. The special values each function's comment
* states: infinities, NaNs and zeros; the product at the range's end and on
* a midpoint, its low part deciding; two vectors nearly opposed; log near
* the cut. slerp's and nlerp's ends. In C++, each overload the family's
* function, bit for bit.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_quat.c -o t_quat -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_quat.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.05
*                                                            revised: 2026.11.07
*******************************************************************************/
// std
#include <float.h>   // LDBL_MANT_DIG, FLT_MAX_EXP
#include <math.h>    // ceil
#include <stdio.h>   // printf, sprintf
#include <string.h>  // strlen
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/decimal.h"  // d_math_decimal_to_f
#include "../../../../../inc/djinterp/c/modules/math/quat.h"  // unit under test
// tests
#include "./math_tests_quat_refs.h"  // g_quat_ref_multiply ... g_quat_ref_pow


#define D_TESTS_FAR  1000000ul

static long          g_checks   = 0;
static long          g_failures = 0;
static unsigned long g_state    = 1234567u;

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

        if (g_failures <= 40)
        {
            printf("  FAIL line %d: %s\n", _line, _what);
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what)                                              \
    d_tests_expect((ok), (what), __LINE__)

// a value in [-4, 4) with a random fraction, stored, so that it is a
// double: clang on x87 hands inlined code the quotient's 64-bit register
// value otherwise
static double
d_tests_random(void)
{
    volatile double value = 0.0;

    g_state = (g_state * 1103515245u) + 12345u;
    value   = ((double)((g_state >> 4) % 2000003u) / 250000.0) - 4.0;

    return value;
}


//==============================================================================
// 1.  THE REFERENCES
//==============================================================================

// the functions of the reference tables, in their order
enum
{
    D_TESTS_MULTIPLY,
    D_TESTS_INVERSE,
    D_TESTS_DIVIDE,
    D_TESTS_LEFT_DIVIDE,
    D_TESTS_ROTATE,
    D_TESTS_TO_MATRIX3,
    D_TESTS_NORM,
    D_TESTS_NORMALIZE,
    D_TESTS_FROM_MATRIX3,
    D_TESTS_FROM_TWO_VECTORS,
    D_TESTS_NLERP,
    D_TESTS_SLERP,
    D_TESTS_EXP,
    D_TESTS_LOG,
    D_TESTS_SQRT,
    D_TESTS_POW,
    D_TESTS_FUNCTIONS
};

// dj_tests_table
//   one function's references: its name, rows, inputs and parts; whether it
// has a _c form and a plain one (forms 2) or one form; whether its plain
// form is the _c form's kernel on the library's root, the same bits; and
// whether each form is measured against the function's largest part, as a
// sum that cancels or the cosine of a rounded |v| is not each part's own.
struct dj_tests_table
{
    const char*                     name;
    const struct dj_tests_quat_ref* rows;
    size_t                          count;
    size_t                          n_in;
    size_t                          n_out;
    int                             forms;
    bool                            same_bits;
    bool                            normwise[2];
};

#define D_TESTS_TABLE(name, n_in, n_out, forms, same, norm_c, norm_plain)    \
    { #name, g_quat_ref_##name,                                               \
      sizeof(g_quat_ref_##name) / sizeof(g_quat_ref_##name[0]),               \
      (n_in), (n_out), (forms), (same), { (norm_c), (norm_plain) } }

static const struct dj_tests_table g_tables[D_TESTS_FUNCTIONS] =
{
    D_TESTS_TABLE(multiply,         8, 4, 1, false, false, false),
    D_TESTS_TABLE(inverse,          4, 4, 1, false, false, false),
    D_TESTS_TABLE(divide,           8, 4, 1, false, false, false),
    D_TESTS_TABLE(left_divide,      8, 4, 1, false, false, false),
    D_TESTS_TABLE(rotate,           7, 3, 1, false, false, false),
    D_TESTS_TABLE(to_matrix3,       4, 9, 1, false, false, false),
    D_TESTS_TABLE(norm,             4, 1, 2, true,  false, false),
    D_TESTS_TABLE(normalize,        4, 4, 2, true,  false, false),
    D_TESTS_TABLE(from_matrix3,     9, 4, 2, true,  false, false),
    D_TESTS_TABLE(from_two_vectors, 6, 4, 2, true,  false, false),
    D_TESTS_TABLE(nlerp,            9, 4, 2, true,  false, false),
    D_TESTS_TABLE(slerp,            9, 4, 2, false, true,  true),
    D_TESTS_TABLE(exp,              4, 4, 2, false, false, true),
    D_TESTS_TABLE(log,              4, 4, 2, false, false, true),
    D_TESTS_TABLE(sqrt,             4, 4, 2, false, false, true),
    D_TESTS_TABLE(pow,              5, 4, 2, false, false, true)
};

// the bounds quat.h states, in steps: [function][form][family], form 0 the
// accurate or _c one, 1 the plain one; slerp's, and the plain exp's, log's,
// sqrt's and pow's, of the largest part, the plain exp's and pow's at the
// references' |v| below 16 and |p| below 8, as their error grows with them
static const unsigned long g_bounds[D_TESTS_FUNCTIONS][2][3] =
{
    { {  1,  1,  1 }, {  0,  0,  0 } },   // multiply
    { {  1,  1,  1 }, {  0,  0,  0 } },   // inverse
    { {  1,  1,  1 }, {  0,  0,  0 } },   // divide
    { {  1,  1,  1 }, {  0,  0,  0 } },   // left_divide
    { {  1,  1,  1 }, {  0,  0,  0 } },   // rotate
    { {  1,  1,  1 }, {  0,  0,  0 } },   // to_matrix3
    { {  1,  1,  1 }, {  1,  1,  1 } },   // norm
    { {  1,  1,  1 }, {  1,  1,  1 } },   // normalize
    { {  1,  1,  1 }, {  1,  1,  1 } },   // from_matrix3
    { {  1,  1,  2 }, {  1,  1,  2 } },   // from_two_vectors
    { {  1,  1,  2 }, {  1,  1,  2 } },   // nlerp
    { {  1,  1,  2 }, {  4,  4,  4 } },   // slerp
    { {  1,  1,  4 }, { 12, 12, 12 } },   // exp
    { {  1,  1,  4 }, {  4,  4,  4 } },   // log
    { {  1,  1,  4 }, {  4,  4,  4 } },   // sqrt
    { {  1,  1,  4 }, { 12, 12, 12 } }    // pow
};

// the largest distance measured, and the rows: [function][form][family]
static unsigned long g_worst[D_TESTS_FUNCTIONS][2][3];
static unsigned long g_rows[D_TESTS_FUNCTIONS][2][3];

// d_tests_part
//   an input part, exactly: (m[0] 2^48 + m[1] 2^24 + m[2]) 2^e, the scaling
// in steps of 2^8000, which pow2 reaches, so that only an exact last product
// can be subnormal.
static long double
d_tests_part(
    const struct dj_tests_quat_part* _p
)
{
    const long double limb  = 16777216.0L;
    long double       value = ((((long double)_p->m[0] * limb) +
                                (long double)_p->m[1]) * limb) +
                              (long double)_p->m[2];
    int               e     = _p->e;

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

// d_tests_ulp
//   the gap above |_x| among the family's values.
static long double
d_tests_ulp(
    int         _family,
    long double _x
)
{
    const long double a = (_x < 0.0L) ? -_x : _x;

    if (_family == 0)
    {
        const volatile float f = (float)a;

        return (long double)d_math_f_next_up(f) - (long double)f;
    }

    if (_family == 1)
    {
        const volatile double d = (double)a;

        return (long double)d_math_d_next_up(d) - (long double)d;
    }

    return d_math_ld_next_up(a) - a;
}

// d_tests_same_bits
//   whether two widened parts are the same value: the same bits, any NaN the
// same as any other, and a zero's sign counted.
static bool
d_tests_same_bits(
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

    if ( (_a == 0.0L) &&
         (_b == 0.0L) )
    {
        return ((1.0L / _a) == (1.0L / _b));
    }

    return (_a == _b);
}

// one family's operations, each called from here alone, so that a build
// compiles each of quat.h's functions once in each family: the function _fn
// in the form _form (0 the accurate or _c form, 1 the plain one), of _a, of
// _a and _b, or of _a, _b and _t, into _out
#define D_TESTS_QUAT_CALL(T, S)                                                \
static void                                                                    \
d_tests_op_##S(                                                                \
    int      _fn,                                                              \
    int      _form,                                                            \
    const T* _a,                                                               \
    const T* _b,                                                               \
    T        _t,                                                               \
    T*       _out                                                              \
)                                                                              \
{                                                                              \
    switch (_fn)                                                               \
    {                                                                          \
        case D_TESTS_MULTIPLY:                                                 \
            d_quat_##S##_multiply(_out, _a, _b);                               \
            break;                                                             \
        case D_TESTS_INVERSE:                                                  \
            d_quat_##S##_inverse(_out, _a);                                    \
            break;                                                             \
        case D_TESTS_DIVIDE:                                                   \
            d_quat_##S##_divide(_out, _a, _b);                                 \
            break;                                                             \
        case D_TESTS_LEFT_DIVIDE:                                              \
            d_quat_##S##_left_divide(_out, _a, _b);                            \
            break;                                                             \
        case D_TESTS_ROTATE:                                                   \
            d_quat_##S##_rotate(_out, _a, _b);                                 \
            break;                                                             \
        case D_TESTS_TO_MATRIX3:                                               \
            d_quat_##S##_to_matrix3(_out, _a);                                 \
            break;                                                             \
        case D_TESTS_NORM:                                                     \
            _out[0] = (_form == 0) ? d_quat_##S##_norm_c(_a)                   \
                                   : d_quat_##S##_norm(_a);                    \
            break;                                                             \
        case D_TESTS_NORMALIZE:                                                \
            if (_form == 0) { d_quat_##S##_normalize_c(_out, _a); }            \
            else            { d_quat_##S##_normalize(_out, _a); }              \
            break;                                                             \
        case D_TESTS_FROM_MATRIX3:                                             \
            if (_form == 0) { d_quat_##S##_from_matrix3_c(_out, _a); }         \
            else            { d_quat_##S##_from_matrix3(_out, _a); }           \
            break;                                                             \
        case D_TESTS_FROM_TWO_VECTORS:                                         \
            if (_form == 0) { d_quat_##S##_from_two_vectors_c(_out, _a, _b); } \
            else            { d_quat_##S##_from_two_vectors(_out, _a, _b); }   \
            break;                                                             \
        case D_TESTS_NLERP:                                                    \
            if (_form == 0) { d_quat_##S##_nlerp_c(_out, _a, _b, _t); }        \
            else            { d_quat_##S##_nlerp(_out, _a, _b, _t); }          \
            break;                                                             \
        case D_TESTS_SLERP:                                                    \
            if (_form == 0) { d_quat_##S##_slerp_c(_out, _a, _b, _t); }        \
            else            { d_quat_##S##_slerp(_out, _a, _b, _t); }          \
            break;                                                             \
        case D_TESTS_EXP:                                                      \
            if (_form == 0) { d_quat_##S##_exp_c(_out, _a); }                  \
            else            { d_quat_##S##_exp(_out, _a); }                    \
            break;                                                             \
        case D_TESTS_LOG:                                                      \
            if (_form == 0) { d_quat_##S##_log_c(_out, _a); }                  \
            else            { d_quat_##S##_log(_out, _a); }                    \
            break;                                                             \
        case D_TESTS_SQRT:                                                     \
            if (_form == 0) { d_quat_##S##_sqrt_c(_out, _a); }                 \
            else            { d_quat_##S##_sqrt(_out, _a); }                   \
            break;                                                             \
        default:                                                               \
            if (_form == 0) { d_quat_##S##_pow_c(_out, _a, _t); }              \
            else            { d_quat_##S##_pow(_out, _a, _t); }                \
            break;                                                             \
    }                                                                          \
                                                                               \
    return;                                                                    \
}                                                                              \
                                                                               \
/* the norm, through the operations */                                         \
static T                                                                       \
d_tests_norm_##S(                                                              \
    int      _form,                                                            \
    const T* _a                                                                \
)                                                                              \
{                                                                              \
    T out[1] = { (T)0 };                                                       \
                                                                               \
    d_tests_op_##S(D_TESTS_NORM, _form, _a, _a, (T)0, out);                    \
                                                                               \
    return out[0];                                                             \
}                                                                              \
                                                                               \
/* a reference row's inputs in the family, the function's parts widened: */    \
/* a vector second from the fourth input for two vectors, the fifth for a */   \
/* rotation; t the ninth input, or pow's fifth */                              \
static void                                                                    \
d_tests_call_##S(                                                              \
    int                _fn,                                                    \
    int                _form,                                                  \
    const long double* _in,                                                    \
    long double*       _out                                                    \
)                                                                              \
{                                                                              \
    T      in[9]  = { (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0 };  \
    T      out[9] = { (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0 };  \
    size_t i      = 0u;                                                        \
                                                                               \
    for (i = 0u; i < 9u; ++i)                                                  \
    {                                                                          \
        in[i] = (T)_in[i];                                                     \
    }                                                                          \
                                                                               \
    d_tests_op_##S(_fn, _form, in,                                             \
                   (_fn == D_TESTS_FROM_TWO_VECTORS) ? (in + 3) : (in + 4),    \
                   (_fn == D_TESTS_POW) ? in[4] : in[8], out);                 \
                                                                               \
    for (i = 0u; i < 9u; ++i)                                                  \
    {                                                                          \
        _out[i] = (long double)out[i];                                         \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_QUAT_CALL(float, f)
D_TESTS_QUAT_CALL(double, d)
D_TESTS_QUAT_CALL(long double, ld)

// d_tests_call
//   the function _fn of the family _family (0 float, 1 double, 2 long
// double) in the form _form.
static void
d_tests_call(
    int                _family,
    int                _fn,
    int                _form,
    const long double* _in,
    long double*       _out
)
{
    if (_family == 0)
    {
        d_tests_call_f(_fn, _form, _in, _out);
    }
    else if (_family == 1)
    {
        d_tests_call_d(_fn, _form, _in, _out);
    }
    else
    {
        d_tests_call_ld(_fn, _form, _in, _out);
    }

    return;
}

// d_tests_allowance
//   the steps by which a product's part _part may miss its correctly rounded
// value as Dot2 promises, as accurate as the sum computed in twice the
// pairs' precision and rounded once: 16 u^2 times the sum of its products'
// magnitudes, in steps of _want, u 2^-53 for float and double, whose pairs
// are double's (2^-64's where they are long double's is smaller), and 2^-64
// for long double.
static unsigned long
d_tests_allowance(
    int                _family,
    const long double* _in,
    size_t             _part,
    long double        _want
)
{
    // each part's four products, by index into a (0 to 3) and b (4 to 7)
    static const int terms[4][8] =
    {
        { 3, 4, 0, 7, 1, 6, 2, 5 },
        { 3, 5, 0, 6, 1, 7, 2, 4 },
        { 3, 6, 0, 5, 1, 4, 2, 7 },
        { 3, 7, 0, 4, 1, 5, 2, 6 }
    };
    const long double u     = (_family == 2) ?
                                  d_internal_math_ld_pow2(-64) :
                                  d_internal_math_ld_pow2(-53);
    long double       sum   = 0.0L;
    long double       ratio = 0.0L;
    size_t            k     = 0u;

    for (k = 0u; k < 4u; ++k)
    {
        const long double p = _in[terms[_part][2u * k]] *
                              _in[terms[_part][(2u * k) + 1u]];

        sum += (p < 0.0L) ? -p : p;
    }

    ratio = (16.0L * u * u * sum) / d_tests_ulp(_family, _want);

    return (ratio >= (long double)D_TESTS_FAR) ? D_TESTS_FAR :
           (unsigned long)ceil((double)ratio);
}

// d_tests_distance
//   one row's distance for one family and form: the largest part's steps
// from its reference, or, measured against the largest part, the largest
// error in that part's steps; D_TESTS_FAR for a NaN.
static unsigned long
d_tests_distance(
    const struct dj_tests_table*    _t,
    const struct dj_tests_quat_ref* _row,
    int                             _family,
    int                             _form,
    const long double*              _got,
    const long double*              _in,
    unsigned long*                  _excess
)
{
    unsigned long worst = 0ul;
    long double   big   = 0.0L;
    long double   err   = 0.0L;
    size_t        i     = 0u;

    *_excess = 0ul;

    for (i = 0u; i < _t->n_out; ++i)
    {
        const long double want  = d_tests_text(_family, _row->want[i]);
        const long double a     = (want < 0.0L) ? -want : want;
        const long double d     = _got[i] - want;
        const long double e     = (d < 0.0L) ? -d : d;
        const unsigned long steps = d_tests_steps(_family, _got[i], want);
        const unsigned long allow = (_t->rows == g_quat_ref_multiply) ?
            d_tests_allowance(_family, _in, i, want) : 0ul;
        const unsigned long over  = (steps > allow) ? (steps - allow) : 0ul;

        *_excess = (over > *_excess) ? over : *_excess;

        // a NaN where a number is due
        if (_got[i] != _got[i])
        {
            *_excess = D_TESTS_FAR;

            return D_TESTS_FAR;
        }

        big   = (a > big) ? a : big;
        err   = (e > err) ? e : err;
        worst = (steps > worst) ? steps : worst;
    }

    // against the largest part, finite: the error in its steps
    if ( (_t->normwise[_form]) &&
         (big > 0.0L) &&
         (big < d_tests_text(_family, "1e5000")) )
    {
        const double ratio = (double)(err / d_tests_ulp(_family, big));

        worst = (ratio >= (double)D_TESTS_FAR) ? D_TESTS_FAR
                                               : (unsigned long)ceil(ratio);
        *_excess = worst;
    }

    // a NaN: no allowance
    if (worst == D_TESTS_FAR)
    {
        *_excess = worst;
    }

    return worst;
}

// d_tests_reference_row
//   one row of a table, for each family that holds it and each form: the
// distance of each part from its correctly rounded reference, the largest
// kept, and each held to its bound; the forms of the same bits compared.
static void
d_tests_reference_row(
    const struct dj_tests_table* _t,
    int                          _fn,
    size_t                       _r
)
{
    static const int                masks[3] = { 1, 2, 4 };
    const struct dj_tests_quat_ref* row      = &_t->rows[_r];
    long double                     in[9]    = { 0.0L, 0.0L, 0.0L,
                                                 0.0L, 0.0L, 0.0L,
                                                 0.0L, 0.0L, 0.0L };
    int                             family   = 0;
    size_t                          i        = 0u;

    for (i = 0u; i < _t->n_in; ++i)
    {
        in[i] = d_tests_part(&row->in[i]);
    }

    for (family = 0; family < 3; ++family)
    {
        long double got[2][9];
        int         form = 0;

        // a row the family does not hold, or long double's own where long
        // double is not the x87's
        if ( ((row->mask & masks[family]) == 0) ||
             ( (family == 2) &&
               (LDBL_MANT_DIG != 64) &&
               ((row->mask & 2) == 0) ) )
        {
            continue;
        }

        for (form = 0; form < _t->forms; ++form)
        {
            unsigned long worst  = 0ul;
            unsigned long excess = 0ul;

            d_tests_call(family, _fn, form, in, got[form]);
            worst = d_tests_distance(_t, row, family, form, got[form], in,
                                     &excess);

            if (worst > g_worst[_fn][form][family])
            {
                g_worst[_fn][form][family] = worst;
            }

            ++g_rows[_fn][form][family];

            // past the bound, and any allowance: which row
            if (excess > g_bounds[_fn][form][family])
            {
                char what[96];

                sprintf(what, "%s form %d family %d row %lu: %lu", _t->name,
                        form, family, (unsigned long)_r, worst);
                D_TESTS_EXPECT(false, what);
            }
            else
            {
                D_TESTS_EXPECT(true, _t->name);
            }
        }

        // the forms of the same bits
        if (_t->same_bits)
        {
            bool same = true;

            for (i = 0u; i < _t->n_out; ++i)
            {
                same = ( (same) &&
                         (d_tests_same_bits(got[0][i], got[1][i])) );
            }

            D_TESTS_EXPECT(same, "the _c and plain forms' bits");
        }
    }

    return;
}

// d_tests_references
//   every table's rows (d_tests_reference_row), then the largest distances,
// for the record.
static void
d_tests_references(void)
{
    int fn = 0;

    for (fn = 0; fn < D_TESTS_FUNCTIONS; ++fn)
    {
        size_t r = 0u;

        for (r = 0u; r < g_tables[fn].count; ++r)
        {
            d_tests_reference_row(&g_tables[fn], fn, r);
        }
    }

    // the largest distances, for the record
    printf("t_quat: steps from the correctly rounded part, the largest\n");
    printf("  %-17s %-6s %8s %8s %8s %6s\n", "function", "form", "float",
           "double", "ldouble", "rows");

    for (fn = 0; fn < D_TESTS_FUNCTIONS; ++fn)
    {
        int form = 0;

        for (form = 0; form < g_tables[fn].forms; ++form)
        {
            printf("  %-17s %-6s %8lu %8lu %8lu %6lu%s\n", g_tables[fn].name,
                   (g_tables[fn].forms == 1) ? "one" :
                   ((form == 0) ? "_c" : "plain"),
                   g_worst[fn][form][0], g_worst[fn][form][1],
                   g_worst[fn][form][2],
                   g_rows[fn][form][0] + g_rows[fn][form][1] +
                   g_rows[fn][form][2],
                   g_tables[fn].normwise[form] ?
                       "  (of the largest part)" : "");
        }
    }

    return;
}


//==============================================================================
// 2.  EACH FAMILY
//==============================================================================

// one family's test, stamped for each: T its type, S its suffix, E a
// tolerance for the identities that hold only to rounding, X the binade of
// its largest finite value, and Y a power of 2, relative to a sum's, that
// the kernels' pairs keep and the family does not: below its half step, and
// above the pairs' precision, double's for float, twice its own otherwise
#define D_TESTS_QUAT_FAMILY(T, S, E, X, Y)                                     \
static bool                                                                    \
d_tests_near_##S(const T* _x, const T* _y, size_t _n, T _tol)                  \
{                                                                              \
    size_t k = 0u;                                                             \
                                                                               \
    for (k = 0u; k < _n; ++k)                                                  \
    {                                                                          \
        const T d = _x[k] - _y[k];                                             \
                                                                               \
        if (!(((d < (T)0) ? -d : d) <= _tol))                                  \
        {                                                                      \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
                                                                               \
    return true;                                                               \
}                                                                              \
                                                                               \
/* bit for bit, as stored: read through volatile, so that a value kept in */   \
/* an x87 register is not compared at its 64 bits; a zero's sign counted, */   \
/* and any NaN the same as any other */                                        \
static bool                                                                    \
d_tests_same_##S(const volatile T* _x, const volatile T* _y, size_t _n)        \
{                                                                              \
    size_t k = 0u;                                                             \
                                                                               \
    for (k = 0u; k < _n; ++k)                                                  \
    {                                                                          \
        if (!d_tests_same_bits((long double)_x[k], (long double)_y[k]))        \
        {                                                                      \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
                                                                               \
    return true;                                                               \
}                                                                              \
                                                                               \
/* each part a NaN */                                                          \
static bool                                                                    \
d_tests_nans_##S(const T* _x, size_t _n)                                       \
{                                                                              \
    size_t k = 0u;                                                             \
                                                                               \
    for (k = 0u; k < _n; ++k)                                                  \
    {                                                                          \
        if (_x[k] == _x[k])                                                    \
        {                                                                      \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
                                                                               \
    return true;                                                               \
}                                                                              \
                                                                               \
/* a part's sign bit */                                                        \
static bool                                                                    \
d_tests_negative_##S(T _x)                                                     \
{                                                                              \
    return ( (_x < (T)0) ||                                                    \
             ( (_x == (T)0) &&                                                 \
               (((T)1 / _x) < (T)0) ) );                                       \
}                                                                              \
                                                                               \
/* the _fast forms: the former formulas, bit for bit */                        \
static void                                                                    \
d_tests_fast_##S(const T* a, const T* b, const T* v)                           \
{                                                                              \
    T out[16], want[16], m[9];                                                 \
                                                                               \
    /* the Hamilton product, as the headers wrote it */                        \
    d_quat_##S##_multiply_fast(out, a, b);                                     \
    {                                                                          \
        const T ax = a[0], ay = a[1], az = a[2], aw = a[3];                    \
        const T bx = b[0], by = b[1], bz = b[2], bw = b[3];                    \
        T p = (T)0, q = (T)0, r = (T)0, t = (T)0;                              \
        p = aw * bx; q = ax * bw; r = p + q; p = ay * bz; r = r + p;           \
        t = az * by; want[0] = r - t;                                          \
        p = aw * by; q = ax * bz; r = p - q; p = ay * bw; r = r + p;           \
        t = az * bx; want[1] = r + t;                                          \
        p = aw * bz; q = ax * by; r = p + q; p = ay * bx; r = r - p;           \
        t = az * bw; want[2] = r + t;                                          \
        p = aw * bw; q = ax * bx; r = p - q; p = ay * by; r = r - p;           \
        t = az * bz; want[3] = r - t;                                          \
    }                                                                          \
    D_TESTS_EXPECT(d_tests_same_##S(out, want, 4u),                            \
                   #S " multiply_fast is the formula");                        \
                                                                               \
    /* the rotation: v + w t + u x t, t = 2 u x v, by vec.h as before, */      \
    /* each step stored, as the x87 rounds a formula's steps its own way */    \
    d_quat_##S##_rotate_fast(out, a, v);                                       \
    {                                                                          \
        T t[3], wt[3], ut[3];                                                  \
        d_vec_##S##_cross(t, a, v);                                            \
        d_vec_##S##_scale(t, t, (T)2, 3u);                                     \
        d_vec_##S##_scale(wt, t, a[3], 3u);                                    \
        d_vec_##S##_cross(ut, a, t);                                           \
        d_vec_##S##_add(want, v, wt, 3u);                                      \
        d_vec_##S##_add(want, want, ut, 3u);                                   \
    }                                                                          \
    D_TESTS_EXPECT(d_tests_same_##S(out, want, 3u),                            \
                   #S " rotate_fast is the formula");                          \
                                                                               \
    /* the matrix: 1 - 2 (y^2 + z^2) and so on */                              \
    d_quat_##S##_to_matrix3_fast(m, a);                                        \
    {                                                                          \
        const T x = a[0], y = a[1], z = a[2], w = a[3];                        \
        const T xx = x * x, yy = y * y, zz = z * z;                            \
        const T xy = x * y, xz = x * z, yz = y * z;                            \
        const T wx = w * x, wy = w * y, wz = w * z;                            \
        const T s00 = yy + zz, s11 = xx + zz, s22 = xx + yy;                   \
        const T d01 = xy - wz, d02 = xz + wy, d10 = xy + wz;                   \
        const T d12 = yz - wx, d20 = xz - wy, d21 = yz + wx;                   \
        want[0] = (T)1 - ((T)2 * s00); want[1] = (T)2 * d01;                   \
        want[2] = (T)2 * d02;          want[3] = (T)2 * d10;                   \
        want[4] = (T)1 - ((T)2 * s11); want[5] = (T)2 * d12;                   \
        want[6] = (T)2 * d20;          want[7] = (T)2 * d21;                   \
        want[8] = (T)1 - ((T)2 * s22);                                         \
    }                                                                          \
    D_TESTS_EXPECT(d_tests_same_##S(m, want, 9u),                              \
                   #S " to_matrix3_fast is the formula");                      \
    d_quat_##S##_to_matrix4_fast(out, a);                                      \
    D_TESTS_EXPECT( (out[0] == m[0]) && (out[5] == m[4]) &&                    \
                    (out[10] == m[8]) && (out[15] == (T)1) &&                  \
                    (out[3] == (T)0) && (out[12] == (T)0),                     \
                    #S " the fast 4x4 holds the 3x3");                         \
                                                                               \
    /* normalization by the reciprocal root */                                 \
    d_quat_##S##_normalize_fast(out, a);                                       \
    {                                                                          \
        const T xx = a[0] * a[0], yy = a[1] * a[1];                            \
        const T zz = a[2] * a[2], ww = a[3] * a[3];                            \
        const T s1 = xx + yy, s2 = s1 + zz, s3 = s2 + ww;                      \
        const T k  = (T)1 / d_math_##S##_sqrt(s3);                             \
        size_t  i  = 0u;                                                       \
        for (i = 0u; i < 4u; ++i) { want[i] = a[i] * k; }                      \
    }                                                                          \
    D_TESTS_EXPECT(d_tests_same_##S(out, want, 4u),                            \
                   #S " normalize_fast is the formula");                       \
                                                                               \
    return;                                                                    \
}                                                                              \
                                                                               \
/* a complex number's functions: cplx.h's, bit for bit, y and z as given, */   \
/* a real number's too, x 0 of either sign */                                  \
static void                                                                    \
d_tests_complex_##S(const T* a, long trial)                                    \
{                                                                              \
    const struct dj_cplx_##S p1 = d_cplx_##S##_make((T)2.5, (T)0);             \
    const struct dj_cplx_##S p2 = d_cplx_##S##_make((T)-1.5, (T)0);            \
    bool                     same = true;                                      \
    int                      point = 0;                                        \
                                                                               \
    for (point = 0; point < 2; ++point)                                        \
    {                                                                          \
        const T x = (point == 0) ? a[0] : (((trial / 4) % 2) ? (T)0 : -(T)0);  \
        const T z[4] = { x, ((trial / 2) % 2) ? (T)0 : -(T)0,                  \
                         (trial % 2) ? (T)0 : -(T)0, a[3] };                   \
        const struct dj_cplx_##S zc = d_cplx_##S##_make(a[3], x);              \
        struct dj_cplx_##S       cc = zc;                                      \
        T                        out[4];                                       \
                                                                               \
        cc = d_cplx_##S##_exp_c(zc);                                           \
        d_tests_op_##S(D_TESTS_EXP, 0, z, z, (T)0, out);                       \
        same = same && d_tests_same_##S(out, cc.part + 1, 1u) &&               \
               d_tests_same_##S(out + 3, cc.part, 1u) &&                       \
               d_tests_same_##S(out + 1, z + 1, 2u);                           \
        cc = d_cplx_##S##_exp(zc);                                             \
        d_tests_op_##S(D_TESTS_EXP, 1, z, z, (T)0, out);                       \
        same = same && d_tests_same_##S(out, cc.part + 1, 1u) &&               \
               d_tests_same_##S(out + 3, cc.part, 1u) &&                       \
               d_tests_same_##S(out + 1, z + 1, 2u);                           \
        cc = d_cplx_##S##_log_c(zc);                                           \
        d_tests_op_##S(D_TESTS_LOG, 0, z, z, (T)0, out);                       \
        same = same && d_tests_same_##S(out, cc.part + 1, 1u) &&               \
               d_tests_same_##S(out + 3, cc.part, 1u) &&                       \
               d_tests_same_##S(out + 1, z + 1, 2u);                           \
        cc = d_cplx_##S##_log(zc);                                             \
        d_tests_op_##S(D_TESTS_LOG, 1, z, z, (T)0, out);                       \
        same = same && d_tests_same_##S(out, cc.part + 1, 1u) &&               \
               d_tests_same_##S(out + 3, cc.part, 1u) &&                       \
               d_tests_same_##S(out + 1, z + 1, 2u);                           \
        cc = d_cplx_##S##_sqrt_c(zc);                                          \
        d_tests_op_##S(D_TESTS_SQRT, 0, z, z, (T)0, out);                      \
        same = same && d_tests_same_##S(out, cc.part + 1, 1u) &&               \
               d_tests_same_##S(out + 3, cc.part, 1u) &&                       \
               d_tests_same_##S(out + 1, z + 1, 2u);                           \
        cc = d_cplx_##S##_sqrt(zc);                                            \
        d_tests_op_##S(D_TESTS_SQRT, 1, z, z, (T)0, out);                      \
        same = same && d_tests_same_##S(out, cc.part + 1, 1u) &&               \
               d_tests_same_##S(out + 3, cc.part, 1u) &&                       \
               d_tests_same_##S(out + 1, z + 1, 2u);                           \
        cc = d_cplx_##S##_pow_c(zc, p1);                                       \
        d_tests_op_##S(D_TESTS_POW, 0, z, z, (T)2.5, out);                     \
        same = same && d_tests_same_##S(out, cc.part + 1, 1u) &&               \
               d_tests_same_##S(out + 3, cc.part, 1u) &&                       \
               d_tests_same_##S(out + 1, z + 1, 2u);                           \
        cc = d_cplx_##S##_pow(zc, p2);                                         \
        d_tests_op_##S(D_TESTS_POW, 1, z, z, (T)-1.5, out);                    \
        same = same && d_tests_same_##S(out, cc.part + 1, 1u) &&               \
               d_tests_same_##S(out + 3, cc.part, 1u) &&                       \
               d_tests_same_##S(out + 1, z + 1, 2u);                           \
    }                                                                          \
                                                                               \
    D_TESTS_EXPECT(same, #S " a complex number's functions are cplx.h's");     \
                                                                               \
    return;                                                                    \
}                                                                              \
                                                                               \
/* the identities, on random quaternions */                                    \
static void                                                                    \
d_tests_identities_##S(void)                                                   \
{                                                                              \
    const T one[4] = { (T)0, (T)0, (T)0, (T)1 };                               \
    long    trial  = 0;                                                        \
                                                                               \
    for (trial = 0; trial < 20000; ++trial)                                    \
    {                                                                          \
        T      a[4], b[4], c[4], out[4], want[4], v[3], rv[3], m[9], mv[3];    \
        T      m4[16];                                                         \
        size_t i = 0u;                                                         \
                                                                               \
        for (i = 0u; i < 4u; ++i)                                              \
        {                                                                      \
            a[i] = (T)(d_tests_random() / 4.0);                                \
            b[i] = (T)(d_tests_random() / 4.0);                                \
        }                                                                      \
                                                                               \
        for (i = 0u; i < 3u; ++i)                                              \
        {                                                                      \
            v[i] = (T)d_tests_random();                                        \
        }                                                                      \
                                                                               \
        d_tests_fast_##S(a, b, v);                                             \
        d_tests_complex_##S(a, trial);                                         \
                                                                               \
        /* q q*: the vector parts 0 to Dot2's residue, 2^-100 of |q|^2 */    \
        d_quat_##S##_conjugate(c, a);                                          \
        d_tests_op_##S(D_TESTS_MULTIPLY, 0, a, c, (T)0, out);                  \
        want[0] = d_vec_##S##_norm_squared(a, 4u);                             \
        want[1] = want[0] * (T)7.8886090522101180541e-31L;                     \
        D_TESTS_EXPECT( (!(((out[0] < (T)0) ? -out[0] : out[0]) > want[1])) && \
                        (!(((out[1] < (T)0) ? -out[1] : out[1]) > want[1])) && \
                        (!(((out[2] < (T)0) ? -out[2] : out[2]) > want[1])) && \
                        d_tests_near_##S(out + 3, want, 1u, (T)E),             \
                        #S " q q* = |q|^2");                                   \
                                                                               \
        /* q q^-1, q / q and q \ q: 1, within a step of each part */           \
        d_tests_op_##S(D_TESTS_INVERSE, 0, a, a, (T)0, c);                     \
        d_tests_op_##S(D_TESTS_MULTIPLY, 0, a, c, (T)0, out);                  \
        D_TESTS_EXPECT(d_tests_near_##S(out, one, 4u, (T)E),                   \
                       #S " q q^-1 = 1");                                      \
        d_tests_op_##S(D_TESTS_DIVIDE, 0, a, a, (T)0, out);                    \
        D_TESTS_EXPECT(d_tests_near_##S(out, one, 4u, (T)E),                   \
                       #S " q / q = 1");                                       \
        d_tests_op_##S(D_TESTS_LEFT_DIVIDE, 0, a, a, (T)0, out);               \
        D_TESTS_EXPECT(d_tests_near_##S(out, one, 4u, (T)E),                   \
                       #S " q \\ q = 1");                                      \
                                                                               \
        /* (a b) / b = a and a \ (a b) = b */                                  \
        d_tests_op_##S(D_TESTS_MULTIPLY, 0, a, b, (T)0, c);                    \
        d_tests_op_##S(D_TESTS_DIVIDE, 0, c, b, (T)0, out);                    \
        D_TESTS_EXPECT(d_tests_near_##S(out, a, 4u, (T)(E * 64)),              \
                       #S " (a b) / b = a");                                   \
        d_tests_op_##S(D_TESTS_LEFT_DIVIDE, 0, a, c, (T)0, out);               \
        D_TESTS_EXPECT(d_tests_near_##S(out, b, 4u, (T)(E * 64)),              \
                       #S " a \\ (a b) = b");                                  \
                                                                               \
        /* the rotation: its matrix's product; q, -q and 2^k q alike */        \
        d_tests_op_##S(D_TESTS_ROTATE, 0, a, v, (T)0, rv);                     \
        d_tests_op_##S(D_TESTS_TO_MATRIX3, 0, a, a, (T)0, m);                  \
        d_mat_##S##_multiply_vector(mv, m, v, 3u, 3u);                         \
        D_TESTS_EXPECT(d_tests_near_##S(rv, mv, 3u, (T)(E * 16)),              \
                       #S " rotation is the matrix's product");                \
        d_vec_##S##_negate(c, a, 4u);                                          \
        d_tests_op_##S(D_TESTS_ROTATE, 0, c, v, (T)0, mv);                     \
        D_TESTS_EXPECT(d_tests_same_##S(rv, mv, 3u),                           \
                       #S " q and -q are the same rotation");                  \
        d_vec_##S##_scale(c, a, (T)1024, 4u);                                  \
        d_tests_op_##S(D_TESTS_ROTATE, 0, c, v, (T)0, mv);                     \
        D_TESTS_EXPECT(d_tests_same_##S(rv, mv, 3u),                           \
                       #S " q and 2^10 q are the same rotation");              \
        d_quat_##S##_to_matrix4(m4, a);                                        \
        D_TESTS_EXPECT( (m4[0] == m[0]) && (m4[5] == m[4]) &&                  \
                        (m4[10] == m[8]) && (m4[15] == (T)1) &&                \
                        (m4[3] == (T)0) && (m4[12] == (T)0),                   \
                        #S " the 4x4 holds the 3x3");                          \
                                                                               \
        /* Shepperd's method of the 4x4: the 3x3's, bit for bit */             \
        {                                                                      \
            T q3[4], q4[4];                                                    \
            d_quat_##S##_from_matrix3_c(q3, m);                                \
            d_quat_##S##_from_matrix4_c(q4, m4);                               \
            D_TESTS_EXPECT(d_tests_same_##S(q3, q4, 4u),                       \
                           #S " from_matrix4_c is from_matrix3_c's");          \
            d_quat_##S##_from_matrix3(q3, m);                                  \
            d_quat_##S##_from_matrix4(q4, m4);                                 \
            D_TESTS_EXPECT(d_tests_same_##S(q3, q4, 4u),                       \
                           #S " from_matrix4 is from_matrix3's");              \
        }                                                                      \
                                                                               \
        /* parts of 0, 1/2, 1 and 2 of either sign: every product and sum */   \
        /* exact, so the product is the formula's, a zero's sign too */        \
        {                                                                      \
            static const T small[8] = { (T)0, -(T)0, (T)1, (T)-1,              \
                                        (T)2, (T)-2, (T)0.5, (T)-0.5 };        \
            T p[4], q[4], fast[4];                                             \
            for (i = 0u; i < 4u; ++i)                                          \
            {                                                                  \
                p[i] = small[(trial + (long)(3u * i)) % 8];                    \
                q[i] = small[((trial / 8) + (long)(5u * i)) % 8];              \
                if ((trial % 3) == (long)(i % 3u))                             \
                {                                                              \
                    q[i] = (trial % 2) ? (T)0 : -(T)0;                         \
                }                                                              \
            }                                                                  \
            d_tests_op_##S(D_TESTS_MULTIPLY, 0, p, q, (T)0, out);              \
            d_quat_##S##_multiply_fast(fast, p, q);                            \
            D_TESTS_EXPECT(d_tests_same_##S(out, fast, 4u),                    \
                           #S " an exact product is the formula's");           \
        }                                                                      \
                                                                               \
        /* Shepperd's method inverts the matrix: q / |q| with w >= 0 */        \
        d_tests_op_##S(D_TESTS_FROM_MATRIX3, 0, m, m, (T)0, out);              \
        d_tests_op_##S(D_TESTS_NORMALIZE, 0, a, a, (T)0, want);                \
        if (want[3] < (T)0)                                                    \
        {                                                                      \
            d_vec_##S##_negate(want, want, 4u);                                \
        }                                                                      \
        D_TESTS_EXPECT(d_tests_near_##S(out, want, 4u, (T)(E * 16)),           \
                       #S " from_matrix3 inverts to_matrix3");                 \
                                                                               \
        /* two vectors' quaternion turns the one into the other */             \
        d_tests_op_##S(D_TESTS_FROM_TWO_VECTORS, 0, v, b, (T)0, out);          \
        d_tests_op_##S(D_TESTS_ROTATE, 0, out, v, (T)0, rv);                   \
        {                                                                      \
            const T nv = d_vec_##S##_norm(v, 3u);                              \
            const T nb = d_vec_##S##_norm(b, 3u);                              \
            for (i = 0u; i < 3u; ++i)                                          \
            {                                                                  \
                rv[i] = rv[i] / nv;                                            \
                mv[i] = b[i] / nb;                                             \
            }                                                                  \
        }                                                                      \
        D_TESTS_EXPECT(d_tests_near_##S(rv, mv, 3u, (T)(E * 64)),              \
                       #S " from_two_vectors turns a into b");                 \
                                                                               \
        /* exp(log q) = q; sqrt(q)^2 = q; q^2 = q q; q^(1/2) = sqrt q */       \
        d_tests_op_##S(D_TESTS_LOG, 0, a, a, (T)0, c);                         \
        d_tests_op_##S(D_TESTS_EXP, 0, c, c, (T)0, out);                       \
        D_TESTS_EXPECT(d_tests_near_##S(out, a, 4u, (T)(E * 16)),              \
                       #S " exp(log q) = q");                                  \
        d_tests_op_##S(D_TESTS_SQRT, 0, a, a, (T)0, c);                        \
        d_tests_op_##S(D_TESTS_MULTIPLY, 0, c, c, (T)0, out);                  \
        D_TESTS_EXPECT(d_tests_near_##S(out, a, 4u, (T)(E * 16)),              \
                       #S " sqrt(q)^2 = q");                                   \
        d_tests_op_##S(D_TESTS_POW, 0, a, a, (T)2, out);                       \
        d_tests_op_##S(D_TESTS_MULTIPLY, 0, a, a, (T)0, want);                 \
        D_TESTS_EXPECT(d_tests_near_##S(out, want, 4u, (T)(E * 16)),           \
                       #S " q^2 = q q");                                       \
        d_tests_op_##S(D_TESTS_POW, 0, a, a, (T)0.5, out);                     \
        D_TESTS_EXPECT(d_tests_near_##S(out, c, 4u, (T)(E * 16)),              \
                       #S " q^(1/2) = sqrt q");                                \
                                                                               \
        /* the plain forms near the _c ones */                                 \
        d_tests_op_##S(D_TESTS_EXP, 0, a, a, (T)0, c);                         \
        d_tests_op_##S(D_TESTS_EXP, 1, a, a, (T)0, out);                       \
        D_TESTS_EXPECT(d_tests_near_##S(out, c, 4u, (T)(E * 16)),              \
                       #S " exp near exp_c");                                  \
        d_tests_op_##S(D_TESTS_SLERP, 0, a, b, (T)0.25, c);                    \
        d_tests_op_##S(D_TESTS_SLERP, 1, a, b, (T)0.25, out);                  \
        D_TESTS_EXPECT(d_tests_near_##S(out, c, 4u, (T)(E * 16)),              \
                       #S " slerp near slerp_c");                              \
                                                                               \
        /* the axis-angle constructions, as before */                          \
        {                                                                      \
            const T angle = (T)d_tests_random();                               \
            T       qc[4], qr[4];                                              \
            d_quat_##S##_from_axis_angle_c(qc, v, angle);                      \
            d_quat_##S##_from_axis_angle(qr, v, angle);                        \
            D_TESTS_EXPECT(d_tests_near_##S(qc, qr, 4u, (T)(E / 64)),          \
                           #S " from_axis_angle_c near from_axis_angle");      \
        }                                                                      \
                                                                               \
        /* nlerp's and slerp's ends: a, and b or -b, exactly */                \
        d_tests_op_##S(D_TESTS_NORMALIZE, 0, a, a, (T)0, a);                   \
        d_tests_op_##S(D_TESTS_NORMALIZE, 0, b, b, (T)0, b);                   \
        d_tests_op_##S(D_TESTS_SLERP, 0, a, b, (T)0, out);                     \
        D_TESTS_EXPECT(d_tests_same_##S(out, a, 4u), #S " slerp_c at 0");      \
        d_tests_op_##S(D_TESTS_SLERP, 0, a, b, (T)1, out);                     \
        if (d_vec_##S##_dot(a, b, 4u) < (T)0)                                  \
        {                                                                      \
            d_vec_##S##_negate(c, b, 4u);                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            d_vec_##S##_scale(c, b, (T)1, 4u);                                 \
        }                                                                      \
        D_TESTS_EXPECT(d_tests_same_##S(out, c, 4u), #S " slerp_c at 1");      \
        d_tests_op_##S(D_TESTS_SLERP, 0, a, a, (T)0.375, out);                 \
        D_TESTS_EXPECT(d_tests_same_##S(out, a, 4u),                           \
                       #S " slerp_c between a and a");                         \
        d_tests_op_##S(D_TESTS_NLERP, 0, a, b, (T)0, out);                     \
        d_tests_op_##S(D_TESTS_NORMALIZE, 0, a, a, (T)0, want);                \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, 4u), #S " nlerp_c at 0");   \
        d_tests_op_##S(D_TESTS_NLERP, 0, a, b, (T)1, out);                     \
        d_tests_op_##S(D_TESTS_NORMALIZE, 0, c, c, (T)0, want);                \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, 4u), #S " nlerp_c at 1");   \
    }                                                                          \
                                                                               \
    return;                                                                    \
}                                                                              \
                                                                               \
/* the special values, as quat.h states them */                                \
static void                                                                    \
d_tests_special_##S(void)                                                      \
{                                                                              \
    const T inf      = d_internal_math_##S##_infinity();                       \
    const T nan      = d_internal_math_##S##_nan();                            \
    const T pi       = (T)3.14159265358979323846264338327950288L;              \
    const T zero4[4] = { (T)0, -(T)0, (T)0, (T)0 };                            \
    const T id[4]    = { (T)0, (T)0, (T)0, (T)1 };                             \
    const T q[4]     = { (T)0.5, (T)-0.25, (T)0.75, (T)1.5 };                  \
    const T v[3]     = { (T)1, (T)-2, (T)3 };                                  \
    const T bad[4]   = { inf, (T)1, (T)-2, (T)0.5 };                           \
    const T nq[4]    = { (T)1, nan, (T)0, (T)2 };                              \
    T       out[9], want[9];                                                   \
                                                                               \
    /* an infinity or a NaN in the product: the formula as written */          \
    d_tests_op_##S(D_TESTS_MULTIPLY, 0, bad, q, (T)0, out);                    \
    d_quat_##S##_multiply_fast(want, bad, q);                                  \
    D_TESTS_EXPECT(d_tests_same_##S(out, want, 4u),                            \
                   #S " an infinite factor: the formula");                     \
    d_tests_op_##S(D_TESTS_MULTIPLY, 0, q, nq, (T)0, out);                     \
    d_quat_##S##_multiply_fast(want, q, nq);                                   \
    D_TESTS_EXPECT(d_tests_same_##S(out, want, 4u),                            \
                   #S " a NaN factor: the formula");                           \
                                                                               \
    /* the inverse: NaNs of 0 and NaN, the conjugate's zeros of inf */         \
    d_tests_op_##S(D_TESTS_INVERSE, 0, zero4, zero4, (T)0, out);               \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " 1 / 0 is NaN");             \
    d_tests_op_##S(D_TESTS_INVERSE, 0, nq, nq, (T)0, out);                     \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " 1 / NaN is NaN");           \
    d_tests_op_##S(D_TESTS_INVERSE, 0, bad, bad, (T)0, out);                   \
    D_TESTS_EXPECT( (out[0] == (T)0) && d_tests_negative_##S(out[0]) &&        \
                    d_tests_negative_##S(out[1]) &&                            \
                    (!d_tests_negative_##S(out[2])) &&                         \
                    (!d_tests_negative_##S(out[3])) && (out[3] == (T)0),       \
                    #S " 1 / inf is the conjugate's zeros");                   \
    d_tests_op_##S(D_TESTS_DIVIDE, 0, q, zero4, (T)0, out);                    \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " q / 0 is NaN");             \
    d_tests_op_##S(D_TESTS_DIVIDE, 0, q, bad, (T)0, out);                      \
    D_TESTS_EXPECT( (out[0] == (T)0) && (out[1] == (T)0) &&                    \
                    (out[2] == (T)0) && (out[3] == (T)0),                      \
                    #S " q / inf is 0");                                       \
    d_tests_op_##S(D_TESTS_LEFT_DIVIDE, 0, zero4, q, (T)0, out);               \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " 0 \\ q is NaN");            \
                                                                               \
    /* norms and normalization */                                              \
    D_TESTS_EXPECT(d_tests_norm_##S(0, bad) == inf, #S " |inf| = inf");        \
    D_TESTS_EXPECT(d_tests_norm_##S(1, nq) != d_tests_norm_##S(1, nq),         \
                   #S " |NaN| = NaN");                                         \
    D_TESTS_EXPECT( (d_tests_norm_##S(0, zero4) == (T)0) &&                    \
                    (!d_tests_negative_##S(d_tests_norm_##S(0, zero4))),       \
                    #S " |0| = +0");                                           \
    d_tests_op_##S(D_TESTS_NORMALIZE, 0, zero4, zero4, (T)0, out);             \
    D_TESTS_EXPECT(d_tests_same_##S(out, zero4, 4u),                           \
                   #S " 0 normalizes to itself");                              \
    {                                                                          \
        const T two[4] = { inf, (T)1, -inf, -(T)0 };                           \
        const T r2     = (T)0.707106781186547524400844362104849039L;           \
        d_tests_op_##S(D_TESTS_NORMALIZE, 1, two, two, (T)0, out);             \
        D_TESTS_EXPECT( (out[0] == r2) && (out[1] == (T)0) &&                  \
                        (out[2] == -r2) && (out[3] == (T)0) &&                 \
                        d_tests_negative_##S(out[3]),                          \
                        #S " normalize along the infinities");                 \
    }                                                                          \
    d_tests_op_##S(D_TESTS_NORMALIZE, 0, nq, nq, (T)0, out);                   \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " normalize NaN");            \
                                                                               \
    /* rotations: none of 0, NaN or inf; the zero vector stays */              \
    d_tests_op_##S(D_TESTS_ROTATE, 0, zero4, v, (T)0, out);                    \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 3u), #S " rotate by 0");              \
    d_tests_op_##S(D_TESTS_ROTATE, 0, bad, v, (T)0, out);                      \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 3u), #S " rotate by inf");            \
    d_tests_op_##S(D_TESTS_ROTATE, 0, q, zero4, (T)0, out);                    \
    D_TESTS_EXPECT(d_tests_same_##S(out, zero4, 3u),                           \
                   #S " the zero vector stays");                               \
    d_tests_op_##S(D_TESTS_TO_MATRIX3, 0, zero4, zero4, (T)0, out);            \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 9u), #S " the matrix of 0");          \
    d_tests_op_##S(D_TESTS_TO_MATRIX3, 0, id, id, (T)0, out);                  \
    want[0] = (T)1; want[1] = (T)0; want[2] = (T)0;                            \
    want[3] = (T)0; want[4] = (T)1; want[5] = (T)0;                            \
    want[6] = (T)0; want[7] = (T)0; want[8] = (T)1;                            \
    D_TESTS_EXPECT(d_tests_same_##S(out, want, 9u),                            \
                   #S " the identity's matrix");                               \
    {                                                                          \
        const T m[9] = { (T)1, (T)0, (T)0, (T)0, nan,                          \
                         (T)0, (T)0, (T)0, (T)1 };                             \
        d_tests_op_##S(D_TESTS_FROM_MATRIX3, 0, m, m, (T)0, out);              \
        D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " Shepperd of NaN");      \
        d_tests_op_##S(D_TESTS_FROM_MATRIX3, 1, want, want, (T)0, out);        \
        D_TESTS_EXPECT(d_tests_same_##S(out, id, 4u),                          \
                       #S " Shepperd of the identity");                        \
    }                                                                          \
    d_tests_op_##S(D_TESTS_FROM_TWO_VECTORS, 0, zero4, v, (T)0, out);          \
    D_TESTS_EXPECT(d_tests_same_##S(out, id, 4u),                              \
                   #S " two vectors, one 0: the identity");                    \
    d_tests_op_##S(D_TESTS_FROM_TWO_VECTORS, 1, v, bad, (T)0, out);            \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 4u),                                  \
                   #S " two vectors, one infinite");                           \
    {                                                                          \
        const T x[3]  = { (T)1, (T)0, (T)0 };                                  \
        const T mx[3] = { (T)-3, (T)0, (T)0 };                                 \
        d_tests_op_##S(D_TESTS_FROM_TWO_VECTORS, 0, x, mx, (T)0, out);         \
        D_TESTS_EXPECT( (out[0] == (T)0) && (out[1] == (T)0) &&                \
                        (out[2] == (T)1) && (out[3] == (T)0),                  \
                        #S " opposed: a half turn about x times e_y");         \
    }                                                                          \
                                                                               \
    /* nlerp and slerp of a NaN or infinity */                                 \
    d_tests_op_##S(D_TESTS_SLERP, 0, q, bad, (T)0.5, out);                     \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " slerp_c of inf");           \
    d_tests_op_##S(D_TESTS_NLERP, 1, q, id, nan, out);                         \
    D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " nlerp at NaN");             \
                                                                               \
    /* exp, log, sqrt, pow: principal values and the plane's limits */         \
    {                                                                          \
        const T minus1[4]  = { (T)0, (T)0, (T)0, (T)-1 };                      \
        const T minus1n[4] = { -(T)0, (T)0, (T)0, (T)-1 };                     \
        const T vinf[4]    = { (T)0, (T)0, inf, (T)0 };                        \
        const T winf[4]    = { (T)0, (T)1, (T)0, inf };                        \
        const T vnan[4]    = { (T)1, nan, (T)0, (T)0 };                        \
        d_tests_op_##S(D_TESTS_LOG, 0, minus1, minus1, (T)0, out);             \
        D_TESTS_EXPECT( (out[0] == pi) && (out[3] == (T)0),                    \
                        #S " log(-1) = pi im");                                \
        d_tests_op_##S(D_TESTS_LOG, 1, minus1n, minus1n, (T)0, out);           \
        D_TESTS_EXPECT( (out[0] == -pi) && (out[3] == (T)0),                   \
                        #S " log(-1 - 0 im) = -pi im");                        \
        d_tests_op_##S(D_TESTS_SQRT, 0, minus1, minus1, (T)0, out);            \
        D_TESTS_EXPECT( (out[0] == (T)1) && (out[3] == (T)0),                  \
                        #S " sqrt(-1) = im");                                  \
        d_tests_op_##S(D_TESTS_EXP, 0, winf, winf, (T)0, out);                 \
        D_TESTS_EXPECT( (out[0] == (T)0) && (!d_tests_negative_##S(out[0])) && \
                        (out[1] == inf) && (out[2] == (T)0) &&                 \
                        (out[3] == inf),                                       \
                        #S " exp(inf + jm): 0 parts stay 0");                  \
        d_tests_op_##S(D_TESTS_EXP, 1, winf, winf, (T)0, out);                 \
        D_TESTS_EXPECT( (out[0] == (T)0) && (out[1] == inf) &&                 \
                        (out[2] == (T)0) && (out[3] == inf),                   \
                        #S " exp(inf + jm), plain");                           \
        d_tests_op_##S(D_TESTS_LOG, 0, vinf, vinf, (T)0, out);                 \
        D_TESTS_EXPECT( (out[0] == (T)0) && (out[1] == (T)0) &&                \
                        (out[2] == pi / (T)2) && (out[3] == inf),              \
                        #S " log(inf km) = inf + pi/2 km");                    \
        d_tests_op_##S(D_TESTS_EXP, 0, vnan, vnan, (T)0, out);                 \
        D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " exp of NaN");           \
        d_tests_op_##S(D_TESTS_POW, 1, vnan, vnan, (T)2, out);                 \
        D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " pow of NaN");           \
        d_tests_op_##S(D_TESTS_POW, 0, q, q, nan, out);                        \
        D_TESTS_EXPECT(d_tests_nans_##S(out, 4u), #S " pow to NaN");           \
    }                                                                          \
                                                                               \
    /* slerp: an eighth turn halfway to a quarter turn */                      \
    {                                                                          \
        const T axis[3] = { (T)0, (T)0, (T)2 };                                \
        const T x[3]    = { (T)1, (T)0, (T)0 };                                \
        const T r8      = (T)0.70710678118654752440L;                          \
        T       quarter[4], qq[4], r[3];                                       \
                                                                               \
        d_quat_##S##_from_axis_angle(quarter, axis,                            \
                                     (T)1.57079632679489661923L);              \
        d_tests_op_##S(D_TESTS_SLERP, 1, id, quarter, (T)0.5, qq);             \
        d_tests_op_##S(D_TESTS_ROTATE, 0, qq, x, (T)0, r);                     \
        D_TESTS_EXPECT( (d_tests_near_##S(r, &r8, 1u, (T)E)) &&                \
                        (d_tests_near_##S(r + 1, &r8, 1u, (T)E)),              \
                        #S " halfway to a quarter turn is an eighth");         \
        d_tests_op_##S(D_TESTS_SLERP, 0, id, quarter, (T)0.5, qq);             \
        d_tests_op_##S(D_TESTS_ROTATE, 0, qq, x, (T)0, r);                     \
        D_TESTS_EXPECT( (d_tests_near_##S(r, &r8, 1u, (T)E)) &&                \
                        (d_tests_near_##S(r + 1, &r8, 1u, (T)E)),              \
                        #S " halfway to a quarter turn, _c");                  \
                                                                               \
        /* a zero axis: the identity, or (0, 0, 0, cos) */                     \
        {                                                                      \
            const T zero[3] = { (T)0, (T)0, (T)0 };                            \
            d_quat_##S##_from_axis_angle(qq, zero, (T)1);                      \
            D_TESTS_EXPECT(d_vec_##S##_equal(qq, id, 4u),                      \
                           #S " a zero axis names no rotation");               \
            d_quat_##S##_from_unit_axis_angle(qq, zero, (T)1);                 \
            D_TESTS_EXPECT( (qq[0] == (T)0) &&                                 \
                            (qq[3] == d_math_##S##_cos((T)0.5)),               \
                            #S " a zero unit axis");                           \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* the product at the range's end: parts past it whose products */         \
    /* cancel, scaled; the midpoint above the largest value, the sum below, */ \
    /* rounds to it, and a sum above it to infinity; a sum on a midpoint */    \
    /* between two values, its low part, 2^Y of it, deciding */                \
    {                                                                          \
        const T    one  = (T)1;                                                \
        const T    big  = d_internal_math_##S##_pow2(X);                       \
        const T    far  = d_internal_math_##S##_pow2((X * 11) / 20);           \
        volatile T top  = d_math_##S##_next_down(big);                         \
        volatile T most = big;                                                 \
        volatile T eps  = d_math_##S##_next_up(one);                           \
        const T    s    = d_internal_math_##S##_pow2(Y / 2);                   \
        const T    tiny = big * d_internal_math_##S##_pow2(Y);                 \
        most = (most - top) + most;                                            \
        most = d_math_##S##_next_down(most + most);                            \
        eps  = eps - one;                                                      \
        {                                                                      \
            const T a1[4] = { far, (T)0, (T)0, far };                          \
            const T b1[4] = { far, (T)0, (T)0, -far };                         \
            const T a2[4] = { top, -tiny, (T)0, big };                         \
            const T a3[4] = { big, -tiny, (T)0, big };                         \
            const T b2[4] = { one, (T)0, one, one };                           \
            const T a4[4] = { eps / (T)2, s, (T)0, one };                      \
            const T a5[4] = { (T)3 * (eps / (T)2), -s, (T)0, one };            \
            const T b4[4] = { one, (T)0, s, one };                             \
            d_tests_op_##S(D_TESTS_MULTIPLY, 0, a1, b1, (T)0, out);            \
            D_TESTS_EXPECT( (out[0] == (T)0) && (out[3] == -inf),              \
                            #S " products past the range that cancel");        \
            d_tests_op_##S(D_TESTS_MULTIPLY, 0, a2, b2, (T)0, out);            \
            D_TESTS_EXPECT(out[0] == most,                                     \
                           #S " the midpoint past the range, the sum below");  \
            d_tests_op_##S(D_TESTS_MULTIPLY, 0, a3, b2, (T)0, out);            \
            D_TESTS_EXPECT(out[0] == inf,                                      \
                           #S " a sum above the midpoint past the range");     \
            d_tests_op_##S(D_TESTS_MULTIPLY, 0, a4, b4, (T)0, out);            \
            D_TESTS_EXPECT(out[0] == d_math_##S##_next_up(one),                \
                           #S " a midpoint, the sum above it");                \
            d_tests_op_##S(D_TESTS_MULTIPLY, 0, a5, b4, (T)0, out);            \
            D_TESTS_EXPECT(out[0] == d_math_##S##_next_up(one),                \
                           #S " a midpoint, the sum below it");                \
        }                                                                      \
                                                                               \
        /* the norm: +inf where a part is infinite, a NaN beside it or not; */ \
        /* past the range, of finite parts, +inf */                            \
        {                                                                      \
            const T infnan[4] = { inf, nan, (T)0, (T)1 };                      \
            const T max2[4]   = { most, most, (T)0, (T)0 };                    \
            const T max3[4]   = { most, most, most, (T)0 };                    \
            D_TESTS_EXPECT( (d_tests_norm_##S(0, infnan) == inf) &&            \
                            (d_tests_norm_##S(1, infnan) == inf),              \
                            #S " |(inf, NaN, 0, 1)| = inf");                   \
            D_TESTS_EXPECT( (d_tests_norm_##S(0, max2) == inf) &&              \
                            (d_tests_norm_##S(1, max2) == inf) &&              \
                            (d_tests_norm_##S(0, max3) == inf) &&              \
                            (d_tests_norm_##S(1, max3) == inf),                \
                            #S " a norm past the range is +inf");              \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* the quotients of an infinite operand: the formula as written, a b^-1 */ \
    /* and a^-1 b */                                                           \
    {                                                                          \
        T inverse[4];                                                          \
        d_quat_##S##_inverse(inverse, q);                                      \
        d_tests_op_##S(D_TESTS_DIVIDE, 0, bad, q, (T)0, out);                  \
        d_quat_##S##_multiply_fast(want, bad, inverse);                        \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, 4u),                        \
                       #S " inf / q: the formula, inf q^-1");                  \
        d_tests_op_##S(D_TESTS_LEFT_DIVIDE, 0, q, bad, (T)0, out);             \
        d_quat_##S##_multiply_fast(want, inverse, bad);                        \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, 4u),                        \
                       #S " q \\ inf: the formula, q^-1 inf");                 \
    }                                                                          \
                                                                               \
    /* two vectors nearly opposed: |a| |b| + a . b as a quotient, exactly */   \
    {                                                                          \
        const T k60   = d_internal_math_##S##_pow2(-60);                       \
        const T x1[3] = { (T)1, (T)0, (T)0 };                                  \
        const T mx[3] = { (T)-1, k60, (T)0 };                                  \
        int     form  = 0;                                                     \
        bool    ok    = true;                                                  \
        for (form = 0; form < 2; ++form)                                       \
        {                                                                      \
            d_tests_op_##S(D_TESTS_FROM_TWO_VECTORS, form, x1, mx, (T)0, out); \
            ok = ok && (out[0] == (T)0) && (out[1] == (T)0) &&                 \
                 (out[2] == (T)1) &&                                           \
                 (out[3] == d_internal_math_##S##_pow2(-61));                  \
        }                                                                      \
        D_TESTS_EXPECT(ok, #S " two vectors opposed but for 2^-60");           \
    }                                                                          \
                                                                               \
    /* exp, log, sqrt and pow: a zero part of v of the sign of u's part */     \
    /* times the value's, the value positive here; log near the negative */    \
    /* real axis, v / w's branch only where w > 0; NaNs of a NaN power or */   \
    /* w, a part of v 0 beside them */                                         \
    {                                                                          \
        static const int fns[4] = { D_TESTS_EXP, D_TESTS_LOG, D_TESTS_SQRT,    \
                                    D_TESTS_POW };                             \
        const T k40    = d_internal_math_##S##_pow2(-40);                      \
        const T q0[4]  = { (T)0.5, -(T)0, (T)0.25, (T)1 };                     \
        const T q1[4]  = { k40, k40, (T)0, (T)-1 };                            \
        const T q2[4]  = { (T)0.5, (T)0, (T)0.75, (T)1.5 };                    \
        const T q3[4]  = { (T)1, (T)0, (T)0.5, nan };                          \
        const T cut    = (T)2.22144146907827362880616756679243181L;            \
        bool    signs  = true;                                                 \
        bool    branch = true;                                                 \
        bool    nans   = true;                                                 \
        int     fn     = 0;                                                    \
        int     form   = 0;                                                    \
        for (form = 0; form < 2; ++form)                                       \
        {                                                                      \
            for (fn = 0; fn < 4; ++fn)                                         \
            {                                                                  \
                d_tests_op_##S(fns[fn], form, q0, q0, (T)2, out);              \
                signs = signs && (out[1] == (T)0) &&                           \
                        d_tests_negative_##S(out[1]);                          \
                d_tests_op_##S(fns[fn], form, q3, q3, (T)2, out);              \
                nans = nans && d_tests_nans_##S(out, 4u);                      \
            }                                                                  \
            d_tests_op_##S(D_TESTS_LOG, form, q1, q1, (T)0, out);              \
            branch = branch && d_tests_near_##S(out, &cut, 1u, (T)(E * 4)) &&  \
                     d_tests_near_##S(out + 1, &cut, 1u, (T)(E * 4));          \
            d_tests_op_##S(D_TESTS_POW, form, q2, q2, nan, out);               \
            nans = nans && d_tests_nans_##S(out, 4u);                          \
        }                                                                      \
        D_TESTS_EXPECT(signs, #S " a 0 part of v: u's sign, the value's");     \
        D_TESTS_EXPECT(branch, #S " log near the cut: v / w where w > 0");     \
        D_TESTS_EXPECT(nans, #S " a NaN power or w beside a 0 part: NaNs");    \
    }                                                                          \
                                                                               \
    /* the special values above in both forms of what takes a root; the */     \
    /* direction of three and four infinities; nlerp from 0 */                 \
    {                                                                          \
        const T m[9]     = { (T)1, (T)0, (T)0, (T)0, nan,                      \
                             (T)0, (T)0, (T)0, (T)1 };                         \
        const T two[4]   = { inf, (T)1, -inf, -(T)0 };                         \
        const T three[4] = { inf, -inf, inf, (T)0 };                           \
        const T four[4]  = { inf, inf, -inf, -inf };                           \
        const T vinf[4]  = { (T)0, (T)0, inf, (T)0 };                          \
        const T r2       = (T)0.707106781186547524400844362104849039L;         \
        const T r3       = (T)0.577350269189625764509148780501957456L;         \
        const T half     = (T)0.5;                                             \
        bool    ok       = true;                                               \
        int     form     = 0;                                                  \
        for (form = 0; form < 2; ++form)                                       \
        {                                                                      \
            const T nn = d_tests_norm_##S(form, nq);                           \
                                                                               \
            ok = ok && (nn != nn);                                             \
            ok = ok && (d_tests_norm_##S(form, zero4) == (T)0) &&              \
                 (!d_tests_negative_##S(d_tests_norm_##S(form, zero4)));       \
            d_tests_op_##S(D_TESTS_NORMALIZE, form, zero4, zero4, (T)0, out);  \
            ok = ok && d_tests_same_##S(out, zero4, 4u);                       \
            d_tests_op_##S(D_TESTS_NORMALIZE, form, nq, nq, (T)0, out);        \
            ok = ok && d_tests_nans_##S(out, 4u);                              \
            d_tests_op_##S(D_TESTS_NORMALIZE, form, two, two, (T)0, out);      \
            ok = ok && (out[0] == r2) && (out[1] == (T)0) &&                   \
                 (out[2] == -r2) && d_tests_negative_##S(out[3]);              \
            d_tests_op_##S(D_TESTS_NORMALIZE, form, three, three, (T)0, out);  \
            ok = ok && (out[0] == r3) && (out[1] == -r3) &&                    \
                 (out[2] == r3) && (out[3] == (T)0);                           \
            d_tests_op_##S(D_TESTS_NORMALIZE, form, four, four, (T)0, out);    \
            ok = ok && (out[0] == half) && (out[1] == half) &&                 \
                 (out[2] == -half) && (out[3] == -half);                       \
            d_tests_op_##S(D_TESTS_FROM_MATRIX3, form, m, m, (T)0, out);       \
            ok = ok && d_tests_nans_##S(out, 4u);                              \
            d_tests_op_##S(D_TESTS_FROM_TWO_VECTORS, form, zero4, v, (T)0,     \
                           out);                                               \
            ok = ok && d_tests_same_##S(out, id, 4u);                          \
            d_tests_op_##S(D_TESTS_FROM_TWO_VECTORS, form, v, bad, (T)0, out); \
            ok = ok && d_tests_nans_##S(out, 4u);                              \
            d_tests_op_##S(D_TESTS_NLERP, form, q, id, nan, out);              \
            ok = ok && d_tests_nans_##S(out, 4u);                              \
            d_tests_op_##S(D_TESTS_NLERP, form, zero4, q, half, out);          \
            d_tests_op_##S(D_TESTS_NORMALIZE, form, q, q, (T)0, want);         \
            ok = ok && d_tests_same_##S(out, want, 4u);                        \
            d_tests_op_##S(D_TESTS_LOG, form, vinf, vinf, (T)0, out);          \
            ok = ok && (out[0] == (T)0) && (out[1] == (T)0) &&                 \
                 (out[2] == pi / (T)2) && (out[3] == inf);                     \
        }                                                                      \
        D_TESTS_EXPECT(ok, #S " the special values in both forms");            \
    }                                                                          \
                                                                               \
    /* small parts: a quotient near the subnormals; 0 / q; 0 times a q */      \
    /* past the safe range; normalize_fast of 0 */                             \
    {                                                                          \
        const T tiny     = d_internal_math_##S##_pow2(-(X - 23));              \
        const T small[4] = { tiny, (T)0, (T)0, (T)1 };                         \
        const T far4[4]  = { d_internal_math_##S##_pow2((X * 11) / 20),        \
                             (T)1, (T)-2, (T)3 };                              \
        d_tests_op_##S(D_TESTS_INVERSE, 0, small, small, (T)0, out);           \
        D_TESTS_EXPECT( (out[0] == -tiny) && (out[3] == (T)1),                 \
                        #S " a quotient near the subnormals");                 \
        d_tests_op_##S(D_TESTS_DIVIDE, 0, zero4, q, (T)0, out);                \
        D_TESTS_EXPECT( (out[0] == (T)0) && (out[1] == (T)0) &&                \
                        (out[2] == (T)0) && (out[3] == (T)0),                  \
                        #S " 0 / q is 0");                                     \
        d_tests_op_##S(D_TESTS_MULTIPLY, 0, zero4, far4, (T)0, out);           \
        d_quat_##S##_multiply_fast(want, zero4, far4);                         \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, 4u),                        \
                       #S " 0 times a q past the safe range");                 \
        d_quat_##S##_normalize_fast(out, zero4);                               \
        D_TESTS_EXPECT(d_tests_same_##S(out, zero4, 4u),                       \
                       #S " normalize_fast leaves 0 as it is");                \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_QUAT_FAMILY(float,       f,  1e-5,  (FLT_MAX_EXP - 1),  -70)
D_TESTS_QUAT_FAMILY(double,      d,  1e-13, (DBL_MAX_EXP - 1),  -104)
D_TESTS_QUAT_FAMILY(long double, ld, 1e-16, (LDBL_MAX_EXP - 1), -126)


//==============================================================================
// 3.  C++: THE OVERLOADS AND CONSTANT EXPRESSIONS
//==============================================================================

#ifdef __cplusplus

// D_TESTS_QUAT_OVERLOADS
//   stamps, for one family, d_tests_overloads_<S>: each name for every
// family, chosen by argument type, is the family's function, bit for bit.
#define D_TESTS_QUAT_OVERLOADS(T, S)                                           \
static void                                                                    \
d_tests_overloads_##S(void)                                                    \
{                                                                              \
    const T a[4] = { (T)0.5, (T)-0.25, (T)0.75, (T)1.5 };                      \
    const T b[4] = { (T)-1.25, (T)0.5, (T)0.375, (T)0.875 };                   \
    const T v[3] = { (T)1, (T)-2, (T)3 };                                      \
    /* a q whose normalization the fast form's rounds otherwise */             \
    const T n[4] = { (T)1, (T)1, (T)1, (T)2 };                                 \
    T       ua[4], ub[4], m3[9], m4[16], o[16], w[16];                         \
    bool    same = true;                                                       \
                                                                               \
    d_quat_##S##_normalize_c(ua, a);                                           \
    d_quat_##S##_normalize_c(ub, b);                                           \
    d_quat_##S##_to_matrix3(m3, a);                                            \
    d_quat_##S##_to_matrix4(m4, b);                                            \
    o[0] = d_quat_norm_c(a);                                                   \
    w[0] = d_quat_##S##_norm_c(a);                                             \
    o[1] = d_quat_norm(b);                                                     \
    w[1] = d_quat_##S##_norm(b);                                               \
    same = same && d_tests_same_##S(o, w, 2u);                                 \
    d_quat_multiply_fast(o, a, b);                                             \
    d_quat_##S##_multiply_fast(w, a, b);                                       \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_multiply(o, a, b);                                                  \
    d_quat_##S##_multiply(w, a, b);                                            \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_conjugate(o, a);                                                    \
    d_quat_##S##_conjugate(w, a);                                              \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_inverse(o, a);                                                      \
    d_quat_##S##_inverse(w, a);                                                \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_divide(o, a, b);                                                    \
    d_quat_##S##_divide(w, a, b);                                              \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_left_divide(o, a, b);                                               \
    d_quat_##S##_left_divide(w, a, b);                                         \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_normalize_c(o, n);                                                  \
    d_quat_##S##_normalize_c(w, n);                                            \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_normalize(o, n);                                                    \
    d_quat_##S##_normalize(w, n);                                              \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_normalize_fast(o, n);                                               \
    d_quat_##S##_normalize_fast(w, n);                                         \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_rotate_fast(o, a, v);                                               \
    d_quat_##S##_rotate_fast(w, a, v);                                         \
    same = same && d_tests_same_##S(o, w, 3u);                                 \
    d_quat_rotate(o, a, v);                                                    \
    d_quat_##S##_rotate(w, a, v);                                              \
    same = same && d_tests_same_##S(o, w, 3u);                                 \
    d_quat_to_matrix3_fast(o, a);                                              \
    d_quat_##S##_to_matrix3_fast(w, a);                                        \
    same = same && d_tests_same_##S(o, w, 9u);                                 \
    d_quat_to_matrix3(o, b);                                                   \
    d_quat_##S##_to_matrix3(w, b);                                             \
    same = same && d_tests_same_##S(o, w, 9u);                                 \
    d_quat_to_matrix4_fast(o, a);                                              \
    d_quat_##S##_to_matrix4_fast(w, a);                                        \
    same = same && d_tests_same_##S(o, w, 16u);                                \
    d_quat_to_matrix4(o, b);                                                   \
    d_quat_##S##_to_matrix4(w, b);                                             \
    same = same && d_tests_same_##S(o, w, 16u);                                \
    d_quat_from_matrix3_c(o, m3);                                              \
    d_quat_##S##_from_matrix3_c(w, m3);                                        \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_from_matrix3(o, m3);                                                \
    d_quat_##S##_from_matrix3(w, m3);                                          \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_from_matrix4_c(o, m4);                                              \
    d_quat_##S##_from_matrix4_c(w, m4);                                        \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_from_matrix4(o, m4);                                                \
    d_quat_##S##_from_matrix4(w, m4);                                          \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_from_two_vectors_c(o, v, a);                                        \
    d_quat_##S##_from_two_vectors_c(w, v, a);                                  \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_from_two_vectors(o, a, v);                                          \
    d_quat_##S##_from_two_vectors(w, a, v);                                    \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_from_axis_angle_c(o, v, (T)0.75);                                   \
    d_quat_##S##_from_axis_angle_c(w, v, (T)0.75);                             \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_from_axis_angle(o, v, (T)-1.5);                                     \
    d_quat_##S##_from_axis_angle(w, v, (T)-1.5);                               \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_from_unit_axis_angle_c(o, v, (T)0.75);                              \
    d_quat_##S##_from_unit_axis_angle_c(w, v, (T)0.75);                        \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_from_unit_axis_angle(o, v, (T)-1.5);                                \
    d_quat_##S##_from_unit_axis_angle(w, v, (T)-1.5);                          \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_nlerp_c(o, ua, ub, (T)0.375);                                       \
    d_quat_##S##_nlerp_c(w, ua, ub, (T)0.375);                                 \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_nlerp(o, ub, ua, (T)0.625);                                         \
    d_quat_##S##_nlerp(w, ub, ua, (T)0.625);                                   \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_slerp_c(o, ua, ub, (T)0.375);                                       \
    d_quat_##S##_slerp_c(w, ua, ub, (T)0.375);                                 \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_slerp(o, ub, ua, (T)0.625);                                         \
    d_quat_##S##_slerp(w, ub, ua, (T)0.625);                                   \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_exp_c(o, a);                                                        \
    d_quat_##S##_exp_c(w, a);                                                  \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_exp(o, b);                                                          \
    d_quat_##S##_exp(w, b);                                                    \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_log_c(o, a);                                                        \
    d_quat_##S##_log_c(w, a);                                                  \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_log(o, b);                                                          \
    d_quat_##S##_log(w, b);                                                    \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_sqrt_c(o, a);                                                       \
    d_quat_##S##_sqrt_c(w, a);                                                 \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_sqrt(o, b);                                                         \
    d_quat_##S##_sqrt(w, b);                                                   \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_pow_c(o, a, (T)2.5);                                                \
    d_quat_##S##_pow_c(w, a, (T)2.5);                                          \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    d_quat_pow(o, b, (T)-1.5);                                                 \
    d_quat_##S##_pow(w, b, (T)-1.5);                                           \
    same = same && d_tests_same_##S(o, w, 4u);                                 \
    D_TESTS_EXPECT(same, #S " the C++ overloads are the family's functions");  \
                                                                               \
    return;                                                                    \
}

D_TESTS_QUAT_OVERLOADS(float,       f)
D_TESTS_QUAT_OVERLOADS(double,      d)
D_TESTS_QUAT_OVERLOADS(long double, ld)

#endif  // __cplusplus

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// D_TESTS_QUAT_CONSTANT
//   stamps, for one family, a function giving every accurate function's and
// _c form's parts at one point, evaluated once as a constant expression and
// once at run time from volatile inputs, and d_tests_constant_<S>, which
// compares the two bit for bit.
#define D_TESTS_QUAT_CONSTANT(T, S, X, Y, Z, W)                                \
struct dj_tests_values_##S                                                     \
{                                                                              \
    T part[72];                                                                \
};                                                                             \
                                                                               \
static constexpr struct dj_tests_values_##S                                    \
d_tests_values_of_##S(T _x, T _y, T _z, T _w)                                  \
{                                                                              \
    struct dj_tests_values_##S r = { { (T)0 } };                               \
    const T a[4] = { _x, _y, _z, _w };                                         \
    const T b[4] = { _w, _z, _x, _y };                                         \
    const T v[3] = { _y, _w, _x };                                             \
    T       o[9] = { (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0 };   \
    T       m[9] = { (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0 };   \
    T       ua[4] = { (T)0, (T)0, (T)0, (T)0 };                                \
    T       ub[4] = { (T)0, (T)0, (T)0, (T)0 };                                \
    size_t  k = 0u;                                                            \
    size_t  i = 0u;                                                            \
                                                                               \
    d_quat_##S##_multiply(o, a, b);                                            \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_inverse(o, a);                                                \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_divide(o, a, b);                                              \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_left_divide(o, a, b);                                         \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_rotate(o, a, v);                                              \
    for (i = 0u; i < 3u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_to_matrix3(m, a);                                             \
    for (i = 0u; i < 9u; ++i) { r.part[k++] = m[i]; }                          \
    r.part[k++] = d_quat_##S##_norm_c(a);                                      \
    d_quat_##S##_normalize_c(ua, a);                                           \
    d_quat_##S##_normalize_c(ub, b);                                           \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = ua[i]; }                         \
    d_quat_##S##_from_matrix3_c(o, m);                                         \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_from_two_vectors_c(o, v, a);                                  \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_nlerp_c(o, ua, ub, (T)0.375);                                 \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_slerp_c(o, ua, ub, (T)0.375);                                 \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_exp_c(o, a);                                                  \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_log_c(o, a);                                                  \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_sqrt_c(o, a);                                                 \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
    d_quat_##S##_pow_c(o, a, (T)2.5);                                          \
    for (i = 0u; i < 4u; ++i) { r.part[k++] = o[i]; }                          \
                                                                               \
    return r;                                                                  \
}                                                                              \
                                                                               \
static constexpr struct dj_tests_values_##S g_cq_##S =                         \
    d_tests_values_of_##S(X, Y, Z, W);                                         \
                                                                               \
static void                                                                    \
d_tests_constant_##S(void)                                                     \
{                                                                              \
    volatile T                       x  = X;                                   \
    volatile T                       y  = Y;                                   \
    volatile T                       z  = Z;                                   \
    volatile T                       w  = W;                                   \
    const struct dj_tests_values_##S r  = d_tests_values_of_##S(x, y, z, w);   \
    bool                             ok = true;                                \
    size_t                           k  = 0u;                                  \
                                                                               \
    for (k = 0u; k < 72u; ++k)                                                 \
    {                                                                          \
        volatile T run = r.part[k];                                            \
                                                                               \
        if (!d_tests_same_bits((long double)run,                               \
                               (long double)g_cq_##S.part[k]))                 \
        {                                                                      \
            ok = false;                                                        \
            printf("  constant %s %lu: %Lg at run time %Lg\n", #S,            \
                   (unsigned long)k, (long double)g_cq_##S.part[k],            \
                   (long double)run);                                          \
        }                                                                      \
    }                                                                          \
                                                                               \
    D_TESTS_EXPECT(ok, #S " the constant expressions are the run time's");     \
                                                                               \
    return;                                                                    \
}

D_TESTS_QUAT_CONSTANT(float,       f,  0.375f, -1.25f, 0.5f, 0.875f)
D_TESTS_QUAT_CONSTANT(double,      d,  0.375,  -1.25,  0.5,  0.875)
D_TESTS_QUAT_CONSTANT(long double, ld, 0.375L, -1.25L, 0.5L, 0.875L)

#endif  // C++14

int
main(void)
{
    d_tests_references();
    d_tests_identities_f();
    d_tests_identities_d();
    d_tests_identities_ld();
    d_tests_special_f();
    d_tests_special_d();
    d_tests_special_ld();
#ifdef __cplusplus
    d_tests_overloads_f();
    d_tests_overloads_d();
    d_tests_overloads_ld();
#endif
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_f();
    d_tests_constant_d();
    d_tests_constant_ld();
#endif

    printf("t_quat: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
