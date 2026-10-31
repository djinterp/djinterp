/*******************************************************************************
* djinterp [c/math]                                                      value.h
*
* One run-time type for any number the math module computes with: the C core
* of the generic value (roadmap Phase 1).
*   A value has a kind. The numbers stand in a tower: integer and rational,
* which are exact, then float, double and long double, which are not. The
* other kinds are a boolean, an error (domain, pole, range, invalid or
* memory) and none, the absence of a value. Each kind's value lives in one
* field; the fields a kind does not use hold 0, so a value has one
* representation. A kind that does not fit in the value is boxed (below).
*
*   THE TOWER. An operation on two exact numbers is exact: integers stay
* integers, a quotient is a rational, and a rational whose denominator comes
* to 1 is an integer. Where an exact result does not fit dj_math_imax, it is
* kept as the long double nearest it, and the value is flagged
* D_MATH_VALUE_FLAG_INEXACT (roadmap gate G2, until bignum). An exact number
* with an inexact one is converted, correctly rounded, to the inexact one's
* type; two inexact numbers meet in the wider type. Floating-point results
* follow IEEE 754, so 1.0 / 0.0 is an infinity; only an exact division by
* exact zero gives an error: a pole, or invalid for 0 / 0.
*   ERRORS ARE VALUES. An error operand passes through any operation, the
* first one where there are two. A boolean or none in arithmetic gives a
* domain error. Flags are kept: a result carries every flag its operands did.
*   COMPARISON is exact across kinds: with 64-bit words, 2^53 + 1 is above
* the double 2^53, though the two convert to the same double. A NaN, and an
* error, are unordered, even with themselves; booleans order false below
* true; none equals none; and numbers, booleans and none do not compare with
* each other.
*
*   BOXED KINDS. A kind that is not inline (a wide integer, and later
* bignum's kinds, complex numbers, vectors and matrices) registers by filling
* in a table, struct dj_math_boxed_kind: its name; its rank, which is its place
* in the tower and its own; whether it is a number, and whether an exact one;
* and its operations: retain and release, arithmetic, comparison, conversion
* and text. A value of such a kind is D_MATH_VALUE_BOXED, with type pointing
* to the table and box to the kind's storage. Since each value carries its
* kind's table, there is no list of kinds and no global state, and a new
* kind changes neither this header nor the inline kinds' code.
*   In an operation with a boxed operand, the operand higher in the tower
* handles it. A boxed kind handles it through its table. An inline number
* handles it by converting the boxed operand to its own kind, through the
* boxed kind's table, then going on as with two inline numbers. To compare,
* it converts the boxed operand to the nearest long double, and where that
* equals the inline number, the rounding's direction gives the order, so
* comparison stays exact. Before any kind is asked, an error passes through,
* and a boolean or none is a domain error, or unordered in a comparison, as
* with inline operands.
*   A box is its kind's to keep. A value an operation returns is the caller's,
* to give back with d_math_value_release, which does nothing more for an
* inline kind. Operands are lent: an operation never keeps or releases one.
* d_math_value_retain makes a copy, which is released in its turn.
*
*   ONE TEXT, BOTH LANGUAGES. Every function is a header inline in C and
* below C++14, and from C++14 a constant expression for the inline kinds
* (D_MATH_FN). There is no union, which constant evaluation could not switch
* between members of before C++20. A boxed kind's operations are its own,
* and run at run time.
*
*
* path:      /inc/djinterp/c/modules/math/value.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.18
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LAYOUT
    ------
    1.  Kinds and errors
         1.  dj_math_value_kind
         2.  dj_math_value_error
         3.  dj_math_value_order
         4.  D_MATH_VALUE_FLAG_INEXACT
    2.  The tower and the operations
         1.  D_MATH_VALUE_RANK_INTEGER to D_MATH_VALUE_RANK_LONG_DOUBLE
         2.  dj_math_value_op
    3.  The value
         1.  dj_math_value
    4.  Boxed kinds
         1.  D_MATH_BOXED_NUMBER, D_MATH_BOXED_EXACT
         2.  dj_math_boxed_retain_fn to dj_math_boxed_text_fn
         3.  dj_math_boxed_kind
2.  CONSTRUCTION
    ------------
    1.  None, errors and booleans
    2.  Exact numbers
    3.  Floating-point numbers
    4.  d_math_value_retain, d_math_value_release
3.  QUERIES AND CONVERSIONS
    -----------------------
    1.  d_math_value_is_number, d_math_value_is_exact
    2.  d_math_value_rank, d_math_value_kind_name
    3.  Internal helpers
    4.  d_math_value_to_f, d_math_value_to_d, d_math_value_to_ld
    5.  d_math_value_to_rational
4.  ARITHMETIC
    ----------
    1.  Internal helpers
    2.  d_math_value_neg, d_math_value_abs
    3.  d_math_value_add, d_math_value_sub, d_math_value_mul, d_math_value_div
5.  COMPARISON
    ----------
    1.  Internal helpers
    2.  d_math_value_cmp, d_math_value_eq
*/

#ifndef DJINTERP_C_MATH_VALUE_H
#define DJINTERP_C_MATH_VALUE_H 1

// std
#include <float.h>           // LDBL_MANT_DIG, LDBL_MAX
#include <stddef.h>          // size_t, NULL
// djinterp
#include "./math_common.h"   // D_MATH_FN, dj_math_imax, dj_math_umax
#include "./rational.h"      // dj_rational, its kernels and conversions


//==============================================================================
// 1.  LAYOUT
//==============================================================================


// 1.1    Kinds and errors
//------------------------------------------------------------------------------
// 1.1.1
// dj_math_value_kind
//   enum: what a value holds. The numbers come in the tower's order, so the
// wider of two inexact kinds is the greater.
enum dj_math_value_kind
{
    D_MATH_VALUE_NONE        = 0,  // no value
    D_MATH_VALUE_ERROR       = 1,  // an error, in code
    D_MATH_VALUE_BOOLEAN     = 2,  // true or false, in code
    D_MATH_VALUE_INTEGER     = 3,  // exact, in exact: n/1
    D_MATH_VALUE_RATIONAL    = 4,  // exact, in exact: its denominator not 1
    D_MATH_VALUE_FLOAT       = 5,  // inexact, in real: a float's value
    D_MATH_VALUE_DOUBLE      = 6,  // inexact, in real: a double's value
    D_MATH_VALUE_LONG_DOUBLE = 7,  // inexact, in real
    D_MATH_VALUE_BOXED       = 8   // a boxed kind's: its table in type, its
                                   // storage in box
};

