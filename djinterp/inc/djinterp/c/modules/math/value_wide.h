/*******************************************************************************
* djinterp [c/math]                                                 value_wide.h
*
* The wide integer as a boxed kind of the generic value: wide.h's signed
* integers of 256 bits, registered with value.h through a table of their own,
* and value.h unchanged (roadmap 1d, its third part).
*   A value of the kind is D_MATH_VALUE_BOXED, its type the kind's table and
* its box a count of its holders and the integer, which never changes. The
* kind's rank, D_MATH_VALUE_RANK_WIDE, is 210: past the rational's and below
* the float's, where value.h places an exact kind that holds more than
* dj_math_imax, and at the low end of that range, so that bignum's kinds,
* which will hold every wide integer, can stand above it.
*
*   ITS ARITHMETIC. The kind handles every operation whose other operand is no
* higher: a wide integer, an inline integer or rational, or a lower boxed
* number that is exact, read through its own table as a number of the family.
* The result is exact. An integer is a wide integer, however small, so that
* arithmetic begun with one stays exact past dj_math_imax; a result that is no
* integer, a quotient or anything with a rational, is an inline rational,
* where it fits the family. A result past both -- an integer of more than 256
* bits, or a ratio whose terms do not fit dj_math_imax -- is the long double
* nearest it, flagged D_MATH_VALUE_FLAG_INEXACT (roadmap gate G2, until
* bignum). That long double is rounded once, from the exact ratio, whose
* terms run to twice a wide integer's width: wide.h's division takes the
* quotient to two bits past the long double's precision, a remainder that is
* not 0 is set into its lowest bit, and wide.h's rounding does the rest. An
* exact division by 0 is a pole, or invalid for 0 / 0, as in value.h, and
* comparison is exact.
*   Above the kind, a float, double or long double meets a wide integer as
* value.h has every inline number meet a boxed one: converted through the
* kind's table to the nearest value of its type, ties to even, the rounding's
* direction keeping a comparison exact. A lower boxed operand the kind cannot
* read exactly -- one that is no number, an inexact one, or an exact one past
* the family -- gives a domain error, and no order, as value.h gives where an
* inline number cannot read a lower boxed operand.
*
*   ITS STORAGE. Every result of the kind is a box of its own, allocated
* through D_CFG_MATH_VALUE_WIDE_MALLOC and given back through
* D_CFG_MATH_VALUE_WIDE_FREE (cfg_value_wide.h: malloc and free unless set
* otherwise); where no storage is given, the result is
* D_MATH_VALUE_ERROR_MEMORY. A box's holders are counted by value.h's
* d_math_boxed_hold and d_math_boxed_unhold, atomically wherever the
* compiler can (cfg_value.h, S134), so that a value may be copied and given
* back in several threads at once; where D_MATH_BOXED_HOLDERS_ATOMIC is 0, a
* value shared between threads must be retained and released under a lock
* of the caller's.
*   The table is d_math_value_wide_kind's. In C every translation unit that
* includes this header has a copy of its own, at its own address; the kind
* knows its values by their rank, as value.h's kinds do, so values made under
* any copy mix, and a result is made under the table of the operand of the
* kind that handled it.
*
*   ONE TEXT, BOTH LANGUAGES. The exact arithmetic, the comparison and the
* rounding are pure arithmetic (D_MATH_FN), constant expressions from C++14.
* The table's entries and the constructors allocate, or read a box, and run
* at run time: the constructors D_MATH_FN_RT, and the entries, which only
* calls through the table reach, D_MATH_FN_ENTRY, which no compiler is told
* it must inline.
*
*
* path:      /inc/djinterp/c/modules/math/value_wide.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.11.07
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LAYOUT
    ------
    1.  The kind
         1.  D_MATH_VALUE_RANK_WIDE
         2.  dj_internal_math_value_wide_box
    2.  Exact terms and ratios
         1.  dj_internal_math_value_wide_term
         2.  D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS
         3.  D_INTERNAL_MATH_VALUE_WIDE_SHIFTED_LIMBS
         4.  dj_internal_math_value_wide_ratio
2.  EXACT ARITHMETIC
    ----------------
    1.  Products of terms
    2.  Sums, products and quotients
    3.  Order
    4.  The nearest long double
3.  THE KIND
    --------
    1.  d_math_value_is_wide
    2.  Boxes
    3.  Results and operands
    4.  The entries
    5.  d_math_value_wide_kind
4.  CONSTRUCTION AND CONVERSION
    ---------------------------
    1.  d_math_value_from_wide
    2.  d_math_value_to_wide
*/

#ifndef DJINTERP_C_MATH_VALUE_WIDE_H
#define DJINTERP_C_MATH_VALUE_WIDE_H 1

