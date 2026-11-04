/*******************************************************************************
* djinterp [c/math]                                                  t_literal.c
*
*   Conformance harness for number literals read as values
* (c/math/literal.h), against the exact reference of math_tests_exact.h.
*   The harness reads each literal itself, apart from literal.h, as a ratio
* of integers of up to 256 bits, and asks of the value:
*   - where the ratio, reduced, lies within d_math_imax: that it is exact,
*     an integer where its denominator is 1 and a rational otherwise, in the
*     one form, unflagged and equal to the ratio by cross products; and that
*     converted to float, double and long double, it is what decimal.h reads
*     from the same text, rounded once either way (G3);
*   - otherwise: that it is the long double nearest the ratio, ties to even,
*     flagged inexact (G2), and for a decimal literal, what decimal.h reads;
*   - for a fraction over 0, a pole, or invalid for 0/0; for a fraction
*     whose numerator or denominator, less the zeros both end with, passes
*     two words, a mixed number w n/d being (w d + n)/d, a range error;
*   - for a decimal literal past the reference, more significant digits than
*     any exact one has or an exponent past any exact one's: the long double
*     decimal.h reads, flagged inexact, or a range error where decimal.h
*     overflows.
*   Each literal's length is checked too, alone and with a character after
* it, and a text that is not one literal must store nothing.
*   The texts: the syntax's corner cases; known values; the integers around
* the family's ends; exact decimals written out from random rationals whose
* denominators are 2^x 5^y, in several forms, the longest an exact decimal
* can be among them, and their neighbours just past the family; random
* decimals of up to 25 digits; long decimals and huge exponents; random
* fractions and mixed numbers, of every size up to past two words, with
* common factors and common zeros; and texts of a million characters. From
* C++14 the reading is also checked as a constant expression.
*   The same file compiles as C++ (-x c++), at any level from C++98:
* literal.h is one text in both languages.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/math/t_literal.c -o t_literal -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_literal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.15
*******************************************************************************/
// std
#include <float.h>   // LDBL_MAX
#include <math.h>    // HUGE_VAL
#include <stdio.h>   // printf, sprintf
#include <string.h>  // strlen, memcpy, memset
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/literal.h"  // unit under test
#include "./math_tests_exact.h"  // the exact reference


#define D_TESTS_WORD        ((int)D_INTERNAL_RATIONAL_WORD_BITS)
#define D_TESTS_TEXT_SIZE   4096
#define D_TESTS_LONG_ZEROS  1000005L
#define D_TESTS_EXACT_MOST  (D_INTERNAL_MATH_LITERAL_DIGITS - 2L)

// what the reference makes of a literal
#define D_TESTS_REF_VALUE    0  // a ratio, n / d
#define D_TESTS_REF_POLE     1  // over 0
#define D_TESTS_REF_INVALID  2  // 0 over 0
#define D_TESTS_REF_RANGE    3  // terms past two words
#define D_TESTS_REF_BEYOND   4  // past the reference, and not exact

static long g_checks   = 0;
static long g_failures = 0;


//==============================================================================
// 1.  REPORTING
//==============================================================================