// 1.1.2
// dj_math_value_error
//   enum: why there is no number.
enum dj_math_value_error
{
    D_MATH_VALUE_ERROR_DOMAIN  = 1,  // an operand outside the operation's
                                     // domain: a boolean or none in arithmetic
    D_MATH_VALUE_ERROR_POLE    = 2,  // an exact nonzero number over exact 0
    D_MATH_VALUE_ERROR_RANGE   = 3,  // a result past every kind; none yet
    D_MATH_VALUE_ERROR_INVALID = 4,  // exact 0 over exact 0
    D_MATH_VALUE_ERROR_MEMORY  = 5   // no storage for a boxed kind's result
};

// 1.1.3
// dj_math_value_order
//   enum: how one value stands to another.
enum dj_math_value_order
{
    D_MATH_VALUE_LESS      = -1,  // below
    D_MATH_VALUE_EQUAL     = 0,   // the same
    D_MATH_VALUE_GREATER   = 1,   // above
    D_MATH_VALUE_UNORDERED = 2    // no order: a NaN, an error, or unlike
                                  // kinds
};

// 1.1.4
// D_MATH_VALUE_FLAG_INEXACT
//   constant: the flag of a value that an exact operation should have given
// exactly, but whose result did not fit dj_math_imax and is the long double
// nearest it instead (G2); and of every value computed from one.
#define D_MATH_VALUE_FLAG_INEXACT  1u

// 1.2    The tower and the operations
//------------------------------------------------------------------------------
// 1.2.1
// D_MATH_VALUE_RANK_INTEGER to D_MATH_VALUE_RANK_LONG_DOUBLE
//   constants: the inline numbers' places in the tower, as ranks. A boxed
// kind takes a rank of its own between or beyond them: an exact kind past
// the rational's and below the float's, as a wide integer or bignum's kinds;
// an inexact one past the long double's, as a big float; and a kind that
// holds numbers, as a vector does, past every number's. Of two operands,
// the one of the higher rank handles an operation; none, an error and a
// boolean have no rank (d_math_value_rank gives them 0).
#define D_MATH_VALUE_RANK_INTEGER      100
#define D_MATH_VALUE_RANK_RATIONAL     200
#define D_MATH_VALUE_RANK_FLOAT        300
#define D_MATH_VALUE_RANK_DOUBLE       400
#define D_MATH_VALUE_RANK_LONG_DOUBLE  500

// 1.2.2
// dj_math_value_op
//   enum: an operation, as a boxed kind's table is asked for it.
enum dj_math_value_op
{
    D_MATH_VALUE_OP_ADD = 0,  // a + b
    D_MATH_VALUE_OP_SUB = 1,  // a - b
    D_MATH_VALUE_OP_MUL = 2,  // a * b
    D_MATH_VALUE_OP_DIV = 3,  // a / b
    D_MATH_VALUE_OP_NEG = 4,  // -x
    D_MATH_VALUE_OP_ABS = 5   // |x|
};

// 1.3    The value
//------------------------------------------------------------------------------
// 1.3.1
// dj_math_value
//   struct: a value of any kind. kind says which field holds it; the others
// hold 0 (exact 0/1, type and box NULL), so a value has one representation.
struct dj_math_boxed_kind;

struct dj_math_value
{
    enum dj_math_value_kind          kind;   // what the value is
    unsigned                        flags;  // D_MATH_VALUE_FLAG_*
    int                             code;   // a boolean, 0 or 1; or an error
    struct dj_rational               exact;  // an integer, as n/1; or a
                                            // rational, in the one form
    long double                     real;   // a float, double or long
                                            // double's value, held exactly
    const struct dj_math_boxed_kind* type;   // a boxed value's kind: its table
    void*                           box;    // a boxed value's storage
};

// 1.4    Boxed kinds
//------------------------------------------------------------------------------
// 1.4.1
// D_MATH_BOXED_NUMBER, D_MATH_BOXED_EXACT
//   constants: a boxed kind's traits. A number stands in the tower and
// converts to the inline numbers; an exact number is exact, as an integer
// or a rational is. A kind with neither, as a vector, is no number.
#define D_MATH_BOXED_NUMBER  1u
#define D_MATH_BOXED_EXACT   2u

// 1.4.2
// dj_math_boxed_retain_fn to dj_math_boxed_text_fn
//   types: a boxed kind's entries. Each is given values of its kind, as
// below, and every entry must be set.
//   retain and release: one holder more of _box, and one fewer; the last
// release frees it.
//   unary: -_x for D_MATH_VALUE_OP_NEG, |_x| for D_MATH_VALUE_OP_ABS.
//   binary: _a _op _b, for the four operations, where one operand is of
// this kind and the other is a number, or a boxed value, whose rank is no
// higher; neither is an error, a boolean or none.
//   cmp: how _a stands to _b, on the same terms.
//   A result is the caller's, as every result is, and may be of any kind:
// an error, D_MATH_VALUE_ERROR_MEMORY where there was no storage for it,
// included. value.h adds the operands' flags to it.
//   convert: a number of this kind as an inline number of kind _to, into
// *_out, and true. For D_MATH_VALUE_INTEGER and D_MATH_VALUE_RATIONAL, its
// exact value, an integer or a rational in the one form, where it is one
// that fits dj_math_imax, with *_direction 0; otherwise false. For
// D_MATH_VALUE_FLOAT, _DOUBLE and _LONG_DOUBLE, the nearest value of that
// type, ties to even, an infinity past the largest; *_direction is -1, 0 or
// 1 as it is below, equal to or above the number (0 for a NaN).
//   text: _x as text, into _buffer: at most _size characters, the closing NUL
// among them (nothing where _size is 0). It returns the whole text's length
// without the NUL, whatever _size is, as snprintf does.
typedef void (*dj_math_boxed_retain_fn)(void* _box);
typedef void (*dj_math_boxed_release_fn)(void* _box);
typedef struct dj_math_value (*dj_math_boxed_unary_fn)(
    enum dj_math_value_op _op,
    struct dj_math_value  _x);
