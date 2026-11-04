/*******************************************************************************
* djinterp [c/math]                                                   rational.h
*
* Exact rationals over the widest signed integer the build can spell: the C
* core of the math module's rational numbers.
*   A rational is a numerator and a positive denominator with no common
* factor, so every value has one form: 0 is 0/1, and the numerator carries the
* sign. Every kernel takes rationals in that form and gives one back;
* d_rational_make brings any numerator and nonzero denominator to it, and
* d_rational_is_valid tests for it.
*
*   EXACT OR NOTHING. A kernel either stores the exact result and returns
* D_RATIONAL_OK, or stores nothing and says why: the result is a rational that
* does not fit (D_RATIONAL_OVERFLOW), or the operation divides a nonzero value
* by zero (D_RATIONAL_POLE) or zero by zero (D_RATIONAL_INVALID). Overflow is
* reported exactly: when the reduced result does not fit, and never because an
* intermediate would not.
*   Products and quotients take Knuth's form (TAOCP 4.5.1): each numerator is
* cancelled against the other's denominator first, so the reduced result is
* formed directly and no intermediate exceeds it. A sum's one intermediate
* that can exceed a word -- the cross term t = a * (d / g) + c * (b / g) for
* a/b + c/d, g = gcd(b, d) -- is formed in double width, two words of
* d_math_umax, and divided down before the result is tested; double width is
* also how the products are tested and how comparisons are made, so nothing
* here needs a type wider than d_math_umax.
*
*   ONE TEXT, BOTH LANGUAGES. As in the rest of the C core, every kernel is
* pure arithmetic (D_MATH_FN): a header inline in C and below C++14, and a
* constant expression from C++14 -- also the ones that store their result
* through a pointer, when it points at the evaluation's own object. A
* kernel's arguments come by value, two words each.
*
*   TO FLOATING POINT. d_rational_to_f, _to_d and _to_ld give the nearest
* float, double or long double, ties to even, whatever the environment's
* rounding mode: the rounding is done once, in integers, and the result is
* then built exactly. A ratio of two-word integers converts the same way,
* for the value layer, which keeps an exact result too wide for a rational
* as the nearest long double.
*
*
* path:      /inc/djinterp/c/modules/math/rational.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LAYOUT AND STATUS
    -----------------
    1.  Layout
         1.  d_rational
    2.  Status
         1.  d_rational_status
2.  DOUBLE WIDTH
    ------------
    1.  Words
         1.  D_INTERNAL_RATIONAL_WORD_BITS
         2.  D_INTERNAL_RATIONAL_HALF_BITS
         3.  D_INTERNAL_RATIONAL_HALF_MASK
    2.  Layout
         1.  d_internal_rational_wide
         2.  d_internal_rational_divmod
         3.  d_internal_rational_ratio
    3.  Helpers
3.  CONSTRUCTION
    ------------
    1.  d_rational_from_int
    2.  d_rational_make
    3.  d_rational_is_valid
4.  ARITHMETIC
    ----------
    1.  Signs
    2.  Sums
    3.  Products and quotients
5.  COMPARISON
    ----------
    1.  d_rational_sign, d_rational_is_integer
    2.  d_rational_eq, d_rational_cmp
6.  TO FLOATING POINT
    -----------------
    1.  Rounding
         1.  d_internal_rational_rounded
    2.  Building the result
    3.  d_rational_to_f, d_rational_to_d, d_rational_to_ld
    4.  Two-word ratios
*/

#ifndef DJINTERP_C_MATH_RATIONAL_H
#define DJINTERP_C_MATH_RATIONAL_H 1

// std
#include <float.h>           // FLT_MANT_DIG, DBL_MANT_DIG, LDBL_MANT_DIG
#include <limits.h>          // CHAR_BIT
// djinterp
#include "./math_common.h"   // D_MATH_FN, d_math_imax, d_math_umax,
                             // D_MATH_IMAX_MAX, D_MATH_UMAX_MAX
#include "./float_order.h"   // d_internal_math_f_pow2 ... _ld_pow2


//==============================================================================
// 1.  LAYOUT AND STATUS
//==============================================================================


// 1.1    Layout
//------------------------------------------------------------------------------
// 1.1.1
// d_rational
//   struct: a rational over the widest signed integer the build can spell:
// den > 0, and num and den have no common factor; 0 is 0/1.
struct d_rational
{
    d_math_imax num;  // the numerator, which carries the sign
    d_math_imax den;  // the denominator, positive
};

// 1.2    Status
//------------------------------------------------------------------------------
// 1.2.1
// d_rational_status
//   enum: what a kernel came to. Only D_RATIONAL_OK stores a result; the
// others leave the destination as it was.
enum d_rational_status
{
    D_RATIONAL_OK       = 0,  // the exact result, stored
    D_RATIONAL_OVERFLOW = 1,  // a rational beyond d_math_imax
    D_RATIONAL_POLE     = 2,  // a nonzero value divided by zero
    D_RATIONAL_INVALID  = 3   // zero divided by zero
};


