"""gen_c_math.circular - the text of circular.h (chunk 69, step 55, T4).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (banner)
from .exp_d import (exp_d_round, exp_d_literal, exp_d_digits_literal)


# -----------------------------------------------------------------------------
# circular.h
# -----------------------------------------------------------------------------
# The double family's reduction by pi/2 and its wrapping of an angle into one
# turn, as ruling 15 places them with the sine and cosine (step 55, T4). The
# constants are computed here with integers alone: pi by Machin's formula, 16
# atan(1/5) - 4 atan(1/239), each series' error bounded, each value taken
# where both ends of the interval holding it agree (S511).

CIRC_BITS = 1600          # pi's interval, in bits past the point
CIRC_DIGITS = 320         # 2/pi's hexadecimal digits: 1,280 bits


def circ_atan_inv(q, bits):
    """atan(1/q) 2^bits, each term truncated: within the returned count of
    units of the exact value."""
    one = 1 << bits
    total, k, term, q2 = 0, 0, one // q, q * q
    while term:
        part = term // (2 * k + 1)
        total += part if k % 2 == 0 else -part
        term //= q2
        k += 1
    return total, k + 2


def circ_pi_interval(bits):
    """(lo, hi): lo <= pi 2^bits <= hi."""
    a, ea = circ_atan_inv(5, bits)
    b, eb = circ_atan_inv(239, bits)
    v = 16 * a - 4 * b
    err = 16 * ea + 4 * eb
    return v - err, v + err


def circ_floor_pi(n):
    """floor(pi 2^n), the same at both ends of its interval."""
    lo, hi = circ_pi_interval(n + 64)
    assert (lo >> 64) == (hi >> 64)
    return lo >> 64


def circ_floor_two_over_pi(n):
    """floor(2/pi 2^n), the same at both ends of its interval."""
    lo, hi = circ_pi_interval(n + 68)
    num = 1 << (2 * n + 69)
    assert num // hi == num // lo
    return num // hi


def circ_pi_fraction():
    """pi as an exact fraction within 2^-CIRC_BITS of it, below: for the
    doubles' rounding, each checked at both ends."""
    from fractions import Fraction
    lo, hi = circ_pi_interval(CIRC_BITS)
    return Fraction(lo, 1 << CIRC_BITS), Fraction(hi, 1 << CIRC_BITS)


def circ_round_both(lo, hi, bits=53):
    """the rounding of a value in [lo, hi], the same at both ends."""
    a = exp_d_round(lo.numerator, lo.denominator, bits)
    b = exp_d_round(hi.numerator, hi.denominator, bits)
    assert a == b, "the interval straddles a rounding"
    return a


def circ_value(m, e):
    from fractions import Fraction
    return Fraction(m) * Fraction(2) ** e


def circ_constants():
    """the literals: 2/pi rounded, pi/2 in four doubles, c pi/2 as pairs,
    and pi, 2 pi rounded."""
    from fractions import Fraction
    plo, phi = circ_pi_fraction()
    consts = {}
    consts["INV"] = exp_d_literal(*circ_round_both(2 / phi, 2 / plo))
    # pi/2 = C0 + C1 + C2 + C3 + tau, each the double nearest what remains
    lo, hi = plo / 2, phi / 2
    for i in range(4):
        m, e = circ_round_both(lo, hi)
        consts["C%d" % i] = exp_d_literal(m, e)
        lo -= circ_value(m, e)
        hi -= circ_value(m, e)
    mults = []
    for c in range(-2, 5):
        if c == 0:
            mults.append(("0.0", "0.0"))
            continue
        lo, hi = sorted((c * plo / 2, c * phi / 2))
        mh, eh = circ_round_both(lo, hi)
        h = circ_value(mh, eh)
        ml, el = circ_round_both(lo - h, hi - h)
        mults.append((exp_d_literal(mh, eh), exp_d_literal(ml, el)))
    consts["PI"] = exp_d_literal(*circ_round_both(plo, phi))
    consts["TWO_PI"] = exp_d_literal(*circ_round_both(2 * plo, 2 * phi))
    return consts, mults


def circ_digits():
    """2/pi's first 1,280 bits and pi/2 2^224 rounded down, as hexadecimal
    digit strings."""
    t = circ_floor_two_over_pi(4 * CIRC_DIGITS)
    h = circ_floor_pi(223)
    return "%0*x" % (CIRC_DIGITS, t), "%057x" % h