typedef struct dj_math_value (*dj_math_boxed_binary_fn)(
    enum dj_math_value_op _op,
    struct dj_math_value  _a,
    struct dj_math_value  _b);
typedef enum dj_math_value_order (*dj_math_boxed_cmp_fn)(
    struct dj_math_value _a,
    struct dj_math_value _b);
typedef bool (*dj_math_boxed_convert_fn)(
    struct dj_math_value*   _out,
    int*                   _direction,
    struct dj_math_value    _x,
    enum dj_math_value_kind _to);
typedef size_t (*dj_math_boxed_text_fn)(
    char*               _buffer,
    size_t              _size,
    struct dj_math_value _x);

// 1.4.3
// dj_math_boxed_kind
//   struct: a boxed kind's table, which is its registration. A kind's rank is
// its own: no two kinds share one, so a kind knows its values by their rank,
// whichever translation unit's copy of the table they point to. It is not
// an inline number's rank either.
struct dj_math_boxed_kind
{
    const char*             name;     // the kind's name: "wide integer"
    int                     rank;     // its place in the tower
    unsigned                traits;   // D_MATH_BOXED_*
    dj_math_boxed_retain_fn  retain;   // one holder more
    dj_math_boxed_release_fn release;  // one holder fewer
    dj_math_boxed_unary_fn   unary;    // -x, |x|
    dj_math_boxed_binary_fn  binary;   // a + b, a - b, a * b, a / b
    dj_math_boxed_cmp_fn     cmp;      // how a stands to b
    dj_math_boxed_convert_fn convert;  // to an inline number
    dj_math_boxed_text_fn    text;     // as text
};


//==============================================================================
// 2.  CONSTRUCTION
//==============================================================================

// d_internal_math_value_blank
//   a value of kind _kind with _flags and every field 0: what each
// construction starts from.
D_MATH_FN struct dj_math_value
d_internal_math_value_blank(
    enum dj_math_value_kind _kind,
    unsigned               _flags
)
{
    struct dj_math_value result = { D_MATH_VALUE_NONE, 0u, 0, { 0, 1 }, 0.0L,
                                   NULL, NULL };

    result.kind  = _kind;
    result.flags = _flags;

    return result;
}


// 2.1    None, errors and booleans
//------------------------------------------------------------------------------

D_MATH_FN
struct dj_math_value d_math_value_none(void)
{
    return d_internal_math_value_blank(D_MATH_VALUE_NONE, 0u);
}

D_MATH_FN struct dj_math_value
d_math_value_from_error(
    enum dj_math_value_error _error
)
{
    struct dj_math_value result =
        d_internal_math_value_blank(D_MATH_VALUE_ERROR, 0u);

    result.code = (int)_error;

    return result;
}

D_MATH_FN
struct dj_math_value d_math_value_from_bool(
    bool _truth
)
{
    struct dj_math_value result =
        d_internal_math_value_blank(D_MATH_VALUE_BOOLEAN, 0u);

    result.code = (_truth) ? 1 : 0;

    return result;
}

// 2.2    Exact numbers
//------------------------------------------------------------------------------

D_MATH_FN
struct dj_math_value d_math_value_from_int(
    dj_math_imax _n
)
{
    struct dj_math_value result =
        d_internal_math_value_blank(D_MATH_VALUE_INTEGER, 0u);

    result.exact.num = _n;

    return result;
}

// d_internal_math_value_from_ratio
//   the value of the exact result _r, with _flags: an integer where its
// denominator is 1, a rational where it fits otherwise, and where it does
// not, the long double nearest it, flagged inexact (G2).
D_MATH_FN
struct dj_math_value d_internal_math_value_from_ratio(
    struct dj_internal_rational_ratio _r,
    unsigned                          _flags
)
{
    struct dj_rational   packed = { 0, 1 };
    struct dj_math_value result =
        d_internal_math_value_blank(D_MATH_VALUE_RATIONAL, _flags);

    // past the family: the nearest long double
    if (d_internal_rational_pack_ratio(&packed, _r) != D_RATIONAL_OK)
    {
        result       = d_internal_math_value_blank(
            D_MATH_VALUE_LONG_DOUBLE, _flags | D_MATH_VALUE_FLAG_INEXACT);
        result.real  = d_internal_rational_wide_to_ld(_r.negative, _r.num,
                                                      _r.den);

        return result;
    }

    // a denominator of 1: an integer
    if (packed.den == 1)
    {
        result.kind = D_MATH_VALUE_INTEGER;
    }

    result.exact = packed;

    return result;
}

// d_math_value_from_uint
//   _n exactly: an integer where it fits dj_math_imax; past D_MATH_IMAX_MAX,
// the long double nearest it, flagged inexact, as any exact result too wide
// for dj_math_imax is (G2).
D_MATH_FN
struct dj_math_value d_math_value_from_uint(
    dj_math_umax _n
)
{
    return d_internal_math_value_from_ratio(
        d_internal_rational_ratio_of(false, _n, 1u), 0u);
}

// d_math_value_from_ratio
//   _num / _den, exactly: an integer or a rational, in the one form. Over 0,
// a pole, or invalid for 0 / 0; and D_MATH_IMAX_MIN / -1, which does not
// fit, the long double nearest it, flagged inexact.
D_MATH_FN
struct dj_math_value d_math_value_from_ratio(
    dj_math_imax _num,
    dj_math_imax _den
)
{
    const dj_math_umax n = d_internal_rational_magnitude(_num);
    const dj_math_umax d = d_internal_rational_magnitude(_den);

    // over 0
    if (_den == 0)
    {
        return d_math_value_from_error((_num == 0) ? D_MATH_VALUE_ERROR_INVALID
                                              : D_MATH_VALUE_ERROR_POLE);
    }

    const dj_math_umax g = d_internal_rational_gcd(n, d);

    return d_internal_math_value_from_ratio(
        d_internal_rational_ratio_of(((_num < 0) != (_den < 0)), n / g, d / g),
        0u);
}

D_MATH_FN
struct dj_math_value d_math_value_from_rational(
    struct dj_rational _x
)
{
    return d_math_value_from_ratio(_x.num, _x.den);
}

// 2.3    Floating-point numbers
//------------------------------------------------------------------------------

