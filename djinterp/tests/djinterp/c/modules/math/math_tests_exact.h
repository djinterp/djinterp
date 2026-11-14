/*******************************************************************************
* djinterp [c/math]                                           math_tests_exact.h
*
*   The math harnesses' exact reference: signed integers of 256 bits or more,
* exact rationals over them, an oracle for the floating-point value nearest an
* exact ratio, and the exact order of a rational and a floating-point value.
* t_rational.c checks the rational kernels against it, t_value.c the value
* layer, t_value_boxed.c the value layer's boxed kinds, and t_value_wide.c
* the wide integer's kind.
*   The integers are sixteen limbs of 16 bits each, magnitude and sign, or
* as many as a harness sets in D_TESTS_EXACT_LIMBS before including this
* header: t_value_wide.c takes 48, 768 bits. The product of two limbs fits 32
* bits, so unsigned long holds every step, and the reference needs no type
* wider than 32 bits. It runs alike in every build, ISO strict C++98 on a
* 32-bit target included. A sum, a product or a shift that could pass the
* limbs sets g_reference_overrun; a harness checks at its end that it never
* was.
*   The oracle takes a value's neighbours from the C library's nextafter and
* its mantissa and exponent from frexp, not from the kernels, and compares
* distances exactly, scaled to integers.
*
*
* path:      /tests/djinterp/c/modules/math/math_tests_exact.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.11.07
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_MATH_MATH_TESTS_EXACT_H
#define DJINTERP_TESTS_C_MATH_MATH_TESTS_EXACT_H 1

// std
#include <float.h>   // FLT_MANT_DIG ... LDBL_MAX
#include <math.h>    // frexpf, frexp, frexpl, nextafterf, nextafter,
                     // nextafterl, ldexpl, floorl
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/rational.h"  // dj_rational, words


//==============================================================================
// 1.  INTEGERS OF 256 BITS OR MORE
//==============================================================================
// D_TESTS_LIMBS limbs of 16 bits, magnitude and sign, and exact rationals
// over them. No value may pass the limbs: a sum, a product or a shift that
// could sets g_reference_overrun, and a harness that finds it set fails.

// d_tests_magnitude
//   |_x| as a dj_math_umax, exact for every value, D_MATH_IMAX_MIN included.
static inline dj_math_umax
d_tests_magnitude(
    dj_math_imax _x
)
{
    return (_x < 0) ? ((dj_math_umax)0 - (dj_math_umax)_x)
                    : (dj_math_umax)_x;
}

// d_tests_text_umax
//   _x in decimal, into _buffer (at least 24 characters), returned.
static inline const char*
d_tests_text_umax(
    dj_math_umax _x,
    char*        _buffer
)
{
    char         digits[24];
    int          count = 0;
    int          at    = 0;
    dj_math_umax rest  = _x;

    // the digits, least significant first
    do
    {
        digits[count] = (char)('0' + (int)(rest % 10u));
        ++count;
        rest = rest / 10u;
    } while (rest != 0u);

    // then most significant first
    while (count > 0)
    {
        --count;
        _buffer[at] = digits[count];
        ++at;
    }

    _buffer[at] = '\0';

    return _buffer;
}

// d_tests_text
//   _x in decimal, its sign first, into _buffer (at least 24 characters),
// returned.
static inline const char*
d_tests_text(
    dj_math_imax _x,
    char*        _buffer
)
{
    // below zero: the sign, then the magnitude
    if (_x < 0)
    {
        _buffer[0] = '-';
        (void)d_tests_text_umax(d_tests_magnitude(_x), _buffer + 1);

        return _buffer;
    }

    return d_tests_text_umax(d_tests_magnitude(_x), _buffer);
}

// the reference's limbs: sixteen, 256 bits, unless the including harness
// defines D_TESTS_EXACT_LIMBS first, as t_value_wide.c does for 768
#ifndef D_TESTS_EXACT_LIMBS
    #define D_TESTS_EXACT_LIMBS  16
#endif  // D_TESTS_EXACT_LIMBS

#define D_TESTS_LIMBS      D_TESTS_EXACT_LIMBS
#define D_TESTS_LIMB_MASK  0xFFFFUL

// dj_tests_big
//   struct: a signed integer, its magnitude in D_TESTS_LIMBS limbs.
struct dj_tests_big
{
    unsigned long limb[D_TESTS_LIMBS];  // the magnitude, low limb first
    int           negative;             // 1 below zero; never for zero
};

// dj_tests_exact
//   struct: an exact rational n / d, d > 0, not necessarily reduced.
struct dj_tests_exact
{
    struct dj_tests_big n;  // the numerator, which carries the sign
    struct dj_tests_big d;  // the denominator, positive
};

// set when a value could pass the reference's limbs
static bool g_reference_overrun = false;

// d_tests_big_length
//   the limbs _x uses: those up to its highest nonzero one.
static inline int
d_tests_big_length(
    struct dj_tests_big _x
)
{
    int length = D_TESTS_LIMBS;

    // down past the zero limbs at the top
    while ( (length > 0) &&
            (_x.limb[length - 1] == 0UL) )
    {
        --length;
    }

    return length;
}

static inline bool
d_tests_big_is_zero(
    struct dj_tests_big _x
)
{
    return (d_tests_big_length(_x) == 0);
}

// d_tests_big_of_umax
//   _magnitude, negative where _negative and it is not 0.
static inline struct dj_tests_big
d_tests_big_of_umax(
    dj_math_umax _magnitude,
    bool         _negative
)
{
    struct dj_tests_big result = { { 0UL }, 0 };
    dj_math_umax        rest   = _magnitude;
    int                 i      = 0;

    // sixteen bits at a time, the low ones first
    for (i = 0; i < D_TESTS_LIMBS; ++i)
    {
        result.limb[i] = (unsigned long)(rest & D_TESTS_LIMB_MASK);
        rest           = rest >> 16;
    }

    result.negative = ( (_negative) &&
                        (_magnitude != 0u) ) ? 1 : 0;

    return result;
}

// d_tests_big_of
//   _value, exactly, D_MATH_IMAX_MIN included.
static inline struct dj_tests_big
d_tests_big_of(
    dj_math_imax _value
)
{
    return d_tests_big_of_umax(d_tests_magnitude(_value), (_value < 0));
}

// d_tests_big_neg
//   -_x; 0 stays 0, never negative.
static inline struct dj_tests_big
d_tests_big_neg(
    struct dj_tests_big _x
)
{
    struct dj_tests_big result = _x;

    result.negative = d_tests_big_is_zero(_x) ? 0 : (1 - _x.negative);

    return result;
}

static inline struct dj_tests_big
d_tests_big_abs(
    struct dj_tests_big _x
)
{
    struct dj_tests_big result = _x;

    result.negative = 0;

    return result;
}

// d_tests_big_cmp_mag
//   -1, 0 or 1 as |_a| is below, equal to or above |_b|.
static inline int
d_tests_big_cmp_mag(
    struct dj_tests_big _a,
    struct dj_tests_big _b
)
{
    int i = 0;

    // from the top limb down, the first that differs decides
    for (i = D_TESTS_LIMBS - 1; i >= 0; --i)
    {
        if (_a.limb[i] != _b.limb[i])
        {
            return (_a.limb[i] < _b.limb[i]) ? -1 : 1;
        }
    }

    return 0;
}

// d_tests_big_cmp
//   -1, 0 or 1 as _a is below, equal to or above _b.
static inline int
d_tests_big_cmp(
    struct dj_tests_big _a,
    struct dj_tests_big _b
)
{
    const int order = d_tests_big_cmp_mag(_a, _b);

    // unlike signs: the negative one is below (0 is never negative)
    if (_a.negative != _b.negative)
    {
        return (_a.negative != 0) ? -1 : 1;
    }

    return (_a.negative != 0) ? -order : order;
}

// d_tests_big_add
//   _a + _b: like signs add the magnitudes, unlike ones take the smaller
// from the larger; either way the larger's sign.
static inline struct dj_tests_big
d_tests_big_add(
    struct dj_tests_big _a,
    struct dj_tests_big _b
)
{
    const bool                like    = (_a.negative == _b.negative);
    const bool                a_more  = (d_tests_big_cmp_mag(_a, _b) >= 0);
    const struct dj_tests_big larger  = a_more ? _a : _b;
    const struct dj_tests_big smaller = a_more ? _b : _a;

    struct dj_tests_big result = { { 0UL }, 0 };
    unsigned long       carry  = 0UL;
    int                 i      = 0;

    // limb by limb, a carry or a borrow passed up
    for (i = 0; i < D_TESTS_LIMBS; ++i)
    {
        if (like)
        {
            const unsigned long step =
                larger.limb[i] + smaller.limb[i] + carry;

            result.limb[i] = step & D_TESTS_LIMB_MASK;
            carry          = step >> 16;
        }
        else
        {
            const unsigned long take = smaller.limb[i] + carry;

            carry          = (larger.limb[i] < take) ? 1UL : 0UL;
            result.limb[i] = (larger.limb[i] + (carry << 16)) - take;
        }
    }

    // a carry out of the top limb: past the reference's bits
    if ( (like) &&
         (carry != 0UL) )
    {
        g_reference_overrun = true;
    }

    result.negative = d_tests_big_is_zero(result) ? 0 : larger.negative;

    return result;
}

// d_tests_big_mul
//   _a * _b, schoolbook over the limbs in use.
static inline struct dj_tests_big
d_tests_big_mul(
    struct dj_tests_big _a,
    struct dj_tests_big _b
)
{
    const int a_length = d_tests_big_length(_a);
    const int b_length = d_tests_big_length(_b);

    struct dj_tests_big result = { { 0UL }, 0 };
    int                 i      = 0;
    int                 j      = 0;

    // an m-limb by an n-limb magnitude needs m + n limbs at most
    if ((a_length + b_length) > D_TESTS_LIMBS)
    {
        g_reference_overrun = true;

        return result;
    }

    // a row per limb of _a: a partial product, the limb already there and
    // the carry stay within 32 bits, (2^16 - 1)^2 + 2 (2^16 - 1) = 2^32 - 1
    for (i = 0; i < a_length; ++i)
    {
        unsigned long carry = 0UL;

        for (j = 0; j < b_length; ++j)
        {
            const unsigned long step =
                (_a.limb[i] * _b.limb[j]) + result.limb[i + j] + carry;

            result.limb[i + j] = step & D_TESTS_LIMB_MASK;
            carry              = step >> 16;
        }

        result.limb[i + b_length] = carry;
    }

    result.negative = ( (_a.negative != _b.negative) &&
                        (!d_tests_big_is_zero(result)) ) ? 1 : 0;

    return result;
}

// d_tests_big_halve
//   |_x| / 2, rounded down.
static inline struct dj_tests_big
d_tests_big_halve(
    struct dj_tests_big _x
)
{
    struct dj_tests_big result = { { 0UL }, 0 };
    int                 i      = 0;

    // each limb takes the low bit of the one above as its top bit
    for (i = 0; i < D_TESTS_LIMBS; ++i)
    {
        const unsigned long above = ((i + 1) < D_TESTS_LIMBS)
                                        ? (_x.limb[i + 1] & 1UL)
                                        : 0UL;

        result.limb[i] = (_x.limb[i] >> 1) | (above << 15);
    }

    return result;
}

// d_tests_big_twice
//   |_x| * 2.
static inline struct dj_tests_big
d_tests_big_twice(
    struct dj_tests_big _x
)
{
    struct dj_tests_big result = { { 0UL }, 0 };
    int                 i      = 0;

    // each limb takes the top bit of the one below as its low bit
    for (i = 0; i < D_TESTS_LIMBS; ++i)
    {
        const unsigned long below = (i > 0) ? (_x.limb[i - 1] >> 15) : 0UL;

        result.limb[i] = ((_x.limb[i] << 1) & D_TESTS_LIMB_MASK) | below;
    }

    // a bit out of the top limb: past the reference's bits
    if ((_x.limb[D_TESTS_LIMBS - 1] >> 15) != 0UL)
    {
        g_reference_overrun = true;
    }

    return result;
}

// d_tests_big_bits
//   the bits |_x| needs: 0 for 0.
static inline int
d_tests_big_bits(
    struct dj_tests_big _x
)
{
    const int     length = d_tests_big_length(_x);
    unsigned long top    = (length > 0) ? _x.limb[length - 1] : 0UL;
    int           count  = (length > 0) ? ((length - 1) * 16) : 0;

    // the top limb's bits
    while (top != 0UL)
    {
        top = top >> 1;
        ++count;
    }

    return count;
}

// d_tests_big_shl
//   |_x| * 2^_k, _k >= 0: whole limbs moved up, then the bits left over.
static inline struct dj_tests_big
d_tests_big_shl(
    struct dj_tests_big _x,
    int                 _k
)
{
    const int limbs  = _k / 16;
    const int bits   = _k % 16;
    const int length = d_tests_big_length(_x);

    struct dj_tests_big result = { { 0UL }, 0 };
    int                 i      = 0;

    // past the reference's bits
    if ((d_tests_big_bits(_x) + _k) > (16 * D_TESTS_LIMBS))
    {
        g_reference_overrun = true;

        return result;
    }

    // each limb to its place, its high bits into the next
    for (i = 0; i < length; ++i)
    {
        const unsigned long moved = _x.limb[i] << bits;

        result.limb[i + limbs] |= moved & D_TESTS_LIMB_MASK;

        // the part above sixteen bits, where there is room for it
        if ((i + limbs + 1) < D_TESTS_LIMBS)
        {
            result.limb[i + limbs + 1] |= moved >> 16;
        }
    }

    return result;
}

// d_tests_big_gcd
//   the greatest common divisor of |_a| and |_b|, both nonzero, by Stein's
// binary method: shifts and subtractions only, so the reference needs no
// division.
static inline struct dj_tests_big
d_tests_big_gcd(
    struct dj_tests_big _a,
    struct dj_tests_big _b
)
{
    struct dj_tests_big a    = d_tests_big_abs(_a);
    struct dj_tests_big b    = d_tests_big_abs(_b);
    int                 twos = 0;
    int                 i    = 0;

    // the factors of two they share, set aside
    while ( ((a.limb[0] & 1UL) == 0UL) &&
            ((b.limb[0] & 1UL) == 0UL) )
    {
        a = d_tests_big_halve(a);
        b = d_tests_big_halve(b);
        ++twos;
    }

    // a made odd, which it stays
    while ((a.limb[0] & 1UL) == 0UL)
    {
        a = d_tests_big_halve(a);
    }

    // b made odd, the smaller kept in a, and b replaced by the difference
    while (!d_tests_big_is_zero(b))
    {
        while ((b.limb[0] & 1UL) == 0UL)
        {
            b = d_tests_big_halve(b);
        }

        // a the smaller
        if (d_tests_big_cmp_mag(a, b) > 0)
        {
            const struct dj_tests_big larger = a;

            a = b;
            b = larger;
        }

        b = d_tests_big_add(b, d_tests_big_neg(a));
    }

    // the shared twos restored
    for (i = 0; i < twos; ++i)
    {
        a = d_tests_big_twice(a);
    }

    return a;
}

// d_tests_exact_from
//   _n / _d, as they are; _d positive.
static inline struct dj_tests_exact
d_tests_exact_from(
    struct dj_tests_big _n,
    struct dj_tests_big _d
)
{
    struct dj_tests_exact result = { _n, _d };

    return result;
}

// d_tests_exact_of
//   _num / _den, _den nonzero, the sign moved to the numerator.
static inline struct dj_tests_exact
d_tests_exact_of(
    dj_math_imax _num,
    dj_math_imax _den
)
{
    const struct dj_tests_big num = d_tests_big_of(_num);

    return d_tests_exact_from((_den < 0) ? d_tests_big_neg(num) : num,
                              d_tests_big_abs(d_tests_big_of(_den)));
}

static inline struct dj_tests_exact
d_tests_exact_neg(
    struct dj_tests_exact _x
)
{
    return d_tests_exact_from(d_tests_big_neg(_x.n), _x.d);
}

static inline struct dj_tests_exact
d_tests_exact_abs(
    struct dj_tests_exact _x
)
{
    return d_tests_exact_from(d_tests_big_abs(_x.n), _x.d);
}

// d_tests_exact_inv
//   1 / _x, _x nonzero: the terms swapped, the sign kept on the numerator.
static inline struct dj_tests_exact
d_tests_exact_inv(
    struct dj_tests_exact _x
)
{
    return d_tests_exact_from((_x.n.negative != 0) ? d_tests_big_neg(_x.d)
                                                   : _x.d,
                              d_tests_big_abs(_x.n));
}

// d_tests_exact_add
//   _x + _y, over the product of the denominators.
static inline struct dj_tests_exact
d_tests_exact_add(
    struct dj_tests_exact _x,
    struct dj_tests_exact _y
)
{
    return d_tests_exact_from(d_tests_big_add(d_tests_big_mul(_x.n, _y.d),
                                              d_tests_big_mul(_y.n, _x.d)),
                              d_tests_big_mul(_x.d, _y.d));
}

static inline struct dj_tests_exact
d_tests_exact_mul(
    struct dj_tests_exact _x,
    struct dj_tests_exact _y
)
{
    return d_tests_exact_from(d_tests_big_mul(_x.n, _y.n),
                              d_tests_big_mul(_x.d, _y.d));
}

// d_tests_exact_cmp
//   -1, 0 or 1 as _x is below, equal to or above _y: the cross products,
// the denominators being positive.
static inline int
d_tests_exact_cmp(
    struct dj_tests_exact _x,
    struct dj_tests_exact _y
)
{
    return d_tests_big_cmp(d_tests_big_mul(_x.n, _y.d),
                           d_tests_big_mul(_y.n, _x.d));
}

// d_tests_exact_fits
//   whether _x, reduced, lies within dj_math_imax. With g the greatest common
// divisor of |n| and d, the reduced terms are |n| / g and d / g, so without
// a division: d <= D_MATH_IMAX_MAX * g, and |n| <= D_MATH_IMAX_MAX * g, or
// (D_MATH_IMAX_MAX + 1) * g below zero.
static inline bool
d_tests_exact_fits(
    struct dj_tests_exact _x
)
{
    const dj_math_umax most   = (dj_math_umax)D_MATH_IMAX_MAX;
    const dj_math_umax n_most = (_x.n.negative != 0) ? (most + 1u) : most;

    // 0 is 0/1
    if (d_tests_big_is_zero(_x.n))
    {
        return true;
    }

    const struct dj_tests_big g = d_tests_big_gcd(_x.n, _x.d);
    const struct dj_tests_big d_bound =
        d_tests_big_mul(d_tests_big_of_umax(most, false), g);
    const struct dj_tests_big n_bound =
        d_tests_big_mul(d_tests_big_of_umax(n_most, false), g);

    return ( (d_tests_big_cmp_mag(_x.d, d_bound) <= 0) &&
             (d_tests_big_cmp_mag(_x.n, n_bound) <= 0) );
}

// d_tests_gcd
//   the greatest common divisor of two words: the harness's own Euclid.
static inline dj_math_umax
d_tests_gcd(
    dj_math_umax _a,
    dj_math_umax _b
)
{
    dj_math_umax a = _a;
    dj_math_umax b = _b;

    // the remainder replaces the larger until it vanishes
    while (b != 0u)
    {
        const dj_math_umax rest = a % b;

        a = b;
        b = rest;
    }

    return a;
}

// d_tests_in_one_form
//   a positive denominator and no common factor.
static inline bool
d_tests_in_one_form(
    struct dj_rational _x
)
{
    return ( (_x.den > 0) &&
             (d_tests_gcd(d_tests_magnitude(_x.num),
                          (dj_math_umax)_x.den) == 1u) );
}

// d_tests_big_of_wide
//   _x, hi * 2^w + lo, exactly.
static inline struct dj_tests_big
d_tests_big_of_wide(
    struct dj_internal_rational_wide _x
)
{
    const dj_math_umax top = (dj_math_umax)1
                                << (D_INTERNAL_RATIONAL_WORD_BITS - 1u);
    const struct dj_tests_big base =
        d_tests_big_twice(d_tests_big_of_umax(top, false));

    return d_tests_big_add(
        d_tests_big_mul(d_tests_big_of_umax(_x.hi, false), base),
        d_tests_big_of_umax(_x.lo, false));
}

// d_tests_big_of_product
//   _a * _b, exactly.
static inline struct dj_tests_big
d_tests_big_of_product(
    dj_math_umax _a,
    dj_math_umax _b
)
{
    return d_tests_big_mul(d_tests_big_of_umax(_a, false),
                           d_tests_big_of_umax(_b, false));
}


//==============================================================================
// 2.  THE NEAREST VALUE OF A TYPE
//==============================================================================
// A value r of a type is the one nearest an exact ratio, ties to even, when
// neither of r's neighbours is nearer, and on a tie, r's mantissa is even.
// The neighbours come from the C library's nextafter, and the mantissas and
// exponents from its frexp; the distances are then compared exactly, scaled
// to integers. For a ratio of two-word integers they reach 2^(P + 2w + 1), P
// the type's precision: 2^193 with 64-bit words and the x87's 64 bits, and
// 2^242 where a long double has 113 bits.

// dj_tests_parts
//   struct: a positive finite value, or 0, as m * 2^e, m an integer below
// 2^P.
struct dj_tests_parts
{
    struct dj_tests_big mantissa;  // m
    int                 exponent;  // e
};

// d_tests_parts_of
//   the parts of _fraction * 2^_exponent, as frexp gave them for a value of
// a type with _precision bits: _fraction is 0 or in [1/2, 1), so _fraction *
// 2^_precision is an integer, split here into limbs by exact divisions.
static inline struct dj_tests_parts
d_tests_parts_of(
    long double _fraction,
    int         _exponent,
    int         _precision
)
{
    struct dj_tests_parts result = { { { 0UL }, 0 }, 0 };
    long double           rest   = ldexpl(_fraction, _precision);
    int                   i      = 0;

    // sixteen bits at a time, the low ones first
    while ( (rest != 0.0L) &&
            (i < D_TESTS_LIMBS) )
    {
        const long double above = floorl(rest / 65536.0L);
        const long double limb  = rest - (above * 65536.0L);

        result.mantissa.limb[i] = (unsigned long)limb;
        rest                    = above;
        ++i;
    }

    result.exponent = _exponent - _precision;

    return result;
}

// d_tests_distance
//   |_n * 2^_s - m * _d * 2^(e + _s)|, for e + _s >= 0: how far _n / _d
// lies from m * 2^e, times _d * 2^_s.
static inline struct dj_tests_big
d_tests_distance(
    struct dj_tests_big   _n,
    struct dj_tests_big   _d,
    struct dj_tests_parts _v,
    int                   _s
)
{
    const struct dj_tests_big scaled = d_tests_big_shl(_n, _s);
    const struct dj_tests_big value  =
        d_tests_big_shl(d_tests_big_mul(_v.mantissa, _d), _v.exponent + _s);

    return d_tests_big_abs(d_tests_big_add(scaled, d_tests_big_neg(value)));
}

// d_tests_nearest
//   whether _r is the value of its type nearest _n / _d, both positive, ties
// to even: neither neighbour, _below nor _above, nearer, and on a tie with
// one of them, _r's mantissa even.
static inline bool
d_tests_nearest(
    struct dj_tests_big   _n,
    struct dj_tests_big   _d,
    struct dj_tests_parts _r,
    struct dj_tests_parts _below,
    struct dj_tests_parts _above
)
{
    const int lower = (_below.exponent < _r.exponent) ? _below.exponent
                                                      : _r.exponent;
    const int least = (_above.exponent < lower) ? _above.exponent : lower;
    const int scale = (least < 0) ? -least : 0;

    const struct dj_tests_big at = d_tests_distance(_n, _d, _r, scale);
    const int to_below = d_tests_big_cmp_mag(
        at, d_tests_distance(_n, _d, _below, scale));
    const int to_above = d_tests_big_cmp_mag(
        at, d_tests_distance(_n, _d, _above, scale));
    const bool even = ((_r.mantissa.limb[0] & 1UL) == 0UL);

    return ( (to_below <= 0) &&
             (to_above <= 0) &&
             ( ( (to_below < 0) &&
                 (to_above < 0) ) ||
               (even) ) );
}

// d_tests_nearest_f
//   whether _r is the float nearest -(_n / _d) where _negative, else
// _n / _d; _n and _d positive.
static inline bool
d_tests_nearest_f(
    bool                _negative,
    struct dj_tests_big _n,
    struct dj_tests_big _d,
    float               _r
)
{
    const float magnitude = _negative ? -_r : _r;
    int         e_r       = 0;
    int         e_below   = 0;
    int         e_above   = 0;

    // positive and finite: the sign right, and a value at all
    if ( (!(magnitude > 0.0f)) ||
         (!(magnitude <= FLT_MAX)) )
    {
        return false;
    }

    const float below   = nextafterf(magnitude, 0.0f);
    const float above   = nextafterf(magnitude, FLT_MAX);
    const float f_r     = frexpf(magnitude, &e_r);
    const float f_below = frexpf(below, &e_below);
    const float f_above = frexpf(above, &e_above);

    return d_tests_nearest(_n, _d,
                           d_tests_parts_of(f_r, e_r, FLT_MANT_DIG),
                           d_tests_parts_of(f_below, e_below, FLT_MANT_DIG),
                           d_tests_parts_of(f_above, e_above, FLT_MANT_DIG));
}

// d_tests_nearest_d
//   the same for a double.
static inline bool
d_tests_nearest_d(
    bool                _negative,
    struct dj_tests_big _n,
    struct dj_tests_big _d,
    double              _r
)
{
    const double magnitude = _negative ? -_r : _r;
    int          e_r       = 0;
    int          e_below   = 0;
    int          e_above   = 0;

    // positive and finite: the sign right, and a value at all
    if ( (!(magnitude > 0.0)) ||
         (!(magnitude <= DBL_MAX)) )
    {
        return false;
    }

    const double below   = nextafter(magnitude, 0.0);
    const double above   = nextafter(magnitude, DBL_MAX);
    const double f_r     = frexp(magnitude, &e_r);
    const double f_below = frexp(below, &e_below);
    const double f_above = frexp(above, &e_above);

    return d_tests_nearest(_n, _d,
                           d_tests_parts_of(f_r, e_r, DBL_MANT_DIG),
                           d_tests_parts_of(f_below, e_below, DBL_MANT_DIG),
                           d_tests_parts_of(f_above, e_above, DBL_MANT_DIG));
}

// d_tests_nearest_ld
//   the same for a long double.
static inline bool
d_tests_nearest_ld(
    bool                _negative,
    struct dj_tests_big _n,
    struct dj_tests_big _d,
    long double         _r
)
{
    const long double magnitude = _negative ? -_r : _r;
    int               e_r       = 0;
    int               e_below   = 0;
    int               e_above   = 0;

    // positive and finite: the sign right, and a value at all
    if ( (!(magnitude > 0.0L)) ||
         (!(magnitude <= LDBL_MAX)) )
    {
        return false;
    }

    const long double below   = nextafterl(magnitude, 0.0L);
    const long double above   = nextafterl(magnitude, LDBL_MAX);
    const long double f_r     = frexpl(magnitude, &e_r);
    const long double f_below = frexpl(below, &e_below);
    const long double f_above = frexpl(above, &e_above);

    return d_tests_nearest(_n, _d,
                           d_tests_parts_of(f_r, e_r, LDBL_MANT_DIG),
                           d_tests_parts_of(f_below, e_below, LDBL_MANT_DIG),
                           d_tests_parts_of(f_above, e_above, LDBL_MANT_DIG));
}


//==============================================================================
// 3.  A RATIONAL AGAINST A FLOATING-POINT VALUE
//==============================================================================

// d_tests_exact_cmp_real
//   the order of the exact number _r against the finite or infinite _f: -1,
// 0 or 1 as _r is below, equal to or above it, and 2 where _f is a NaN.
// Past 2^w, or below 2^(-w), a nonzero _f is beyond every nonzero rational's
// magnitude, or within; between, _r = n/d against f = m * 2^e (frexpl's
// mantissa and exponent), as n * 2^s against m * d * 2^(e + s), exactly.
static inline int
d_tests_exact_cmp_real(
    struct dj_rational _r,
    long double        _f
)
{
    const int width  = (int)D_INTERNAL_RATIONAL_WORD_BITS;
    const int r_sign = (_r.num > 0) ? 1 : ((_r.num < 0) ? -1 : 0);
    const int f_sign = (_f > 0.0L) ? 1 : ((_f < 0.0L) ? -1 : 0);

    struct dj_tests_big left     = { { 0UL }, 0 };
    struct dj_tests_big right    = { { 0UL }, 0 };
    int                 exponent = 0;
    int                 order    = 0;
    long double         fraction = 0.0L;

    // NaN
    if (_f != _f)
    {
        return 2;
    }

    // a zero, or unlike signs
    if ( (r_sign == 0) ||
         (f_sign == 0) ||
         (r_sign != f_sign) )
    {
        return (r_sign < f_sign) ? -1 : ((r_sign > f_sign) ? 1 : 0);
    }

    // an infinity, or a magnitude past 2^w: beyond _r
    if ( (_f > LDBL_MAX) ||
         (_f < -LDBL_MAX) )
    {
        return (f_sign > 0) ? -1 : 1;
    }

    fraction = frexpl((_f < 0.0L) ? -_f : _f, &exponent);

    // |f| at least 2^w: beyond |_r|; below 2^(-w): within it
    if (exponent > width)
    {
        return (f_sign > 0) ? -1 : 1;
    }

    if (exponent <= -width)
    {
        return (r_sign > 0) ? 1 : -1;
    }

    // exactly: |n| * 2^s against m * d * 2^(e + s)
    {
        const struct dj_tests_parts parts =
            d_tests_parts_of(fraction, exponent, LDBL_MANT_DIG);
        const int shift = (parts.exponent < 0) ? -parts.exponent : 0;

        left  = d_tests_big_shl(
            d_tests_big_of_umax(d_tests_magnitude(_r.num), false), shift);
        right = d_tests_big_shl(
            d_tests_big_mul(parts.mantissa,
                            d_tests_big_of_umax((dj_math_umax)_r.den, false)),
            parts.exponent + shift);
        order = d_tests_big_cmp_mag(left, right);
    }

    // the magnitudes' order, turned for two negatives
    order = (r_sign > 0) ? order : -order;

    return (order < 0) ? -1 : ((order > 0) ? 1 : 0);
}


#endif  // DJINTERP_TESTS_C_MATH_MATH_TESTS_EXACT_H