def circ_digits_macro(name, digits):
    lines = exp_d_digits_literal(name, digits, 64)
    out = ["#define " + name + " " * (79 - 9 - len(name)) + "\\"]
    for i, line in enumerate(lines):
        last = (i == len(lines) - 1)
        out.append(line if last else line + " " * (79 - len(line)) + "\\")
    return "\n".join(out)


CIRCULAR_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE REDUCTION BY PI/2
    ---------------------
    1.  Constants
    2.  Payne and Hanek's, in fixed point
    3.  Cody and Waite's, in pairs of doubles
    4.  The reduction
2.  THE WRAPPING
    ------------
    1.  Into a turn, in radians
    2.  Into a turn, in degrees
*/

#ifndef DJINTERP_C_MATH_CIRCULAR_H
#define DJINTERP_C_MATH_CIRCULAR_H 1

// std
#include <float.h>             // DBL_MAX
#include <math.h>              // NAN
// djinterp
#include "./dd.h"              // dj_math_dd, d_math_dd_two_sum, ...
#include "./exponential_d.h"   // d_internal_math_d_exp_fixed_*, _exp_pow2
#include "./float_order.h"     // d_internal_math_d_binade_of
#include "./math_common.h"     // D_MATH_FN, D_MATH_DBL_EVAL_WIDE
#include "./remainder.h"       // d_math_d_remainder_c, d_math_d_fmod_c
"""

CIRCULAR_BODY = r"""

//==============================================================================
// 1.  THE REDUCTION BY PI/2
//==============================================================================
// x = (4 j + q) pi/2 + r, q in [0, 4) and |r| at most pi/4 and a hair, j an
// integer: q and r are what a sine, a cosine or a wrapping into one turn needs
// of x, and r must be good to its own last bits however near x lies to a
// multiple of pi/2. Two paths. Below 2^31, Cody and Waite's: k, the integer
// nearest x 2/pi, and x - k pi/2 with pi/2 in four doubles, every product by k
// exact as a pair and every sum that cancels exact, the result a pair within
// 2^-103 of r, relatively (section 1.3). At any finite x, Payne and Hanek's:
// x's significand times the 290 bits of 2/pi its exponent reaches, in integer
// columns of 16-bit limbs, the integer part kept mod 4, the fraction f in
// units of 2^-224 (section 1.2); then r = f pi/2, either as a pair, within
// 2^-102 of r, or in fixed point, within 6.6 units of 2^-224 of the angle, for
// a wrapping the pair cannot round. No double lies nearer a multiple of pi/2
// than 2^-60.89, 6381956970095103 2^797 the nearest, so each path keeps r to
// better than 2^-102 of itself (S511).


// 1.1    Constants
//------------------------------------------------------------------------------
// The generator computes them with integers alone (S511).

// D_INTERNAL_MATH_D_TWO_OVER_PI_DIGITS
//   2/pi's first 1,280 bits, in 320 hexadecimal digits from the binary
// point: digit d holds bits 4 d + 1 to 4 d + 4, bit i of weight 2^-i. The
// fixed path reads bits up to a double's largest exponent, 1023, plus 236.
@TWO_OVER_PI@

// D_INTERNAL_MATH_D_HALF_PI_DIGITS
//   pi/2 times 2^224, rounded down, in 57 hexadecimal digits.
@HALF_PI@

// d_internal_math_d_circ_digit
//   the value of the hexadecimal digit _c.
D_MATH_FN unsigned long
d_internal_math_d_circ_digit(
    char _c
)
{
    return (_c <= '9') ? (unsigned long)(_c - '0')
                       : (unsigned long)((_c - 'a') + 10);
}

// d_internal_math_d_circ_dd
//   the pair _high + _low, as given.
D_MATH_FN struct dj_math_dd
d_internal_math_d_circ_dd(
    double _high,
    double _low
)
{
    struct dj_math_dd result = { 0.0, 0.0 };

    result.high = _high;
    result.low  = _low;

    return result;
}

// d_internal_math_d_half_pi_multiple
//   c pi/2 as a pair for c from -2 to 4, its high part (_low false) the
// double nearest, its low part (_low true) the double nearest the rest:
// within 2^-107 of c pi/2, relatively; 0 for c 0. Each exact as written.
D_MATH_FN double
d_internal_math_d_half_pi_multiple(
    int  _c,
    bool _low
)
{
    switch (_c)
    {
        case -2:
            return _low ? @M_2L@
                        : @M_2H@;
        case -1:
            return _low ? @M_1L@
                        : @M_1H@;
        case 1:
            return _low ? @M1L@
                        : @M1H@;
        case 2:
            return _low ? @M2L@
                        : @M2H@;
        case 3:
            return _low ? @M3L@
                        : @M3H@;
        case 4:
            return _low ? @M4L@
                        : @M4H@;
        default:
            return 0.0;
    }
}

