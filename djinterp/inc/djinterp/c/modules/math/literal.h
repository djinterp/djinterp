/*******************************************************************************
* djinterp [c/math]                                                    literal.h
*
* A number's text, read as a value, exactly wherever the value allows: the
* reading of literals for the generic value (roadmap item 1c).
*   THE TEXT is one literal and nothing else, in one of three forms, each
* with an optional sign in front:
*   - a decimal number, as decimal.h reads it: an integer (42), a decimal
*     (0.1, .5, 5.) or scientific notation (6.02e23);
*   - a fraction: digits, a slash and digits, with no point or exponent
*     (2/3);
*   - a mixed number: digits, one or more spaces, then a fraction (1 1/2).
*     The sign is the whole number's: -1 1/2 is -3/2.
* d_math_literal_length measures the literal at the start of a longer text,
* the longest there, for a tokenizer.
*   THE VALUE is exact wherever the literal's value is a rational over
* dj_math_imax: 42 is the integer 42, 0.1 the rational 1/10, 6/4 and 1 1/2
* the rational 3/2, and 1500e-2 the integer 15. Otherwise it is the long
* double nearest the literal, ties to even, flagged inexact, as an exact
* result too wide for dj_math_imax is (roadmap gate G2). A decimal literal is
* read straight into a float, double or long double, rounded once, by
* decimal.h (G3).
*   A fraction over 0 is a pole, or invalid for 0/0, as an exact division
* is. A decimal literal past the largest long double is a range error; so is
* a fraction whose numerator or denominator does not fit two words, a mixed
* number w n/d being the fraction (w d + n)/d, until bignum exists to read
* them. The zeros both terms end with do not count.
*   COST. An exact literal of a usual length takes a fraction of a
* microsecond; the longest exact decimals, whose factors of 2 and 5 are
* taken out digit by digit, a few microseconds. A decimal that is not exact
* is read twice, once to see and once by decimal.h for the long double, and
* costs about what decimal.h does: milliseconds at the ends of a long
* double's range, which are past GCC's and clang's default limits in
* constant evaluation. The stack holds one of decimal.h's buffers of digits
* at a time: about 12 KB in an optimized build where long double is the
* x87's.
*
*
* path:      /inc/djinterp/c/modules/math/literal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LAYOUT
    ------
    1.  Forms
         1.  dj_internal_math_literal_form
         2.  dj_internal_math_literal_parts
    2.  Exact decimals
         1.  D_INTERNAL_MATH_LITERAL_DIGITS
2.  TWO WORDS
    ---------
3.  SCANNING
    --------
4.  READING
    -------
    1.  Decimal numbers
    2.  Fractions and mixed numbers
5.  THE CONVERSIONS
    ---------------
    1.  d_math_literal_length
    2.  d_math_literal_to_value
*/

#ifndef DJINTERP_C_MATH_LITERAL_H
#define DJINTERP_C_MATH_LITERAL_H 1

// std
#include <stddef.h>          // size_t
// djinterp
#include "./math_common.h"   // D_MATH_FN, dj_math_imax, dj_math_umax
#include "./rational.h"      // two-word arithmetic, ratios
#include "./value.h"         // dj_math_value
#include "./decimal.h"       // decimal numbers: syntax, digits, rounding


//==============================================================================
// 1.  LAYOUT
//==============================================================================


// 1.1    Forms
//------------------------------------------------------------------------------
// 1.1.1
// dj_internal_math_literal_form
//   enum: what a literal is.
enum dj_internal_math_literal_form
{
    D_INTERNAL_MATH_LITERAL_NONE     = 0,  // no literal
    D_INTERNAL_MATH_LITERAL_DECIMAL  = 1,  // an integer, a decimal, or
                                           // scientific notation
    D_INTERNAL_MATH_LITERAL_FRACTION = 2,  // digits, a slash, digits
    D_INTERNAL_MATH_LITERAL_MIXED    = 3   // digits, spaces, a fraction
};

