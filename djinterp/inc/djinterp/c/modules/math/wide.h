/*******************************************************************************
* djinterp [c/math]                                                       wide.h
*
* Fixed-width wide integers: signed integers of 256 bits, kept in limbs, with
* exact arithmetic and no allocation -- the C core of the math module's wide
* integers (roadmap P1).
*   A wide integer is a magnitude below 2^256 and a sign, so its range is
* symmetric, -(2^256 - 1) to 2^256 - 1: negation and the absolute value never
* fail, and 0, which is never negative, has one form. Every kernel takes wide
* integers in that form and gives one back; d_math_wide_is_valid tests for
* it.
*
*   EXACT OR NOTHING, as in rational.h. A kernel either stores the exact
* result and returns D_MATH_WIDE_OK, or stores nothing and says why: the
* result does not fit its destination (D_MATH_WIDE_OVERFLOW), a nonzero value
* is divided by zero (D_MATH_WIDE_POLE) or zero by zero (D_MATH_WIDE_INVALID),
* a rational is no integer (D_MATH_WIDE_FRACTION), or a text is no integer
* (D_MATH_WIDE_SYNTAX). Division truncates, as C's does: the quotient toward
* zero, and the remainder with the dividend's sign.
*
*   LIMBS. The magnitude is kept in limbs of half a word, dj_math_umax, the low
* limb first. The product of two limbs, plus two limbs more, fits a word, so
* every step of a product or a quotient is a word's arithmetic. A limb is 32
* bits where the word is 64, and 16 bits in ISO strict C++98 on a 32-bit
* target, where the word is 32.
*   Below the type are kernels on limbs, each told its operands' lengths, in
* the style of GMP's mpn layer: comparison, sums, products, shifts, division
* by one limb and by several (Knuth's algorithm D, TAOCP 4.3.1), and rounding
* to a floating-point type's precision. They serve any length: the
* intermediates of twice the width that the boxed kind will need (step 9's
* third part), and a wider integer later, are written over them.
*
*   ONE TEXT, BOTH LANGUAGES. As in the rest of the C core, every kernel is
* pure arithmetic (D_MATH_FN): a header inline in C and below C++14, and a
* constant expression from C++14, the ones that store through a pointer
* included, where it points at the evaluation's own object. Only
* d_math_wide_to_text, which writes into a buffer, is not (D_MATH_FN_RT).
*
*   TO FLOATING POINT. d_math_wide_to_f, _to_d and _to_ld give the nearest
* float, double or long double, ties to even, whatever the environment's
* rounding mode, and, where asked, which way the rounding went. The rounding
* is done once, in integers, and the result then built exactly, as
* rational.h builds its own. A double's range and a long double's hold every
* wide integer; a float's does not, and past it the result is an infinity.
*
*   TEXT. Decimal both ways: an optional sign, then digits. A wide integer's
* text needs at most D_MATH_WIDE_TEXT_SIZE characters, its NUL among them.
*
*
* path:      /inc/djinterp/c/modules/math/wide.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.11.07
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LAYOUT AND STATUS
    -----------------
    1.  Limbs
         1.  dj_math_wide_limb
         2.  D_MATH_WIDE_LIMB_BITS
         3.  D_MATH_WIDE_BITS
         4.  D_MATH_WIDE_LIMBS
    2.  Layout
         1.  dj_math_wide
    3.  Status
         1.  dj_math_wide_status
    4.  Text
         1.  D_MATH_WIDE_TEXT_SIZE
2.  LIMB KERNELS
    ------------
    1.  Words and limbs
         1.  D_INTERNAL_MATH_WIDE_LIMB_MASK
         2.  D_INTERNAL_MATH_WIDE_WORD_LIMBS
         3.  D_INTERNAL_MATH_WIDE_TEN_POWER
         4.  D_INTERNAL_MATH_WIDE_TEN_DIGITS
    2.  Counting
    3.  Comparison
    4.  Sums
    5.  Products
    6.  Shifts
    7.  Quotients
    8.  Rounding
3.  CONSTRUCTION
    ------------
    1.  d_math_wide_from_umax, d_math_wide_from_imax
    2.  d_math_wide_is_valid
4.  ARITHMETIC
    ----------
    1.  Signs
    2.  Sums
    3.  Products
    4.  Quotients
    5.  d_math_wide_gcd
5.  COMPARISON
    ----------
    1.  d_math_wide_sign, d_math_wide_bits
    2.  d_math_wide_eq, d_math_wide_cmp
6.  CONVERSIONS
    -----------
    1.  The family's integers
    2.  Rationals
    3.  To floating point
7.  TEXT
    ----
    1.  d_math_wide_from_text
    2.  d_math_wide_to_text
*/

#ifndef DJINTERP_C_MATH_WIDE_H
#define DJINTERP_C_MATH_WIDE_H 1

// std
#include <float.h>           // FLT_MANT_DIG ... LDBL_MAX_EXP
#include <limits.h>          // CHAR_BIT
#include <math.h>            // HUGE_VAL
#include <stddef.h>          // size_t, NULL
// djinterp
#include "./math_common.h"   // D_MATH_FN, D_MATH_FN_RT, dj_math_imax,
                             // dj_math_umax, D_MATH_UMAX_MAX, D_MATH_IMAX_MAX
#include "./rational.h"      // dj_rational, the two-word helpers,
                             // dj_internal_rational_rounded, _build_f ... _ld
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // uint16_t, uint32_t,
                                                   // INTMAX_MAX


//==============================================================================
// 1.  LAYOUT AND STATUS
//==============================================================================


// 1.1    Limbs
//------------------------------------------------------------------------------
// 1.1.1
// dj_math_wide_limb
//   type: a limb, half a word: uint32_t where the word is 64 bits, which is
// where dstdint.h declares intmax_t; uint16_t where it is 32.
// 1.1.2
// D_MATH_WIDE_LIMB_BITS
//   constant: the bits in a limb.
#if defined(INTMAX_MAX)
    typedef uint32_t                dj_math_wide_limb;
    #define D_MATH_WIDE_LIMB_BITS   32u
#else
    typedef uint16_t                dj_math_wide_limb;
    #define D_MATH_WIDE_LIMB_BITS   16u
#endif

// 1.1.3
// D_MATH_WIDE_BITS
//   constant: the bits in a wide integer's magnitude.
#define D_MATH_WIDE_BITS            256u

// 1.1.4
// D_MATH_WIDE_LIMBS
//   constant: the limbs in a wide integer's magnitude: 8, or 16 where a limb
// is 16 bits.
#define D_MATH_WIDE_LIMBS           (D_MATH_WIDE_BITS / D_MATH_WIDE_LIMB_BITS)

D_STATIC_ASSERT( ((sizeof(dj_math_wide_limb) * 2u) == sizeof(dj_math_umax)),
                 "wide.h: a limb must be half a word" );