//==============================================================================
// 2.  DOUBLE WIDTH
//==============================================================================
// Two words of d_math_umax, the high and the low, hold what a word cannot:
// the product of two words, and sums of such products. Multiplication splits
// each word into halves, so every partial product fits a word; division by a
// word takes the high word natively and the low word a bit at a time.


// 2.1    Words
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_RATIONAL_WORD_BITS
//   constant: the bits in a d_math_umax, which has no padding bits on any
// target the framework supports.
#define D_INTERNAL_RATIONAL_WORD_BITS                                          \
    ((unsigned)(sizeof(d_math_umax) * CHAR_BIT))

// 2.1.2
// D_INTERNAL_RATIONAL_HALF_BITS
//   constant: the bits in half a word.
#define D_INTERNAL_RATIONAL_HALF_BITS  (D_INTERNAL_RATIONAL_WORD_BITS / 2u)

// 2.1.3
// D_INTERNAL_RATIONAL_HALF_MASK
//   constant: the low half of a word, all ones.
#define D_INTERNAL_RATIONAL_HALF_MASK                                          \
    (D_MATH_UMAX_MAX >> D_INTERNAL_RATIONAL_HALF_BITS)

// 2.2    Layout
//------------------------------------------------------------------------------
// 2.2.1
// d_internal_rational_wide
//   struct: an unsigned number of two words, hi * 2^w + lo.
struct d_internal_rational_wide
{
    d_math_umax hi;  // the high word
    d_math_umax lo;  // the low word
};

// 2.2.2
// d_internal_rational_divmod
//   struct: a double-width quotient and its one-word remainder.
struct d_internal_rational_divmod
{
    struct d_internal_rational_wide quotient;   // the quotient
    d_math_umax                     remainder;  // less than the divisor
};

// 2.2.3
// d_internal_rational_ratio
//   struct: a sum's or a product's exact result, before it is tested against
// the family: magnitudes of two words, no common factor, and a sign. 0 is
// 0/1, never negative.
struct d_internal_rational_ratio
{
    bool                            negative;  // below zero
    struct d_internal_rational_wide num;       // the numerator's magnitude
    struct d_internal_rational_wide den;       // the denominator, positive
};

// 2.3    Helpers
//------------------------------------------------------------------------------

// d_internal_rational_magnitude
//   |_x| as a d_math_umax, exact for every value, D_MATH_IMAX_MIN included.
D_MATH_FN d_math_umax
d_internal_rational_magnitude(
    d_math_imax _x
)
{
    return (_x < 0) ? ((d_math_umax)0 - (d_math_umax)_x)
                    : (d_math_umax)_x;
}

// d_internal_rational_gcd
//   the greatest common divisor of _a and _b: Euclid. gcd(0, _b) is _b, and
// gcd(0, 0) is 0, which no caller divides by.
D_MATH_FN d_math_umax
d_internal_rational_gcd(
    d_math_umax _a,
    d_math_umax _b
)
{
    d_math_umax a = _a;
    d_math_umax b = _b;

    // the remainder replaces the larger until it vanishes
    while (b != 0u)
    {
        const d_math_umax rest = a % b;

        a = b;
        b = rest;
    }

    return a;
}

// d_internal_rational_mul_wide
//   the exact product _a * _b in two words: each factor split into halves,
// the four partial products each within a word, the middle ones summed with
// the low product's carry.
D_MATH_FN struct d_internal_rational_wide
d_internal_rational_mul_wide(
    d_math_umax _a,
    d_math_umax _b
)
{
    const unsigned    half   = D_INTERNAL_RATIONAL_HALF_BITS;
    const d_math_umax mask   = D_INTERNAL_RATIONAL_HALF_MASK;
    const d_math_umax a_low  = _a & mask;
    const d_math_umax a_high = _a >> half;
    const d_math_umax b_low  = _b & mask;
    const d_math_umax b_high = _b >> half;
    const d_math_umax ll     = a_low * b_low;
    const d_math_umax lh     = a_low * b_high;
    const d_math_umax hl     = a_high * b_low;
    const d_math_umax hh     = a_high * b_high;
    const d_math_umax middle = (ll >> half) + (lh & mask) + (hl & mask);

    struct d_internal_rational_wide result = { 0u, 0u };

    result.lo = (ll & mask) | (middle << half);
    result.hi = hh + (lh >> half) + (hl >> half) + (middle >> half);

    return result;
}

// d_internal_rational_add_wide
//   _a + _b, which the callers' bounds keep within two words.
D_MATH_FN struct d_internal_rational_wide
d_internal_rational_add_wide(
    struct d_internal_rational_wide _a,
    struct d_internal_rational_wide _b
)
{
    struct d_internal_rational_wide result = { 0u, 0u };

    result.lo = _a.lo + _b.lo;
    result.hi = _a.hi + _b.hi + ((result.lo < _a.lo) ? 1u : 0u);

    return result;
}