// d_internal_math_d_half_pi_fixed
//   pi/2 2^224, rounded down, from its digits: the limbs from the last digit
// up, four digits a limb.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_half_pi_fixed(void)
{
    struct dj_internal_math_d_exp_fixed result = { { 0 } };
    int i = 0;

    for (i = 0; i < 57; ++i)
    {
        const int place = 56 - i;

        result.limb[place / 4] |=
            d_internal_math_d_circ_digit(D_INTERNAL_MATH_D_HALF_PI_DIGITS[i])
            << (4 * (place % 4));
    }

    return result;
}

// dj_internal_math_d_fixed_angle
//   struct: an argument reduced in fixed point: x = (4 j + quadrant) pi/2 +
// f pi/2, |f| <= 1/2 and a hair, f's magnitude in units of 2^-224.
struct dj_internal_math_d_fixed_angle
{
    struct dj_internal_math_d_exp_fixed f;         // |f| 2^224
    bool                                negative;  // whether f < 0
    int                                 quadrant;  // in [0, 4)
};


// 1.2    Payne and Hanek's, in fixed point
//------------------------------------------------------------------------------
// |x| = m 2^(e - 52), m an integer below 2^53, so x 2/pi = m 2^(e - 52) times
// the sum of 2/pi's bits b_i 2^-i. Each bit before i = e - 53 adds a multiple
// of 4, which the quadrant drops, and the bits past i = e + 236 add less
// than 2^(e + 1 - (e + 236)) = 2^-235 in all; so m times the window of bits
// e - 67 to e + 236 (19 limbs of 16, the first 14 bits multiples of 4 too)
// is x 2/pi mod 4 in units of 2^-288. The product is taken in columns of
// 16-bit limbs, each product of two limbs below 2^32 and split in halves as
// it lands, so no column passes 2^19 and no integer wider than 32 bits is
// needed; the lowest 64 bits are dropped and the bits from 2^2 up, leaving x
// 2/pi mod 4 in units of 2^-224, truncated: below the exact value by less
// than 1 + 2^-11 units. Its integer part is the quadrant; a fraction of 1/2
// or more is taken as f - 1, the quadrant one more, its magnitude 2^224 less
// the fraction. A negative x negates both.

// d_internal_math_d_two_over_pi_bits
//   bits _i to _i + 15 of 2/pi as a 16-bit integer, bit _i its highest; a
// bit before the first, i below 1, is 0. Five digits read, 20 bits, and the
// 16 wanted shifted down.
D_MATH_FN unsigned long
d_internal_math_d_two_over_pi_bits(
    int _i
)
{
    unsigned long v = 0;
    int           d = 0;
    int           k = 0;

    if (_i < 1)
    {
        return (_i <= -15) ? 0ul
                           : (d_internal_math_d_two_over_pi_bits(1) >>
                              (1 - _i));
    }

    d = (_i - 1) / 4;

    for (k = 0; k < 5; ++k)
    {
        v = (v << 4) |
            d_internal_math_d_circ_digit(
                D_INTERNAL_MATH_D_TWO_OVER_PI_DIGITS[d + k]);
    }

    return (v >> (4 - ((_i - 1) % 4))) & 0xFFFFul;
}

