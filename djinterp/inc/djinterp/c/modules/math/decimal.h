/*******************************************************************************
* djinterp [c/math]                                                    decimal.h
*
* Decimal text to binary floating point: the float, double or long double
* nearest a decimal number of any length, ties to even, whatever the
* environment's rounding mode. The C core's strtof, strtod and strtold, with
* one answer on every build, and constant expressions from C++14.
*   THE TEXT is one decimal number and nothing else: an optional sign; digits
* with an optional decimal point, at least one digit on one side of it; and
* an optional exponent, e or E, an optional sign and at least one digit. No
* space, no hexadecimal, no infinity or NaN: those are other tokens'
* business. d_math_decimal_length measures such a number at the start of a
* longer text, for a tokenizer.
*   THE ALGORITHM is Simple Decimal Conversion, as in Go's strconv and in
* Wuffs. The number is kept as decimal digits and multiplied or divided by
* powers of two, in decimal, until it lies in [1/2, 1); then its first P
* bits, P the type's precision, are read off and rounded to nearest, ties to
* even, by the digits that follow. A digit past the buffer's capacity is
* dropped, and a flag records whether any dropped digit was nonzero, which
* settles a tie. The capacity is more than the significant digits of any
* value halfway between two adjacent long doubles, so the rounding is exact
* for every input, however long.
*   A result below the normal range is rounded on the subnormal grid; a
* result past the largest finite value is an infinity. Both are reported:
* D_MATH_DECIMAL_UNDERFLOW for a subnormal or zero that is not exact,
* D_MATH_DECIMAL_OVERFLOW for an infinity.
*   COST. The digits are worked on in place, in a buffer of that capacity on
* the stack: about 11.5 KB where long double is the x87's. Time grows with
* the digits and with the exponent: under a microsecond for a usual literal,
* tens of microseconds at the ends of a double's range, and milliseconds at
* the ends of a long double's. In constant evaluation, a double's ends are
* within GCC's and clang's default limits; a long double's are not
* (-fconstexpr-ops-limit, -fconstexpr-steps).
*
*
* path:      /inc/djinterp/c/modules/math/decimal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LAYOUT AND STATUS
    -----------------
    1.  Status
         1.  dj_math_decimal_status
    2.  The digits
         1.  D_INTERNAL_MATH_DECIMAL_DIGITS
         2.  D_INTERNAL_MATH_DECIMAL_MAX_SHIFT
         3.  D_INTERNAL_MATH_DECIMAL_PLACE_MAX
         4.  dj_internal_math_decimal
         5.  dj_internal_math_decimal_bits
2.  THE DIGITS' ARITHMETIC
    ----------------------
    1.  Shifts
    2.  The integer part
3.  READING
    -------
4.  ROUNDING TO A TYPE
    ------------------
5.  THE CONVERSIONS
    ---------------
    1.  d_math_decimal_length
    2.  d_math_decimal_to_f, d_math_decimal_to_d, d_math_decimal_to_ld
*/

#ifndef DJINTERP_C_MATH_DECIMAL_H
#define DJINTERP_C_MATH_DECIMAL_H 1

// std
#include <float.h>           // FLT_MANT_DIG ... LDBL_MAX_10_EXP
#include <math.h>            // HUGE_VAL
#include <stddef.h>          // size_t, NULL
// djinterp
#include "./math_common.h"   // D_MATH_FN, dj_math_umax
#include "./float_order.h"   // d_internal_math_f_pow2 ... _ld_pow2
#include "./rational.h"      // the two-word helpers


//==============================================================================
// 1.  LAYOUT AND STATUS
//==============================================================================


// 1.1    Status
//------------------------------------------------------------------------------
// 1.1.1
// dj_math_decimal_status
//   enum: what a conversion came to. Only D_MATH_DECIMAL_SYNTAX stores
// nothing.
enum dj_math_decimal_status
{
    D_MATH_DECIMAL_OK        = 0,  // the nearest value, stored
    D_MATH_DECIMAL_SYNTAX    = 1,  // the text is no decimal number
    D_MATH_DECIMAL_OVERFLOW  = 2,  // past the largest finite value: an
                                   // infinity, stored
    D_MATH_DECIMAL_UNDERFLOW = 3   // a subnormal or zero, not exact,
                                   // stored
};

