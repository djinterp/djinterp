/*******************************************************************************
* djinterp [test]                                                      t_value.c
*
*   Conformance harness for the value layer (c/math/value.h): every operation
* on every pairing of kinds, against the exact reference of
* math_tests_exact.h.
*   An exact result must be exact: an integer where its reduced denominator
* is 1, a rational otherwise, equal to the exact value by cross-multiplication
* and in the one form; or, where it does not fit dj_math_imax, the long double
* nearest it, ties to even, flagged inexact. An exact division by exact zero
* must be a pole, or invalid for 0 / 0. An inexact result must be the
* operation done in the wider of the operands' floating-point types, an exact
* operand converted to it as rational.h converts it (t_rational.c checks those
* conversions against the reference); the harness does the operation itself,
* each step stored through a volatile object. An error must pass through, the
* first of two; a boolean or none in arithmetic must be a domain error; and a
* result must carry every flag of its operands. Every value made must hold 0
* in the fields its kind does not use.
*   A comparison must agree with the reference: exact numbers by their cross
* products; an exact number and a floating-point one by the latter's exact
* mantissa and exponent, from the C library's frexp; two floating-point ones
* as long doubles. eq must be cmp's EQUAL, and cmp(b, a) cmp(a, b) turned.
*   The operands: 33 exact numbers -- small ones, the family's ends, and the
* values around 2^(w/2) and 2^(w-1) where sums and products overflow -- 54
* floating-point ones, 18 of each type, zeros, infinities and a NaN among
* them; the nearest float, double and long double of -1/3 and of MAX/3, which
* a comparison must tell from the rationals themselves; none, both booleans
* and the four errors; and two flagged values, an exact sum past the family
* and an error made from it: each pair, for every operation and the
* comparison. Then 20,000 pairs of exact numbers drawn at
* random, each term of a random length. from_uint is checked against the
* reference too, an integer up to dj_math_imax's top and the long double
* nearest past it, flagged inexact: every power of two in the word and its
* neighbours, and 2,000 drawn at random. From C++14 the operations are also
* checked as constant expressions.
*   The same file compiles as C++ (-x c++), at any level from C++98: value.h
* is one text in both languages.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_value.c -o t_value -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_value.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.18
*******************************************************************************/
// std
#include <float.h>   // FLT_MAX, FLT_MIN, DBL_MAX ... LDBL_MIN
#include <stdio.h>   // printf, sprintf
#include <string.h>  // strlen
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/value.h"  // unit under test
// tests
#include "./math_tests_exact.h"  // the exact reference


// the family's ends, and 2^(w/2), where the product of two terms passes a word
#define D_TESTS_MOST   D_MATH_IMAX_MAX
#define D_TESTS_LEAST  D_MATH_IMAX_MIN
#define D_TESTS_HALF                                                           \
    ((dj_math_imax)((dj_math_umax)1 << D_INTERNAL_RATIONAL_HALF_BITS))

// the pairs of exact numbers drawn at random
#define D_TESTS_TRIALS  20000L

// the operations, as value.h numbers them
#define D_TESTS_ADD  D_MATH_VALUE_OP_ADD
#define D_TESTS_SUB  D_MATH_VALUE_OP_SUB
#define D_TESTS_MUL  D_MATH_VALUE_OP_MUL
#define D_TESTS_DIV  D_MATH_VALUE_OP_DIV

static long g_checks   = 0;
static long g_failures = 0;


//==============================================================================
// 1.  REPORTING
//==============================================================================

// d_tests_value_text
//   _x as text, into _buffer (at least 80 characters), returned.
static
const char* d_tests_value_text(
    struct dj_math_value _x,
    char*                _buffer
)
{
    char num[24];
    char den[24];

    // by kind
    if (_x.kind == D_MATH_VALUE_NONE)
    {
        sprintf(_buffer, "none");
    }
    else if (_x.kind == D_MATH_VALUE_ERROR)
    {
        sprintf(_buffer, "error %d", _x.code);
    }
    else if (_x.kind == D_MATH_VALUE_BOOLEAN)
    {
        sprintf(_buffer, "boolean %d", _x.code);
    }
    else if ( (_x.kind == D_MATH_VALUE_INTEGER) ||
              (_x.kind == D_MATH_VALUE_RATIONAL) )
    {
        sprintf(_buffer, "%s %s/%s",
                (_x.kind == D_MATH_VALUE_INTEGER) ? "integer" : "rational",
                d_tests_text(_x.exact.num, num),
                d_tests_text(_x.exact.den, den));
    }
    else
    {
        sprintf(_buffer, "%s %.21Lg",
                (_x.kind == D_MATH_VALUE_FLOAT)
                    ? "float"
                    : ( (_x.kind == D_MATH_VALUE_DOUBLE) ? "double"
                                                         : "long double" ),
                _x.real);
    }

    // the flags, where there are any
    if (_x.flags != 0u)
    {
        sprintf(_buffer + strlen(_buffer), " (flags %u)", _x.flags);
    }

    return _buffer;
}