// d_internal_math_d_reduce_fixed
//   _x reduced by pi/2 in fixed point, _x finite and |_x| >= 2^-60: f within
// 1 + 2^-11 units of 2^-224 of the exact fraction.
D_MATH_FN struct dj_internal_math_d_fixed_angle
d_internal_math_d_reduce_fixed(
    double _x
)
{
    const double ax = (_x < 0.0) ? -_x : _x;
    const struct dj_internal_math_d_binade binade =
        d_internal_math_d_binade_of(ax);
    // 2^e <= |x| < 2^(e + 1); m = |x| 2^(52 - e), an integer, exactly
    const int     e      = binade.exponent;
    double        m      = (ax / binade.power) * 4503599627370496.0;
    unsigned long digits[4]   = { 0 };
    unsigned long window[19]  = { 0 };
    unsigned long columns[24] = { 0 };
    struct dj_internal_math_d_fixed_angle result =
        { { { 0 } }, false, 0 };
    int i = 0;
    int j = 0;

    // m's limbs, from 2^48 down, each division by a power of 2 exact
    for (i = 3; i >= 0; --i)
    {
        const double        weight = d_internal_math_d_exp_pow2(16 * i);
        const unsigned long digit  = (unsigned long)(m / weight);

        digits[i] = digit;
        m        -= (double)digit * weight;
    }

    // the window, limb j holding bits e + 221 - 16 j to e + 236 - 16 j
    for (j = 0; j < 19; ++j)
    {
        window[j] = d_internal_math_d_two_over_pi_bits((e + 221) - (16 * j));
    }

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 19; ++j)
        {
            const unsigned long p = digits[i] * window[j];

            columns[i + j]     += p & 0xFFFFul;
            columns[i + j + 1] += p >> 16;
        }
    }

    d_internal_math_d_exp_fixed_carry(columns, 24);

    // units of 2^-224 from column 4; the integer part's two bits in 18
    for (i = 0; i < 14; ++i)
    {
        result.f.limb[i] = columns[i + 4];
    }

    result.quadrant = (int)(columns[18] & 3ul);

    // a fraction of 1/2 or more: f - 1, the quadrant one more
    if (result.f.limb[13] >= 0x8000ul)
    {
        struct dj_internal_math_d_exp_fixed one = { { 0 } };

        one.limb[14]    = 1ul;
        result.f        = d_internal_math_d_exp_fixed_sub(one, result.f);
        result.negative = true;
        result.quadrant = (result.quadrant + 1) % 4;
    }

    if (_x < 0.0)
    {
        result.negative = !result.negative;
        result.quadrant = (4 - result.quadrant) % 4;
    }

    return result;
}

// d_internal_math_d_circ_part
//   _v's bits from 2^_s up (_high true) or below 2^_s (_high false), in units
// of 2^-224.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_circ_part(
    struct dj_internal_math_d_exp_fixed _v,
    int                                 _s,
    bool                                _high
)
{
    const unsigned long below = (1ul << (_s % 16)) - 1ul;
    int i = 0;

    for (i = 0; i < 16; ++i)
    {
        if ((16 * i) + 16 <= _s)
        {
            _v.limb[i] = _high ? 0ul : _v.limb[i];
        }
        else if ((16 * i) >= _s)
        {
            _v.limb[i] = _high ? _v.limb[i] : 0ul;
        }
        else
        {
            _v.limb[i] = _high ? (_v.limb[i] & ~below)
                               : (_v.limb[i] & below);
        }
    }

    return _v;
}

// d_internal_math_d_circ_round
//   _v 2^-224 rounded once to a double, ties to even; 0 for 0. The value is
// below 2^32, so no rounding reaches the subnormals. With b its bits and s =
// b - 53, m = floor(_v / 2^s) is gathered limb by limb from the top, each
// partial sum an integer below 2^53 and so exact; the bit below 2^s and
// whether any lies under it decide the rounding.
D_MATH_FN double
d_internal_math_d_circ_round(
    struct dj_internal_math_d_exp_fixed _v
)
{
    const int bits = d_internal_math_d_exp_fixed_bits(_v);
    const int s    = (bits > 53) ? (bits - 53) : 0;
    const int q    = s / 16;
    const int o    = s % 16;
    double m       = 0.0;
    bool   half    = false;
    bool   sticky  = false;
    int    i       = 0;

    if (bits == 0)
    {
        return 0.0;
    }

    for (i = 15; i > q; --i)
    {
        m = (m * 65536.0) + (double)_v.limb[i];
    }

    m = (m * d_internal_math_d_exp_pow2(16 - o)) + (double)(_v.limb[q] >> o);

    if (s > 0)
    {
        // the bit at 2^(s - 1), and every bit below it
        const int r  = s - 1;
        const int rq = r / 16;
        const int ro = r % 16;

        half   = ((_v.limb[rq] >> ro) & 1ul) != 0;
        sticky = (_v.limb[rq] & ((1ul << ro) - 1ul)) != 0;

        for (i = 0; i < rq; ++i)
        {
            sticky = sticky || (_v.limb[i] != 0);
        }
    }

    if ( half && ( sticky || (d_internal_math_d_exp_fixed_bit(_v, s) != 0) ) )
    {
        m += 1.0;
    }

    return m * d_internal_math_d_exp_pow2(s - 224);
}