// d_internal_rational_sub_wide
//   _a - _b, for _a >= _b.
D_MATH_FN struct d_internal_rational_wide
d_internal_rational_sub_wide(
    struct d_internal_rational_wide _a,
    struct d_internal_rational_wide _b
)
{
    struct d_internal_rational_wide result = { 0u, 0u };

    result.lo = _a.lo - _b.lo;
    result.hi = _a.hi - _b.hi - ((_a.lo < _b.lo) ? 1u : 0u);

    return result;
}

// d_internal_rational_cmp_wide
//   -1, 0 or 1 as _a is below, equal to or above _b.
D_MATH_FN int
d_internal_rational_cmp_wide(
    struct d_internal_rational_wide _a,
    struct d_internal_rational_wide _b
)
{
    // the high words decide, unless they are equal
    if (_a.hi != _b.hi)
    {
        return (_a.hi < _b.hi) ? -1 : 1;
    }

    // then the low words
    if (_a.lo != _b.lo)
    {
        return (_a.lo < _b.lo) ? -1 : 1;
    }

    return 0;
}

// d_internal_rational_divmod_wide
//   _n / _d and _n % _d, for any _d > 0: the high word divided natively, its
// remainder r carried into the low word's division, one bit at a time. r is
// below _d throughout, so 2r + 1 is below 2 * _d; where doubling r carries
// out of the word, the true value is past _d, and the wrapped subtraction
// gives the remainder exactly. The sums divide by a gcd of denominators,
// below 2^(w-1), where nothing carries; the carry serves larger divisors.
D_MATH_FN struct d_internal_rational_divmod
d_internal_rational_divmod_wide(
    struct d_internal_rational_wide _n,
    d_math_umax                     _d
)
{
    const unsigned width = D_INTERNAL_RATIONAL_WORD_BITS;

    struct d_internal_rational_divmod result = { { 0u, 0u }, 0u };
    d_math_umax                       rest   = 0u;
    unsigned                          i      = 0u;

    // within a word: native
    if (_n.hi == 0u)
    {
        result.quotient.lo = _n.lo / _d;
        result.remainder   = _n.lo % _d;

        return result;
    }

    result.quotient.hi = _n.hi / _d;
    rest               = _n.hi % _d;

    // the low word's bits, the highest first
    for (i = width; i > 0u; --i)
    {
        const unsigned    bit   = i - 1u;
        const bool        carry = ((rest >> (width - 1u)) != 0u);
        const d_math_umax next  = (_n.lo >> bit) & 1u;

        rest = (rest << 1) | next;

        // the divisor goes once more
        if ( (carry) ||
             (rest >= _d) )
        {
            rest               = rest - _d;
            result.quotient.lo = result.quotient.lo | ((d_math_umax)1 << bit);
        }
    }

    result.remainder = rest;

    return result;
}

// d_internal_rational_pack
//   stores -_n/_d where _negative, else _n/_d, if it fits: _n and _d have no
// common factor and _d > 0. The denominator must fit a d_math_imax, and the
// numerator too, or be D_MATH_IMAX_MIN's magnitude where it is negative. A
// zero numerator stores 0/1, whatever the sign.
D_MATH_FN enum d_rational_status
d_internal_rational_pack(
    struct d_rational* _out,
    bool               _negative,
    d_math_umax        _n,
    d_math_umax        _d
)
{
    const d_math_umax most = (d_math_umax)D_MATH_IMAX_MAX;

    // zero has one form
    if (_n == 0u)
    {
        _out->num = 0;
        _out->den = 1;

        return D_RATIONAL_OK;
    }

    // beyond the family
    if ( (_d > most) ||
         (_n > (_negative ? (most + 1u) : most)) )
    {
        return D_RATIONAL_OVERFLOW;
    }

    // -(_n - 1) - 1 reaches D_MATH_IMAX_MIN without passing through +2^(w-1)
    _out->num = _negative ? (-(d_math_imax)(_n - 1u) - 1)
                          : (d_math_imax)_n;
    _out->den = (d_math_imax)_d;

    return D_RATIONAL_OK;
}

// d_internal_rational_ratio_of
//   -_n/_d where _negative, else _n/_d, as a ratio: one word each, already
// without a common factor; 0 as 0/1, never negative.
D_MATH_FN struct d_internal_rational_ratio
d_internal_rational_ratio_of(
    bool        _negative,
    d_math_umax _n,
    d_math_umax _d
)
{
    struct d_internal_rational_ratio result = { false, { 0u, 0u },
                                                { 0u, 1u } };

    // zero has one form
    if (_n == 0u)
    {
        return result;
    }

    result.negative = _negative;
    result.num.lo   = _n;
    result.den.lo   = _d;

    return result;
}

// d_internal_rational_pack_ratio
//   stores _r if it fits, as d_internal_rational_pack does; past a word, a
// reduced ratio cannot fit.
D_MATH_FN enum d_rational_status
d_internal_rational_pack_ratio(
    struct d_rational*               _out,
    struct d_internal_rational_ratio _r
)
{
    // a word or more past the family
    if ( (_r.num.hi != 0u) ||
         (_r.den.hi != 0u) )
    {
        return D_RATIONAL_OVERFLOW;
    }

    return d_internal_rational_pack(_out, _r.negative, _r.num.lo, _r.den.lo);
}