// 1.2    The digits
//------------------------------------------------------------------------------
// 1.2.1
// D_INTERNAL_MATH_DECIMAL_DIGITS
//   constant: the buffer's capacity. A value halfway between two adjacent
// long doubles is (2m + 1) 2^(q - 1), with 2m + 1 below 2^(P + 1) and q at
// least emin - P, P = LDBL_MANT_DIG and emin = LDBL_MIN_EXP; so its
// significant digits, those of (2m + 1) 5^(P + 1 - emin) at most, number
// no more than (P + 1) log10(2) + (P + 1 - emin) log10(5), plus one. One
// more for safety: 11,516 for the x87's long double, 769 where long double
// is double.
#define D_INTERNAL_MATH_DECIMAL_DIGITS                                         \
    ( ( ( (((long)LDBL_MANT_DIG + 1L) * 30103L) +                              \
          (((long)LDBL_MANT_DIG + 1L - (long)LDBL_MIN_EXP) * 69897L) ) /       \
        100000L ) + 2L )

// 1.2.2
// D_INTERNAL_MATH_DECIMAL_MAX_SHIFT
//   constant: the most bits one shift moves. A digit times 2^k, plus a
// carry below 2^k, is below 10 * 2^k, which must fit a word.
#define D_INTERNAL_MATH_DECIMAL_MAX_SHIFT                                      \
    (D_INTERNAL_RATIONAL_WORD_BITS - 4u)

// 1.2.3
// D_INTERNAL_MATH_DECIMAL_PLACE_MAX
//   constant: the farthest from 0 a decimal point's place is kept. Every
// type overflows, or underflows to 0, well before it.
#define D_INTERNAL_MATH_DECIMAL_PLACE_MAX  1000000L

D_STATIC_ASSERT( ( (((long)LDBL_MAX_10_EXP + 2L) <
                    D_INTERNAL_MATH_DECIMAL_PLACE_MAX) &&
                   (((long)LDBL_MIN_10_EXP - (long)LDBL_MANT_DIG - 2L) >
                    -D_INTERNAL_MATH_DECIMAL_PLACE_MAX) ),
                 "decimal.h: every type's range must lie inside the place's" );

// 1.2.4
// dj_internal_math_decimal
//   struct: a nonnegative number as decimal digits:
// 0.digit[0] digit[1] ... digit[count - 1] * 10^point. No leading or
// trailing zero digit is kept; 0 has no digits, and its point at 0.
struct dj_internal_math_decimal
{
    unsigned char digit[D_INTERNAL_MATH_DECIMAL_DIGITS];  // 0 to 9, the most
                                                          // significant first
    long          count;      // the digits in use
    long          point;      // the decimal point's place
    bool          truncated;  // a nonzero digit past the last was dropped
};

// 1.2.5
// dj_internal_math_decimal_bits
//   struct: a decimal number rounded to a type's precision P: mantissa *
// 2^(exponent - P), the mantissa below 2^P, and with its top bit set where
// the value is normal.
struct dj_internal_math_decimal_bits
{
    enum dj_math_decimal_status      status;    // OK, OVERFLOW or UNDERFLOW
    struct dj_internal_rational_wide mantissa;  // P bits at most, two words
    long                            exponent;  // as frexp's: 2^(e-1) <= x
                                               // < 2^e for a normal x
};


//==============================================================================
// 2.  THE DIGITS' ARITHMETIC
//==============================================================================


// 2.1    Shifts
//------------------------------------------------------------------------------