// 1.1.2
// dj_internal_math_literal_parts
//   struct: the literal at the start of a text: its form, its length and
// sign, and where a fraction's or mixed number's terms lie, each from its
// first digit to just past its last.
struct dj_internal_math_literal_parts
{
    enum dj_internal_math_literal_form form;       // what it is
    size_t                            length;     // its length; 0 for none
    bool                              negative;   // a minus sign
    size_t                            whole;      // a mixed number's whole
    size_t                            whole_end;  // part
    size_t                            num;        // the numerator
    size_t                            num_end;
    size_t                            den;        // the denominator
    size_t                            den_end;
};

// 1.2    Exact decimals
//------------------------------------------------------------------------------
// 1.2.1
// D_INTERNAL_MATH_LITERAL_DIGITS
//   constant: room for the significant digits of any decimal that is a
// rational in the family, and a carry. Such a decimal is p / (2^x 5^y), p at
// most 2^(w-1) and the denominator below 2^(w-1); written out, it is p
// 2^(k-x) 5^(k-y) / 10^k, k the larger of x and y, whose digits number at
// most (w - 1) log10(2) + (w - 2) log10(5) + 1: 63 with 64-bit words, 31
// with 32-bit ones. Two more: 65 and 33.
#define D_INTERNAL_MATH_LITERAL_DIGITS                                         \
    ( ( ( (((long)D_INTERNAL_RATIONAL_WORD_BITS - 1L) * 30103L) +             \
          (((long)D_INTERNAL_RATIONAL_WORD_BITS - 2L) * 69897L) ) /           \
        100000L ) + 3L )


//==============================================================================
// 2.  TWO WORDS
//==============================================================================
// What a fraction's terms need beyond rational.h's two-word helpers: reading
// digits, shifting down, a greatest common divisor, exact division, and a
// product that says when it overflows.

// d_internal_math_literal_of_digits
//   _text[_start .. _end), digits, as a number of two words; *_overflow set,
// and the reading stopped, where it does not fit.
D_MATH_FN
struct dj_internal_rational_wide d_internal_math_literal_of_digits(
    const char* _text,
    size_t      _start,
    size_t      _end,
    bool*       _overflow
)
{
    struct dj_internal_rational_wide result = { 0u, 0u };
    size_t                          i      = 0u;

    // each digit: times 10, plus it
    for (i = _start; i < _end; ++i)
    {
        const dj_math_umax                     digit =
            (dj_math_umax)(_text[i] - '0');
        const struct dj_internal_rational_wide low   =
            d_internal_rational_mul_wide(result.lo, 10u);
        const dj_math_umax                     sum   = low.lo + digit;
        const dj_math_umax                     carry = (sum < low.lo) ? 1u
                                                                     : 0u;

        // the high word times 10, with the carries, past a word
        if ( (result.hi > ((D_MATH_UMAX_MAX - low.hi) / 10u)) ||
             ( ((result.hi * 10u) + low.hi) > (D_MATH_UMAX_MAX - carry) ) )
        {
            *_overflow = true;

            return result;
        }

        result.hi = (result.hi * 10u) + low.hi + carry;
        result.lo = sum;
    }

    return result;
}

// d_internal_math_literal_shift_down
//   _x / 2^_k, rounded down, for _k below 2w.
D_MATH_FN
struct dj_internal_rational_wide d_internal_math_literal_shift_down(
    struct dj_internal_rational_wide _x,
    unsigned                         _k
)
{
    const unsigned width = D_INTERNAL_RATIONAL_WORD_BITS;

    struct dj_internal_rational_wide result = { 0u, 0u };

    // a word or more: the high word moves down
    if (_k >= width)
    {
        result.lo = _x.hi >> (_k - width);

        return result;
    }

    // no shift: a shift by w would not be defined
    if (_k == 0u)
    {
        return _x;
    }

    result.lo = (_x.lo >> _k) | (_x.hi << (width - _k));
    result.hi = _x.hi >> _k;

    return result;
}