// d_internal_rational_sum_ratio
//   (-1 or 1) * _an/_ad + (-1 or 1) * _bn/_bd, exactly, the signs given by
// _a_negative and _b_negative, each fraction without a common factor and its
// denominator positive. Knuth's form: with g = gcd(_ad, _bd), the cross term
// t = _an * (_bd / g) + _bn * (_ad / g), signs applied, formed in double width
// (each product is below 2^(2w - 2), so their sum fits two words); then with
// g2 = gcd(t, g), the sum is (t / g2) / ((_ad / g) * (_bd / g2)), reduced
// already.
D_MATH_FN struct d_internal_rational_ratio
d_internal_rational_sum_ratio(
    bool        _a_negative,
    d_math_umax _an,
    d_math_umax _ad,
    bool        _b_negative,
    d_math_umax _bn,
    d_math_umax _bd
)
{
    // a zero term: the sum is the other
    if (_an == 0u)
    {
        return d_internal_rational_ratio_of(_b_negative, _bn, _bd);
    }

    if (_bn == 0u)
    {
        return d_internal_rational_ratio_of(_a_negative, _an, _ad);
    }

    const d_math_umax g      = d_internal_rational_gcd(_ad, _bd);
    const d_math_umax a_part = _ad / g;
    const d_math_umax b_part = _bd / g;

    const struct d_internal_rational_wide p =
        d_internal_rational_mul_wide(_an, b_part);
    const struct d_internal_rational_wide q =
        d_internal_rational_mul_wide(_bn, a_part);

    struct d_internal_rational_ratio result   = { false, { 0u, 0u },
                                                  { 0u, 1u } };
    struct d_internal_rational_wide  t        = { 0u, 0u };
    bool                             negative = _a_negative;

    // like signs add; unlike signs leave the larger magnitude's sign
    if (_a_negative == _b_negative)
    {
        t = d_internal_rational_add_wide(p, q);
    }
    else
    {
        const int order = d_internal_rational_cmp_wide(p, q);

        // the terms cancel
        if (order == 0)
        {
            return result;
        }

        if (order > 0)
        {
            t = d_internal_rational_sub_wide(p, q);
        }
        else
        {
            t        = d_internal_rational_sub_wide(q, p);
            negative = _b_negative;
        }
    }

    // g2 = gcd(t, g) = gcd(t mod g, g): t needs no division where g is 1
    const d_math_umax g2 =
        (g == 1u)
            ? 1u
            : d_internal_rational_gcd(
                  d_internal_rational_divmod_wide(t, g).remainder, g);

    result.negative = negative;
    result.num      = (g2 == 1u)
                          ? t
                          : d_internal_rational_divmod_wide(t, g2).quotient;
    result.den      = d_internal_rational_mul_wide(a_part, _bd / g2);

    return result;
}

// d_internal_rational_product_ratio
//   (-1 or 1) * (_an/_ad) * (_bn/_bd), exactly, the sign given by _negative,
// each fraction without a common factor and its denominator positive.
// Knuth's form: with g1 = gcd(_an, _bd) and g2 = gcd(_bn, _ad), the product
// is ((_an / g1) * (_bn / g2)) / ((_ad / g2) * (_bd / g1)), reduced already.
D_MATH_FN struct d_internal_rational_ratio
d_internal_rational_product_ratio(
    bool        _negative,
    d_math_umax _an,
    d_math_umax _ad,
    d_math_umax _bn,
    d_math_umax _bd
)
{
    struct d_internal_rational_ratio result = { false, { 0u, 0u },
                                                { 0u, 1u } };

    // a zero factor
    if ( (_an == 0u) ||
         (_bn == 0u) )
    {
        return result;
    }

    const d_math_umax g1 = d_internal_rational_gcd(_an, _bd);
    const d_math_umax g2 = d_internal_rational_gcd(_bn, _ad);

    result.negative = _negative;
    result.num      = d_internal_rational_mul_wide(_an / g1, _bn / g2);
    result.den      = d_internal_rational_mul_wide(_ad / g2, _bd / g1);

    return result;
}

// d_internal_rational_sum
//   d_internal_rational_sum_ratio's result, stored if it fits.
D_MATH_FN enum d_rational_status
d_internal_rational_sum(
    struct d_rational* _out,
    bool               _a_negative,
    d_math_umax        _an,
    d_math_umax        _ad,
    bool               _b_negative,
    d_math_umax        _bn,
    d_math_umax        _bd
)
{
    return d_internal_rational_pack_ratio(
        _out,
        d_internal_rational_sum_ratio(_a_negative, _an, _ad,
                                      _b_negative, _bn, _bd));
}

// d_internal_rational_product
//   d_internal_rational_product_ratio's result, stored if it fits.
D_MATH_FN enum d_rational_status
d_internal_rational_product(
    struct d_rational* _out,
    bool               _negative,
    d_math_umax        _an,
    d_math_umax        _ad,
    d_math_umax        _bn,
    d_math_umax        _bd
)
{
    return d_internal_rational_pack_ratio(
        _out,
        d_internal_rational_product_ratio(_negative, _an, _ad, _bn, _bd));
}