// d_internal_math_d_circ_pair
//   _v 2^-224, of sign _negative, as a pair: its first 53 bits exactly, the
// rest rounded once, then normalized exactly by Fast2Sum: within 2^-105 of
// _v, relatively, the high part a double nearest _v.
D_MATH_FN struct dj_math_dd
d_internal_math_d_circ_pair(
    struct dj_internal_math_d_exp_fixed _v,
    bool                                _negative
)
{
    const int bits = d_internal_math_d_exp_fixed_bits(_v);
    const int s    = (bits > 53) ? (bits - 53) : 0;
    const double high = d_internal_math_d_circ_round(
        d_internal_math_d_circ_part(_v, s, true));
    const double low  = d_internal_math_d_circ_round(
        d_internal_math_d_circ_part(_v, s, false));
    const struct dj_math_dd result = d_math_dd_fast_two_sum(high, low);

    return _negative ? d_math_dd_neg(result)
                     : result;
}

// dj_internal_math_d_angle
//   struct: an argument reduced by pi/2 as a pair: x = (4 j + quadrant) pi/2
// + r, |r| <= pi/4 and a hair.
struct dj_internal_math_d_angle
{
    struct dj_math_dd r;
    int               quadrant;  // in [0, 4)
};

#if !D_MATH_DBL_EVAL_WIDE
// d_internal_math_d_reduce_far
//   _x reduced by pi/2 through the fixed path, _x finite and |_x| >= 1/2: f
// as a pair, within 2^-105 of f, relatively, times pi/2 as the pair C0 + C1,
// within 2^-108.3 of it, by dd.h's product, within 7u^2: r within 2^-102 of
// itself, relatively.
D_MATH_FN struct dj_internal_math_d_angle
d_internal_math_d_reduce_far(
    double _x
)
{
    const struct dj_internal_math_d_fixed_angle a =
        d_internal_math_d_reduce_fixed(_x);
    struct dj_internal_math_d_angle result = { { 0.0, 0.0 }, 0 };

    result.r = d_math_dd_mul(
        d_internal_math_d_circ_pair(a.f, a.negative),
        d_internal_math_d_circ_dd(@C0@,
                                  @C1@));
    result.quadrant = a.quadrant;

    return result;
}
#endif  // !D_MATH_DBL_EVAL_WIDE


// 1.3    Cody and Waite's, in pairs of doubles
//------------------------------------------------------------------------------
// For |x| < 2^31: k, the integer nearest x 2/pi, taken from the exact
// product's high part by the shift 1.5 2^52, so that no compiler can fuse it
// with the shift, |k| <= 1367130551; pi/2 = C0 + C1 + C2 + C3 + tau, each the
// double nearest what remains, |tau| < 2^-217. Then, each product as an exact
// pair (dd.h's two_product), k C0 = p1 + e1; s = x - p1, exact (x and p1 lie
// within a binade of each other and |s| < 1, a multiple of the smaller one's
// last place); s - e1 and that less p2's high part exactly, by TwoSum, to h2
// plus the rests; the rests, e2, k C2 and the high part of k C3 summed as a
// pair, the last step by TwoSum; and h2 plus that pair by AccurateDWPlusDW.
// Every sum that cancels is exact, so the only roundings are the pair sum's,
// within 3u^2 of the result, the small sum's four, and those of k C3 and tau,
// together below 2^-115.8 of r: within 2^-104.3 of r, relatively, since |r| >=
// 2^-60.49 below 2^31 (chunk 69's record, proofs/reduce_bounds.py), stated as
// 2^-103. Where double is evaluated wider than double, a 32-bit x87, TwoSum is
// not exact and this path is not taken.

#if !D_MATH_DBL_EVAL_WIDE
// d_internal_math_d_reduce_pair
//   _x reduced by pi/2, |_x| < 2^31: r within 2^-103 of x - k pi/2,
// relatively.
D_MATH_FN struct dj_internal_math_d_angle
d_internal_math_d_reduce_pair(
    double _x
)
{
    // 2/pi, and 1.5 2^52
    const double inv   = @INV@;
    const double shift = 6755399441055744.0;
    // pi/2 in four doubles
    const double c0 = @C0@;
    const double c1 = @C1@;
    const double c2 = @C2@;
    const double c3 = @C3@;
    const double k  = (d_math_dd_two_product(_x, inv).high + shift) - shift;
    struct dj_internal_math_d_angle result = { { 0.0, 0.0 }, 0 };

    if (k == 0.0)
    {
        result.r.high = _x;

        return result;
    }

    {
        const struct dj_math_dd p1 = d_math_dd_two_product(k, c0);
        const struct dj_math_dd p2 = d_math_dd_two_product(k, c1);
        const struct dj_math_dd p3 = d_math_dd_two_product(k, c2);
        const double            p4 = d_math_dd_two_product(k, c3).high;
        const double            s  = _x - p1.high;
        const struct dj_math_dd h1 = d_math_dd_two_sum(s, -p1.low);
        const struct dj_math_dd h2 = d_math_dd_two_sum(h1.high, -p2.high);
        const struct dj_math_dd a1 = d_math_dd_two_sum(h1.low, h2.low);
        const struct dj_math_dd a2 = d_math_dd_two_sum(a1.high, -p2.low);
        const struct dj_math_dd a3 = d_math_dd_two_sum(a2.high, -p3.high);
        const double            b  = ((a1.low + a2.low) + a3.low) +
                                     (-p3.low - p4);
        const long              n  = (long)k;

        result.r = d_math_dd_add(d_internal_math_d_circ_dd(h2.high, 0.0),
                                 d_math_dd_two_sum(a3.high, b));
        result.quadrant = (int)(((n % 4) + 4) % 4);
    }

    return result;
}
#endif  // !D_MATH_DBL_EVAL_WIDE