// d_internal_math_literal_low_zeros
//   the zero bits below _x's lowest 1, _x not 0.
D_MATH_FN
unsigned d_internal_math_literal_low_zeros(
    struct dj_internal_rational_wide _x
)
{
    dj_math_umax word  = (_x.lo != 0u) ? _x.lo : _x.hi;
    unsigned    count = (_x.lo != 0u) ? 0u : D_INTERNAL_RATIONAL_WORD_BITS;

    // one bit at a time
    while ((word & 1u) == 0u)
    {
        word = word >> 1;
        ++count;
    }

    return count;
}

// d_internal_math_literal_gcd
//   the greatest common divisor of _a and _b, by Stein's binary algorithm:
// the powers of two they share set aside, then the larger, made odd, less
// the smaller, until they meet. gcd(0, _b) is _b.
D_MATH_FN
struct dj_internal_rational_wide d_internal_math_literal_gcd(
    struct dj_internal_rational_wide _a,
    struct dj_internal_rational_wide _b
)
{
    const struct dj_internal_rational_wide zero = { 0u, 0u };

    struct dj_internal_rational_wide a      = _a;
    struct dj_internal_rational_wide b      = _b;
    struct dj_internal_rational_wide either = { 0u, 0u };
    unsigned                        shared = 0u;

    // a 0 on either side
    if (d_internal_rational_cmp_wide(a, zero) == 0)
    {
        return b;
    }

    if (d_internal_rational_cmp_wide(b, zero) == 0)
    {
        return a;
    }

    either.hi = a.hi | b.hi;
    either.lo = a.lo | b.lo;
    shared    = d_internal_math_literal_low_zeros(either);
    a         = d_internal_math_literal_shift_down(
                    a, d_internal_math_literal_low_zeros(a));

    // b made odd, then the smaller taken from the larger, until 0
    do
    {
        b = d_internal_math_literal_shift_down(
                b, d_internal_math_literal_low_zeros(b));

        // the smaller in a
        if (d_internal_rational_cmp_wide(a, b) > 0)
        {
            const struct dj_internal_rational_wide swap = a;

            a = b;
            b = swap;
        }

        b = d_internal_rational_sub_wide(b, a);
    } while (d_internal_rational_cmp_wide(b, zero) != 0);

    return d_internal_rational_shift_wide(a, shared);
}

// d_internal_math_literal_divide
//   _n / _d, which divides exactly, _d not 0. A divisor of one word divides
// natively; a larger one leaves a quotient of one word, found a bit at a
// time, the highest first.
D_MATH_FN
struct dj_internal_rational_wide d_internal_math_literal_divide(
    struct dj_internal_rational_wide _n,
    struct dj_internal_rational_wide _d
)
{
    const unsigned width  = D_INTERNAL_RATIONAL_WORD_BITS;
    const unsigned d_bits = d_internal_rational_bits(_d);

    struct dj_internal_rational_wide result = { 0u, 0u };
    struct dj_internal_rational_wide rest   = _n;
    unsigned                        bit    = 0u;

    // one word
    if (_d.hi == 0u)
    {
        return d_internal_rational_divmod_wide(_n, _d.lo).quotient;
    }

    // each bit of the quotient, where the divisor shifted to it fits
    for (bit = width; bit > 0u; --bit)
    {
        const unsigned place = bit - 1u;

        // the shifted divisor within two words, and not past the rest
        if ( ((d_bits + place) <= (2u * width)) &&
             (d_internal_rational_cmp_wide(
                  rest,
                  d_internal_rational_shift_wide(_d, place)) >= 0) )
        {
            rest      = d_internal_rational_sub_wide(
                            rest, d_internal_rational_shift_wide(_d, place));
            result.lo = result.lo | ((dj_math_umax)1 << place);
        }
    }

    return result;
}