D_STATIC_ASSERT( ((sizeof(dj_math_wide_limb) * CHAR_BIT) ==
                  D_MATH_WIDE_LIMB_BITS),
                 "wide.h: a limb must have D_MATH_WIDE_LIMB_BITS bits" );

// 1.2    Layout
//------------------------------------------------------------------------------
// 1.2.1
// dj_math_wide
//   struct: a signed integer of 256 bits: its magnitude in limbs, the low
// limb first, and its sign. 0 is never negative.
struct dj_math_wide
{
    dj_math_wide_limb limb[D_MATH_WIDE_LIMBS];  // |x|, the low limb first
    bool              negative;                 // below zero; never for 0
};

// 1.3    Status
//------------------------------------------------------------------------------
// 1.3.1
// dj_math_wide_status
//   enum: what a kernel came to. Only D_MATH_WIDE_OK stores a result; the
// others leave every destination as it was. The first four are rational.h's
// own, by number too.
enum dj_math_wide_status
{
    D_MATH_WIDE_OK       = 0,  // the exact result, stored
    D_MATH_WIDE_OVERFLOW = 1,  // a result its destination cannot hold
    D_MATH_WIDE_POLE     = 2,  // a nonzero value divided by zero
    D_MATH_WIDE_INVALID  = 3,  // zero divided by zero
    D_MATH_WIDE_FRACTION = 4,  // a rational that is no integer
    D_MATH_WIDE_SYNTAX   = 5   // a text that is no integer
};

// 1.4    Text
//------------------------------------------------------------------------------
// 1.4.1
// D_MATH_WIDE_TEXT_SIZE
//   constant: the characters a wide integer's text needs at most, its NUL
// among them: 2^256 - 1 has 78 digits, and a sign may come before them.
#define D_MATH_WIDE_TEXT_SIZE       80u


//==============================================================================
// 2.  LIMB KERNELS
//==============================================================================
// Kernels on limbs, each given its operands' lengths: an integer is _n limbs,
// the low one first, and its high limbs may be 0. Where a kernel gives an
// integer back, it writes it into a destination of the length it says; the
// destination may be an operand where the kernel says so, and is otherwise
// apart from them. Each step is a word's arithmetic: a limb, widened to a
// word, times another, plus two more, is below 2^w, w the bits in a word.


// 2.1    Words and limbs
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_MATH_WIDE_LIMB_MASK
//   constant: a limb's bits, all ones, as a word.
#define D_INTERNAL_MATH_WIDE_LIMB_MASK                                         \
    (D_MATH_UMAX_MAX >> D_MATH_WIDE_LIMB_BITS)

// 2.1.2
// D_INTERNAL_MATH_WIDE_WORD_LIMBS
//   constant: the limbs in a word.
#define D_INTERNAL_MATH_WIDE_WORD_LIMBS  2u

// 2.1.3
// D_INTERNAL_MATH_WIDE_TEN_POWER
//   constant: the greatest power of ten below a limb's range, by which a
// wide integer's text is divided off: 10^9, or 10^4 where a limb is 16 bits.
// 2.1.4
// D_INTERNAL_MATH_WIDE_TEN_DIGITS
//   constant: its zeros, the digits each such division gives.
#if defined(INTMAX_MAX)
    #define D_INTERNAL_MATH_WIDE_TEN_POWER   1000000000u
    #define D_INTERNAL_MATH_WIDE_TEN_DIGITS  9u
#else
    #define D_INTERNAL_MATH_WIDE_TEN_POWER   10000u
    #define D_INTERNAL_MATH_WIDE_TEN_DIGITS  4u
#endif

// 2.2    Counting
//------------------------------------------------------------------------------

// d_internal_math_wide_length
//   the limbs _x needs, its _n limbs less the high ones that are 0: 0 for 0.
D_MATH_FN size_t
d_internal_math_wide_length(
    const dj_math_wide_limb* _x,
    size_t                   _n
)
{
    size_t n = _n;

    // the high limbs that are 0
    while ( (n > 0u) &&
            (_x[n - 1u] == 0u) )
    {
        --n;
    }

    return n;
}

// d_internal_math_wide_bits
//   the bits _x needs: 0 for 0, otherwise its top bit's place plus one.
D_MATH_FN size_t
d_internal_math_wide_bits(
    const dj_math_wide_limb* _x,
    size_t                   _n
)
{
    const size_t n = d_internal_math_wide_length(_x, _n);

    dj_math_umax top   = 0u;
    size_t       count = 0u;

    // 0
    if (n == 0u)
    {
        return 0u;
    }

    top   = (dj_math_umax)_x[n - 1u];
    count = (n - 1u) * (size_t)D_MATH_WIDE_LIMB_BITS;

    // the top limb's own bits, one at a time
    while (top != 0u)
    {
        top = top >> 1;
        ++count;
    }

    return count;
}

// 2.3    Comparison
//------------------------------------------------------------------------------

// d_internal_math_wide_cmp
//   _a against _b, _n limbs each: -1, 0 or 1 as _a is below, equal to or
// above _b. The highest limb that differs decides.
D_MATH_FN int
d_internal_math_wide_cmp(
    const dj_math_wide_limb* _a,
    const dj_math_wide_limb* _b,
    size_t                   _n
)
{
    size_t i = _n;

    // from the highest limb down
    while (i > 0u)
    {
        --i;

        if (_a[i] != _b[i])
        {
            return (_a[i] < _b[i]) ? -1 : 1;
        }
    }

    return 0;
}

// 2.4    Sums
//------------------------------------------------------------------------------

// d_internal_math_wide_add
//   _a + _b, _n limbs each, into _out's _n limbs, which may be either
// operand's; returns the carry out of the top limb, 0 or 1.
D_MATH_FN dj_math_wide_limb
d_internal_math_wide_add(
    dj_math_wide_limb*       _out,
    const dj_math_wide_limb* _a,
    const dj_math_wide_limb* _b,
    size_t                   _n
)
{
    dj_math_umax carry = 0u;
    size_t       i     = 0u;

    // limb by limb, the carry passed up
    for (i = 0u; i < _n; ++i)
    {
        const dj_math_umax sum =
            (dj_math_umax)_a[i] + (dj_math_umax)_b[i] + carry;

        _out[i] = (dj_math_wide_limb)(sum & D_INTERNAL_MATH_WIDE_LIMB_MASK);
        carry   = sum >> D_MATH_WIDE_LIMB_BITS;
    }

    return (dj_math_wide_limb)carry;
}

