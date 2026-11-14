/*******************************************************************************
* djinterp [c/math]                                                  t_decimal.c
*
*   Conformance harness for decimal text to binary floating point
* (c/math/decimal.h): every result against an exact reference.
*   A result must be the float, double or long double nearest the text's
* value, ties to even, or an infinity from the largest finite value's upper
* midpoint on; a result's sign, a zero's included, must be the text's; and
* the status must say what happened: D_MATH_DECIMAL_OVERFLOW for an
* infinity, D_MATH_DECIMAL_UNDERFLOW for a subnormal or zero that is not
* exact, D_MATH_DECIMAL_OK otherwise. The reference reads the text itself,
* as n * 10^e, and compares it exactly with the midpoints between the result
* and its neighbours, and with the result itself, in integers of up to
* 54,400 bits.
*   The texts: the syntax's corner cases, each with the length
* d_math_decimal_length must give, and D_MATH_DECIMAL_SYNTAX, nothing
* stored, where the text is not one number; known values, from the
* compilers' own correctly rounded literals; for each type, values drawn at
* random from its least subnormal to its largest finite value, each written
* out exactly, and the midpoint above it, exactly, nudged up and nudged down:
* within the buffer's capacity; at its last digit, which a shift pushes out;
* and past it, where only the dropped digits tell a nudge from a tie. Then
* random texts of up to 2,000 digits, in every form the syntax allows,
* across each type's range and past both ends; and texts of a million
* characters, where the decimal point's place passes the limit it is held
* to. From C++14 the conversions are also checked as constant expressions.
*   The same file compiles as C++ (-x c++), at any level from C++98:
* decimal.h is one text in both languages.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_decimal.c -o t_decimal -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_decimal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.11.07
*******************************************************************************/
// std
#include <float.h>   // FLT_MANT_DIG ... LDBL_MAX
#include <math.h>    // frexpl, ldexpl, floorl, HUGE_VAL
#include <stdio.h>   // printf, sprintf
#include <stdlib.h>  // labs
#include <string.h>  // strlen, memcpy, memset
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/decimal.h"  // unit under test


#define D_TESTS_HUGE_LIMBS  3400L     // 54,400 bits
#define D_TESTS_HUGE_MASK   0xFFFFUL
#define D_TESTS_TEXT_SIZE   40000L    // a text, with room for its nudges
#define D_TESTS_LONG_ZEROS  1000005L  // past D_INTERNAL_MATH_DECIMAL_PLACE_MAX
#define D_TESTS_LONG_SIZE   (D_TESTS_LONG_ZEROS + 100L)

#define D_TESTS_FLOAT        0
#define D_TESTS_DOUBLE       1
#define D_TESTS_LONG_DOUBLE  2

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
// 2.  INTEGERS OF ANY SIZE, NEARLY
//==============================================================================
// The reference's integers: up to D_TESTS_HUGE_LIMBS limbs of 16 bits, the
// least significant first, each in an unsigned long, which holds a product
// of two limbs plus two carries. length counts the limbs in use, the top one
// nonzero; 0 has none. A result that would pass the limbs sets
// g_huge_overrun instead.

struct dj_tests_huge
{
    long          length;                     // the limbs in use
    unsigned long limb[D_TESTS_HUGE_LIMBS];   // 16 bits each, low first
};

static bool g_huge_overrun = false;

// d_tests_huge_set
//   _h = _value, below 2^32.
static void
d_tests_huge_set(
    struct dj_tests_huge* _h,
    unsigned long         _value
)
{
    unsigned long rest = _value & 0xFFFFFFFFUL;

    _h->length = 0;

    // sixteen bits at a time
    while (rest != 0UL)
    {
        _h->limb[_h->length] = rest & D_TESTS_HUGE_MASK;
        ++_h->length;
        rest = rest >> 16;
    }

    return;
}

// d_tests_huge_copy
//   _to = _from.
static void
d_tests_huge_copy(
    struct dj_tests_huge*       _to,
    const struct dj_tests_huge* _from
)
{
    _to->length = _from->length;

    // the limbs in use only
    if (_from->length > 0)
    {
        memcpy(_to->limb, _from->limb,
               (size_t)_from->length * sizeof(_from->limb[0]));
    }

    return;
}

// d_tests_huge_trim
//   drops zero limbs from the top.
static void
d_tests_huge_trim(
    struct dj_tests_huge* _h
)
{
    // the zeros on top
    while ( (_h->length > 0) &&
            (_h->limb[_h->length - 1] == 0UL) )
    {
        --_h->length;
    }

    return;
}

// d_tests_huge_bits
//   the bits _h needs: 0 for 0.
static long
d_tests_huge_bits(
    const struct dj_tests_huge* _h
)
{
    long          result = 0;
    unsigned long top    = 0UL;

    // 0
    if (_h->length == 0)
    {
        return 0;
    }

    result = (_h->length - 1) * 16L;
    top    = _h->limb[_h->length - 1];

    // the top limb's bits
    while (top != 0UL)
    {
        ++result;
        top = top >> 1;
    }

    return result;
}

// d_tests_huge_mul_add
//   _h = _h * _m + _a, _m and _a below 2^16.
static void
d_tests_huge_mul_add(
    struct dj_tests_huge* _h,
    unsigned long         _m,
    unsigned long         _a
)
{
    unsigned long carry = _a;
    long          i     = 0;

    // each limb, the carry moving up
    for (i = 0; i < _h->length; ++i)
    {
        const unsigned long product = (_h->limb[i] * _m) + carry;

        _h->limb[i] = product & D_TESTS_HUGE_MASK;
        carry       = product >> 16;
    }

    // the carry's limbs
    while (carry != 0UL)
    {
        // no room
        if (_h->length >= D_TESTS_HUGE_LIMBS)
        {
            g_huge_overrun = true;

            return;
        }

        _h->limb[_h->length] = carry & D_TESTS_HUGE_MASK;
        ++_h->length;
        carry = carry >> 16;
    }

    d_tests_huge_trim(_h);

    return;
}

// d_tests_huge_pow5
//   _h = _h * 5^_count: by 5^6 = 15625 while it can, then by 5.
static void
d_tests_huge_pow5(
    struct dj_tests_huge* _h,
    long                  _count
)
{
    long left = _count;

    // six at a time
    while (left >= 6)
    {
        d_tests_huge_mul_add(_h, 15625UL, 0UL);
        left = left - 6;
    }

    // the rest
    while (left > 0)
    {
        d_tests_huge_mul_add(_h, 5UL, 0UL);
        --left;
    }

    return;
}

// d_tests_huge_shl
//   _h = _h * 2^_bits, _bits 0 or more.
static void
d_tests_huge_shl(
    struct dj_tests_huge* _h,
    long                  _bits
)
{
    const long words = _bits / 16L;
    const int  rest  = (int)(_bits % 16L);

    unsigned long carry = 0UL;
    long          i     = 0;

    // 0, or nothing to do
    if ( (_h->length == 0) ||
         (_bits <= 0) )
    {
        return;
    }

    // no room
    if ((_h->length + words + 1) > D_TESTS_HUGE_LIMBS)
    {
        g_huge_overrun = true;
        _h->length     = 0;

        return;
    }

    // whole limbs up
    for (i = _h->length - 1; i >= 0; --i)
    {
        _h->limb[i + words] = _h->limb[i];
    }

    // zeros under them
    for (i = 0; i < words; ++i)
    {
        _h->limb[i] = 0UL;
    }

    _h->length = _h->length + words;

    // then the bits within a limb
    if (rest > 0)
    {
        for (i = words; i < _h->length; ++i)
        {
            const unsigned long shifted = (_h->limb[i] << rest) | carry;

            _h->limb[i] = shifted & D_TESTS_HUGE_MASK;
            carry       = shifted >> 16;
        }

        // the bits out of the top
        if (carry != 0UL)
        {
            _h->limb[_h->length] = carry;
            ++_h->length;
        }
    }

    return;
}