// 1.4    The reduction
//------------------------------------------------------------------------------
// reduce_half_pi gives r as a pair and the quadrant q in [0, 4), x = (4 j +
// q) pi/2 + r for an integer j, |r| at most pi/4 and a hair: r within 2^-102
// of itself, relatively, at every finite x, by Cody and Waite's way below
// 2^31 and Payne and Hanek's past it (S512). |x| below pi/4 is r itself, q
// 0; NaN and the infinities give NaN, q 0. A constant expression from
// C++14. Where double is evaluated wider than double, a 32-bit x87, it
// refuses to compile, as exp_c and log_c do: build with -msse2
// -mfpmath=sse (math_common.h, D_MATH_DBL_REFUSE).

D_MATH_FN struct dj_math_dd
d_math_d_reduce_half_pi(
    double _x,
    int*   _quadrant
)
{
    const double ax = (_x < 0.0) ? -_x : _x;

    D_MATH_DBL_REFUSE(_x);

    *_quadrant = 0;

    if (!(ax <= DBL_MAX))
    {
        return d_internal_math_d_circ_dd(_x - _x, 0.0);
    }

    // pi/4's double, which is below pi/4
    if (ax <= @PIO4@)
    {
        return d_internal_math_d_circ_dd(_x, 0.0);
    }

#if !D_MATH_DBL_EVAL_WIDE
    if (ax < 2147483648.0)
    {
        const struct dj_internal_math_d_angle a =
            d_internal_math_d_reduce_pair(_x);

        *_quadrant = a.quadrant;

        return a.r;
    }

    {
        const struct dj_internal_math_d_angle a =
            d_internal_math_d_reduce_far(_x);

        *_quadrant = a.quadrant;

        return a.r;
    }
#else
    return d_internal_math_d_circ_dd(_x - _x, 0.0);
#endif  // !D_MATH_DBL_EVAL_WIDE
}


//==============================================================================
// 2.  THE WRAPPING
//==============================================================================


// 2.1    Into a turn, in radians
//------------------------------------------------------------------------------
// wrap_pi is the double nearest x - 2 pi n, n the integer nearest x / (2 pi):
// x's angle in [-pi, pi], correctly rounded at every finite x, however large
// (S513). pi is not a double, and the double nearest it lies below it, so no
// result passes it either way and no x's exact angle is pi; |x| <= pi is x
// itself. wrap_two_pi is the double nearest x - 2 pi n, n the greatest integer
// at most x / (2 pi): the angle in [0, 2 pi), which the double nearest 2 pi,
// below it, bounds too; x in [0, 2 pi) is x itself, a zero's sign kept, and x
// in (-2^-60, 0) that double. The angle is the reduction's r as a pair plus c
// pi/2, c from -2 to 4 as the quadrant and r's sign give it: within 2^-100 of
// the angle, relatively, and rounded where the interval that bound allows
// rounds one way, as Ziv's method has it; where it does not, the fixed path:
// (q + f) pi/2, its magnitude within 6.6 units of 2^-224 of the angle's, some
// 2^-160.4 of the angle relatively, which no double's angle is known to come
// near a rounding boundary by - of some 2^63 doubles, each might come some
// 2^-116 near, by count (S513). NaN and the infinities give NaN. A constant
// expression from C++14. Where double is evaluated wider than double, a 32-bit
// x87, every x takes the fixed path, which is integer arithmetic and exact
// conversions: the same bits, slower.