D_MATH_FN
struct dj_math_value d_math_value_from_f(
    float _x
)
{
    struct dj_math_value result =
        d_internal_math_value_blank(D_MATH_VALUE_FLOAT, 0u);

    result.real = (long double)_x;

    return result;
}

D_MATH_FN
struct dj_math_value d_math_value_from_d(
    double _x
)
{
    struct dj_math_value result =
        d_internal_math_value_blank(D_MATH_VALUE_DOUBLE, 0u);

    result.real = (long double)_x;

    return result;
}

D_MATH_FN
struct dj_math_value d_math_value_from_ld(
    long double _x
)
{
    struct dj_math_value result =
        d_internal_math_value_blank(D_MATH_VALUE_LONG_DOUBLE, 0u);

    result.real = _x;

    return result;
}

// 2.4    d_math_value_retain, d_math_value_release
//------------------------------------------------------------------------------
// retain gives _x again, with one holder more of its box where it is boxed:
// a copy, to be released in its turn. release gives up *_x's hold on its
// box, where it is boxed, and leaves *_x none; NULL is ignored. For an
// inline kind, neither does more.

D_MATH_FN
struct dj_math_value d_math_value_retain(
    struct dj_math_value _x
)
{
    // a box: one holder more
    if (_x.kind == D_MATH_VALUE_BOXED)
    {
        _x.type->retain(_x.box);
    }

    return _x;
}

D_MATH_FN
void d_math_value_release(
    struct dj_math_value* _x
)
{
    // nothing to release
    if (_x == NULL)
    {
        return;
    }

    // a box: one holder fewer
    if (_x->kind == D_MATH_VALUE_BOXED)
    {
        _x->type->release(_x->box);
    }

    *_x = d_math_value_none();

    return;
}


//==============================================================================
// 3.  QUERIES AND CONVERSIONS
//==============================================================================


// 3.1    d_math_value_is_number, d_math_value_is_exact
//------------------------------------------------------------------------------
// A boxed value is a number, or an exact one, as its kind's traits say.

D_MATH_FN
bool d_math_value_is_number(
    struct dj_math_value _x
)
{
    // boxed: its kind's traits
    if (_x.kind == D_MATH_VALUE_BOXED)
    {
        return ((_x.type->traits & D_MATH_BOXED_NUMBER) != 0u);
    }

    return ( (_x.kind >= D_MATH_VALUE_INTEGER) &&
             (_x.kind <= D_MATH_VALUE_LONG_DOUBLE) );
}

D_MATH_FN
bool d_math_value_is_exact(
    struct dj_math_value _x
)
{
    // boxed: its kind's traits
    if (_x.kind == D_MATH_VALUE_BOXED)
    {
        return ( ((_x.type->traits & D_MATH_BOXED_NUMBER) != 0u) &&
                 ((_x.type->traits & D_MATH_BOXED_EXACT) != 0u) );
    }

    return ( (_x.kind == D_MATH_VALUE_INTEGER) ||
             (_x.kind == D_MATH_VALUE_RATIONAL) );
}

// 3.2    d_math_value_rank, d_math_value_kind_name
//------------------------------------------------------------------------------
// rank is _x's place in the tower: an inline number's D_MATH_VALUE_RANK_*, a
// boxed kind's own rank, and 0 for none, an error or a boolean. kind_name is
// its kind's name: "none", "error", "boolean", "integer", "rational",
// "float", "double", "long double", or a boxed kind's own ("unknown" for a
// kind out of range).

D_MATH_FN
int d_math_value_rank(
    struct dj_math_value _x
)
{
    // by kind
    switch (_x.kind)
    {
        case D_MATH_VALUE_BOXED:
            return _x.type->rank;

        case D_MATH_VALUE_INTEGER:
            return D_MATH_VALUE_RANK_INTEGER;

        case D_MATH_VALUE_RATIONAL:
            return D_MATH_VALUE_RANK_RATIONAL;

        case D_MATH_VALUE_FLOAT:
            return D_MATH_VALUE_RANK_FLOAT;

        case D_MATH_VALUE_DOUBLE:
            return D_MATH_VALUE_RANK_DOUBLE;

        case D_MATH_VALUE_LONG_DOUBLE:
            return D_MATH_VALUE_RANK_LONG_DOUBLE;

        default:
            return 0;
    }
}

D_MATH_FN
const char* d_math_value_kind_name(
    struct dj_math_value _x
)
{
    // by kind
    switch (_x.kind)
    {
        case D_MATH_VALUE_BOXED:
            return _x.type->name;

        case D_MATH_VALUE_NONE:
            return "none";

        case D_MATH_VALUE_ERROR:
            return "error";

        case D_MATH_VALUE_BOOLEAN:
            return "boolean";

        case D_MATH_VALUE_INTEGER:
            return "integer";

        case D_MATH_VALUE_RATIONAL:
            return "rational";

        case D_MATH_VALUE_FLOAT:
            return "float";

        case D_MATH_VALUE_DOUBLE:
            return "double";

        case D_MATH_VALUE_LONG_DOUBLE:
            return "long double";

        default:
            return "unknown";
    }
}

// 3.3    Internal helpers
//------------------------------------------------------------------------------

// d_internal_math_value_unbox
//   *_x, a boxed number, replaced by its value as an inline number of kind
// _to, through its kind's conversion; *_direction is how that value stands
// to it, -1 below, 0 equal or 1 above. False, *_x as it was, where it is no
// number or does not convert. Its flags are not kept: every caller has
// taken them already.
D_MATH_FN bool
d_internal_math_value_unbox(
    struct dj_math_value*   _x,
    int*                   _direction,
    enum dj_math_value_kind _to
)
{
    struct dj_math_value converted = d_math_value_none();

    *_direction = 0;

    // no number, or none of that kind
    if ( (!d_math_value_is_number(*_x)) ||
         (!_x->type->convert(&converted, _direction, *_x, _to)) )
    {
        *_direction = 0;

        return false;
    }

    *_x = converted;

    return true;
}

// 3.4    d_math_value_to_f, d_math_value_to_d, d_math_value_to_ld
//------------------------------------------------------------------------------
// _x as a float, double or long double, into *_out, and true; false, and
// *_out as it was, where _x is no number. An exact number converts to the
// nearest value, ties to even; an inexact one as C converts it; a boxed one
// as its kind converts it to that type.