// std
#include <float.h>           // LDBL_MANT_DIG, LDBL_MAX_EXP, LDBL_MIN_EXP
#include <stddef.h>          // size_t, NULL
#include <stdlib.h>          // malloc, free: the allocation hooks' defaults
// djinterp
#include "./math_common.h"   // D_MATH_FN, D_MATH_FN_RT,
                             // D_MATH_FN_ENTRY, dj_math_imax, dj_math_umax
#include "./rational.h"      // dj_rational, d_internal_rational_pack,
                             // _rounded, _build_ld
#include "./value.h"         // dj_math_value, dj_math_boxed_kind, the ranks
#include "./wide.h"          // dj_math_wide and its limb kernels
#include "../../../config/c/modules/math/cfg_value_wide.h"  // D_CFG_MATH_VALUE_WIDE_*


//==============================================================================
// 1.  LAYOUT
//==============================================================================


// 1.1    The kind
//------------------------------------------------------------------------------
// 1.1.1
// D_MATH_VALUE_RANK_WIDE
//   constant: the kind's place in value.h's tower, and its identity: past
// the rational's rank, 200, and below the float's, 300.
#define D_MATH_VALUE_RANK_WIDE  210

// 1.1.2
// dj_internal_math_value_wide_box
//   struct: a box of the kind: the integer and the values that hold it. The
// integer never changes, so holders may share it.
struct dj_internal_math_value_wide_box
{
    size_t              holders;  // the values that hold the box
    struct dj_math_wide value;    // the integer
};

// 1.2    Exact terms and ratios
//------------------------------------------------------------------------------
// 1.2.1
// dj_internal_math_value_wide_term
//   struct: an operand, read exactly: num / den, the sign on num. A wide
// integer and an inline integer have den 1; an inline rational, its own
// denominator. num and den have no common factor.
struct dj_internal_math_value_wide_term
{
    struct dj_math_wide num;  // the numerator, with the sign
    dj_math_umax        den;  // the denominator, positive
};

// 1.2.2
// D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS
//   constant: the limbs of an exact result's terms: twice a wide integer's,
// which holds the product of two wide integers, and every term below.
#define D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS  (2u * D_MATH_WIDE_LIMBS)

// 1.2.3
// D_INTERNAL_MATH_VALUE_WIDE_SHIFTED_LIMBS
//   constant: the limbs of a ratio's numerator moved up for its rounding. It
// is moved until it has P + 2 bits more than the denominator, P the long
// double's precision: at most 512 + P + 2 bits, and P is at most two words
// (rational.h), so 2 words and 2 limbs more than a term hold it, the limb its
// last bits may carry into included.
#define D_INTERNAL_MATH_VALUE_WIDE_SHIFTED_LIMBS                               \
    (D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS +                                  \
     (2u * D_INTERNAL_MATH_WIDE_WORD_LIMBS) + 2u)

// 1.2.4
// dj_internal_math_value_wide_ratio
//   struct: an exact result: num / den, both magnitudes of
// D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS limbs, the low first, with no
// common factor and den positive, and a sign. 0 is 0 / 1, never negative.
struct dj_internal_math_value_wide_ratio
{
    bool              negative;                                    // below 0
    dj_math_wide_limb num[D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS];  // |n|
    dj_math_wide_limb den[D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS];  // d
};

// every ratio's terms are below 2^512, and its value between 2^-512 and
// 2^512: a long double's normal range must hold it, and every rounding of it
D_STATIC_ASSERT( ( (LDBL_MAX_EXP > (int)(2u * D_MATH_WIDE_BITS)) &&
                   ((LDBL_MIN_EXP - 1) <= -(int)(2u * D_MATH_WIDE_BITS)) ),
                 "value_wide.h: a long double's normal range must hold "
                 "every ratio of 512-bit terms" );


//==============================================================================
// 2.  EXACT ARITHMETIC
//==============================================================================
// An operation of the kind's, done exactly on its operands' terms, gives a
// ratio. One of the operands is always the kind's own, so an integer; the
// other may be an inline rational, whose denominator is a word.


// 2.1    Products of terms
//------------------------------------------------------------------------------

// d_internal_math_value_wide_product
//   |_a| * |_b|, into _out's D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS limbs:
// the limbs above the two operands' lengths are 0, and the product of those
// lengths is wide.h's.
D_MATH_FN void
d_internal_math_value_wide_product(
    dj_math_wide_limb*  _out,
    struct dj_math_wide _a,
    struct dj_math_wide _b
)
{
    const size_t na = d_internal_math_wide_length(_a.limb, D_MATH_WIDE_LIMBS);
    const size_t nb = d_internal_math_wide_length(_b.limb, D_MATH_WIDE_LIMBS);

    size_t i = 0u;

    // the limbs past the product
    for (i = na + nb; i < D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS; ++i)
    {
        _out[i] = 0u;
    }

    d_internal_math_wide_mul(_out, _a.limb, na, _b.limb, nb);

    return;
}