// d_internal_math_literal_mul
//   _a * _b, with *_overflow set where it does not fit two words.
D_MATH_FN
struct dj_internal_rational_wide d_internal_math_literal_mul(
    struct dj_internal_rational_wide _a,
    struct dj_internal_rational_wide _b,
    bool*                            _overflow
)
{
    const struct dj_internal_rational_wide low =
        d_internal_rational_mul_wide(_a.lo, _b.lo);
    const struct dj_internal_rational_wide cross =
        (_a.hi != 0u) ? d_internal_rational_mul_wide(_a.hi, _b.lo)
                      : d_internal_rational_mul_wide(_a.lo, _b.hi);

    struct dj_internal_rational_wide result = { 0u, 0u };

    // two high words, or a cross product past one word: past two words
    if ( ( (_a.hi != 0u) &&
           (_b.hi != 0u) ) ||
         (cross.hi != 0u) ||
         (low.hi > (D_MATH_UMAX_MAX - cross.lo)) )
    {
        *_overflow = true;

        return result;
    }

    result.hi = low.hi + cross.lo;
    result.lo = low.lo;

    return result;
}


//==============================================================================
// 3.  SCANNING
//==============================================================================

// d_internal_math_literal_run
//   where the run of digits from _i in _text ends: _i itself if there is
// none.
D_MATH_FN
size_t d_internal_math_literal_run(
    const char* _text,
    size_t      _length,
    size_t      _i
)
{
    size_t end = _i;

    // the digits
    while ( (end < _length) &&
            (d_internal_math_decimal_is_digit(_text[end])) )
    {
        ++end;
    }

    return end;
}

// d_internal_math_literal_scan
//   the longest literal at the start of _text. After the sign, digits
// followed by a slash and digits are a fraction; digits followed by spaces,
// digits, a slash and digits, a mixed number; anything else is decimal.h's
// to read. No decimal number is longer than either where one matches.
D_MATH_FN
struct dj_internal_math_literal_parts d_internal_math_literal_scan(
    const char* _text,
    size_t      _length
)
{
    struct dj_internal_math_literal_parts result =
        { D_INTERNAL_MATH_LITERAL_NONE, 0u, false, 0u, 0u, 0u, 0u, 0u, 0u };

    size_t i     = 0u;
    size_t first = 0u;   // past the first run of digits
    size_t gap   = 0u;   // past the spaces after it
    size_t over  = 0u;   // past the run after a slash
    size_t mid   = 0u;   // past the run after the spaces

    // the sign
    if ( (i < _length) &&
         ( (_text[i] == '+') ||
           (_text[i] == '-') ) )
    {
        result.negative = (_text[i] == '-');
        ++i;
    }

    first = d_internal_math_literal_run(_text, _length, i);

    // a fraction: digits, a slash, digits
    if ( (first > i) &&
         (first < _length) &&
         (_text[first] == '/') )
    {
        over = d_internal_math_literal_run(_text, _length, first + 1u);

        // digits after the slash
        if (over > (first + 1u))
        {
            result.form    = D_INTERNAL_MATH_LITERAL_FRACTION;
            result.length  = over;
            result.num     = i;
            result.num_end = first;
            result.den     = first + 1u;
            result.den_end = over;

            return result;
        }
    }

    gap = first;

    // a mixed number: digits, spaces, then a fraction
    while ( (first > i) &&
            (gap < _length) &&
            (_text[gap] == ' ') )
    {
        ++gap;
    }

    mid = d_internal_math_literal_run(_text, _length, gap);

    // the fraction after the spaces
    if ( (gap > first) &&
         (mid > gap) &&
         (mid < _length) &&
         (_text[mid] == '/') )
    {
        over = d_internal_math_literal_run(_text, _length, mid + 1u);

        // digits after the slash
        if (over > (mid + 1u))
        {
            result.form      = D_INTERNAL_MATH_LITERAL_MIXED;
            result.length    = over;
            result.whole     = i;
            result.whole_end = first;
            result.num       = gap;
            result.num_end   = mid;
            result.den       = mid + 1u;
            result.den_end   = over;

            return result;
        }
    }

    result.length = d_math_decimal_length(_text, _length);
    result.form   = (result.length > 0u) ? D_INTERNAL_MATH_LITERAL_DECIMAL
                                         : D_INTERNAL_MATH_LITERAL_NONE;

    return result;
}