D_MATH_FN
bool d_math_value_to_f(
    float*               _out,
    struct dj_math_value _x
)
{
    int direction = 0;

    // boxed: its kind's float, or no number
    if ( (_x.kind == D_MATH_VALUE_BOXED) &&
         (!d_internal_math_value_unbox(&_x, &direction, D_MATH_VALUE_FLOAT)) )
    {
        return false;
    }

    // no number
    if (!d_math_value_is_number(_x))
    {
        return false;
    }

    *_out = d_math_value_is_exact(_x) ? d_rational_to_f(_x.exact)
                                      : (float)_x.real;

    return true;
}

D_MATH_FN
bool d_math_value_to_d(
    double*              _out,
    struct dj_math_value _x
)
{
    int direction = 0;

    // boxed: its kind's double, or no number
    if ( (_x.kind == D_MATH_VALUE_BOXED) &&
         (!d_internal_math_value_unbox(&_x, &direction, D_MATH_VALUE_DOUBLE)) )
    {
        return false;
    }

    // no number
    if (!d_math_value_is_number(_x))
    {
        return false;
    }

    *_out = d_math_value_is_exact(_x) ? d_rational_to_d(_x.exact)
                                      : (double)_x.real;

    return true;
}

D_MATH_FN
bool d_math_value_to_ld(
    long double*         _out,
    struct dj_math_value _x
)
{
    int direction = 0;

    // boxed: its kind's long double, or no number
    if ( (_x.kind == D_MATH_VALUE_BOXED) &&
         (!d_internal_math_value_unbox(&_x, &direction,
                                       D_MATH_VALUE_LONG_DOUBLE)) )
    {
        return false;
    }

    // no number
    if (!d_math_value_is_number(_x))
    {
        return false;
    }

    *_out = d_math_value_is_exact(_x) ? d_rational_to_ld(_x.exact)
                                      : _x.real;

    return true;
}

// 3.5    d_math_value_to_rational
//------------------------------------------------------------------------------
// an exact _x into *_out, and true; false, and *_out as it was, otherwise.
// A boxed exact number converts where its kind gives it as a dj_rational.

D_MATH_FN
bool d_math_value_to_rational(
    struct dj_rational*  _out,
    struct dj_math_value _x
)
{
    int direction = 0;

    // boxed: exact, and in the family, or not converted
    if ( (_x.kind == D_MATH_VALUE_BOXED) &&
         ( (!d_math_value_is_exact(_x)) ||
           (!d_internal_math_value_unbox(&_x, &direction,
                                         D_MATH_VALUE_RATIONAL)) ) )
    {
        return false;
    }

    // inexact, or no number
    if (!d_math_value_is_exact(_x))
    {
        return false;
    }

    *_out = _x.exact;

    return true;
}


//==============================================================================
// 4.  ARITHMETIC
//==============================================================================
// A result's flags are its operands', and D_MATH_VALUE_FLAG_INEXACT where an
// exact result did not fit. Floating-point arithmetic is one operation per
// statement, rounded to its type as it is assigned.


// 4.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_math_value_as_f, _as_d, _as_ld
//   a number of a kind at most the type's: exact, correctly rounded; a
// narrower float kind, exactly.
D_MATH_FN
float d_internal_math_value_as_f(
    struct dj_math_value _x
)
{
    return d_math_value_is_exact(_x) ? d_rational_to_f(_x.exact)
                                     : (float)_x.real;
}

D_MATH_FN
double d_internal_math_value_as_d(
    struct dj_math_value _x
)
{
    return d_math_value_is_exact(_x) ? d_rational_to_d(_x.exact)
                                     : (double)_x.real;
}

D_MATH_FN
long double d_internal_math_value_as_ld(
    struct dj_math_value _x
)
{
    return d_math_value_is_exact(_x) ? d_rational_to_ld(_x.exact)
                                     : _x.real;
}

// d_internal_math_value_op_f, _op_d, _op_ld
//   _a _op _b in the type, one rounding.
D_MATH_FN float
d_internal_math_value_op_f(
    enum dj_math_value_op _op,
    float                _a,
    float                _b
)
{
    float result = (float)0;

    // the operation
    if (_op == D_MATH_VALUE_OP_ADD)
    {
        result = _a + _b;
    }
    else if (_op == D_MATH_VALUE_OP_SUB)
    {
        result = _a - _b;
    }
    else if (_op == D_MATH_VALUE_OP_MUL)
    {
        result = _a * _b;
    }
    else
    {
        result = _a / _b;
    }

    return result;
}

D_MATH_FN double
d_internal_math_value_op_d(
    enum dj_math_value_op _op,
    double               _a,
    double               _b
)
{
    double result = (double)0;

    // the operation
    if (_op == D_MATH_VALUE_OP_ADD)
    {
        result = _a + _b;
    }
    else if (_op == D_MATH_VALUE_OP_SUB)
    {
        result = _a - _b;
    }
    else if (_op == D_MATH_VALUE_OP_MUL)
    {
        result = _a * _b;
    }
    else
    {
        result = _a / _b;
    }

    return result;
}

D_MATH_FN long double
d_internal_math_value_op_ld(
    enum dj_math_value_op _op,
    long double          _a,
    long double          _b
)
{
    long double result = (long double)0;

    // the operation
    if (_op == D_MATH_VALUE_OP_ADD)
    {
        result = _a + _b;
    }
    else if (_op == D_MATH_VALUE_OP_SUB)
    {
        result = _a - _b;
    }
    else if (_op == D_MATH_VALUE_OP_MUL)
    {
        result = _a * _b;
    }
    else
    {
        result = _a / _b;
    }

    return result;
}