// d_internal_math_decimal_trim
//   drops trailing zero digits; 0 has no digits and its point at 0.
D_MATH_FN
void d_internal_math_decimal_trim(
    struct dj_internal_math_decimal* _d
)
{
    // the zeros at the end
    while ( (_d->count > 0) &&
            (_d->digit[_d->count - 1] == 0u) )
    {
        --_d->count;
    }

    // nothing left
    if (_d->count == 0)
    {
        _d->point = 0;
    }

    return;
}

// d_internal_math_decimal_shift_right
//   _d / 2^_k, _k from 1 to D_INTERNAL_MATH_DECIMAL_MAX_SHIFT: long division
// from the left, each quotient digit written where the digits already read
// were. A remainder n stays below 2^_k, so n * 10 + 9 fits a word.
D_MATH_FN
void d_internal_math_decimal_shift_right(
    struct dj_internal_math_decimal* _d,
    unsigned                         _k
)
{
    const dj_math_umax mask = ((dj_math_umax)1 << _k) - 1u;

    long        read  = 0;
    long        write = 0;
    dj_math_umax n     = 0u;

    // enough leading digits for the first quotient digit
    while ((n >> _k) == 0u)
    {
        // past the digits: zeros follow
        if (read >= _d->count)
        {
            // nothing at all
            if (n == 0u)
            {
                _d->count = 0;
                _d->point = 0;

                return;
            }

            // as many zeros as it takes
            while ((n >> _k) == 0u)
            {
                n = n * 10u;
                ++read;
            }

            break;
        }

        n = (n * 10u) + (dj_math_umax)_d->digit[read];
        ++read;
    }

    _d->point = _d->point - (read - 1);

    // a digit read, a digit written
    for (; read < _d->count; ++read)
    {
        const dj_math_umax next = (dj_math_umax)_d->digit[read];

        _d->digit[write] = (unsigned char)(n >> _k);
        ++write;
        n = ((n & mask) * 10u) + next;
    }

    // the remainder's digits, while there is room
    while (n > 0u)
    {
        const dj_math_umax quotient = n >> _k;

        n = n & mask;

        // room for it, or a nonzero digit dropped
        if (write < D_INTERNAL_MATH_DECIMAL_DIGITS)
        {
            _d->digit[write] = (unsigned char)quotient;
            ++write;
        }
        else if (quotient > 0u)
        {
            _d->truncated = true;
        }

        n = n * 10u;
    }

    _d->count = write;
    d_internal_math_decimal_trim(_d);

    return;
}

// d_internal_math_decimal_shift_left
//   _d * 2^_k, _k from 1 to D_INTERNAL_MATH_DECIMAL_MAX_SHIFT: the carry out
// of the top found first, which says how many digits the product gains;
// then each digit multiplied, from the right, into its place.
D_MATH_FN
void d_internal_math_decimal_shift_left(
    struct dj_internal_math_decimal* _d,
    unsigned                         _k
)
{
    dj_math_umax carry = 0u;
    dj_math_umax n     = 0u;
    long        gain  = 0;
    long        read  = 0;
    long        write = 0;

    // the carry out of the top digit
    for (read = _d->count - 1; read >= 0; --read)
    {
        carry = (carry + ((dj_math_umax)_d->digit[read] << _k)) / 10u;
    }

    // its digits, new at the top
    while (carry > 0u)
    {
        carry = carry / 10u;
        ++gain;
    }

    write = _d->count + gain;

    // each digit, from the right; past the buffer, dropped
    for (read = _d->count - 1; read >= 0; --read)
    {
        n = n + ((dj_math_umax)_d->digit[read] << _k);
        --write;

        // room for it, or a nonzero digit dropped
        if (write < D_INTERNAL_MATH_DECIMAL_DIGITS)
        {
            _d->digit[write] = (unsigned char)(n % 10u);
        }
        else if ((n % 10u) != 0u)
        {
            _d->truncated = true;
        }

        n = n / 10u;
    }

    // the carry's digits
    while (n > 0u)
    {
        --write;
        _d->digit[write] = (unsigned char)(n % 10u);
        n                = n / 10u;
    }

    _d->count = _d->count + gain;

    // past the buffer
    if (_d->count > D_INTERNAL_MATH_DECIMAL_DIGITS)
    {
        _d->count = D_INTERNAL_MATH_DECIMAL_DIGITS;
    }

    _d->point = _d->point + gain;
    d_internal_math_decimal_trim(_d);

    return;
}