// d_tests_expect
//   counts a check, and reports the first few failures with their text, cut
// to 72 characters.
static void
d_tests_expect(
    bool        _ok,
    const char* _what,
    const char* _text,
    size_t      _length,
    int         _line
)
{
    ++g_checks;

    // the first failures, with their text
    if (!_ok)
    {
        ++g_failures;

        if (g_failures <= 40)
        {
            printf("  FAIL line %d: %s (\"%.*s\"%s, %lu characters)\n",
                   _line, _what,
                   (int)((_length > 72u) ? 72u : _length), _text,
                   (_length > 72u) ? "..." : "", (unsigned long)_length);
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what, text, length)                                 \
    d_tests_expect((ok), (what), (text), (length), __LINE__)


//==============================================================================
// 2.  THE REFERENCE'S READING
//==============================================================================
// The harness reads a literal on its own: a decimal number as n * 10^e, its
// trailing zeros moved into e, then reduced by the 2s and 5s it shares with
// 10^-e; a fraction as its terms, less the zeros both end with; a mixed
// number as w d + n over d. Only texts that are one literal are read.

// d_tests_ref
//   struct: what a literal is: a ratio n / d, unreduced, with a sign; or an
// error; or past the reference.
struct d_tests_ref
{
    int                kind;      // D_TESTS_REF_*
    bool               negative;  // a minus sign, and the value not 0
    bool               decimal;   // a decimal number, not a fraction
    struct d_tests_big n;         // the numerator's magnitude
    struct d_tests_big d;         // the denominator, positive
};

// d_tests_big_div_small
//   |_x| / _divisor, rounded down, _divisor from 1 to 2^16; the remainder
// into *_rest.
static struct d_tests_big
d_tests_big_div_small(
    struct d_tests_big _x,
    unsigned long      _divisor,
    unsigned long*     _rest
)
{
    struct d_tests_big result = { { 0UL }, 0 };
    unsigned long      rest   = 0UL;
    int                i      = 0;

    // long division, from the top limb
    for (i = D_TESTS_LIMBS - 1; i >= 0; --i)
    {
        const unsigned long part = (rest << 16) | _x.limb[i];

        result.limb[i] = part / _divisor;
        rest           = part % _divisor;
    }

    *_rest = rest;

    return result;
}

// d_tests_digits_of
//   |_x|'s decimal digits into _text, the most significant first, and a
// NUL; returns how many.
static int
d_tests_digits_of(
    struct d_tests_big _x,
    char*              _text
)
{
    char          reversed[D_TESTS_TEXT_SIZE];
    unsigned long rest  = 0UL;
    int           count = 0;
    int           i     = 0;

    // the digits, the least significant first
    do
    {
        _x                = d_tests_big_div_small(_x, 10UL, &rest);
        reversed[count++] = (char)('0' + (int)rest);
    } while (!d_tests_big_is_zero(_x));

    for (i = 0; i < count; ++i)
    {
        _text[i] = reversed[count - 1 - i];
    }

    _text[count] = '\0';

    return count;
}

// d_tests_big_of_text
//   the digits _text[_start .. _end) as an integer; the caller keeps them
// below 76.
static struct d_tests_big
d_tests_big_of_text(
    const char* _text,
    size_t      _start,
    size_t      _end
)
{
    const struct d_tests_big ten    = d_tests_big_of_umax(10u, false);
    struct d_tests_big       result = d_tests_big_of_umax(0u, false);
    size_t                   i      = 0u;

    // each digit: times 10, plus it
    for (i = _start; i < _end; ++i)
    {
        result = d_tests_big_add(
            d_tests_big_mul(result, ten),
            d_tests_big_of_umax((d_math_umax)(_text[i] - '0'), false));
    }

    return result;
}

// d_tests_significant
//   the digits of _text[_start .. _end) from the first nonzero one: where
// they start, into *_first.
static size_t
d_tests_significant(
    const char* _text,
    size_t      _start,
    size_t      _end,
    size_t*     _first
)
{
    size_t first = _start;

    // the leading zeros
    while ( (first < _end) &&
            (_text[first] == '0') )
    {
        ++first;
    }

    *_first = first;

    return _end - first;
}

// d_tests_two_words
//   2^(2w), the first number past two words.
static struct d_tests_big
d_tests_two_words(void)
{
    return d_tests_big_shl(d_tests_big_of_umax(1u, false), 2 * D_TESTS_WORD);
}

// d_tests_ref_decimal
//   the decimal number _text, as its digits d and exponent e, then as a
// reduced ratio; past the reference where d has more significant digits
// than an exact decimal can, or e is past any exact decimal's.
static struct d_tests_ref
d_tests_ref_decimal(
    const char* _text,
    size_t      _length,
    size_t      _start
)
{
    static char digits[D_TESTS_TEXT_SIZE];

    struct d_tests_ref result   = { D_TESTS_REF_VALUE, false, true,
                                    { { 0UL }, 0 }, { { 0UL }, 0 } };
    size_t             i        = _start;
    long               count    = 0;     // significant digits kept
    long               fraction = 0;     // digits after the point
    long               exponent = 0;
    long               e        = 0;
    long               twos     = 0;
    long               fives    = 0;
    long               k        = 0;
    bool               after    = false;
    bool               below    = false;
    bool               many     = false;
    unsigned long      rest     = 0UL;

    // the digits and the point
    while ( (i < _length) &&
            ( ( (_text[i] >= '0') &&
                (_text[i] <= '9') ) ||
              (_text[i] == '.') ) )
    {
        // the point, or a digit: kept from the first nonzero one
        if (_text[i] == '.')
        {
            after = true;
        }
        else
        {
            if ( (count > 0) ||
                 (_text[i] != '0') )
            {
                many = (count >= (D_TESTS_TEXT_SIZE - 1)) || many;
                digits[(many) ? (D_TESTS_TEXT_SIZE - 1) : count] = _text[i];
                count = (many) ? count : (count + 1);
            }

            fraction = (after) ? (fraction + 1) : fraction;
        }

        ++i;
    }

    // the exponent
    if (i < _length)
    {
        ++i;

        // its sign
        if ( (_text[i] == '+') ||
             (_text[i] == '-') )
        {
            below = (_text[i] == '-');
            ++i;
        }

        // its digits
        while (i < _length)
        {
            exponent = (exponent * 10L) + (long)(_text[i] - '0');
            ++i;
        }

        exponent = (below) ? -exponent : exponent;
    }

    // the zeros at the end of the digits, into the exponent
    e = exponent - fraction;

    while ( (count > 0) &&
            (digits[count - 1] == '0') )
    {
        --count;
        ++e;
    }

    // 0
    if (count == 0)
    {
        result.n = d_tests_big_of_umax(0u, false);
        result.d = d_tests_big_of_umax(1u, false);

        return result;
    }

    // more digits than an exact decimal has, or past the reference
    if ( (many) ||
         (count > D_TESTS_EXACT_MOST) ||
         (e >= 20) ||
         (e <= -118) )
    {
        result.kind = D_TESTS_REF_BEYOND;

        return result;
    }

    result.n = d_tests_big_of_text(digits, 0u, (size_t)count);
    result.d = d_tests_big_of_umax(1u, false);

    // a whole number: times 10^e
    for (k = 0; k < e; ++k)
    {
        result.n = d_tests_big_mul(result.n, d_tests_big_of_umax(10u, false));
    }

    // a fraction of 10^k: the 2s and 5s it shares taken out
    k = (e < 0) ? -e : 0;

    while ( (twos < k) &&
            ((result.n.limb[0] & 1UL) == 0UL) )
    {
        result.n = d_tests_big_halve(result.n);
        ++twos;
    }

    while (fives < k)
    {
        const struct d_tests_big fifth =
            d_tests_big_div_small(result.n, 5UL, &rest);

        // no more 5s
        if (rest != 0UL)
        {
            break;
        }

        result.n = fifth;
        ++fives;
    }

    // the denominator 2^(k - twos) 5^(k - fives)
    result.d = d_tests_big_shl(d_tests_big_of_umax(1u, false),
                               (int)(k - twos));

    for (i = 0u; (long)i < (k - fives); ++i)
    {
        result.d = d_tests_big_mul(result.d, d_tests_big_of_umax(5u, false));
    }

    return result;
}

// d_tests_ref_of
//   what _text, one literal, is.
static struct d_tests_ref
d_tests_ref_of(
    const char* _text,
    size_t      _length
)
{
    struct d_tests_ref result   = { D_TESTS_REF_VALUE, false, false,
                                    { { 0UL }, 0 }, { { 0UL }, 0 } };
    size_t             i        = 0u;
    size_t             slash    = 0u;
    size_t             space    = 0u;
    size_t             w_start  = 0u;
    size_t             w_end    = 0u;
    size_t             n_start  = 0u;
    size_t             d_start  = 0u;
    size_t             n_zeros  = 0u;
    size_t             d_zeros  = 0u;
    size_t             common   = 0u;
    size_t             first    = 0u;
    bool               negative = false;
    bool               mixed    = false;

    // the sign
    if ( (_text[0] == '+') ||
         (_text[0] == '-') )
    {
        negative = (_text[0] == '-');
        i        = 1u;
    }

    // a slash, and a space before it
    for (slash = i; (slash < _length) && (_text[slash] != '/'); ++slash)
    {
        space = (_text[slash] == ' ') ? slash : space;
    }

    // no slash: a decimal number
    if (slash == _length)
    {
        result          = d_tests_ref_decimal(_text, _length, i);
        result.negative = negative;

        return result;
    }

    mixed   = (space > 0u);
    w_start = i;
    w_end   = i;

    // a mixed number: its whole part, then the spaces
    if (mixed)
    {
        while (_text[w_end] != ' ')
        {
            ++w_end;
        }

        n_start = space + 1u;
    }
    else
    {
        n_start = i;
    }

    d_start = slash + 1u;

    // the zeros both terms end with
    while ( ((slash - n_zeros) > n_start) &&
            (_text[slash - n_zeros - 1u] == '0') )
    {
        ++n_zeros;
    }

    while ( ((_length - d_zeros) > d_start) &&
            (_text[_length - d_zeros - 1u] == '0') )
    {
        ++d_zeros;
    }

    common = (n_zeros < d_zeros) ? n_zeros : d_zeros;

    // a term past the reference's digits is past two words
    if ( (d_tests_significant(_text, n_start, slash - common, &first) > 75u) ||
         (d_tests_significant(_text, d_start, _length - common, &first) >
          75u) ||
         ( (mixed) &&
           (d_tests_significant(_text, w_start, w_end, &first) > 75u) ) )
    {
        result.kind = D_TESTS_REF_RANGE;

        // over 0 even so
        if (d_tests_significant(_text, d_start, _length - common, &first) ==
                0u)
        {
            result.kind = D_TESTS_REF_POLE;
        }

        return result;
    }

    (void)d_tests_significant(_text, n_start, slash - common, &first);
    result.n = d_tests_big_of_text(_text, first, slash - common);
    (void)d_tests_significant(_text, d_start, _length - common, &first);
    result.d = d_tests_big_of_text(_text, first, _length - common);

    // over 0
    if (d_tests_big_is_zero(result.d))
    {
        result.kind = (d_tests_big_is_zero(result.n)) ? D_TESTS_REF_INVALID
                                                      : D_TESTS_REF_POLE;

        return result;
    }

    // terms past two words
    if ( (d_tests_big_cmp_mag(result.n, d_tests_two_words()) >= 0) ||
         (d_tests_big_cmp_mag(result.d, d_tests_two_words()) >= 0) )
    {
        result.kind = D_TESTS_REF_RANGE;

        return result;
    }

    // a whole part: the numerator w d + n, past two words where it is
    if (mixed)
    {
        struct d_tests_big w = d_tests_big_of_umax(0u, false);

        (void)d_tests_significant(_text, w_start, w_end, &first);
        w        = d_tests_big_of_text(_text, first, w_end);
        result.n = d_tests_big_add(d_tests_big_mul(w, result.d), result.n);

        // past two words
        if (d_tests_big_cmp_mag(result.n, d_tests_two_words()) >= 0)
        {
            result.kind = D_TESTS_REF_RANGE;

            return result;
        }
    }

    result.negative = ( (negative) &&
                        (!d_tests_big_is_zero(result.n)) );

    return result;
}


//==============================================================================
// 3.  WHAT A VALUE MUST BE
//==============================================================================

// d_tests_is_plus_zero
//   whether _x is +0: 0, its reciprocal +infinity.
static bool
d_tests_is_plus_zero(
    long double _x
)
{
    volatile long double zero = _x;

    return ( (_x == 0.0L) &&
             ((1.0L / zero) > 0.0L) );
}

// d_tests_same
//   whether _a and _b are the same long double, a zero's sign included.
static bool
d_tests_same(
    long double _a,
    long double _b
)
{
    return ( (_a == _b) &&
             (d_tests_is_plus_zero(_a) == d_tests_is_plus_zero(_b)) );
}

// d_tests_is_error
//   whether _v is the error _code, unflagged.
static bool
d_tests_is_error(
    struct d_math_value     _v,
    enum d_math_value_error _code
)
{
    return ( (_v.kind == D_MATH_VALUE_ERROR) &&
             (_v.code == (int)_code) &&
             (_v.flags == 0u) );
}

// d_tests_exact_value
//   whether _v is exactly n / d of _ref: an integer where its denominator
// is 1, a rational otherwise, in the one form, unflagged, its unused field
// 0, and equal to the ratio by cross products.
static bool
d_tests_exact_value(
    struct d_math_value       _v,
    const struct d_tests_ref* _ref
)
{
    struct d_tests_exact want = d_tests_exact_from(_ref->n, _ref->d);
    struct d_tests_exact got  = d_tests_exact_of(0, 1);

    // the kind and the form
    if ( ( (_v.kind != D_MATH_VALUE_INTEGER) &&
           (_v.kind != D_MATH_VALUE_RATIONAL) ) ||
         ( (_v.kind == D_MATH_VALUE_INTEGER) != (_v.exact.den == 1) ) ||
         (!d_tests_in_one_form(_v.exact)) ||
         (_v.flags != 0u) ||
         (_v.real != 0.0L) )
    {
        return false;
    }

    want.n.negative = (_ref->negative) ? 1 : 0;
    got             = d_tests_exact_of(_v.exact.num, _v.exact.den);

    return (d_tests_exact_cmp(want, got) == 0);
}

// d_tests_ref_fits
//   whether _ref's ratio, reduced and signed, lies within d_math_imax.
static bool
d_tests_ref_fits(
    const struct d_tests_ref* _ref
)
{
    struct d_tests_exact x = d_tests_exact_from(_ref->n, _ref->d);

    x.n.negative = (_ref->negative) ? 1 : 0;

    return d_tests_exact_fits(x);
}

// d_tests_check_decimal_types
//   for a decimal literal with an exact value: that the value converted to
// each type is what decimal.h reads from the text, one rounding either way.
static bool
d_tests_check_decimal_types(
    struct d_math_value _v,
    const char*         _text,
    size_t              _length
)
{
    float       f_value  = 0.0f;
    double      d_value  = 0.0;
    long double ld_value = 0.0L;
    float       f_text   = 0.0f;
    double      d_text   = 0.0;
    long double ld_text  = 0.0L;

    (void)d_math_value_to_f(&f_value, _v);
    (void)d_math_value_to_d(&d_value, _v);
    (void)d_math_value_to_ld(&ld_value, _v);
    (void)d_math_decimal_to_f(&f_text, _text, _length);
    (void)d_math_decimal_to_d(&d_text, _text, _length);
    (void)d_math_decimal_to_ld(&ld_text, _text, _length);

    // an exact 0 has no sign; the text's 0 may
    return ( (f_value == f_text) &&
             (d_value == d_text) &&
             (ld_value == ld_text) );
}

// d_tests_check
//   _text, one literal: its value, as the reference asks; its length; and
// with a character after it, a length unchanged and no value.
static void
d_tests_check(
    const char* _text,
    size_t      _length,
    int         _line
)
{
    static char        longer[D_TESTS_TEXT_SIZE + 1];
    static struct d_tests_ref ref;

    struct d_math_value value    = d_math_value_from_bool(true);
    long double         nearest  = 0.0L;
    bool                read     = false;
    bool                ok       = false;

    read = d_math_literal_to_value(&value, _text, _length);
    ref  = d_tests_ref_of(_text, _length);

    // by what the reference makes of it
    if (!read)
    {
        ok = false;
    }
    else if (ref.kind == D_TESTS_REF_POLE)
    {
        ok = d_tests_is_error(value, D_MATH_VALUE_ERROR_POLE);
    }
    else if (ref.kind == D_TESTS_REF_INVALID)
    {
        ok = d_tests_is_error(value, D_MATH_VALUE_ERROR_INVALID);
    }
    else if (ref.kind == D_TESTS_REF_RANGE)
    {
        ok = d_tests_is_error(value, D_MATH_VALUE_ERROR_RANGE);
    }
    else if (ref.kind == D_TESTS_REF_BEYOND)
    {
        // decimal.h's long double, or past it, a range error
        ok = (d_math_decimal_to_ld(&nearest, _text, _length) ==
              D_MATH_DECIMAL_OVERFLOW)
                 ? d_tests_is_error(value, D_MATH_VALUE_ERROR_RANGE)
                 : ( (value.kind == D_MATH_VALUE_LONG_DOUBLE) &&
                     (value.flags == D_MATH_VALUE_FLAG_INEXACT) &&
                     (d_tests_same(value.real, nearest)) );
    }
    else if (d_tests_ref_fits(&ref))
    {
        ok = ( (d_tests_exact_value(value, &ref)) &&
               ( (!ref.decimal) ||
                 (d_tests_check_decimal_types(value, _text, _length)) ) );
    }
    else
    {
        // the nearest long double, flagged; for a decimal, decimal.h's
        ok = ( (value.kind == D_MATH_VALUE_LONG_DOUBLE) &&
               (value.flags == D_MATH_VALUE_FLAG_INEXACT) &&
               ( (d_tests_big_is_zero(ref.n))
                     ? d_tests_is_plus_zero(value.real)
                     : d_tests_nearest_ld(ref.negative, ref.n, ref.d,
                                          value.real) ) );

        // and decimal.h's reading of the same text
        if ( (ok) &&
             (ref.decimal) )
        {
            (void)d_math_decimal_to_ld(&nearest, _text, _length);
            ok = d_tests_same(value.real, nearest);
        }
    }

    d_tests_expect(ok, "the value", _text, _length, _line);
    d_tests_expect( (d_math_literal_length(_text, _length) == _length),
                    "the whole text, measured", _text, _length, _line );

    // a character after it
    if (_length < (size_t)D_TESTS_TEXT_SIZE)
    {
        memcpy(longer, _text, _length);
        longer[_length] = ';';
        value           = d_math_value_from_bool(true);
        d_tests_expect( (d_math_literal_length(longer, _length + 1u) ==
                         _length) &&
                        (!d_math_literal_to_value(&value, longer,
                                                  _length + 1u)) &&
                        (value.kind == D_MATH_VALUE_BOOLEAN),
                        "a character after it: measured apart, no value",
                        _text, _length, _line );
    }

    return;
}

#define D_TESTS_CHECK(text)                                                    \
    d_tests_check((text), strlen(text), __LINE__)


//==============================================================================
// 4.  THE TEXTS
//==============================================================================


// 4.1    The syntax
//------------------------------------------------------------------------------

// d_tests_syntax_case
//   struct: a text and the length of the literal it starts with.
struct d_tests_syntax_case
{
    const char* text;
    size_t      length;
};

static const struct d_tests_syntax_case g_syntax[] =
{
    { "",            0u }, { "+",           0u }, { "-",           0u },
    { "/",           0u }, { "/2",          0u }, { "1/",          1u },
    { "1/2",         3u }, { "-1/2",        4u }, { "+1/2",        4u },
    { "1 /2",        1u }, { "1/ 2",        1u }, { "1/-2",        1u },
    { "1/+2",        1u }, { "1/2/3",       3u }, { "1/2.5",       3u },
    { "1.5/2",       3u }, { "1e3/2",       3u }, { "1/2e3",       3u },
    { "1 1/2",       5u }, { "1  1/2",      6u }, { "-1 1/2",      6u },
    { "1 1",         1u }, { "1 1/",        1u }, { "1 /2",        1u },
    { "1 -1/2",      1u }, { "1.5 1/2",     3u }, { " 1/2",        0u },
    { "1 1/2 ",      5u }, { "1 1/2/3",     5u }, { "1\t1/2",      1u },
    { "0/0",         3u }, { "007/010",     7u }, { "1 0/0",       5u },
    { "12 3/4x",     6u }, { ".5/2",        2u }, { "5./2",        2u },
    { "1e5",         3u }, { "1e",          1u }, { "1x",          1u },
    { "x1",          0u }, { "--1",         0u }, { "+-1/2",       0u },
    { "1/2 3/4",     3u }, { "1 1 1/2",     1u }, { "00 00/00",    8u }
};

// d_tests_syntax
//   each case's length; its value where the whole text is the literal, and
// otherwise no value and nothing stored.
static void
d_tests_syntax(void)
{
    const size_t cases = sizeof(g_syntax) / sizeof(g_syntax[0]);

    size_t i = 0u;

    // each case
    for (i = 0u; i < cases; ++i)
    {
        const char*         text   = g_syntax[i].text;
        const size_t        length = strlen(text);
        struct d_math_value value  = d_math_value_from_bool(true);

        D_TESTS_EXPECT( (d_math_literal_length(text, length) ==
                         g_syntax[i].length),
                        "the literal's length", text, length );

        // the whole text one literal, or not
        if ( (g_syntax[i].length == length) &&
             (length > 0u) )
        {
            d_tests_check(text, length, __LINE__);
        }
        else
        {
            D_TESTS_EXPECT( (!d_math_literal_to_value(&value, text, length)) &&
                            (value.kind == D_MATH_VALUE_BOOLEAN) &&
                            (value.code == 1),
                            "no literal: no value, nothing stored", text,
                            length );
        }
    }

    // the length given, not the NUL, ends the text
    D_TESTS_EXPECT( (d_math_literal_length("1/23", 3u) == 3u) &&
                    (d_math_literal_length("1 1/2", 4u) == 1u) &&
                    (d_math_literal_length("12/", 2u) == 2u),
                    "a length short of the NUL", "1/23", 4u );
    d_tests_check("1/23", 3u, __LINE__);
    d_tests_check("1 1/25", 5u, __LINE__);

    return;
}

// 4.2    Known values
//------------------------------------------------------------------------------

// d_tests_known_exact
//   _text must be exactly _num / _den.
static void
d_tests_known_exact(
    const char* _text,
    d_math_imax _num,
    d_math_imax _den,
    int         _line
)
{
    const size_t        length = strlen(_text);
    struct d_math_value value  = d_math_value_none();

    (void)d_math_literal_to_value(&value, _text, length);
    d_tests_expect( ( (value.kind == D_MATH_VALUE_INTEGER) ||
                      (value.kind == D_MATH_VALUE_RATIONAL) ) &&
                    (value.exact.num == _num) &&
                    (value.exact.den == _den),
                    "the known exact value", _text, length, _line );
    d_tests_check(_text, length, _line);

    return;
}

// d_tests_known_other
//   _text must be the error _code, or with _code 0, the long double _real,
// flagged inexact.
static void
d_tests_known_other(
    const char* _text,
    int         _code,
    long double _real,
    bool        _reference,
    int         _line
)
{
    const size_t         length = strlen(_text);
    volatile long double real   = _real;
    struct d_math_value  value  = d_math_value_none();

    (void)d_math_literal_to_value(&value, _text, length);
    d_tests_expect( (_code != 0)
                        ? ( (value.kind == D_MATH_VALUE_ERROR) &&
                            (value.code == _code) )
                        : ( (value.kind == D_MATH_VALUE_LONG_DOUBLE) &&
                            (value.flags == D_MATH_VALUE_FLAG_INEXACT) &&
                            (d_tests_same(value.real, real)) ),
                    "the known value", _text, length, _line );

    // the reference too
    if (_reference)
    {
        d_tests_check(_text, length, _line);
    }

    return;
}

#define D_TESTS_POLE     ((int)D_MATH_VALUE_ERROR_POLE)
#define D_TESTS_INVALID  ((int)D_MATH_VALUE_ERROR_INVALID)
#define D_TESTS_RANGE    ((int)D_MATH_VALUE_ERROR_RANGE)

// d_tests_known
//   each form's plain cases, signs and zeros, poles, and the ends.
static void
d_tests_known(void)
{
    const long double ld_zero = 0.0L;

    // decimal numbers
    d_tests_known_exact("42", 42, 1, __LINE__);
    d_tests_known_exact("-42", -42, 1, __LINE__);
    d_tests_known_exact("0.1", 1, 10, __LINE__);
    d_tests_known_exact(".5", 1, 2, __LINE__);
    d_tests_known_exact("5.", 5, 1, __LINE__);
    d_tests_known_exact("+2.50", 5, 2, __LINE__);
    d_tests_known_exact("1500e-2", 15, 1, __LINE__);
    d_tests_known_exact("0.0625", 1, 16, __LINE__);
    d_tests_known_exact("-3.2E-3", -2, 625, __LINE__);
    d_tests_known_exact("12e3", 12000, 1, __LINE__);
    d_tests_known_exact("-0", 0, 1, __LINE__);
    d_tests_known_exact("-0.000e-9", 0, 1, __LINE__);

    // fractions and mixed numbers
    d_tests_known_exact("2/3", 2, 3, __LINE__);
    d_tests_known_exact("6/4", 3, 2, __LINE__);
    d_tests_known_exact("-6/4", -3, 2, __LINE__);
    d_tests_known_exact("+7/14", 1, 2, __LINE__);
    d_tests_known_exact("8/2", 4, 1, __LINE__);
    d_tests_known_exact("0/5", 0, 1, __LINE__);
    d_tests_known_exact("-0/5", 0, 1, __LINE__);
    d_tests_known_exact("00010/0020", 1, 2, __LINE__);
    d_tests_known_exact("1 1/2", 3, 2, __LINE__);
    d_tests_known_exact("-1 1/2", -3, 2, __LINE__);
    d_tests_known_exact("2 6/4", 7, 2, __LINE__);
    d_tests_known_exact("0 1/3", 1, 3, __LINE__);
    d_tests_known_exact("3 0/7", 3, 1, __LINE__);
    d_tests_known_exact("-0 0/7", 0, 1, __LINE__);
    d_tests_known_exact("1   10/100", 11, 10, __LINE__);

    // over 0
    d_tests_known_other("1/0", D_TESTS_POLE, ld_zero, true, __LINE__);
    d_tests_known_other("-1/00", D_TESTS_POLE, ld_zero, true, __LINE__);
    d_tests_known_other("0/0", D_TESTS_INVALID, ld_zero, true, __LINE__);
    d_tests_known_other("1 1/0", D_TESTS_POLE, ld_zero, true, __LINE__);
    d_tests_known_other("1 0/0", D_TESTS_INVALID, ld_zero, true, __LINE__);
    d_tests_known_other("00 00/00", D_TESTS_INVALID, ld_zero, true,
                        __LINE__);

    // past every long double, and below the least
    d_tests_known_other("1e5000", D_TESTS_RANGE, ld_zero, true, __LINE__);
    d_tests_known_other("-1e5000", D_TESTS_RANGE, ld_zero, true, __LINE__);
    d_tests_known_other("1e-5000", 0, 0.0L, true, __LINE__);
    d_tests_known_other("-1e-5000", 0, -0.0L, true, __LINE__);
    d_tests_known_other("1e99999999999999999999999999", D_TESTS_RANGE,
                        ld_zero, false, __LINE__);
    d_tests_known_other("-2e-99999999999999999999999999", 0, -0.0L, false,
                        __LINE__);

    // decimals beyond the family
    d_tests_known_other("1e30", 0, 1e30L, true, __LINE__);
    d_tests_known_other("0.1234567890123456789012345678", 0,
                        0.1234567890123456789012345678L, true, __LINE__);

    return;
}

// 4.3    Integers at the family's ends
//------------------------------------------------------------------------------

// d_tests_ends
//   the integers from -6 to 6, and within 3 of 2^(w-1) - 1 and of its
// negative, which takes in -2^(w-1), each with a sign and written plainly,
// with a point and zeros, and in scientific notation; and powers of 10 and
// 2 to past the family.
static void
d_tests_ends(void)
{
    static char text[D_TESTS_TEXT_SIZE];
    static char digits[64];

    const d_math_umax most = (d_math_umax)D_MATH_IMAX_MAX;

    d_math_umax base   = 0u;
    d_math_umax offset = 0u;
    int         sign   = 0;
    int         form   = 0;
    int         power  = 0;

    // around each end, either sign
    for (sign = 0; sign < 2; ++sign)
    {
        for (base = 0u; base < 2u; ++base)
        {
            for (offset = 0u; offset < 7u; ++offset)
            {
                const d_math_umax magnitude =
                    (base == 0u) ? offset : (most - 3u + offset);
                size_t            count     = 0u;

                (void)d_tests_text_umax(magnitude, digits);
                count = strlen(digits);

                // plainly; with ".000"; as d.ddd e(count - 1)
                for (form = 0; form < 3; ++form)
                {
                    char* at = text;

                    *at = (sign == 0) ? '-' : '+';
                    ++at;

                    // the form
                    if (form == 0)
                    {
                        sprintf(at, "%s", digits);
                    }
                    else if (form == 1)
                    {
                        sprintf(at, "%s.000", digits);
                    }
                    else
                    {
                        sprintf(at, "%c.%se%lu", digits[0], digits + 1,
                                (unsigned long)(count - 1u));
                    }

                    D_TESTS_CHECK(text);
                }
            }
        }
    }

    // powers of 10 and of 2, to past the family
    for (power = 0; power < 22; ++power)
    {
        sprintf(text, "1e%d", power);
        D_TESTS_CHECK(text);
        sprintf(text, "-1e-%d", power);
        D_TESTS_CHECK(text);
    }

    for (power = 0; power < (D_TESTS_WORD + 2); ++power)
    {
        const struct d_tests_big big =
            d_tests_big_shl(d_tests_big_of_umax(1u, false), power);
        unsigned long            rest = 0UL;
        struct d_tests_big       x    = big;
        int                      n    = 0;
        int                      j    = 0;
        char                     reversed[80];

        // its digits
        do
        {
            x             = d_tests_big_div_small(x, 10UL, &rest);
            reversed[n++] = (char)('0' + (int)rest);
        } while (!d_tests_big_is_zero(x));

        for (j = 0; j < n; ++j)
        {
            text[j] = reversed[n - 1 - j];
        }

        text[n] = '\0';
        D_TESTS_CHECK(text);
        sprintf(text + n, "/%d", 3);
        D_TESTS_CHECK(text);
    }

    return;
}

// d_tests_two_word_edges
//   fractions and mixed numbers at the edge of two words: 2^(2w) - 1 and
// 2^(2w) as a numerator, a denominator and a whole part, and mixed numbers
// whose w d + n comes to 2^(2w) - 1 or to 2^(2w) exactly.
static void
d_tests_two_word_edges(void)
{
    static char text[D_TESTS_TEXT_SIZE];
    static char top[128];
    static char less[128];
    static char past[128];

    const struct d_tests_big one = d_tests_big_of_umax(1u, false);
    const struct d_tests_big two = d_tests_big_of_umax(2u, false);

    (void)d_tests_digits_of(d_tests_big_add(d_tests_two_words(),
                                            d_tests_big_neg(one)), top);
    (void)d_tests_digits_of(d_tests_big_add(d_tests_two_words(),
                                            d_tests_big_neg(two)), less);
    (void)d_tests_digits_of(d_tests_two_words(), past);

    // as a numerator and a denominator
    sprintf(text, "%s/1", top);
    D_TESTS_CHECK(text);
    sprintf(text, "-%s/1", past);
    D_TESTS_CHECK(text);
    sprintf(text, "1/%s", top);
    D_TESTS_CHECK(text);
    sprintf(text, "1/%s", past);
    D_TESTS_CHECK(text);
    sprintf(text, "%s/%s", top, top);
    D_TESTS_CHECK(text);

    // a mixed number's w d + n: within two words, and just past them
    sprintf(text, "%s 1/1", less);
    D_TESTS_CHECK(text);
    sprintf(text, "%s 1/1", top);
    D_TESTS_CHECK(text);
    sprintf(text, "-1 %s/1", less);
    D_TESTS_CHECK(text);
    sprintf(text, "1 %s/1", top);
    D_TESTS_CHECK(text);
    sprintf(text, "%s 0/1", top);
    D_TESTS_CHECK(text);
    sprintf(text, "%s 1/2", top);
    D_TESTS_CHECK(text);

    return;
}

// 4.4    Exact decimals from rationals
//------------------------------------------------------------------------------

static unsigned long g_state = 1414213562UL;

// d_tests_random32
//   32 random bits: xorshift, in an unsigned long.
static unsigned long
d_tests_random32(void)
{
    g_state = g_state ^ ((g_state << 13) & 0xFFFFFFFFUL);
    g_state = g_state ^ (g_state >> 17);
    g_state = g_state ^ ((g_state << 5) & 0xFFFFFFFFUL);
    g_state = g_state & 0xFFFFFFFFUL;

    return g_state;
}

// d_tests_below
//   a random number from 0 to _n - 1.
static long
d_tests_below(
    long _n
)
{
    return (long)(d_tests_random32() % (unsigned long)_n);
}

// d_tests_random_umax
//   a random number of 0 to _bits bits, the length itself random.
static d_math_umax
d_tests_random_umax(
    int _bits
)
{
    const int   length = (int)d_tests_below((long)_bits + 1L);
    d_math_umax result = 0u;
    int         i      = 0;

    // sixteen bits at a time
    for (i = 0; i < length; i += 16)
    {
        result = (result << 8) << 8;
        result = result | (d_math_umax)(d_tests_random32() & 0xFFFFUL);
    }

    // cut to the length
    if (length < D_TESTS_WORD)
    {
        result = result & ((((d_math_umax)1) << length) - 1u);
    }

    return result;
}

// d_tests_plain
//   _digits, an integer m, as m / 10^_k written plainly at _at: the point
// _k digits from the right, "0." and zeros before them where they are
// fewer; returns the end.
static char*
d_tests_plain(
    char*       _at,
    const char* _digits,
    int         _k
)
{
    const int count = (int)strlen(_digits);

    int i = 0;

    // fewer digits than _k: "0.", zeros, then the digits
    if ( (_k > 0) &&
         (_k >= count) )
    {
        *_at++ = '0';
        *_at++ = '.';

        for (i = 0; i < (_k - count); ++i)
        {
            *_at++ = '0';
        }
    }

    // the digits, the point among them
    for (i = 0; i < count; ++i)
    {
        // the point
        if ( (_k > 0) &&
             (_k < count) &&
             (i == (count - _k)) )
        {
            *_at++ = '.';
        }

        *_at++ = _digits[i];
    }

    *_at = '\0';

    return _at;
}

// d_tests_write_decimal
//   _digits, an integer m, as m / 10^_k in one of five forms, into _text:
// plainly; with leading zeros, and trailing ones after a point; as the
// integer and an exponent; as d.ddd and an exponent; and plainly with the
// point elsewhere, and an exponent that moves it back. _sign first, where
// it is not NUL.
static void
d_tests_write_decimal(
    char*       _text,
    const char* _digits,
    int         _k,
    int         _form,
    char        _sign
)
{
    const int count = (int)strlen(_digits);

    char* at = _text;

    // the sign
    if (_sign != '\0')
    {
        *at++ = _sign;
    }

    // the form
    if (_form == 0)
    {
        (void)d_tests_plain(at, _digits, _k);
    }
    else if (_form == 1)
    {
        *at++ = '0';
        *at++ = '0';
        at    = d_tests_plain(at, _digits, _k);
        (void)sprintf(at, "%s000", (_k == 0) ? "." : "");
    }
    else if (_form == 2)
    {
        (void)sprintf(at, "%se-%d", _digits, _k);
    }
    else if (_form == 3)
    {
        (void)sprintf(at, "%c.%sE%d", _digits[0],
                      (count > 1) ? (_digits + 1) : "0", (count - 1) - _k);
    }
    else
    {
        const int moved = (int)d_tests_below((long)count + 1L);

        at = d_tests_plain(at, _digits, moved);
        (void)sprintf(at, "e%d", moved - _k);
    }

    return;
}

// d_tests_exact_decimals
//   _count rationals p / (2^x 5^y) within the family, one in six with a
// factor of 5 more in the denominator, which takes it past the family where
// 2^x 5^y was near the top; then four more: (2^(w-1) - 1) / 2^(w-2), the
// longest an exact decimal can be, and -2^(w-1) over the same, -2 with
// w - 2 zeros after the point, each also with the factor of 5 more. Each
// is written out as a decimal in every form, and checked.
static void
d_tests_exact_decimals(
    long _count
)
{
    static char text[D_TESTS_TEXT_SIZE];
    static char digits[128];

    const d_math_umax most = (d_math_umax)D_MATH_IMAX_MAX;

    long i    = 0;
    int  form = 0;

    // each rational
    for (i = 0; i < (_count + 4L); ++i)
    {
        d_math_umax        p     = d_tests_random_umax(D_TESTS_WORD - 1);
        d_math_umax        q     = 1u;
        int                x     = 0;
        int                y     = 0;
        int                k     = 0;
        int                j     = 0;
        bool               past  = (d_tests_below(6L) == 0);
        char               sign  = '\0';
        struct d_tests_big m     = d_tests_big_of_umax(0u, false);

        // the longest: the most digits there are, and those at both ends
        if (i >= _count)
        {
            p = (i < (_count + 2L)) ? most : (most + 1u);
            x = D_TESTS_WORD - 2;
            y = 0;
            past = ((i % 2L) == 1L);
        }
        else
        {
            x = (int)d_tests_below((long)D_TESTS_WORD - 1L);
            y = (int)d_tests_below(D_TESTS_WORD / 2L - 4L);
        }

        // the denominator, within the family, or one factor past it
        for (j = 0; j < x; ++j)
        {
            q = (q <= (most / 2u)) ? (q * 2u) : q;
        }

        for (j = 0; j < y; ++j)
        {
            q = (q <= (most / 5u)) ? (q * 5u) : q;
        }

        x = 0;
        y = 0;

        // its powers of 2 and 5 again, as built
        while ((q % ((d_math_umax)1 << (x + 1))) == 0u)
        {
            ++x;

            // the largest power of 2 a word holds, at most
            if (x == (D_TESTS_WORD - 2))
            {
                break;
            }
        }

        {
            d_math_umax rest = q >> x;

            while ( (rest % 5u) == 0u )
            {
                rest = rest / 5u;
                ++y;
            }
        }

        // past: a factor of 5 more in the denominator
        y = y + ((past) ? 1 : 0);

        // the digits, m = p 2^(k-x) 5^(k-y), a reference integer: the
        // rational is m / 10^k
        k = (x > y) ? x : y;
        m = d_tests_big_of_umax(p, false);
        m = d_tests_big_shl(m, k - x);

        for (j = 0; j < (k - y); ++j)
        {
            m = d_tests_big_mul(m, d_tests_big_of_umax(5u, false));
        }

        (void)d_tests_digits_of(m, digits);

        // a sign: -, + or none
        sign = (p == (most + 1u)) ? '-'
             : ( (d_tests_below(3L) == 0) ? '-'
                                          : ( (d_tests_below(2L) == 0)
                                                  ? '+' : '\0' ) );

        // every form
        for (form = 0; form < 5; ++form)
        {
            d_tests_write_decimal(text, digits, k, form, sign);
            D_TESTS_CHECK(text);
        }
    }

    return;
}

// 4.5    Random decimals
//------------------------------------------------------------------------------

// d_tests_random_decimals
//   _count decimals: up to 25 random digits, a point anywhere or none, and
// an exponent from -20 to 20, or none, which keeps the reference's numbers
// within its bits; checked against the reference.
static void
d_tests_random_decimals(
    long _count
)
{
    static char text[D_TESTS_TEXT_SIZE];

    long i = 0;

    // each text
    for (i = 0; i < _count; ++i)
    {
        const long digits = 1L + d_tests_below(25L);
        const long point  = d_tests_below(digits + 2L) - 1L;
        long       n      = 0;
        long       j      = 0;

        // a sign, sometimes
        if (d_tests_below(3L) == 0)
        {
            text[n++] = (d_tests_below(2L) == 0) ? '-' : '+';
        }

        // the digits: often few among zeros, so that some are exact
        for (j = 0; j < digits; ++j)
        {
            // the point
            if (j == point)
            {
                text[n++] = '.';
            }

            text[n++] = (d_tests_below(3L) == 0)
                            ? '0'
                            : (char)('0' + d_tests_below(10L));
        }

        // an exponent, mostly
        if (d_tests_below(4L) != 0)
        {
            n = n + (long)sprintf(text + n, "e%ld",
                                  d_tests_below(41L) - 20L);
        }

        text[n] = '\0';
        d_tests_check(text, (size_t)n, __LINE__);
    }

    return;
}

// 4.6    Long decimals and huge exponents
//------------------------------------------------------------------------------

// d_tests_long_decimals
//   _count decimals with more significant digits than any exact one has,
// to 1,500, the last one nonzero; and small ones with exponents past any
// exact one's, to past every long double: each the long double decimal.h
// reads, flagged, or a range error.
static void
d_tests_long_decimals(
    long _count
)
{
    static char text[D_TESTS_TEXT_SIZE];

    long i = 0;

    // each text
    for (i = 0; i < _count; ++i)
    {
        const long digits = D_TESTS_EXACT_MOST + 1L + d_tests_below(1500L);
        const long point  = d_tests_below(digits);
        long       n      = 0;
        long       j      = 0;

        // a sign, sometimes
        if (d_tests_below(2L) == 0)
        {
            text[n++] = '-';
        }

        for (j = 0; j < digits; ++j)
        {
            // the point
            if (j == point)
            {
                text[n++] = '.';
            }

            text[n++] = (j == (digits - 1L))
                            ? (char)('1' + d_tests_below(9L))
                            : (char)('0' + d_tests_below(10L));
        }

        n = n + (long)sprintf(text + n, "e%ld", d_tests_below(9001L) - 4500L);
        text[n] = '\0';
        d_tests_check(text, (size_t)n, __LINE__);

        // a few digits, and an exponent past any exact decimal's
        n = (long)sprintf(text, "%s%ld.%lde%ld",
                          (d_tests_below(2L) == 0) ? "-" : "",
                          1L + d_tests_below(999L), d_tests_below(1000L),
                          (d_tests_below(2L) == 0)
                              ? (20L + d_tests_below(5000L))
                              : (-130L - d_tests_below(5000L)));
        d_tests_check(text, (size_t)n, __LINE__);
    }

    return;
}

// 4.7    Fractions and mixed numbers
//------------------------------------------------------------------------------

// d_tests_write_term
//   the integer _x's digits at _at, then _zeros zeros; returns the end.
static char*
d_tests_write_term(
    char*              _at,
    struct d_tests_big _x,
    int                _zeros
)
{
    int i = 0;

    _at = _at + d_tests_digits_of(_x, _at);

    for (i = 0; i < _zeros; ++i)
    {
        *_at++ = '0';
    }

    *_at = '\0';

    return _at;
}

// d_tests_random_big
//   a random integer of 0 to _bits bits, below 256.
static struct d_tests_big
d_tests_random_big(
    int _bits
)
{
    const int          length = (int)d_tests_below((long)_bits + 1L);
    struct d_tests_big result = { { 0UL }, 0 };
    int                i      = 0;

    // sixteen bits at a time, the top cut to the length
    for (i = 0; (i * 16) < length; ++i)
    {
        result.limb[i] = d_tests_random32() & D_TESTS_LIMB_MASK;

        // the last limb
        if (((i + 1) * 16) > length)
        {
            result.limb[i] = result.limb[i] &
                             ((1UL << (length - (i * 16))) - 1UL);
        }
    }

    return result;
}

// d_tests_fractions
//   _count fractions and _count mixed numbers: terms of every size to a
// little past two words, a common factor multiplied in, common zeros
// appended, zeros and over 0 now and then.
static void
d_tests_fractions(
    long _count
)
{
    static char text[D_TESTS_TEXT_SIZE];

    const int top = (2 * D_TESTS_WORD) + 6;

    long i = 0;

    // each pair
    for (i = 0; i < (2L * _count); ++i)
    {
        const bool         mixed  = (i >= _count);
        struct d_tests_big n      = d_tests_random_big(top);
        struct d_tests_big d      = d_tests_random_big(top);
        struct d_tests_big w      = d_tests_random_big(
                                        (mixed) ? (D_TESTS_WORD + 4) : 0);
        struct d_tests_big g      = d_tests_random_big(24);
        const int          zeros  = (d_tests_below(5L) == 0)
                                        ? (int)d_tests_below(45L) : 0;
        char*              at     = text;

        // a common factor, where the terms stay small enough; or, now and
        // then, one of two words, over small cofactors
        if ( (d_tests_big_bits(n) < 100) &&
             (d_tests_big_bits(d) < 100) &&
             (!d_tests_big_is_zero(g)) )
        {
            n = d_tests_big_mul(n, g);
            d = d_tests_big_mul(d, g);
        }
        else if (d_tests_below(5L) == 0)
        {
            g = d_tests_big_of_umax(1u, false);
            g = d_tests_big_add(
                    d_tests_big_shl(g, D_TESTS_WORD +
                                       (int)d_tests_below(D_TESTS_WORD - 20)),
                    d_tests_random_big(D_TESTS_WORD));
            n = d_tests_big_mul(g, d_tests_random_big(16));
            d = d_tests_big_mul(g, d_tests_big_add(
                                       d_tests_random_big(16),
                                       d_tests_big_of_umax(1u, false)));
        }

        // over 0, or 0 over, now and then
        if (d_tests_below(40L) == 0)
        {
            d = d_tests_big_of_umax(0u, false);
        }

        if (d_tests_below(30L) == 0)
        {
            n = d_tests_big_of_umax(0u, false);
        }

        // a mixed number's whole part keeps w d within the reference
        if ( (mixed) &&
             ((d_tests_big_bits(w) + d_tests_big_bits(d)) > 200) )
        {
            w = d_tests_big_of_umax(d_tests_random_umax(20), false);
        }

        // a sign, sometimes
        if (d_tests_below(3L) == 0)
        {
            *at++ = (d_tests_below(2L) == 0) ? '-' : '+';
        }

        // the whole part and its spaces
        if (mixed)
        {
            at    = d_tests_write_term(at, w, 0);
            *at++ = ' ';

            // more spaces, sometimes
            if (d_tests_below(4L) == 0)
            {
                *at++ = ' ';
            }
        }

        at    = d_tests_write_term(at, n, zeros);
        *at++ = '/';
        at    = d_tests_write_term(at, d, zeros + (int)d_tests_below(2L));
        d_tests_check(text, (size_t)(at - text), __LINE__);
    }

    return;
}

// 4.8    A million characters
//------------------------------------------------------------------------------

static char g_long[(2L * D_TESTS_LONG_ZEROS) + 64L];

// d_tests_long_text
//   _head, then _zeros zeros, then _middle, then _zeros zeros again where
// _twice, then _tail, into g_long; returns its length.
static size_t
d_tests_long_text(
    const char* _head,
    const char* _middle,
    bool        _twice,
    const char* _tail
)
{
    const long zeros = D_TESTS_LONG_ZEROS;

    size_t n = 0u;

    n = (size_t)sprintf(g_long, "%s", _head);
    memset(g_long + n, '0', (size_t)zeros);
    n = n + (size_t)zeros;
    n = n + (size_t)sprintf(g_long + n, "%s", _middle);

    // the zeros again
    if (_twice)
    {
        memset(g_long + n, '0', (size_t)zeros);
        n = n + (size_t)zeros;
    }

    n = n + (size_t)sprintf(g_long + n, "%s", _tail);

    return n;
}

// d_tests_long_texts
//   a million zeros: after a point, with an exponent that brings the 1
// back; leading a fraction's terms; and ending both terms, which leaves 1.
static void
d_tests_long_texts(void)
{
    struct d_math_value value  = d_math_value_none();
    size_t              length = 0u;

    length = d_tests_long_text("0.", "1e1000006", false, "");
    (void)d_math_literal_to_value(&value, g_long, length);
    D_TESTS_EXPECT( (value.kind == D_MATH_VALUE_INTEGER) &&
                    (value.exact.num == 1),
                    "a million zeros after the point, e1000006: 1", g_long,
                    length );

    length = d_tests_long_text("-", "3/", true, "6");
    (void)d_math_literal_to_value(&value, g_long, length);
    D_TESTS_EXPECT( (value.kind == D_MATH_VALUE_RATIONAL) &&
                    (value.exact.num == -1) &&
                    (value.exact.den == 2),
                    "a million leading zeros on both terms: -1/2", g_long,
                    length );

    length = d_tests_long_text("7", "/2", false, "");
    memset(g_long + length, '0', (size_t)D_TESTS_LONG_ZEROS);
    length = length + (size_t)D_TESTS_LONG_ZEROS;
    g_long[length] = '\0';
    (void)d_math_literal_to_value(&value, g_long, length);
    D_TESTS_EXPECT( (value.kind == D_MATH_VALUE_RATIONAL) &&
                    (value.exact.num == 7) &&
                    (value.exact.den == 2),
                    "a million zeros ending both terms: 7/2", g_long,
                    length );

    length = d_tests_long_text("1 1", "/1", true, "");
    (void)d_math_literal_to_value(&value, g_long, length);
    D_TESTS_EXPECT( (value.kind == D_MATH_VALUE_INTEGER) &&
                    (value.exact.num == 2),
                    "1 and 10^n over 10^n: 2", g_long, length );

    length = d_tests_long_text("1", "/3", false, "");
    (void)d_math_literal_to_value(&value, g_long, length);
    D_TESTS_EXPECT( d_tests_is_error(value, D_MATH_VALUE_ERROR_RANGE),
                    "10^1000005 / 3: a range error", g_long, length );

    return;
}


//==============================================================================
// 5.  CONSTANT EVALUATION
//==============================================================================
// From C++14 the reading is a constant expression.

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

static constexpr struct d_math_value
d_tests_constexpr_literal(
    const char* _text,
    size_t      _length
)
{
    struct d_math_value result = d_math_value_none();

    d_math_literal_to_value(&result, _text, _length);

    return result;
}

static_assert( (d_tests_constexpr_literal("0.1", 3u).kind ==
                D_MATH_VALUE_RATIONAL) &&
               (d_tests_constexpr_literal("0.1", 3u).exact.num == 1) &&
               (d_tests_constexpr_literal("0.1", 3u).exact.den == 10),
               "0.1 is 1/10 as a constant expression" );
static_assert( (d_tests_constexpr_literal("-1 1/2", 6u).exact.num == -3) &&
               (d_tests_constexpr_literal("-1 1/2", 6u).exact.den == 2),
               "-1 1/2 is -3/2 as a constant expression" );
static_assert( (d_tests_constexpr_literal("1/0", 3u).kind ==
                D_MATH_VALUE_ERROR) &&
               (d_tests_constexpr_literal("1/0", 3u).code ==
                (int)D_MATH_VALUE_ERROR_POLE),
               "1/0 is a pole as a constant expression" );
static_assert( (d_tests_constexpr_literal("1e30", 4u).kind ==
                D_MATH_VALUE_LONG_DOUBLE) &&
               (d_tests_constexpr_literal("1e30", 4u).flags ==
                D_MATH_VALUE_FLAG_INEXACT),
               "1e30 is a flagged long double as a constant expression" );
static_assert( d_math_literal_length("12 3/4x", 7u) == 6u,
               "a length measured as a constant expression" );

// d_tests_constant_evaluation
//   the same at run time.
static void
d_tests_constant_evaluation(void)
{
    D_TESTS_EXPECT( (d_tests_constexpr_literal("0.1", 3u).exact.den == 10) &&
                    (d_tests_constexpr_literal("-1 1/2", 6u).exact.num ==
                     -3) &&
                    (d_tests_constexpr_literal("1/0", 3u).code ==
                     (int)D_MATH_VALUE_ERROR_POLE) &&
                    (d_tests_constexpr_literal("1e30", 4u).flags ==
                     D_MATH_VALUE_FLAG_INEXACT) &&
                    (d_math_literal_length("12 3/4x", 7u) == 6u),
                    "the constant expressions, at run time", "", 0u );

    return;
}

#endif  // C++14


int
main(void)
{
    d_tests_syntax();
    d_tests_known();
    d_tests_ends();
    d_tests_two_word_edges();
    d_tests_exact_decimals(3000L);
    d_tests_random_decimals(20000L);
    d_tests_long_decimals(300L);
    d_tests_fractions(6000L);
    d_tests_long_texts();
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    // the reference never needed more than its bits
    D_TESTS_EXPECT(!g_reference_overrun, "the reference's 256 bits held", "",
                   0u);

    printf("t_literal: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