// d_internal_math_value_exact_op
//   _a _op _b for two exact numbers, exactly: through rational.h's sums and
// products, whose results come back whole, fitting or not. Over exact 0, a
// pole, or invalid for 0 / 0.
D_MATH_FN struct dj_math_value
d_internal_math_value_exact_op(
    enum dj_math_value_op _op,
    struct dj_rational    _a,
    struct dj_rational    _b,
    unsigned             _flags
)
{
    const dj_math_umax an = d_internal_rational_magnitude(_a.num);
    const dj_math_umax ad = (dj_math_umax)_a.den;
    const dj_math_umax bn = d_internal_rational_magnitude(_b.num);
    const dj_math_umax bd = (dj_math_umax)_b.den;

    struct dj_math_value result = d_math_value_none();

    // a sum, or a difference: the second term's sign turned
    if ( (_op == D_MATH_VALUE_OP_ADD) ||
         (_op == D_MATH_VALUE_OP_SUB) )
    {
        const bool b_negative = (_op == D_MATH_VALUE_OP_ADD)
                                    ? (_b.num < 0)
                                    : (_b.num > 0);

        return d_internal_math_value_from_ratio(
            d_internal_rational_sum_ratio((_a.num < 0), an, ad,
                                          b_negative, bn, bd),
            _flags);
    }

    // a product
    if (_op == D_MATH_VALUE_OP_MUL)
    {
        return d_internal_math_value_from_ratio(
            d_internal_rational_product_ratio(((_a.num < 0) != (_b.num < 0)),
                                              an, ad, bn, bd),
            _flags);
    }

    // a quotient over 0: a pole, or invalid for 0 / 0
    if (_b.num == 0)
    {
        result       = d_math_value_from_error((_a.num == 0)
                                              ? D_MATH_VALUE_ERROR_INVALID
                                              : D_MATH_VALUE_ERROR_POLE);
        result.flags = _flags;

        return result;
    }

    // a quotient: _a times _b's reciprocal, its magnitudes swapped
    return d_internal_math_value_from_ratio(
        d_internal_rational_product_ratio(((_a.num < 0) != (_b.num < 0)),
                                          an, ad, bd, bn),
        _flags);
}

// d_internal_math_value_handler
//   the table of the boxed kind that handles an operation on _a and _b, one
// of them boxed: the boxed operand's, where its rank is at least the
// other's, which for two of one rank is one kind; NULL where the other is
// an inline number of a higher rank.
D_MATH_FN
const struct dj_math_boxed_kind* d_internal_math_value_handler(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    const int a_rank = d_math_value_rank(_a);
    const int b_rank = d_math_value_rank(_b);

    // a boxed operand as high as the other
    if ( (_a.kind == D_MATH_VALUE_BOXED) &&
         (a_rank >= b_rank) )
    {
        return _a.type;
    }

    if ( (_b.kind == D_MATH_VALUE_BOXED) &&
         (b_rank >= a_rank) )
    {
        return _b.type;
    }

    return NULL;
}

// d_internal_math_value_is_other
//   whether _x is none, an error or a boolean: no number, and not boxed.
D_MATH_FN
bool d_internal_math_value_is_other(
    struct dj_math_value _x
)
{
    return ( (_x.kind == D_MATH_VALUE_NONE)  ||
             (_x.kind == D_MATH_VALUE_ERROR) ||
             (_x.kind == D_MATH_VALUE_BOOLEAN) );
}

// d_internal_math_value_boxed_binary
//   *_a _op *_b, one or both boxed and neither an error, into *_result, and
// true, where it is settled here: a boolean or none beside a boxed value is
// a domain error, and a boxed kind at least as high as the other operand
// computes it, the operands' flags added. Otherwise an inline number of a
// higher rank handles it: the boxed operand is replaced by its value in
// that number's kind, and false sends the two on as inline numbers; a
// boxed operand that does not convert is a domain error.
D_MATH_FN bool
d_internal_math_value_boxed_binary(
    struct dj_math_value* _result,
    enum dj_math_value_op _op,
    struct dj_math_value* _a,
    struct dj_math_value* _b
)
{
    const unsigned flags = _a->flags | _b->flags;

    const struct dj_math_boxed_kind* handler   = NULL;
    struct dj_math_value*            low       = NULL;
    int                             direction = 0;

    // a boolean or none: outside the domain
    if ( (d_internal_math_value_is_other(*_a)) ||
         (d_internal_math_value_is_other(*_b)) )
    {
        *_result       = d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
        _result->flags = flags;

        return true;
    }

    handler = d_internal_math_value_handler(*_a, *_b);

    // a boxed kind as high as the other operand: its own arithmetic
    if (handler != NULL)
    {
        *_result       = handler->binary(_op, *_a, *_b);
        _result->flags = _result->flags | flags;

        return true;
    }

    // an inline number above the boxed operand: the boxed one in its kind
    low = (_a->kind == D_MATH_VALUE_BOXED) ? _a : _b;

    if (!d_internal_math_value_unbox(low, &direction,
                                     (low == _a) ? _b->kind : _a->kind))
    {
        *_result       = d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
        _result->flags = flags;

        return true;
    }

    return false;
}

// d_internal_math_value_binary
//   _a _op _b for any two values: an error passes through, the first of
// two; a boxed operand goes to the operand higher in the tower; a boolean or
// none is a domain error; two exact numbers stay exact; otherwise the
// operation is done in the wider of the inexact kinds, an exact operand
// converted to it.
D_MATH_FN struct dj_math_value
d_internal_math_value_binary(
    enum dj_math_value_op _op,
    struct dj_math_value  _a,
    struct dj_math_value  _b
)
{
    const unsigned flags = _a.flags | _b.flags;

    struct dj_math_value result = d_math_value_none();

    // an error operand, the first of two
    if ( (_a.kind == D_MATH_VALUE_ERROR) ||
         (_b.kind == D_MATH_VALUE_ERROR) )
    {
        result       = (_a.kind == D_MATH_VALUE_ERROR) ? _a : _b;
        result.flags = flags;

        return result;
    }

    // a boxed operand: settled by its kind, or converted for an inline one
    if ( ( (_a.kind == D_MATH_VALUE_BOXED) ||
           (_b.kind == D_MATH_VALUE_BOXED) ) &&
         (d_internal_math_value_boxed_binary(&result, _op, &_a, &_b)) )
    {
        return result;
    }

    // a boolean or none
    if ( (!d_math_value_is_number(_a)) ||
         (!d_math_value_is_number(_b)) )
    {
        result       = d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
        result.flags = flags;

        return result;
    }

    // both exact
    if ( (d_math_value_is_exact(_a)) &&
         (d_math_value_is_exact(_b)) )
    {
        return d_internal_math_value_exact_op(_op, _a.exact, _b.exact, flags);
    }

    // the wider of the inexact kinds: an exact kind is below them all
    result = d_internal_math_value_blank(
        (_a.kind > _b.kind) ? _a.kind : _b.kind, flags);

    // the type's own arithmetic, the operands converted to it
    if (result.kind == D_MATH_VALUE_FLOAT)
    {
        result.real = (long double)d_internal_math_value_op_f(
            _op, d_internal_math_value_as_f(_a),
            d_internal_math_value_as_f(_b));
    }
    else if (result.kind == D_MATH_VALUE_DOUBLE)
    {
        result.real = (long double)d_internal_math_value_op_d(
            _op, d_internal_math_value_as_d(_a),
            d_internal_math_value_as_d(_b));
    }
    else
    {
        result.real = d_internal_math_value_op_ld(
            _op, d_internal_math_value_as_ld(_a),
            d_internal_math_value_as_ld(_b));
    }

    return result;
}