// d_internal_math_decimal_shift
//   _d * 2^_k, _k of either sign, in shifts of at most
// D_INTERNAL_MATH_DECIMAL_MAX_SHIFT bits.
D_MATH_FN
void d_internal_math_decimal_shift(
    struct dj_internal_math_decimal* _d,
    long                             _k
)
{
    const long most = (long)D_INTERNAL_MATH_DECIMAL_MAX_SHIFT;

    long left = _k;

    // up
    while (left > 0)
    {
        const long step = (left > most) ? most : left;

        d_internal_math_decimal_shift_left(_d, (unsigned)step);
        left = left - step;
    }

    // down
    while (left < 0)
    {
        const long step = (-left > most) ? most : -left;

        d_internal_math_decimal_shift_right(_d, (unsigned)step);
        left = left + step;
    }

    return;
}

// d_internal_math_decimal_pow_shift
//   for a point at _i, 0 or more, the bits n to shift by to move a value
// toward 1 without passing it: with 2^n at most 10^_i, the largest such n
// that one shift may move, and 1 for a point at 0. 3321/1000 is a little
// under log2(10), so n never passes floor(_i log2(10)).
D_MATH_FN
unsigned d_internal_math_decimal_pow_shift(
    long _i
)
{
    const long most = (long)D_INTERNAL_MATH_DECIMAL_MAX_SHIFT;
    const long i    = (_i > most) ? most : _i;
    const long bits = (i * 3321L) / 1000L;

    // a point at 0: one bit
    if (i <= 0)
    {
        return 1u;
    }

    return (unsigned)((bits > most) ? most : bits);
}

// 2.2    The integer part
//------------------------------------------------------------------------------

// d_internal_math_decimal_round_up
//   whether _d cut after its first _cut digits rounds up: a digit above 5
// does, a 5 with anything after it does, and a lone 5 does where the digit
// before it is odd, ties to even. A dropped nonzero digit is something
// after. A cut before the first digit leaves less than 1/10: no.
D_MATH_FN
bool d_internal_math_decimal_round_up(
    const struct dj_internal_math_decimal* _d,
    long                                   _cut
)
{
    // nothing after the cut, or too little
    if ( (_cut < 0) ||
         (_cut >= _d->count) )
    {
        return false;
    }

    // exactly halfway, unless digits were dropped: to even
    if ( (_d->digit[_cut] == 5u) &&
         ((_cut + 1) == _d->count) )
    {
        return ( (_d->truncated) ||
                 ( (_cut > 0) &&
                   ((_d->digit[_cut - 1] % 2u) == 1u) ) );
    }

    return (_d->digit[_cut] >= 5u);
}

// d_internal_math_decimal_integer
//   _d's integer part, in two words; the caller keeps it below 2^(2w).
// *_exact says whether nothing follows it, *_up whether what follows rounds
// it up, to nearest, ties to even.
D_MATH_FN
struct dj_internal_rational_wide d_internal_math_decimal_integer(
    const struct dj_internal_math_decimal* _d,
    bool*                                  _exact,
    bool*                                  _up
)
{
    struct dj_internal_rational_wide result = { 0u, 0u };
    long                            i      = 0;

    // each digit of the integer part, then its zeros: times 10, plus it
    for (i = 0; i < _d->point; ++i)
    {
        const struct dj_internal_rational_wide low =
            d_internal_rational_mul_wide(result.lo, 10u);
        const dj_math_umax                     digit =
            (i < _d->count) ? (dj_math_umax)_d->digit[i] : 0u;
        const struct dj_internal_rational_wide add = { 0u, digit };

        result.hi = (result.hi * 10u) + low.hi;
        result.lo = low.lo;
        result    = d_internal_rational_add_wide(result, add);
    }

    *_exact = ( (_d->count <= _d->point) &&
                (!_d->truncated) );
    *_up    = d_internal_math_decimal_round_up(_d, _d->point);

    return result;
}