// d_internal_math_wide_sub
//   _a - _b, _n limbs each, into _out's _n limbs, which may be either
// operand's; returns the borrow out of the top limb, 0 or 1. Where it is 1,
// _out holds _a - _b + 2^(n h), h the bits in a limb.
D_MATH_FN dj_math_wide_limb
d_internal_math_wide_sub(
    dj_math_wide_limb*       _out,
    const dj_math_wide_limb* _a,
    const dj_math_wide_limb* _b,
    size_t                   _n
)
{
    dj_math_umax borrow = 0u;
    size_t       i      = 0u;

    // limb by limb, the borrow passed up; a word's wrap, cut to a limb, is
    // the difference modulo the limb's range
    for (i = 0u; i < _n; ++i)
    {
        const dj_math_umax have = (dj_math_umax)_a[i];
        const dj_math_umax take = (dj_math_umax)_b[i] + borrow;

        _out[i] = (dj_math_wide_limb)((have - take) &
                                      D_INTERNAL_MATH_WIDE_LIMB_MASK);
        borrow  = (have < take) ? 1u : 0u;
    }

    return (dj_math_wide_limb)borrow;
}

// 2.5    Products
//------------------------------------------------------------------------------

// d_internal_math_wide_mul_1
//   _a * _m + _carry, _a of _n limbs, into _out's _n limbs, which may be
// _a's; returns the limb carried out of the top.
D_MATH_FN dj_math_wide_limb
d_internal_math_wide_mul_1(
    dj_math_wide_limb*       _out,
    const dj_math_wide_limb* _a,
    size_t                   _n,
    dj_math_wide_limb        _m,
    dj_math_wide_limb        _carry
)
{
    dj_math_umax carry = (dj_math_umax)_carry;
    size_t       i     = 0u;

    // limb by limb: each product, plus the carry, within a word
    for (i = 0u; i < _n; ++i)
    {
        const dj_math_umax part =
            ((dj_math_umax)_a[i] * (dj_math_umax)_m) + carry;

        _out[i] = (dj_math_wide_limb)(part & D_INTERNAL_MATH_WIDE_LIMB_MASK);
        carry   = part >> D_MATH_WIDE_LIMB_BITS;
    }

    return (dj_math_wide_limb)carry;
}

// d_internal_math_wide_mul
//   _a * _b, of _na and _nb limbs, into _out's _na + _nb limbs, apart from
// both operands: the schoolbook product, a row for each of _a's limbs, each
// step a limb times a limb, plus the row's carry and the limb already there,
// within a word.
D_MATH_FN void
d_internal_math_wide_mul(
    dj_math_wide_limb*       _out,
    const dj_math_wide_limb* _a,
    size_t                   _na,
    const dj_math_wide_limb* _b,
    size_t                   _nb
)
{
    size_t i = 0u;
    size_t j = 0u;

    for (i = 0u; i < (_na + _nb); ++i)
    {
        _out[i] = 0u;
    }

    // a row for each of _a's limbs, added in at its place
    for (i = 0u; i < _na; ++i)
    {
        const dj_math_umax factor = (dj_math_umax)_a[i];

        dj_math_umax carry = 0u;

        for (j = 0u; j < _nb; ++j)
        {
            const dj_math_umax part = (factor * (dj_math_umax)_b[j]) +
                                      (dj_math_umax)_out[i + j] + carry;

            _out[i + j] = (dj_math_wide_limb)(part &
                                              D_INTERNAL_MATH_WIDE_LIMB_MASK);
            carry       = part >> D_MATH_WIDE_LIMB_BITS;
        }

        // the row's last carry: no earlier row reached this limb
        _out[i + _nb] = (dj_math_wide_limb)carry;
    }

    return;
}

// 2.6    Shifts
//------------------------------------------------------------------------------

// d_internal_math_wide_shl
//   _a * 2^_s, _a of _n limbs and _s below a limb's bits, into _out's _n
// limbs, which may be _a's; returns the bits shifted out of the top limb.
D_MATH_FN dj_math_wide_limb
d_internal_math_wide_shl(
    dj_math_wide_limb*       _out,
    const dj_math_wide_limb* _a,
    size_t                   _n,
    unsigned                 _s
)
{
    dj_math_umax carry = 0u;
    size_t       i     = 0u;

    // from the low limb up, each limb's top bits carried into the next
    for (i = 0u; i < _n; ++i)
    {
        const dj_math_umax part = ((dj_math_umax)_a[i] << _s) | carry;

        _out[i] = (dj_math_wide_limb)(part & D_INTERNAL_MATH_WIDE_LIMB_MASK);
        carry   = part >> D_MATH_WIDE_LIMB_BITS;
    }

    return (dj_math_wide_limb)carry;
}

// d_internal_math_wide_shr
//   _a / 2^_s, rounded down, _a of _n limbs and _s below a limb's bits, into
// _out's _n limbs, which may be _a's.
D_MATH_FN void
d_internal_math_wide_shr(
    dj_math_wide_limb*       _out,
    const dj_math_wide_limb* _a,
    size_t                   _n,
    unsigned                 _s
)
{
    size_t i = 0u;

    // from the low limb up, each limb with the next above it
    for (i = 0u; i < _n; ++i)
    {
        const dj_math_umax high = ((i + 1u) < _n) ? (dj_math_umax)_a[i + 1u]
                                                  : 0u;
        const dj_math_umax pair = (high << D_MATH_WIDE_LIMB_BITS) |
                                  (dj_math_umax)_a[i];

        _out[i] = (dj_math_wide_limb)((pair >> _s) &
                                      D_INTERNAL_MATH_WIDE_LIMB_MASK);
    }

    return;
}

// 2.7    Quotients
//------------------------------------------------------------------------------

// d_internal_math_wide_divmod_1
//   _a / _d, _a of _n limbs and _d a nonzero limb: the quotient into _q's _n
// limbs, which may be _a's, and the remainder returned. Each step divides
// the remainder so far, which is below _d, and the next limb, within a word.
D_MATH_FN dj_math_wide_limb
d_internal_math_wide_divmod_1(
    dj_math_wide_limb*       _q,
    const dj_math_wide_limb* _a,
    size_t                   _n,
    dj_math_wide_limb        _d
)
{
    dj_math_umax rest = 0u;
    size_t       i    = _n;

    // from the highest limb down
    while (i > 0u)
    {
        --i;

        const dj_math_umax part = (rest << D_MATH_WIDE_LIMB_BITS) |
                                  (dj_math_umax)_a[i];

        _q[i] = (dj_math_wide_limb)(part / (dj_math_umax)_d);
        rest  = part % (dj_math_umax)_d;
    }

    return (dj_math_wide_limb)rest;
}