// d_tests_huge_sub_one
//   _h = _h - 1, _h positive.
static void
d_tests_huge_sub_one(
    struct dj_tests_huge* _h
)
{
    long i = 0;

    // the borrow through the zero limbs
    while ( (i < _h->length) &&
            (_h->limb[i] == 0UL) )
    {
        _h->limb[i] = D_TESTS_HUGE_MASK;
        ++i;
    }

    // the first nonzero limb
    if (i < _h->length)
    {
        _h->limb[i] = _h->limb[i] - 1UL;
    }

    d_tests_huge_trim(_h);

    return;
}

// d_tests_huge_mul
//   _out = _a * _b, _out neither of them.
static void
d_tests_huge_mul(
    struct dj_tests_huge*       _out,
    const struct dj_tests_huge* _a,
    const struct dj_tests_huge* _b
)
{
    long i = 0;
    long j = 0;

    // 0
    if ( (_a->length == 0) ||
         (_b->length == 0) )
    {
        _out->length = 0;

        return;
    }

    // no room
    if ((_a->length + _b->length) > D_TESTS_HUGE_LIMBS)
    {
        g_huge_overrun = true;
        _out->length   = 0;

        return;
    }

    // the product's limbs, 0 to start
    for (i = 0; i < (_a->length + _b->length); ++i)
    {
        _out->limb[i] = 0UL;
    }

    // each limb of _a, times all of _b
    for (i = 0; i < _a->length; ++i)
    {
        unsigned long carry = 0UL;

        for (j = 0; j < _b->length; ++j)
        {
            const unsigned long sum = _out->limb[i + j] +
                                      (_a->limb[i] * _b->limb[j]) + carry;

            _out->limb[i + j] = sum & D_TESTS_HUGE_MASK;
            carry             = sum >> 16;
        }

        _out->limb[i + _b->length] = carry;
    }

    _out->length = _a->length + _b->length;
    d_tests_huge_trim(_out);

    return;
}

// d_tests_huge_cmp
//   the sign of _a - _b: -1, 0 or 1.
static int
d_tests_huge_cmp(
    const struct dj_tests_huge* _a,
    const struct dj_tests_huge* _b
)
{
    long i = 0;

    // more limbs, more value
    if (_a->length != _b->length)
    {
        return (_a->length < _b->length) ? -1 : 1;
    }

    // the first limb that differs, from the top
    for (i = _a->length - 1; i >= 0; --i)
    {
        if (_a->limb[i] != _b->limb[i])
        {
            return (_a->limb[i] < _b->limb[i]) ? -1 : 1;
        }
    }

    return 0;
}

// d_tests_huge_div_10000
//   _h = _h / 10000; returns the remainder.
static unsigned long
d_tests_huge_div_10000(
    struct dj_tests_huge* _h
)
{
    unsigned long rest = 0UL;
    long          i    = 0;

    // long division, from the top
    for (i = _h->length - 1; i >= 0; --i)
    {
        const unsigned long part = (rest << 16) | _h->limb[i];

        _h->limb[i] = part / 10000UL;
        rest        = part % 10000UL;
    }

    d_tests_huge_trim(_h);

    return rest;
}

// d_tests_huge_digits
//   _h's decimal digits into _buffer, of _size characters, the most
// significant first, and a NUL; returns how many. _h is used up.
static long
d_tests_huge_digits(
    struct dj_tests_huge* _h,
    char*                 _buffer,
    long                  _size
)
{
    long count = 0;
    long i     = 0;

    // four digits at a time, the least significant first
    while (_h->length > 0)
    {
        unsigned long group = d_tests_huge_div_10000(_h);
        int           k     = 0;

        // no room
        if ((count + 5) > _size)
        {
            g_huge_overrun = true;

            break;
        }

        for (k = 0; k < 4; ++k)
        {
            _buffer[count] = (char)('0' + (int)(group % 10UL));
            group          = group / 10UL;
            ++count;
        }
    }

    // the zeros over the top digit
    while ( (count > 1) &&
            (_buffer[count - 1] == '0') )
    {
        --count;
    }

    // 0
    if (count == 0)
    {
        _buffer[0] = '0';
        count      = 1;
    }

    // the most significant first
    for (i = 0; i < (count / 2); ++i)
    {
        const char swap = _buffer[i];

        _buffer[i]             = _buffer[count - 1 - i];
        _buffer[count - 1 - i] = swap;
    }

    _buffer[count] = '\0';

    return count;
}


//==============================================================================
// 3.  THE TEXT'S VALUE, EXACTLY
//==============================================================================

// dj_tests_number
//   struct: a decimal number's magnitude as n * 10^e, n without trailing
// zeros, ready to compare with any k * 2^j: scaled is n * 5^e where e is 0
// or more, else n; fives is 5^-e where e is below 0, else 1.
struct dj_tests_number
{
    bool                 negative;  // a minus sign
    bool                 zero;      // no nonzero digit
    long                 e;         // the power of ten
    long                 bits;      // scaled's bits
    struct dj_tests_huge scaled;    // n * 5^max(e, 0)
    struct dj_tests_huge fives;     // 5^max(-e, 0)
};

static char g_digits[D_TESTS_LONG_SIZE];

// d_tests_number_of
//   _x from _text, a decimal number whose exponent, if any, is below 10^8 in
// size: its own reading, apart from decimal.h's.
static void
d_tests_number_of(
    struct dj_tests_number* _x,
    const char*             _text,
    size_t                  _length
)
{
    size_t i        = 0u;
    long   count    = 0;      // the digits from the first nonzero one
    long   fraction = 0;      // the digits after the point
    long   exponent = 0;
    long   at       = 0;
    bool   after    = false;
    bool   below    = false;

    _x->negative = false;

    // the sign
    if ( (i < _length) &&
         ( (_text[i] == '+') ||
           (_text[i] == '-') ) )
    {
        _x->negative = (_text[i] == '-');
        ++i;
    }

    // the digits and the point
    while ( (i < _length) &&
            ( ( (_text[i] >= '0') &&
                (_text[i] <= '9') ) ||
              (_text[i] == '.') ) )
    {
        // the point
        if (_text[i] == '.')
        {
            after = true;
        }
        else
        {
            // from the first nonzero digit on
            if ( (count > 0) ||
                 (_text[i] != '0') )
            {
                g_digits[count] = _text[i];
                ++count;
            }

            fraction = (after) ? (fraction + 1) : fraction;
        }

        ++i;
    }

    // the exponent
    if ( (i < _length) &&
         ( (_text[i] == 'e') ||
           (_text[i] == 'E') ) )
    {
        ++i;

        // its sign
        if ( (i < _length) &&
             ( (_text[i] == '+') ||
               (_text[i] == '-') ) )
        {
            below = (_text[i] == '-');
            ++i;
        }

        // its digits
        while ( (i < _length) &&
                (_text[i] >= '0') &&
                (_text[i] <= '9') )
        {
            exponent = (exponent * 10L) + (long)(_text[i] - '0');
            ++i;
        }

        exponent = (below) ? -exponent : exponent;
    }

    _x->e = exponent - fraction;

    // the zeros at the end, into the exponent
    while ( (count > 0) &&
            (g_digits[count - 1] == '0') )
    {
        --count;
        ++_x->e;
    }

    _x->zero = (count == 0);
    _x->e    = (_x->zero) ? 0 : _x->e;

    // n, four digits at a time
    d_tests_huge_set(&_x->scaled, 0UL);

    while (at < count)
    {
        unsigned long group = 0UL;
        unsigned long power = 1UL;
        const long    take  = ((count - at) >= 4) ? 4 : (count - at);
        long          k     = 0;

        for (k = 0; k < take; ++k)
        {
            group = (group * 10UL) + (unsigned long)(g_digits[at + k] - '0');
            power = power * 10UL;
        }

        d_tests_huge_mul_add(&_x->scaled, power, group);
        at = at + take;
    }

    // the power of ten's five part, on the side it is an integer on
    d_tests_huge_set(&_x->fives, 1UL);

    if (_x->e >= 0)
    {
        d_tests_huge_pow5(&_x->scaled, _x->e);
    }
    else
    {
        d_tests_huge_pow5(&_x->fives, -_x->e);
    }

    _x->bits = d_tests_huge_bits(&_x->scaled);

    return;
}