//==============================================================================
// 3.  CONSTRUCTION
//==============================================================================


// 3.1    d_rational_from_int
//------------------------------------------------------------------------------
// the integer _n as a rational, _n/1; it always fits.
D_MATH_FN struct d_rational
d_rational_from_int(
    d_math_imax _n
)
{
    struct d_rational result = { 0, 1 };

    result.num = _n;

    return result;
}

// 3.2    d_rational_make
//------------------------------------------------------------------------------
// _num/_den brought to the one form: the common factor divided out and the
// sign moved to the numerator. D_RATIONAL_POLE for a nonzero _num over 0,
// D_RATIONAL_INVALID for 0/0, and D_RATIONAL_OVERFLOW where the reduced form
// does not fit, as for D_MATH_IMAX_MIN / -1.
D_MATH_FN enum d_rational_status
d_rational_make(
    struct d_rational* _out,
    d_math_imax        _num,
    d_math_imax        _den
)
{
    // a zero denominator: a pole, or no value at all
    if (_den == 0)
    {
        return (_num == 0) ? D_RATIONAL_INVALID : D_RATIONAL_POLE;
    }

    const d_math_umax n = d_internal_rational_magnitude(_num);
    const d_math_umax d = d_internal_rational_magnitude(_den);
    const d_math_umax g = d_internal_rational_gcd(n, d);

    return d_internal_rational_pack(_out,
                                    ((_num < 0) != (_den < 0)),
                                    n / g,
                                    d / g);
}

// 3.3    d_rational_is_valid
//------------------------------------------------------------------------------
// true when _x is in the one form: a positive denominator and no common
// factor, so zero only as 0/1.
D_MATH_FN bool
d_rational_is_valid(
    struct d_rational _x
)
{
    return ( (_x.den > 0) &&
             (d_internal_rational_gcd(d_internal_rational_magnitude(_x.num),
                                      (d_math_umax)_x.den) == 1u) );
}


//==============================================================================
// 4.  ARITHMETIC
//==============================================================================
// Each kernel stores its result through _out and returns D_RATIONAL_OK, or
// stores nothing and returns why. _out may be one of the operands' objects:
// the operands come by value.


// 4.1    Signs
//------------------------------------------------------------------------------
// neg is -_x and abs |_x|; both overflow only for a numerator of
// D_MATH_IMAX_MIN. inv is 1/_x: a pole at 0, and an overflow where _x's
// numerator is D_MATH_IMAX_MIN, whose magnitude no denominator can hold.

D_MATH_FN enum d_rational_status
d_rational_neg(
    struct d_rational* _out,
    struct d_rational  _x
)
{
    return d_internal_rational_pack(_out,
                                    (_x.num > 0),
                                    d_internal_rational_magnitude(_x.num),
                                    (d_math_umax)_x.den);
}

D_MATH_FN enum d_rational_status
d_rational_abs(
    struct d_rational* _out,
    struct d_rational  _x
)
{
    return d_internal_rational_pack(_out,
                                    false,
                                    d_internal_rational_magnitude(_x.num),
                                    (d_math_umax)_x.den);
}

D_MATH_FN enum d_rational_status
d_rational_inv(
    struct d_rational* _out,
    struct d_rational  _x
)
{
    // 1/0
    if (_x.num == 0)
    {
        return D_RATIONAL_POLE;
    }

    return d_internal_rational_pack(_out,
                                    (_x.num < 0),
                                    (d_math_umax)_x.den,
                                    d_internal_rational_magnitude(_x.num));
}

// 4.2    Sums
//------------------------------------------------------------------------------
// add is _a + _b, sub _a - _b: overflow only where the exact result does not
// fit.

D_MATH_FN enum d_rational_status
d_rational_add(
    struct d_rational* _out,
    struct d_rational  _a,
    struct d_rational  _b
)
{
    return d_internal_rational_sum(_out,
                                   (_a.num < 0),
                                   d_internal_rational_magnitude(_a.num),
                                   (d_math_umax)_a.den,
                                   (_b.num < 0),
                                   d_internal_rational_magnitude(_b.num),
                                   (d_math_umax)_b.den);
}

D_MATH_FN enum d_rational_status
d_rational_sub(
    struct d_rational* _out,
    struct d_rational  _a,
    struct d_rational  _b
)
{
    // _a + (-_b), the sign turned without negating D_MATH_IMAX_MIN
    return d_internal_rational_sum(_out,
                                   (_a.num < 0),
                                   d_internal_rational_magnitude(_a.num),
                                   (d_math_umax)_a.den,
                                   (_b.num > 0),
                                   d_internal_rational_magnitude(_b.num),
                                   (d_math_umax)_b.den);
}

// 4.3    Products and quotients
//------------------------------------------------------------------------------
// mul is _a * _b, div _a / _b: overflow only where the exact result does not
// fit; div by zero is a pole, or invalid for 0 / 0.