// d_internal_math_value_wide_zero
//   0, as a ratio: 0 / 1.
D_MATH_FN void
d_internal_math_value_wide_zero(
    struct dj_internal_math_value_wide_ratio* _out
)
{
    size_t i = 0u;

    for (i = 0u; i < D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS; ++i)
    {
        _out->num[i] = 0u;
        _out->den[i] = 0u;
    }

    _out->den[0]   = 1u;
    _out->negative = false;

    return;
}

// d_internal_math_value_wide_divided
//   |_x| / _g, _g a divisor of it: the quotient, exact.
D_MATH_FN struct dj_math_wide
d_internal_math_value_wide_divided(
    struct dj_math_wide _x,
    struct dj_math_wide _g
)
{
    struct dj_math_wide result = { { 0u }, false };

    (void)d_math_wide_divmod(&result, NULL, d_math_wide_abs(_x), _g);

    return result;
}

// 2.2    Sums, products and quotients
//------------------------------------------------------------------------------

// d_internal_math_value_wide_sum
//   _a + _b, or _a - _b where _subtract, exactly, into *_out. One term is an
// integer, so the sum over the other's denominator needs no reduction: with
// _b's denominator 1, a factor of _a's divides n_a + n_b d_a only where it
// divides n_a, which it does not. Each cross term is below 2^(256 + w), and
// their sum fits a ratio's limbs.
D_MATH_FN void
d_internal_math_value_wide_sum(
    struct dj_internal_math_value_wide_ratio* _out,
    struct dj_internal_math_value_wide_term   _a,
    struct dj_internal_math_value_wide_term   _b,
    bool                                      _subtract
)
{
    const bool b_negative = (_subtract) ? (d_math_wide_sign(_b.num) > 0)
                                        : _b.num.negative;

    dj_math_wide_limb p[D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS] = { 0u };
    dj_math_wide_limb q[D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS] = { 0u };

    // n_a d_b and n_b d_a, over d_a d_b
    d_internal_math_value_wide_product(p, _a.num,
                                       d_math_wide_from_umax(_b.den));
    d_internal_math_value_wide_product(q, _b.num,
                                       d_math_wide_from_umax(_a.den));
    d_internal_math_value_wide_product(_out->den, d_math_wide_from_umax(_a.den),
                                       d_math_wide_from_umax(_b.den));

    // like signs add; unlike signs take the smaller from the larger, whose
    // sign the result takes
    if (_a.num.negative == b_negative)
    {
        (void)d_internal_math_wide_add(_out->num, p, q,
                                       D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS);
        _out->negative = _a.num.negative;
    }
    else if (d_internal_math_wide_cmp(p, q,
                                      D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS)
                 >= 0)
    {
        (void)d_internal_math_wide_sub(_out->num, p, q,
                                       D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS);
        _out->negative = _a.num.negative;
    }
    else
    {
        (void)d_internal_math_wide_sub(_out->num, q, p,
                                       D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS);
        _out->negative = b_negative;
    }

    // 0 is never negative
    _out->negative =
        ( (_out->negative) &&
          (d_internal_math_wide_length(_out->num,
                                       D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS)
               != 0u) );

    return;
}

// d_internal_math_value_wide_times
//   _a * _b, exactly, into *_out. Knuth's form: with g1 = gcd(n_a, d_b) and
// g2 = gcd(n_b, d_a), the product is ((n_a / g1)(n_b / g2)) / ((d_a / g2)
// (d_b / g1)), reduced already; its numerator is below 2^512.
D_MATH_FN void
d_internal_math_value_wide_times(
    struct dj_internal_math_value_wide_ratio* _out,
    struct dj_internal_math_value_wide_term   _a,
    struct dj_internal_math_value_wide_term   _b
)
{
    const struct dj_math_wide a_den = d_math_wide_from_umax(_a.den);
    const struct dj_math_wide b_den = d_math_wide_from_umax(_b.den);

    struct dj_math_wide g1 = { { 0u }, false };
    struct dj_math_wide g2 = { { 0u }, false };

    // a factor of 0: 0
    if ( (d_math_wide_sign(_a.num) == 0) ||
         (d_math_wide_sign(_b.num) == 0) )
    {
        d_internal_math_value_wide_zero(_out);

        return;
    }

    g1 = d_math_wide_gcd(_a.num, b_den);
    g2 = d_math_wide_gcd(_b.num, a_den);

    d_internal_math_value_wide_product(
        _out->num, d_internal_math_value_wide_divided(_a.num, g1),
        d_internal_math_value_wide_divided(_b.num, g2));
    d_internal_math_value_wide_product(
        _out->den, d_internal_math_value_wide_divided(a_den, g2),
        d_internal_math_value_wide_divided(b_den, g1));
    _out->negative = (_a.num.negative != _b.num.negative);

    return;
}