// d_tests_cmp_dyadic
//   the sign of |_x| - _k * 2^_j: -1, 0 or 1. Both sides are multiplied by
// 5^-e where e is negative, then by the power of two that leaves both
// integers; most comparisons end at the bits.
static int
d_tests_cmp_dyadic(
    const struct dj_tests_number* _x,
    const struct dj_tests_huge*   _k,
    long                          _j
)
{
    static struct dj_tests_huge left;
    static struct dj_tests_huge right;

    long least      = 0;
    long left_bits  = 0;
    long right_bits = 0;

    // a 0 on either side
    if ( (_x->zero) ||
         (_k->length == 0) )
    {
        return ( (_x->zero) &&
                 (_k->length == 0) ) ? 0 : ( (_x->zero) ? -1 : 1 );
    }

    least      = (_x->e < _j) ? _x->e : _j;
    left_bits  = _x->bits + (_x->e - least);
    right_bits = d_tests_huge_bits(_k) + d_tests_huge_bits(&_x->fives) +
                 (_j - least);

    // apart by more than a bit: the product of k and the fives has its two
    // factors' bits, or one less
    if (left_bits > right_bits)
    {
        return 1;
    }

    if (left_bits < (right_bits - 1))
    {
        return -1;
    }

    d_tests_huge_copy(&left, &_x->scaled);
    d_tests_huge_mul(&right, _k, &_x->fives);
    d_tests_huge_shl(&left, _x->e - least);
    d_tests_huge_shl(&right, _j - least);

    return d_tests_huge_cmp(&left, &right);
}


//==============================================================================
// 4.  WHAT A RESULT MUST BE
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

// dj_tests_result
//   struct: a result's magnitude as m * 2^u, m below 2^P and u at least
// emin - P, so that m and u are those of the type's own grid, subnormals'
// too.
struct dj_tests_result
{
    bool                 nan;       // no number
    bool                 infinite;  // an infinity
    bool                 negative;  // a minus sign, a zero's too
    struct dj_tests_huge m;         // the mantissa
    long                 u;         // the weight of its lowest bit
};

// d_tests_result_of
//   _r from _x, a value of a type with _precision bits and exponents from
// _min_exp, as a long double: frexp's exponent, held at _min_exp, gives u;
// _x * 2^-u is then m, split into limbs by exact divisions.
static void
d_tests_result_of(
    struct dj_tests_result* _r,
    long double             _x,
    int                     _precision,
    long                    _min_exp
)
{
    long double magnitude = 0.0L;
    long double rest      = 0.0L;
    int         e         = 0;

    _r->nan      = (_x != _x);
    _r->infinite = false;
    _r->negative = ( (_x < 0.0L) ||
                     ( (_x == 0.0L) &&
                       (!d_tests_is_plus_zero(_x)) ) );
    _r->m.length = 0;
    _r->u        = _min_exp - (long)_precision;

    // no number
    if (_r->nan)
    {
        return;
    }

    magnitude = (_r->negative) ? -_x : _x;

    // an infinity
    if (magnitude > LDBL_MAX)
    {
        _r->infinite = true;

        return;
    }

    // 0
    if (magnitude == 0.0L)
    {
        return;
    }

    (void)frexpl(magnitude, &e);
    _r->u = ( ((long)e > _min_exp) ? (long)e : _min_exp ) -
            (long)_precision;
    rest  = ldexpl(magnitude, (int)-_r->u);

    // sixteen bits at a time, the low ones first
    while (rest != 0.0L)
    {
        const long double above = floorl(rest / 65536.0L);
        const long double limb  = rest - (above * 65536.0L);

        _r->m.limb[_r->m.length] = (unsigned long)limb;
        ++_r->m.length;
        rest = above;
    }

    return;
}

// d_tests_type_name ... d_tests_max_exp
//   a type's name and parameters.
static const char*
d_tests_type_name(
    int _type
)
{
    return (_type == D_TESTS_FLOAT)
               ? "float"
               : ( (_type == D_TESTS_DOUBLE) ? "double" : "long double" );
}

static int
d_tests_precision(
    int _type
)
{
    return (_type == D_TESTS_FLOAT)
               ? FLT_MANT_DIG
               : ( (_type == D_TESTS_DOUBLE) ? DBL_MANT_DIG : LDBL_MANT_DIG );
}

static long
d_tests_min_exp(
    int _type
)
{
    return (_type == D_TESTS_FLOAT)
               ? (long)FLT_MIN_EXP
               : ( (_type == D_TESTS_DOUBLE) ? (long)DBL_MIN_EXP
                                             : (long)LDBL_MIN_EXP );
}

static long
d_tests_max_exp(
    int _type
)
{
    return (_type == D_TESTS_FLOAT)
               ? (long)FLT_MAX_EXP
               : ( (_type == D_TESTS_DOUBLE) ? (long)DBL_MAX_EXP
                                             : (long)LDBL_MAX_EXP );
}

// d_tests_check
//   checks _r, the result of converting _text, whose value is _x, to _type,
// returned with _status: the value nearest, ties to even; the sign; the
// status. A result between the midpoints below and above it is nearest; on
// either midpoint, its mantissa must be even. An infinity must be at least
// the largest finite value's upper midpoint, where that value's odd
// mantissa sends a tie up.
static void
d_tests_check(
    const struct dj_tests_number* _x,
    const char*                   _text,
    size_t                        _length,
    int                           _type,
    long double                   _r,
    enum dj_math_decimal_status   _status,
    int                           _line
)
{
    static struct dj_tests_result r;
    static struct dj_tests_huge   k;
    static struct dj_tests_huge   half;

    const int  precision = d_tests_precision(_type);
    const long min_exp   = d_tests_min_exp(_type);
    const long max_exp   = d_tests_max_exp(_type);

    char what[96];
    bool nearest = false;
    bool sign    = false;
    bool status  = false;

    d_tests_result_of(&r, _r, precision, min_exp);
    sign = ( (!r.nan) &&
             (r.negative == _x->negative) );

    // an infinity: from the largest finite value's upper midpoint on
    if (r.infinite)
    {
        d_tests_huge_set(&k, 1UL);
        d_tests_huge_shl(&k, (long)precision + 1L);
        d_tests_huge_sub_one(&k);
        nearest = (d_tests_cmp_dyadic(_x, &k,
                                      max_exp - (long)precision - 1L) >= 0);
        status  = (_status == D_MATH_DECIMAL_OVERFLOW);
    }
    // a number: between its midpoints
    else if (!r.nan)
    {
        const bool even = ( (r.m.length == 0) ||
                            ((r.m.limb[0] & 1UL) == 0UL) );

        int  above = 0;
        int  under = 1;
        bool exact = false;
        bool tiny  = false;

        d_tests_huge_set(&half, 1UL);
        d_tests_huge_shl(&half, (long)precision - 1L);

        // the midpoint above: (2m + 1) 2^(u - 1)
        d_tests_huge_copy(&k, &r.m);
        d_tests_huge_shl(&k, 1L);
        d_tests_huge_mul_add(&k, 1UL, 1UL);
        above = d_tests_cmp_dyadic(_x, &k, r.u - 1L);

        // the midpoint below: (4m - 1) 2^(u - 2) at a binade's foot, the
        // neighbour below half as far; (2m - 1) 2^(u - 1) otherwise
        if ( (r.m.length > 0) &&
             (d_tests_huge_cmp(&r.m, &half) == 0) &&
             (r.u > (min_exp - (long)precision)) )
        {
            d_tests_huge_copy(&k, &r.m);
            d_tests_huge_shl(&k, 2L);
            d_tests_huge_sub_one(&k);
            under = d_tests_cmp_dyadic(_x, &k, r.u - 2L);
        }
        else if (r.m.length > 0)
        {
            d_tests_huge_copy(&k, &r.m);
            d_tests_huge_shl(&k, 1L);
            d_tests_huge_sub_one(&k);
            under = d_tests_cmp_dyadic(_x, &k, r.u - 1L);
        }

        nearest = ( ( (above < 0) ||
                      ( (above == 0) &&
                        (even) ) ) &&
                    ( (under > 0) ||
                      ( (under == 0) &&
                        (even) ) ) );
        exact   = (d_tests_cmp_dyadic(_x, &r.m, r.u) == 0);
        tiny    = (d_tests_huge_cmp(&r.m, &half) < 0);
        status  = ( (tiny) &&
                    (!exact) ) ? (_status == D_MATH_DECIMAL_UNDERFLOW)
                               : (_status == D_MATH_DECIMAL_OK);
    }

    sprintf(what, "%s: the nearest value, ties to even",
            d_tests_type_name(_type));
    d_tests_expect(nearest, what, _text, _length, _line);
    sprintf(what, "%s: the status", d_tests_type_name(_type));
    d_tests_expect(status, what, _text, _length, _line);
    sprintf(what, "%s: the text's sign", d_tests_type_name(_type));
    d_tests_expect(sign, what, _text, _length, _line);

    return;
}