D_MATH_FN enum d_rational_status
d_rational_mul(
    struct d_rational* _out,
    struct d_rational  _a,
    struct d_rational  _b
)
{
    return d_internal_rational_product(_out,
                                       ((_a.num < 0) != (_b.num < 0)),
                                       d_internal_rational_magnitude(_a.num),
                                       (d_math_umax)_a.den,
                                       d_internal_rational_magnitude(_b.num),
                                       (d_math_umax)_b.den);
}

D_MATH_FN enum d_rational_status
d_rational_div(
    struct d_rational* _out,
    struct d_rational  _a,
    struct d_rational  _b
)
{
    // by zero
    if (_b.num == 0)
    {
        return (_a.num == 0) ? D_RATIONAL_INVALID : D_RATIONAL_POLE;
    }

    // _a times _b's reciprocal, its magnitudes swapped
    return d_internal_rational_product(_out,
                                       ((_a.num < 0) != (_b.num < 0)),
                                       d_internal_rational_magnitude(_a.num),
                                       (d_math_umax)_a.den,
                                       (d_math_umax)_b.den,
                                       d_internal_rational_magnitude(_b.num));
}


//==============================================================================
// 5.  COMPARISON
//==============================================================================


// 5.1    d_rational_sign, d_rational_is_integer
//------------------------------------------------------------------------------
// sign is -1, 0 or 1; is_integer is true when the denominator is 1.

D_MATH_FN int
d_rational_sign(
    struct d_rational _x
)
{
    return (_x.num > 0) ? 1 : ((_x.num < 0) ? -1 : 0);
}

D_MATH_FN bool
d_rational_is_integer(
    struct d_rational _x
)
{
    return (_x.den == 1);
}

// 5.2    d_rational_eq, d_rational_cmp
//------------------------------------------------------------------------------
// eq is true when _a and _b are the same value, which in the one form is the
// same numerator and denominator; cmp is -1, 0 or 1 as _a is below, equal to
// or above _b, by the cross products' magnitudes in double width, exactly.

D_MATH_FN bool
d_rational_eq(
    struct d_rational _a,
    struct d_rational _b
)
{
    return ( (_a.num == _b.num) &&
             (_a.den == _b.den) );
}

D_MATH_FN int
d_rational_cmp(
    struct d_rational _a,
    struct d_rational _b
)
{
    const int a_sign = d_rational_sign(_a);
    const int b_sign = d_rational_sign(_b);

    // different signs decide; two zeros are equal
    if ( (a_sign != b_sign) ||
         (a_sign == 0) )
    {
        return (a_sign < b_sign) ? -1 : ((a_sign > b_sign) ? 1 : 0);
    }

    // like signs: |_a.num| * _b.den against |_b.num| * _a.den
    const int order = d_internal_rational_cmp_wide(
        d_internal_rational_mul_wide(d_internal_rational_magnitude(_a.num),
                                     (d_math_umax)_b.den),
        d_internal_rational_mul_wide(d_internal_rational_magnitude(_b.num),
                                     (d_math_umax)_a.den));

    return (a_sign > 0) ? order : -order;
}


//==============================================================================
// 6.  TO FLOATING POINT
//==============================================================================
// A ratio to the nearest float, double or long double, ties to even, whatever
// the floating-point environment's rounding mode. The rounding is done once,
// in integers: a long division gives the quotient's leading P bits, P the
// type's precision, then the bit to round on, then whether a remainder is
// left. The result is then built from that P-bit mantissa and its exponent by
// products with powers of two, each exact. A rational lies between 2^(1-w)
// and 2^(w-1), w the bits in a word, inside even a float's normal range; a
// ratio of two-word integers, which converts to long double only, between
// 2^(-2w) and 2^(2w), inside a long double's. So no result is subnormal, and
// none overflows.

D_STATIC_ASSERT( ((unsigned)LDBL_MANT_DIG <=
                  2u * D_INTERNAL_RATIONAL_WORD_BITS),
                 "rational.h: a long double's mantissa must fit two words" );


// 6.1    Rounding
//------------------------------------------------------------------------------
// 6.1.1
// d_internal_rational_rounded
//   struct: a positive ratio rounded to P bits: mantissa * 2^exponent, the
// mantissa below 2^P with its top bit set, and which way the rounding went.
struct d_internal_rational_rounded
{
    struct d_internal_rational_wide mantissa;   // P bits, in two words
    int                             exponent;   // of the mantissa's lowest bit
    int                             direction;  // -1 down, 0 exact, 1 up
};

// d_internal_rational_bits
//   the bits _x needs: 0 for 0, otherwise its top bit's place plus one.
D_MATH_FN unsigned
d_internal_rational_bits(
    struct d_internal_rational_wide _x
)
{
    d_math_umax word  = (_x.hi != 0u) ? _x.hi : _x.lo;
    unsigned    count = (_x.hi != 0u) ? D_INTERNAL_RATIONAL_WORD_BITS : 0u;

    // the word's own bits, one at a time
    while (word != 0u)
    {
        word = word >> 1;
        ++count;
    }

    return count;
}