// d_internal_math_value_wide_over
//   _a / _b, exactly, into *_out, and D_MATH_WIDE_OK; D_MATH_WIDE_POLE for a
// nonzero _a over 0, and D_MATH_WIDE_INVALID for 0 over 0, *_out as it was.
// Knuth's form, one denominator being 1: with g = gcd(n_a, n_b), the
// quotient is ((n_a / g) d_b) / (d_a (n_b / g)), reduced already, since d_a
// and d_b, one of them 1, have no common factor.
D_MATH_FN enum dj_math_wide_status
d_internal_math_value_wide_over(
    struct dj_internal_math_value_wide_ratio* _out,
    struct dj_internal_math_value_wide_term   _a,
    struct dj_internal_math_value_wide_term   _b
)
{
    struct dj_math_wide g = { { 0u }, false };

    // over 0: a pole, or invalid for 0 / 0
    if (d_math_wide_sign(_b.num) == 0)
    {
        return (d_math_wide_sign(_a.num) == 0) ? D_MATH_WIDE_INVALID
                                               : D_MATH_WIDE_POLE;
    }

    // 0 over anything else: 0
    if (d_math_wide_sign(_a.num) == 0)
    {
        d_internal_math_value_wide_zero(_out);

        return D_MATH_WIDE_OK;
    }

    g = d_math_wide_gcd(_a.num, _b.num);

    d_internal_math_value_wide_product(
        _out->num, d_internal_math_value_wide_divided(_a.num, g),
        d_math_wide_from_umax(_b.den));
    d_internal_math_value_wide_product(
        _out->den, d_math_wide_from_umax(_a.den),
        d_internal_math_value_wide_divided(_b.num, g));
    _out->negative = (_a.num.negative != _b.num.negative);

    return D_MATH_WIDE_OK;
}

// d_internal_math_value_wide_exact
//   _a _op _b, exactly, into *_out, for the four operations, one of _a and
// _b an integer: D_MATH_WIDE_OK, or for a quotient over 0
// D_MATH_WIDE_POLE, or D_MATH_WIDE_INVALID for 0 / 0, *_out as it was.
D_MATH_FN enum dj_math_wide_status
d_internal_math_value_wide_exact(
    struct dj_internal_math_value_wide_ratio* _out,
    enum dj_math_value_op                     _op,
    struct dj_internal_math_value_wide_term   _a,
    struct dj_internal_math_value_wide_term   _b
)
{
    // a sum or a difference; a product; a quotient
    if ( (_op == D_MATH_VALUE_OP_ADD) ||
         (_op == D_MATH_VALUE_OP_SUB) )
    {
        d_internal_math_value_wide_sum(_out, _a, _b,
                                       (_op == D_MATH_VALUE_OP_SUB));

        return D_MATH_WIDE_OK;
    }

    if (_op == D_MATH_VALUE_OP_MUL)
    {
        d_internal_math_value_wide_times(_out, _a, _b);

        return D_MATH_WIDE_OK;
    }

    return d_internal_math_value_wide_over(_out, _a, _b);
}

// 2.3    Order
//------------------------------------------------------------------------------

// d_internal_math_value_wide_order
//   -1, 0 or 1 as _a is below, equal to or above _b, exactly: the signs,
// and for like signs the cross products, n_a d_b against n_b d_a, the
// denominators being positive.
D_MATH_FN int
d_internal_math_value_wide_order(
    struct dj_internal_math_value_wide_term _a,
    struct dj_internal_math_value_wide_term _b
)
{
    const int a_sign = d_math_wide_sign(_a.num);
    const int b_sign = d_math_wide_sign(_b.num);

    dj_math_wide_limb p[D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS] = { 0u };
    dj_math_wide_limb q[D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS] = { 0u };
    int               order = 0;

    // unlike signs, or two zeros: the signs decide
    if ( (a_sign != b_sign) ||
         (a_sign == 0) )
    {
        return (a_sign < b_sign) ? -1 : ((a_sign > b_sign) ? 1 : 0);
    }

    d_internal_math_value_wide_product(p, _a.num,
                                       d_math_wide_from_umax(_b.den));
    d_internal_math_value_wide_product(q, _b.num,
                                       d_math_wide_from_umax(_a.den));
    order = d_internal_math_wide_cmp(p, q,
                                     D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS);

    // the magnitudes' order, turned for two negatives
    return (a_sign > 0) ? order : -order;
}

// 2.4    The nearest long double
//------------------------------------------------------------------------------
// A ratio past the family is kept as the long double nearest it (G2), rounded
// once. Its quotient, taken to two bits past the precision P, with a
// remainder that is not 0 set into its lowest bit, rounds to P bits as the
// ratio does: the bit below the mantissa is one of the quotient's own, so
// the lowest bit lies below it, and is set where anything at all lies there.