// d_internal_math_wide_divmod
//   _a / _b, of _na and _nb limbs, _b's top limb nonzero and _nb at most _na:
// the quotient into _q's _na - _nb + 1 limbs, the remainder into _r's _nb
// limbs. _work, _na + _nb + 1 limbs, holds the operands normalized, so _q
// and _r may each be an operand's limbs, but neither the other's nor
// _work's.
//   Knuth's algorithm D (TAOCP 4.3.1), unsigned throughout. Both operands
// are shifted until the divisor's top bit is set; then each quotient limb
// is guessed from the dividend's top two limbs and the divisor's top one.
// The guess is at most two too large, and the divisor's second limb takes
// away nearly every excess (Knuth's step D3); the guess times the divisor is
// then taken from the dividend, and where that goes below zero, the guess
// was one too large still, and the divisor is added back (D6).
D_MATH_FN void
d_internal_math_wide_divmod(
    dj_math_wide_limb*       _q,
    dj_math_wide_limb*       _r,
    const dj_math_wide_limb* _a,
    size_t                   _na,
    const dj_math_wide_limb* _b,
    size_t                   _nb,
    dj_math_wide_limb*       _work
)
{
    const dj_math_umax base = D_INTERNAL_MATH_WIDE_LIMB_MASK + 1u;

    dj_math_wide_limb* u     = _work;              // _a, _na + 1 limbs
    dj_math_wide_limb* v     = _work + _na + 1u;   // _b, _nb limbs
    dj_math_umax       top   = 0u;
    unsigned           shift = 0u;
    size_t             i     = 0u;
    size_t             j     = 0u;

    // a divisor of one limb: the short division
    if (_nb == 1u)
    {
        _r[0] = d_internal_math_wide_divmod_1(_q, _a, _na, _b[0]);

        return;
    }

    // normalized: the divisor shifted until its top bit is set, and the
    // dividend alike, into a limb more
    top = (dj_math_umax)_b[_nb - 1u];

    while ((top & (base >> 1)) == 0u)
    {
        top = top << 1;
        ++shift;
    }

    (void)d_internal_math_wide_shl(v, _b, _nb, shift);
    u[_na] = d_internal_math_wide_shl(u, _a, _na, shift);

    // each quotient limb, the highest first
    j = _na - _nb + 1u;

    while (j > 0u)
    {
        --j;

        const dj_math_umax top_two =
            ((dj_math_umax)u[j + _nb] << D_MATH_WIDE_LIMB_BITS) |
            (dj_math_umax)u[j + _nb - 1u];

        dj_math_umax guess  = top_two / (dj_math_umax)v[_nb - 1u];
        dj_math_umax rest   = top_two % (dj_math_umax)v[_nb - 1u];
        dj_math_umax carry  = 0u;
        dj_math_umax borrow = 0u;
        dj_math_umax have   = 0u;
        dj_math_umax take   = 0u;

        // too large: a limb's range or more, or past what the divisor's
        // second limb allows; the test needs rest below a limb's range
        while ( (guess >= base) ||
                ( (guess * (dj_math_umax)v[_nb - 2u]) >
                  ((rest << D_MATH_WIDE_LIMB_BITS) |
                   (dj_math_umax)u[j + _nb - 2u]) ) )
        {
            --guess;
            rest = rest + (dj_math_umax)v[_nb - 1u];

            if (rest >= base)
            {
                break;
            }
        }

        // the guess times the divisor, taken from the dividend's limbs j to
        // j + _nb
        for (i = 0u; i < _nb; ++i)
        {
            const dj_math_umax part = (guess * (dj_math_umax)v[i]) + carry;

            have     = (dj_math_umax)u[i + j];
            take     = (part & D_INTERNAL_MATH_WIDE_LIMB_MASK) + borrow;
            carry    = part >> D_MATH_WIDE_LIMB_BITS;
            u[i + j] = (dj_math_wide_limb)((have - take) &
                                           D_INTERNAL_MATH_WIDE_LIMB_MASK);
            borrow   = (have < take) ? 1u : 0u;
        }

        // the top limb: below zero where more is taken than it holds. What
        // is left is below the divisor, so that limb is 0 after, and no step
        // reads it again: the next works a limb lower, and the remainder is
        // the low _nb limbs
        have = (dj_math_umax)u[j + _nb];
        take = carry + borrow;

        // below zero: one too large still, and the divisor added back; its
        // carry out cancels the borrow
        if (have < take)
        {
            --guess;
            carry = 0u;

            for (i = 0u; i < _nb; ++i)
            {
                const dj_math_umax sum = (dj_math_umax)u[i + j] +
                                         (dj_math_umax)v[i] + carry;

                u[i + j] = (dj_math_wide_limb)(sum &
                                               D_INTERNAL_MATH_WIDE_LIMB_MASK);
                carry    = sum >> D_MATH_WIDE_LIMB_BITS;
            }
        }

        _q[j] = (dj_math_wide_limb)guess;
    }

    // the remainder, below the divisor, in the low _nb limbs: shifted back
    d_internal_math_wide_shr(_r, u, _nb, shift);

    return;
}

// 2.8    Rounding
//------------------------------------------------------------------------------
// An integer to P bits, the precision of a floating-point type: its top P
// bits, then the bit below them and whether anything is set below that one,
// round it to nearest, ties to even. The result is rational.h's: a mantissa
// of P bits, its top bit set, in two words, the place of its lowest bit, and
// the direction. A ratio rounds through the same kernel: its quotient,
// computed to two bits past P, with a remainder that is not 0 set into the
// lowest bit, rounds as the ratio does.

// d_internal_math_wide_limb_at
//   the bits of _x, _n limbs, from _place up, a limb's worth, as a word;
// the bits past the top are 0.
D_MATH_FN dj_math_umax
d_internal_math_wide_limb_at(
    const dj_math_wide_limb* _x,
    size_t                   _n,
    size_t                   _place
)
{
    const size_t   at   = _place / (size_t)D_MATH_WIDE_LIMB_BITS;
    const unsigned part = (unsigned)(_place % (size_t)D_MATH_WIDE_LIMB_BITS);
    const dj_math_umax low  = (at < _n) ? (dj_math_umax)_x[at] : 0u;
    const dj_math_umax high = ((at + 1u) < _n) ? (dj_math_umax)_x[at + 1u]
                                               : 0u;

    return (((high << D_MATH_WIDE_LIMB_BITS) | low) >> part) &
           D_INTERNAL_MATH_WIDE_LIMB_MASK;
}

// d_internal_math_wide_any_below
//   whether any bit of _x, _n limbs, below _place is set.
D_MATH_FN bool
d_internal_math_wide_any_below(
    const dj_math_wide_limb* _x,
    size_t                   _n,
    size_t                   _place
)
{
    const size_t   whole = _place / (size_t)D_MATH_WIDE_LIMB_BITS;
    const unsigned part  = (unsigned)(_place % (size_t)D_MATH_WIDE_LIMB_BITS);

    size_t i = 0u;

    // the limbs wholly below
    for (i = 0u; ( (i < whole) &&
                   (i < _n) ); ++i)
    {
        if (_x[i] != 0u)
        {
            return true;
        }
    }

    // the low bits of the next
    return ( (part != 0u) &&
             (whole < _n) &&
             (((dj_math_umax)_x[whole] &
               (((dj_math_umax)1 << part) - 1u)) != 0u) );
}