//==============================================================================
// 4.  READING
//==============================================================================


// 4.1    Decimal numbers
//------------------------------------------------------------------------------
// A decimal is the integer its digits make, d, times 10^e. For e at least 0
// it is exact where d 10^e fits a word. Below, it is d / 10^k, k = -e, and
// what d shares with 10^k is a power of 2 and a power of 5: taken out of d,
// one factor at a time, by working on its digits (d / 2 is d * 5 / 10, and
// d / 5 is d * 2 / 10, the last digit then a 0 to drop), the rest is
// p / (2^(k-a) 5^(k-b)), in lowest terms, exact where p and the denominator
// fit. Otherwise decimal.h gives the long double nearest it.

// d_internal_math_literal_times_drop
//   the digits _digit[0 .. *_count) times _m, 2 or 5, the result's last
// digit, a 0, dropped: the number divided by 5 or by 2. A carry out of the
// top becomes a new first digit.
D_MATH_FN
void d_internal_math_literal_times_drop(
    unsigned char* _digit,
    long*          _count,
    unsigned       _m
)
{
    unsigned carry = 0u;
    long     i     = 0;

    // each digit, from the last
    for (i = *_count - 1; i >= 0; --i)
    {
        const unsigned product = ((unsigned)_digit[i] * _m) + carry;

        _digit[i] = (unsigned char)(product % 10u);
        carry     = product / 10u;
    }

    // a carry out of the top: a new first digit
    if (carry > 0u)
    {
        for (i = *_count; i > 0; --i)
        {
            _digit[i] = _digit[i - 1];
        }

        _digit[0] = (unsigned char)carry;
        ++*_count;
    }

    --*_count;

    return;
}

// d_internal_math_literal_word
//   *_x * 10 + _digit, false where it does not fit a word.
D_MATH_FN
bool d_internal_math_literal_word(
    dj_math_umax* _x,
    dj_math_umax  _digit
)
{
    // past a word
    if (*_x > ((D_MATH_UMAX_MAX - _digit) / 10u))
    {
        return false;
    }

    *_x = (*_x * 10u) + _digit;

    return true;
}