// d_tests_expect
//   counts a check, and reports the first few failures with their operands.
static
void d_tests_expect(
    bool                 _ok,
    const char*          _what,
    struct dj_math_value _a,
    struct dj_math_value _b,
    int                  _line
)
{
    char text[2][80];

    ++g_checks;

    // the first failures, with their operands
    if (!_ok)
    {
        ++g_failures;

        if (g_failures <= 40)
        {
            printf("  FAIL line %d: %s (%s; %s)\n", _line, _what,
                   d_tests_value_text(_a, text[0]),
                   d_tests_value_text(_b, text[1]));
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what, a, b)                                         \
    d_tests_expect((ok), (what), (a), (b), __LINE__)


//==============================================================================
// 2.  WHAT A RESULT MUST BE
//==============================================================================

// d_tests_is_plus_zero
//   whether _x is +0: 0, its reciprocal +infinity.
static
bool d_tests_is_plus_zero(
    long double _x
)
{
    volatile long double zero = _x;

    return ( (_x == 0.0L) &&
             ((1.0L / zero) > 0.0L) );
}

// d_tests_same_real
//   whether _a and _b are the same floating-point value: both NaN, or equal
// with the same sign, -0 and +0 told apart.
static
bool d_tests_same_real(
    long double _a,
    long double _b
)
{
    // NaN
    if ( (_a != _a) ||
         (_b != _b) )
    {
        return ( (_a != _a) &&
                 (_b != _b) );
    }

    // a zero: the same sign too
    if ( (_a == 0.0L) &&
         (_b == 0.0L) )
    {
        return (d_tests_is_plus_zero(_a) == d_tests_is_plus_zero(_b));
    }

    return (_a == _b);
}

// d_tests_canonical
//   whether the fields _x's kind does not use hold 0 (exact 0/1), and the
// ones it uses hold what the kind allows.
static
bool d_tests_canonical(
    struct dj_math_value _x
)
{
    const bool no_code  = (_x.code == 0);
    const bool no_exact = ( (_x.exact.num == 0) &&
                            (_x.exact.den == 1) );
    const bool no_real  = d_tests_is_plus_zero(_x.real);

    // no table and no box: no inline kind has either
    if ( (_x.type != NULL) ||
         (_x.box != NULL) )
    {
        return false;
    }

    // by kind
    switch (_x.kind)
    {
        case D_MATH_VALUE_NONE:
            return (no_code && no_exact && no_real);

        case D_MATH_VALUE_ERROR:
            return ( (_x.code >= (int)D_MATH_VALUE_ERROR_DOMAIN) &&
                     (_x.code <= (int)D_MATH_VALUE_ERROR_INVALID) &&
                     (no_exact) &&
                     (no_real) );

        case D_MATH_VALUE_BOOLEAN:
            return ( ( (_x.code == 0) ||
                       (_x.code == 1) ) &&
                     (no_exact) &&
                     (no_real) );

        case D_MATH_VALUE_INTEGER:
            return ( (no_code) &&
                     (_x.exact.den == 1) &&
                     (no_real) );

        case D_MATH_VALUE_RATIONAL:
            return ( (no_code) &&
                     (d_tests_in_one_form(_x.exact)) &&
                     (_x.exact.den != 1) &&
                     (no_real) );

        case D_MATH_VALUE_FLOAT:
            return ( (no_code) &&
                     (no_exact) &&
                     ( (_x.real != _x.real) ||
                       ((long double)(float)_x.real == _x.real) ) );

        case D_MATH_VALUE_DOUBLE:
            return ( (no_code) &&
                     (no_exact) &&
                     ( (_x.real != _x.real) ||
                       ((long double)(double)_x.real == _x.real) ) );

        case D_MATH_VALUE_LONG_DOUBLE:
            return ( (no_code) &&
                     (no_exact) );

        default:
            return false;
    }
}

// d_tests_exact_of_value
//   an exact value's number, for the reference.
static
struct dj_tests_exact d_tests_exact_of_value(
    struct dj_math_value _x
)
{
    return d_tests_exact_of(_x.exact.num, _x.exact.den);
}

// d_tests_is_exactly
//   whether _r, a value an exact operation gave with operands flagged
// _flags, is the exact number _want: an integer where _want's reduced
// denominator is 1, a rational where not, equal to _want and in the one
// form; or, where _want does not fit dj_math_imax, the long double nearest
// it, ties to even, flagged inexact.
static
bool d_tests_is_exactly(
    struct dj_math_value  _r,
    struct dj_tests_exact _want,
    unsigned              _flags
)
{
    // too wide for the exact kinds
    if (!d_tests_exact_fits(_want))
    {
        return ( (_r.kind == D_MATH_VALUE_LONG_DOUBLE) &&
                 (_r.flags == (_flags | D_MATH_VALUE_FLAG_INEXACT)) &&
                 (d_tests_canonical(_r)) &&
                 (d_tests_nearest_ld((_want.n.negative != 0),
                                     d_tests_big_abs(_want.n), _want.d,
                                     _r.real)) );
    }

    // an integer exactly where the denominator divides the numerator
    const bool whole =
        ( (d_tests_big_is_zero(_want.n)) ||
          (d_tests_big_cmp_mag(d_tests_big_gcd(_want.n, _want.d), _want.d)
               == 0) );

    return ( (_r.kind == (whole ? D_MATH_VALUE_INTEGER
                                : D_MATH_VALUE_RATIONAL)) &&
             (_r.flags == _flags) &&
             (d_tests_canonical(_r)) &&
             (d_tests_in_one_form(_r.exact)) &&
             (d_tests_exact_cmp(d_tests_exact_of_value(_r), _want) == 0) );
}

// d_tests_exact_want
//   _a _op _b by the reference, _b nonzero for a quotient.
static
struct dj_tests_exact d_tests_exact_want(
    int                  _op,
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    const struct dj_tests_exact a = d_tests_exact_of_value(_a);
    const struct dj_tests_exact b = d_tests_exact_of_value(_b);

    // the operation
    switch (_op)
    {
        case D_TESTS_ADD:
            return d_tests_exact_add(a, b);

        case D_TESTS_SUB:
            return d_tests_exact_add(a, d_tests_exact_neg(b));

        case D_TESTS_MUL:
            return d_tests_exact_mul(a, b);

        default:
            return d_tests_exact_mul(a, d_tests_exact_inv(b));
    }
}

// d_tests_real_f, _d, _ld
//   a number in a type at least its kind's: exact, as rational.h converts
// it; inexact, exactly.
static
float d_tests_real_f(
    struct dj_math_value _x
)
{
    return d_math_value_is_exact(_x) ? d_rational_to_f(_x.exact)
                                     : (float)_x.real;
}

static
double d_tests_real_d(
    struct dj_math_value _x
)
{
    return d_math_value_is_exact(_x) ? d_rational_to_d(_x.exact)
                                     : (double)_x.real;
}

static
long double d_tests_real_ld(
    struct dj_math_value _x
)
{
    return d_math_value_is_exact(_x) ? d_rational_to_ld(_x.exact)
                                     : _x.real;
}

// d_tests_real_want
//   _a _op _b in the type of kind _kind, the harness's own: each operand and
// the result stored through a volatile object of the type.
static long double
d_tests_real_want(
    int                    _op,
    enum dj_math_value_kind _kind,
    struct dj_math_value    _a,
    struct dj_math_value    _b
)
{
    // float
    if (_kind == D_MATH_VALUE_FLOAT)
    {
        volatile float a = d_tests_real_f(_a);
        volatile float b = d_tests_real_f(_b);
        volatile float r = 0.0f;

        if (_op == D_TESTS_ADD)      { r = a + b; }
        else if (_op == D_TESTS_SUB) { r = a - b; }
        else if (_op == D_TESTS_MUL) { r = a * b; }
        else                         { r = a / b; }

        return (long double)r;
    }

    // double
    if (_kind == D_MATH_VALUE_DOUBLE)
    {
        volatile double a = d_tests_real_d(_a);
        volatile double b = d_tests_real_d(_b);
        volatile double r = 0.0;

        if (_op == D_TESTS_ADD)      { r = a + b; }
        else if (_op == D_TESTS_SUB) { r = a - b; }
        else if (_op == D_TESTS_MUL) { r = a * b; }
        else                         { r = a / b; }

        return (long double)r;
    }

    // long double
    {
        volatile long double a = d_tests_real_ld(_a);
        volatile long double b = d_tests_real_ld(_b);
        volatile long double r = 0.0L;

        if (_op == D_TESTS_ADD)      { r = a + b; }
        else if (_op == D_TESTS_SUB) { r = a - b; }
        else if (_op == D_TESTS_MUL) { r = a * b; }
        else                         { r = a / b; }

        return r;
    }
}

// d_tests_cmp_exact_real
//   the reference's order of the exact number _r against the finite or
// infinite _f (math_tests_exact.h), as value.h gives an order.
static enum dj_math_value_order
d_tests_cmp_exact_real(
    struct dj_rational _r,
    long double       _f
)
{
    const int order = d_tests_exact_cmp_real(_r, _f);

    return (order == 2) ? D_MATH_VALUE_UNORDERED
                        : (enum dj_math_value_order)order;
}

// d_tests_cmp_want
//   the order the reference gives _a and _b.
static enum dj_math_value_order
d_tests_cmp_want(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    // two numbers
    if ( (d_math_value_is_number(_a)) &&
         (d_math_value_is_number(_b)) )
    {
        if ( (d_math_value_is_exact(_a)) &&
             (d_math_value_is_exact(_b)) )
        {
            const int order = d_tests_exact_cmp(d_tests_exact_of_value(_a),
                                                d_tests_exact_of_value(_b));

            return (order < 0) ? D_MATH_VALUE_LESS
                               : ( (order > 0) ? D_MATH_VALUE_GREATER
                                               : D_MATH_VALUE_EQUAL );
        }

        if (d_math_value_is_exact(_a))
        {
            return d_tests_cmp_exact_real(_a.exact, _b.real);
        }

        if (d_math_value_is_exact(_b))
        {
            const enum dj_math_value_order order =
                d_tests_cmp_exact_real(_b.exact, _a.real);

            return (order == D_MATH_VALUE_LESS)
                       ? D_MATH_VALUE_GREATER
                       : ( (order == D_MATH_VALUE_GREATER) ? D_MATH_VALUE_LESS
                                                           : order );
        }

        // two floating-point values, as long doubles
        if ( (_a.real != _a.real) ||
             (_b.real != _b.real) )
        {
            return D_MATH_VALUE_UNORDERED;
        }

        return (_a.real < _b.real)
                   ? D_MATH_VALUE_LESS
                   : ( (_a.real > _b.real) ? D_MATH_VALUE_GREATER
                                           : D_MATH_VALUE_EQUAL );
    }

    // two booleans, two nones
    if ( (_a.kind == D_MATH_VALUE_BOOLEAN) &&
         (_b.kind == D_MATH_VALUE_BOOLEAN) )
    {
        return (_a.code < _b.code)
                   ? D_MATH_VALUE_LESS
                   : ( (_a.code > _b.code) ? D_MATH_VALUE_GREATER
                                           : D_MATH_VALUE_EQUAL );
    }

    if ( (_a.kind == D_MATH_VALUE_NONE) &&
         (_b.kind == D_MATH_VALUE_NONE) )
    {
        return D_MATH_VALUE_EQUAL;
    }

    return D_MATH_VALUE_UNORDERED;
}


//==============================================================================
// 3.  ONE OPERATION, CHECKED
//==============================================================================

// d_tests_apply
//   value.h's _a _op _b.
static
struct dj_math_value d_tests_apply(
    int                  _op,
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    // the operation
    switch (_op)
    {
        case D_TESTS_ADD:
            return d_math_value_add(_a, _b);

        case D_TESTS_SUB:
            return d_math_value_sub(_a, _b);

        case D_TESTS_MUL:
            return d_math_value_mul(_a, _b);

        default:
            return d_math_value_div(_a, _b);
    }
}

// d_tests_check_binary
//   _a _op _b against what it must be, one check.
static
void d_tests_check_binary(
    int                  _op,
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    static const char* const names[4] = { "add", "sub", "mul", "div" };

    const struct dj_math_value r     = d_tests_apply(_op, _a, _b);
    const unsigned            flags = _a.flags | _b.flags;

    bool ok = false;

    // an error passes through, the first of two
    if ( (_a.kind == D_MATH_VALUE_ERROR) ||
         (_b.kind == D_MATH_VALUE_ERROR) )
    {
        const struct dj_math_value first =
            (_a.kind == D_MATH_VALUE_ERROR) ? _a : _b;

        ok = ( (r.kind == D_MATH_VALUE_ERROR) &&
               (r.code == first.code) &&
               (r.flags == flags) &&
               (d_tests_canonical(r)) );
    }
    // a boolean or none: outside the domain
    else if ( (!d_math_value_is_number(_a)) ||
              (!d_math_value_is_number(_b)) )
    {
        ok = ( (r.kind == D_MATH_VALUE_ERROR) &&
               (r.code == (int)D_MATH_VALUE_ERROR_DOMAIN) &&
               (r.flags == flags) &&
               (d_tests_canonical(r)) );
    }
    // exact over exact 0
    else if ( (d_math_value_is_exact(_a)) &&
              (d_math_value_is_exact(_b)) &&
              (_op == D_TESTS_DIV) &&
              (_b.exact.num == 0) )
    {
        ok = ( (r.kind == D_MATH_VALUE_ERROR) &&
               (r.code == (int)((_a.exact.num == 0)
                                    ? D_MATH_VALUE_ERROR_INVALID
                                    : D_MATH_VALUE_ERROR_POLE)) &&
               (r.flags == flags) &&
               (d_tests_canonical(r)) );
    }
    // exact and exact
    else if ( (d_math_value_is_exact(_a)) &&
              (d_math_value_is_exact(_b)) )
    {
        ok = d_tests_is_exactly(r, d_tests_exact_want(_op, _a, _b), flags);
    }
    // inexact: the wider type's own arithmetic
    else
    {
        const enum dj_math_value_kind kind = (_a.kind > _b.kind) ? _a.kind
                                                                : _b.kind;

        ok = ( (r.kind == kind) &&
               (r.flags == flags) &&
               (d_tests_canonical(r)) &&
               (d_tests_same_real(r.real,
                                  d_tests_real_want(_op, kind, _a, _b))) );
    }

    D_TESTS_EXPECT(ok, names[_op], _a, _b);

    return;
}

// d_tests_check_unary
//   neg and abs of _x against what they must be, one check.
static
void d_tests_check_unary(
    struct dj_math_value _x
)
{
    const struct dj_math_value negated  = d_math_value_neg(_x);
    const struct dj_math_value absolute = d_math_value_abs(_x);

    bool ok = false;

    // an error passes through
    if (_x.kind == D_MATH_VALUE_ERROR)
    {
        ok = ( (negated.kind == D_MATH_VALUE_ERROR) &&
               (negated.code == _x.code) &&
               (negated.flags == _x.flags) &&
               (absolute.kind == D_MATH_VALUE_ERROR) &&
               (absolute.code == _x.code) &&
               (absolute.flags == _x.flags) );
    }
    // a boolean or none
    else if (!d_math_value_is_number(_x))
    {
        ok = ( (negated.kind == D_MATH_VALUE_ERROR) &&
               (negated.code == (int)D_MATH_VALUE_ERROR_DOMAIN) &&
               (negated.flags == _x.flags) &&
               (absolute.kind == D_MATH_VALUE_ERROR) &&
               (absolute.code == (int)D_MATH_VALUE_ERROR_DOMAIN) &&
               (absolute.flags == _x.flags) );
    }
    // exact: -x and |x| by the reference
    else if (d_math_value_is_exact(_x))
    {
        const struct dj_tests_exact x = d_tests_exact_of_value(_x);

        ok = ( (d_tests_is_exactly(negated, d_tests_exact_neg(x),
                                   _x.flags)) &&
               (d_tests_is_exactly(absolute, d_tests_exact_abs(x),
                                   _x.flags)) );
    }
    // inexact: the sign turned; cleared, a zero's included
    else
    {
        volatile long double turned = -_x.real;
        volatile long double plain  =
            (_x.real < 0.0L) ? -_x.real
                             : ( (_x.real == 0.0L) ? 0.0L : _x.real );

        ok = ( (negated.kind == _x.kind) &&
               (negated.flags == _x.flags) &&
               (d_tests_canonical(negated)) &&
               (d_tests_same_real(negated.real, turned)) &&
               (absolute.kind == _x.kind) &&
               (absolute.flags == _x.flags) &&
               (d_tests_canonical(absolute)) &&
               (d_tests_same_real(absolute.real, plain)) );
    }

    D_TESTS_EXPECT( (ok) &&
                    (d_tests_canonical(negated)) &&
                    (d_tests_canonical(absolute)),
                    "neg and abs", _x, _x);

    return;
}

// d_tests_check_compare
//   cmp both ways and eq on (_a, _b) against the reference, one check.
static
void d_tests_check_compare(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    const enum dj_math_value_order want = d_tests_cmp_want(_a, _b);
    const enum dj_math_value_order back =
        (want == D_MATH_VALUE_LESS)
            ? D_MATH_VALUE_GREATER
            : ( (want == D_MATH_VALUE_GREATER) ? D_MATH_VALUE_LESS : want );

    D_TESTS_EXPECT( (d_math_value_cmp(_a, _b) == want) &&
                    (d_math_value_cmp(_b, _a) == back) &&
                    (d_math_value_eq(_a, _b) == (want == D_MATH_VALUE_EQUAL)),
                    "cmp and eq", _a, _b);

    return;
}

// d_tests_check_convert
//   to_f, to_d, to_ld and to_rational of _x, one check: a number converts,
// an exact one as rational.h converts it; anything else is refused, and
// the destination left as it was.
static
void d_tests_check_convert(
    struct dj_math_value _x
)
{
    const bool number = d_math_value_is_number(_x);
    const bool exact  = d_math_value_is_exact(_x);

    float             f   = 12345.0f;
    double            d   = 12345.0;
    long double       ld  = 12345.0L;
    struct dj_rational r   = { 12345, 0 };
    const bool        got = ( (d_math_value_to_f(&f, _x) == number) &&
                              (d_math_value_to_d(&d, _x) == number) &&
                              (d_math_value_to_ld(&ld, _x) == number) &&
                              (d_math_value_to_rational(&r, _x) == exact) );

    bool ok = got;

    // the values given, or the destinations untouched
    if (exact)
    {
        ok = ( (ok) &&
               (d_tests_same_real((long double)f,
                                  (long double)d_rational_to_f(_x.exact))) &&
               (d_tests_same_real((long double)d,
                                  (long double)d_rational_to_d(_x.exact))) &&
               (d_tests_same_real(ld, d_rational_to_ld(_x.exact))) &&
               (r.num == _x.exact.num) &&
               (r.den == _x.exact.den) );
    }
    else if (number)
    {
        volatile float  as_f = (float)_x.real;
        volatile double as_d = (double)_x.real;

        ok = ( (ok) &&
               (d_tests_same_real((long double)f, (long double)as_f)) &&
               (d_tests_same_real((long double)d, (long double)as_d)) &&
               (d_tests_same_real(ld, _x.real)) &&
               (r.num == 12345) &&
               (r.den == 0) );
    }
    else
    {
        ok = ( (ok) &&
               (f == 12345.0f) &&
               (d == 12345.0) &&
               (ld == 12345.0L) &&
               (r.num == 12345) &&
               (r.den == 0) );
    }

    D_TESTS_EXPECT(ok, "to_f, to_d, to_ld, to_rational", _x, _x);

    return;
}


//==============================================================================
// 4.  THE OPERANDS
//==============================================================================

// one of each kind's interesting values; built at run time, so that
// infinities and NaN come from IEEE arithmetic
#define D_TESTS_MAX_VALUES  128

static struct dj_math_value g_values[D_TESTS_MAX_VALUES];
static int                 g_count = 0;

static
void d_tests_add_value(
    struct dj_math_value _x
)
{
    // room for it
    if (g_count < D_TESTS_MAX_VALUES)
    {
        g_values[g_count] = _x;
        ++g_count;
    }

    return;
}

// d_tests_make_values
//   the operands: exact numbers, floating-point ones of each type, and the
// other kinds.
static
void d_tests_make_values(void)
{
    // each in the one form; integers as n/1
    static const dj_math_imax exact[][2] = {
        { 0, 1 },          { 1, 1 },               { -1, 1 },
        { 2, 1 },          { -3, 1 },              { 7, 1 },
        { 1, 2 },          { -1, 3 },              { 2, 3 },
        { -5, 6 },
        { D_TESTS_MOST,      1                },
        { D_TESTS_LEAST,     1                },
        { D_TESTS_MOST - 1,  1                },
        { D_TESTS_LEAST + 1, 1                },
        { D_TESTS_MOST,      2                },
        { D_TESTS_MOST,      3                },
        { -D_TESTS_MOST,     6                },
        { 1,                 D_TESTS_MOST     },
        { -1,                D_TESTS_MOST     },
        { D_TESTS_MOST - 1,  D_TESTS_MOST     },
        { D_TESTS_LEAST,     D_TESTS_MOST     },
        { D_TESTS_HALF,      1                },
        { -D_TESTS_HALF,     1                },
        { D_TESTS_HALF + 1,  1                },
        { D_TESTS_HALF - 1,  1                },
        { 1,                 D_TESTS_HALF     },
        { D_TESTS_HALF + 1,  D_TESTS_HALF - 1 },
        { 3,                 D_TESTS_HALF     },
        { D_TESTS_LEAST / 2, 1                },
        { D_TESTS_MOST / 2,  1                },
        { 1,                 D_TESTS_MOST / 2 },
        { 5,                 D_TESTS_MOST / 4 },
        { D_TESTS_MOST,      D_TESTS_HALF + 1 }
    };
    const int exact_count = (int)(sizeof(exact) / sizeof(exact[0]));

    volatile float       zero_f  = 0.0f;
    volatile double      zero_d  = 0.0;
    volatile long double zero_ld = 0.0L;
    int                  i       = 0;

    // the exact numbers, as values
    for (i = 0; i < exact_count; ++i)
    {
        d_tests_add_value(d_math_value_from_ratio(exact[i][0], exact[i][1]));
    }

    // float
    d_tests_add_value(d_math_value_from_f(0.0f));
    d_tests_add_value(d_math_value_from_f(-zero_f));
    d_tests_add_value(d_math_value_from_f(1.0f));
    d_tests_add_value(d_math_value_from_f(-1.0f));
    d_tests_add_value(d_math_value_from_f(0.5f));
    d_tests_add_value(d_math_value_from_f(0.1f));
    d_tests_add_value(d_math_value_from_f(1.0f / 3.0f));
    d_tests_add_value(d_math_value_from_f(-2.5f));
    d_tests_add_value(d_math_value_from_f(16777216.0f));
    d_tests_add_value(d_math_value_from_f(9.2233720e18f));
    d_tests_add_value(d_math_value_from_f(1.0e30f));
    d_tests_add_value(d_math_value_from_f(-1.0e-30f));
    d_tests_add_value(d_math_value_from_f(FLT_MAX));
    d_tests_add_value(d_math_value_from_f(FLT_MIN));
    d_tests_add_value(d_math_value_from_f(FLT_MIN / 4.0f));
    d_tests_add_value(d_math_value_from_f(1.0f / zero_f));
    d_tests_add_value(d_math_value_from_f(-1.0f / zero_f));
    d_tests_add_value(d_math_value_from_f(zero_f / zero_f));

    // double
    d_tests_add_value(d_math_value_from_d(0.0));
    d_tests_add_value(d_math_value_from_d(-zero_d));
    d_tests_add_value(d_math_value_from_d(1.0));
    d_tests_add_value(d_math_value_from_d(-1.0));
    d_tests_add_value(d_math_value_from_d(0.5));
    d_tests_add_value(d_math_value_from_d(0.1));
    d_tests_add_value(d_math_value_from_d(1.0 / 3.0));
    d_tests_add_value(d_math_value_from_d(-2.5));
    d_tests_add_value(d_math_value_from_d(9007199254740992.0));
    d_tests_add_value(d_math_value_from_d(9223372036854775808.0));
    d_tests_add_value(d_math_value_from_d(1.0e300));
    d_tests_add_value(d_math_value_from_d(-1.0e-300));
    d_tests_add_value(d_math_value_from_d(DBL_MAX));
    d_tests_add_value(d_math_value_from_d(DBL_MIN));
    d_tests_add_value(d_math_value_from_d(DBL_MIN / 4.0));
    d_tests_add_value(d_math_value_from_d(1.0 / zero_d));
    d_tests_add_value(d_math_value_from_d(-1.0 / zero_d));
    d_tests_add_value(d_math_value_from_d(zero_d / zero_d));

    // long double
    d_tests_add_value(d_math_value_from_ld(0.0L));
    d_tests_add_value(d_math_value_from_ld(-zero_ld));
    d_tests_add_value(d_math_value_from_ld(1.0L));
    d_tests_add_value(d_math_value_from_ld(-1.0L));
    d_tests_add_value(d_math_value_from_ld(0.5L));
    d_tests_add_value(d_math_value_from_ld(0.1L));
    d_tests_add_value(d_math_value_from_ld(1.0L / 3.0L));
    d_tests_add_value(d_math_value_from_ld(-2.5L));
    d_tests_add_value(d_math_value_from_ld(9007199254740993.0L));
    d_tests_add_value(d_math_value_from_ld(9223372036854775807.0L));
    d_tests_add_value(d_math_value_from_ld(1.0e300L));
    d_tests_add_value(d_math_value_from_ld(-1.0e-300L));
    d_tests_add_value(d_math_value_from_ld(LDBL_MAX));
    d_tests_add_value(d_math_value_from_ld(LDBL_MIN));
    d_tests_add_value(d_math_value_from_ld(LDBL_MIN / 4.0L));
    d_tests_add_value(d_math_value_from_ld(1.0L / zero_ld));
    d_tests_add_value(d_math_value_from_ld(-1.0L / zero_ld));
    d_tests_add_value(d_math_value_from_ld(zero_ld / zero_ld));

    // the other kinds
    d_tests_add_value(d_math_value_none());
    d_tests_add_value(d_math_value_from_bool(false));
    d_tests_add_value(d_math_value_from_bool(true));
    d_tests_add_value(d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN));
    d_tests_add_value(d_math_value_from_error(D_MATH_VALUE_ERROR_POLE));
    d_tests_add_value(d_math_value_from_error(D_MATH_VALUE_ERROR_RANGE));
    d_tests_add_value(d_math_value_from_error(D_MATH_VALUE_ERROR_INVALID));

    // the nearest float, double and long double of two rationals that none
    // of them equals: where the comparison must not settle for the rounding
    {
        const struct dj_math_value odd[2] = {
            d_math_value_from_ratio(-1, 3),
            d_math_value_from_ratio(D_TESTS_MOST, 3)
        };

        float       near_f  = 0.0f;
        double      near_d  = 0.0;
        long double near_ld = 0.0L;

        for (i = 0; i < 2; ++i)
        {
            (void)d_math_value_to_f(&near_f, odd[i]);
            (void)d_math_value_to_d(&near_d, odd[i]);
            (void)d_math_value_to_ld(&near_ld, odd[i]);
            d_tests_add_value(d_math_value_from_f(near_f));
            d_tests_add_value(d_math_value_from_d(near_d));
            d_tests_add_value(d_math_value_from_ld(near_ld));
        }
    }

    // flagged: an exact sum past the family, and an error made from it
    {
        const struct dj_math_value past =
            d_math_value_add(d_math_value_from_int(D_TESTS_MOST),
                             d_math_value_from_int(1));

        d_tests_add_value(past);
        d_tests_add_value(d_math_value_add(past, d_math_value_none()));
    }

    return;
}

// d_tests_construction
//   what each construction gives: from_ratio over every numerator and
// denominator in [-12, 12] and the ends, against the reference; from_int,
// the floating-point kinds, the booleans, none and the errors.
static
void d_tests_construction(void)
{
    static const dj_math_imax ends[] = { D_TESTS_MOST, D_TESTS_LEAST,
                                        D_TESTS_MOST - 1, D_TESTS_LEAST + 1,
                                        D_TESTS_HALF, -1, 1, 2, -2 };
    const int ends_count = (int)(sizeof(ends) / sizeof(ends[0]));

    dj_math_imax num = 0;
    dj_math_imax den = 0;
    int         i   = 0;
    int         j   = 0;

    // small numerators over small denominators, 0 and negatives among them
    for (num = -12; num <= 12; ++num)
    {
        for (den = -12; den <= 12; ++den)
        {
            const struct dj_math_value x    = d_math_value_from_ratio(num, den);
            struct dj_math_value       args = d_math_value_from_int(num);

            // the arguments, for a failure's report
            args.exact.den = den;

            // over 0: a pole, or invalid for 0 / 0
            if (den == 0)
            {
                D_TESTS_EXPECT( (x.kind == D_MATH_VALUE_ERROR) &&
                                (x.code == (int)((num == 0)
                                    ? D_MATH_VALUE_ERROR_INVALID
                                    : D_MATH_VALUE_ERROR_POLE)) &&
                                (x.flags == 0u) &&
                                (d_tests_canonical(x)),
                                "from_ratio over 0", args, args);
            }
            else
            {
                D_TESTS_EXPECT(d_tests_is_exactly(x,
                                                  d_tests_exact_of(num, den),
                                                  0u),
                               "from_ratio", args, args);
            }
        }
    }

    // the ends over the ends, either sign
    for (i = 0; i < ends_count; ++i)
    {
        for (j = 0; j < ends_count; ++j)
        {
            const struct dj_math_value x =
                d_math_value_from_ratio(ends[i], ends[j]);
            struct dj_math_value       args = d_math_value_from_int(ends[i]);

            // the arguments, for a failure's report
            args.exact.den = ends[j];

            D_TESTS_EXPECT(d_tests_is_exactly(x,
                                              d_tests_exact_of(ends[i],
                                                               ends[j]),
                                              0u),
                           "from_ratio, the ends", args, args);
        }
    }


    // from_int, the floating-point kinds, the booleans, none, the errors;
    // the double 0.1 stored first, as a constant may be held wider
    {
        volatile double           tenth = 0.1;
        const struct dj_math_value most  = d_math_value_from_int(D_TESTS_MOST);
        const struct dj_math_value f     = d_math_value_from_f(0.25f);
        const struct dj_math_value d     = d_math_value_from_d(tenth);
        const struct dj_math_value ld    = d_math_value_from_ld(0.1L);
        const struct dj_math_value yes   = d_math_value_from_bool(true);
        const struct dj_math_value no    = d_math_value_from_bool(false);
        const struct dj_math_value none  = d_math_value_none();
        const struct dj_math_value range =
            d_math_value_from_error(D_MATH_VALUE_ERROR_RANGE);
        const struct dj_math_value third =
            d_math_value_from_rational(d_rational_from_int(3));

        D_TESTS_EXPECT( (most.kind == D_MATH_VALUE_INTEGER) &&
                        (most.exact.num == D_TESTS_MOST) &&
                        (d_tests_canonical(most)) &&
                        (f.kind == D_MATH_VALUE_FLOAT) &&
                        (f.real == 0.25L) &&
                        (d_tests_canonical(f)) &&
                        (d.kind == D_MATH_VALUE_DOUBLE) &&
                        (d.real == (long double)tenth) &&
                        (d_tests_canonical(d)) &&
                        (ld.kind == D_MATH_VALUE_LONG_DOUBLE) &&
                        (ld.real == 0.1L) &&
                        (d_tests_canonical(ld)) &&
                        (yes.kind == D_MATH_VALUE_BOOLEAN) &&
                        (yes.code == 1) &&
                        (d_tests_canonical(yes)) &&
                        (no.code == 0) &&
                        (d_tests_canonical(no)) &&
                        (none.kind == D_MATH_VALUE_NONE) &&
                        (d_tests_canonical(none)) &&
                        (range.kind == D_MATH_VALUE_ERROR) &&
                        (range.code == (int)D_MATH_VALUE_ERROR_RANGE) &&
                        (d_tests_canonical(range)) &&
                        (third.kind == D_MATH_VALUE_INTEGER) &&
                        (third.exact.num == 3),
                        "from_int, from_f, from_d, from_ld, from_bool, none, "
                        "from_error, from_rational", most, d);
    }

    return;
}

// d_tests_named
//   results worth naming: the tower's steps, G2's overflow and its flag
// carried on, and the exact comparisons that a conversion would get wrong.
static
void d_tests_named(void)
{
    const struct dj_math_value third = d_math_value_from_ratio(1, 3);
    const struct dj_math_value sixth = d_math_value_from_ratio(1, 6);
    const struct dj_math_value one   = d_math_value_from_int(1);
    const struct dj_math_value two   = d_math_value_from_int(2);
    const struct dj_math_value most  = d_math_value_from_int(D_TESTS_MOST);
    const struct dj_math_value half  = d_math_value_from_d(0.5);

    const struct dj_math_value sum   = d_math_value_add(third, sixth);
    const struct dj_math_value whole =
        d_math_value_add(third, d_math_value_from_ratio(2, 3));
    const struct dj_math_value past  = d_math_value_add(most, one);
    const struct dj_math_value on    = d_math_value_mul(past, two);
    const struct dj_math_value mixed = d_math_value_add(one, half);

    // 1/3 + 1/6 is the rational 1/2; 1/3 + 2/3 the integer 1
    D_TESTS_EXPECT( (sum.kind == D_MATH_VALUE_RATIONAL) &&
                    (sum.exact.num == 1) &&
                    (sum.exact.den == 2) &&
                    (whole.kind == D_MATH_VALUE_INTEGER) &&
                    (whole.exact.num == 1),
                    "1/3 + 1/6 and 1/3 + 2/3", third, sixth);

    // MAX + 1 is the long double 2^(w-1), flagged, and the flag goes on
    D_TESTS_EXPECT( (past.kind == D_MATH_VALUE_LONG_DOUBLE) &&
                    (past.flags == D_MATH_VALUE_FLAG_INEXACT) &&
                    (past.real == (long double)D_TESTS_MOST + 1.0L) &&
                    (on.kind == D_MATH_VALUE_LONG_DOUBLE) &&
                    (on.flags == D_MATH_VALUE_FLAG_INEXACT),
                    "MAX + 1, and twice that", most, one);

    // 1 + 0.5 is the double 1.5
    D_TESTS_EXPECT( (mixed.kind == D_MATH_VALUE_DOUBLE) &&
                    (mixed.real == 1.5L) &&
                    (mixed.flags == 0u),
                    "1 + 0.5", one, half);

    // rationals that no binary floating-point value equals, against their
    // own nearest double and float: the conversion rounds them, the
    // comparison does not
    {
        const struct dj_math_value odd[3] = {
            d_math_value_from_ratio(1, 3),
            d_math_value_from_ratio(-2, 3),
            d_math_value_from_ratio(D_TESTS_MOST, 3)
        };

        int i = 0;

        // each, both ways
        for (i = 0; i < 3; ++i)
        {
            double near_d = 0.0;
            float  near_f = 0.0f;

            (void)d_math_value_to_d(&near_d, odd[i]);
            (void)d_math_value_to_f(&near_f, odd[i]);

            D_TESTS_EXPECT( (!d_math_value_eq(odd[i],
                                              d_math_value_from_d(near_d))) &&
                            (!d_math_value_eq(odd[i],
                                              d_math_value_from_f(near_f))) &&
                            (d_math_value_eq(d_math_value_from_d(near_d),
                                             d_math_value_from_d(near_d))),
                            "a rational against its nearest floats",
                            odd[i], d_math_value_from_d(near_d));
        }
    }

    return;
}


//==============================================================================
// 5.  THE CHECKS
//==============================================================================

// d_tests_pairs
//   every operand alone, and each pair, for every operation and the
// comparison.
static
void d_tests_pairs(void)
{
    int i  = 0;
    int j  = 0;
    int op = 0;

    // each operand
    for (i = 0; i < g_count; ++i)
    {
        d_tests_check_unary(g_values[i]);
        d_tests_check_convert(g_values[i]);

        // with each
        for (j = 0; j < g_count; ++j)
        {
            for (op = D_TESTS_ADD; op <= D_TESTS_DIV; ++op)
            {
                d_tests_check_binary(op, g_values[i], g_values[j]);
            }

            d_tests_check_compare(g_values[i], g_values[j]);
        }
    }

    return;
}

static unsigned long g_state = 31415926UL;

// d_tests_random16
//   16 bits from a 32-bit linear congruential generator: the same in every
// build.
static
unsigned long d_tests_random16(void)
{
    g_state = ((g_state * 1103515245UL) + 12345UL) & 0xFFFFFFFFUL;

    return (g_state >> 16) & 0xFFFFUL;
}

// d_tests_random_bits
//   a dj_math_umax below 2^_bits, _bits at most the word's.
static
dj_math_umax d_tests_random_bits(
    unsigned _bits
)
{
    dj_math_umax value = 0u;
    unsigned    have  = 0u;

    // sixteen bits at a time, then the excess shifted away
    while (have < _bits)
    {
        value = (value << 16) | (dj_math_umax)d_tests_random16();
        have  = have + 16u;
    }

    return value >> (have - _bits);
}

// d_tests_check_from_uint
//   from_uint(_n) against the reference, _n exactly over 1.
static
void d_tests_check_from_uint(
    dj_math_umax _n
)
{
    const struct dj_math_value x = d_math_value_from_uint(_n);

    D_TESTS_EXPECT(d_tests_is_exactly(x,
                                      d_tests_exact_from(
                                          d_tests_big_of_umax(_n, false),
                                          d_tests_big_of_umax(1u, false)),
                                      0u),
                   "from_uint", x, d_math_value_none());

    return;
}

// d_tests_from_uint
//   from_uint against the reference: an integer up to dj_math_imax's top,
// and past it the long double nearest, flagged inexact (G2). Every power of
// two in the word and its neighbours, the top's, and 2,000 drawn at random,
// of every length.
static
void d_tests_from_uint(void)
{
    const unsigned width = D_INTERNAL_RATIONAL_WORD_BITS;

    dj_math_umax around[3] = { 0u, 0u, 0u };
    unsigned    k         = 0u;
    int         i         = 0;
    long        trial     = 0;

    // 2^k - 1, 2^k and 2^k + 1, and at the top the word's last values
    for (k = 0u; k <= width; ++k)
    {
        const dj_math_umax power = (k < width) ? ((dj_math_umax)1 << k) : 0u;

        around[0] = power - 1u;
        around[1] = (k < width) ? power : D_MATH_UMAX_MAX - 1u;
        around[2] = (k < width) ? power + 1u : D_MATH_UMAX_MAX;

        for (i = 0; i < 3; ++i)
        {
            d_tests_check_from_uint(around[i]);
        }
    }

    // any length
    for (trial = 0; trial < 2000L; ++trial)
    {
        d_tests_check_from_uint(d_tests_random_bits(
            1u + (unsigned)(d_tests_random16() % width)));
    }

    return;
}

// d_tests_random_exact
//   an exact value: a numerator below 2^n, either sign, over a denominator
// below 2^m, each length drawn up to w - 1; one time in four an integer.
static
struct dj_math_value d_tests_random_exact(void)
{
    const unsigned    width = D_INTERNAL_RATIONAL_WORD_BITS;
    const unsigned    nbits = (unsigned)(d_tests_random16() % width);
    const unsigned    dbits =
        1u + (unsigned)(d_tests_random16() % (width - 1u));
    const bool        whole = ((d_tests_random16() & 3UL) == 0UL);
    const dj_math_umax n     = d_tests_random_bits(nbits);
    const dj_math_umax drawn = d_tests_random_bits(dbits);
    const bool        neg   = ((d_tests_random16() & 1UL) != 0UL);
    const dj_math_umax d     = ( (whole) ||
                                (drawn == 0u) ) ? 1u : drawn;

    return d_math_value_from_ratio(neg ? -(dj_math_imax)n : (dj_math_imax)n,
                                   (dj_math_imax)d);
}

// d_tests_random
//   D_TESTS_TRIALS pairs of exact values, every operation and the
// comparison on each.
static
void d_tests_random(void)
{
    long trial = 0;
    int  op    = 0;

    // each pair
    for (trial = 0; trial < D_TESTS_TRIALS; ++trial)
    {
        const struct dj_math_value a = d_tests_random_exact();
        const struct dj_math_value b = d_tests_random_exact();

        for (op = D_TESTS_ADD; op <= D_TESTS_DIV; ++op)
        {
            d_tests_check_binary(op, a, b);
        }

        d_tests_check_compare(a, b);
    }

    return;
}


//==============================================================================
// 6.  CONSTANT EVALUATION
//==============================================================================
// From C++14 every operation is a constant expression.

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

static constexpr
struct dj_math_value d_tests_constexpr_sum()
{
    return d_math_value_add(d_math_value_from_ratio(1, 3),
                            d_math_value_from_ratio(1, 6));
}

static constexpr
struct dj_math_value d_tests_constexpr_past()
{
    return d_math_value_add(d_math_value_from_int(D_TESTS_MOST),
                            d_math_value_from_int(1));
}

static constexpr
struct dj_math_value d_tests_constexpr_mixed()
{
    return d_math_value_add(d_math_value_from_int(1),
                            d_math_value_from_d(0.5));
}

static constexpr enum dj_math_value_order
d_tests_constexpr_order()
{
    return d_math_value_cmp(
        d_math_value_from_int(((dj_math_imax)1 << 53) + 1),
        d_math_value_from_d(9007199254740992.0));
}

static constexpr
struct dj_math_value d_tests_constexpr_pole()
{
    return d_math_value_div(d_math_value_from_int(1),
                            d_math_value_from_int(0));
}

static constexpr
struct dj_math_value d_tests_constexpr_uint()
{
    return d_math_value_from_uint(D_MATH_UMAX_MAX);
}

static_assert( (d_tests_constexpr_sum().kind == D_MATH_VALUE_RATIONAL) &&
               (d_tests_constexpr_sum().exact.num == 1) &&
               (d_tests_constexpr_sum().exact.den == 2),
               "1/3 + 1/6 is the rational 1/2 as a constant expression");
static_assert( (d_tests_constexpr_past().kind ==
                D_MATH_VALUE_LONG_DOUBLE) &&
               (d_tests_constexpr_past().flags ==
                D_MATH_VALUE_FLAG_INEXACT) &&
               (d_tests_constexpr_past().real == 9223372036854775808.0L),
               "MAX + 1 is a flagged long double as a constant expression");
static_assert( (d_tests_constexpr_mixed().kind == D_MATH_VALUE_DOUBLE) &&
               (d_tests_constexpr_mixed().real == 1.5L),
               "1 + 0.5 is the double 1.5 as a constant expression");
static_assert(d_tests_constexpr_order() == D_MATH_VALUE_GREATER,
              "2^53 + 1 is above the double 2^53 as a constant expression");
static_assert( (d_tests_constexpr_pole().kind == D_MATH_VALUE_ERROR) &&
               (d_tests_constexpr_pole().code ==
                (int)D_MATH_VALUE_ERROR_POLE),
               "1 / 0 is a pole as a constant expression");
static_assert( (d_tests_constexpr_uint().kind ==
                D_MATH_VALUE_LONG_DOUBLE) &&
               (d_tests_constexpr_uint().flags ==
                D_MATH_VALUE_FLAG_INEXACT) &&
               (d_tests_constexpr_uint().real ==
                (long double)D_MATH_UMAX_MAX),
               "the greatest dj_math_umax is a flagged long double as a "
               "constant expression");

// d_tests_constant_evaluation
//   the same at run time.
static
void d_tests_constant_evaluation(void)
{
    const struct dj_math_value sum  = d_tests_constexpr_sum();
    const struct dj_math_value past = d_tests_constexpr_past();

    D_TESTS_EXPECT( (sum.kind == D_MATH_VALUE_RATIONAL) &&
                    (sum.exact.den == 2) &&
                    (past.flags == D_MATH_VALUE_FLAG_INEXACT) &&
                    (d_tests_constexpr_mixed().real == 1.5L) &&
                    (d_tests_constexpr_order() == D_MATH_VALUE_GREATER) &&
                    (d_tests_constexpr_pole().code ==
                     (int)D_MATH_VALUE_ERROR_POLE) &&
                    (d_tests_constexpr_uint().flags ==
                     D_MATH_VALUE_FLAG_INEXACT),
                    "the constant expressions, at run time", sum, past);

    return;
}

#endif  // C++14


int main(void)
{
    const struct dj_math_value none = d_math_value_none();

    d_tests_make_values();
    d_tests_construction();
    d_tests_from_uint();
    d_tests_named();
    d_tests_pairs();
    d_tests_random();
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    // the reference never needed more than its bits
    D_TESTS_EXPECT(!g_reference_overrun, "the reference's 256 bits held",
                   none, none);

    printf("t_value: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