//==============================================================================
// 3.  READING
//==============================================================================

// d_internal_math_decimal_is_digit
//   whether _c is a decimal digit, 0 to 9.
D_MATH_FN
bool d_internal_math_decimal_is_digit(
    char _c
)
{
    return ( (_c >= '0') &&
             (_c <= '9') );
}

// d_internal_math_decimal_place
//   the decimal point's place, _a plus _b, each with its sign: _a counts
// the digits before the point, or the zeros after it, and _b is the
// exponent. _a is at most the text's length, so it cannot overflow; _b is
// held at the largest size_t. The place is held within
// D_INTERNAL_MATH_DECIMAL_PLACE_MAX of 0.
D_MATH_FN
long d_internal_math_decimal_place(
    size_t _a,
    bool   _a_below,
    size_t _b,
    bool   _b_below
)
{
    const size_t most = (size_t)D_INTERNAL_MATH_DECIMAL_PLACE_MAX;

    size_t size  = 0u;
    bool   below = false;

    // one sign: the sizes add
    if (_a_below == _b_below)
    {
        below = _a_below;
        size  = ( (_a >= most) ||
                  (_b >= (most - _a)) ) ? most : (_a + _b);
    }
    // opposite signs: the larger wins
    else if (_a >= _b)
    {
        below = _a_below;
        size  = _a - _b;
    }
    else
    {
        below = _b_below;
        size  = _b - _a;
    }

    size = (size > most) ? most : size;

    return (below) ? -(long)size : (long)size;
}

// d_internal_math_decimal_read
//   reads the decimal number at the start of _text: its digits and their
// place into *_d, which starts empty, and its sign into *_negative, each
// where not NULL. Returns the length of the longest decimal number there, 0
// if there is none. An e is part of the number only where a digit follows
// it, its sign between.
D_MATH_FN
size_t d_internal_math_decimal_read(
    struct dj_internal_math_decimal* _d,
    bool*                            _negative,
    const char*                      _text,
    size_t                           _length
)
{
    const size_t most = ~(size_t)0;

    size_t i        = 0u;
    size_t end      = 0u;
    size_t whole    = 0u;     // the digits kept before the point
    size_t zeros    = 0u;     // the zeros after it, before any other digit
    size_t size     = 0u;     // the exponent's size, held at most
    bool   any      = false;  // a digit read
    bool   started  = false;  // a nonzero digit read
    bool   after    = false;  // the point read
    bool   negative = false;  // a minus sign read
    bool   below    = false;  // a negative exponent read

    // the sign
    if ( (i < _length) &&
         ( (_text[i] == '+') ||
           (_text[i] == '-') ) )
    {
        negative = (_text[i] == '-');
        ++i;
    }

    // the digits, the point among them
    while (i < _length)
    {
        const char c = _text[i];

        // a digit: kept from the first nonzero one on
        if (d_internal_math_decimal_is_digit(c))
        {
            const unsigned char value = (unsigned char)(c - '0');

            any = true;

            // a significant digit
            if ( (started) ||
                 (value != 0u) )
            {
                started = true;
                whole   = (after) ? whole : (whole + 1u);

                // room for it, or a nonzero digit dropped
                if ( (_d != NULL) &&
                     (_d->count < D_INTERNAL_MATH_DECIMAL_DIGITS) )
                {
                    _d->digit[_d->count] = value;
                    ++_d->count;
                }
                else if ( (_d != NULL) &&
                          (value != 0u) )
                {
                    _d->truncated = true;
                }
            }
            // a zero after the point, before any other digit
            else if (after)
            {
                ++zeros;
            }

            ++i;
        }
        // the point, once
        else if ( (c == '.') &&
                  (!after) )
        {
            after = true;
            ++i;
        }
        else
        {
            break;
        }
    }

    // no digit: no number
    if (!any)
    {
        return 0u;
    }

    end = i;

    // the exponent, where a digit follows its e and sign
    if ( (i < _length) &&
         ( (_text[i] == 'e') ||
           (_text[i] == 'E') ) )
    {
        size_t j     = i + 1u;
        bool   minus = false;

        // its sign
        if ( (j < _length) &&
             ( (_text[j] == '+') ||
               (_text[j] == '-') ) )
        {
            minus = (_text[j] == '-');
            ++j;
        }

        // its digits, the size held at most past it
        while ( (j < _length) &&
                (d_internal_math_decimal_is_digit(_text[j])) )
        {
            const size_t digit = (size_t)(_text[j] - '0');

            size  = (size > ((most - digit) / 10u)) ? most
                                                    : ((size * 10u) + digit);
            below = minus;
            ++j;
            end   = j;
        }
    }

    // the point in place
    if (_d != NULL)
    {
        _d->point = d_internal_math_decimal_place(
                        (whole > 0u) ? whole : zeros,
                        (whole == 0u),
                        size,
                        below);
        d_internal_math_decimal_trim(_d);
    }

    // the sign
    if (_negative != NULL)
    {
        *_negative = negative;
    }

    return end;
}


