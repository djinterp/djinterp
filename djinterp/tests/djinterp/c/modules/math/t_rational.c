/*******************************************************************************
* djinterp [test]                                                   t_rational.c
*
*   Conformance harness for the rational kernels (c/math/rational.h): every
* kernel against an exact reference, whose integers are 256 bits in 16-bit
* limbs, so that it needs no type wider than 32 bits and runs alike in every
* build, ISO strict C++98 on a 32-bit target included.
*   A result a kernel stores must be in the one form (a positive denominator,
* no common factor) and equal the exact value, by cross-multiplication; being
* stored at all, it fits. An overflow must be one: the exact value, reduced by
* its greatest common divisor, must lie outside dj_math_imax, and the
* destination must be as it was. A pole or an invalid result must be the case
* that calls for it, the destination again as it was. A comparison must agree
* with the exact cross products.
*   A conversion to float, double or long double must give the value of its
* type nearest the exact ratio, ties to even: by exact comparison, neither of
* its neighbours, found with the C library's nextafter, is nearer.
*   The operands, all made by the harness, none by the kernels: every
* numerator and denominator in [-12, 12], for make and the one-form test, and
* every rational among them, each pair; the family's ends and the values
* around 2^(w/2), where the product of two terms passes a word, each pair;
* and 40,000 pairs drawn at random, each term of a random length up to the
* family's, which must reach both answers, a stored result and an overflow,
* often. Each rational among them is also converted. The double-width helpers
* are checked on their own as well, divmod_wide with divisors of every
* length, the ones past 2^(w-1) included, which the kernels never give it;
* and so are 20,000 two-word ratios to long double, and the ties and the runs
* of ones of each precision that the terms can reach. From C++14 the kernels
* are checked as constant expressions too.
*   The same file compiles as C++ (-x c++), at any level from C++98: the
* kernels are one text in both languages.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_rational.c -o t_rational
*
*
* path:      /tests/djinterp/c/modules/math/t_rational.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/
// std
#include <math.h>    // frexpf, frexp, frexpl, nextafterf, nextafter,
                     // nextafterl, ldexpl, floorl
#include <stdio.h>   // printf
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/rational.h"  // unit under test
// tests
#include "./math_tests_exact.h"  // the exact reference


// the family's ends, and 2^(w/2), where the product of two terms passes a word
#define D_TESTS_MOST    D_MATH_IMAX_MAX
#define D_TESTS_LEAST   D_MATH_IMAX_MIN
#define D_TESTS_HALF                                                           \
    ((dj_math_imax)((dj_math_umax)1 << D_INTERNAL_RATIONAL_HALF_BITS))

// the pairs drawn at random, and the draws for the double-width helpers
#define D_TESTS_TRIALS       40000L
#define D_TESTS_WIDE_TRIALS  20000L

// a destination's numerator before a kernel runs, over 0: no kernel stores it
#define D_TESTS_MARKER  12345

static long g_checks   = 0;
static long g_failures = 0;

// d_tests_count
//   counts a check: true when it failed and is among the first few failures,
// which are reported in full.
static
bool d_tests_count(
    bool _ok
)
{
    ++g_checks;

    // a failure
    if (!_ok)
    {
        ++g_failures;

        return (g_failures <= 40);
    }

    return false;
}

// d_tests_expect
//   counts a check on rationals, and reports the first few failures with
// their operands.
static
void d_tests_expect(
    bool               _ok,
    const char*        _what,
    struct dj_rational _a,
    struct dj_rational _b,
    int                _line
)
{
    char text[4][24];

    // the first failures, with their operands
    if (d_tests_count(_ok))
    {
        printf("  FAIL line %d: %s (%s/%s, %s/%s)\n", _line, _what,
               d_tests_text(_a.num, text[0]),
               d_tests_text(_a.den, text[1]),
               d_tests_text(_b.num, text[2]),
               d_tests_text(_b.den, text[3]));
    }

    return;
}

// d_tests_expect_words
//   counts a check on words, and reports the first few failures with their
// operands.
static
void d_tests_expect_words(
    bool         _ok,
    const char*  _what,
    dj_math_umax _a,
    dj_math_umax _b,
    int          _line
)
{
    char text[2][24];

    // the first failures, with their operands
    if (d_tests_count(_ok))
    {
        printf("  FAIL line %d: %s (%s, %s)\n", _line, _what,
               d_tests_text_umax(_a, text[0]),
               d_tests_text_umax(_b, text[1]));
    }

    return;
}

// d_tests_expect_wide
//   counts a check on a ratio of two-word integers, and reports the first
// few failures with it, each word in decimal, the high word first.
static
void d_tests_expect_wide(
    bool                             _ok,
    const char*                      _what,
    struct dj_internal_rational_wide _n,
    struct dj_internal_rational_wide _d,
    int                              _line
)
{
    char text[4][24];

    // the first failures, with their operands
    if (d_tests_count(_ok))
    {
        printf("  FAIL line %d: %s (%s:%s / %s:%s)\n", _line, _what,
               d_tests_text_umax(_n.hi, text[0]),
               d_tests_text_umax(_n.lo, text[1]),
               d_tests_text_umax(_d.hi, text[2]),
               d_tests_text_umax(_d.lo, text[3]));
    }

    return;
}

#define D_TESTS_EXPECT(ok, what, a, b)                                         \
    d_tests_expect((ok), (what), (a), (b), __LINE__)
#define D_TESTS_EXPECT_WORDS(ok, what, a, b)                                   \
    d_tests_expect_words((ok), (what), (a), (b), __LINE__)
#define D_TESTS_EXPECT_WIDE(ok, what, n, d)                                    \
    d_tests_expect_wide((ok), (what), (n), (d), __LINE__)


//==============================================================================
// 1.  A KERNEL'S ANSWER
//==============================================================================
// Checked against the exact reference, math_tests_exact.h.

// d_tests_marker
//   what a destination holds before a kernel runs.
static
struct dj_rational d_tests_marker(void)
{
    struct dj_rational marker = { D_TESTS_MARKER, 0 };

    return marker;
}

// d_tests_untouched
//   whether _x still holds the marker.
static
bool d_tests_untouched(
    struct dj_rational _x
)
{
    return ( (_x.num == D_TESTS_MARKER) &&
             (_x.den == 0) );
}

// d_tests_answer_ok
//   whether a kernel's answer, its status and what _out holds after it ran,
// is right for the exact value _want: stored in the one form and equal to
// it, or an overflow that is one, _out untouched.
static bool
d_tests_answer_ok(
    enum dj_rational_status _status,
    struct dj_rational      _out,
    struct dj_tests_exact   _want
)
{
    // stored: the one form, and the exact value
    if (_status == D_RATIONAL_OK)
    {
        return ( (d_tests_in_one_form(_out)) &&
                 (d_tests_exact_cmp(d_tests_exact_of(_out.num, _out.den),
                                    _want) == 0) );
    }

    return ( (_status == D_RATIONAL_OVERFLOW) &&
             (!d_tests_exact_fits(_want)) &&
             (d_tests_untouched(_out)) );
}


//==============================================================================
// 2.  ONE OPERATION, CHECKED
//==============================================================================
// Each takes operands in the one form and makes one check of the kernels'
// whole answer.

// how many sums and products stored a result, and how many overflowed
static long g_stored     = 0;
static long g_overflowed = 0;

static void
d_tests_tally(
    enum dj_rational_status _status
)
{
    g_stored     += (_status == D_RATIONAL_OK) ? 1L : 0L;
    g_overflowed += (_status == D_RATIONAL_OVERFLOW) ? 1L : 0L;

    return;
}

// d_tests_check_sum
//   add, or sub where _subtract, on (_a, _b).
static
void d_tests_check_sum(
    struct dj_rational _a,
    struct dj_rational _b,
    bool               _subtract
)
{
    const struct dj_tests_exact a    = d_tests_exact_of(_a.num, _a.den);
    const struct dj_tests_exact b    = d_tests_exact_of(_b.num, _b.den);
    const struct dj_tests_exact want =
        d_tests_exact_add(a, _subtract ? d_tests_exact_neg(b) : b);

    struct dj_rational            out    = d_tests_marker();
    const enum dj_rational_status status = _subtract
                                              ? d_rational_sub(&out, _a, _b)
                                              : d_rational_add(&out, _a, _b);

    d_tests_tally(status);
    D_TESTS_EXPECT(d_tests_answer_ok(status, out, want),
                   _subtract ? "sub" : "add", _a, _b);

    return;
}

// d_tests_check_product
//   mul, or div where _divide, on (_a, _b).
static
void d_tests_check_product(
    struct dj_rational _a,
    struct dj_rational _b,
    bool               _divide
)
{
    struct dj_rational            out    = d_tests_marker();
    const enum dj_rational_status status = _divide
                                              ? d_rational_div(&out, _a, _b)
                                              : d_rational_mul(&out, _a, _b);

    // by zero: a pole, or invalid for 0 / 0, and nothing stored
    if ( (_divide) &&
         (_b.num == 0) )
    {
        const enum dj_rational_status want = (_a.num == 0)
                                                ? D_RATIONAL_INVALID
                                                : D_RATIONAL_POLE;

        D_TESTS_EXPECT( (status == want) &&
                        (d_tests_untouched(out)),
                        "div by zero", _a, _b);

        return;
    }

    const struct dj_tests_exact a    = d_tests_exact_of(_a.num, _a.den);
    const struct dj_tests_exact b    = d_tests_exact_of(_b.num, _b.den);
    const struct dj_tests_exact want =
        d_tests_exact_mul(a, _divide ? d_tests_exact_inv(b) : b);

    d_tests_tally(status);
    D_TESTS_EXPECT(d_tests_answer_ok(status, out, want),
                   _divide ? "div" : "mul", _a, _b);

    return;
}

// d_tests_check_compare
//   cmp and eq on (_a, _b).
static
void d_tests_check_compare(
    struct dj_rational _a,
    struct dj_rational _b
)
{
    const int want = d_tests_exact_cmp(d_tests_exact_of(_a.num, _a.den),
                                       d_tests_exact_of(_b.num, _b.den));

    D_TESTS_EXPECT( (d_rational_cmp(_a, _b) == want) &&
                    (d_rational_eq(_a, _b) == (want == 0)),
                    "cmp and eq", _a, _b);

    return;
}

// d_tests_check_unary
//   neg, abs and inv of _x, its sign, and the integer and one-form tests:
// one check.
static
void d_tests_check_unary(
    struct dj_rational _x
)
{
    const struct dj_tests_exact x    = d_tests_exact_of(_x.num, _x.den);
    const int                  sign = (_x.num > 0) ? 1
                                                   : ((_x.num < 0) ? -1 : 0);

    struct dj_rational            negated    = d_tests_marker();
    struct dj_rational            absolute   = d_tests_marker();
    struct dj_rational            inverse    = d_tests_marker();
    const enum dj_rational_status neg_status = d_rational_neg(&negated, _x);
    const enum dj_rational_status abs_status = d_rational_abs(&absolute, _x);
    const enum dj_rational_status inv_status = d_rational_inv(&inverse, _x);

    const bool neg_ok =
        d_tests_answer_ok(neg_status, negated, d_tests_exact_neg(x));
    const bool abs_ok =
        d_tests_answer_ok(abs_status, absolute, d_tests_exact_abs(x));

    // 1/0 is a pole, and stores nothing
    const bool inv_ok =
        (_x.num == 0)
            ? ( (inv_status == D_RATIONAL_POLE) &&
                (d_tests_untouched(inverse)) )
            : d_tests_answer_ok(inv_status, inverse, d_tests_exact_inv(x));

    D_TESTS_EXPECT( (neg_ok) &&
                    (abs_ok) &&
                    (inv_ok) &&
                    (d_rational_sign(_x) == sign) &&
                    (d_rational_is_integer(_x) == (_x.den == 1)) &&
                    (d_rational_is_valid(_x)) &&
                    (d_tests_in_one_form(_x)),
                    "neg, abs, inv, sign, is_integer, is_valid", _x, _x);

    return;
}

// d_tests_check_make
//   d_rational_make(_num, _den) against the exact quotient.
static
void d_tests_check_make(
    dj_math_imax _num,
    dj_math_imax _den
)
{
    const struct dj_rational      args   = { _num, _den };
    struct dj_rational            out    = d_tests_marker();
    const enum dj_rational_status status = d_rational_make(&out, _num, _den);

    // over zero: a pole, or invalid for 0/0, and nothing stored
    if (_den == 0)
    {
        D_TESTS_EXPECT( (status == ((_num == 0) ? D_RATIONAL_INVALID
                                                : D_RATIONAL_POLE)) &&
                        (d_tests_untouched(out)),
                        "make over zero", args, args);

        return;
    }

    D_TESTS_EXPECT(d_tests_answer_ok(status, out,
                                     d_tests_exact_of(_num, _den)),
                   "make", args, args);

    return;
}

// d_tests_check_pair
//   every binary kernel on (_a, _b).
static
void d_tests_check_pair(
    struct dj_rational _a,
    struct dj_rational _b
)
{
    d_tests_check_sum(_a, _b, false);
    d_tests_check_sum(_a, _b, true);
    d_tests_check_product(_a, _b, false);
    d_tests_check_product(_a, _b, true);
    d_tests_check_compare(_a, _b);

    return;
}


//==============================================================================
// 3.  DRAWS
//==============================================================================
// A 32-bit linear congruential generator, the same in every build, and the
// words and rationals drawn from it.

static unsigned long g_state = 2718281UL;

// d_tests_random16
//   16 bits from the generator: the high half of its state.
static
unsigned long d_tests_random16(void)
{
    g_state = ((g_state * 1103515245UL) + 12345UL) & 0xFFFFFFFFUL;

    return (g_state >> 16) & 0xFFFFUL;
}

// d_tests_random_bits
//   a dj_math_umax below 2^_bits, _bits at most the word's, every value
// alike.
static
dj_math_umax d_tests_random_bits(
    unsigned _bits
)
{
    dj_math_umax value = 0u;
    unsigned    have  = 0u;

    // sixteen bits at a time, then the excess shifted away
    while (have < _bits)
    {
        value = (value << 16) | (dj_math_umax)d_tests_random16();
        have  = have + 16u;
    }

    return value >> (have - _bits);
}

// d_tests_random_word
//   one time in four a word with its top bit set; otherwise one below 2^n,
// n from 1 to w.
static
dj_math_umax d_tests_random_word(void)
{
    const unsigned    width = D_INTERNAL_RATIONAL_WORD_BITS;
    const dj_math_umax top   = (dj_math_umax)1 << (width - 1u);

    // the top bit set
    if ((d_tests_random16() & 3UL) == 0UL)
    {
        return d_tests_random_bits(width) | top;
    }

    return d_tests_random_bits(1u + (unsigned)(d_tests_random16() % width));
}

// d_tests_random_rational
//   a numerator below 2^n, either sign, over a denominator below 2^m, each
// length drawn up to the family's, reduced by the harness's own gcd.
static
struct dj_rational d_tests_random_rational(void)
{
    const unsigned    width = D_INTERNAL_RATIONAL_WORD_BITS;
    const unsigned    nbits = (unsigned)(d_tests_random16() % width);
    const unsigned    dbits =
        1u + (unsigned)(d_tests_random16() % (width - 1u));
    const dj_math_umax n     = d_tests_random_bits(nbits);
    const dj_math_umax drawn = d_tests_random_bits(dbits);
    const bool        neg   = ((d_tests_random16() & 1UL) != 0UL);

    // a zero denominator drawn is taken as 1
    const dj_math_umax d = (drawn == 0u) ? 1u : drawn;
    const dj_math_umax g = d_tests_gcd(n, d);

    struct dj_rational result = { 0, 1 };

    result.num = neg ? -(dj_math_imax)(n / g) : (dj_math_imax)(n / g);
    result.den = (dj_math_imax)(d / g);

    return result;
}


//==============================================================================
// 4.  DOUBLE WIDTH
//==============================================================================
// The helpers on their own, against the reference. The kernels divide only by
// divisors below 2^(w-1), so divmod_wide's bit carried out of the word is
// reached here alone, by divisors of every length.

// d_tests_wide
//   D_TESTS_WIDE_TRIALS draws of four words and a divisor: mul_wide on two
// pairs of them, each against the exact product; add_wide on the products
// of their halves, which stay within the callers' bound; cmp_wide and
// sub_wide on the two full products; and divmod_wide of the first by the
// divisor, its quotient times the divisor plus its remainder giving the
// product back, the remainder below the divisor.
static
void d_tests_wide(void)
{
    long trial = 0;

    // each draw
    for (trial = 0; trial < D_TESTS_WIDE_TRIALS; ++trial)
    {
        const dj_math_umax a     = d_tests_random_word();
        const dj_math_umax b     = d_tests_random_word();
        const dj_math_umax c     = d_tests_random_word();
        const dj_math_umax e     = d_tests_random_word();
        const dj_math_umax drawn = d_tests_random_word();
        const dj_math_umax d     = (drawn == 0u) ? 1u : drawn;

        // the helpers' answers
        const struct dj_internal_rational_wide p =
            d_internal_rational_mul_wide(a, b);
        const struct dj_internal_rational_wide q =
            d_internal_rational_mul_wide(c, e);
        const struct dj_internal_rational_wide p_half =
            d_internal_rational_mul_wide(a >> 1, b >> 1);
        const struct dj_internal_rational_wide q_half =
            d_internal_rational_mul_wide(c >> 1, e >> 1);
        const struct dj_internal_rational_wide sum =
            d_internal_rational_add_wide(p_half, q_half);
        const int order = d_internal_rational_cmp_wide(p, q);
        const struct dj_internal_rational_wide difference =
            (order >= 0) ? d_internal_rational_sub_wide(p, q)
                         : d_internal_rational_sub_wide(q, p);
        const struct dj_internal_rational_divmod split =
            d_internal_rational_divmod_wide(p, d);

        // the reference's
        const struct dj_tests_big exact_p = d_tests_big_of_product(a, b);
        const struct dj_tests_big exact_q = d_tests_big_of_product(c, e);
        const struct dj_tests_big halves  =
            d_tests_big_add(d_tests_big_of_wide(p_half),
                            d_tests_big_of_wide(q_half));
        const struct dj_tests_big gap     = d_tests_big_abs(
            d_tests_big_add(exact_p, d_tests_big_neg(exact_q)));
        const struct dj_tests_big back    = d_tests_big_add(
            d_tests_big_mul(d_tests_big_of_wide(split.quotient),
                            d_tests_big_of_umax(d, false)),
            d_tests_big_of_umax(split.remainder, false));

        // the products, exact
        D_TESTS_EXPECT_WORDS(
            (d_tests_big_cmp(d_tests_big_of_wide(p), exact_p) == 0) &&
            (d_tests_big_cmp(d_tests_big_of_wide(q), exact_q) == 0),
            "mul_wide", a, c);

        // the sum of the halves' products
        D_TESTS_EXPECT_WORDS(
            d_tests_big_cmp(d_tests_big_of_wide(sum), halves) == 0,
            "add_wide", a, c);

        // the order, and the larger less the smaller
        D_TESTS_EXPECT_WORDS(
            (order == d_tests_big_cmp(exact_p, exact_q)) &&
            (d_tests_big_cmp(d_tests_big_of_wide(difference), gap) == 0),
            "cmp_wide and sub_wide", a, c);

        // the quotient and the remainder
        D_TESTS_EXPECT_WORDS( (split.remainder < d) &&
                              (d_tests_big_cmp(back, exact_p) == 0),
                              "divmod_wide", p.hi, d);
    }

    return;
}

//==============================================================================
// 5.  TO FLOATING POINT
//==============================================================================
// Each result against math_tests_exact.h's oracle: the value of its type
// nearest the exact ratio, ties to even; and each rounding's direction.

// d_tests_direction_ok
//   whether the rounding _r of the positive ratio _n / _d says rightly which
// way it went: its direction the sign of m * 2^e less _n / _d, compared
// exactly, as m * _d * 2^(e + s) against _n * 2^s.
static
bool d_tests_direction_ok(
    struct dj_tests_big                 _n,
    struct dj_tests_big                 _d,
    struct dj_internal_rational_rounded _r
)
{
    const int                shift  = (_r.exponent < 0) ? -_r.exponent : 0;
    const struct dj_tests_big m      = d_tests_big_of_wide(_r.mantissa);
    const struct dj_tests_big exact  = d_tests_big_shl(_n, shift);
    const struct dj_tests_big result =
        d_tests_big_shl(d_tests_big_mul(m, _d), _r.exponent + shift);

    return (d_tests_big_cmp_mag(result, exact) == _r.direction);
}

// d_tests_directions_ok
//   d_tests_direction_ok for the ratio _n / _d of words, rounded to each of
// the three precisions.
static
bool d_tests_directions_ok(
    dj_math_umax _n,
    dj_math_umax _d
)
{
    const struct dj_tests_big n         = d_tests_big_of_umax(_n, false);
    const struct dj_tests_big d         = d_tests_big_of_umax(_d, false);
    const unsigned           widths[3] = { (unsigned)FLT_MANT_DIG,
                                           (unsigned)DBL_MANT_DIG,
                                           (unsigned)LDBL_MANT_DIG };

    struct dj_internal_rational_wide wn = { 0u, 0u };
    struct dj_internal_rational_wide wd = { 0u, 0u };
    bool                            ok = true;
    int                             i  = 0;

    wn.lo = _n;
    wd.lo = _d;

    // each precision
    for (i = 0; i < 3; ++i)
    {
        ok = ok && d_tests_direction_ok(
                       n, d, d_internal_rational_round(wn, wd, widths[i]));
    }

    return ok;
}

// d_tests_check_to_float
//   d_rational_to_f, _to_d and _to_ld on _x, one check: each the nearest
// value of its type, ties to even, and 0 as +0; and the rounding's direction
// right at each precision.
static
void d_tests_check_to_float(
    struct dj_rational _x
)
{
    const float       f  = d_rational_to_f(_x);
    const double      d  = d_rational_to_d(_x);
    const long double ld = d_rational_to_ld(_x);

    // 0: +0, whose reciprocal is +infinity
    if (_x.num == 0)
    {
        D_TESTS_EXPECT( (f == 0.0f) &&
                        ((1.0f / f) > 0.0f) &&
                        (d == 0.0) &&
                        ((1.0 / d) > 0.0) &&
                        (ld == 0.0L) &&
                        ((1.0L / ld) > 0.0L),
                        "to_f, to_d, to_ld of 0", _x, _x);

        return;
    }

    const struct dj_tests_big n   = d_tests_big_abs(d_tests_big_of(_x.num));
    const struct dj_tests_big den = d_tests_big_of(_x.den);
    const bool               neg = (_x.num < 0);

    D_TESTS_EXPECT( (d_tests_nearest_f(neg, n, den, f)) &&
                    (d_tests_nearest_d(neg, n, den, d)) &&
                    (d_tests_nearest_ld(neg, n, den, ld)) &&
                    (d_tests_directions_ok(d_tests_magnitude(_x.num),
                                           (dj_math_umax)_x.den)),
                    "to_f, to_d, to_ld", _x, _x);

    return;
}

// d_tests_check_wide_to_ld
//   d_internal_rational_wide_to_ld on -(_n / _d) where _negative, else
// _n / _d: the nearest long double, ties to even, and 0 as +0.
static
void d_tests_check_wide_to_ld(
    bool                             _negative,
    struct dj_internal_rational_wide _n,
    struct dj_internal_rational_wide _d
)
{
    const long double        r =
        d_internal_rational_wide_to_ld(_negative, _n, _d);
    const struct dj_tests_big n = d_tests_big_of_wide(_n);
    const struct dj_tests_big d = d_tests_big_of_wide(_d);

    // 0: +0, whatever the sign asked for
    if (d_tests_big_is_zero(n))
    {
        D_TESTS_EXPECT_WIDE( (r == 0.0L) &&
                             ((1.0L / r) > 0.0L),
                             "wide_to_ld of 0", _n, _d);

        return;
    }

    D_TESTS_EXPECT_WIDE( (d_tests_nearest_ld(_negative, n, d, r)) &&
                         (d_tests_direction_ok(
                              n, d,
                              d_internal_rational_round(
                                  _n, _d, (unsigned)LDBL_MANT_DIG))),
                         "wide_to_ld", _n, _d);

    return;
}

// d_tests_wide_of
//   _high * 2^w + _low.
static
struct dj_internal_rational_wide d_tests_wide_of(
    dj_math_umax _high,
    dj_math_umax _low
)
{
    struct dj_internal_rational_wide result = { 0u, 0u };

    result.hi = _high;
    result.lo = _low;

    return result;
}

// d_tests_power
//   2^_k, for _k below 2w, in two words.
static
struct dj_internal_rational_wide d_tests_power(
    unsigned _k
)
{
    const unsigned width = D_INTERNAL_RATIONAL_WORD_BITS;

    // the high word's bit, or the low word's
    if (_k >= width)
    {
        return d_tests_wide_of((dj_math_umax)1 << (_k - width), 0u);
    }

    return d_tests_wide_of(0u, (dj_math_umax)1 << _k);
}

// d_tests_plus
//   _x + _k, for a small _k and an _x that leaves room for it.
static
struct dj_internal_rational_wide d_tests_plus(
    struct dj_internal_rational_wide _x,
    dj_math_umax                     _k
)
{
    struct dj_internal_rational_wide result = _x;

    result.lo = _x.lo + _k;

    // the carry
    if (result.lo < _x.lo)
    {
        result.hi = result.hi + 1u;
    }

    return result;
}

// d_tests_check_known_ratio
//   the conversions of _n / _d, both nonzero: as a two-word ratio, either
// sign; and where both terms fit dj_math_imax, as a rational, reduced by the
// harness's own gcd, either sign.
static
void d_tests_check_known_ratio(
    struct dj_internal_rational_wide _n,
    struct dj_internal_rational_wide _d
)
{
    const dj_math_umax most = (dj_math_umax)D_TESTS_MOST;

    d_tests_check_wide_to_ld(false, _n, _d);
    d_tests_check_wide_to_ld(true, _n, _d);

    // both terms within a word, and below MAX
    if ( (_n.hi == 0u) &&
         (_d.hi == 0u) &&
         (_n.lo <= most) &&
         (_d.lo <= most) )
    {
        const dj_math_umax g = d_tests_gcd(_n.lo, _d.lo);

        struct dj_rational x = { 0, 1 };

        x.num = (dj_math_imax)(_n.lo / g);
        x.den = (dj_math_imax)(_d.lo / g);
        d_tests_check_to_float(x);
        x.num = -x.num;
        d_tests_check_to_float(x);
    }

    return;
}

// d_tests_to_float_known
//   conversions worth naming, for each precision P there is that the terms
// can reach: the ties 2^P + 1, down to 2^P, and 2^P + 3, up to 2^P + 4, and
// (2^P + 1) / 2; 2^(P + 1) / (2^(P + 1) + 1), whose mantissa is all ones and
// rounds up to 1; then the two-word ratios' ends, and 0.
static
void d_tests_to_float_known(void)
{
    const unsigned width        = D_INTERNAL_RATIONAL_WORD_BITS;
    const unsigned precisions[] = { (unsigned)FLT_MANT_DIG,
                                    (unsigned)DBL_MANT_DIG,
                                    (unsigned)LDBL_MANT_DIG };
    const struct dj_internal_rational_wide one  = d_tests_wide_of(0u, 1u);
    const struct dj_internal_rational_wide two  = d_tests_wide_of(0u, 2u);
    const struct dj_internal_rational_wide zero = d_tests_wide_of(0u, 0u);
    const struct dj_internal_rational_wide full =
        d_tests_wide_of(D_MATH_UMAX_MAX, D_MATH_UMAX_MAX);
    const struct dj_internal_rational_wide half =
        d_tests_power((2u * width) - 1u);

    int i = 0;

    // each precision: the ties need P + 1 bits, the run of ones P + 2
    for (i = 0; i < 3; ++i)
    {
        const unsigned p = precisions[i];

        if ((p + 1u) < (2u * width))
        {
            d_tests_check_known_ratio(d_tests_plus(d_tests_power(p), 1u), one);
            d_tests_check_known_ratio(d_tests_plus(d_tests_power(p), 3u), one);
            d_tests_check_known_ratio(d_tests_plus(d_tests_power(p), 1u), two);
        }

        if ((p + 2u) < (2u * width))
        {
            d_tests_check_known_ratio(
                d_tests_power(p + 1u),
                d_tests_plus(d_tests_power(p + 1u), 1u));
        }
    }

    // the ends: 2^(2w) - 1 over 1, and back; 1 itself; one length over the
    // same length less one
    d_tests_check_known_ratio(full, one);
    d_tests_check_known_ratio(one, full);
    d_tests_check_known_ratio(full, full);
    d_tests_check_known_ratio(half, d_tests_wide_of(D_MATH_UMAX_MAX >> 1,
                                                    D_MATH_UMAX_MAX));
    d_tests_check_wide_to_ld(true, zero, two);

    return;
}

// d_tests_random_wide
//   a two-word integer below 2^b, b drawn from _least to 2w; or one time in
// four, all 2w bits with the top one set.
static
struct dj_internal_rational_wide d_tests_random_wide(
    unsigned _least
)
{
    const unsigned    width = D_INTERNAL_RATIONAL_WORD_BITS;
    const dj_math_umax top   = (dj_math_umax)1 << (width - 1u);
    const unsigned    bits  =
        _least + (unsigned)(d_tests_random16() % ((2u * width) + 1u - _least));

    // the top bit set
    if ((d_tests_random16() & 3UL) == 0UL)
    {
        return d_tests_wide_of(d_tests_random_bits(width) | top,
                               d_tests_random_bits(width));
    }

    // the high word's share, then the low word's
    if (bits > width)
    {
        return d_tests_wide_of(d_tests_random_bits(bits - width),
                               d_tests_random_bits(width));
    }

    return d_tests_wide_of(0u, d_tests_random_bits(bits));
}

// d_tests_to_float_wide
//   D_TESTS_WIDE_TRIALS two-word ratios drawn at random, either sign, each
// to long double.
static
void d_tests_to_float_wide(void)
{
    long trial = 0;

    // each draw; a zero denominator is taken as 1
    for (trial = 0; trial < D_TESTS_WIDE_TRIALS; ++trial)
    {
        const struct dj_internal_rational_wide n = d_tests_random_wide(0u);
        const struct dj_internal_rational_wide drawn = d_tests_random_wide(1u);
        const struct dj_internal_rational_wide d =
            ( (drawn.hi == 0u) &&
              (drawn.lo == 0u) ) ? d_tests_wide_of(0u, 1u) : drawn;
        const bool negative = ((d_tests_random16() & 1UL) != 0UL);

        d_tests_check_wide_to_ld(negative, n, d);
    }

    return;
}


//==============================================================================
// 6.  THE OPERANDS
//==============================================================================

// d_tests_known
//   results worth naming: sums whose cross terms pass dj_math_imax, and a
// word, while the sum fits; the ends' overflows; a destination that is also
// an operand; and from_int.
static
void d_tests_known(void)
{
    const struct dj_rational third  = { 1, 3 };
    const struct dj_rational sixth  = { 1, 6 };
    const struct dj_rational most_3 = { D_TESTS_MOST, 3 };
    const struct dj_rational most_6 = { D_TESTS_MOST, 6 };
    const struct dj_rational most   = d_rational_from_int(D_TESTS_MOST);
    const struct dj_rational least  = d_rational_from_int(D_TESTS_LEAST);

    struct dj_rational out = d_tests_marker();

    // from_int: n/1, at the ends too
    D_TESTS_EXPECT( (most.num == D_TESTS_MOST) &&
                    (most.den == 1) &&
                    (least.num == D_TESTS_LEAST) &&
                    (least.den == 1),
                    "from_int", most, least);

    // 1/3 + 1/6 = 1/2
    D_TESTS_EXPECT( (d_rational_add(&out, third, sixth) == D_RATIONAL_OK) &&
                    (out.num == 1) &&
                    (out.den == 2),
                    "1/3 + 1/6 is 1/2", third, sixth);

    // MAX/6 + MAX/6 = MAX/3: the cross term, 2 MAX, passes dj_math_imax
    D_TESTS_EXPECT( (d_rational_add(&out, most_6, most_6) == D_RATIONAL_OK) &&
                    (out.num == D_TESTS_MOST) &&
                    (out.den == 3),
                    "MAX/6 + MAX/6 is MAX/3", most_6, most_6);

    // MAX/3 + MAX/6 = MAX/2: the cross term, 3 MAX, passes a word
    D_TESTS_EXPECT( (d_rational_add(&out, most_3, most_6) == D_RATIONAL_OK) &&
                    (out.num == D_TESTS_MOST) &&
                    (out.den == 2),
                    "MAX/3 + MAX/6 is MAX/2", most_3, most_6);

    // the destination one of the operands: 1/3 + 1/3 into the first
    out = third;
    D_TESTS_EXPECT( (d_rational_add(&out, out, out) == D_RATIONAL_OK) &&
                    (out.num == 2) &&
                    (out.den == 3),
                    "1/3 + 1/3 into an operand", third, third);

    // -MIN, |MIN| and 1/MIN do not fit, and store nothing
    out = d_tests_marker();
    D_TESTS_EXPECT( (d_rational_neg(&out, least) == D_RATIONAL_OVERFLOW) &&
                    (d_rational_abs(&out, least) == D_RATIONAL_OVERFLOW) &&
                    (d_rational_inv(&out, least) == D_RATIONAL_OVERFLOW) &&
                    (d_tests_untouched(out)),
                    "-MIN, |MIN| and 1/MIN overflow", least, least);

    // MIN / -1 does not fit; MIN / -2 is 2^(w - 2)
    D_TESTS_EXPECT( (d_rational_make(&out, D_TESTS_LEAST, -1) ==
                     D_RATIONAL_OVERFLOW) &&
                    (d_tests_untouched(out)) &&
                    (d_rational_make(&out, D_TESTS_LEAST, -2) ==
                     D_RATIONAL_OK) &&
                    (out.num == (D_TESTS_MOST / 2) + 1) &&
                    (out.den == 1),
                    "MIN / -1 overflows; MIN / -2 is 2^(w-2)", least, least);

    return;
}

// d_tests_small
//   every numerator over every denominator in [-12, 12]: make, and the
// one-form test against the harness's own; then every rational among them,
// on its own and with each.
static
void d_tests_small(void)
{
    struct dj_rational values[25 * 12];
    int               count = 0;
    dj_math_imax       num   = 0;
    dj_math_imax       den   = 0;
    int               i     = 0;
    int               j     = 0;

    // 0 and the negative denominators included
    for (den = -12; den <= 12; ++den)
    {
        for (num = -12; num <= 12; ++num)
        {
            const struct dj_rational r = { num, den };

            D_TESTS_EXPECT(d_rational_is_valid(r) == d_tests_in_one_form(r),
                           "is_valid", r, r);
            d_tests_check_make(num, den);

            // the operands: those in the one form, so each value once
            if (d_tests_in_one_form(r))
            {
                values[count] = r;
                ++count;
            }
        }
    }

    // each on its own, and with each
    for (i = 0; i < count; ++i)
    {
        d_tests_check_unary(values[i]);
        d_tests_check_to_float(values[i]);

        for (j = 0; j < count; ++j)
        {
            d_tests_check_pair(values[i], values[j]);
        }
    }

    return;
}

// d_tests_extremes
//   the family's ends, their neighbours and the values around 2^(w/2): each
// on its own and with each; and make on each numerator over each
// denominator, either sign, and over each numerator.
static
void d_tests_extremes(void)
{
    // each in the one form
    static const dj_math_imax terms[][2] = {
        // small values
        { 0, 1 },  { 1, 1 },  { -1, 1 },  { 2, 1 },  { -2, 1 },
        { 1, 2 },  { -1, 2 }, { 1, 3 },   { 1, 6 },
        // the ends and their neighbours, as numerators and as denominators
        { D_TESTS_MOST,      1                },
        { -D_TESTS_MOST,     1                },
        { D_TESTS_LEAST,     1                },
        { D_TESTS_LEAST + 1, 2                },
        { D_TESTS_MOST - 1,  1                },
        { 1,                 D_TESTS_MOST     },
        { -1,                D_TESTS_MOST     },
        { D_TESTS_MOST - 1,  D_TESTS_MOST     },
        { D_TESTS_MOST,      D_TESTS_MOST - 1 },
        { D_TESTS_LEAST,     D_TESTS_MOST     },
        { D_TESTS_MOST,      3                },
        { D_TESTS_MOST,      6                },
        { -D_TESTS_MOST,     6                },
        // around 2^(w/2), where the product of two terms passes a word
        { D_TESTS_HALF,      3                },
        { D_TESTS_HALF + 1,  D_TESTS_HALF - 1 },
        { 3,                 D_TESTS_HALF     },
        { 1,                 D_TESTS_HALF / 2 },
        { 1,                 D_TESTS_HALF - 1 },
        { -1,                D_TESTS_HALF + 1 }
    };
    const int count = (int)(sizeof(terms) / sizeof(terms[0]));

    int i = 0;
    int j = 0;

    // each on its own, and with each
    for (i = 0; i < count; ++i)
    {
        const struct dj_rational a = { terms[i][0], terms[i][1] };

        d_tests_check_unary(a);
        d_tests_check_to_float(a);

        for (j = 0; j < count; ++j)
        {
            const struct dj_rational b = { terms[j][0], terms[j][1] };

            d_tests_check_pair(a, b);
            d_tests_check_make(terms[i][0], terms[j][1]);
            d_tests_check_make(terms[i][0], -terms[j][1]);
            d_tests_check_make(terms[i][0], terms[j][0]);
        }
    }

    return;
}

// d_tests_random
//   D_TESTS_TRIALS pairs drawn at random, and each first term on its own;
// then, that the draws reach both answers often: a tenth of the sums and
// products at least stored, and as many overflowed.
static
void d_tests_random(void)
{
    const long              stored_before     = g_stored;
    const long              overflowed_before = g_overflowed;
    const struct dj_rational none              = { 0, 1 };

    long trial = 0;

    // each pair, and its first term on its own
    for (trial = 0; trial < D_TESTS_TRIALS; ++trial)
    {
        const struct dj_rational a = d_tests_random_rational();
        const struct dj_rational b = d_tests_random_rational();

        d_tests_check_unary(a);
        d_tests_check_to_float(a);
        d_tests_check_pair(a, b);
    }

    D_TESTS_EXPECT( ((g_stored - stored_before) * 10 >=
                     4 * D_TESTS_TRIALS) &&
                    ((g_overflowed - overflowed_before) * 10 >=
                     4 * D_TESTS_TRIALS),
                    "the random pairs reach both answers", none, none);

    return;
}


//==============================================================================
// 7.  CONSTANT EVALUATION
//==============================================================================
// From C++14 every kernel is a constant expression, the ones that store
// through a pointer included, where it points at the evaluation's own
// object.

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// d_tests_constexpr_sum
//   1/3 + 1/6.
static constexpr
struct dj_rational d_tests_constexpr_sum()
{
    struct dj_rational out   = { 0, 1 };
    struct dj_rational third = { 1, 3 };
    struct dj_rational sixth = { 1, 6 };

    (void)d_rational_add(&out, third, sixth);

    return out;
}

// d_tests_constexpr_wide
//   MAX/3 + MAX/6, whose cross term, 3 MAX, passes a word.
static constexpr
struct dj_rational d_tests_constexpr_wide()
{
    struct dj_rational out    = { 0, 1 };
    struct dj_rational most_3 = { 0, 1 };
    struct dj_rational most_6 = { 0, 1 };

    (void)d_rational_make(&most_3, D_TESTS_MOST, 3);
    (void)d_rational_make(&most_6, D_TESTS_MOST, 6);
    (void)d_rational_add(&out, most_3, most_6);

    return out;
}

// d_tests_constexpr_order
//   MAX/(MAX - 1) against (MAX - 1)/(MAX - 2), by cross products of two
// words.
static constexpr
int d_tests_constexpr_order()
{
    struct dj_rational a = { 0, 1 };
    struct dj_rational b = { 0, 1 };

    (void)d_rational_make(&a, D_TESTS_MOST, D_TESTS_MOST - 1);
    (void)d_rational_make(&b, D_TESTS_MOST - 1, D_TESTS_MOST - 2);

    return d_rational_cmp(a, b);
}

// d_tests_constexpr_overflow
//   -MIN's status.
static constexpr enum dj_rational_status
d_tests_constexpr_overflow()
{
    struct dj_rational out = { 0, 1 };

    return d_rational_neg(&out, d_rational_from_int(D_TESTS_LEAST));
}

// d_tests_constexpr_third_f, d_tests_constexpr_third_d
//   1/3 to the nearest float and double.
static constexpr
float d_tests_constexpr_third_f()
{
    struct dj_rational third = { 1, 3 };

    return d_rational_to_f(third);
}

static constexpr
double d_tests_constexpr_third_d()
{
    struct dj_rational third = { 1, 3 };

    return d_rational_to_d(third);
}

// d_tests_constexpr_wide_ld
//   -(3 / 4), a two-word ratio, to long double.
static constexpr
long double d_tests_constexpr_wide_ld()
{
    struct dj_internal_rational_wide three = { 0u, 3u };
    struct dj_internal_rational_wide four  = { 0u, 4u };

    return d_internal_rational_wide_to_ld(true, three, four);
}

static_assert( (d_tests_constexpr_sum().num == 1) &&
               (d_tests_constexpr_sum().den == 2),
               "1/3 + 1/6 is 1/2 as a constant expression");
static_assert( (d_tests_constexpr_wide().num == D_TESTS_MOST) &&
               (d_tests_constexpr_wide().den == 2),
               "MAX/3 + MAX/6 is MAX/2 as a constant expression");
static_assert(d_tests_constexpr_order() == -1,
              "a comparison in double width as a constant expression");
static_assert(d_tests_constexpr_overflow() == D_RATIONAL_OVERFLOW,
              "an overflow as a constant expression");
static_assert(d_tests_constexpr_third_f() == 0.333333343f,
              "1/3 to the nearest float as a constant expression");
static_assert(d_tests_constexpr_third_d() == 0.33333333333333331,
              "1/3 to the nearest double as a constant expression");
static_assert(d_tests_constexpr_wide_ld() == -0.75L,
              "a two-word ratio to long double as a constant expression");

// d_tests_constant_evaluation
//   the same at run time.
static
void d_tests_constant_evaluation(void)
{
    const struct dj_rational sum  = d_tests_constexpr_sum();
    const struct dj_rational wide = d_tests_constexpr_wide();

    D_TESTS_EXPECT( (sum.num == 1) &&
                    (sum.den == 2) &&
                    (wide.num == D_TESTS_MOST) &&
                    (wide.den == 2) &&
                    (d_tests_constexpr_order() == -1) &&
                    (d_tests_constexpr_overflow() == D_RATIONAL_OVERFLOW) &&
                    (d_tests_constexpr_third_f() == 0.333333343f) &&
                    (d_tests_constexpr_third_d() == 0.33333333333333331) &&
                    (d_tests_constexpr_wide_ld() == -0.75L),
                    "the constant expressions, at run time", sum, wide);

    return;
}

#endif  // C++14


int main(void)
{
    const struct dj_rational none = { 0, 1 };

    d_tests_known();
    d_tests_wide();
    d_tests_to_float_known();
    d_tests_to_float_wide();
    d_tests_small();
    d_tests_extremes();
    d_tests_random();
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif

    // the reference never needed more than its bits
    D_TESTS_EXPECT(!g_reference_overrun, "the reference's 256 bits held",
                   none, none);

    printf("t_rational: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