// d_internal_rational_shift_wide
//   _x * 2^_k, for _k below 2w, the bits shifted past the two words dropped.
D_MATH_FN struct d_internal_rational_wide
d_internal_rational_shift_wide(
    struct d_internal_rational_wide _x,
    unsigned                        _k
)
{
    const unsigned width = D_INTERNAL_RATIONAL_WORD_BITS;

    struct d_internal_rational_wide result = { 0u, 0u };

    // a word or more: the low word moves up
    if (_k >= width)
    {
        result.hi = _x.lo << (_k - width);

        return result;
    }

    // no shift: a shift by w would not be defined
    if (_k == 0u)
    {
        return _x;
    }

    result.hi = (_x.hi << _k) | (_x.lo >> (width - _k));
    result.lo = _x.lo << _k;

    return result;
}

// d_internal_rational_ones
//   2^_k - 1, the low _k bits set, for _k from 1 to 2w.
D_MATH_FN struct d_internal_rational_wide
d_internal_rational_ones(
    unsigned _k
)
{
    const unsigned width = D_INTERNAL_RATIONAL_WORD_BITS;

    struct d_internal_rational_wide result = { 0u, 0u };

    // the low word full, and some of the high one
    if (_k >= width)
    {
        result.lo = D_MATH_UMAX_MAX;
        result.hi = (_k == (2u * width))
                        ? D_MATH_UMAX_MAX
                        : (((d_math_umax)1 << (_k - width)) - 1u);

        return result;
    }

    result.lo = ((d_math_umax)1 << _k) - 1u;

    return result;
}

// d_internal_rational_round
//   _n / _d, both nonzero, rounded to _precision bits, at most 2w, to
// nearest, ties to even. _n and _d are first brought to one length, so that
// their quotient lies between 1/2 and 2 and its leading 1 is found by one
// comparison; the next bits come one at a time. The remainder r stays below
// the divisor, so doubling it carries out of the two words only where the
// true value is past the divisor, and there the wrapped subtraction gives the
// remainder exactly, as in divmod_wide.
D_MATH_FN struct d_internal_rational_rounded
d_internal_rational_round(
    struct d_internal_rational_wide _n,
    struct d_internal_rational_wide _d,
    unsigned                        _precision
)
{
    const unsigned width  = D_INTERNAL_RATIONAL_WORD_BITS;
    const unsigned n_bits = d_internal_rational_bits(_n);
    const unsigned d_bits = d_internal_rational_bits(_d);
    const struct d_internal_rational_wide one     = { 0u, 1u };
    const struct d_internal_rational_wide divisor =
        (d_bits < n_bits) ? d_internal_rational_shift_wide(_d, n_bits - d_bits)
                          : _d;

    struct d_internal_rational_rounded result = { { 0u, 0u }, 0, 0 };
    struct d_internal_rational_wide    rest   =
        (n_bits < d_bits) ? d_internal_rational_shift_wide(_n, d_bits - n_bits)
                          : _n;
    int      exponent = (int)n_bits - (int)d_bits;
    bool     carry    = false;
    bool     bit      = false;
    bool     round    = false;
    unsigned i        = 0u;

    // the leading 1: at 2^exponent, or one place lower, where the rest is
    // doubled first (2r - d < d, so a carry out of 2r does no harm)
    if (d_internal_rational_cmp_wide(rest, divisor) < 0)
    {
        rest = d_internal_rational_shift_wide(rest, 1u);
        --exponent;
    }

    rest            = d_internal_rational_sub_wide(rest, divisor);
    result.mantissa = one;

    // the next _precision - 1 bits, the highest first
    for (i = 1u; i < _precision; ++i)
    {
        carry = ((rest.hi >> (width - 1u)) != 0u);
        rest  = d_internal_rational_shift_wide(rest, 1u);
        bit   = ( (carry) ||
                  (d_internal_rational_cmp_wide(rest, divisor) >= 0) );

        // the divisor goes once more
        if (bit)
        {
            rest = d_internal_rational_sub_wide(rest, divisor);
        }

        result.mantissa    = d_internal_rational_shift_wide(result.mantissa,
                                                            1u);
        result.mantissa.lo = result.mantissa.lo | (bit ? 1u : 0u);
    }

    // the bit to round on
    carry = ((rest.hi >> (width - 1u)) != 0u);
    rest  = d_internal_rational_shift_wide(rest, 1u);
    round = ( (carry) ||
              (d_internal_rational_cmp_wide(rest, divisor) >= 0) );

    // the remainder past it
    if (round)
    {
        rest = d_internal_rational_sub_wide(rest, divisor);
    }

    // up where that bit is 1 and something follows it, or nothing does and
    // the mantissa is odd; down where anything at all was cut off
    const bool sticky = ( (rest.hi != 0u) ||
                          (rest.lo != 0u) );
    const bool up     = ( (round) &&
                          ( (sticky) ||
                            ((result.mantissa.lo & 1u) != 0u) ) );

    result.direction = (up) ? 1
                            : ( ( (round) ||
                                  (sticky) ) ? -1 : 0 );

    if (up)
    {
        // all P bits 1: the carry makes 2^P, which is 2^(P - 1) a binade up
        if (d_internal_rational_cmp_wide(result.mantissa,
                                         d_internal_rational_ones(_precision))
                == 0)
        {
            result.mantissa =
                d_internal_rational_shift_wide(one, _precision - 1u);
            ++exponent;
        }
        else
        {
            result.mantissa = d_internal_rational_add_wide(result.mantissa,
                                                           one);
        }
    }

    result.exponent = exponent - ((int)_precision - 1);

    return result;
}