// 4.2    d_math_value_neg, d_math_value_abs
//------------------------------------------------------------------------------
// -_x and |_x|: exact for an exact number, where -D_MATH_IMAX_MIN alone does
// not fit and is the long double 2^(w-1), flagged inexact; for an inexact
// number, -0 is negated, and |-0| is +0. A boxed number's kind computes
// them, its flags added. An error passes through, and a boolean or none is
// a domain error.

// d_internal_math_value_unary
//   -_x where _negate, else |_x|.
D_MATH_FN
struct dj_math_value d_internal_math_value_unary(
    bool                 _negate,
    struct dj_math_value _x
)
{
    const bool          below  = (_x.exact.num < 0);
    struct dj_math_value result = _x;

    // an error passes through
    if (_x.kind == D_MATH_VALUE_ERROR)
    {
        return _x;
    }

    // boxed: its kind's
    if (_x.kind == D_MATH_VALUE_BOXED)
    {
        result       = _x.type->unary((_negate) ? D_MATH_VALUE_OP_NEG
                                                : D_MATH_VALUE_OP_ABS,
                                      _x);
        result.flags = result.flags | _x.flags;

        return result;
    }

    // a boolean or none
    if (!d_math_value_is_number(_x))
    {
        result       = d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
        result.flags = _x.flags;

        return result;
    }

    // exact: the magnitude, its sign turned or cleared
    if (d_math_value_is_exact(_x))
    {
        return d_internal_math_value_from_ratio(
            d_internal_rational_ratio_of(
                (_negate) ? (!below) : false,
                d_internal_rational_magnitude(_x.exact.num),
                (dj_math_umax)_x.exact.den),
            _x.flags);
    }

    // inexact: the sign; |x| of a zero, either sign, is +0
    if (_negate)
    {
        result.real = -_x.real;
    }
    else if (_x.real == 0.0L)
    {
        result.real = 0.0L;
    }
    else if (_x.real < 0.0L)
    {
        result.real = -_x.real;
    }

    return result;
}

D_MATH_FN
struct dj_math_value d_math_value_neg(
    struct dj_math_value _x
)
{
    return d_internal_math_value_unary(true, _x);
}

D_MATH_FN
struct dj_math_value d_math_value_abs(
    struct dj_math_value _x
)
{
    return d_internal_math_value_unary(false, _x);
}

// 4.3    d_math_value_add, d_math_value_sub, d_math_value_mul, d_math_value_div
//------------------------------------------------------------------------------

D_MATH_FN
struct dj_math_value d_math_value_add(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    return d_internal_math_value_binary(D_MATH_VALUE_OP_ADD, _a, _b);
}

D_MATH_FN
struct dj_math_value d_math_value_sub(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    return d_internal_math_value_binary(D_MATH_VALUE_OP_SUB, _a, _b);
}

D_MATH_FN
struct dj_math_value d_math_value_mul(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    return d_internal_math_value_binary(D_MATH_VALUE_OP_MUL, _a, _b);
}

D_MATH_FN
struct dj_math_value d_math_value_div(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    return d_internal_math_value_binary(D_MATH_VALUE_OP_DIV, _a, _b);
}


//==============================================================================
// 5.  COMPARISON
//==============================================================================


// 5.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_math_value_turn
//   the order seen from the other side.
D_MATH_FN enum dj_math_value_order
d_internal_math_value_turn(
    enum dj_math_value_order _order
)
{
    return (_order == D_MATH_VALUE_LESS)
               ? D_MATH_VALUE_GREATER
               : ( (_order == D_MATH_VALUE_GREATER) ? D_MATH_VALUE_LESS
                                                    : _order );
}

// d_internal_math_value_cmp_real
//   _a against _b, two values of floating-point types held as long doubles,
// so compared exactly.
D_MATH_FN enum dj_math_value_order
d_internal_math_value_cmp_real(
    long double _a,
    long double _b
)
{
    // below, above, the same; a NaN is none of them
    if (_a < _b)
    {
        return D_MATH_VALUE_LESS;
    }

    if (_a > _b)
    {
        return D_MATH_VALUE_GREATER;
    }

    return (_a == _b) ? D_MATH_VALUE_EQUAL : D_MATH_VALUE_UNORDERED;
}