// d_internal_math_wide_round
//   _x, _n limbs and not 0, rounded to _precision bits, 1 to 2w, to nearest,
// ties to even. An _x of no more bits is exact, its mantissa _x moved up to
// P bits; otherwise the mantissa is its top P bits, and the bit below them,
// and whether anything is set below that, round it. A mantissa of P ones
// that rounds up is 2^(P - 1), a binade up.
D_MATH_FN struct dj_internal_rational_rounded
d_internal_math_wide_round(
    const dj_math_wide_limb* _x,
    size_t                   _n,
    unsigned                 _precision
)
{
    const struct dj_internal_rational_wide one = { 0u, 1u };

    const size_t   bits  = d_internal_math_wide_bits(_x, _n);
    const size_t   low   = (bits > (size_t)_precision)
                               ? (bits - (size_t)_precision) : 0u;
    const unsigned taken = (bits > (size_t)_precision) ? _precision
                                                       : (unsigned)bits;

    struct dj_internal_rational_rounded result   = { { 0u, 0u }, 0, 0 };
    struct dj_internal_rational_wide    mantissa = { 0u, 0u };
    dj_math_umax                        part[4]  = { 0u, 0u, 0u, 0u };
    unsigned                            k        = 0u;
    int                                 exponent = 0;
    bool                                half     = false;
    bool                                sticky   = false;
    bool                                up       = false;

    // the bits taken, a limb's worth at a time from the lowest; those past
    // the top are 0, so the top part needs no cut
    for (k = 0u; (k * D_MATH_WIDE_LIMB_BITS) < taken; ++k)
    {
        part[k] = d_internal_math_wide_limb_at(_x, _n,
                                               low + ((size_t)k *
                                                      D_MATH_WIDE_LIMB_BITS));
    }

    mantissa.lo = part[0] | (part[1] << D_MATH_WIDE_LIMB_BITS);
    mantissa.hi = part[2] | (part[3] << D_MATH_WIDE_LIMB_BITS);

    // no more bits than the type has: exact
    if (bits <= (size_t)_precision)
    {
        result.mantissa = d_internal_rational_shift_wide(mantissa,
                                                         _precision - taken);
        result.exponent = (int)bits - (int)_precision;

        return result;
    }

    // the bit below the mantissa, and anything below that
    half   = ((d_internal_math_wide_limb_at(_x, _n, low - 1u) & 1u) != 0u);
    sticky = d_internal_math_wide_any_below(_x, _n, low - 1u);

    // up where that bit is 1 and something follows it, or nothing does and
    // the mantissa is odd; down where anything at all was cut off
    up               = ( (half) &&
                         ( (sticky) ||
                           ((mantissa.lo & 1u) != 0u) ) );
    result.direction = (up) ? 1
                            : ( ( (half) ||
                                  (sticky) ) ? -1 : 0 );
    exponent         = (int)low;

    if (up)
    {
        // all P bits 1: the carry makes 2^P, which is 2^(P - 1) a binade up
        if (d_internal_rational_cmp_wide(mantissa,
                                         d_internal_rational_ones(_precision))
                == 0)
        {
            mantissa = d_internal_rational_shift_wide(one, _precision - 1u);
            ++exponent;
        }
        else
        {
            mantissa = d_internal_rational_add_wide(mantissa, one);
        }
    }

    result.mantissa = mantissa;
    result.exponent = exponent;

    return result;
}


//==============================================================================
// 3.  CONSTRUCTION
//==============================================================================


// 3.1    d_math_wide_from_umax, d_math_wide_from_imax
//------------------------------------------------------------------------------
// _x as a wide integer; every value of the family is one.

D_MATH_FN struct dj_math_wide
d_math_wide_from_umax(
    dj_math_umax _x
)
{
    struct dj_math_wide result = { { 0u }, false };

    // a word is two limbs
    result.limb[0] = (dj_math_wide_limb)(_x & D_INTERNAL_MATH_WIDE_LIMB_MASK);
    result.limb[1] = (dj_math_wide_limb)(_x >> D_MATH_WIDE_LIMB_BITS);

    return result;
}

D_MATH_FN struct dj_math_wide
d_math_wide_from_imax(
    dj_math_imax _x
)
{
    struct dj_math_wide result =
        d_math_wide_from_umax(d_internal_rational_magnitude(_x));

    result.negative = (_x < 0);

    return result;
}

// 3.2    d_math_wide_is_valid
//------------------------------------------------------------------------------
// whether _x is in the one form: every limb holds a limb's value, so the
// only other form is a negative 0.

D_MATH_FN bool
d_math_wide_is_valid(
    struct dj_math_wide _x
)
{
    return ( (!_x.negative) ||
             (d_internal_math_wide_length(_x.limb, D_MATH_WIDE_LIMBS) != 0u) );
}


//==============================================================================
// 4.  ARITHMETIC
//==============================================================================


// 4.1    Signs
//------------------------------------------------------------------------------
// -_x and |_x|, which every wide integer has: the range is symmetric.

D_MATH_FN struct dj_math_wide
d_math_wide_neg(
    struct dj_math_wide _x
)
{
    struct dj_math_wide result = _x;

    // 0 stays as it is
    result.negative =
        ( (!_x.negative) &&
          (d_internal_math_wide_length(_x.limb, D_MATH_WIDE_LIMBS) != 0u) );

    return result;
}

D_MATH_FN struct dj_math_wide
d_math_wide_abs(
    struct dj_math_wide _x
)
{
    struct dj_math_wide result = _x;

    result.negative = false;

    return result;
}

// 4.2    Sums
//------------------------------------------------------------------------------

// d_internal_math_wide_sum
//   _a plus |_b| with the sign _b_negative gives it, into *_out: like signs
// add their magnitudes, and a carry out of the top passes 2^256 - 1; unlike
// signs take the smaller magnitude from the larger, which never fails, and
// the larger's sign.
D_MATH_FN enum dj_math_wide_status
d_internal_math_wide_sum(
    struct dj_math_wide* _out,
    struct dj_math_wide  _a,
    struct dj_math_wide  _b,
    bool                 _b_negative
)
{
    struct dj_math_wide result = { { 0u }, false };

    // like signs: the magnitudes add
    if (_a.negative == _b_negative)
    {
        if (d_internal_math_wide_add(result.limb, _a.limb, _b.limb,
                                     D_MATH_WIDE_LIMBS) != 0u)
        {
            return D_MATH_WIDE_OVERFLOW;
        }

        result.negative = _a.negative;
    }
    // unlike: the larger magnitude's sign
    else if (d_internal_math_wide_cmp(_a.limb, _b.limb, D_MATH_WIDE_LIMBS) >= 0)
    {
        (void)d_internal_math_wide_sub(result.limb, _a.limb, _b.limb,
                                       D_MATH_WIDE_LIMBS);
        result.negative = _a.negative;
    }
    else
    {
        (void)d_internal_math_wide_sub(result.limb, _b.limb, _a.limb,
                                       D_MATH_WIDE_LIMBS);
        result.negative = _b_negative;
    }

    // 0 is never negative
    result.negative =
        ( (result.negative) &&
          (d_internal_math_wide_length(result.limb, D_MATH_WIDE_LIMBS) != 0u) );
    *_out = result;

    return D_MATH_WIDE_OK;
}