// d_internal_math_d_wrap_fixed
//   _x's angle by the fixed path: in [-pi, pi] (_upper false) or [0, 2 pi)
// (_upper true), rounded once.
D_MATH_FN double
d_internal_math_d_wrap_fixed(
    double _x,
    bool   _upper
)
{
    const struct dj_internal_math_d_fixed_angle a =
        d_internal_math_d_reduce_fixed(_x);
    struct dj_internal_math_d_exp_fixed whole = { { 0 } };
    struct dj_internal_math_d_exp_fixed s     = { { 0 } };
    // s = q + f as A plus or minus |f|, its sign negative; A from 0 to 4
    int  base     = a.quadrant;
    bool negative = false;
    bool add      = !a.negative;

    if (_upper)
    {
        // [0, 4): a negative f below quadrant 0 wraps to 4 - |f|
        if ( (a.quadrant == 0) && a.negative )
        {
            base = 4;
        }
    }
    else if (!a.negative)
    {
        // q + f in [2, 4): s - 4, negative, magnitude 4 - q - f
        if (a.quadrant >= 2)
        {
            base     = 4 - a.quadrant;
            negative = true;
            add      = false;
        }
    }
    else if (a.quadrant == 0)
    {
        negative = true;
        add      = true;
    }
    else if (a.quadrant == 3)
    {
        // 3 - |f| in (2.5, 3]: s - 4 = -(1 + |f|)
        base     = 1;
        negative = true;
        add      = true;
    }

    whole.limb[14] = (unsigned long)base;
    s = add ? d_internal_math_d_exp_fixed_add(whole, a.f)
            : d_internal_math_d_exp_fixed_sub(whole, a.f);

    {
        const double w = d_internal_math_d_circ_round(
            d_internal_math_d_exp_fixed_mul(
                s, d_internal_math_d_half_pi_fixed()));

        return negative ? -w : w;
    }
}

#if !D_MATH_DBL_EVAL_WIDE
// dj_internal_math_d_wrap_result
//   struct: a wrapping's result, where the pair path decided it.
struct dj_internal_math_d_wrap_result
{
    bool   decided;
    double value;
};

// d_internal_math_d_wrap_pair
//   _x's angle from its reduction as a pair: c pi/2 + r within 2^-100 of
// it, relatively, rounded where both ends of that interval round alike.
D_MATH_FN struct dj_internal_math_d_wrap_result
d_internal_math_d_wrap_pair(
    double _x,
    bool   _upper
)
{
    const struct dj_internal_math_d_angle a =
        ( (_x < 2147483648.0) && (_x > -2147483648.0) )
        ? d_internal_math_d_reduce_pair(_x)
        : d_internal_math_d_reduce_far(_x);
    struct dj_internal_math_d_wrap_result result = { false, 0.0 };
    struct dj_math_dd w = a.r;
    int c = a.quadrant;

    if (_upper)
    {
        c = ( (a.quadrant == 0) && (a.r.high < 0.0) ) ? 4 : a.quadrant;
    }
    else if (a.quadrant == 2)
    {
        c = (a.r.high < 0.0) ? 2 : -2;
    }
    else if (a.quadrant == 3)
    {
        c = -1;
    }

    if (c != 0)
    {
        w = d_math_dd_add(
                d_internal_math_d_circ_dd(
                    d_internal_math_d_half_pi_multiple(c, false),
                    d_internal_math_d_half_pi_multiple(c, true)),
                w);
    }

    {
        // 2^-100 of the result's high part, an exact product
        const double err  = ((w.high < 0.0) ? -w.high : w.high) *
                            (1.0 / 1267650600228229401496703205376.0);
        const double low  = w.high + (w.low - err);
        const double high = w.high + (w.low + err);

        if (low == high)
        {
            result.decided = true;
            result.value   = low;
        }
    }

    return result;
}
#endif  // !D_MATH_DBL_EVAL_WIDE

// d_internal_math_d_wrap
//   _x's angle, in [-pi, pi] or [0, 2 pi), _x finite and outside its turn.
D_MATH_FN double
d_internal_math_d_wrap(
    double _x,
    bool   _upper
)
{
#if !D_MATH_DBL_EVAL_WIDE
    const struct dj_internal_math_d_wrap_result one =
        d_internal_math_d_wrap_pair(_x, _upper);

    if (one.decided)
    {
        return one.value;
    }
#endif  // !D_MATH_DBL_EVAL_WIDE

    return d_internal_math_d_wrap_fixed(_x, _upper);
}

D_MATH_FN double
d_math_d_wrap_pi(
    double _x
)
{
    const double ax = (_x < 0.0) ? -_x : _x;

    if (!(ax <= DBL_MAX))
    {
        return _x - _x;
    }

    // pi's double, which is below pi
    if (ax <= @PI@)
    {
        return _x;
    }

    return d_internal_math_d_wrap(_x, false);
}