//==============================================================================
// 4.  ROUNDING TO A TYPE
//==============================================================================

// d_internal_math_decimal_round
//   _d, positive or 0, rounded to a type of _precision bits, at most 2w, and
// exponents from _min_exp to _max_exp (as frexp's), its powers of ten from
// _min_10_exp to _max_10_exp. _d is worked on in place.
//   Far past either end, the answer is plain: at least ten times the
// largest finite value is an infinity, and less than a tenth of the least
// subnormal is 0. Otherwise _d is shifted into [1/2, 1), the shifts
// counted; below the normal range, down onto the subnormal grid; then P bits
// up, and its integer part, rounded, is the mantissa. A mantissa of P ones
// that rounds up is 2^(P - 1), a binade up.
D_MATH_FN
struct dj_internal_math_decimal_bits d_internal_math_decimal_round(
    struct dj_internal_math_decimal* _d,
    unsigned                         _precision,
    long                             _min_exp,
    long                             _max_exp,
    long                             _min_10_exp,
    long                             _max_10_exp
)
{
    const struct dj_internal_rational_wide one = { 0u, 1u };
    const long low = _min_10_exp - ( (((long)_precision * 30103L) / 100000L)
                                     + 2L );

    struct dj_internal_math_decimal_bits result   = { D_MATH_DECIMAL_OK,
                                                     { 0u, 0u }, 0 };
    long                                exponent = 0;
    bool                                exact    = true;
    bool                                up       = false;

    // 0
    if (_d->count == 0)
    {
        return result;
    }

    // at least 10^(_max_10_exp + 2): an infinity
    if (_d->point > (_max_10_exp + 2))
    {
        result.status = D_MATH_DECIMAL_OVERFLOW;

        return result;
    }

    // below 10^(low - 1), under half the least subnormal: 0
    if (_d->point < low)
    {
        result.status = D_MATH_DECIMAL_UNDERFLOW;

        return result;
    }

    // down below 1
    while (_d->point > 0)
    {
        const unsigned step = d_internal_math_decimal_pow_shift(_d->point);

        d_internal_math_decimal_shift_right(_d, step);
        exponent = exponent + (long)step;
    }

    // up to 1/2 or more
    while ( (_d->point < 0) ||
            ( (_d->point == 0) &&
              (_d->digit[0] < 5u) ) )
    {
        const unsigned step = d_internal_math_decimal_pow_shift(-_d->point);

        d_internal_math_decimal_shift_left(_d, step);
        exponent = exponent - (long)step;
    }

    // below the normal range: onto the subnormal grid
    if (exponent < _min_exp)
    {
        d_internal_math_decimal_shift(_d, exponent - _min_exp);
        exponent = _min_exp;
    }

    // past it
    if (exponent > _max_exp)
    {
        result.status = D_MATH_DECIMAL_OVERFLOW;

        return result;
    }

    // the mantissa: P bits up, the integer part
    d_internal_math_decimal_shift(_d, (long)_precision);
    result.mantissa = d_internal_math_decimal_integer(_d, &exact, &up);

    // rounded up: P ones make 2^P, which is 2^(P - 1) a binade up
    if ( (up) &&
         (d_internal_rational_cmp_wide(
              result.mantissa,
              d_internal_rational_ones(_precision)) == 0) )
    {
        result.mantissa = d_internal_rational_shift_wide(one,
                                                         _precision - 1u);
        exponent        = exponent + 1;

        // past the range
        if (exponent > _max_exp)
        {
            result.status = D_MATH_DECIMAL_OVERFLOW;

            return result;
        }
    }
    else if (up)
    {
        result.mantissa = d_internal_rational_add_wide(result.mantissa, one);
    }

    result.exponent = exponent;

    // a subnormal or 0, not exact
    if ( (!exact) &&
         (d_internal_rational_cmp_wide(
              result.mantissa,
              d_internal_rational_shift_wide(one, _precision - 1u)) < 0) )
    {
        result.status = D_MATH_DECIMAL_UNDERFLOW;
    }

    return result;
}