// d_internal_math_literal_exact
//   the decimal number _text as an exact value, *_exact set, where it is a
// rational in the family; *_exact false, and none, otherwise.
D_MATH_FN
struct dj_math_value d_internal_math_literal_exact(
    const char* _text,
    size_t      _length,
    bool*       _exact
)
{
    const dj_math_umax most = (dj_math_umax)D_MATH_IMAX_MAX;

    struct dj_internal_math_decimal decimal = { { 0u }, 0, 0, false };
    unsigned char                  digit[D_INTERNAL_MATH_LITERAL_DIGITS] =
        { 0u };
    bool                           negative = false;
    bool                           fits     = true;
    long                           count    = 0;
    long                           e        = 0;
    long                           k        = 0;
    long                           twos     = 0;
    long                           fives    = 0;
    long                           i        = 0;
    dj_math_umax                    p        = 0u;
    dj_math_umax                    q        = 1u;

    *_exact = false;
    (void)d_internal_math_decimal_read(&decimal, &negative, _text, _length);

    // 0
    if (decimal.count == 0)
    {
        *_exact = true;

        return d_math_value_from_int(0);
    }

    // a digit dropped, or more digits than any rational in the family has
    if ( (decimal.truncated) ||
         (decimal.count > (D_INTERNAL_MATH_LITERAL_DIGITS - 2L)) )
    {
        return d_math_value_none();
    }

    count = decimal.count;
    e     = decimal.point - count;

    // the digits, to work on
    for (i = 0; i < count; ++i)
    {
        digit[i] = decimal.digit[i];
    }

    // below 1 in its last place: the factors of 2, then of 5, it shares
    // with 10^k
    if (e < 0)
    {
        k = -e;

        while ( (twos < k) &&
                ((digit[count - 1] % 2u) == 0u) )
        {
            d_internal_math_literal_times_drop(digit, &count, 5u);
            ++twos;
        }

        while ( (fives < k) &&
                (digit[count - 1] == 5u) )
        {
            d_internal_math_literal_times_drop(digit, &count, 2u);
            ++fives;
        }
    }

    // the numerator: its digits, then any zeros
    for (i = 0; (fits) && (i < count); ++i)
    {
        fits = d_internal_math_literal_word(&p, (dj_math_umax)digit[i]);
    }

    for (i = 0; (fits) && (i < e); ++i)
    {
        fits = d_internal_math_literal_word(&p, 0u);
    }

    // the denominator: 2^(k - twos) 5^(k - fives), within the family
    for (i = twos; (fits) && (i < k); ++i)
    {
        fits = (q <= (most / 2u));
        q    = (fits) ? (q * 2u) : q;
    }

    for (i = fives; (fits) && (i < k); ++i)
    {
        fits = (q <= (most / 5u));
        q    = (fits) ? (q * 5u) : q;
    }

    // past the family
    if ( (!fits) ||
         (p > ((negative) ? (most + 1u) : most)) )
    {
        return d_math_value_none();
    }

    *_exact = true;

    return d_internal_math_value_from_ratio(
        d_internal_rational_ratio_of(negative, p, q), 0u);
}

// d_internal_math_literal_decimal
//   the decimal number _text as a value: exact where it can be; otherwise
// the long double nearest it, flagged inexact, or past every long double, a
// range error.
D_MATH_FN
struct dj_math_value d_internal_math_literal_decimal(
    const char* _text,
    size_t      _length
)
{
    struct dj_math_value result  = d_math_value_none();
    long double         nearest = 0.0L;
    bool                exact   = false;

    result = d_internal_math_literal_exact(_text, _length, &exact);

    // exact
    if (exact)
    {
        return result;
    }

    // past every long double
    if (d_math_decimal_to_ld(&nearest, _text, _length) ==
            D_MATH_DECIMAL_OVERFLOW)
    {
        return d_math_value_from_error(D_MATH_VALUE_ERROR_RANGE);
    }

    result       = d_math_value_from_ld(nearest);
    result.flags = D_MATH_VALUE_FLAG_INEXACT;

    return result;
}

// 4.2    Fractions and mixed numbers
//------------------------------------------------------------------------------
// A mixed number w n/d is the fraction (w d + n)/d. The terms are read in
// two words, less the zeros that n and d both end with, a common factor of a
// power of ten. Over 0 is a pole, or invalid for 0/0, whatever the other
// terms. Otherwise a numerator or denominator past two words is a range
// error; and the ratio, reduced, is exact where it fits the family, and
// otherwise the long double nearest it, flagged inexact.

// d_internal_math_literal_zeros
//   the zeros that _text[_start .. _end) ends with.
D_MATH_FN
size_t d_internal_math_literal_zeros(
    const char* _text,
    size_t      _start,
    size_t      _end
)
{
    size_t count = 0u;

    // from the end
    while ( ((_end - count) > _start) &&
            (_text[_end - count - 1u] == '0') )
    {
        ++count;
    }

    return count;
}