// d_math_wide_add, d_math_wide_sub
//   _a + _b and _a - _b into *_out, or D_MATH_WIDE_OVERFLOW past 2^256 - 1.

D_MATH_FN enum dj_math_wide_status
d_math_wide_add(
    struct dj_math_wide* _out,
    struct dj_math_wide  _a,
    struct dj_math_wide  _b
)
{
    return d_internal_math_wide_sum(_out, _a, _b, _b.negative);
}

D_MATH_FN enum dj_math_wide_status
d_math_wide_sub(
    struct dj_math_wide* _out,
    struct dj_math_wide  _a,
    struct dj_math_wide  _b
)
{
    return d_internal_math_wide_sum(_out, _a, _b, !_b.negative);
}

// 4.3    Products
//------------------------------------------------------------------------------

// d_math_wide_mul
//   _a * _b into *_out, or D_MATH_WIDE_OVERFLOW past 2^256 - 1: the whole
// product, of twice the limbs, and its high half must be 0.
D_MATH_FN enum dj_math_wide_status
d_math_wide_mul(
    struct dj_math_wide* _out,
    struct dj_math_wide  _a,
    struct dj_math_wide  _b
)
{
    const size_t na =
        d_internal_math_wide_length(_a.limb, D_MATH_WIDE_LIMBS);
    const size_t nb =
        d_internal_math_wide_length(_b.limb, D_MATH_WIDE_LIMBS);

    dj_math_wide_limb   product[2u * D_MATH_WIDE_LIMBS] = { 0u };
    struct dj_math_wide result = { { 0u }, false };
    size_t              i      = 0u;

    // the limbs that are not 0, multiplied
    d_internal_math_wide_mul(product, _a.limb, na, _b.limb, nb);

    // the high half: past 2^256 - 1
    if (d_internal_math_wide_length(product + D_MATH_WIDE_LIMBS,
                                    D_MATH_WIDE_LIMBS) != 0u)
    {
        return D_MATH_WIDE_OVERFLOW;
    }

    for (i = 0u; i < D_MATH_WIDE_LIMBS; ++i)
    {
        result.limb[i] = product[i];
    }

    // unlike signs, and not 0: negative
    result.negative =
        ( (_a.negative != _b.negative) &&
          (d_internal_math_wide_length(result.limb, D_MATH_WIDE_LIMBS) != 0u) );
    *_out = result;

    return D_MATH_WIDE_OK;
}

// 4.4    Quotients
//------------------------------------------------------------------------------

// d_math_wide_divmod
//   _a / _b, truncated: the quotient, toward zero, into *_quotient, and the
// remainder, with _a's sign, into *_remainder, each where not NULL; _a is
// the quotient times _b, plus the remainder. A nonzero _a over 0 is
// D_MATH_WIDE_POLE, and 0 over 0 D_MATH_WIDE_INVALID; nothing else fails.
D_MATH_FN enum dj_math_wide_status
d_math_wide_divmod(
    struct dj_math_wide* _quotient,
    struct dj_math_wide* _remainder,
    struct dj_math_wide  _a,
    struct dj_math_wide  _b
)
{
    const size_t na =
        d_internal_math_wide_length(_a.limb, D_MATH_WIDE_LIMBS);
    const size_t nb =
        d_internal_math_wide_length(_b.limb, D_MATH_WIDE_LIMBS);

    struct dj_math_wide q = { { 0u }, false };
    struct dj_math_wide r = { { 0u }, false };
    dj_math_wide_limb   work[(2u * D_MATH_WIDE_LIMBS) + 1u] = { 0u };

    // by zero
    if (nb == 0u)
    {
        return (na == 0u) ? D_MATH_WIDE_INVALID : D_MATH_WIDE_POLE;
    }

    // fewer limbs than the divisor: the quotient 0, the remainder _a
    if (na < nb)
    {
        r = _a;
    }
    else
    {
        d_internal_math_wide_divmod(q.limb, r.limb, _a.limb, na,
                                    _b.limb, nb, work);
    }

    // the quotient's sign from both, the remainder's from _a; 0 neither
    q.negative =
        ( (_a.negative != _b.negative) &&
          (d_internal_math_wide_length(q.limb, D_MATH_WIDE_LIMBS) != 0u) );
    r.negative =
        ( (_a.negative) &&
          (d_internal_math_wide_length(r.limb, D_MATH_WIDE_LIMBS) != 0u) );

    if (_quotient != NULL)
    {
        *_quotient = q;
    }

    if (_remainder != NULL)
    {
        *_remainder = r;
    }

    return D_MATH_WIDE_OK;
}

// 4.5    d_math_wide_gcd
//------------------------------------------------------------------------------
// the greatest common divisor of _a and _b, never negative: Euclid, the
// remainder replacing the smaller until it is 0. gcd(_a, 0) is |_a|, and
// gcd(0, 0) is 0.

D_MATH_FN struct dj_math_wide
d_math_wide_gcd(
    struct dj_math_wide _a,
    struct dj_math_wide _b
)
{
    const struct dj_math_wide none = { { 0u }, false };

    struct dj_math_wide larger   = d_math_wide_abs(_a);
    struct dj_math_wide smaller  = d_math_wide_abs(_b);
    struct dj_math_wide rest     = none;
    dj_math_wide_limb   quotient[D_MATH_WIDE_LIMBS]         = { 0u };
    dj_math_wide_limb   work[(2u * D_MATH_WIDE_LIMBS) + 1u] = { 0u };
    size_t              n_larger  = 0u;
    size_t              n_smaller =
        d_internal_math_wide_length(smaller.limb, D_MATH_WIDE_LIMBS);

    // until the remainder is 0
    while (n_smaller != 0u)
    {
        n_larger = d_internal_math_wide_length(larger.limb, D_MATH_WIDE_LIMBS);
        rest     = none;

        // fewer limbs: the remainder is the dividend
        if (n_larger < n_smaller)
        {
            rest = larger;
        }
        else
        {
            d_internal_math_wide_divmod(quotient, rest.limb,
                                        larger.limb, n_larger,
                                        smaller.limb, n_smaller, work);
        }

        larger    = smaller;
        smaller   = rest;
        n_smaller = d_internal_math_wide_length(smaller.limb,
                                                D_MATH_WIDE_LIMBS);
    }

    return larger;
}


//==============================================================================
// 5.  COMPARISON
//==============================================================================


// 5.1    d_math_wide_sign, d_math_wide_bits
//------------------------------------------------------------------------------
// _x's sign, -1, 0 or 1; and the bits its magnitude needs, 0 for 0.