D_MATH_FN double
d_math_d_wrap_two_pi(
    double _x
)
{
    if (!( (_x <= DBL_MAX) && (_x >= -DBL_MAX) ))
    {
        return _x - _x;
    }

    // 2 pi's double, which is below 2 pi; a zero of either sign
    if ( ( (_x >= 0.0) && (_x <= @TWO_PI@) ) ||
         (_x == 0.0) )
    {
        return _x;
    }

    // above -2^-60, 2 pi + x rounds as 2 pi does, which lies 0.276 of an
    // ulp above its double: that double
    if ( (_x < 0.0) &&
         (_x > -(1.0 / 1152921504606846976.0)) )
    {
        return @TWO_PI@;
    }

    return d_internal_math_d_wrap(_x, true);
}


// 2.2    Into a turn, in degrees
//------------------------------------------------------------------------------
// wrap_180 is x's angle in (-180, 180]: remainder.h's exact remainder by 360,
// in [-180, 180], with -180 taken as 180, so that every angle has one value;
// exact, and a zero's sign kept. wrap_360 is the double nearest the angle in
// [0, 360): the exact remainder by 360 with x's sign (fmod), 360 more where
// it is negative, rounded once there; a result that rounds to 360 is the
// same angle as 0, and is +0. NaN and the infinities give NaN (S514). Each
// a constant expression from C++14.

D_MATH_FN double
d_math_d_wrap_180(
    double _x
)
{
    const double r = d_math_d_remainder_c(_x, 360.0);

    return (r == -180.0) ? 180.0 : r;
}

D_MATH_FN double
d_math_d_wrap_360(
    double _x
)
{
    const double r = d_math_d_fmod_c(_x, 360.0);

    if (r < 0.0)
    {
        const double w = r + 360.0;

        return (w == 360.0) ? 0.0 : w;
    }

    return r;
}
"""

CIRCULAR_TAIL = r"""

#endif  // DJINTERP_C_MATH_CIRCULAR_H
"""


def circular():
    consts, mults = circ_constants()
    two, half = circ_digits()
    from fractions import Fraction
    plo, phi = circ_pi_fraction()
    pio4 = exp_d_literal(*circ_round_both(plo / 4, phi / 4))
    lines = [
        "The double family's reduction by pi/2 and the wrapping of an angle into",
        "one turn (step 55, T4, ruling 15), the first part of the sine and",
        "cosine's work in 6a (chunk 69).",
        "  GENERATED by scripts/gen_c_math_float.py: edit the generator, not this",
        "file.",
        "",
        "  reduce_half_pi gives x = (4 j + q) pi/2 + r as q in [0, 4) and r as a",
        "pair of doubles within 2^-102 of itself, relatively, at every finite x,",
        "however near a multiple of pi/2: Cody and Waite's way below 2^31, Payne",
        "and Hanek's in fixed point past it (S511, S512). wrap_pi and",
        "wrap_two_pi give x's angle in [-pi, pi] and in [0, 2 pi), correctly",
        "rounded at every finite x (S513); wrap_180 and wrap_360 the same in",
        "degrees, the first exact (S514). Every function is double arithmetic",
        "and integers alone - no long double and no C library - and a constant",
        "expression from C++14, and its results are the same bits wherever it",
        "compiles; where double is evaluated wider than double, a 32-bit x87,",
        "the wraps take the fixed path, which needs no exact double sum, and",
        "reduce_half_pi refuses to compile, as exp_c does.",
    ]
    text = banner("circular.h", lines, "2026.11.27", "2026.11.27")
    text += "\n" + CIRCULAR_HEAD
    body = CIRCULAR_BODY
    body = body.replace("@TWO_OVER_PI@", circ_digits_macro(
        "D_INTERNAL_MATH_D_TWO_OVER_PI_DIGITS", two))
    body = body.replace("@HALF_PI@", circ_digits_macro(
        "D_INTERNAL_MATH_D_HALF_PI_DIGITS", half))
    for key, value in consts.items():
        body = body.replace("@%s@" % key, value)
    body = body.replace("@PIO4@", pio4)
    names = {-2: "M_2", -1: "M_1", 1: "M1", 2: "M2", 3: "M3", 4: "M4"}
    for c, (h, l) in zip(range(-2, 5), mults):
        if c == 0:
            continue
        body = body.replace("@%sH@" % names[c], h)
        body = body.replace("@%sL@" % names[c], l)
    assert "@" not in body.replace("@param", ""), "a placeholder left"
    text += body
    text += CIRCULAR_TAIL
    return text