// d_internal_math_literal_fraction
//   the fraction or mixed number that _scan found in _text, as a value.
D_MATH_FN
struct dj_math_value d_internal_math_literal_fraction(
    const char*                           _text,
    struct dj_internal_math_literal_parts _scan
)
{
    const struct dj_internal_rational_wide zero = { 0u, 0u };
    const size_t num_zeros =
        d_internal_math_literal_zeros(_text, _scan.num, _scan.num_end);
    const size_t den_zeros =
        d_internal_math_literal_zeros(_text, _scan.den, _scan.den_end);
    const size_t common = (num_zeros < den_zeros) ? num_zeros : den_zeros;

    struct dj_internal_rational_ratio ratio  = { false, { 0u, 0u },
                                                { 0u, 1u } };
    struct dj_internal_rational_wide  n      = { 0u, 0u };
    struct dj_internal_rational_wide  d      = { 0u, 0u };
    struct dj_internal_rational_wide  w      = { 0u, 0u };
    struct dj_internal_rational_wide  g      = { 0u, 0u };
    bool                             n_past = false;
    bool                             d_past = false;
    bool                             w_past = false;

    n = d_internal_math_literal_of_digits(_text, _scan.num,
                                          _scan.num_end - common, &n_past);
    d = d_internal_math_literal_of_digits(_text, _scan.den,
                                          _scan.den_end - common, &d_past);

    // over 0, whatever the other terms
    if ( (!d_past) &&
         (d_internal_rational_cmp_wide(d, zero) == 0) )
    {
        return d_math_value_from_error(
            ( (n_past) ||
              (d_internal_rational_cmp_wide(n, zero) != 0) )
                ? D_MATH_VALUE_ERROR_POLE
                : D_MATH_VALUE_ERROR_INVALID);
    }

    // a whole part: w d + n
    if (_scan.form == D_INTERNAL_MATH_LITERAL_MIXED)
    {
        w = d_internal_math_literal_of_digits(_text, _scan.whole,
                                              _scan.whole_end, &w_past);
        w = d_internal_math_literal_mul(w, d, &w_past);

        // the sum past two words
        if (d_internal_rational_cmp_wide(
                d_internal_rational_add_wide(w, n), w) < 0)
        {
            w_past = true;
        }

        n = d_internal_rational_add_wide(w, n);
    }

    // a term past two words
    if ( (n_past) ||
         (d_past) ||
         (w_past) )
    {
        return d_math_value_from_error(D_MATH_VALUE_ERROR_RANGE);
    }

    g = d_internal_math_literal_gcd(n, d);

    ratio.negative = ( (_scan.negative) &&
                       (d_internal_rational_cmp_wide(n, zero) != 0) );
    ratio.num      = d_internal_math_literal_divide(n, g);
    ratio.den      = d_internal_math_literal_divide(d, g);

    return d_internal_math_value_from_ratio(ratio, 0u);
}


//==============================================================================
// 5.  THE CONVERSIONS
//==============================================================================


// 5.1    d_math_literal_length
//------------------------------------------------------------------------------
// the length of the literal at the start of _text, the longest there; 0
// where there is none.

D_MATH_FN
size_t d_math_literal_length(
    const char* _text,
    size_t      _length
)
{
    return d_internal_math_literal_scan(_text, _length).length;
}

// 5.2    d_math_literal_to_value
//------------------------------------------------------------------------------
// _text, all _length characters of it one literal, as a value into *_out:
// exact where the literal's value is a rational in the family; otherwise the
// long double nearest it, flagged inexact; a pole, invalid or a range error
// where it has no value yet. Returns false, storing nothing, where the text
// is not one literal.

D_MATH_FN
bool d_math_literal_to_value(
    struct dj_math_value* _out,
    const char*           _text,
    size_t                _length
)
{
    const struct dj_internal_math_literal_parts scan =
        d_internal_math_literal_scan(_text, _length);

    // one literal, and nothing else
    if ( (scan.length == 0u) ||
         (scan.length != _length) )
    {
        return false;
    }

    // by form
    if (scan.form == D_INTERNAL_MATH_LITERAL_DECIMAL)
    {
        *_out = d_internal_math_literal_decimal(_text, _length);
    }
    else
    {
        *_out = d_internal_math_literal_fraction(_text, scan);
    }

    return true;
}


#endif  // DJINTERP_C_MATH_LITERAL_H