D_MATH_FN int
d_math_wide_sign(
    struct dj_math_wide _x
)
{
    // 0
    if (d_internal_math_wide_length(_x.limb, D_MATH_WIDE_LIMBS) == 0u)
    {
        return 0;
    }

    return (_x.negative) ? -1 : 1;
}

D_MATH_FN unsigned
d_math_wide_bits(
    struct dj_math_wide _x
)
{
    return (unsigned)d_internal_math_wide_bits(_x.limb, D_MATH_WIDE_LIMBS);
}

// 5.2    d_math_wide_eq, d_math_wide_cmp
//------------------------------------------------------------------------------
// whether _a equals _b; and -1, 0 or 1 as _a is below, equal to or above
// _b.

D_MATH_FN bool
d_math_wide_eq(
    struct dj_math_wide _a,
    struct dj_math_wide _b
)
{
    return ( (_a.negative == _b.negative) &&
             (d_internal_math_wide_cmp(_a.limb, _b.limb,
                                       D_MATH_WIDE_LIMBS) == 0) );
}

D_MATH_FN int
d_math_wide_cmp(
    struct dj_math_wide _a,
    struct dj_math_wide _b
)
{
    const int a_sign = d_math_wide_sign(_a);
    const int b_sign = d_math_wide_sign(_b);

    int order = 0;

    // unlike signs, or two zeros: the signs decide
    if ( (a_sign != b_sign) ||
         (a_sign == 0) )
    {
        return (a_sign < b_sign) ? -1 : ((a_sign > b_sign) ? 1 : 0);
    }

    order = d_internal_math_wide_cmp(_a.limb, _b.limb, D_MATH_WIDE_LIMBS);

    // the magnitudes' order, turned for two negatives
    return (a_sign > 0) ? order : -order;
}


//==============================================================================
// 6.  CONVERSIONS
//==============================================================================


// 6.1    The family's integers
//------------------------------------------------------------------------------
// _x into *_out, or D_MATH_WIDE_OVERFLOW where it lies outside dj_math_imax,
// or outside dj_math_umax: past a word, or, for dj_math_umax, below 0.

// d_internal_math_wide_low_word
//   _x's low word: its two low limbs.
D_MATH_FN dj_math_umax
d_internal_math_wide_low_word(
    struct dj_math_wide _x
)
{
    return (dj_math_umax)_x.limb[0] |
           ((dj_math_umax)_x.limb[1] << D_MATH_WIDE_LIMB_BITS);
}

D_MATH_FN enum dj_math_wide_status
d_math_wide_to_imax(
    dj_math_imax*       _out,
    struct dj_math_wide _x
)
{
    const dj_math_umax most  = (dj_math_umax)D_MATH_IMAX_MAX;
    const dj_math_umax value = d_internal_math_wide_low_word(_x);

    // past a word, or past the family's end on its side; the negative end
    // is one further
    if ( (d_internal_math_wide_length(_x.limb, D_MATH_WIDE_LIMBS) >
          D_INTERNAL_MATH_WIDE_WORD_LIMBS) ||
         (value > (most + ((_x.negative) ? 1u : 0u))) )
    {
        return D_MATH_WIDE_OVERFLOW;
    }

    // below zero: -(value - 1) - 1, which reaches the least without passing
    // the greatest
    *_out = (_x.negative) ? (-(dj_math_imax)(value - 1u) - 1)
                          : (dj_math_imax)value;

    return D_MATH_WIDE_OK;
}

D_MATH_FN enum dj_math_wide_status
d_math_wide_to_umax(
    dj_math_umax*       _out,
    struct dj_math_wide _x
)
{
    // below zero, or past a word
    if ( (_x.negative) ||
         (d_internal_math_wide_length(_x.limb, D_MATH_WIDE_LIMBS) >
          D_INTERNAL_MATH_WIDE_WORD_LIMBS) )
    {
        return D_MATH_WIDE_OVERFLOW;
    }

    *_out = d_internal_math_wide_low_word(_x);

    return D_MATH_WIDE_OK;
}

// 6.2    Rationals
//------------------------------------------------------------------------------
// A rational in the one form is an integer where its denominator is 1:
// d_math_wide_from_rational gives it, and D_MATH_WIDE_FRACTION otherwise.
// d_math_wide_to_rational gives _x over 1, or D_MATH_WIDE_OVERFLOW where it
// lies outside dj_math_imax.

D_MATH_FN enum dj_math_wide_status
d_math_wide_from_rational(
    struct dj_math_wide* _out,
    struct dj_rational   _x
)
{
    // no integer
    if (_x.den != 1)
    {
        return D_MATH_WIDE_FRACTION;
    }

    *_out = d_math_wide_from_imax(_x.num);

    return D_MATH_WIDE_OK;
}

D_MATH_FN enum dj_math_wide_status
d_math_wide_to_rational(
    struct dj_rational* _out,
    struct dj_math_wide _x
)
{
    dj_math_imax num = 0;

    // outside the family
    if (d_math_wide_to_imax(&num, _x) != D_MATH_WIDE_OK)
    {
        return D_MATH_WIDE_OVERFLOW;
    }

    _out->num = num;
    _out->den = 1;

    return D_MATH_WIDE_OK;
}

// 6.3    To floating point
//------------------------------------------------------------------------------
// _x as the nearest float, double or long double, ties to even; 0 is +0. A
// float past the largest is an infinity of _x's sign. *_direction, where not
// NULL, is -1, 0 or 1 as the result is below, equal to or above _x.
//   Rounded, a magnitude is m 2^e, m of P bits, so it is past the type's
// range, at 2^MAX_EXP or more, where e + P passes MAX_EXP; IEEE 754's
// threshold for an infinity, the largest finite value and half its last
// place, is where rounding to P bits first gives 2^MAX_EXP. A double's and a
// long double's range is past 2^256, so only a float's needs the test.

D_STATIC_ASSERT( ( (DBL_MAX_EXP > (int)D_MATH_WIDE_BITS) &&
                   (LDBL_MAX_EXP > (int)D_MATH_WIDE_BITS) ),
                 "wide.h: a double's and a long double's range must hold "
                 "every wide integer" );

D_MATH_FN float
d_math_wide_to_f(
    struct dj_math_wide _x,
    int*                _direction
)
{
    struct dj_internal_rational_rounded rounded = { { 0u, 0u }, 0, 0 };
    float                               value   = (float)0;
    int                                 way     = 0;

    // 0 is exact; otherwise rounded, and past the range an infinity, above
    // the magnitude
    if (d_internal_math_wide_length(_x.limb, D_MATH_WIDE_LIMBS) != 0u)
    {
        rounded = d_internal_math_wide_round(_x.limb, D_MATH_WIDE_LIMBS,
                                             (unsigned)FLT_MANT_DIG);

        if ((rounded.exponent + FLT_MANT_DIG) > FLT_MAX_EXP)
        {
            value = (float)HUGE_VAL;
            way   = 1;
        }
        else
        {
            value = d_internal_rational_build_f(false, rounded);
            way   = rounded.direction;
        }
    }

    // the sign turns the direction too
    if (_direction != NULL)
    {
        *_direction = (_x.negative) ? -way : way;
    }

    return (_x.negative) ? -value : value;
}