//==============================================================================
// 5.  CONVERSIONS, CHECKED
//==============================================================================

// d_tests_convert
//   _text to _type, the result as a long double, which holds it exactly.
static enum dj_math_decimal_status
d_tests_convert(
    int          _type,
    const char*  _text,
    size_t       _length,
    long double* _out
)
{
    enum dj_math_decimal_status status = D_MATH_DECIMAL_SYNTAX;

    // by type
    if (_type == D_TESTS_FLOAT)
    {
        float value = 0.0f;

        status = d_math_decimal_to_f(&value, _text, _length);
        *_out  = (long double)value;
    }
    else if (_type == D_TESTS_DOUBLE)
    {
        double value = 0.0;

        status = d_math_decimal_to_d(&value, _text, _length);
        *_out  = (long double)value;
    }
    else
    {
        long double value = 0.0L;

        status = d_math_decimal_to_ld(&value, _text, _length);
        *_out  = value;
    }

    return status;
}

// d_tests_one
//   _text, a decimal number, converted to _type and checked; the result and
// status into *_value and *_status.
static void
d_tests_one(
    int                          _type,
    const char*                  _text,
    size_t                       _length,
    long double*                 _value,
    enum dj_math_decimal_status* _status,
    int                          _line
)
{
    static struct dj_tests_number x;

    d_tests_number_of(&x, _text, _length);
    *_status = d_tests_convert(_type, _text, _length, _value);
    d_tests_check(&x, _text, _length, _type, *_value, *_status, _line);

    return;
}

// d_tests_as
//   _text converted to _type must give _value, the same sign included, and
// _status: those of a text known to round the same way, checked against
// the reference already.
static void
d_tests_as(
    int                         _type,
    const char*                 _text,
    size_t                      _length,
    long double                 _value,
    enum dj_math_decimal_status _status,
    int                         _line
)
{
    long double                     value  = 0.0L;
    const enum dj_math_decimal_status status =
        d_tests_convert(_type, _text, _length, &value);
    char                            what[96];

    sprintf(what, "%s: as the same value within the buffer",
            d_tests_type_name(_type));
    d_tests_expect( ( (value == _value) ||
                      ( (value != value) &&
                        (_value != _value) ) ) &&
                    (d_tests_is_plus_zero(value) ==
                     d_tests_is_plus_zero(_value)) &&
                    (status == _status),
                    what, _text, _length, _line );

    return;
}

// d_tests_all
//   _text, a decimal number, converted to each type and checked; and its
// length measured, alone and with a character after it that ends it.
static void
d_tests_all(
    const char* _text,
    size_t      _length,
    int         _line
)
{
    static struct dj_tests_number x;
    static char                   longer[D_TESTS_TEXT_SIZE];

    long double                 value  = 0.0L;
    enum dj_math_decimal_status status = D_MATH_DECIMAL_OK;
    int                         type   = 0;

    d_tests_number_of(&x, _text, _length);

    // each type
    for (type = D_TESTS_FLOAT; type <= D_TESTS_LONG_DOUBLE; ++type)
    {
        status = d_tests_convert(type, _text, _length, &value);
        d_tests_check(&x, _text, _length, type, value, status, _line);
    }

    d_tests_expect( (d_math_decimal_length(_text, _length) == _length),
                    "the whole text, measured", _text, _length, _line );

    // with a character after it
    if (_length < (size_t)(D_TESTS_TEXT_SIZE - 1L))
    {
        memcpy(longer, _text, _length);
        longer[_length] = ';';
        d_tests_expect( (d_math_decimal_length(longer, _length + 1u) ==
                         _length) &&
                        (d_tests_convert(D_TESTS_DOUBLE, longer, _length + 1u,
                                         &value) == D_MATH_DECIMAL_SYNTAX),
                        "a character after it: measured apart, no number",
                        _text, _length, _line );
    }

    return;
}


//==============================================================================
// 6.  THE TEXTS
//==============================================================================


// 6.1    The syntax
//------------------------------------------------------------------------------

// dj_tests_syntax_case
//   struct: a text and the length of the decimal number it starts with.
struct dj_tests_syntax_case
{
    const char* text;
    size_t      length;
};

static const struct dj_tests_syntax_case g_syntax[] =
{
    { "",          0u }, { "+",         0u }, { "-",         0u },
    { ".",         0u }, { "+.",        0u }, { "-.e1",      0u },
    { "e5",        0u }, { "E5",        0u }, { ".e5",       0u },
    { "..5",       0u }, { "--1",       0u }, { "+-1",       0u },
    { " 1",        0u }, { "inf",       0u }, { "nan",       0u },
    { "x1",        0u }, { "1",         1u }, { "12",        2u },
    { "1.5x",      3u }, { "1e",        1u }, { "1E",        1u },
    { "1e+",       1u }, { "1e-",       1u }, { "1e-x",      1u },
    { "1e5",       3u }, { "1E5",       3u }, { "1.e5",      4u },
    { ".5e-3",     5u }, { "1..2",      2u }, { "1.2.3",     3u },
    { "12e3e4",    4u }, { "1 ",        1u }, { "0x1p3",     1u },
    { "1e+5.",     4u }, { "+.5",       3u }, { "-5.",       3u },
    { "5.e",       2u }, { "00",        2u }, { "0.0e0",     5u },
    { "1e0001",    6u }, { "1,5",       1u }, { "1_000",     1u },
    { ".5.",       2u }, { "1.5E+10F",  7u }, { "-0",        2u },
    { "+0.",       3u }, { "1e+-5",     1u }, { "1ee5",      1u },
    { "-.5E-0",    6u }, { "9.",        2u }, { "0.",        2u }
};

// d_tests_syntax
//   each case: its length measured; the conversions, which must agree with
// the reference where the whole text is the number, and must store nothing
// otherwise; and prefixes of a text, the length limiting what is read.
static void
d_tests_syntax(void)
{
    const size_t cases = sizeof(g_syntax) / sizeof(g_syntax[0]);

    size_t i = 0u;

    // each case
    for (i = 0u; i < cases; ++i)
    {
        const char*  text   = g_syntax[i].text;
        const size_t length = strlen(text);

        D_TESTS_EXPECT( (d_math_decimal_length(text, length) ==
                         g_syntax[i].length),
                        "the decimal number's length", text, length );

        // the whole text a number, or not
        if ( (g_syntax[i].length == length) &&
             (length > 0u) )
        {
            d_tests_all(text, length, __LINE__);
        }
        else
        {
            float       f  = 1.5f;
            double      d  = 2.5;
            long double ld = 3.5L;

            D_TESTS_EXPECT( (d_math_decimal_to_f(&f, text, length) ==
                             D_MATH_DECIMAL_SYNTAX) &&
                            (d_math_decimal_to_d(&d, text, length) ==
                             D_MATH_DECIMAL_SYNTAX) &&
                            (d_math_decimal_to_ld(&ld, text, length) ==
                             D_MATH_DECIMAL_SYNTAX),
                            "no number: D_MATH_DECIMAL_SYNTAX", text,
                            length );
            D_TESTS_EXPECT( (f == 1.5f) &&
                            (d == 2.5) &&
                            (ld == 3.5L),
                            "no number: nothing stored", text, length );
        }
    }

    // the length given, not the NUL, ends the text
    D_TESTS_EXPECT( (d_math_decimal_length("123", 2u) == 2u) &&
                    (d_math_decimal_length("1e5", 2u) == 1u) &&
                    (d_math_decimal_length("1.5", 2u) == 2u) &&
                    (d_math_decimal_length("-1", 1u) == 0u) &&
                    (d_math_decimal_length("7", 0u) == 0u),
                    "a length short of the NUL", "123", 3u );
    d_tests_all("123", 2u, __LINE__);
    d_tests_all("1.5", 2u, __LINE__);
    d_tests_all("2.5e+12", 6u, __LINE__);

    // and a text with no NUL at all
    {
        const char block[4] = { '4', '.', '2', '5' };
        double     d        = 0.0;

        D_TESTS_EXPECT( (d_math_decimal_to_d(&d, block, 4u) ==
                         D_MATH_DECIMAL_OK) &&
                        (d == 4.25),
                        "a text with no NUL", "4.25", 4u );
    }

    return;
}