// d_internal_math_value_cmp_exact
//   the exact number _r against _f, a float, double or long double's value,
// exactly. Rounding to the nearest long double is monotonic and leaves _f
// as it is, since every such value is a long double: so where |_r| rounds
// to a value other than |_f|, that value's order with |_f| is |_r|'s, and
// where it rounds to |_f| itself, the rounding's direction is the order,
// reversed. |_r| lies between 2^(1-w) and 2^(w-1), so its rounding is a
// normal long double.
D_MATH_FN enum dj_math_value_order
d_internal_math_value_cmp_exact(
    struct dj_rational _r,
    long double       _f
)
{
    const int r_sign = d_rational_sign(_r);
    const int f_sign = (_f > 0.0L) ? 1 : ((_f < 0.0L) ? -1 : 0);

    struct dj_internal_rational_wide    n       = { 0u, 0u };
    struct dj_internal_rational_wide    d       = { 0u, 0u };
    struct dj_internal_rational_rounded rounded = { { 0u, 0u }, 0, 0 };
    enum dj_math_value_order            order   = D_MATH_VALUE_EQUAL;
    long double                        near    = 0.0L;
    long double                        far     = 0.0L;

    // a NaN
    if (_f != _f)
    {
        return D_MATH_VALUE_UNORDERED;
    }

    // unlike signs, or two zeros: the signs decide
    if ( (r_sign != f_sign) ||
         (r_sign == 0) )
    {
        return (r_sign < f_sign) ? D_MATH_VALUE_LESS
                                 : ( (r_sign > f_sign) ? D_MATH_VALUE_GREATER
                                                       : D_MATH_VALUE_EQUAL );
    }

    far = (_f < 0.0L) ? -_f : _f;

    // an infinity is past every rational
    if (far > LDBL_MAX)
    {
        order = D_MATH_VALUE_LESS;
    }
    else
    {
        n.lo    = d_internal_rational_magnitude(_r.num);
        d.lo    = (dj_math_umax)_r.den;
        rounded = d_internal_rational_round(n, d, (unsigned)LDBL_MANT_DIG);
        near    = d_internal_rational_build_ld(false, rounded);

        // the rounded magnitude's order, or the rounding's direction
        if (near < far)
        {
            order = D_MATH_VALUE_LESS;
        }
        else if (near > far)
        {
            order = D_MATH_VALUE_GREATER;
        }
        else
        {
            order = (rounded.direction > 0)
                        ? D_MATH_VALUE_LESS
                        : ( (rounded.direction < 0) ? D_MATH_VALUE_GREATER
                                                    : D_MATH_VALUE_EQUAL );
        }
    }

    // the magnitudes' order, turned for two negatives
    return (r_sign > 0) ? order : d_internal_math_value_turn(order);
}

// d_internal_math_value_cmp_inline
//   _a against _b, neither boxed: numbers by value, exactly, across kinds, a
// NaN unordered; booleans false below true; none equal to none; errors, and
// unlike kinds, unordered.
D_MATH_FN enum dj_math_value_order
d_internal_math_value_cmp_inline(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    // two numbers
    if ( (d_math_value_is_number(_a)) &&
         (d_math_value_is_number(_b)) )
    {
        // exact and exact; exact and inexact, either way; inexact and inexact
        if ( (d_math_value_is_exact(_a)) &&
             (d_math_value_is_exact(_b)) )
        {
            return (enum dj_math_value_order)d_rational_cmp(_a.exact, _b.exact);
        }

        if (d_math_value_is_exact(_a))
        {
            return d_internal_math_value_cmp_exact(_a.exact, _b.real);
        }

        if (d_math_value_is_exact(_b))
        {
            return d_internal_math_value_turn(
                d_internal_math_value_cmp_exact(_b.exact, _a.real));
        }

        return d_internal_math_value_cmp_real(_a.real, _b.real);
    }

    // two booleans
    if ( (_a.kind == D_MATH_VALUE_BOOLEAN) &&
         (_b.kind == D_MATH_VALUE_BOOLEAN) )
    {
        return (_a.code < _b.code)
                   ? D_MATH_VALUE_LESS
                   : ( (_a.code > _b.code) ? D_MATH_VALUE_GREATER
                                           : D_MATH_VALUE_EQUAL );
    }

    // two nones
    if ( (_a.kind == D_MATH_VALUE_NONE) &&
         (_b.kind == D_MATH_VALUE_NONE) )
    {
        return D_MATH_VALUE_EQUAL;
    }

    return D_MATH_VALUE_UNORDERED;
}

// d_internal_math_value_cmp_boxed
//   _a against _b, one or both boxed. None, an error or a boolean has no
// order with a boxed value. A boxed kind at least as high as the other
// operand compares them. An inline number above the boxed operand compares
// with its value: in an exact kind, exactly; otherwise as the nearest long
// double, and where that equals the inline number, the rounding's direction
// gives the order: a value rounded up is below its rounding.
D_MATH_FN enum dj_math_value_order
d_internal_math_value_cmp_boxed(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    const struct dj_math_boxed_kind* handler   = NULL;
    struct dj_math_value*            low       = NULL;
    int                             direction = 0;
    enum dj_math_value_order         order     = D_MATH_VALUE_UNORDERED;

    // none, an error or a boolean
    if ( (d_internal_math_value_is_other(_a)) ||
         (d_internal_math_value_is_other(_b)) )
    {
        return D_MATH_VALUE_UNORDERED;
    }

    handler = d_internal_math_value_handler(_a, _b);

    // a boxed kind as high as the other operand: its own comparison
    if (handler != NULL)
    {
        return handler->cmp(_a, _b);
    }

    // an inline number above: the boxed operand's value, exact or nearest
    low = (_a.kind == D_MATH_VALUE_BOXED) ? &_a : &_b;

    if (!d_internal_math_value_unbox(
            low, &direction,
            (d_math_value_is_exact((low == &_a) ? _b : _a))
                ? D_MATH_VALUE_RATIONAL
                : D_MATH_VALUE_LONG_DOUBLE))
    {
        return D_MATH_VALUE_UNORDERED;
    }

    order = d_internal_math_value_cmp_inline(_a, _b);

    // equal to its rounding: below it where rounded up, seen from _a
    if ( (order == D_MATH_VALUE_EQUAL) &&
         (direction != 0) )
    {
        order = ((direction > 0) == (low == &_a)) ? D_MATH_VALUE_LESS
                                                  : D_MATH_VALUE_GREATER;
    }

    return order;
}

// 5.2    d_math_value_cmp, d_math_value_eq
//------------------------------------------------------------------------------
// cmp is how _a stands to _b: numbers by value, exactly, across kinds, a NaN
// unordered; booleans false below true; none equal to none; errors, and
// unlike kinds, unordered; a boxed value as above. eq is cmp's EQUAL.

D_MATH_FN enum dj_math_value_order
d_math_value_cmp(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    // a boxed operand
    if ( (_a.kind == D_MATH_VALUE_BOXED) ||
         (_b.kind == D_MATH_VALUE_BOXED) )
    {
        return d_internal_math_value_cmp_boxed(_a, _b);
    }

    return d_internal_math_value_cmp_inline(_a, _b);
}

D_MATH_FN
bool d_math_value_eq(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    return (d_math_value_cmp(_a, _b) == D_MATH_VALUE_EQUAL);
}


#endif  // DJINTERP_C_MATH_VALUE_H