// d_internal_math_value_wide_round_ratio
//   _num / _den, of _nn and _nd limbs, at most
// D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS each and neither 0, rounded to
// _precision bits, 1 to 2w, to nearest, ties to even, as rational.h's
// rounding gives it: a mantissa of P bits, the place of its lowest bit, and
// the direction. _num is moved up by s bits, until it has P + 2 bits more
// than _den, so that the quotient has P + 2 bits or P + 3; the quotient's
// rounding is then the ratio's, its place s lower.
D_MATH_FN struct dj_internal_rational_rounded
d_internal_math_value_wide_round_ratio(
    const dj_math_wide_limb* _num,
    size_t                   _nn,
    const dj_math_wide_limb* _den,
    size_t                   _nd,
    unsigned                 _precision
)
{
    const size_t   nn     = d_internal_math_wide_length(_num, _nn);
    const size_t   nd     = d_internal_math_wide_length(_den, _nd);
    const size_t   n_bits = d_internal_math_wide_bits(_num, nn);
    const size_t   d_bits = d_internal_math_wide_bits(_den, nd);
    const size_t   need   = (size_t)_precision + 2u + d_bits;
    const size_t   shift  = (need > n_bits) ? (need - n_bits) : 0u;
    const size_t   whole  = shift / (size_t)D_MATH_WIDE_LIMB_BITS;
    const unsigned part   =
        (unsigned)(shift % (size_t)D_MATH_WIDE_LIMB_BITS);

    struct dj_internal_rational_rounded result = { { 0u, 0u }, 0, 0 };
    dj_math_wide_limb u[D_INTERNAL_MATH_VALUE_WIDE_SHIFTED_LIMBS] = { 0u };
    dj_math_wide_limb q[D_INTERNAL_MATH_VALUE_WIDE_SHIFTED_LIMBS] = { 0u };
    dj_math_wide_limb r[D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS]   = { 0u };
    dj_math_wide_limb work[D_INTERNAL_MATH_VALUE_WIDE_SHIFTED_LIMBS +
                           D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS +
                           1u] = { 0u };
    size_t            nu = 0u;

    // the numerator moved up: whole limbs, then the bits left over, which
    // may carry into a limb more
    u[whole + nn] = d_internal_math_wide_shl(u + whole, _num, nn, part);
    nu            = d_internal_math_wide_length(u, whole + nn + 1u);

    // the quotient, a remainder that is not 0 set into its lowest bit
    d_internal_math_wide_divmod(q, r, u, nu, _den, nd, work);

    if (d_internal_math_wide_length(r, nd) != 0u)
    {
        q[0] = (dj_math_wide_limb)(q[0] | 1u);
    }

    result          = d_internal_math_wide_round(q, nu - nd + 1u, _precision);
    result.exponent = result.exponent - (int)shift;

    return result;
}

// d_internal_math_value_wide_nearest
//   the long double nearest *_r, which is not 0, ties to even.
D_MATH_FN long double
d_internal_math_value_wide_nearest(
    const struct dj_internal_math_value_wide_ratio* _r
)
{
    return d_internal_rational_build_ld(
        _r->negative,
        d_internal_math_value_wide_round_ratio(
            _r->num, D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS,
            _r->den, D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS,
            (unsigned)LDBL_MANT_DIG));
}


//==============================================================================
// 3.  THE KIND
//==============================================================================


// 3.1    d_math_value_is_wide
//------------------------------------------------------------------------------
// whether _x is a value of the kind: boxed, whole, and of its rank, under any
// copy of its table.

D_MATH_FN bool
d_math_value_is_wide(
    struct dj_math_value _x
)
{
    return ( (_x.kind == D_MATH_VALUE_BOXED) &&
             (_x.type != NULL) &&
             (_x.box != NULL) &&
             (_x.type->rank == D_MATH_VALUE_RANK_WIDE) );
}

// 3.2    Boxes
//------------------------------------------------------------------------------

// d_internal_math_value_wide_held
//   the integer a value of the kind holds.
D_MATH_FN_RT struct dj_math_wide
d_internal_math_value_wide_held(
    struct dj_math_value _x
)
{
    return ((const struct dj_internal_math_value_wide_box*)_x.box)->value;
}

// d_internal_math_value_wide_box_of
//   a value of the kind, under the table _type, holding _x in a box of its
// own with one holder: the caller's, to release. D_MATH_VALUE_ERROR_MEMORY
// where no storage is given.
D_MATH_FN_RT struct dj_math_value
d_internal_math_value_wide_box_of(
    const struct dj_math_boxed_kind* _type,
    struct dj_math_wide              _x
)
{
    struct dj_internal_math_value_wide_box* box =
        (struct dj_internal_math_value_wide_box*)D_CFG_MATH_VALUE_WIDE_MALLOC(
            sizeof(struct dj_internal_math_value_wide_box));
    struct dj_math_value result =
        d_internal_math_value_blank(D_MATH_VALUE_BOXED, 0u);

    // no storage
    if (box == NULL)
    {
        return d_math_value_from_error(D_MATH_VALUE_ERROR_MEMORY);
    }

    box->holders = 1u;
    box->value   = _x;
    result.type  = _type;
    result.box   = box;

    return result;
}