//==============================================================================
// 5.  THE CONVERSIONS
//==============================================================================


// 5.1    d_math_decimal_length
//------------------------------------------------------------------------------
// the length of the decimal number at the start of _text, the longest
// there; 0 where there is none.

D_MATH_FN
size_t d_math_decimal_length(
    const char* _text,
    size_t      _length
)
{
    return d_internal_math_decimal_read(NULL, NULL, _text, _length);
}

// 5.2    d_math_decimal_to_f, d_math_decimal_to_d, d_math_decimal_to_ld
//------------------------------------------------------------------------------
// _text, all _length characters of it a decimal number, as the nearest
// float, double or long double, ties to even, into *_out. An infinity is
// stored, and D_MATH_DECIMAL_OVERFLOW returned, past the largest finite
// value; a subnormal or 0 that is not exact is D_MATH_DECIMAL_UNDERFLOW; no
// decimal number, or anything after it, is D_MATH_DECIMAL_SYNTAX, and
// nothing is stored. A result of 0 has the text's sign. The value is built
// exactly: the mantissa's two words each convert without rounding, and two
// powers of two, each normal, put it in place, subnormal or not.

D_MATH_FN enum dj_math_decimal_status
d_math_decimal_to_f(
    float*      _out,
    const char* _text,
    size_t      _length
)
{
    struct dj_internal_math_decimal      decimal  = { { 0u }, 0, 0, false };
    struct dj_internal_math_decimal_bits bits     = { D_MATH_DECIMAL_OK,
                                                     { 0u, 0u }, 0 };
    size_t                              used     = 0u;
    bool                                negative = false;
    float                               value    = (float)0;

    used = d_internal_math_decimal_read(&decimal, &negative, _text, _length);

    // a decimal number, and nothing else
    if ( (used == 0u) ||
         (used != _length) )
    {
        return D_MATH_DECIMAL_SYNTAX;
    }

    bits = d_internal_math_decimal_round(&decimal, (unsigned)FLT_MANT_DIG,
                                         (long)FLT_MIN_EXP,
                                         (long)FLT_MAX_EXP,
                                         (long)FLT_MIN_10_EXP,
                                         (long)FLT_MAX_10_EXP);

    // an infinity; otherwise the mantissa, times 2^(1 - P), times 2^(e - 1)
    if (bits.status == D_MATH_DECIMAL_OVERFLOW)
    {
        value = (float)HUGE_VAL;
    }
    else
    {
        const float word  =
            d_internal_math_f_pow2((int)D_INTERNAL_RATIONAL_WORD_BITS);
        const float high  = (float)bits.mantissa.hi;
        const float low   = (float)bits.mantissa.lo;
        const float upper = high * word;
        const float whole = upper + low;
        const float unit  = whole * d_internal_math_f_pow2(1 - FLT_MANT_DIG);

        value = unit * d_internal_math_f_pow2((int)bits.exponent - 1);
    }

    *_out = (negative) ? -value : value;

    return bits.status;
}

