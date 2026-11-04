/*******************************************************************************
* djinterp [c/math]                                                     t_wide.c
*
*   Conformance harness for the wide integers (c/math/wide.h): every kernel
* against an exact reference of the harness's own, naturals of 640 bits in
* 16-bit limbs and a sign, so that it needs no type wider than 32 bits and
* runs alike in every build, ISO strict C++98 on a 32-bit target included.
* The reference is written another way than the unit: its division goes a
* bit at a time, its greatest common divisor is binary (Stein's), and its
* text comes a digit at a time.
*   A result a kernel stores must be the exact value, in the one form, with
* D_MATH_WIDE_OK. Past 2^256 - 1 the status must be D_MATH_WIDE_OVERFLOW,
* and a division by zero D_MATH_WIDE_POLE, or D_MATH_WIDE_INVALID for 0/0,
* with every destination as it was. A quotient must be truncated toward zero
* and its remainder take the dividend's sign, and either may be left out.
*   A conversion to float, double or long double must give the value of its
* type nearest the integer, ties to even: by exact comparison, neither of its
* neighbours, found with the C library's nextafter, is nearer; and past the
* largest float, an infinity. Its direction must be how it stands to the
* integer, exactly.
*   Text: each operand's text must be the reference's digits, read back to
* the same integer, with a sign or leading zeros too, and cut short as
* snprintf cuts, its whole length returned; a text that is no integer, or
* one past 2^256 - 1, must be refused with nothing stored.
*   The operands: 0 and the integers about the powers of two that matter --
* the limbs', the word's, the float's range and the top -- both signs, each
* pair; the divisions where Knuth's algorithm guesses a limb's range,
* corrects its guess or adds the divisor back; the integers about the types'
* precisions, their ties and runs of ones, and the powers of ten, each
* alone; and 20,000 pairs drawn at random, each of a random length up to 256
* bits, some in runs of ones. The limb kernels are checked on their own too,
* at every length up to 512 bits, which the boxed kind will need, and with
* the destination an operand where they allow it; rounding there by its own
* criterion, within half a unit of the integer's last place kept. From C++14
* the kernels are checked as constant expressions as well.
*   The same file compiles as C++ (-x c++), at any level from C++98.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/math/t_wide.c -o t_wide -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_wide.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/
// std
#include <float.h>   // FLT_MANT_DIG ... LDBL_MAX
#include <limits.h>  // CHAR_BIT
#include <math.h>    // frexpl, nextafterf, nextafter, nextafterl, ldexpl,
                     // floorl
#include <stdio.h>   // printf
#include <string.h>  // strlen, strcmp, memcpy
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/wide.h"  // unit under test


// the reference: forty limbs of 16 bits, 640 bits, past any product of two
// operands and any distance the oracle measures; and the room its text needs
#define D_TESTS_NAT_LIMBS  40
#define D_TESTS_NAT_BITS   640
#define D_TESTS_TEXT       200

// the unit's limb and integer, as the harness counts them, and the reference
// limbs in one of the unit's
#define D_TESTS_H       ((int)D_MATH_WIDE_LIMB_BITS)
#define D_TESTS_N       ((int)D_MATH_WIDE_LIMBS)
#define D_TESTS_PIECES  (D_TESTS_H / 16)

// the limb kernels' longest operands: twice a wide integer's limbs
#define D_TESTS_KERNEL_LIMBS  (2 * D_TESTS_N)

// the pairs drawn at random, and the draws for the limb kernels
#define D_TESTS_TRIALS         20000L
#define D_TESTS_KERNEL_TRIALS  3000L

// the operands' lists, and the constructed divisions'
#define D_TESTS_PAIRS_MAX      128
#define D_TESTS_ONES_MAX       1400
#define D_TESTS_DIVISIONS_MAX  1200

static long          g_checks   = 0;
static long          g_failures = 0;
static bool          g_overrun  = false;
static unsigned long g_state    = 1618033UL;


//==============================================================================
// 1.  THE REFERENCE
//==============================================================================
// Naturals of forty limbs of 16 bits, the low first, each held in an unsigned
// long: a limb times a limb, plus two limbs, fits 32 bits, so no step needs
// a wider type. A sum, product or shift that would pass the 640 bits sets
// g_overrun, and so does a difference below 0; the harness checks at its end
// that neither happened. An integer is a natural and a sign.


// 1.1    Naturals
//------------------------------------------------------------------------------

// d_tests_nat
//   struct: a natural below 2^640.
struct d_tests_nat
{
    unsigned long limb[D_TESTS_NAT_LIMBS];  // 16 bits each, the low first
};

// d_tests_nat_length
//   the limbs _x needs: 0 for 0.
static int
d_tests_nat_length(
    const struct d_tests_nat* _x
)
{
    int n = D_TESTS_NAT_LIMBS;

    while ( (n > 0) &&
            (_x->limb[n - 1] == 0UL) )
    {
        --n;
    }

    return n;
}

static bool
d_tests_nat_is_zero(
    const struct d_tests_nat* _x
)
{
    return (d_tests_nat_length(_x) == 0);
}

// d_tests_nat_of
//   _x, below 2^32, as a natural.
static struct d_tests_nat
d_tests_nat_of(
    unsigned long _x
)
{
    struct d_tests_nat result = { { 0UL } };

    result.limb[0] = _x & 0xFFFFUL;
    result.limb[1] = (_x >> 16) & 0xFFFFUL;

    return result;
}

// d_tests_nat_of_umax
//   _x as a natural, sixteen bits at a time.
static struct d_tests_nat
d_tests_nat_of_umax(
    d_math_umax _x
)
{
    struct d_tests_nat result = { { 0UL } };
    d_math_umax        rest   = _x;
    int                i      = 0;

    while (rest != 0u)
    {
        result.limb[i] = (unsigned long)(rest & 0xFFFFu);
        rest           = rest >> 16;
        ++i;
    }

    return result;
}

// d_tests_nat_pow2
//   2^_k, _k below 640.
static struct d_tests_nat
d_tests_nat_pow2(
    int _k
)
{
    struct d_tests_nat result = { { 0UL } };

    result.limb[_k / 16] = 1UL << (_k % 16);

    return result;
}

// d_tests_nat_cmp
//   -1, 0 or 1 as _a is below, equal to or above _b.
static int
d_tests_nat_cmp(
    const struct d_tests_nat* _a,
    const struct d_tests_nat* _b
)
{
    int i = D_TESTS_NAT_LIMBS;

    while (i > 0)
    {
        --i;

        if (_a->limb[i] != _b->limb[i])
        {
            return (_a->limb[i] < _b->limb[i]) ? -1 : 1;
        }
    }

    return 0;
}

// d_tests_nat_bits
//   the bits _x needs: 0 for 0.
static int
d_tests_nat_bits(
    const struct d_tests_nat* _x
)
{
    const int n = d_tests_nat_length(_x);

    unsigned long top   = 0UL;
    int           count = 0;

    if (n == 0)
    {
        return 0;
    }

    top   = _x->limb[n - 1];
    count = 16 * (n - 1);

    while (top != 0UL)
    {
        top = top >> 1;
        ++count;
    }

    return count;
}

// d_tests_nat_bit
//   bit _i of _x, 0 or 1.
static unsigned long
d_tests_nat_bit(
    const struct d_tests_nat* _x,
    int                       _i
)
{
    return (_x->limb[_i / 16] >> (_i % 16)) & 1UL;
}

// d_tests_nat_add
//   _a + _b.
static struct d_tests_nat
d_tests_nat_add(
    const struct d_tests_nat* _a,
    const struct d_tests_nat* _b
)
{
    struct d_tests_nat result = { { 0UL } };
    unsigned long      carry  = 0UL;
    int                i      = 0;

    for (i = 0; i < D_TESTS_NAT_LIMBS; ++i)
    {
        const unsigned long sum = _a->limb[i] + _b->limb[i] + carry;

        result.limb[i] = sum & 0xFFFFUL;
        carry          = sum >> 16;
    }

    if (carry != 0UL)
    {
        g_overrun = true;
    }

    return result;
}

// d_tests_nat_sub
//   _a - _b, _a at least _b.
static struct d_tests_nat
d_tests_nat_sub(
    const struct d_tests_nat* _a,
    const struct d_tests_nat* _b
)
{
    struct d_tests_nat result = { { 0UL } };
    unsigned long      borrow = 0UL;
    int                i      = 0;

    for (i = 0; i < D_TESTS_NAT_LIMBS; ++i)
    {
        const unsigned long take = _b->limb[i] + borrow;

        result.limb[i] = (_a->limb[i] + 0x10000UL - take) & 0xFFFFUL;
        borrow         = (_a->limb[i] < take) ? 1UL : 0UL;
    }

    if (borrow != 0UL)
    {
        g_overrun = true;
    }

    return result;
}

// d_tests_nat_mul
//   _a * _b: the schoolbook product into twice the limbs, whose high half
// must be 0.
static struct d_tests_nat
d_tests_nat_mul(
    const struct d_tests_nat* _a,
    const struct d_tests_nat* _b
)
{
    const int na = d_tests_nat_length(_a);
    const int nb = d_tests_nat_length(_b);

    unsigned long      wide[2 * D_TESTS_NAT_LIMBS] = { 0UL };
    struct d_tests_nat result = { { 0UL } };
    int                i      = 0;
    int                j      = 0;

    for (i = 0; i < na; ++i)
    {
        unsigned long carry = 0UL;

        for (j = 0; j < nb; ++j)
        {
            const unsigned long part = (_a->limb[i] * _b->limb[j]) +
                                       wide[i + j] + carry;

            wide[i + j] = part & 0xFFFFUL;
            carry       = part >> 16;
        }

        wide[i + nb] = carry;
    }

    for (i = 0; i < (2 * D_TESTS_NAT_LIMBS); ++i)
    {
        if (i < D_TESTS_NAT_LIMBS)
        {
            result.limb[i] = wide[i];
        }
        else if (wide[i] != 0UL)
        {
            g_overrun = true;
        }
    }

    return result;
}

// d_tests_nat_shl
//   _x * 2^_k, _k at least 0.
static struct d_tests_nat
d_tests_nat_shl(
    const struct d_tests_nat* _x,
    int                       _k
)
{
    const int whole = _k / 16;
    const int part  = _k % 16;
    const int bits  = d_tests_nat_bits(_x);

    struct d_tests_nat result = { { 0UL } };
    int                i      = 0;

    if ( (bits > 0) &&
         ((bits + _k) > D_TESTS_NAT_BITS) )
    {
        g_overrun = true;
    }

    for (i = 0; i < D_TESTS_NAT_LIMBS; ++i)
    {
        const int     from  = i - whole;
        unsigned long value = 0UL;

        if (from >= 0)
        {
            value = (_x->limb[from] << part) & 0xFFFFUL;
        }

        if ( (part != 0) &&
             ((from - 1) >= 0) )
        {
            value = value | (_x->limb[from - 1] >> (16 - part));
        }

        result.limb[i] = value;
    }

    return result;
}

// d_tests_nat_shr
//   _x / 2^_k, rounded down, _k at least 0.
static struct d_tests_nat
d_tests_nat_shr(
    const struct d_tests_nat* _x,
    int                       _k
)
{
    const int whole = _k / 16;
    const int part  = _k % 16;

    struct d_tests_nat result = { { 0UL } };
    int                i      = 0;

    for (i = 0; i < D_TESTS_NAT_LIMBS; ++i)
    {
        const int     from  = i + whole;
        unsigned long value = 0UL;

        if (from < D_TESTS_NAT_LIMBS)
        {
            value = _x->limb[from] >> part;
        }

        if ( (part != 0) &&
             ((from + 1) < D_TESTS_NAT_LIMBS) )
        {
            value = value | ((_x->limb[from + 1] << (16 - part)) & 0xFFFFUL);
        }

        result.limb[i] = value;
    }

    return result;
}

// d_tests_nat_zeros
//   the 0 bits below _x's lowest 1; _x not 0.
static int
d_tests_nat_zeros(
    const struct d_tests_nat* _x
)
{
    int i = 0;

    while (_x->limb[i / 16] == 0UL)
    {
        i = i + 16;
    }

    while (d_tests_nat_bit(_x, i) == 0UL)
    {
        ++i;
    }

    return i;
}

// d_tests_nat_divmod
//   _a / _b, _b not 0, into *_q and *_r: long division a bit at a time, from
// _a's top bit down; the remainder is doubled, the next bit brought down,
// and _b taken away where it goes.
static void
d_tests_nat_divmod(
    const struct d_tests_nat* _a,
    const struct d_tests_nat* _b,
    struct d_tests_nat*       _q,
    struct d_tests_nat*       _r
)
{
    struct d_tests_nat q = { { 0UL } };
    struct d_tests_nat r = { { 0UL } };
    int                i = d_tests_nat_bits(_a);

    while (i > 0)
    {
        unsigned long carry = 0UL;
        int           k     = 0;

        --i;
        carry = d_tests_nat_bit(_a, i);

        // r = 2r + the bit
        for (k = 0; k < D_TESTS_NAT_LIMBS; ++k)
        {
            const unsigned long twice = (r.limb[k] << 1) | carry;

            r.limb[k] = twice & 0xFFFFUL;
            carry     = twice >> 16;
        }

        if (d_tests_nat_cmp(&r, _b) >= 0)
        {
            r              = d_tests_nat_sub(&r, _b);
            q.limb[i / 16] = q.limb[i / 16] | (1UL << (i % 16));
        }
    }

    *_q = q;
    *_r = r;

    return;
}

// d_tests_nat_gcd
//   the greatest common divisor of _a and _b, binary (Stein): the common
// factors of 2 set aside, the smaller odd number taken from the larger, and
// the difference's own factors of 2 removed, until it is 0.
static struct d_tests_nat
d_tests_nat_gcd(
    const struct d_tests_nat* _a,
    const struct d_tests_nat* _b
)
{
    struct d_tests_nat u     = *_a;
    struct d_tests_nat v     = *_b;
    struct d_tests_nat swap  = { { 0UL } };
    int                shift = 0;
    int                zu    = 0;
    int                zv    = 0;

    if (d_tests_nat_is_zero(&u))
    {
        return v;
    }

    if (d_tests_nat_is_zero(&v))
    {
        return u;
    }

    zu    = d_tests_nat_zeros(&u);
    zv    = d_tests_nat_zeros(&v);
    shift = (zu < zv) ? zu : zv;
    u     = d_tests_nat_shr(&u, zu);

    do
    {
        v = d_tests_nat_shr(&v, d_tests_nat_zeros(&v));

        if (d_tests_nat_cmp(&u, &v) > 0)
        {
            swap = u;
            u    = v;
            v    = swap;
        }

        v = d_tests_nat_sub(&v, &u);
    } while (!d_tests_nat_is_zero(&v));

    return d_tests_nat_shl(&u, shift);
}

// d_tests_nat_of_text
//   the digits of _text, all of them, as a natural.
static struct d_tests_nat
d_tests_nat_of_text(
    const char* _text
)
{
    const struct d_tests_nat ten = d_tests_nat_of(10UL);

    struct d_tests_nat result = { { 0UL } };
    size_t             i      = 0u;

    for (i = 0u; _text[i] != '\0'; ++i)
    {
        const struct d_tests_nat digit =
            d_tests_nat_of((unsigned long)(_text[i] - '0'));
        const struct d_tests_nat tens = d_tests_nat_mul(&result, &ten);

        result = d_tests_nat_add(&tens, &digit);
    }

    return result;
}

// d_tests_nat_text
//   _x in decimal into _buffer, at least D_TESTS_TEXT characters, returned:
// a digit at a time, the lowest first, by division by ten.
static const char*
d_tests_nat_text(
    const struct d_tests_nat* _x,
    char*                     _buffer
)
{
    struct d_tests_nat rest  = *_x;
    char               digits[D_TESTS_TEXT];
    int                count = 0;
    int                at    = 0;
    int                i     = 0;

    do
    {
        unsigned long carry = 0UL;

        for (i = d_tests_nat_length(&rest) - 1; i >= 0; --i)
        {
            const unsigned long part = (carry << 16) | rest.limb[i];

            rest.limb[i] = part / 10UL;
            carry        = part % 10UL;
        }

        digits[count] = (char)('0' + (int)carry);
        ++count;
    } while (!d_tests_nat_is_zero(&rest));

    while (count > 0)
    {
        --count;
        _buffer[at] = digits[count];
        ++at;
    }

    _buffer[at] = '\0';

    return _buffer;
}


// 1.2    Integers
//------------------------------------------------------------------------------

// d_tests_int
//   struct: an integer, a natural and a sign.
struct d_tests_int
{
    struct d_tests_nat mag;       // the magnitude
    bool               negative;  // below zero; never for 0
};

static const struct d_tests_int g_none = { { { 0UL } }, false };

// d_tests_int_make
//   _mag with the sign _negative gives it; 0 is never negative.
static struct d_tests_int
d_tests_int_make(
    struct d_tests_nat _mag,
    bool               _negative
)
{
    struct d_tests_int result = g_none;

    result.mag      = _mag;
    result.negative = ( (_negative) &&
                        (!d_tests_nat_is_zero(&_mag)) );

    return result;
}

static struct d_tests_int
d_tests_int_neg(
    const struct d_tests_int* _x
)
{
    return d_tests_int_make(_x->mag, !_x->negative);
}

static struct d_tests_int
d_tests_int_add(
    const struct d_tests_int* _a,
    const struct d_tests_int* _b
)
{
    // like signs: the magnitudes add
    if (_a->negative == _b->negative)
    {
        return d_tests_int_make(d_tests_nat_add(&_a->mag, &_b->mag),
                                _a->negative);
    }

    // unlike: the larger's sign
    if (d_tests_nat_cmp(&_a->mag, &_b->mag) >= 0)
    {
        return d_tests_int_make(d_tests_nat_sub(&_a->mag, &_b->mag),
                                _a->negative);
    }

    return d_tests_int_make(d_tests_nat_sub(&_b->mag, &_a->mag),
                            _b->negative);
}

static struct d_tests_int
d_tests_int_mul(
    const struct d_tests_int* _a,
    const struct d_tests_int* _b
)
{
    return d_tests_int_make(d_tests_nat_mul(&_a->mag, &_b->mag),
                            (_a->negative != _b->negative));
}

// d_tests_int_sign
//   -1, 0 or 1.
static int
d_tests_int_sign(
    const struct d_tests_int* _x
)
{
    if (d_tests_nat_is_zero(&_x->mag))
    {
        return 0;
    }

    return (_x->negative) ? -1 : 1;
}

static int
d_tests_int_cmp(
    const struct d_tests_int* _a,
    const struct d_tests_int* _b
)
{
    const int a_sign = d_tests_int_sign(_a);
    const int b_sign = d_tests_int_sign(_b);
    const int order  = d_tests_nat_cmp(&_a->mag, &_b->mag);

    if (a_sign != b_sign)
    {
        return (a_sign < b_sign) ? -1 : 1;
    }

    return (a_sign >= 0) ? order : -order;
}

// d_tests_int_same
//   whether _a and _b are one integer with one sign flag: a negative 0 is not
// 0 here.
static bool
d_tests_int_same(
    const struct d_tests_int* _a,
    const struct d_tests_int* _b
)
{
    return ( (_a->negative == _b->negative) &&
             (d_tests_nat_cmp(&_a->mag, &_b->mag) == 0) );
}

// d_tests_int_divmod
//   _a / _b, _b not 0, truncated: the quotient toward zero, the remainder
// with _a's sign.
static void
d_tests_int_divmod(
    const struct d_tests_int* _a,
    const struct d_tests_int* _b,
    struct d_tests_int*       _q,
    struct d_tests_int*       _r
)
{
    struct d_tests_nat q = { { 0UL } };
    struct d_tests_nat r = { { 0UL } };

    d_tests_nat_divmod(&_a->mag, &_b->mag, &q, &r);
    *_q = d_tests_int_make(q, (_a->negative != _b->negative));
    *_r = d_tests_int_make(r, _a->negative);

    return;
}

// d_tests_int_fits
//   whether |_x| is below 2^256.
static bool
d_tests_int_fits(
    const struct d_tests_int* _x
)
{
    return (d_tests_nat_bits(&_x->mag) <= 256);
}

// d_tests_int_text
//   _x in decimal, its sign first, into _buffer, returned.
static const char*
d_tests_int_text(
    const struct d_tests_int* _x,
    char*                     _buffer
)
{
    if (_x->negative)
    {
        _buffer[0] = '-';
        (void)d_tests_nat_text(&_x->mag, _buffer + 1);

        return _buffer;
    }

    return d_tests_nat_text(&_x->mag, _buffer);
}

// d_tests_int_of_imax
//   _x as an integer.
static struct d_tests_int
d_tests_int_of_imax(
    d_math_imax _x
)
{
    const d_math_umax magnitude = (_x < 0) ? ((d_math_umax)0 - (d_math_umax)_x)
                                           : (d_math_umax)_x;

    return d_tests_int_make(d_tests_nat_of_umax(magnitude), (_x < 0));
}


// 1.3    Drawing at random
//------------------------------------------------------------------------------

// d_tests_random16
//   16 bits from the generator: the high half of its state.
static unsigned long
d_tests_random16(void)
{
    g_state = ((g_state * 1103515245UL) + 12345UL) & 0xFFFFFFFFUL;

    return (g_state >> 16) & 0xFFFFUL;
}

// d_tests_random_below
//   a draw below _n, at most 65536.
static int
d_tests_random_below(
    int _n
)
{
    return (int)(d_tests_random16() % (unsigned long)_n);
}

// d_tests_random_nat
//   a natural of _bits bits, its top one set: random limbs, or, where _runs,
// runs of ones and zeros, each up to 48 long.
static struct d_tests_nat
d_tests_random_nat(
    int  _bits,
    bool _runs
)
{
    struct d_tests_nat result = { { 0UL } };
    unsigned long      bit    = 0UL;
    int                run    = 0;
    int                i      = 0;

    if (_bits <= 0)
    {
        return result;
    }

    if (!_runs)
    {
        for (i = 0; i < ((_bits + 15) / 16); ++i)
        {
            result.limb[i] = d_tests_random16();
        }

        if ((_bits % 16) != 0)
        {
            result.limb[(_bits - 1) / 16] =
                result.limb[(_bits - 1) / 16] &
                ((1UL << (_bits % 16)) - 1UL);
        }
    }
    else
    {
        bit = d_tests_random16() & 1UL;

        for (i = 0; i < _bits; ++i)
        {
            if (run == 0)
            {
                run = 1 + d_tests_random_below(48);
                bit = bit ^ 1UL;
            }

            result.limb[i / 16] = result.limb[i / 16] | (bit << (i % 16));
            --run;
        }
    }

    result.limb[(_bits - 1) / 16] =
        result.limb[(_bits - 1) / 16] | (1UL << ((_bits - 1) % 16));

    return result;
}

// d_tests_random_int
//   an integer of either sign: a full 256 bits one time in eight, at most 16
// bits one in eight, otherwise of any length up to 256; one in four in runs.
static struct d_tests_int
d_tests_random_int(void)
{
    const int  pick     = d_tests_random_below(8);
    const int  bits     = (pick == 0) ? 256
                                      : ( (pick == 1)
                                              ? d_tests_random_below(17)
                                              : d_tests_random_below(257) );
    const bool runs     = (d_tests_random_below(4) == 0);
    const bool negative = (d_tests_random_below(2) == 0);

    return d_tests_int_make(d_tests_random_nat(bits, runs), negative);
}


//==============================================================================
// 2.  CHECKS
//==============================================================================


// 2.1    Counting and reports
//------------------------------------------------------------------------------

// d_tests_count
//   counts a check: true when it failed and is among the first few failures,
// which are reported in full.
static bool
d_tests_count(
    bool _ok
)
{
    ++g_checks;

    if (!_ok)
    {
        ++g_failures;

        return (g_failures <= 40);
    }

    return false;
}

// d_tests_expect
//   counts a check, and reports the first few failures with their operands.
static void
d_tests_expect(
    bool                      _ok,
    const char*               _what,
    const struct d_tests_int* _a,
    const struct d_tests_int* _b,
    int                       _line
)
{
    char text[2][D_TESTS_TEXT];

    if (d_tests_count(_ok))
    {
        printf("  FAIL line %d: %s (%s, %s)\n", _line, _what,
               d_tests_int_text(_a, text[0]),
               d_tests_int_text(_b, text[1]));
    }

    return;
}

#define D_TESTS_EXPECT(ok, what, a, b)                                         \
    d_tests_expect((ok), (what), (a), (b), __LINE__)


// 2.2    The unit's integers and the reference's
//------------------------------------------------------------------------------
// Each of the unit's limbs is one or two of the reference's: the conversions
// move bits only, and no arithmetic of the unit's is used for them.

// d_tests_of_wide
//   the unit's integer as the reference's, its sign flag as it is.
static struct d_tests_int
d_tests_of_wide(
    struct d_math_wide _x
)
{
    struct d_tests_int result = g_none;
    int                i      = 0;
    int                k      = 0;

    for (i = 0; i < D_TESTS_N; ++i)
    {
        for (k = 0; k < D_TESTS_PIECES; ++k)
        {
            result.mag.limb[(i * D_TESTS_PIECES) + k] =
                ((unsigned long)_x.limb[i] >> (16 * k)) & 0xFFFFUL;
        }
    }

    result.negative = _x.negative;

    return result;
}

// d_tests_to_wide
//   _x, below 2^256 in magnitude, as the unit's integer.
static struct d_math_wide
d_tests_to_wide(
    const struct d_tests_int* _x
)
{
    struct d_math_wide result = { { 0u }, false };
    int                i      = 0;
    int                k      = 0;

    for (i = 0; i < D_TESTS_N; ++i)
    {
        unsigned long value = 0UL;

        for (k = 0; k < D_TESTS_PIECES; ++k)
        {
            value = value |
                    (_x->mag.limb[(i * D_TESTS_PIECES) + k] << (16 * k));
        }

        result.limb[i] = (d_math_wide_limb)value;
    }

    result.negative = _x->negative;

    return result;
}

// d_tests_nat_of_limbs, d_tests_limbs_of
//   _n of the unit's limbs as a natural, and back.
static struct d_tests_nat
d_tests_nat_of_limbs(
    const d_math_wide_limb* _x,
    int                     _n
)
{
    struct d_tests_nat result = { { 0UL } };
    int                i      = 0;
    int                k      = 0;

    for (i = 0; i < _n; ++i)
    {
        for (k = 0; k < D_TESTS_PIECES; ++k)
        {
            result.limb[(i * D_TESTS_PIECES) + k] =
                ((unsigned long)_x[i] >> (16 * k)) & 0xFFFFUL;
        }
    }

    return result;
}

static void
d_tests_limbs_of(
    const struct d_tests_nat* _x,
    d_math_wide_limb*         _out,
    int                       _n
)
{
    int i = 0;
    int k = 0;

    for (i = 0; i < _n; ++i)
    {
        unsigned long value = 0UL;

        for (k = 0; k < D_TESTS_PIECES; ++k)
        {
            value = value | (_x->limb[(i * D_TESTS_PIECES) + k] << (16 * k));
        }

        _out[i] = (d_math_wide_limb)value;
    }

    return;
}

// d_tests_wide_valid
//   whether _x is in the one form, by the harness's own test: no negative 0.
static bool
d_tests_wide_valid(
    struct d_math_wide _x
)
{
    int i = 0;

    if (!_x.negative)
    {
        return true;
    }

    for (i = 0; i < D_TESTS_N; ++i)
    {
        if (_x.limb[i] != 0u)
        {
            return true;
        }
    }

    return false;
}

// d_tests_wide_same
//   whether _a and _b hold the same limbs and sign.
static bool
d_tests_wide_same(
    const struct d_math_wide* _a,
    const struct d_math_wide* _b
)
{
    int i = 0;

    for (i = 0; i < D_TESTS_N; ++i)
    {
        if (_a->limb[i] != _b->limb[i])
        {
            return false;
        }
    }

    return (_a->negative == _b->negative);
}

// d_tests_marker
//   what a destination holds before a kernel runs: a pattern in every limb,
// and negative. A kernel that refuses must leave it.
static struct d_math_wide
d_tests_marker(void)
{
    struct d_math_wide result = { { 0u }, true };
    int                i      = 0;

    for (i = 0; i < D_TESTS_N; ++i)
    {
        result.limb[i] = (d_math_wide_limb)(0xA5A5A5A5UL >> (32 - D_TESTS_H));
    }

    return result;
}

// d_tests_result
//   checks a kernel's status and stored result against the exact value: where
// it fits, stored exactly and in the one form; past 2^256 - 1, an overflow,
// and the destination as it was.
static void
d_tests_result(
    enum d_math_wide_status   _status,
    const struct d_math_wide* _out,
    const struct d_tests_int* _exact,
    const char*               _what,
    const struct d_tests_int* _a,
    const struct d_tests_int* _b,
    int                       _line
)
{
    const struct d_math_wide marker = d_tests_marker();
    const struct d_tests_int got    = d_tests_of_wide(*_out);

    if (d_tests_int_fits(_exact))
    {
        d_tests_expect( (_status == D_MATH_WIDE_OK) &&
                        (d_tests_wide_valid(*_out)) &&
                        (d_tests_int_same(&got, _exact)),
                        _what, _a, _b, _line );
    }
    else
    {
        d_tests_expect( (_status == D_MATH_WIDE_OVERFLOW) &&
                        (d_tests_wide_same(_out, &marker)),
                        _what, _a, _b, _line );
    }

    return;
}

// 2.3    The layout
//------------------------------------------------------------------------------

// d_tests_layout
//   the limbs: half a word, as many as 256 bits need; the text's room; and
// a negative 0, made by hand, not in the one form.
static void
d_tests_layout(void)
{
    struct d_math_wide negative_zero = { { 0u }, true };

    D_TESTS_EXPECT( (!d_math_wide_is_valid(negative_zero)) &&
                    (!d_tests_wide_valid(negative_zero)),
                    "a negative 0 is not in the one form", &g_none, &g_none );

    D_TESTS_EXPECT( (D_MATH_WIDE_BITS == 256u) &&
                    ((D_MATH_WIDE_LIMBS * D_MATH_WIDE_LIMB_BITS) == 256u) &&
                    ((sizeof(d_math_wide_limb) * CHAR_BIT) ==
                     D_MATH_WIDE_LIMB_BITS) &&
                    ((2u * sizeof(d_math_wide_limb)) ==
                     sizeof(d_math_umax)) &&
                    (D_MATH_WIDE_TEXT_SIZE == 80u),
                    "the layout", &g_none, &g_none );

    return;
}


//==============================================================================
// 3.  THE NEAREST VALUE
//==============================================================================
// A positive finite value of a type of P bits is m 2^e, m below 2^P: frexpl
// gives its fraction and exponent, and the fraction times 2^P is m, split
// into limbs by exact divisions. The integer and the three candidates, the
// result and its neighbours, are then put on one scale, 2^s, s making every
// exponent at least 0, and their distances compared exactly.


// d_tests_parts
//   struct: a value as m 2^e.
struct d_tests_parts
{
    struct d_tests_nat mantissa;  // m
    int                exponent;  // e
};

// d_tests_parts_of
//   _value, positive and finite, of a type of _precision bits, as m 2^e.
static struct d_tests_parts
d_tests_parts_of(
    long double _value,
    int         _precision
)
{
    struct d_tests_parts result   = { { { 0UL } }, 0 };
    int                  exponent = 0;
    long double          rest     = ldexpl(frexpl(_value, &exponent),
                                           _precision);
    int                  i        = 0;

    while ( (rest != 0.0L) &&
            (i < D_TESTS_NAT_LIMBS) )
    {
        const long double above = floorl(rest / 65536.0L);
        const long double limb  = rest - (above * 65536.0L);

        result.mantissa.limb[i] = (unsigned long)limb;
        rest                    = above;
        ++i;
    }

    result.exponent = exponent - _precision;

    return result;
}

// d_tests_past
//   2^MAX_EXP as m 2^e: the value past a type's largest finite one, which
// IEEE 754 rounds to an infinity where it is the nearer.
static struct d_tests_parts
d_tests_past(
    int _precision,
    int _max_exp
)
{
    struct d_tests_parts result = { { { 0UL } }, 0 };

    result.mantissa = d_tests_nat_pow2(_precision - 1);
    result.exponent = _max_exp - _precision + 1;

    return result;
}

// d_tests_threshold
//   the least natural a type of _precision bits rounds to an infinity:
// 2^MAX_EXP - 2^(MAX_EXP - P - 1), the largest finite value and half its
// last place; 0 where that is past the reference, and past every operand.
static struct d_tests_nat
d_tests_threshold(
    int _precision,
    int _max_exp
)
{
    const struct d_tests_nat none = { { 0UL } };

    if (_max_exp >= D_TESTS_NAT_BITS)
    {
        return none;
    }

    {
        const struct d_tests_nat top  = d_tests_nat_pow2(_max_exp);
        const struct d_tests_nat half =
            d_tests_nat_pow2(_max_exp - _precision - 1);

        return d_tests_nat_sub(&top, &half);
    }
}

// d_tests_place
//   m 2^(e + _s), e + _s at least 0.
static struct d_tests_nat
d_tests_place(
    const struct d_tests_parts* _v,
    int                         _s
)
{
    return d_tests_nat_shl(&_v->mantissa, _v->exponent + _s);
}

// d_tests_gap
//   |_a - _b|.
static struct d_tests_nat
d_tests_gap(
    const struct d_tests_nat* _a,
    const struct d_tests_nat* _b
)
{
    return (d_tests_nat_cmp(_a, _b) >= 0) ? d_tests_nat_sub(_a, _b)
                                          : d_tests_nat_sub(_b, _a);
}

// d_tests_nearest
//   whether _r is the nearest the positive natural _n of the three, _r,
// _below and _above, ties to even, and _direction how _r stands to _n.
static bool
d_tests_nearest(
    const struct d_tests_nat*   _n,
    const struct d_tests_parts* _r,
    const struct d_tests_parts* _below,
    const struct d_tests_parts* _above,
    int                         _direction
)
{
    const int lower = (_below->exponent < _r->exponent) ? _below->exponent
                                                        : _r->exponent;
    const int least = (_above->exponent < lower) ? _above->exponent : lower;
    const int scale = (least < 0) ? -least : 0;

    const struct d_tests_nat n     = d_tests_nat_shl(_n, scale);
    const struct d_tests_nat r     = d_tests_place(_r, scale);
    const struct d_tests_nat below = d_tests_place(_below, scale);
    const struct d_tests_nat above = d_tests_place(_above, scale);
    const struct d_tests_nat at    = d_tests_gap(&n, &r);
    const struct d_tests_nat to_b  = d_tests_gap(&n, &below);
    const struct d_tests_nat to_a  = d_tests_gap(&n, &above);
    const int  versus_below = d_tests_nat_cmp(&at, &to_b);
    const int  versus_above = d_tests_nat_cmp(&at, &to_a);
    const bool even         = ((_r->mantissa.limb[0] & 1UL) == 0UL);

    return ( (versus_below <= 0) &&
             (versus_above <= 0) &&
             ( ( (versus_below < 0) &&
                 (versus_above < 0) ) ||
               (even) ) &&
             (d_tests_nat_cmp(&r, &n) == _direction) );
}

// d_tests_check_f, d_tests_check_d, d_tests_check_ld
//   whether _r, and _direction, are what the unit must give for _x: 0 for 0,
// exactly; past the type's range, an infinity of _x's sign, above it; and
// otherwise the nearest value of _x's sign, the direction turned with it.

static bool
d_tests_check_f(
    const struct d_tests_int* _x,
    float                     _r,
    int                       _direction
)
{
    const int                sign      = (_x->negative) ? -1 : 1;
    const float              magnitude = (_x->negative) ? -_r : _r;
    const struct d_tests_nat threshold =
        d_tests_threshold(FLT_MANT_DIG, FLT_MAX_EXP);

    if (d_tests_nat_is_zero(&_x->mag))
    {
        return ( (_r == 0.0f) &&
                 (_direction == 0) );
    }

    if ( (!d_tests_nat_is_zero(&threshold)) &&
         (d_tests_nat_cmp(&_x->mag, &threshold) >= 0) )
    {
        return ( (magnitude > FLT_MAX) &&
                 (_direction == sign) );
    }

    if ( (!(magnitude > 0.0f)) ||
         (!(magnitude <= FLT_MAX)) )
    {
        return false;
    }

    {
        const float below = nextafterf(magnitude, 0.0f);
        const float above = nextafterf(magnitude, FLT_MAX);
        const struct d_tests_parts r_parts =
            d_tests_parts_of((long double)magnitude, FLT_MANT_DIG);
        const struct d_tests_parts b_parts =
            d_tests_parts_of((long double)below, FLT_MANT_DIG);
        const struct d_tests_parts a_parts =
            (above > magnitude)
                ? d_tests_parts_of((long double)above, FLT_MANT_DIG)
                : d_tests_past(FLT_MANT_DIG, FLT_MAX_EXP);

        return d_tests_nearest(&_x->mag, &r_parts, &b_parts, &a_parts,
                               _direction * sign);
    }
}

static bool
d_tests_check_d(
    const struct d_tests_int* _x,
    double                    _r,
    int                       _direction
)
{
    const int                sign      = (_x->negative) ? -1 : 1;
    const double             magnitude = (_x->negative) ? -_r : _r;
    const struct d_tests_nat threshold =
        d_tests_threshold(DBL_MANT_DIG, DBL_MAX_EXP);

    if (d_tests_nat_is_zero(&_x->mag))
    {
        return ( (_r == 0.0) &&
                 (_direction == 0) );
    }

    if ( (!d_tests_nat_is_zero(&threshold)) &&
         (d_tests_nat_cmp(&_x->mag, &threshold) >= 0) )
    {
        return ( (magnitude > DBL_MAX) &&
                 (_direction == sign) );
    }

    if ( (!(magnitude > 0.0)) ||
         (!(magnitude <= DBL_MAX)) )
    {
        return false;
    }

    {
        const double below = nextafter(magnitude, 0.0);
        const double above = nextafter(magnitude, DBL_MAX);
        const struct d_tests_parts r_parts =
            d_tests_parts_of((long double)magnitude, DBL_MANT_DIG);
        const struct d_tests_parts b_parts =
            d_tests_parts_of((long double)below, DBL_MANT_DIG);
        const struct d_tests_parts a_parts =
            (above > magnitude)
                ? d_tests_parts_of((long double)above, DBL_MANT_DIG)
                : d_tests_past(DBL_MANT_DIG, DBL_MAX_EXP);

        return d_tests_nearest(&_x->mag, &r_parts, &b_parts, &a_parts,
                               _direction * sign);
    }
}

static bool
d_tests_check_ld(
    const struct d_tests_int* _x,
    long double               _r,
    int                       _direction
)
{
    const int                sign      = (_x->negative) ? -1 : 1;
    const long double        magnitude = (_x->negative) ? -_r : _r;
    const struct d_tests_nat threshold =
        d_tests_threshold(LDBL_MANT_DIG, LDBL_MAX_EXP);

    if (d_tests_nat_is_zero(&_x->mag))
    {
        return ( (_r == 0.0L) &&
                 (_direction == 0) );
    }

    if ( (!d_tests_nat_is_zero(&threshold)) &&
         (d_tests_nat_cmp(&_x->mag, &threshold) >= 0) )
    {
        return ( (magnitude > LDBL_MAX) &&
                 (_direction == sign) );
    }

    if ( (!(magnitude > 0.0L)) ||
         (!(magnitude <= LDBL_MAX)) )
    {
        return false;
    }

    {
        const long double below = nextafterl(magnitude, 0.0L);
        const long double above = nextafterl(magnitude, LDBL_MAX);
        const struct d_tests_parts r_parts =
            d_tests_parts_of(magnitude, LDBL_MANT_DIG);
        const struct d_tests_parts b_parts =
            d_tests_parts_of(below, LDBL_MANT_DIG);
        const struct d_tests_parts a_parts =
            (above > magnitude)
                ? d_tests_parts_of(above, LDBL_MANT_DIG)
                : d_tests_past(LDBL_MANT_DIG, LDBL_MAX_EXP);

        return d_tests_nearest(&_x->mag, &r_parts, &b_parts, &a_parts,
                               _direction * sign);
    }
}


//==============================================================================
// 4.  THE OPERANDS
//==============================================================================
// Magnitudes, each with both signs: those taken in pairs, and more taken
// alone. Every magnitude is below 2^256.

static struct d_tests_int g_pairs[D_TESTS_PAIRS_MAX];
static int                g_pair_count = 0;
static struct d_tests_int g_ones[D_TESTS_ONES_MAX];
static int                g_one_count  = 0;
static bool               g_lists_held = true;

// d_tests_operand
//   _mag, with both signs, among the operands taken alone, and, where _pair,
// among those taken in pairs; a magnitude past 2^256 - 1 is left out.
static void
d_tests_operand(
    struct d_tests_nat _mag,
    bool               _pair
)
{
    int sign = 0;

    if (d_tests_nat_bits(&_mag) > 256)
    {
        return;
    }

    for (sign = 0; sign < 2; ++sign)
    {
        const struct d_tests_int x = d_tests_int_make(_mag, (sign == 1));

        // 0 once
        if ( (sign == 1) &&
             (d_tests_nat_is_zero(&_mag)) )
        {
            break;
        }

        if ( (g_one_count >= D_TESTS_ONES_MAX) ||
             ( (_pair) &&
               (g_pair_count >= D_TESTS_PAIRS_MAX) ) )
        {
            g_lists_held = false;

            return;
        }

        g_ones[g_one_count] = x;
        ++g_one_count;

        if (_pair)
        {
            g_pairs[g_pair_count] = x;
            ++g_pair_count;
        }
    }

    return;
}

// d_tests_about
//   2^_k + _delta, _delta from -2 to 2.
static struct d_tests_nat
d_tests_about(
    int _k,
    int _delta
)
{
    const struct d_tests_nat power = d_tests_nat_pow2(_k);
    const struct d_tests_nat delta =
        d_tests_nat_of((unsigned long)((_delta < 0) ? -_delta : _delta));

    return (_delta < 0) ? d_tests_nat_sub(&power, &delta)
                        : d_tests_nat_add(&power, &delta);
}

// d_tests_shifted
//   (2^_p + _c) 2^_k + _d, _c and _d from -1 to 3.
static struct d_tests_nat
d_tests_shifted(
    int _p,
    int _c,
    int _k,
    int _d
)
{
    const struct d_tests_nat base  = d_tests_about(_p, _c);
    const struct d_tests_nat moved = d_tests_nat_shl(&base, _k);
    const struct d_tests_nat delta =
        d_tests_nat_of((unsigned long)((_d < 0) ? -_d : _d));

    return (_d < 0) ? d_tests_nat_sub(&moved, &delta)
                    : d_tests_nat_add(&moved, &delta);
}

// d_tests_build_operands
//   the lists.
static void
d_tests_build_operands(void)
{
    static const unsigned long s_small[] =
    {
        0UL, 1UL, 2UL, 3UL, 10UL, 65535UL, 65536UL, 65537UL
    };
    static const int s_powers[] =
    {
        31, 32, 33, 63, 64, 65, 127, 128, 129, 192, 255
    };
    static const int s_alone[] =
    {
        15, 16, 17, 23, 24, 25, 47, 48, 52, 53, 54, 96, 112, 113, 114, 160,
        223, 224, 254
    };
    static const int s_shifts[] = { 0, 1, 7, 64, 100, 130 };

    const int precision[3] = { FLT_MANT_DIG, DBL_MANT_DIG, LDBL_MANT_DIG };

    struct d_tests_nat pattern = { { 0UL } };
    struct d_tests_nat ten     = d_tests_nat_of(1UL);
    const struct d_tests_nat ten_1 = d_tests_nat_of(10UL);
    const struct d_tests_nat one   = d_tests_nat_of(1UL);
    size_t i = 0u;
    size_t j = 0u;
    int    k = 0;

    // in pairs: small ones, about the powers, the top, and patterns
    for (i = 0u; i < (sizeof(s_small) / sizeof(s_small[0])); ++i)
    {
        d_tests_operand(d_tests_nat_of(s_small[i]), true);
    }

    for (i = 0u; i < (sizeof(s_powers) / sizeof(s_powers[0])); ++i)
    {
        for (k = -1; k <= 1; ++k)
        {
            d_tests_operand(d_tests_about(s_powers[i], k), true);
        }
    }

    d_tests_operand(d_tests_about(256, -1), true);
    d_tests_operand(d_tests_about(256, -2), true);
    d_tests_operand(d_tests_about(255, 1), true);

    // 0x5555..., 0xAAAA..., and ones in the top half only
    for (k = 0; k < 16; ++k)
    {
        pattern.limb[k] = 0x5555UL;
    }

    d_tests_operand(pattern, true);
    d_tests_operand(d_tests_nat_shl(&pattern, 1), true);

    {
        const struct d_tests_nat low  = d_tests_about(128, -1);
        const struct d_tests_nat high = d_tests_nat_shl(&low, 128);

        d_tests_operand(high, true);
    }

    // alone: about more powers
    for (i = 0u; i < (sizeof(s_alone) / sizeof(s_alone[0])); ++i)
    {
        for (k = -1; k <= 1; ++k)
        {
            d_tests_operand(d_tests_about(s_alone[i], k), false);
        }
    }

    // the powers of ten, and their neighbours
    for (k = 1; k <= 77; ++k)
    {
        ten = d_tests_nat_mul(&ten, &ten_1);

        d_tests_operand(ten, false);
        d_tests_operand(d_tests_nat_sub(&ten, &one), false);
        d_tests_operand(d_tests_nat_add(&ten, &one), false);
    }

    // each precision's ties, the values just past them, its runs of ones
    // that round up a binade, and its largest exact ones, at several places
    for (j = 0u; j < 3u; ++j)
    {
        const int p = precision[j];

        for (i = 0u; i < (sizeof(s_shifts) / sizeof(s_shifts[0])); ++i)
        {
            const int s = s_shifts[i];

            if ((p + 2 + s) > 256)
            {
                continue;
            }

            d_tests_operand(d_tests_shifted(p, 1, s, 0), false);
            d_tests_operand(d_tests_shifted(p, 3, s, 0), false);
            d_tests_operand(d_tests_shifted(p + 1, -1, s, 0), false);
            d_tests_operand(d_tests_shifted(p, -1, s, 0), false);

            if (s >= 1)
            {
                d_tests_operand(d_tests_shifted(p, 1, s, 1), false);
                d_tests_operand(d_tests_shifted(p, 3, s, -1), false);
            }
        }
    }

    // the float's range: its largest finite value, the least that rounds
    // to an infinity, and their neighbours
    {
        const struct d_tests_nat largest =
            d_tests_shifted(FLT_MANT_DIG, -1, FLT_MAX_EXP - FLT_MANT_DIG, 0);
        const struct d_tests_nat threshold =
            d_tests_threshold(FLT_MANT_DIG, FLT_MAX_EXP);

        d_tests_operand(largest, false);
        d_tests_operand(d_tests_nat_sub(&largest, &one), false);
        d_tests_operand(d_tests_nat_add(&largest, &one), false);
        d_tests_operand(threshold, false);
        d_tests_operand(d_tests_nat_sub(&threshold, &one), false);
        d_tests_operand(d_tests_nat_add(&threshold, &one), false);
    }

    // runs of ones: 2^k - 2^j
    for (k = 8; k <= 256; k = k + 31)
    {
        for (i = 1u; i < (size_t)k; i = i + 37u)
        {
            const struct d_tests_nat top = d_tests_nat_pow2(k);
            const struct d_tests_nat cut = d_tests_nat_pow2((int)i);

            d_tests_operand(d_tests_nat_sub(&top, &cut), false);
        }
    }

    return;
}


//==============================================================================
// 5.  ONE OPERAND
//==============================================================================
// Each operand taken alone: its signs, its form, its conversions and its
// text.

// d_tests_conversions
//   _x to the family's integers and rationals, and back.
static void
d_tests_conversions(
    const struct d_tests_int* _x,
    struct d_math_wide        _w
)
{
    const struct d_math_wide marker = d_tests_marker();
    const struct d_tests_nat most   =
        d_tests_nat_of_umax((d_math_umax)D_MATH_IMAX_MAX);
    const struct d_tests_nat one    = d_tests_nat_of(1UL);
    const struct d_tests_nat least  = d_tests_nat_add(&most, &one);
    const struct d_tests_nat most_u = d_tests_nat_of_umax(D_MATH_UMAX_MAX);
    const bool fits_i = (_x->negative)
                            ? (d_tests_nat_cmp(&_x->mag, &least) <= 0)
                            : (d_tests_nat_cmp(&_x->mag, &most) <= 0);
    const bool fits_u = ( (!_x->negative) &&
                          (d_tests_nat_cmp(&_x->mag, &most_u) <= 0) );

    d_math_imax             value_i  = 12345;
    d_math_umax             value_u  = 12345u;
    struct d_rational       rational = { 12345, 7 };
    struct d_math_wide      back     = marker;
    enum d_math_wide_status status   = D_MATH_WIDE_OK;

    // to d_math_imax, and back by from_imax
    status = d_math_wide_to_imax(&value_i, _w);

    if (fits_i)
    {
        const struct d_tests_int got = d_tests_int_of_imax(value_i);

        back = d_math_wide_from_imax(value_i);
        D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                        (d_tests_int_same(&got, _x)) &&
                        (d_tests_wide_same(&back, &_w)),
                        "to_imax and from_imax", _x, &g_none );
    }
    else
    {
        D_TESTS_EXPECT( (status == D_MATH_WIDE_OVERFLOW) &&
                        (value_i == 12345),
                        "to_imax outside the family", _x, &g_none );
    }

    // to d_math_umax, and back by from_umax
    status = d_math_wide_to_umax(&value_u, _w);

    if (fits_u)
    {
        const struct d_tests_nat got = d_tests_nat_of_umax(value_u);

        back = d_math_wide_from_umax(value_u);
        D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                        (d_tests_nat_cmp(&got, &_x->mag) == 0) &&
                        (d_tests_wide_same(&back, &_w)),
                        "to_umax and from_umax", _x, &g_none );
    }
    else
    {
        D_TESTS_EXPECT( (status == D_MATH_WIDE_OVERFLOW) &&
                        (value_u == 12345u),
                        "to_umax outside the family", _x, &g_none );
    }

    // to a rational, over 1, and back
    status = d_math_wide_to_rational(&rational, _w);

    if (fits_i)
    {
        const struct d_tests_int got = d_tests_int_of_imax(rational.num);

        back = marker;
        D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                        (rational.den == 1) &&
                        (d_tests_int_same(&got, _x)) &&
                        (d_math_wide_from_rational(&back, rational) ==
                         D_MATH_WIDE_OK) &&
                        (d_tests_wide_same(&back, &_w)),
                        "to_rational and from_rational", _x, &g_none );
    }
    else
    {
        D_TESTS_EXPECT( (status == D_MATH_WIDE_OVERFLOW) &&
                        (rational.num == 12345) &&
                        (rational.den == 7),
                        "to_rational outside the family", _x, &g_none );
    }

    return;
}

// d_tests_floating
//   _x to float, double and long double, each with its direction and
// without.
static void
d_tests_floating(
    const struct d_tests_int* _x,
    struct d_math_wide        _w
)
{
    int               dir_f  = 7;
    int               dir_d  = 7;
    int               dir_ld = 7;
    const float       f      = d_math_wide_to_f(_w, &dir_f);
    const double      d      = d_math_wide_to_d(_w, &dir_d);
    const long double ld     = d_math_wide_to_ld(_w, &dir_ld);

    D_TESTS_EXPECT(d_tests_check_f(_x, f, dir_f),
                   "to_f: the nearest float, and its direction",
                   _x, &g_none);
    D_TESTS_EXPECT(d_tests_check_d(_x, d, dir_d),
                   "to_d: the nearest double, and its direction",
                   _x, &g_none);
    D_TESTS_EXPECT(d_tests_check_ld(_x, ld, dir_ld),
                   "to_ld: the nearest long double, and its direction",
                   _x, &g_none);

    // no direction asked for: the same values
    D_TESTS_EXPECT( (d_math_wide_to_f(_w, NULL) == f) &&
                    (d_math_wide_to_d(_w, NULL) == d) &&
                    (d_math_wide_to_ld(_w, NULL) == ld),
                    "to_f, to_d, to_ld with no direction", _x, &g_none );

    return;
}

// d_tests_text_one
//   _x's text: the reference's digits, its length, and read back, also with
// a plus sign or leading zeros.
static void
d_tests_text_one(
    const struct d_tests_int* _x,
    struct d_math_wide        _w
)
{
    const struct d_math_wide marker = d_tests_marker();

    char                    expect[D_TESTS_TEXT];
    char                    text[D_MATH_WIDE_TEXT_SIZE + 8u];
    char                    padded[D_MATH_WIDE_TEXT_SIZE + 64u];
    struct d_math_wide      back   = marker;
    enum d_math_wide_status status = D_MATH_WIDE_OK;
    size_t                  length = 0u;
    size_t                  digits = 0u;
    size_t                  at     = 0u;
    size_t                  i      = 0u;

    (void)d_tests_int_text(_x, expect);
    length = d_math_wide_to_text(text, sizeof(text), _w);

    D_TESTS_EXPECT( (length == strlen(expect)) &&
                    (length < D_MATH_WIDE_TEXT_SIZE) &&
                    (strcmp(text, expect) == 0),
                    "to_text: the digits", _x, &g_none );

    // read back
    status = d_math_wide_from_text(&back, text, length);
    D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                    (d_tests_wide_same(&back, &_w)),
                    "from_text: read back", _x, &g_none );

    // a sign, plus where it has none, and forty leading zeros
    digits = (_x->negative) ? (length - 1u) : length;
    padded[at] = (_x->negative) ? '-' : '+';
    ++at;

    for (i = 0u; i < 40u; ++i)
    {
        padded[at] = '0';
        ++at;
    }

    for (i = 0u; i < digits; ++i)
    {
        padded[at] = text[(length - digits) + i];
        ++at;
    }

    back   = marker;
    status = d_math_wide_from_text(&back, padded, at);
    D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                    (d_tests_wide_same(&back, &_w)),
                    "from_text: a sign and leading zeros", _x, &g_none );

    return;
}

// d_tests_rationals
//   from_rational: an integer's, and a fraction's refused.
static void
d_tests_rationals(void)
{
    static const d_math_imax s_fractions[][2] =
    {
        { 1, 2 }, { -1, 2 }, { 7, 3 }, { -22, 7 }, { 1, 1000 }
    };

    const struct d_math_wide marker = d_tests_marker();

    struct d_math_wide      out    = marker;
    enum d_math_wide_status status = D_MATH_WIDE_OK;
    size_t                  i      = 0u;

    for (i = 0u; i < (sizeof(s_fractions) / sizeof(s_fractions[0])); ++i)
    {
        struct d_rational r = { 0, 1 };

        r.num  = s_fractions[i][0];
        r.den  = s_fractions[i][1];
        out    = marker;
        status = d_math_wide_from_rational(&out, r);
        D_TESTS_EXPECT( (status == D_MATH_WIDE_FRACTION) &&
                        (d_tests_wide_same(&out, &marker)),
                        "from_rational: a fraction", &g_none, &g_none );
    }

    // the family's ends, over 1
    {
        struct d_rational r = { D_MATH_IMAX_MIN, 1 };
        struct d_tests_int got  = g_none;
        struct d_tests_int want = d_tests_int_of_imax(D_MATH_IMAX_MIN);

        out    = marker;
        status = d_math_wide_from_rational(&out, r);
        got    = d_tests_of_wide(out);
        D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                        (d_tests_int_same(&got, &want)),
                        "from_rational: the least", &got, &want );

        r.num  = D_MATH_IMAX_MAX;
        want   = d_tests_int_of_imax(D_MATH_IMAX_MAX);
        out    = marker;
        status = d_math_wide_from_rational(&out, r);
        got    = d_tests_of_wide(out);
        D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                        (d_tests_int_same(&got, &want)),
                        "from_rational: the greatest", &got, &want );
    }

    return;
}

// d_tests_one
//   every check of one operand.
static void
d_tests_one(
    const struct d_tests_int* _x
)
{
    const struct d_math_wide w         = d_tests_to_wide(_x);
    const struct d_tests_int back      = d_tests_of_wide(w);
    const struct d_tests_int minus     = d_tests_int_neg(_x);
    const struct d_tests_int magnitude = d_tests_int_make(_x->mag, false);
    const struct d_math_wide neg       = d_math_wide_neg(w);
    const struct d_math_wide absolute  = d_math_wide_abs(w);
    const struct d_tests_int neg_got   = d_tests_of_wide(neg);
    const struct d_tests_int abs_got   = d_tests_of_wide(absolute);

    // the harness's own conversions
    D_TESTS_EXPECT(d_tests_int_same(&back, _x),
                   "the harness's conversions", _x, &g_none);

    // the one form, and the queries
    D_TESTS_EXPECT( (d_math_wide_is_valid(w)) &&
                    (d_math_wide_sign(w) == d_tests_int_sign(_x)) &&
                    ((int)d_math_wide_bits(w) == d_tests_nat_bits(&_x->mag)) &&
                    (d_math_wide_eq(w, w)) &&
                    (d_math_wide_cmp(w, w) == 0),
                    "is_valid, sign, bits, eq and cmp", _x, &g_none );

    // -x and |x|
    D_TESTS_EXPECT( (d_tests_wide_valid(neg)) &&
                    (d_tests_int_same(&neg_got, &minus)) &&
                    (d_tests_wide_valid(absolute)) &&
                    (d_tests_int_same(&abs_got, &magnitude)),
                    "neg and abs", _x, &g_none );

    // the gcd with 0, and with -x: |x|
    {
        const struct d_math_wide zero = d_math_wide_from_imax(0);
        const struct d_math_wide g0   = d_math_wide_gcd(w, zero);
        const struct d_math_wide g1   = d_math_wide_gcd(zero, w);
        const struct d_math_wide g2   = d_math_wide_gcd(w, neg);

        D_TESTS_EXPECT( (d_tests_wide_same(&g0, &absolute)) &&
                        (d_tests_wide_same(&g1, &absolute)) &&
                        (d_tests_wide_same(&g2, &absolute)),
                        "gcd with 0 and with -x", _x, &g_none );
    }

    d_tests_conversions(_x, w);
    d_tests_floating(_x, w);
    d_tests_text_one(_x, w);

    return;
}


//==============================================================================
// 6.  TWO OPERANDS
//==============================================================================

// d_tests_two
//   every check of a pair: the sum, the difference, the product, the
// quotient and remainder, the order, and, where _gcd, the gcd.
static void
d_tests_two(
    const struct d_tests_int* _a,
    const struct d_tests_int* _b,
    bool                      _gcd
)
{
    const struct d_math_wide a       = d_tests_to_wide(_a);
    const struct d_math_wide b       = d_tests_to_wide(_b);
    const struct d_math_wide marker  = d_tests_marker();
    const struct d_tests_int minus_b = d_tests_int_neg(_b);
    const int                order   = d_tests_int_cmp(_a, _b);

    struct d_math_wide      out        = marker;
    struct d_math_wide      rest       = marker;
    struct d_tests_int      exact      = g_none;
    struct d_tests_int      exact_rest = g_none;
    enum d_math_wide_status status     = D_MATH_WIDE_OK;

    // a + b, a - b, a * b
    exact  = d_tests_int_add(_a, _b);
    out    = marker;
    status = d_math_wide_add(&out, a, b);
    d_tests_result(status, &out, &exact, "add", _a, _b, __LINE__);

    exact  = d_tests_int_add(_a, &minus_b);
    out    = marker;
    status = d_math_wide_sub(&out, a, b);
    d_tests_result(status, &out, &exact, "sub", _a, _b, __LINE__);

    exact  = d_tests_int_mul(_a, _b);
    out    = marker;
    status = d_math_wide_mul(&out, a, b);
    d_tests_result(status, &out, &exact, "mul", _a, _b, __LINE__);

    // a / b: by 0, a pole, or 0/0 invalid, nothing stored
    if (d_tests_nat_is_zero(&_b->mag))
    {
        const enum d_math_wide_status want =
            (d_tests_nat_is_zero(&_a->mag)) ? D_MATH_WIDE_INVALID
                                            : D_MATH_WIDE_POLE;

        out    = marker;
        rest   = marker;
        status = d_math_wide_divmod(&out, &rest, a, b);
        D_TESTS_EXPECT( (status == want) &&
                        (d_tests_wide_same(&out, &marker)) &&
                        (d_tests_wide_same(&rest, &marker)) &&
                        (d_math_wide_divmod(NULL, NULL, a, b) == want),
                        "divmod by 0", _a, _b );
    }
    else
    {
        d_tests_int_divmod(_a, _b, &exact, &exact_rest);

        // the reference's own: a = q b + r, |r| below |b|
        {
            const struct d_tests_int product = d_tests_int_mul(&exact, _b);
            const struct d_tests_int whole   =
                d_tests_int_add(&product, &exact_rest);

            D_TESTS_EXPECT( (d_tests_int_same(&whole, _a)) &&
                            (d_tests_nat_cmp(&exact_rest.mag, &_b->mag) < 0),
                            "the reference's division", _a, _b );
        }

        {
            struct d_tests_int got_q = g_none;
            struct d_tests_int got_r = g_none;

            out    = marker;
            rest   = marker;
            status = d_math_wide_divmod(&out, &rest, a, b);
            got_q  = d_tests_of_wide(out);
            got_r  = d_tests_of_wide(rest);
            D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                            (d_tests_wide_valid(out)) &&
                            (d_tests_wide_valid(rest)) &&
                            (d_tests_int_same(&got_q, &exact)) &&
                            (d_tests_int_same(&got_r, &exact_rest)),
                            "divmod", _a, _b );

            // either destination alone
            out    = marker;
            status = d_math_wide_divmod(&out, NULL, a, b);
            got_q  = d_tests_of_wide(out);
            rest   = marker;
            D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                            (d_tests_int_same(&got_q, &exact)) &&
                            (d_math_wide_divmod(NULL, &rest, a, b) ==
                             D_MATH_WIDE_OK),
                            "divmod, the quotient alone", _a, _b );
            got_r = d_tests_of_wide(rest);
            D_TESTS_EXPECT(d_tests_int_same(&got_r, &exact_rest),
                           "divmod, the remainder alone", _a, _b);
        }
    }

    // the order
    D_TESTS_EXPECT( (d_math_wide_cmp(a, b) == order) &&
                    (d_math_wide_eq(a, b) == (order == 0)),
                    "cmp and eq", _a, _b );

    // the gcd, never negative
    if (_gcd)
    {
        const struct d_tests_int want =
            d_tests_int_make(d_tests_nat_gcd(&_a->mag, &_b->mag), false);
        const struct d_tests_int got =
            d_tests_of_wide(d_math_wide_gcd(a, b));

        D_TESTS_EXPECT(d_tests_int_same(&got, &want), "gcd", _a, _b);
    }

    return;
}

// d_tests_division_cases
//   naturals in pairs, below 2^_bits, that reach the corners of Knuth's
// division in the unit's limbs, b = 2^h, into _a and _b; returns how many.
//   The divisor added back: b^k over b^k / 2 + 1, whose guess, 2, passes the
// second limb's test, k from 3 limbs; the guess a limb's range and more,
// b + 1, and its correction cut short: a dividend of top limbs b/2, b - 2
// over a divisor of b/2, b - 1. Each also moved down a bit or a limb's
// worth, so that normalizing shifts it back, and up by whole limbs. The
// guess two too large, which only the second limb's test brings to one.
// Then runs of ones over runs of ones, and divisors whose top limb is 1.
static int
d_tests_division_cases(
    struct d_tests_nat* _a,
    struct d_tests_nat* _b,
    int                 _bits,
    int                 _capacity
)
{
    const int  h      = D_TESTS_H;
    const int  most   = _bits / h;
    const int  t[3]   = { 0, 1, h - 1 };
    const struct d_tests_nat one = d_tests_nat_of(1UL);
    int        count  = 0;
    int        k      = 0;
    int        m      = 0;
    int        i      = 0;
    int        z      = 0;

    // the divisor added back
    for (k = 3; k < most; ++k)
    {
        for (i = 0; i < 3; ++i)
        {
            const struct d_tests_nat half =
                d_tests_nat_pow2((k * h) - t[i] - 1);

            if (count < _capacity)
            {
                _a[count] = d_tests_nat_pow2((k * h) - t[i]);
                _b[count] = d_tests_nat_add(&half, &one);
                ++count;
            }
        }
    }

    // the guess b + 1, cut short
    for (m = 0; (m + 3) <= most; ++m)
    {
        for (z = 0; z < 3; ++z)
        {
            for (i = 0; i < 3; ++i)
            {
                struct d_tests_nat a = { { 0UL } };
                struct d_tests_nat b = { { 0UL } };
                const unsigned long low =
                    (z == 0) ? 0UL
                             : ( (z == 1) ? 1UL
                                          : ( (h == 16) ? 0xFFFFUL
                                                        : 0xFFFFFFFFUL ) );
                const struct d_tests_nat top_a = d_tests_nat_pow2(h - 1);
                const struct d_tests_nat next_a =
                    d_tests_about(h, -2);
                const struct d_tests_nat next_b =
                    d_tests_about(h, -1);
                struct d_tests_nat part = { { 0UL } };

                // a = [low, b - 2, b/2], b = [b - 1, b/2], in limbs of h
                a = d_tests_nat_shl(&top_a, 2 * h);
                part = d_tests_nat_shl(&next_a, h);
                a = d_tests_nat_add(&a, &part);
                part = d_tests_nat_of(low);
                a = d_tests_nat_add(&a, &part);
                b = d_tests_nat_shl(&top_a, h);
                b = d_tests_nat_add(&b, &next_b);

                a = d_tests_nat_shl(&a, m * h);
                b = d_tests_nat_shl(&b, m * h);
                a = d_tests_nat_shr(&a, t[i]);
                b = d_tests_nat_shr(&b, t[i]);

                if ( (count < _capacity) &&
                     (d_tests_nat_bits(&a) <= _bits) )
                {
                    _a[count] = a;
                    _b[count] = b;
                    ++count;
                }
            }
        }
    }

    // the guess two too large, which the second limb's test alone brings to
    // one too large: a dividend of top limbs b/2 - 1, b/2, 0, 0 over a
    // divisor of b/2, b/2 + 1, b - 1
    for (m = 0; (m + 4) <= most; ++m)
    {
        const struct d_tests_nat half      = d_tests_nat_pow2(h - 1);
        const struct d_tests_nat half_less = d_tests_about(h - 1, -1);
        const struct d_tests_nat half_more = d_tests_about(h - 1, 1);
        const struct d_tests_nat full      = d_tests_about(h, -1);
        struct d_tests_nat       a         = { { 0UL } };
        struct d_tests_nat       b         = { { 0UL } };
        struct d_tests_nat       part      = { { 0UL } };

        a    = d_tests_nat_shl(&half_less, 3 * h);
        part = d_tests_nat_shl(&half, 2 * h);
        a    = d_tests_nat_add(&a, &part);
        b    = d_tests_nat_shl(&half, 2 * h);
        part = d_tests_nat_shl(&half_more, h);
        b    = d_tests_nat_add(&b, &part);
        b    = d_tests_nat_add(&b, &full);

        if (count < _capacity)
        {
            _a[count] = d_tests_nat_shl(&a, m * h);
            _b[count] = d_tests_nat_shl(&b, m * h);
            ++count;
        }
    }

    // runs of ones, over runs of ones, and over divisors of top limb 1: of
    // one limb, two, and one and none fewer than the dividend
    for (k = 1; k <= most; ++k)
    {
        for (i = 1; i <= k; ++i)
        {
            if ( (i > 2) &&
                 (i < (k - 1)) )
            {
                continue;
            }

            const struct d_tests_nat top_k = d_tests_nat_pow2(
                (k * h < _bits) ? (k * h) : _bits);
            const struct d_tests_nat top_i = d_tests_nat_pow2(i * h);
            const struct d_tests_nat ones_k = d_tests_nat_sub(&top_k, &one);
            const struct d_tests_nat ones_i = d_tests_nat_sub(&top_i, &one);
            const struct d_tests_nat low_i  = d_tests_nat_pow2((i - 1) * h);

            if ((count + 1) < _capacity)
            {
                _a[count] = ones_k;
                _b[count] = ones_i;
                ++count;
                _a[count] = ones_k;
                _b[count] = d_tests_nat_add(&low_i, &one);
                ++count;
            }
        }
    }

    return count;
}

// d_tests_pairs
//   each pair of the operands taken in pairs.
static void
d_tests_pairs(void)
{
    int i = 0;
    int j = 0;

    for (i = 0; i < g_pair_count; ++i)
    {
        for (j = 0; j < g_pair_count; ++j)
        {
            d_tests_two(&g_pairs[i], &g_pairs[j], true);
        }
    }

    return;
}

// d_tests_divisions
//   the constructed divisions, with every sign.
static void
d_tests_divisions(void)
{
    static struct d_tests_nat s_a[D_TESTS_DIVISIONS_MAX];
    static struct d_tests_nat s_b[D_TESTS_DIVISIONS_MAX];

    int i     = 0;
    int j     = 0;
    int count = 0;

    count = d_tests_division_cases(s_a, s_b, 256, D_TESTS_DIVISIONS_MAX);
    D_TESTS_EXPECT(count < D_TESTS_DIVISIONS_MAX,
                   "the constructed divisions' list held them",
                   &g_none, &g_none);

    for (i = 0; i < count; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            const struct d_tests_int a = d_tests_int_make(s_a[i], (j & 1) != 0);
            const struct d_tests_int b = d_tests_int_make(s_b[i], (j & 2) != 0);

            d_tests_two(&a, &b, (j == 0));
        }
    }

    return;
}

// d_tests_random
//   pairs drawn at random, each also taken alone, and how often a product
// fit and how often it did not.
static void
d_tests_random(void)
{
    long t    = 0;
    long over = 0;
    long fits = 0;

    // each result, and how often each answer came
    for (t = 0; t < D_TESTS_TRIALS; ++t)
    {
        const struct d_tests_int a       = d_tests_random_int();
        const struct d_tests_int b       = d_tests_random_int();
        const struct d_tests_int product = d_tests_int_mul(&a, &b);

        d_tests_two(&a, &b, true);
        d_tests_one(&a);

        if (d_tests_int_fits(&product))
        {
            ++fits;
        }
        else
        {
            ++over;
        }
    }

    D_TESTS_EXPECT( (fits > (D_TESTS_TRIALS / 10L)) &&
                    (over > (D_TESTS_TRIALS / 10L)),
                    "the random pairs reach both answers of mul",
                    &g_none, &g_none );

    return;
}


//==============================================================================
// 7.  TEXT
//==============================================================================

// d_tests_text_status
//   _text read as a wide integer: its status, and, where refused, nothing
// stored.
static void
d_tests_text_status(
    const char*             _text,
    size_t                  _length,
    enum d_math_wide_status _want,
    const char*             _what
)
{
    const struct d_math_wide marker = d_tests_marker();

    struct d_math_wide            out    = marker;
    const enum d_math_wide_status status =
        d_math_wide_from_text(&out, _text, _length);

    if (d_tests_count( (status == _want) &&
                       ( (_want == D_MATH_WIDE_OK) ||
                         (d_tests_wide_same(&out, &marker)) ) ))
    {
        printf("  FAIL: %s, \"%s\": status %d\n", _what, _text, (int)status);
    }

    return;
}

// d_tests_capacity
//   _x's text into every room from 0 to two past its length: its whole
// length returned each time, as much as fits written, then the NUL, and
// nothing after.
static void
d_tests_capacity(
    const struct d_tests_int* _x
)
{
    const struct d_math_wide w = d_tests_to_wide(_x);

    char   expect[D_TESTS_TEXT];
    char   buffer[D_MATH_WIDE_TEXT_SIZE + 8u];
    size_t length   = 0u;
    size_t capacity = 0u;
    size_t i        = 0u;

    (void)d_tests_int_text(_x, expect);
    length = strlen(expect);

    for (capacity = 0u; capacity <= (length + 2u); ++capacity)
    {
        const size_t kept = (capacity == 0u)
                                ? 0u
                                : ( ((capacity - 1u) < length)
                                        ? (capacity - 1u) : length );
        bool         ok   = true;

        for (i = 0u; i < sizeof(buffer); ++i)
        {
            buffer[i] = '#';
        }

        ok = (d_math_wide_to_text(buffer, capacity, w) == length);

        // the characters that fit, the NUL, then nothing touched
        for (i = 0u; i < sizeof(buffer); ++i)
        {
            if (i < kept)
            {
                ok = ( (ok) &&
                       (buffer[i] == expect[i]) );
            }
            else if ( (i == kept) &&
                      (capacity > 0u) )
            {
                ok = ( (ok) &&
                       (buffer[i] == '\0') );
            }
            else
            {
                ok = ( (ok) &&
                       (buffer[i] == '#') );
            }
        }

        D_TESTS_EXPECT(ok, "to_text into a room too small", _x, &g_none);
    }

    // no buffer at all
    D_TESTS_EXPECT(d_math_wide_to_text(NULL, 0u, w) == length,
                   "to_text measuring", _x, &g_none);

    return;
}

// d_tests_text
//   texts refused and accepted, and texts cut short.
static void
d_tests_text(void)
{
    static const char* const s_syntax[] =
    {
        "", "+", "-", " 1", "1 ", "1a", "a1", "--1", "+-1", "-+1", "++1",
        "0x10", "1.0", "1e3", "1,000", "\t1", "1\n", "- 1", "1-", "+ 0",
        "0-", "\xd9\xa1"
    };
    static const char s_over[] =
        "1157920892373161954235709850086879078532699846656405640394575840079"
        "13129639936";
    static const char s_over_1[] =
        "1157920892373161954235709850086879078532699846656405640394575840079"
        "13129639937";
    static const char s_top[] =
        "1157920892373161954235709850086879078532699846656405640394575840079"
        "13129639935";

    char                    text[600];
    struct d_math_wide      out    = d_tests_marker();
    struct d_tests_int      x      = g_none;
    enum d_math_wide_status status = D_MATH_WIDE_OK;
    size_t                  i      = 0u;
    size_t                  n      = 0u;

    // no integer
    for (i = 0u; i < (sizeof(s_syntax) / sizeof(s_syntax[0])); ++i)
    {
        d_tests_text_status(s_syntax[i], strlen(s_syntax[i]),
                            D_MATH_WIDE_SYNTAX, "no integer");
    }

    // past 2^256 - 1, of either sign
    d_tests_text_status(s_over, strlen(s_over), D_MATH_WIDE_OVERFLOW,
                        "2^256");
    d_tests_text_status(s_over_1, strlen(s_over_1), D_MATH_WIDE_OVERFLOW,
                        "2^256 + 1");
    text[0] = '-';
    memcpy(text + 1, s_over, sizeof(s_over));
    d_tests_text_status(text, strlen(text), D_MATH_WIDE_OVERFLOW, "-2^256");

    // 10^78, 78 nines, and 500 digits
    text[0] = '1';

    for (i = 1u; i <= 78u; ++i)
    {
        text[i] = '0';
    }

    text[79] = '\0';
    d_tests_text_status(text, 79u, D_MATH_WIDE_OVERFLOW, "10^78");

    for (i = 0u; i < 78u; ++i)
    {
        text[i] = '9';
    }

    text[78] = '\0';
    d_tests_text_status(text, 78u, D_MATH_WIDE_OVERFLOW, "78 nines");

    for (i = 0u; i < 500u; ++i)
    {
        text[i] = (char)('1' + (int)(i % 9u));
    }

    text[500] = '\0';
    d_tests_text_status(text, 500u, D_MATH_WIDE_OVERFLOW, "500 digits");

    // no integer before too large: the syntax is found first
    text[500] = 'x';
    text[501] = '\0';
    d_tests_text_status(text, 501u, D_MATH_WIDE_SYNTAX,
                        "500 digits and an x");

    // 2^256 - 1 behind 500 zeros
    for (i = 0u; i < 500u; ++i)
    {
        text[i] = '0';
    }

    memcpy(text + 500, s_top, sizeof(s_top));
    n      = strlen(text);
    out    = d_tests_marker();
    status = d_math_wide_from_text(&out, text, n);
    x      = d_tests_of_wide(out);
    {
        const struct d_tests_int want =
            d_tests_int_make(d_tests_nat_of_text(s_top), false);

        D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                        (d_tests_int_same(&x, &want)),
                        "2^256 - 1 behind 500 zeros", &x, &want );
    }

    // zeros of every sign: 0, never negative
    {
        static const char* const s_zeros[] =
        {
            "0", "-0", "+0", "000", "-000", "+000"
        };

        for (i = 0u; i < (sizeof(s_zeros) / sizeof(s_zeros[0])); ++i)
        {
            out    = d_tests_marker();
            status = d_math_wide_from_text(&out, s_zeros[i],
                                           strlen(s_zeros[i]));
            x      = d_tests_of_wide(out);
            D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                            (d_tests_int_same(&x, &g_none)),
                            "zeros: 0, never negative", &x, &g_none );
        }
    }

    // the length bounds the text: what lies past it is not read
    out    = d_tests_marker();
    status = d_math_wide_from_text(&out, "12345", 3u);
    x      = d_tests_of_wide(out);
    {
        const struct d_tests_int want =
            d_tests_int_make(d_tests_nat_of(123UL), false);

        D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                        (d_tests_int_same(&x, &want)),
                        "the length bounds the text", &x, &want );
    }

    out    = d_tests_marker();
    status = d_math_wide_from_text(&out, "-12x", 3u);
    x      = d_tests_of_wide(out);
    {
        const struct d_tests_nat twelve = d_tests_nat_of(12UL);
        const struct d_tests_int want   = d_tests_int_make(twelve, true);

        D_TESTS_EXPECT( (status == D_MATH_WIDE_OK) &&
                        (d_tests_int_same(&x, &want)),
                        "the length bounds the text", &x, &want );
    }

    d_tests_text_status("123", 0u, D_MATH_WIDE_SYNTAX, "no characters");
    d_tests_text_status("-5", 1u, D_MATH_WIDE_SYNTAX, "a sign alone");

    // the room: 0, -1, the ends, and powers of ten
    for (i = 0u; i < (size_t)g_pair_count; ++i)
    {
        d_tests_capacity(&g_pairs[i]);
    }

    // the longest text: 79 characters, and the NUL
    {
        const struct d_tests_nat top  = d_tests_nat_of_text(s_top);
        const struct d_tests_int most = d_tests_int_make(top, true);
        const struct d_math_wide w    = d_tests_to_wide(&most);

        D_TESTS_EXPECT( (d_math_wide_to_text(NULL, 0u, w) + 1u) ==
                        D_MATH_WIDE_TEXT_SIZE,
                        "D_MATH_WIDE_TEXT_SIZE is the longest text's room",
                        &most, &g_none );
    }

    return;
}


//==============================================================================
// 8.  THE LIMB KERNELS
//==============================================================================
// The kernels on limbs, at lengths up to twice a wide integer's, against the
// reference: each written result, every limb past it untouched, and, where a
// kernel allows it, the destination an operand. Rounding by its own
// criterion: an integer x of n bits, rounded to P < n bits, must give a
// multiple R of 2^(n - P) with |x - R| at most half of 2^(n - P), on a tie
// the even multiple; and the direction must be R's order against x.

// d_tests_random_limbs
//   _n limbs: random, from runs of ones, or each one of the limbs that test
// Knuth's estimate (0, 1, 2, b/2 - 1, b/2, b/2 + 1, b - 2, b - 1).
static void
d_tests_random_limbs(
    d_math_wide_limb* _x,
    int               _n
)
{
    const int mode = d_tests_random_below(3);

    int i = 0;

    if (mode == 2)
    {
        for (i = 0; i < _n; ++i)
        {
            const int         kind  = d_tests_random_below(8);
            const d_math_umax half  =
                (d_math_umax)1 << (D_MATH_WIDE_LIMB_BITS - 1u);
            const d_math_umax full  = D_INTERNAL_MATH_WIDE_LIMB_MASK;
            d_math_umax       value = 0u;

            switch (kind)
            {
                case 0:  value = 0u;          break;
                case 1:  value = 1u;          break;
                case 2:  value = 2u;          break;
                case 3:  value = half - 1u;   break;
                case 4:  value = half;        break;
                case 5:  value = half + 1u;   break;
                case 6:  value = full - 1u;   break;
                default: value = full;        break;
            }

            _x[i] = (d_math_wide_limb)value;
        }

        return;
    }

    {
        const struct d_tests_nat x =
            d_tests_random_nat(d_tests_random_below((_n * D_TESTS_H) + 1),
                               (mode == 1));

        d_tests_limbs_of(&x, _x, _n);
    }

    return;
}

// d_tests_kernel_report
//   counts a check of a kernel, and reports the first failures with two
// naturals.
static void
d_tests_kernel_report(
    bool                      _ok,
    const char*               _what,
    const struct d_tests_nat* _a,
    const struct d_tests_nat* _b,
    int                       _line
)
{
    const struct d_tests_int a = d_tests_int_make(*_a, false);
    const struct d_tests_int b = d_tests_int_make(*_b, false);

    d_tests_expect(_ok, _what, &a, &b, _line);

    return;
}

#define D_TESTS_KERNEL(ok, what, a, b)                                         \
    d_tests_kernel_report((ok), (what), (a), (b), __LINE__)

// d_tests_kernel_sums
//   add and sub, at a random length, and in place.
static void
d_tests_kernel_sums(void)
{
    const int n = 1 + d_tests_random_below(D_TESTS_KERNEL_LIMBS);

    d_math_wide_limb a[D_TESTS_KERNEL_LIMBS + 1]   = { 0u };
    d_math_wide_limb b[D_TESTS_KERNEL_LIMBS + 1]   = { 0u };
    d_math_wide_limb out[D_TESTS_KERNEL_LIMBS + 1] = { 0u };
    d_math_wide_limb carry = 0u;

    d_tests_random_limbs(a, n);
    d_tests_random_limbs(b, n);
    out[n] = 0x5Au;

    {
        const struct d_tests_nat x     = d_tests_nat_of_limbs(a, n);
        const struct d_tests_nat y     = d_tests_nat_of_limbs(b, n);
        const struct d_tests_nat sum   = d_tests_nat_add(&x, &y);
        const struct d_tests_nat top   = d_tests_nat_pow2(n * D_TESTS_H);
        const bool               above = (d_tests_nat_cmp(&sum, &top) >= 0);
        const struct d_tests_nat low   =
            (above) ? d_tests_nat_sub(&sum, &top) : sum;
        const bool               below = (d_tests_nat_cmp(&x, &y) < 0);
        const struct d_tests_nat diff  =
            (below) ? d_tests_nat_add(&x, &top) : x;
        const struct d_tests_nat wrap  = d_tests_nat_sub(&diff, &y);
        struct d_tests_nat       got   = { { 0UL } };

        carry = d_internal_math_wide_add(out, a, b, (size_t)n);
        got   = d_tests_nat_of_limbs(out, n);
        D_TESTS_KERNEL( (carry == (above ? 1u : 0u)) &&
                        (d_tests_nat_cmp(&got, &low) == 0) &&
                        (out[n] == 0x5Au),
                        "the kernel add", &x, &y );

        carry = d_internal_math_wide_sub(out, a, b, (size_t)n);
        got   = d_tests_nat_of_limbs(out, n);
        D_TESTS_KERNEL( (carry == (below ? 1u : 0u)) &&
                        (d_tests_nat_cmp(&got, &wrap) == 0) &&
                        (out[n] == 0x5Au),
                        "the kernel sub", &x, &y );

        // in place: into the first operand, then the second
        (void)memcpy(out, a, sizeof(a[0]) * (size_t)n);
        (void)d_internal_math_wide_add(out, out, b, (size_t)n);
        got = d_tests_nat_of_limbs(out, n);
        D_TESTS_KERNEL(d_tests_nat_cmp(&got, &low) == 0,
                       "the kernel add in place", &x, &y);

        (void)memcpy(out, b, sizeof(b[0]) * (size_t)n);
        (void)d_internal_math_wide_sub(out, a, out, (size_t)n);
        got = d_tests_nat_of_limbs(out, n);
        D_TESTS_KERNEL(d_tests_nat_cmp(&got, &wrap) == 0,
                       "the kernel sub in place", &x, &y);

        // the order of equal lengths
        D_TESTS_KERNEL( (d_internal_math_wide_cmp(a, b, (size_t)n) ==
                         d_tests_nat_cmp(&x, &y)) &&
                        (d_internal_math_wide_cmp(a, a, (size_t)n) == 0),
                        "the kernel cmp", &x, &y );

        // the lengths and bits
        D_TESTS_KERNEL( ((int)d_internal_math_wide_bits(a, (size_t)n) ==
                         d_tests_nat_bits(&x)) &&
                        ((int)d_internal_math_wide_length(a, (size_t)n) ==
                         ((d_tests_nat_bits(&x) + D_TESTS_H - 1) /
                          D_TESTS_H)),
                        "the kernel length and bits", &x, &y );
    }

    return;
}

// d_tests_kernel_products
//   mul, mul_1, shl, shr and divmod_1, at random lengths, and in place where
// allowed.
static void
d_tests_kernel_products(void)
{
    const int na = d_tests_random_below(D_TESTS_N + 1);
    const int nb = d_tests_random_below(D_TESTS_N + 1);
    const int n  = 1 + d_tests_random_below(D_TESTS_KERNEL_LIMBS);

    d_math_wide_limb a[D_TESTS_KERNEL_LIMBS + 1]         = { 0u };
    d_math_wide_limb b[D_TESTS_KERNEL_LIMBS + 1]         = { 0u };
    d_math_wide_limb out[(2 * D_TESTS_KERNEL_LIMBS) + 1] = { 0u };
    d_math_wide_limb m[2]                                = { 0u, 0u };
    d_math_wide_limb result = 0u;
    unsigned         s      = 0u;

    d_tests_random_limbs(a, na);
    d_tests_random_limbs(b, nb);
    out[na + nb] = 0x5Au;

    // the product of two lengths, its limbs past them untouched
    {
        const struct d_tests_nat x    = d_tests_nat_of_limbs(a, na);
        const struct d_tests_nat y    = d_tests_nat_of_limbs(b, nb);
        const struct d_tests_nat want = d_tests_nat_mul(&x, &y);
        struct d_tests_nat       got  = { { 0UL } };

        d_internal_math_wide_mul(out, a, (size_t)na, b, (size_t)nb);
        got = d_tests_nat_of_limbs(out, na + nb);
        D_TESTS_KERNEL( (d_tests_nat_cmp(&got, &want) == 0) &&
                        (out[na + nb] == 0x5Au),
                        "the kernel mul", &x, &y );
    }

    // times a limb, plus a carry; then in place
    d_tests_random_limbs(a, n);
    d_tests_random_limbs(m, 2);
    out[n] = 0x5Au;
    {
        const struct d_tests_nat x     = d_tests_nat_of_limbs(a, n);
        const struct d_tests_nat by    = d_tests_nat_of_limbs(m, 1);
        const struct d_tests_nat plus  = d_tests_nat_of_limbs(m + 1, 1);
        const struct d_tests_nat part  = d_tests_nat_mul(&x, &by);
        const struct d_tests_nat want  = d_tests_nat_add(&part, &plus);
        struct d_tests_nat       got   = { { 0UL } };
        struct d_tests_nat       carry = { { 0UL } };
        struct d_tests_nat       whole = { { 0UL } };

        result = d_internal_math_wide_mul_1(out, a, (size_t)n, m[0], m[1]);
        got    = d_tests_nat_of_limbs(out, n);
        carry  = d_tests_nat_of_limbs(&result, 1);
        carry  = d_tests_nat_shl(&carry, n * D_TESTS_H);
        whole  = d_tests_nat_add(&got, &carry);
        D_TESTS_KERNEL( (d_tests_nat_cmp(&whole, &want) == 0) &&
                        (out[n] == 0x5Au),
                        "the kernel mul_1", &x, &by );

        (void)d_internal_math_wide_mul_1(a, a, (size_t)n, m[0], m[1]);
        whole = d_tests_nat_of_limbs(a, n);
        D_TESTS_KERNEL(d_tests_nat_cmp(&whole, &got) == 0,
                       "the kernel mul_1 in place", &x, &by);
    }

    // shifted up and down by less than a limb; in place
    d_tests_random_limbs(a, n);
    s = (unsigned)d_tests_random_below(D_TESTS_H);
    out[n] = 0x5Au;
    {
        const struct d_tests_nat x    = d_tests_nat_of_limbs(a, n);
        const struct d_tests_nat up   = d_tests_nat_shl(&x, (int)s);
        const struct d_tests_nat down = d_tests_nat_shr(&x, (int)s);
        const struct d_tests_nat by   = d_tests_nat_of((unsigned long)s);
        struct d_tests_nat       got  = { { 0UL } };
        struct d_tests_nat       top  = { { 0UL } };

        result = d_internal_math_wide_shl(out, a, (size_t)n, s);
        got    = d_tests_nat_of_limbs(out, n);
        top    = d_tests_nat_of_limbs(&result, 1);
        top    = d_tests_nat_shl(&top, n * D_TESTS_H);
        got    = d_tests_nat_add(&got, &top);
        D_TESTS_KERNEL( (d_tests_nat_cmp(&got, &up) == 0) &&
                        (out[n] == 0x5Au),
                        "the kernel shl", &x, &by );

        d_internal_math_wide_shr(out, a, (size_t)n, s);
        got = d_tests_nat_of_limbs(out, n);
        D_TESTS_KERNEL( (d_tests_nat_cmp(&got, &down) == 0) &&
                        (out[n] == 0x5Au),
                        "the kernel shr", &x, &by );

        (void)memcpy(out, a, sizeof(a[0]) * (size_t)n);
        (void)d_internal_math_wide_shl(out, out, (size_t)n, s);
        d_internal_math_wide_shr(out, out, (size_t)n, s);
        got = d_tests_nat_of_limbs(out, n);
        {
            const struct d_tests_nat range =
                d_tests_nat_pow2((n * D_TESTS_H) - (int)s);
            struct d_tests_nat       kept  = x;

            // the bits shifted out at the top are lost in place
            if (d_tests_nat_cmp(&x, &range) >= 0)
            {
                const struct d_tests_nat q = d_tests_nat_shr(&x,
                    (n * D_TESTS_H) - (int)s);
                const struct d_tests_nat cut = d_tests_nat_shl(&q,
                    (n * D_TESTS_H) - (int)s);

                kept = d_tests_nat_sub(&x, &cut);
            }

            D_TESTS_KERNEL(d_tests_nat_cmp(&got, &kept) == 0,
                           "the kernel shl and shr in place", &x, &by);
        }
    }

    // divided by a limb, not 0; in place
    d_tests_random_limbs(a, n);
    d_tests_random_limbs(m, 1);
    m[0] = (m[0] == 0u) ? (d_math_wide_limb)7u : m[0];
    out[n] = 0x5Au;
    {
        const struct d_tests_nat x    = d_tests_nat_of_limbs(a, n);
        const struct d_tests_nat by   = d_tests_nat_of_limbs(m, 1);
        struct d_tests_nat       q    = { { 0UL } };
        struct d_tests_nat       r    = { { 0UL } };
        struct d_tests_nat       got  = { { 0UL } };
        struct d_tests_nat       rest = { { 0UL } };

        d_tests_nat_divmod(&x, &by, &q, &r);
        result = d_internal_math_wide_divmod_1(out, a, (size_t)n, m[0]);
        got    = d_tests_nat_of_limbs(out, n);
        rest   = d_tests_nat_of_limbs(&result, 1);
        D_TESTS_KERNEL( (d_tests_nat_cmp(&got, &q) == 0) &&
                        (d_tests_nat_cmp(&rest, &r) == 0) &&
                        (out[n] == 0x5Au),
                        "the kernel divmod_1", &x, &by );

        result = d_internal_math_wide_divmod_1(a, a, (size_t)n, m[0]);
        got    = d_tests_nat_of_limbs(a, n);
        rest   = d_tests_nat_of_limbs(&result, 1);
        D_TESTS_KERNEL( (d_tests_nat_cmp(&got, &q) == 0) &&
                        (d_tests_nat_cmp(&rest, &r) == 0),
                        "the kernel divmod_1 in place", &x, &by );
    }

    return;
}

// d_tests_kernel_divide
//   divmod of _a by _b, naturals of _na and _nb limbs, _b's top limb not 0:
// the quotient and remainder, the limbs past them and past the work
// untouched; then again with the quotient in _a's limbs and the remainder
// in _b's.
static void
d_tests_kernel_divide(
    const struct d_tests_nat* _a,
    const struct d_tests_nat* _b,
    int                       _na,
    int                       _nb
)
{
    const int nq = _na - _nb + 1;

    d_math_wide_limb a[D_TESTS_KERNEL_LIMBS + 1]          = { 0u };
    d_math_wide_limb b[D_TESTS_KERNEL_LIMBS + 1]          = { 0u };
    d_math_wide_limb q[D_TESTS_KERNEL_LIMBS + 2]          = { 0u };
    d_math_wide_limb r[D_TESTS_KERNEL_LIMBS + 2]          = { 0u };
    d_math_wide_limb work[(2 * D_TESTS_KERNEL_LIMBS) + 2] = { 0u };
    struct d_tests_nat want_q = { { 0UL } };
    struct d_tests_nat want_r = { { 0UL } };
    struct d_tests_nat got_q  = { { 0UL } };
    struct d_tests_nat got_r  = { { 0UL } };

    d_tests_limbs_of(_a, a, _na);
    d_tests_limbs_of(_b, b, _nb);
    d_tests_nat_divmod(_a, _b, &want_q, &want_r);

    q[nq]               = 0x5Au;
    r[_nb]              = 0x5Au;
    work[_na + _nb + 1] = 0x5Au;
    d_internal_math_wide_divmod(q, r, a, (size_t)_na, b, (size_t)_nb, work);
    got_q = d_tests_nat_of_limbs(q, nq);
    got_r = d_tests_nat_of_limbs(r, _nb);
    D_TESTS_KERNEL( (d_tests_nat_cmp(&got_q, &want_q) == 0) &&
                    (d_tests_nat_cmp(&got_r, &want_r) == 0) &&
                    (q[nq] == 0x5Au) &&
                    (r[_nb] == 0x5Au) &&
                    (work[_na + _nb + 1] == 0x5Au),
                    "the kernel divmod", _a, _b );

    // the quotient into the dividend's limbs, the remainder the divisor's
    d_internal_math_wide_divmod(a, b, a, (size_t)_na, b, (size_t)_nb, work);
    got_q = d_tests_nat_of_limbs(a, nq);
    got_r = d_tests_nat_of_limbs(b, _nb);
    D_TESTS_KERNEL( (d_tests_nat_cmp(&got_q, &want_q) == 0) &&
                    (d_tests_nat_cmp(&got_r, &want_r) == 0),
                    "the kernel divmod into its operands", _a, _b );

    return;
}

// d_tests_kernel_round
//   _x, not 0, of _n limbs, rounded to _precision bits, by the criterion
// above.
static void
d_tests_kernel_round(
    const struct d_tests_nat* _x,
    int                       _n,
    int                       _precision
)
{
    d_math_wide_limb x[D_TESTS_KERNEL_LIMBS + 1] = { 0u };
    const struct d_tests_nat precision =
        d_tests_nat_of((unsigned long)_precision);
    struct d_internal_rational_rounded r = { { 0u, 0u }, 0, 0 };
    bool ok = false;

    d_tests_limbs_of(_x, x, _n);
    r = d_internal_math_wide_round(x, (size_t)_n, (unsigned)_precision);

    {
        const struct d_tests_nat hi = d_tests_nat_of_umax(r.mantissa.hi);
        const struct d_tests_nat lo = d_tests_nat_of_umax(r.mantissa.lo);
        const struct d_tests_nat up =
            d_tests_nat_shl(&hi, (int)D_INTERNAL_RATIONAL_WORD_BITS);
        const struct d_tests_nat m    = d_tests_nat_add(&up, &lo);
        const int                bits = d_tests_nat_bits(_x);

        // P bits, the top one set
        ok = (d_tests_nat_bits(&m) == _precision);

        // no more bits than P: exact, moved up
        if (bits <= _precision)
        {
            const struct d_tests_nat moved =
                d_tests_nat_shl(_x, _precision - bits);

            ok = ( (ok) &&
                   (r.exponent == (bits - _precision)) &&
                   (d_tests_nat_cmp(&m, &moved) == 0) &&
                   (r.direction == 0) );
        }
        // a multiple of 2^E, E = bits - P, within half of it, a tie even
        else if ( (r.exponent != (bits - _precision)) &&
                  (r.exponent != (bits - _precision + 1)) )
        {
            ok = false;
        }
        else
        {
            const int                place = bits - _precision;
            const struct d_tests_nat value = d_tests_nat_shl(&m, r.exponent);
            const struct d_tests_nat gap   = d_tests_gap(_x, &value);
            const struct d_tests_nat twice = d_tests_nat_shl(&gap, 1);
            const struct d_tests_nat unit  = d_tests_nat_pow2(place);
            const struct d_tests_nat units = d_tests_nat_shr(&value, place);
            const int                versus = d_tests_nat_cmp(&twice, &unit);
            const bool               even  = ((units.limb[0] & 1UL) == 0UL);

            ok = ( (ok) &&
                   ( (versus < 0) ||
                     ( (versus == 0) &&
                       (even) ) ) &&
                   (d_tests_nat_cmp(&value, _x) == r.direction) );
        }
    }

    D_TESTS_KERNEL(ok, "the kernel round", _x, &precision);

    return;
}

// d_tests_kernels
//   each kernel at random lengths, the constructed divisions at every length
// up to 512 bits, and rounding at every precision two words hold.
static void
d_tests_kernels(void)
{
    static struct d_tests_nat s_a[D_TESTS_DIVISIONS_MAX];
    static struct d_tests_nat s_b[D_TESTS_DIVISIONS_MAX];

    const int width     = (int)D_INTERNAL_RATIONAL_WORD_BITS;
    const int precision[8] =
    {
        1, 2, 3, FLT_MANT_DIG, DBL_MANT_DIG, LDBL_MANT_DIG, 113, 2 * width
    };

    int  count = 0;
    int  i     = 0;
    long t     = 0;

    for (t = 0; t < D_TESTS_KERNEL_TRIALS; ++t)
    {
        d_math_wide_limb a[D_TESTS_KERNEL_LIMBS + 1] = { 0u };
        d_math_wide_limb b[D_TESTS_KERNEL_LIMBS + 1] = { 0u };
        const int        na = 1 + d_tests_random_below(D_TESTS_KERNEL_LIMBS);
        const int        nb = 1 + d_tests_random_below(na);
        const int        p  = precision[d_tests_random_below(8)];

        d_tests_kernel_sums();
        d_tests_kernel_products();

        // a division: the divisor's top limb not 0
        d_tests_random_limbs(a, na);
        d_tests_random_limbs(b, nb);
        b[nb - 1] = (b[nb - 1] == 0u) ? (d_math_wide_limb)1u : b[nb - 1];
        {
            const struct d_tests_nat x = d_tests_nat_of_limbs(a, na);
            const struct d_tests_nat y = d_tests_nat_of_limbs(b, nb);

            d_tests_kernel_divide(&x, &y, na, nb);
        }

        // a rounding: not 0, to a precision two words hold
        d_tests_random_limbs(a, na);
        a[na - 1] = (a[na - 1] == 0u) ? (d_math_wide_limb)3u : a[na - 1];

        if (p <= (2 * width))
        {
            const struct d_tests_nat x = d_tests_nat_of_limbs(a, na);

            d_tests_kernel_round(&x, na, p);
        }
    }

    // the constructed divisions, at every length up to 512 bits
    count = d_tests_division_cases(s_a, s_b, 2 * 256, D_TESTS_DIVISIONS_MAX);
    D_TESTS_EXPECT(count < D_TESTS_DIVISIONS_MAX,
                   "the constructed divisions' list held them",
                   &g_none, &g_none);

    for (i = 0; i < count; ++i)
    {
        const int na = (d_tests_nat_bits(&s_a[i]) + D_TESTS_H - 1) / D_TESTS_H;
        const int nb = (d_tests_nat_bits(&s_b[i]) + D_TESTS_H - 1) / D_TESTS_H;

        if (na >= nb)
        {
            d_tests_kernel_divide(&s_a[i], &s_b[i], na, nb);
        }
    }

    // every precision on the ties and runs of ones of each, at 512 bits too
    for (i = 0; i < 8; ++i)
    {
        const int p = precision[i];
        int       k = 0;

        if (p > (2 * width))
        {
            continue;
        }

        for (k = 0; (p + 2 + k) <= 512; k = k + 61)
        {
            const struct d_tests_nat tie   = d_tests_shifted(p, 1, k, 0);
            const struct d_tests_nat tie_3 = d_tests_shifted(p, 3, k, 0);
            const struct d_tests_nat ones  = d_tests_shifted(p + 1, -1, k, 0);
            const struct d_tests_nat exact = d_tests_shifted(p, -1, k, 0);

            d_tests_kernel_round(&tie,
                                 (d_tests_nat_bits(&tie) + D_TESTS_H - 1) /
                                     D_TESTS_H, p);
            d_tests_kernel_round(&tie_3,
                                 (d_tests_nat_bits(&tie_3) + D_TESTS_H - 1) /
                                     D_TESTS_H, p);
            d_tests_kernel_round(&ones,
                                 (d_tests_nat_bits(&ones) + D_TESTS_H - 1) /
                                     D_TESTS_H, p);
            d_tests_kernel_round(&exact,
                                 (d_tests_nat_bits(&exact) + D_TESTS_H - 1) /
                                     D_TESTS_H, p);
        }
    }

    return;
}


//==============================================================================
// 9.  CONSTANT EVALUATION
//==============================================================================
// From C++14 every kernel is a constant expression, the ones that store
// through a pointer included, where it points at the evaluation's own
// object; text is read from a literal.

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

#define D_TESTS_CX_TEXT(text)                                                  \
    d_tests_constexpr_text((text), sizeof(text) - 1u)

// d_tests_constexpr_text
//   a literal's integer.
static constexpr struct d_math_wide
d_tests_constexpr_text(
    const char* _text,
    size_t      _length
)
{
    struct d_math_wide out = { { 0u }, false };

    (void)d_math_wide_from_text(&out, _text, _length);

    return out;
}

// d_tests_constexpr_product
//   (2^128 - 1)(2^128 + 1).
static constexpr struct d_math_wide
d_tests_constexpr_product()
{
    struct d_math_wide out = { { 0u }, false };

    (void)d_math_wide_mul(&out,
                          D_TESTS_CX_TEXT(
                              "340282366920938463463374607431768211455"),
                          D_TESTS_CX_TEXT(
                              "340282366920938463463374607431768211457"));

    return out;
}

// d_tests_constexpr_overflow
//   (2^256 - 1) + 1's status.
static constexpr enum d_math_wide_status
d_tests_constexpr_overflow()
{
    struct d_math_wide out = { { 0u }, false };

    return d_math_wide_add(&out,
                           D_TESTS_CX_TEXT(
                               "115792089237316195423570985008687907853"
                               "269984665640564039457584007913129639935"),
                           d_math_wide_from_imax(1));
}

// d_tests_constexpr_quotient, d_tests_constexpr_remainder
//   (2^256 - 1) / (10^38 + 7), and -(3^160) / 7.
static constexpr struct d_math_wide
d_tests_constexpr_quotient()
{
    struct d_math_wide q = { { 0u }, false };

    (void)d_math_wide_divmod(&q, NULL,
                             D_TESTS_CX_TEXT(
                                 "115792089237316195423570985008687907853"
                                 "269984665640564039457584007913129639935"),
                             D_TESTS_CX_TEXT(
                                 "100000000000000000000000000000000000007"));

    return q;
}

static constexpr struct d_math_wide
d_tests_constexpr_remainder()
{
    struct d_math_wide r = { { 0u }, false };

    (void)d_math_wide_divmod(NULL, &r,
                             D_TESTS_CX_TEXT(
                                 "-21847450052839212624230656502990235142"
                                 "567050104912751880812823948662932355201"),
                             d_math_wide_from_imax(7));

    return r;
}

// d_tests_constexpr_gcd
//   gcd(3 2^200, 9 2^100).
static constexpr struct d_math_wide
d_tests_constexpr_gcd()
{
    return d_math_wide_gcd(
        D_TESTS_CX_TEXT("482081413277697082662588627702348780756660898134"
                        "8378505904128"),
        D_TESTS_CX_TEXT("11408855402054064613470328848384"));
}

// d_tests_constexpr_d, d_tests_constexpr_f, d_tests_constexpr_ld
//   2^256 - 1 to double, 2^128 to float, -(2^64 + 1) to long double, and
// each one's direction.
static constexpr double
d_tests_constexpr_d()
{
    return d_math_wide_to_d(
        D_TESTS_CX_TEXT("115792089237316195423570985008687907853"
                        "269984665640564039457584007913129639935"),
        NULL);
}

static constexpr int
d_tests_constexpr_d_direction()
{
    int direction = 7;

    (void)d_math_wide_to_d(
        D_TESTS_CX_TEXT("115792089237316195423570985008687907853"
                        "269984665640564039457584007913129639935"),
        &direction);

    return direction;
}

static constexpr float
d_tests_constexpr_f()
{
    return d_math_wide_to_f(
        D_TESTS_CX_TEXT("340282366920938463463374607431768211456"), NULL);
}

static constexpr long double
d_tests_constexpr_ld()
{
    return d_math_wide_to_ld(D_TESTS_CX_TEXT("-18446744073709551617"), NULL);
}

static constexpr int
d_tests_constexpr_ld_direction()
{
    int direction = 7;

    (void)d_math_wide_to_ld(D_TESTS_CX_TEXT("-18446744073709551617"),
                            &direction);

    return direction;
}

// d_tests_constexpr_syntax
//   "12a"'s status.
static constexpr enum d_math_wide_status
d_tests_constexpr_syntax()
{
    struct d_math_wide out = { { 0u }, false };

    return d_math_wide_from_text(&out, "12a", 3u);
}

// d_tests_constexpr_least
//   the family's least, there and back.
static constexpr d_math_imax
d_tests_constexpr_least()
{
    d_math_imax out = 0;

    (void)d_math_wide_to_imax(&out, d_math_wide_from_imax(D_MATH_IMAX_MIN));

    return out;
}

static_assert(d_math_wide_eq(d_tests_constexpr_product(),
                             D_TESTS_CX_TEXT(
                                 "115792089237316195423570985008687907853"
                                 "269984665640564039457584007913129639935")),
              "a product of 256 bits as a constant expression");
static_assert(d_tests_constexpr_overflow() == D_MATH_WIDE_OVERFLOW,
              "an overflow as a constant expression");
static_assert(d_math_wide_eq(d_tests_constexpr_quotient(),
                             D_TESTS_CX_TEXT(
                                 "1157920892373161954235709850086879078451")),
              "Knuth's division as a constant expression");
static_assert(d_math_wide_eq(d_tests_constexpr_remainder(),
                             d_math_wide_from_imax(-4)),
              "a remainder with the dividend's sign as a constant expression");
static_assert(d_math_wide_eq(d_tests_constexpr_gcd(),
                             D_TESTS_CX_TEXT(
                                 "3802951800684688204490109616128")),
              "a gcd as a constant expression");
static_assert( (d_tests_constexpr_d() == 1.157920892373162e77) &&
               (d_tests_constexpr_d_direction() == 1),
               "2^256 - 1 to double, rounded up, as a constant expression");
static_assert(d_tests_constexpr_f() > FLT_MAX,
              "2^128 to float, an infinity, as a constant expression");
static_assert( (LDBL_MANT_DIG >= 65)
                   ? ( (d_tests_constexpr_ld() == -18446744073709551617.0L) &&
                       (d_tests_constexpr_ld_direction() == 0) )
                   : ( (d_tests_constexpr_ld() == -18446744073709551616.0L) &&
                       (d_tests_constexpr_ld_direction() == 1) ),
               "-(2^64 + 1) to long double as a constant expression");
static_assert(d_tests_constexpr_syntax() == D_MATH_WIDE_SYNTAX,
              "a text refused as a constant expression");
static_assert(d_math_wide_cmp(D_TESTS_CX_TEXT("-12"),
                              d_math_wide_from_imax(3)) == -1,
              "an order as a constant expression");
static_assert(d_tests_constexpr_least() == D_MATH_IMAX_MIN,
              "the family's least there and back as a constant expression");

// d_tests_constant_evaluation
//   the same at run time.
static void
d_tests_constant_evaluation(void)
{
    const struct d_math_wide top =
        D_TESTS_CX_TEXT("115792089237316195423570985008687907853"
                        "269984665640564039457584007913129639935");
    const struct d_math_wide product   = d_tests_constexpr_product();
    const struct d_math_wide quotient  = d_tests_constexpr_quotient();
    const struct d_math_wide remainder = d_tests_constexpr_remainder();
    const struct d_math_wide gcd       = d_tests_constexpr_gcd();
    const bool ld_ok = (LDBL_MANT_DIG >= 65)
        ? ( (d_tests_constexpr_ld() == -18446744073709551617.0L) &&
            (d_tests_constexpr_ld_direction() == 0) )
        : ( (d_tests_constexpr_ld() == -18446744073709551616.0L) &&
            (d_tests_constexpr_ld_direction() == 1) );

    D_TESTS_EXPECT( (d_math_wide_eq(product, top)) &&
                    (d_tests_constexpr_overflow() == D_MATH_WIDE_OVERFLOW) &&
                    (d_math_wide_eq(quotient,
                         D_TESTS_CX_TEXT(
                             "1157920892373161954235709850086879078451"))) &&
                    (d_math_wide_eq(remainder, d_math_wide_from_imax(-4))) &&
                    (d_math_wide_eq(gcd,
                         D_TESTS_CX_TEXT(
                             "3802951800684688204490109616128"))) &&
                    (d_tests_constexpr_d() == 1.157920892373162e77) &&
                    (d_tests_constexpr_d_direction() == 1) &&
                    (d_tests_constexpr_f() > FLT_MAX) &&
                    (ld_ok) &&
                    (d_tests_constexpr_syntax() == D_MATH_WIDE_SYNTAX) &&
                    (d_tests_constexpr_least() == D_MATH_IMAX_MIN),
                    "the constant expressions, at run time",
                    &g_none, &g_none );

    return;
}

#endif  // C++14


int
main(void)
{
    int i = 0;

    d_tests_layout();
    d_tests_build_operands();
    D_TESTS_EXPECT(g_lists_held, "the operands' lists held them",
                   &g_none, &g_none);

    for (i = 0; i < g_one_count; ++i)
    {
        d_tests_one(&g_ones[i]);
    }

    d_tests_pairs();
    d_tests_divisions();
    d_tests_random();
    d_tests_text();
    d_tests_rationals();
    d_tests_kernels();
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    // the reference never needed more than its bits
    D_TESTS_EXPECT(!g_overrun, "the reference's 640 bits held",
                   &g_none, &g_none);

    printf("t_wide: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