D_MATH_FN double
d_math_wide_to_d(
    struct dj_math_wide _x,
    int*                _direction
)
{
    struct dj_internal_rational_rounded rounded = { { 0u, 0u }, 0, 0 };
    double                              value   = (double)0;

    // 0 is exact; otherwise rounded
    if (d_internal_math_wide_length(_x.limb, D_MATH_WIDE_LIMBS) != 0u)
    {
        rounded = d_internal_math_wide_round(_x.limb, D_MATH_WIDE_LIMBS,
                                             (unsigned)DBL_MANT_DIG);
        value   = d_internal_rational_build_d(false, rounded);
    }

    // the sign turns the direction too
    if (_direction != NULL)
    {
        *_direction = (_x.negative) ? -rounded.direction : rounded.direction;
    }

    return (_x.negative) ? -value : value;
}

D_MATH_FN long double
d_math_wide_to_ld(
    struct dj_math_wide _x,
    int*                _direction
)
{
    struct dj_internal_rational_rounded rounded = { { 0u, 0u }, 0, 0 };
    long double                         value   = (long double)0;

    // 0 is exact; otherwise rounded
    if (d_internal_math_wide_length(_x.limb, D_MATH_WIDE_LIMBS) != 0u)
    {
        rounded = d_internal_math_wide_round(_x.limb, D_MATH_WIDE_LIMBS,
                                             (unsigned)LDBL_MANT_DIG);
        value   = d_internal_rational_build_ld(false, rounded);
    }

    // the sign turns the direction too
    if (_direction != NULL)
    {
        *_direction = (_x.negative) ? -rounded.direction : rounded.direction;
    }

    return (_x.negative) ? -value : value;
}


//==============================================================================
// 7.  TEXT
//==============================================================================
// A wide integer's text is decimal: an optional sign, + or -, then one digit
// or more, and nothing else. Leading zeros are allowed, and -0 is 0. Text is
// written with a minus sign below zero and no leading zero.


// 7.1    d_math_wide_from_text
//------------------------------------------------------------------------------
// _text, all _length characters of it, as a wide integer, into *_out:
// D_MATH_WIDE_SYNTAX where it is no integer, and D_MATH_WIDE_OVERFLOW where
// it is one past 2^256 - 1 in magnitude. Each digit makes the value so far
// ten times greater, plus the digit, and a limb carried out of the top
// passes 2^256 - 1, where it stays.

D_MATH_FN enum dj_math_wide_status
d_math_wide_from_text(
    struct dj_math_wide* _out,
    const char*          _text,
    size_t               _length
)
{
    struct dj_math_wide result   = { { 0u }, false };
    size_t              first    = 0u;
    size_t              i        = 0u;
    bool                negative = false;

    // the sign
    if ( (_length > 0u) &&
         ( (_text[0] == '+') ||
           (_text[0] == '-') ) )
    {
        negative = (_text[0] == '-');
        first    = 1u;
    }

    // a digit at least, and digits only
    if (first == _length)
    {
        return D_MATH_WIDE_SYNTAX;
    }

    for (i = first; i < _length; ++i)
    {
        if ( (_text[i] < '0') ||
             (_text[i] > '9') )
        {
            return D_MATH_WIDE_SYNTAX;
        }
    }

    // the digits, the highest first
    for (i = first; i < _length; ++i)
    {
        const dj_math_wide_limb digit = (dj_math_wide_limb)(_text[i] - '0');

        if (d_internal_math_wide_mul_1(result.limb, result.limb,
                                       D_MATH_WIDE_LIMBS, 10u, digit) != 0u)
        {
            return D_MATH_WIDE_OVERFLOW;
        }
    }

    // 0 is never negative
    result.negative =
        ( (negative) &&
          (d_internal_math_wide_length(result.limb, D_MATH_WIDE_LIMBS) != 0u) );
    *_out = result;

    return D_MATH_WIDE_OK;
}

// 7.2    d_math_wide_to_text
//------------------------------------------------------------------------------
// _x's text into _buffer, as snprintf writes: at most _capacity characters,
// its NUL among them, and none where _capacity is 0, when _buffer may be
// NULL. Returns the whole text's length without the NUL, whatever
// _capacity is: a result not below _capacity means the text was cut short.
// D_MATH_WIDE_TEXT_SIZE characters always suffice.
//   The digits come from dividing by D_INTERNAL_MATH_WIDE_TEN_POWER, the
// remainders a group of digits each, the lowest first: every group but the
// highest has all its digits, zeros included.

D_MATH_FN_RT size_t
d_math_wide_to_text(
    char*               _buffer,
    size_t              _capacity,
    struct dj_math_wide _x
)
{
    const dj_math_wide_limb ten =
        (dj_math_wide_limb)D_INTERNAL_MATH_WIDE_TEN_POWER;

    struct dj_math_wide rest   = _x;
    char                digits[D_MATH_WIDE_TEXT_SIZE] = { 0 };
    size_t              n      =
        d_internal_math_wide_length(_x.limb, D_MATH_WIDE_LIMBS);
    size_t              count  = 0u;
    size_t              length = 0u;
    size_t              at     = 0u;
    unsigned            k      = 0u;

    // the groups of digits, the lowest first, and their digits likewise
    do
    {
        dj_math_umax group = (dj_math_umax)
            d_internal_math_wide_divmod_1(rest.limb, rest.limb, n, ten);

        n = d_internal_math_wide_length(rest.limb, n);

        // the highest group stops at its own highest digit
        for (k = 0u; k < D_INTERNAL_MATH_WIDE_TEN_DIGITS; ++k)
        {
            if ( (n == 0u) &&
                 (group == 0u) &&
                 (k > 0u) )
            {
                break;
            }

            digits[count] = (char)('0' + (int)(group % 10u));
            ++count;
            group = group / 10u;
        }
    } while (n > 0u);

    length = count + ((_x.negative) ? 1u : 0u);

    // as much as fits before the NUL: the sign, then the digits
    if ( (_buffer != NULL) &&
         (_capacity > 0u) )
    {
        if ( (_x.negative) &&
             ((at + 1u) < _capacity) )
        {
            _buffer[at] = '-';
            ++at;
        }

        while ( (count > 0u) &&
                ((at + 1u) < _capacity) )
        {
            --count;
            _buffer[at] = digits[count];
            ++at;
        }

        _buffer[at] = '\0';
    }

    return length;
}


#endif  // DJINTERP_C_MATH_WIDE_H