// 6.2    Known values
//------------------------------------------------------------------------------
// The compilers' literals are correctly rounded: GCC's through MPFR,
// clang's through APFloat. Each is first stored through a volatile object,
// which leaves the x87 no extra precision to keep.

// d_tests_known_f ... d_tests_known_ld
//   _text as _type must be _want, the same sign included, with _status; then
// the reference checks it in every type, unless _alone (an exponent too big
// for the reference to read).
static bool
d_tests_same(
    long double _a,
    long double _b
)
{
    return ( (_a == _b) &&
             (d_tests_is_plus_zero(_a) == d_tests_is_plus_zero(_b)) );
}

static void
d_tests_known_f(
    const char*                 _text,
    float                       _want,
    enum dj_math_decimal_status _status,
    bool                        _alone,
    int                         _line
)
{
    volatile float want   = _want;
    float          got    = 0.0f;
    const size_t   length = strlen(_text);

    const enum dj_math_decimal_status status =
        d_math_decimal_to_f(&got, _text, length);

    d_tests_expect( (d_tests_same((long double)got, (long double)want)) &&
                    (status == _status),
                    "float: the known value and status", _text, length,
                    _line );

    // the reference too
    if (!_alone)
    {
        d_tests_all(_text, length, _line);
    }

    return;
}

static void
d_tests_known_d(
    const char*                 _text,
    double                      _want,
    enum dj_math_decimal_status _status,
    bool                        _alone,
    int                         _line
)
{
    volatile double want   = _want;
    double          got    = 0.0;
    const size_t    length = strlen(_text);

    const enum dj_math_decimal_status status =
        d_math_decimal_to_d(&got, _text, length);

    d_tests_expect( (d_tests_same((long double)got, (long double)want)) &&
                    (status == _status),
                    "double: the known value and status", _text, length,
                    _line );

    // the reference too
    if (!_alone)
    {
        d_tests_all(_text, length, _line);
    }

    return;
}

static void
d_tests_known_ld(
    const char*                 _text,
    long double                 _want,
    enum dj_math_decimal_status _status,
    bool                        _alone,
    int                         _line
)
{
    volatile long double want   = _want;
    long double          got    = 0.0L;
    const size_t         length = strlen(_text);

    const enum dj_math_decimal_status status =
        d_math_decimal_to_ld(&got, _text, length);

    d_tests_expect( (d_tests_same(got, want)) &&
                    (status == _status),
                    "long double: the known value and status", _text,
                    length, _line );

    // the reference too
    if (!_alone)
    {
        d_tests_all(_text, length, _line);
    }

    return;
}

#define D_TESTS_OK         D_MATH_DECIMAL_OK
#define D_TESTS_OVERFLOW   D_MATH_DECIMAL_OVERFLOW
#define D_TESTS_UNDERFLOW  D_MATH_DECIMAL_UNDERFLOW