D_MATH_FN enum dj_math_decimal_status
d_math_decimal_to_d(
    double*     _out,
    const char* _text,
    size_t      _length
)
{
    struct dj_internal_math_decimal      decimal  = { { 0u }, 0, 0, false };
    struct dj_internal_math_decimal_bits bits     = { D_MATH_DECIMAL_OK,
                                                     { 0u, 0u }, 0 };
    size_t                              used     = 0u;
    bool                                negative = false;
    double                              value    = (double)0;

    used = d_internal_math_decimal_read(&decimal, &negative, _text, _length);

    // a decimal number, and nothing else
    if ( (used == 0u) ||
         (used != _length) )
    {
        return D_MATH_DECIMAL_SYNTAX;
    }

    bits = d_internal_math_decimal_round(&decimal, (unsigned)DBL_MANT_DIG,
                                         (long)DBL_MIN_EXP,
                                         (long)DBL_MAX_EXP,
                                         (long)DBL_MIN_10_EXP,
                                         (long)DBL_MAX_10_EXP);

    // an infinity; otherwise the mantissa, times 2^(1 - P), times 2^(e - 1)
    if (bits.status == D_MATH_DECIMAL_OVERFLOW)
    {
        value = (double)HUGE_VAL;
    }
    else
    {
        const double word  =
            d_internal_math_d_pow2((int)D_INTERNAL_RATIONAL_WORD_BITS);
        const double high  = (double)bits.mantissa.hi;
        const double low   = (double)bits.mantissa.lo;
        const double upper = high * word;
        const double whole = upper + low;
        const double unit  = whole * d_internal_math_d_pow2(1 - DBL_MANT_DIG);

        value = unit * d_internal_math_d_pow2((int)bits.exponent - 1);
    }

    *_out = (negative) ? -value : value;

    return bits.status;
}

D_MATH_FN enum dj_math_decimal_status
d_math_decimal_to_ld(
    long double* _out,
    const char*  _text,
    size_t       _length
)
{
    struct dj_internal_math_decimal      decimal  = { { 0u }, 0, 0, false };
    struct dj_internal_math_decimal_bits bits     = { D_MATH_DECIMAL_OK,
                                                     { 0u, 0u }, 0 };
    size_t                              used     = 0u;
    bool                                negative = false;
    long double                         value    = (long double)0;

    used = d_internal_math_decimal_read(&decimal, &negative, _text, _length);

    // a decimal number, and nothing else
    if ( (used == 0u) ||
         (used != _length) )
    {
        return D_MATH_DECIMAL_SYNTAX;
    }

    bits = d_internal_math_decimal_round(&decimal, (unsigned)LDBL_MANT_DIG,
                                         (long)LDBL_MIN_EXP,
                                         (long)LDBL_MAX_EXP,
                                         (long)LDBL_MIN_10_EXP,
                                         (long)LDBL_MAX_10_EXP);

    // an infinity; otherwise the mantissa, times 2^(1 - P), times 2^(e - 1)
    if (bits.status == D_MATH_DECIMAL_OVERFLOW)
    {
        value = (long double)HUGE_VAL;
    }
    else
    {
        const long double word  =
            d_internal_math_ld_pow2((int)D_INTERNAL_RATIONAL_WORD_BITS);
        const long double high  = (long double)bits.mantissa.hi;
        const long double low   = (long double)bits.mantissa.lo;
        const long double upper = high * word;
        const long double whole = upper + low;
        const long double unit  =
            whole * d_internal_math_ld_pow2(1 - LDBL_MANT_DIG);

        value = unit * d_internal_math_ld_pow2((int)bits.exponent - 1);
    }

    *_out = (negative) ? -value : value;

    return bits.status;
}


#endif  // DJINTERP_C_MATH_DECIMAL_H