// d_internal_math_value_wide_retain, d_internal_math_value_wide_release
//   one holder more of _box, and one fewer; the last release gives the box
// back.
D_MATH_FN_ENTRY void
d_internal_math_value_wide_retain(
    void* _box
)
{
    struct dj_internal_math_value_wide_box* box =
        (struct dj_internal_math_value_wide_box*)_box;

    d_math_boxed_hold(&box->holders);

    return;
}

D_MATH_FN_ENTRY void
d_internal_math_value_wide_release(
    void* _box
)
{
    struct dj_internal_math_value_wide_box* box =
        (struct dj_internal_math_value_wide_box*)_box;

    // the last holder: the box given back
    if (d_math_boxed_unhold(&box->holders))
    {
        D_CFG_MATH_VALUE_WIDE_FREE(box);
    }

    return;
}

// 3.3    Results and operands
//------------------------------------------------------------------------------

// d_internal_math_value_wide_result
//   the value of the exact result *_r, under the table _type: an integer, in
// a box of the kind's where it is within 256 bits; a rational that is no
// integer, inline where its terms fit the family; and otherwise the long
// double nearest it, flagged inexact (G2). An integer past the first test
// has more than 256 bits, so its terms do not fit the family either.
D_MATH_FN_RT struct dj_math_value
d_internal_math_value_wide_result(
    const struct dj_math_boxed_kind*                _type,
    const struct dj_internal_math_value_wide_ratio* _r
)
{
    const size_t nn = d_internal_math_wide_length(
        _r->num, D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS);
    const size_t nd = d_internal_math_wide_length(
        _r->den, D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS);

    struct dj_math_wide  held   = { { 0u }, false };
    struct dj_rational   packed = { 0, 1 };
    struct dj_math_value result =
        d_internal_math_value_blank(D_MATH_VALUE_RATIONAL, 0u);
    size_t               i      = 0u;

    // an integer within 256 bits: the kind's own
    if ( (nd == 1u) &&
         (_r->den[0] == 1u) &&
         (nn <= D_MATH_WIDE_LIMBS) )
    {
        for (i = 0u; i < D_MATH_WIDE_LIMBS; ++i)
        {
            held.limb[i] = _r->num[i];
        }

        held.negative = _r->negative;

        return d_internal_math_value_wide_box_of(_type, held);
    }

    // a rational whose terms fit the family: inline
    if ( (nn <= D_INTERNAL_MATH_WIDE_WORD_LIMBS) &&
         (nd <= D_INTERNAL_MATH_WIDE_WORD_LIMBS) &&
         (d_internal_rational_pack(
              &packed, _r->negative,
              (dj_math_umax)_r->num[0] |
                  ((dj_math_umax)_r->num[1] << D_MATH_WIDE_LIMB_BITS),
              (dj_math_umax)_r->den[0] |
                  ((dj_math_umax)_r->den[1] << D_MATH_WIDE_LIMB_BITS))
              == D_RATIONAL_OK) )
    {
        result.exact = packed;

        return result;
    }

    // past both: the nearest long double, flagged inexact
    result      = d_internal_math_value_blank(D_MATH_VALUE_LONG_DOUBLE,
                                              D_MATH_VALUE_FLAG_INEXACT);
    result.real = d_internal_math_value_wide_nearest(_r);

    return result;
}

// d_internal_math_value_wide_read
//   _x as a term, into *_out, and true: a value of the kind; an inline
// integer or rational; or a boxed number that is exact and that its own
// table converts to one of those. False, *_out as it was, for anything else:
// no number, an inexact one, or an exact one past the family.
D_MATH_FN_RT bool
d_internal_math_value_wide_read(
    struct dj_internal_math_value_wide_term* _out,
    struct dj_math_value                     _x
)
{
    struct dj_math_value exact     = _x;
    int                  direction = 0;

    // the kind's own
    if (d_math_value_is_wide(_x))
    {
        _out->num = d_internal_math_value_wide_held(_x);
        _out->den = 1u;

        return true;
    }

    // another boxed kind: exact, and of the family through its table
    if ( (_x.kind == D_MATH_VALUE_BOXED) &&
         ( (!d_math_value_is_exact(_x)) ||
           (!_x.type->convert(&exact, &direction, _x,
                              D_MATH_VALUE_RATIONAL)) ) )
    {
        return false;
    }

    // an integer or a rational of the family, or nothing the kind reads
    if ( (exact.kind != D_MATH_VALUE_INTEGER) &&
         (exact.kind != D_MATH_VALUE_RATIONAL) )
    {
        return false;
    }

    _out->num = d_math_wide_from_imax(exact.exact.num);
    _out->den = (dj_math_umax)exact.exact.den;

    return true;
}