// 6.2    Building the result
//------------------------------------------------------------------------------
// The mantissa, hi * 2^w + lo, has at most P bits, so each word converts
// exactly, and so do the product and the sum; the power of two then moves it
// to its place, exactly too.

D_MATH_FN float
d_internal_rational_build_f(
    bool                               _negative,
    struct d_internal_rational_rounded _r
)
{
    const float word  =
        d_internal_math_f_pow2((int)D_INTERNAL_RATIONAL_WORD_BITS);
    const float high  = (float)_r.mantissa.hi;
    const float low   = (float)_r.mantissa.lo;
    const float upper = high * word;
    const float whole = upper + low;
    const float value = whole * d_internal_math_f_pow2(_r.exponent);

    return _negative ? -value : value;
}

D_MATH_FN double
d_internal_rational_build_d(
    bool                               _negative,
    struct d_internal_rational_rounded _r
)
{
    const double word  =
        d_internal_math_d_pow2((int)D_INTERNAL_RATIONAL_WORD_BITS);
    const double high  = (double)_r.mantissa.hi;
    const double low   = (double)_r.mantissa.lo;
    const double upper = high * word;
    const double whole = upper + low;
    const double value = whole * d_internal_math_d_pow2(_r.exponent);

    return _negative ? -value : value;
}

D_MATH_FN long double
d_internal_rational_build_ld(
    bool                               _negative,
    struct d_internal_rational_rounded _r
)
{
    const long double word  =
        d_internal_math_ld_pow2((int)D_INTERNAL_RATIONAL_WORD_BITS);
    const long double high  = (long double)_r.mantissa.hi;
    const long double low   = (long double)_r.mantissa.lo;
    const long double upper = high * word;
    const long double whole = upper + low;
    const long double value = whole * d_internal_math_ld_pow2(_r.exponent);

    return _negative ? -value : value;
}


// 6.3    d_rational_to_f, d_rational_to_d, d_rational_to_ld
//------------------------------------------------------------------------------
// _x as the nearest float, double or long double, ties to even; 0 is +0.

D_MATH_FN float
d_rational_to_f(
    struct d_rational _x
)
{
    struct d_internal_rational_wide n = { 0u, 0u };
    struct d_internal_rational_wide d = { 0u, 0u };

    // 0
    if (_x.num == 0)
    {
        return (float)0;
    }

    n.lo = d_internal_rational_magnitude(_x.num);
    d.lo = (d_math_umax)_x.den;

    return d_internal_rational_build_f(
        (_x.num < 0),
        d_internal_rational_round(n, d, (unsigned)FLT_MANT_DIG));
}

D_MATH_FN double
d_rational_to_d(
    struct d_rational _x
)
{
    struct d_internal_rational_wide n = { 0u, 0u };
    struct d_internal_rational_wide d = { 0u, 0u };

    // 0
    if (_x.num == 0)
    {
        return (double)0;
    }

    n.lo = d_internal_rational_magnitude(_x.num);
    d.lo = (d_math_umax)_x.den;

    return d_internal_rational_build_d(
        (_x.num < 0),
        d_internal_rational_round(n, d, (unsigned)DBL_MANT_DIG));
}

D_MATH_FN long double
d_rational_to_ld(
    struct d_rational _x
)
{
    struct d_internal_rational_wide n = { 0u, 0u };
    struct d_internal_rational_wide d = { 0u, 0u };

    // 0
    if (_x.num == 0)
    {
        return (long double)0;
    }

    n.lo = d_internal_rational_magnitude(_x.num);
    d.lo = (d_math_umax)_x.den;

    return d_internal_rational_build_ld(
        (_x.num < 0),
        d_internal_rational_round(n, d, (unsigned)LDBL_MANT_DIG));
}


// 6.4    Two-word ratios
//------------------------------------------------------------------------------

// d_internal_rational_wide_to_ld
//   -(_n / _d) where _negative, else _n / _d, as the nearest long double,
// ties to even; _d nonzero, and 0 is +0. The value layer keeps an exact
// result that does not fit a rational this way, from its numerator and
// denominator in two words each (roadmap gate G2).
D_MATH_FN long double
d_internal_rational_wide_to_ld(
    bool                            _negative,
    struct d_internal_rational_wide _n,
    struct d_internal_rational_wide _d
)
{
    // 0
    if ( (_n.hi == 0u) &&
         (_n.lo == 0u) )
    {
        return (long double)0;
    }

    return d_internal_rational_build_ld(
        _negative,
        d_internal_rational_round(_n, _d, (unsigned)LDBL_MANT_DIG));
}


#endif  // DJINTERP_C_MATH_RATIONAL_H