// d_tests_known
//   ties, the ends of each range, famous hard cases, signs and zeros.
static void
d_tests_known(void)
{
    const float       f_inf  = (float)HUGE_VAL;
    const double      d_inf  = (double)HUGE_VAL;
    const long double ld_inf = (long double)HUGE_VAL;

    // float: ties, the ends, and a case double rounding gets wrong
    d_tests_known_f("0.1", 0.1f, D_TESTS_OK, false, __LINE__);
    d_tests_known_f("16777217", 16777216.0f, D_TESTS_OK, false, __LINE__);
    d_tests_known_f("16777219", 16777220.0f, D_TESTS_OK, false, __LINE__);
    d_tests_known_f("16777217.000000001", 16777218.0f, D_TESTS_OK, false,
                    __LINE__);
    d_tests_known_f("7.038531e-26", 7.038531e-26f, D_TESTS_OK, false,
                    __LINE__);
    d_tests_known_f("1.17549435e-38", FLT_MIN, D_TESTS_OK, false, __LINE__);
    d_tests_known_f("1.4e-45", 1.4e-45f, D_TESTS_UNDERFLOW, false,
                    __LINE__);
    d_tests_known_f("7.0e-46", 0.0f, D_TESTS_UNDERFLOW, false, __LINE__);
    d_tests_known_f("7.1e-46", 1.4e-45f, D_TESTS_UNDERFLOW, false,
                    __LINE__);
    d_tests_known_f("3.4028235e38", FLT_MAX, D_TESTS_OK, false, __LINE__);
    d_tests_known_f("3.4028236e38", f_inf, D_TESTS_OVERFLOW, false,
                    __LINE__);
    d_tests_known_f("-3.4028236e38", -f_inf, D_TESTS_OVERFLOW, false,
                    __LINE__);

    // double: ties, the ends, and famous hard cases
    d_tests_known_d("0.1", 0.1, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("9007199254740993", 9007199254740992.0, D_TESTS_OK,
                    false, __LINE__);
    d_tests_known_d("9007199254740995", 9007199254740996.0, D_TESTS_OK,
                    false, __LINE__);
    d_tests_known_d("9007199254740993.0000000000000000000000001",
                    9007199254740994.0, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("1e23", 1e23, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("8.98846567431158e307", 8.98846567431158e307,
                    D_TESTS_OK, false, __LINE__);
    d_tests_known_d("2.2250738585072011e-308", 2.2250738585072011e-308,
                    D_TESTS_UNDERFLOW, false, __LINE__);
    d_tests_known_d("2.2250738585072012e-308", DBL_MIN, D_TESTS_OK, false,
                    __LINE__);
    d_tests_known_d("4.9406564584124654e-324", 4.9406564584124654e-324,
                    D_TESTS_UNDERFLOW, false, __LINE__);
    d_tests_known_d("5e-324", 4.9406564584124654e-324, D_TESTS_UNDERFLOW,
                    false, __LINE__);
    d_tests_known_d("2.4703282292062327e-324", 0.0, D_TESTS_UNDERFLOW,
                    false, __LINE__);
    d_tests_known_d("2.4703282292062328e-324", 4.9406564584124654e-324,
                    D_TESTS_UNDERFLOW, false, __LINE__);
    d_tests_known_d("1.7976931348623157e308", DBL_MAX, D_TESTS_OK, false,
                    __LINE__);
    d_tests_known_d("1.7976931348623158e308", DBL_MAX, D_TESTS_OK, false,
                    __LINE__);
    d_tests_known_d("1.7976931348623159e308", d_inf, D_TESTS_OVERFLOW, false,
                    __LINE__);
    d_tests_known_d("1e400", d_inf, D_TESTS_OVERFLOW, false, __LINE__);
    d_tests_known_d("1e-400", 0.0, D_TESTS_UNDERFLOW, false, __LINE__);
    d_tests_known_d("-1e-400", -0.0, D_TESTS_UNDERFLOW, false, __LINE__);

    // long double: the type's ends come from the values drawn below
    d_tests_known_ld("0.1", 0.1L, D_TESTS_OK, false, __LINE__);
    d_tests_known_ld("1e4933", ld_inf, D_TESTS_OVERFLOW, false, __LINE__);
    d_tests_known_ld("1e-5000", 0.0L, D_TESTS_UNDERFLOW, false, __LINE__);
    d_tests_known_ld("-2.5", -2.5L, D_TESTS_OK, false, __LINE__);

    // zeros, each with its text's sign, and exact
    d_tests_known_d("0", 0.0, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("-0", -0.0, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("+0.", 0.0, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("-.0e-5", -0.0, D_TESTS_OK, false, __LINE__);
    d_tests_known_f("-0.000", -0.0f, D_TESTS_OK, false, __LINE__);
    d_tests_known_ld("-0e+12", -0.0L, D_TESTS_OK, false, __LINE__);

    // the forms of one number
    d_tests_known_d(".5", 0.5, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("5.", 5.0, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("+5.0E-1", 0.5, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("0005.000", 5.0, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("50e-1", 5.0, D_TESTS_OK, false, __LINE__);
    d_tests_known_d("1e00000000000000000000000000005", 1e5, D_TESTS_OK,
                    false, __LINE__);

    // exponents too big to hold: held at the limit, which every type is
    // past; so 0 times any of them is 0, exactly
    d_tests_known_d("1e99999999999999999999999999999", d_inf,
                    D_TESTS_OVERFLOW, true, __LINE__);
    d_tests_known_d("1e-99999999999999999999999999999", 0.0,
                    D_TESTS_UNDERFLOW, true, __LINE__);
    d_tests_known_ld("-1e-99999999999999999999999999999", -0.0L,
                     D_TESTS_UNDERFLOW, true, __LINE__);
    d_tests_known_f("0e99999999999999999999999999999", 0.0f, D_TESTS_OK,
                    true, __LINE__);
    d_tests_known_ld("123.45e+18446744073709551616", ld_inf,
                     D_TESTS_OVERFLOW, true, __LINE__);

    return;
}

// 6.3    Values and their midpoints
//------------------------------------------------------------------------------
// For a type of P bits: a value m * 2^u drawn at random -- subnormal; at the
// foot or the top of the range; m all ones, which carries out of P bits when
// it rounds up; m a power of two, a binade's foot; or anything -- written out
// exactly, then the midpoint above it, (2m + 1) * 2^(u - 1), exactly, which
// must round to whichever of its neighbours is even, and nudged up and down
// by a digit appended, both inside the buffer and past it.

static unsigned long g_state = 2718281828UL;

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

// d_tests_random_bits
//   _h = a random number of exactly _bits bits, _bits from 1.
static void
d_tests_random_bits(
    struct dj_tests_huge* _h,
    long                  _bits
)
{
    const long limbs = (_bits + 15L) / 16L;
    const long top   = _bits - (16L * (limbs - 1L));

    long i = 0;

    // each limb
    for (i = 0; i < limbs; ++i)
    {
        _h->limb[i] = d_tests_random32() & D_TESTS_HUGE_MASK;
    }

    _h->limb[limbs - 1] = (_h->limb[limbs - 1] & ((1UL << top) - 1UL)) |
                          (1UL << (top - 1L));
    _h->length          = limbs;

    return;
}

// d_tests_dyadic_digits
//   the decimal digits of _k * 2^_j into _digits, *_exponent set so that
// the value is the digits times 10^*_exponent; returns how many.
static long
d_tests_dyadic_digits(
    char*                       _digits,
    const struct dj_tests_huge* _k,
    long                        _j,
    long*                       _exponent
)
{
    static struct dj_tests_huge n;

    d_tests_huge_copy(&n, _k);

    // a whole number, or k 5^-j over 10^-j
    if (_j >= 0)
    {
        d_tests_huge_shl(&n, _j);
        *_exponent = 0;
    }
    else
    {
        d_tests_huge_pow5(&n, -_j);
        *_exponent = _j;
    }

    return d_tests_huge_digits(&n, _digits, D_TESTS_TEXT_SIZE / 2L);
}

// d_tests_compose
//   writes into _text _sign, where not NUL; _digits; then _tail repeated
// _times, then _last where not NUL; then an exponent, so that the text's
// value is the digits written times 10^_exponent. Every third text puts a
// point after the first digit. Returns the text's length.
static size_t
d_tests_compose(
    char*       _text,
    char        _sign,
    const char* _digits,
    long        _count,
    char        _tail,
    long        _times,
    char        _last,
    long        _exponent
)
{
    const long written = _count + _times + ((_last != '\0') ? 1L : 0L);
    const bool point   = (d_tests_below(3) == 0);

    long n = 0;
    long i = 0;

    // the sign
    if (_sign != '\0')
    {
        _text[n] = _sign;
        ++n;
    }

    // the digits, with a point after the first, maybe
    for (i = 0; i < _count; ++i)
    {
        _text[n] = _digits[i];
        ++n;

        if ( (point) &&
             (i == 0) )
        {
            _text[n] = '.';
            ++n;
        }
    }

    // the tail
    for (i = 0; i < _times; ++i)
    {
        _text[n] = _tail;
        ++n;
    }

    if (_last != '\0')
    {
        _text[n] = _last;
        ++n;
    }

    n = n + (long)sprintf(_text + n, "e%ld",
                          (point) ? (_exponent - (_times +
                                                  ((_last != '\0') ? 1L
                                                                   : 0L)) +
                                     (written - 1L))
                                  : (_exponent - (_times +
                                                  ((_last != '\0') ? 1L
                                                                   : 0L))));

    return (size_t)n;
}

// d_tests_decrement
//   _digits minus 1 in its last place, into _out: the same count, a leading
// zero kept.
static void
d_tests_decrement(
    char*       _out,
    const char* _digits,
    long        _count
)
{
    long i = _count - 1;

    memcpy(_out, _digits, (size_t)_count);
    _out[_count] = '\0';

    // the borrow through the zeros
    while ( (i >= 0) &&
            (_out[i] == '0') )
    {
        _out[i] = '9';
        --i;
    }

    // the digit that pays it
    if (i >= 0)
    {
        _out[i] = (char)(_out[i] - 1);
    }

    return;
}

// d_tests_points
//   _count values of _type and their midpoints, as texts, each converted and
// checked; _wide is the share of them whose binade is drawn from the whole
// range, the rest within 2^200 of 1.
static void
d_tests_points(
    int  _type,
    long _count,
    long _wide
)
{
    static struct dj_tests_huge m;
    static struct dj_tests_huge k;
    static char                 digits[D_TESTS_TEXT_SIZE];
    static char                 lower[D_TESTS_TEXT_SIZE];
    static char                 text[D_TESTS_TEXT_SIZE];

    const long precision = (long)d_tests_precision(_type);
    const long min_exp   = d_tests_min_exp(_type);
    const long max_exp   = d_tests_max_exp(_type);
    const long room      = (long)D_INTERNAL_MATH_DECIMAL_DIGITS;

    long i = 0;

    // each value
    for (i = 0; i < _count; ++i)
    {
        const long                  kind        = i % 8L;
        const long                  draw        = d_tests_below(8);
        const char                  sign        = (draw == 0) ? '-'
                                                : ( (draw == 1) ? '+'
                                                                : '\0' );
        long                        e           = 0;  // the binade, as
                                                      // frexp's
        long                        count       = 0;
        long                        exponent    = 0;
        long                        far         = 0;
        size_t                      length      = 0u;
        long double                 value       = 0.0L;
        long double                 tie         = 0.0L;
        long double                 up          = 0.0L;
        long double                 down        = 0.0L;
        enum dj_math_decimal_status status      = D_MATH_DECIMAL_OK;
        enum dj_math_decimal_status tie_status  = D_MATH_DECIMAL_OK;
        enum dj_math_decimal_status up_status   = D_MATH_DECIMAL_OK;
        enum dj_math_decimal_status down_status = D_MATH_DECIMAL_OK;

        // the mantissa and the binade
        if ( (kind == 0) &&
             ((i % 16L) == 8L) )
        {
            // the largest subnormal: its midpoint above is a tie with the
            // least normal value, which is even
            d_tests_huge_set(&m, 1UL);
            d_tests_huge_shl(&m, precision - 1L);
            d_tests_huge_sub_one(&m);
            e = min_exp;
        }
        else if (kind == 0)
        {
            // subnormal: below 2^(P - 1), at the least exponent
            d_tests_random_bits(&m, 1L + d_tests_below(precision - 1L));
            e = min_exp;
        }
        else if ( (kind == 1) ||
                  (kind == 2) )
        {
            // the foot of the range, or the top
            d_tests_random_bits(&m, precision);
            e = (kind == 1) ? (min_exp + d_tests_below(3))
                            : (max_exp - d_tests_below(3));
        }
        else
        {
            // all ones, which carry; a power of two; or anything
            if (kind == 3)
            {
                d_tests_huge_set(&m, 1UL);
                d_tests_huge_shl(&m, precision);
                d_tests_huge_sub_one(&m);
            }
            else if (kind == 4)
            {
                d_tests_huge_set(&m, 1UL);
                d_tests_huge_shl(&m, precision - 1L);
            }
            else
            {
                d_tests_random_bits(&m, precision);
            }

            // the binade: the top, where all ones' midpoint above is the
            // largest finite value's, a tie that goes to infinity; from the
            // whole range; or near 1
            if ( (kind == 3) &&
                 ((i % 16L) == 11L) )
            {
                e = max_exp;
            }
            else if (d_tests_below(100) < _wide)
            {
                e = min_exp + d_tests_below(max_exp - min_exp + 1L);
            }
            else
            {
                e = d_tests_below(401) - 200L;
                e = (e < min_exp) ? min_exp : ((e > max_exp) ? max_exp : e);
            }
        }

        // the value itself, exactly; then a little more, which rounds to it
        // and is not exact, whether the 1 is within the buffer, its last
        // digit, which a shift pushes out, or past it
        count  = d_tests_dyadic_digits(digits, &m, e - precision, &exponent);
        far    = ((room - count) > 3L) ? (room - count + 3L) : 3L;
        length = d_tests_compose(text, sign, digits, count, '0', 0, '\0',
                                 exponent);
        d_tests_one(_type, text, length, &value, &status, __LINE__);
        length = d_tests_compose(text, sign, digits, count, '0', 0, '1',
                                 exponent);
        d_tests_one(_type, text, length, &value, &status, __LINE__);
        length = d_tests_compose(text, sign, digits, count, '0', far, '1',
                                 exponent);
        d_tests_as(_type, text, length, value, status, __LINE__);

        // the buffer's last digit
        if ((room - count) > 1L)
        {
            length = d_tests_compose(text, sign, digits, count, '0',
                                     room - count - 1L, '1', exponent);
            d_tests_as(_type, text, length, value, status, __LINE__);
        }

        // the midpoint above, exactly: a tie
        d_tests_huge_copy(&k, &m);
        d_tests_huge_shl(&k, 1L);
        d_tests_huge_mul_add(&k, 1UL, 1UL);
        count  = d_tests_dyadic_digits(digits, &k, e - precision - 1L,
                                       &exponent);
        length = d_tests_compose(text, sign, digits, count, '0', 0, '\0',
                                 exponent);
        d_tests_one(_type, text, length, &tie, &tie_status, __LINE__);

        // nudged up and down by a digit appended
        length = d_tests_compose(text, sign, digits, count, '0', 0, '1',
                                 exponent);
        d_tests_one(_type, text, length, &up, &up_status, __LINE__);
        d_tests_decrement(lower, digits, count);
        length = d_tests_compose(text, sign, lower, count, '9', 1, '\0',
                                 exponent);
        d_tests_one(_type, text, length, &down, &down_status, __LINE__);

        // past the buffer: zeros dropped leave a tie, a 1 dropped breaks it,
        // and nines dropped leave it broken; each as within the buffer
        far    = ((room - count) > 3L) ? (room - count + 3L) : 3L;
        length = d_tests_compose(text, sign, digits, count, '0', far + 1L,
                                 '\0', exponent);
        d_tests_as(_type, text, length, tie, tie_status, __LINE__);
        length = d_tests_compose(text, sign, digits, count, '0', far, '1',
                                 exponent);
        d_tests_as(_type, text, length, up, up_status, __LINE__);
        length = d_tests_compose(text, sign, lower, count, '9', far + 1L,
                                 '\0', exponent);
        d_tests_as(_type, text, length, down, down_status, __LINE__);

        // a 1 as the buffer's last digit, or the one before: kept as read,
        // and pushed out by a shift, which must say so
        if ((room - count) > 2L)
        {
            length = d_tests_compose(text, sign, digits, count, '0',
                                     room - count - 1L, '1', exponent);
            d_tests_as(_type, text, length, up, up_status, __LINE__);
            length = d_tests_compose(text, sign, digits, count, '0',
                                     room - count - 2L, '1', exponent);
            d_tests_as(_type, text, length, up, up_status, __LINE__);
        }
    }

    return;
}

// 6.4    Random texts
//------------------------------------------------------------------------------

// d_tests_random_text
//   a random decimal number into _text: _digits digits, in a pattern -- a
// few nonzero digits among zeros, runs of nines, a 5 among zeros, or any --
// with a point anywhere or none, maybe leading zeros and a sign, and an
// exponent putting the value near 10^_place. Returns its length.
static size_t
d_tests_random_text(
    char* _text,
    long  _digits,
    long  _place
)
{
    const long pattern = d_tests_below(4);
    const long whole   = d_tests_below(_digits + 1L);

    long n        = 0;
    long i        = 0;
    long exponent = 0;

    // a sign, sometimes
    if (d_tests_below(3) == 0)
    {
        _text[n] = (d_tests_below(2) == 0) ? '-' : '+';
        ++n;
    }

    // leading zeros, sometimes
    if (d_tests_below(8) == 0)
    {
        _text[n]      = '0';
        _text[n + 1L] = '0';
        n             = n + 2L;
    }

    // the digits, the point before the whole part's end
    for (i = 0; i < _digits; ++i)
    {
        char digit = (char)('0' + d_tests_below(10));

        // the point
        if (i == whole)
        {
            _text[n] = '.';
            ++n;
        }

        // the pattern, past the first digit
        if ( (i > 0) &&
             (pattern == 0) )
        {
            digit = (d_tests_below(12) == 0) ? digit : '0';
        }
        else if ( (i > 0) &&
                  (pattern == 1) )
        {
            digit = (d_tests_below(12) == 0) ? digit : '9';
        }
        else if ( (i > 0) &&
                  (pattern == 2) )
        {
            digit = (i == 1) ? '5' : ( (d_tests_below(40) == 0) ? '1'
                                                                : '0' );
        }

        _text[n] = digit;
        ++n;
    }

    // a point at the end, sometimes
    if ( (whole == _digits) &&
         (d_tests_below(4) == 0) )
    {
        _text[n] = '.';
        ++n;
    }

    exponent = _place - whole;

    // the exponent, unless it is 0 and left out
    if ( (exponent != 0) ||
         (d_tests_below(2) == 0) )
    {
        _text[n] = (d_tests_below(2) == 0) ? 'e' : 'E';
        ++n;

        // its sign
        if (exponent < 0)
        {
            _text[n] = '-';
            ++n;
        }
        else if (d_tests_below(2) == 0)
        {
            _text[n] = '+';
            ++n;
        }

        n = n + (long)sprintf(_text + n, "%ld", labs(exponent));
    }

    _text[n] = '\0';

    return (size_t)n;
}

// d_tests_random_texts
//   _count random texts, each converted to every type and checked: values
// near 10^_low to 10^_high, of up to 19 digits mostly, up to 40 often, and
// up to _most now and then.
static void
d_tests_random_texts(
    long _count,
    long _low,
    long _high,
    long _most
)
{
    static char text[D_TESTS_TEXT_SIZE];

    long i = 0;

    // each text
    for (i = 0; i < _count; ++i)
    {
        const long share  = d_tests_below(100);
        const long digits = (share < 60) ? (1L + d_tests_below(19))
                          : ( (share < 95) ? (1L + d_tests_below(40))
                                           : (1L + d_tests_below(_most)) );
        const long place  = _low + d_tests_below(_high - _low + 1L);
        size_t     length = d_tests_random_text(text, digits, place);

        d_tests_all(text, length, __LINE__);
    }

    return;
}

// 6.5    A million characters
//------------------------------------------------------------------------------
// D_TESTS_LONG_ZEROS zeros, more than the place is held to, before or after
// a digit, with an exponent the other way: the place must come out right
// all the same. Then a million digits after a tie, a single 1 at the end of
// them.

static char g_long[D_TESTS_LONG_SIZE];

// d_tests_long_text
//   _head, then _zeros zeros, then _tail, into g_long; returns its length.
static size_t
d_tests_long_text(
    const char* _head,
    long        _zeros,
    const char* _tail
)
{
    const size_t head = strlen(_head);
    const size_t tail = strlen(_tail);

    memcpy(g_long, _head, head);
    memset(g_long + head, '0', (size_t)_zeros);
    memcpy(g_long + head + (size_t)_zeros, _tail, tail);
    g_long[head + (size_t)_zeros + tail] = '\0';

    return head + (size_t)_zeros + tail;
}

// d_tests_long_texts
//   the place past its limit both ways, and ties broken a million digits
// away, for each type: 2^P + 1 is a tie between 2^P and 2^P + 2. The
// reference reads the texts whose digits, past their zeros, and whose power
// of ten fit its bits; the others have known values.
static void
d_tests_long_texts(void)
{
    static struct dj_tests_huge h;
    static char                 tie[64];

    const long        zeros = D_TESTS_LONG_ZEROS;
    const float       f_inf = (float)HUGE_VAL;
    const double      d_inf = (double)HUGE_VAL;

    long double                 value  = 0.0L;
    enum dj_math_decimal_status status = D_MATH_DECIMAL_OK;
    int                         type   = 0;

    // opposite signs, each past the limit: 1, exactly
    (void)d_tests_long_text("0.", zeros, "1e1000006");
    d_tests_known_d(g_long, 1.0, D_TESTS_OK, false, __LINE__);
    (void)d_tests_long_text("1", zeros, "e-1000005");
    d_tests_known_ld(g_long, 1.0L, D_TESTS_OK, false, __LINE__);

    // opposite signs, the sum within every range, or past some
    (void)d_tests_long_text("0.", zeros, "1e1000306");
    d_tests_known_d(g_long, 1e300, D_TESTS_OK, false, __LINE__);
    (void)d_tests_long_text("-1", zeros, "e-1001005");
    d_tests_all(g_long, strlen(g_long), __LINE__);
    (void)d_tests_long_text("1", zeros, ".5e-1000004");
    d_tests_known_d(g_long, 10.0, D_TESTS_OK, true, __LINE__);

    // one sign, past the limit: 0 or an infinity
    (void)d_tests_long_text("0.", zeros, "1e-5");
    d_tests_known_d(g_long, 0.0, D_TESTS_UNDERFLOW, true, __LINE__);
    (void)d_tests_long_text("-1", zeros, "e5");
    d_tests_known_d(g_long, -d_inf, D_TESTS_OVERFLOW, true, __LINE__);
    (void)d_tests_long_text("1", zeros, ".5");
    d_tests_known_f(g_long, f_inf, D_TESTS_OVERFLOW, true, __LINE__);
    (void)d_tests_long_text("-.", zeros, "");
    d_tests_known_d(g_long, -0.0, D_TESTS_OK, false, __LINE__);

    // ties, broken only by a 1 a million digits on
    for (type = D_TESTS_FLOAT; type <= D_TESTS_LONG_DOUBLE; ++type)
    {
        const int         precision = d_tests_precision(type);
        const long double power     = ldexpl(1.0L, precision);
        long              count     = 0;
        size_t            length    = 0u;

        d_tests_huge_set(&h, 1UL);
        d_tests_huge_shl(&h, (long)precision);
        d_tests_huge_mul_add(&h, 1UL, 1UL);
        count = d_tests_huge_digits(&h, tie, (long)sizeof(tie) - 2L);
        tie[count]      = '.';
        tie[count + 1L] = '\0';

        length = d_tests_long_text(tie, zeros, "");
        D_TESTS_EXPECT( (d_tests_convert(type, g_long, length, &value) ==
                         D_MATH_DECIMAL_OK) &&
                        (value == power),
                        "2^P + 1, then a million zeros: a tie, to 2^P",
                        g_long, length );
        d_tests_one(type, g_long, length, &value, &status, __LINE__);

        length = d_tests_long_text(tie, zeros, "1");
        D_TESTS_EXPECT( (d_tests_convert(type, g_long, length, &value) ==
                         D_MATH_DECIMAL_OK) &&
                        (value == (power + 2.0L)),
                        "2^P + 1, a million zeros, then 1: up to 2^P + 2",
                        g_long, length );
    }

    return;
}


//==============================================================================
// 7.  CONSTANT EVALUATION
//==============================================================================
// From C++14 the conversions are constant expressions, the ends of a double's
// range included; a long double's ends pass the compilers' step limits.

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

static constexpr float
d_tests_constexpr_f(
    const char* _text,
    size_t      _length
)
{
    float result = 0.0f;

    d_math_decimal_to_f(&result, _text, _length);

    return result;
}

static constexpr double
d_tests_constexpr_d(
    const char* _text,
    size_t      _length
)
{
    double result = 0.0;

    d_math_decimal_to_d(&result, _text, _length);

    return result;
}

static constexpr long double
d_tests_constexpr_ld(
    const char* _text,
    size_t      _length
)
{
    long double result = 0.0L;

    d_math_decimal_to_ld(&result, _text, _length);

    return result;
}

static constexpr enum dj_math_decimal_status
d_tests_constexpr_status(
    const char* _text,
    size_t      _length
)
{
    double result = 0.0;

    return d_math_decimal_to_d(&result, _text, _length);
}

static_assert( (d_tests_constexpr_f("0.1", 3u) == 0.1f) &&
               (d_tests_constexpr_d("0.1", 3u) == 0.1) &&
               (d_tests_constexpr_ld("0.1", 3u) == 0.1L),
               "0.1 in each type as a constant expression" );
static_assert( d_tests_constexpr_d("9007199254740993", 16u) ==
               9007199254740992.0,
               "a tie goes to even as a constant expression" );
static_assert( (d_tests_constexpr_d("4.9406564584124654e-324", 23u) ==
                4.9406564584124654e-324) &&
               (d_tests_constexpr_d("1.7976931348623157e308", 22u) ==
                1.7976931348623157e308),
               "a double's ends as constant expressions" );
static_assert( (d_tests_constexpr_status("1e400", 5u) ==
                D_MATH_DECIMAL_OVERFLOW) &&
               (d_tests_constexpr_status("1e-400", 6u) ==
                D_MATH_DECIMAL_UNDERFLOW) &&
               (d_tests_constexpr_status("1x", 2u) ==
                D_MATH_DECIMAL_SYNTAX),
               "the statuses as constant expressions" );
static_assert( d_math_decimal_length("12.5e+3x", 8u) == 7u,
               "a length measured as a constant expression" );

// d_tests_constant_evaluation
//   the same at run time.
static void
d_tests_constant_evaluation(void)
{
    D_TESTS_EXPECT( (d_tests_constexpr_f("0.1", 3u) == 0.1f) &&
                    (d_tests_constexpr_d("9007199254740993", 16u) ==
                     9007199254740992.0) &&
                    (d_tests_constexpr_ld("0.1", 3u) == 0.1L) &&
                    (d_tests_constexpr_status("1e-400", 6u) ==
                     D_MATH_DECIMAL_UNDERFLOW) &&
                    (d_math_decimal_length("12.5e+3x", 8u) == 7u),
                    "the constant expressions, at run time", "", 0u );

    return;
}

#endif  // C++14


int
main(void)
{
    d_tests_syntax();
    d_tests_known();
    d_tests_long_texts();
    d_tests_points(D_TESTS_FLOAT, 1200L, 50L);
    d_tests_points(D_TESTS_DOUBLE, 1200L, 50L);
    d_tests_points(D_TESTS_LONG_DOUBLE, 160L, 25L);
    d_tests_random_texts(4000L, -50L, 45L, 60L);
    d_tests_random_texts(16000L, -345L, 315L, 800L);
    d_tests_random_texts(200L, -4970L, 4940L, 2000L);
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    // the reference never needed more than its limbs
    D_TESTS_EXPECT(!g_huge_overrun, "the reference's 54,400 bits held", "",
                   0u);

    printf("t_decimal: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