// 3.4    The entries
//------------------------------------------------------------------------------
// The table's entries, as value.h asks for them (value.h 1.4.2). Each makes
// a value of the kind under the table of the operand of the kind it was
// given; value.h adds the operands' flags to every result.

// d_internal_math_value_wide_unary
//   -_x for D_MATH_VALUE_OP_NEG and |_x| for D_MATH_VALUE_OP_ABS, which every
// wide integer has, the range being symmetric: a box of its own. A domain
// error for any other operation, or an _x not of the kind.
D_MATH_FN_ENTRY struct dj_math_value
d_internal_math_value_wide_unary(
    enum dj_math_value_op _op,
    struct dj_math_value  _x
)
{
    // not of the kind, or no operation of the two
    if ( (!d_math_value_is_wide(_x)) ||
         ( (_op != D_MATH_VALUE_OP_NEG) &&
           (_op != D_MATH_VALUE_OP_ABS) ) )
    {
        return d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
    }

    return d_internal_math_value_wide_box_of(
        _x.type, (_op == D_MATH_VALUE_OP_NEG)
                     ? d_math_wide_neg(d_internal_math_value_wide_held(_x))
                     : d_math_wide_abs(d_internal_math_value_wide_held(_x)));
}

// d_internal_math_value_wide_binary
//   _a _op _b, for the four operations, one operand of the kind: exactly, as
// d_internal_math_value_wide_result gives it; a pole over exact 0, or
// invalid for 0 / 0. A domain error for any other operation, two operands
// neither of the kind, or an operand the kind cannot read exactly.
D_MATH_FN_ENTRY struct dj_math_value
d_internal_math_value_wide_binary(
    enum dj_math_value_op _op,
    struct dj_math_value  _a,
    struct dj_math_value  _b
)
{
    const struct dj_math_boxed_kind* type =
        (d_math_value_is_wide(_a)) ? _a.type : _b.type;

    struct dj_internal_math_value_wide_term  a = { { { 0u }, false }, 1u };
    struct dj_internal_math_value_wide_term  b = { { { 0u }, false }, 1u };
    struct dj_internal_math_value_wide_ratio r = { false, { 0u }, { 0u } };
    enum dj_math_wide_status                 status = D_MATH_WIDE_OK;

    // one of the four, an operand of the kind, and both read exactly
    if ( ( (_op != D_MATH_VALUE_OP_ADD) &&
           (_op != D_MATH_VALUE_OP_SUB) &&
           (_op != D_MATH_VALUE_OP_MUL) &&
           (_op != D_MATH_VALUE_OP_DIV) ) ||
         ( (!d_math_value_is_wide(_a)) &&
           (!d_math_value_is_wide(_b)) ) ||
         (!d_internal_math_value_wide_read(&a, _a)) ||
         (!d_internal_math_value_wide_read(&b, _b)) )
    {
        return d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
    }

    status = d_internal_math_value_wide_exact(&r, _op, a, b);

    // over exact 0: a pole, or invalid for 0 / 0
    if (status != D_MATH_WIDE_OK)
    {
        return d_math_value_from_error((status == D_MATH_WIDE_INVALID)
                                           ? D_MATH_VALUE_ERROR_INVALID
                                           : D_MATH_VALUE_ERROR_POLE);
    }

    return d_internal_math_value_wide_result(type, &r);
}

// d_internal_math_value_wide_cmp
//   how _a stands to _b, one of them of the kind, exactly. No order where
// neither is of the kind, or one cannot be read exactly.
D_MATH_FN_ENTRY enum dj_math_value_order
d_internal_math_value_wide_cmp(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    struct dj_internal_math_value_wide_term a = { { { 0u }, false }, 1u };
    struct dj_internal_math_value_wide_term b = { { { 0u }, false }, 1u };
    int                                     order = 0;

    // an operand of the kind, and both read exactly
    if ( ( (!d_math_value_is_wide(_a)) &&
           (!d_math_value_is_wide(_b)) ) ||
         (!d_internal_math_value_wide_read(&a, _a)) ||
         (!d_internal_math_value_wide_read(&b, _b)) )
    {
        return D_MATH_VALUE_UNORDERED;
    }

    order = d_internal_math_value_wide_order(a, b);

    return (order < 0) ? D_MATH_VALUE_LESS
                       : ( (order > 0) ? D_MATH_VALUE_GREATER
                                       : D_MATH_VALUE_EQUAL );
}

// d_internal_math_value_wide_convert
//   _x, of the kind, as an inline number of kind _to, into *_out, and true,
// *_direction how it stands to _x: to an integer or a rational, the integer,
// where it fits dj_math_imax, with *_direction 0; to a float, double or long
// double, the nearest value of the type, ties to even, a float past the
// largest an infinity, and the rounding's direction, -1, 0 or 1. False, and
// *_out and *_direction as they were, otherwise.
D_MATH_FN_ENTRY bool
d_internal_math_value_wide_convert(
    struct dj_math_value*   _out,
    int*                    _direction,
    struct dj_math_value    _x,
    enum dj_math_value_kind _to
)
{
    struct dj_math_wide x   = { { 0u }, false };
    dj_math_imax        n   = 0;
    int                 way = 0;

    // not of the kind
    if (!d_math_value_is_wide(_x))
    {
        return false;
    }

    x = d_internal_math_value_wide_held(_x);

    // an exact kind, where the integer fits; then each floating-point type
    if ( (_to == D_MATH_VALUE_INTEGER) ||
         (_to == D_MATH_VALUE_RATIONAL) )
    {
        if (d_math_wide_to_imax(&n, x) != D_MATH_WIDE_OK)
        {
            return false;
        }

        *_out = d_math_value_from_int(n);
    }
    else if (_to == D_MATH_VALUE_FLOAT)
    {
        const float value = d_math_wide_to_f(x, &way);

        *_out = d_math_value_from_f(value);
    }
    else if (_to == D_MATH_VALUE_DOUBLE)
    {
        const double value = d_math_wide_to_d(x, &way);

        *_out = d_math_value_from_d(value);
    }
    else if (_to == D_MATH_VALUE_LONG_DOUBLE)
    {
        *_out = d_math_value_from_ld(d_math_wide_to_ld(x, &way));
    }
    else
    {
        return false;
    }

    *_direction = way;

    return true;
}

// d_internal_math_value_wide_text
//   _x's decimal text, as wide.h writes it, into _buffer: at most _size
// characters, the NUL among them, and none where _size is 0. Returns the
// whole text's length, as snprintf does; an _x not of the kind has none.
D_MATH_FN_ENTRY size_t
d_internal_math_value_wide_text(
    char*                _buffer,
    size_t               _size,
    struct dj_math_value _x
)
{
    // not of the kind: no text
    if (!d_math_value_is_wide(_x))
    {
        if (_size > 0u)
        {
            _buffer[0] = '\0';
        }

        return 0u;
    }

    return d_math_wide_to_text(_buffer, _size,
                               d_internal_math_value_wide_held(_x));
}

// 3.5    d_math_value_wide_kind
//------------------------------------------------------------------------------
// the kind's table: its name, "wide integer", its rank, its traits, an exact
// number, and its entries, with no exact entry, as its traits say every
// value is exact. In C, each translation unit's own copy.

D_MATH_FN_RT const struct dj_math_boxed_kind*
d_math_value_wide_kind(void)
{
    static const struct dj_math_boxed_kind table = {
        "wide integer", D_MATH_VALUE_RANK_WIDE,
        D_MATH_BOXED_NUMBER | D_MATH_BOXED_EXACT,
        d_internal_math_value_wide_retain, d_internal_math_value_wide_release,
        d_internal_math_value_wide_unary, d_internal_math_value_wide_binary,
        d_internal_math_value_wide_cmp, d_internal_math_value_wide_convert,
        d_internal_math_value_wide_text, NULL
    };

    return &table;
}


//==============================================================================
// 4.  CONSTRUCTION AND CONVERSION
//==============================================================================


// 4.1    d_math_value_from_wide
//------------------------------------------------------------------------------
// _x as a value of the kind, in a box of its own with one holder: the
// caller's, to give back with d_math_value_release. D_MATH_VALUE_ERROR_MEMORY
// where no storage is given. A negative 0 is 0.

D_MATH_FN_RT struct dj_math_value
d_math_value_from_wide(
    struct dj_math_wide _x
)
{
    struct dj_math_wide x = _x;

    x.negative = (d_math_wide_sign(_x) < 0);

    return d_internal_math_value_wide_box_of(d_math_value_wide_kind(), x);
}

// 4.2    d_math_value_to_wide
//------------------------------------------------------------------------------
// _x as a wide integer, into *_out, and true: a value of the kind, or an
// exact number that is an integer of the family, inline or a boxed kind's.
// False, and *_out as it was, for anything else.

D_MATH_FN_RT bool
d_math_value_to_wide(
    struct dj_math_wide* _out,
    struct dj_math_value _x
)
{
    struct dj_rational exact = { 0, 1 };

    // the kind's own
    if (d_math_value_is_wide(_x))
    {
        *_out = d_internal_math_value_wide_held(_x);

        return true;
    }

    // an integer of the family, or none
    if ( (!d_math_value_to_rational(&exact, _x)) ||
         (exact.den != 1) )
    {
        return false;
    }

    *_out = d_math_wide_from_imax(exact.num);

    return true;
}


#endif  // DJINTERP_C_MATH_VALUE_WIDE_H
