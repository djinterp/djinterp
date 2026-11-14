/*******************************************************************************
* djinterp [c/math]                                               t_value_wide.c
*
*   Conformance harness for the wide integer as a boxed kind
* (c/math/value_wide.h): its table, its arithmetic with itself and with every
* inline number, the long double it keeps where an exact result does not fit
* (G2), its conversions, its text, and its storage.
*   The reference is math_tests_exact.h's, at 768 bits: exact rationals over
* signed integers in 16-bit limbs, Stein's greatest common divisor, and an
* oracle that finds whether a value is the one of its type nearest a ratio by
* comparing distances exactly. None of it is the unit's code.
*   An operation of the kind's must give the exact result as the reference
* reduces and sorts it: an integer within 256 bits as a wide integer, in a
* box of its own under the table of the operand of the kind; a rational that
* is no integer, its reduced terms within the family, as the inline
* rational; and anything else as the long double nearest it, ties to even,
* flagged inexact. A quotient over exact 0 is a pole, or invalid for 0 / 0.
* Every result carries its operands' flags. A comparison must give the exact
* order. A float, double or long double meeting a wide integer must give
* what value.h gives on the integer converted to its type; a conversion, the
* nearest value, with its direction, and past the largest float an infinity;
* and to an exact kind, the integer where it fits the family. Text must be
* the reference's digits, cut short as snprintf cuts. An error passes
* through, and a boolean or none is a domain error, or no order.
*   Storage: the harness sets the kind's allocation hooks, so that every box
* is counted, and one is refused where it asks. An operation must leave its
* operands' holders as they were, and its result must hold one new box where
* it is the kind's and none otherwise; a refused box must give a memory
* error and leave nothing behind; and once the harness has given everything
* back, no box may be left.
*   Three kinds of the harness's own stand below the wide integer: "small",
* an exact kind at 150 that holds an inline integer or refuses to convert, as
* a value past the family would; "blur", an inexact one at 130; and "token",
* at 120, no number. The wide integer must read a small through its table,
* once, and refuse the others without asking them anything.
*   The operands: wide integers about the powers of two that matter -- the
* limbs', the words', the family's ends, the types' precisions and their
* ties, the float's range, the top -- and products of small primes, both
* signs, some flagged; inline integers and rationals about the family's ends;
* floating-point values about 2^256, zeros, infinities and NaN; errors,
* booleans and none; and the harness's kinds. Each wide integer alone, every
* pair with a wide integer among them, for the four operations and the
* comparison, and storage refused for each; then pairs drawn at random. The
* rounding of a ratio is checked on its own too, at every precision a long
* double has, at the extremes of 512-bit terms, at ties and just beside them.
* So is the table: a second copy of it stands for another translation
* unit's; its entries are called with what value.h never gives them; and its
* exact arithmetic, and its reading of an operand, are given buffers already
* written, which each must write whole, or where it fails, not at all. From
* C++14, the exact arithmetic and the rounding are checked as constant
* expressions as well.
*   The same file compiles as C++ (-x c++), at any level from C++98.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_value_wide.c -o t_value_wide -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_value_wide.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.11.07
*******************************************************************************/
// std
#include <float.h>   // FLT_MANT_DIG ... LDBL_MAX
#include <math.h>    // frexpl, HUGE_VAL
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // printf, sprintf
#include <stdlib.h>  // malloc, free
#include <string.h>  // memset, strcmp, strlen

// the kind's storage, through the harness's hooks (section 1)
static void*
d_tests_allocate(size_t _size);
static void
d_tests_give_back(void* _block);

#define D_CFG_MATH_VALUE_WIDE_MALLOC(_size)  d_tests_allocate(_size)
#define D_CFG_MATH_VALUE_WIDE_FREE(_ptr)     d_tests_give_back(_ptr)

// the reference at 768 bits: past the product of two wide integers, and past
// every distance its oracle measures for a ratio of 512-bit terms
#define D_TESTS_EXACT_LIMBS  48

// djinterp
#include "../../../../../inc/djinterp/c/modules/math/value_wide.h"  // unit under test
// tests
#include "./math_tests_exact.h"  // the exact reference


// the unit's limb and integer, as the harness counts them; the reference's
// limbs in one of the unit's; and an exact result's limbs
#define D_TESTS_H       ((int)D_MATH_WIDE_LIMB_BITS)
#define D_TESTS_N       ((int)D_MATH_WIDE_LIMBS)
#define D_TESTS_PIECES  (D_TESTS_H / 16)
#define D_TESTS_RATIO   ((int)D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS)

// the family's ends, and the bits in a word
#define D_TESTS_MOST   D_MATH_IMAX_MAX
#define D_TESTS_LEAST  D_MATH_IMAX_MIN
#define D_TESTS_W      ((int)D_INTERNAL_RATIONAL_WORD_BITS)

// the wide integer's rank and name, as the harness states them
#define D_TESTS_RANK_WIDE  210
#define D_TESTS_NAME_WIDE  "wide integer"

// the room for a value's text
#define D_TESTS_TEXT_ROOM  400

// the pairs and the ratios drawn at random
#define D_TESTS_TRIALS        1500L
#define D_TESTS_ROUND_TRIALS  600L

// the operands' lists
#define D_TESTS_LIST_MAX  128

// the harness's kinds, as it numbers them, and their ranks
#define D_TESTS_SMALL  0
#define D_TESTS_BLUR   1
#define D_TESTS_TOKEN  2
#define D_TESTS_KINDS  3

#define D_TESTS_RANK_SMALL  150
#define D_TESTS_RANK_BLUR   130
#define D_TESTS_RANK_TOKEN  120

// a table's entries, as the harness counts the calls into its kinds'
#define D_TESTS_RETAIN   0
#define D_TESTS_RELEASE  1
#define D_TESTS_UNARY    2
#define D_TESTS_BINARY   3
#define D_TESTS_CMP      4
#define D_TESTS_CONVERT  5
#define D_TESTS_TEXT     6
#define D_TESTS_ENTRIES  7

static long          g_checks   = 0;
static long          g_failures = 0;
static unsigned long g_state    = 2718281UL;

// the kind's boxes: held now, made, and refused; and the allocations to
// come before one is refused, -1 for none
static long g_live    = 0;
static long g_made    = 0;
static long g_refused = 0;
static long g_refuse  = -1;

// the harness's kinds: their boxes held, and the calls into their entries
static long g_kind_live = 0;
static long g_calls[D_TESTS_KINDS][D_TESTS_ENTRIES];

// the result a failure report shows beside its operands
static struct dj_math_value g_shown;


//==============================================================================
// 1.  STORAGE
//==============================================================================
// The kind's boxes come from d_tests_allocate and go back through
// d_tests_give_back, which count them. An allocation is refused where
// g_refuse has come down to 0.

static void*
d_tests_allocate(
    size_t _size
)
{
    void* block = NULL;

    // refused on purpose
    if (g_refuse == 0L)
    {
        g_refuse = -1L;
        ++g_refused;

        return NULL;
    }

    if (g_refuse > 0L)
    {
        --g_refuse;
    }

    block = malloc(_size);

    // made, unless the system refused it
    if (block != NULL)
    {
        ++g_live;
        ++g_made;
    }

    return block;
}

static void
d_tests_give_back(
    void* _block
)
{
    // a box: one fewer held
    if (_block != NULL)
    {
        --g_live;
    }

    free(_block);

    return;
}


//==============================================================================
// 2.  THE HARNESS'S KINDS
//==============================================================================
// Three kinds below the wide integer. A small holds an inline integer and
// gives it to an exact conversion, unless it refuses, as a value past the
// family would; a blur holds a long double and is inexact; a token holds
// nothing and is no number. Their arithmetic and comparisons are never asked
// for here, the wide integer being above them all; each counts the calls
// into its entries.

// dj_tests_box
//   struct: a box of one of the harness's kinds.
struct dj_tests_box
{
    long                 refs;     // its holders
    int                  kind;     // D_TESTS_SMALL ... D_TESTS_TOKEN
    bool                 refuses;  // a small that converts to nothing
    struct dj_math_value held;     // a small's integer, a blur's long double
};

// the entries, defined below
static void
d_tests_kind_retain(void* _box);
static void
d_tests_kind_release(void* _box);
static struct dj_math_value
d_tests_kind_unary(enum dj_math_value_op _op,
                   struct dj_math_value  _x);
static struct dj_math_value
d_tests_kind_binary(enum dj_math_value_op _op,
                    struct dj_math_value _a,
                    struct dj_math_value _b);
static enum dj_math_value_order
d_tests_kind_cmp(struct dj_math_value _a,
                 struct dj_math_value _b);
static bool
d_tests_kind_convert(struct dj_math_value*   _out,
                     int*                    _direction,
                     struct dj_math_value    _x,
                     enum dj_math_value_kind _to);
static size_t
d_tests_kind_text(char*               _buffer,
                  size_t               _size,
                  struct dj_math_value _x);

static const struct dj_math_boxed_kind g_kind_small = {
    "small", D_TESTS_RANK_SMALL, D_MATH_BOXED_NUMBER | D_MATH_BOXED_EXACT,
    d_tests_kind_retain, d_tests_kind_release, d_tests_kind_unary,
    d_tests_kind_binary, d_tests_kind_cmp, d_tests_kind_convert,
    d_tests_kind_text, NULL
};

static const struct dj_math_boxed_kind g_kind_blur = {
    "blur", D_TESTS_RANK_BLUR, D_MATH_BOXED_NUMBER,
    d_tests_kind_retain, d_tests_kind_release, d_tests_kind_unary,
    d_tests_kind_binary, d_tests_kind_cmp, d_tests_kind_convert,
    d_tests_kind_text, NULL
};

static const struct dj_math_boxed_kind g_kind_token = {
    "token", D_TESTS_RANK_TOKEN, 0u,
    d_tests_kind_retain, d_tests_kind_release, d_tests_kind_unary,
    d_tests_kind_binary, d_tests_kind_cmp, d_tests_kind_convert,
    d_tests_kind_text, NULL
};

static const struct dj_math_boxed_kind* const g_tables[D_TESTS_KINDS] = {
    &g_kind_small, &g_kind_blur, &g_kind_token
};

// d_tests_kind_of
//   which of the harness's kinds _x is, known by its rank; -1 for anything
// else.
static int
d_tests_kind_of(
    struct dj_math_value _x
)
{
    int k = 0;

    // not boxed, or not whole
    if ( (_x.kind != D_MATH_VALUE_BOXED) ||
         (_x.type == NULL) ||
         (_x.box == NULL) )
    {
        return -1;
    }

    for (k = 0; k < D_TESTS_KINDS; ++k)
    {
        if (_x.type->rank == g_tables[k]->rank)
        {
            return k;
        }
    }

    return -1;
}

// d_tests_new_box
//   a value of the harness's kind _kind, holding _held, refusing to convert
// where _refuses, with one holder; none where the system has no storage.
static struct dj_math_value
d_tests_new_box(
    int                  _kind,
    struct dj_math_value _held,
    bool                 _refuses
)
{
    struct dj_math_value result = d_math_value_none();
    struct dj_tests_box* box    =
        (struct dj_tests_box*)malloc(sizeof(struct dj_tests_box));

    // no storage
    if (box == NULL)
    {
        return result;
    }

    box->refs    = 1L;
    box->kind    = _kind;
    box->refuses = _refuses;
    box->held    = _held;
    ++g_kind_live;

    result.kind = D_MATH_VALUE_BOXED;
    result.type = g_tables[_kind];
    result.box  = box;

    return result;
}

static void
d_tests_kind_retain(
    void* _box
)
{
    struct dj_tests_box* box = (struct dj_tests_box*)_box;

    ++g_calls[box->kind][D_TESTS_RETAIN];
    ++box->refs;

    return;
}

static void
d_tests_kind_release(
    void* _box
)
{
    struct dj_tests_box* box = (struct dj_tests_box*)_box;

    ++g_calls[box->kind][D_TESTS_RELEASE];
    --box->refs;

    // the last holder
    if (box->refs == 0L)
    {
        free(box);
        --g_kind_live;
    }

    return;
}

// d_tests_kind_unary, d_tests_kind_binary, d_tests_kind_cmp
//   never asked for here: counted, and a domain error, or no order.
static struct dj_math_value
d_tests_kind_unary(
    enum dj_math_value_op _op,
    struct dj_math_value  _x
)
{
    (void)_op;
    ++g_calls[((const struct dj_tests_box*)_x.box)->kind][D_TESTS_UNARY];

    return d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
}

static struct dj_math_value
d_tests_kind_binary(
    enum dj_math_value_op _op,
    struct dj_math_value  _a,
    struct dj_math_value  _b
)
{
    const int kind = (d_tests_kind_of(_a) >= 0) ? d_tests_kind_of(_a)
                                                : d_tests_kind_of(_b);

    (void)_op;

    if (kind >= 0)
    {
        ++g_calls[kind][D_TESTS_BINARY];
    }

    return d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
}

static enum dj_math_value_order
d_tests_kind_cmp(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    const int kind = (d_tests_kind_of(_a) >= 0) ? d_tests_kind_of(_a)
                                                : d_tests_kind_of(_b);

    if (kind >= 0)
    {
        ++g_calls[kind][D_TESTS_CMP];
    }

    return D_MATH_VALUE_UNORDERED;
}

// d_tests_kind_convert
//   a small's integer, to an exact kind, unless it refuses; nothing else
// converts. A small that refuses writes an integer anyway, as value.h's
// contract leaves a kind free to, and says false.
static bool
d_tests_kind_convert(
    struct dj_math_value*   _out,
    int*                    _direction,
    struct dj_math_value    _x,
    enum dj_math_value_kind _to
)
{
    const struct dj_tests_box* box = (const struct dj_tests_box*)_x.box;

    ++g_calls[box->kind][D_TESTS_CONVERT];

    // a small that refuses: something written, and false
    if ( (box->kind == D_TESTS_SMALL) &&
         (box->refuses) )
    {
        *_out = d_math_value_from_int(-424242);

        return false;
    }

    // a small, to an exact kind
    if ( (box->kind != D_TESTS_SMALL) ||
         ( (_to != D_MATH_VALUE_INTEGER) &&
           (_to != D_MATH_VALUE_RATIONAL) ) )
    {
        return false;
    }

    *_out       = box->held;
    *_direction = 0;

    return true;
}

// d_tests_kind_text
//   the kind's name, as snprintf would put it.
static size_t
d_tests_kind_text(
    char*                _buffer,
    size_t               _size,
    struct dj_math_value _x
)
{
    const char*  name   = _x.type->name;
    const size_t length = strlen(name);

    ++g_calls[((const struct dj_tests_box*)_x.box)->kind][D_TESTS_TEXT];

    // room for some of it
    if (_size > 0u)
    {
        const size_t count = (length < (_size - 1u)) ? length : (_size - 1u);

        memcpy(_buffer, name, count);
        _buffer[count] = '\0';
    }

    return length;
}

// dj_tests_counts
//   struct: the calls into the harness's kinds at one moment.
struct dj_tests_counts
{
    long calls[D_TESTS_KINDS][D_TESTS_ENTRIES];  // as g_calls held them
};

static struct dj_tests_counts
d_tests_counted(void)
{
    struct dj_tests_counts result;

    memcpy(result.calls, g_calls, sizeof(g_calls));

    return result;
}

// d_tests_calls_are
//   whether, since _before, the only calls into the harness's kinds are
// _count calls into the conversion of the kind _kind; _kind -1 for none.
static bool
d_tests_calls_are(
    const struct dj_tests_counts* _before,
    int                           _kind,
    long                          _count
)
{
    int k = 0;
    int e = 0;

    for (k = 0; k < D_TESTS_KINDS; ++k)
    {
        for (e = 0; e < D_TESTS_ENTRIES; ++e)
        {
            const long want = ( (k == _kind) &&
                                (e == D_TESTS_CONVERT) ) ? _count : 0L;

            if ((g_calls[k][e] - _before->calls[k][e]) != want)
            {
                return false;
            }
        }
    }

    return true;
}


//==============================================================================
// 3.  THE REFERENCE'S VALUES
//==============================================================================


// 3.1    Wide integers
//------------------------------------------------------------------------------

// d_tests_is_wide
//   whether _x is a value of the wide kind, as the harness knows one: boxed,
// whole, and of the rank it states.
static bool
d_tests_is_wide(
    struct dj_math_value _x
)
{
    return ( (_x.kind == D_MATH_VALUE_BOXED) &&
             (_x.type != NULL) &&
             (_x.box != NULL) &&
             (_x.type->rank == D_TESTS_RANK_WIDE) );
}

// d_tests_held
//   the integer a value of the wide kind holds, read from its box.
static struct dj_math_wide
d_tests_held(
    struct dj_math_value _x
)
{
    return ((const struct dj_internal_math_value_wide_box*)_x.box)->value;
}

// d_tests_holders
//   the holders of _x's box, the wide kind's or the harness's; 0 for an
// inline value.
static long
d_tests_holders(
    struct dj_math_value _x
)
{
    // the harness's, the wide kind's, or none
    if (d_tests_kind_of(_x) >= 0)
    {
        return ((const struct dj_tests_box*)_x.box)->refs;
    }

    if (d_tests_is_wide(_x))
    {
        return (long)((const struct dj_internal_math_value_wide_box*)_x.box)
            ->holders;
    }

    return 0L;
}

// d_tests_drop
//   an operation's result given back, and none in its place. Such a result
// is inline or a wide integer; a boxed one of any other kind, or with no
// table or no box, which only a fault can make, is left as it is, since
// giving it back would call another kind's release on a box not its own,
// or call through nothing. Its box stays held, and the count of boxes shows
// it.
static void
d_tests_drop(
    struct dj_math_value* _x
)
{
    // inline, or the wide integer's
    if ( (_x->kind != D_MATH_VALUE_BOXED) ||
         (d_tests_is_wide(*_x)) )
    {
        d_math_value_release(_x);
    }

    *_x = d_math_value_none();

    return;
}

// d_tests_big_of_integer
//   _x, exactly: each of its limbs in pieces of 16 bits.
static struct dj_tests_big
d_tests_big_of_integer(
    struct dj_math_wide _x
)
{
    struct dj_tests_big result = { { 0UL }, 0 };
    int                 i      = 0;
    int                 k      = 0;

    for (i = 0; i < D_TESTS_N; ++i)
    {
        for (k = 0; k < D_TESTS_PIECES; ++k)
        {
            result.limb[(i * D_TESTS_PIECES) + k] =
                ((unsigned long)_x.limb[i] >> (16 * k)) & 0xFFFFUL;
        }
    }

    result.negative = ( (_x.negative) &&
                        (!d_tests_big_is_zero(result)) ) ? 1 : 0;

    return result;
}

// d_tests_integer_of_big
//   _x, below 2^256 in magnitude, as the unit's integer.
static struct dj_math_wide
d_tests_integer_of_big(
    struct dj_tests_big _x
)
{
    struct dj_math_wide result = { { 0u }, false };
    int                 i      = 0;
    int                 k      = 0;

    for (i = 0; i < D_TESTS_N; ++i)
    {
        for (k = 0; k < D_TESTS_PIECES; ++k)
        {
            result.limb[i] = (dj_math_wide_limb)(
                result.limb[i] |
                (dj_math_wide_limb)(_x.limb[(i * D_TESTS_PIECES) + k]
                                    << (16 * k)));
        }
    }

    result.negative = (_x.negative != 0);

    return result;
}

// d_tests_limbs_of_big
//   |_x| into _out's _n limbs of the unit's.
static void
d_tests_limbs_of_big(
    struct dj_tests_big _x,
    dj_math_wide_limb*  _out,
    int                 _n
)
{
    int i = 0;
    int k = 0;

    for (i = 0; i < _n; ++i)
    {
        _out[i] = 0u;

        for (k = 0; k < D_TESTS_PIECES; ++k)
        {
            _out[i] = (dj_math_wide_limb)(
                _out[i] |
                (dj_math_wide_limb)(_x.limb[(i * D_TESTS_PIECES) + k]
                                    << (16 * k)));
        }
    }

    return;
}

// d_tests_wide_in_one_form
//   whether _x is in the one form: no negative 0.
static bool
d_tests_wide_in_one_form(
    struct dj_math_wide _x
)
{
    int i = 0;

    for (i = 0; i < D_TESTS_N; ++i)
    {
        if (_x.limb[i] != 0u)
        {
            return true;
        }
    }

    return (!_x.negative);
}

// 3.2    The reference's naturals
//------------------------------------------------------------------------------

// d_tests_one
//   1.
static struct dj_tests_big
d_tests_one(void)
{
    return d_tests_big_of_umax(1u, false);
}

// d_tests_power
//   2^_k.
static struct dj_tests_big
d_tests_power(
    int _k
)
{
    return d_tests_big_shl(d_tests_one(), _k);
}

// d_tests_plus
//   _a + _k, _k of either sign and small.
static struct dj_tests_big
d_tests_plus(
    struct dj_tests_big _a,
    long                _k
)
{
    const struct dj_tests_big k = d_tests_big_of_umax(
        (dj_math_umax)((_k < 0L) ? -_k : _k), (_k < 0L));

    return d_tests_big_add(_a, k);
}

// d_tests_minus
//   _a - _b.
static struct dj_tests_big
d_tests_minus(
    struct dj_tests_big _a,
    struct dj_tests_big _b
)
{
    return d_tests_big_add(_a, d_tests_big_neg(_b));
}

// d_tests_raised
//   _base^_k.
static struct dj_tests_big
d_tests_raised(
    unsigned long _base,
    int           _k
)
{
    struct dj_tests_big result = d_tests_one();
    int                 i      = 0;

    for (i = 0; i < _k; ++i)
    {
        result = d_tests_big_mul(result,
                                 d_tests_big_of_umax((dj_math_umax)_base,
                                                     false));
    }

    return result;
}

// d_tests_big_text
//   _x in decimal, a minus sign first below 0, into _buffer (at least
// D_TESTS_TEXT_ROOM characters), returned: for each digit, the magnitude
// divided by ten a limb at a time, the highest first.
static const char*
d_tests_big_text(
    struct dj_tests_big _x,
    char*               _buffer
)
{
    char                digits[D_TESTS_TEXT_ROOM];
    struct dj_tests_big rest  = d_tests_big_abs(_x);
    int                 count = 0;
    int                 at    = 0;
    int                 i     = 0;

    // the digits, the lowest first
    do
    {
        unsigned long remainder = 0UL;

        for (i = D_TESTS_LIMBS - 1; i >= 0; --i)
        {
            const unsigned long part = (remainder << 16) | rest.limb[i];

            rest.limb[i] = part / 10UL;
            remainder    = part % 10UL;
        }

        digits[count] = (char)('0' + (int)remainder);
        ++count;
    } while ( (!d_tests_big_is_zero(rest)) &&
              (count < (D_TESTS_TEXT_ROOM - 2)) );

    // the sign, then the digits, the highest first
    if (_x.negative != 0)
    {
        _buffer[at] = '-';
        ++at;
    }

    while (count > 0)
    {
        --count;
        _buffer[at] = digits[count];
        ++at;
    }

    _buffer[at] = '\0';

    return _buffer;
}

// 3.3    Exact values
//------------------------------------------------------------------------------

// d_tests_value_exact
//   _x's exact value, into *_out, and true, for an operand the kind reads:
// a wide integer, an inline integer or rational, or a small that converts.
// False for anything else.
static bool
d_tests_value_exact(
    struct dj_math_value   _x,
    struct dj_tests_exact* _out
)
{
    struct dj_math_value x = _x;

    // a wide integer
    if (d_tests_is_wide(_x))
    {
        *_out = d_tests_exact_from(d_tests_big_of_integer(d_tests_held(_x)),
                                   d_tests_one());

        return true;
    }

    // a small: what it holds, unless it refuses
    if (d_tests_kind_of(_x) == D_TESTS_SMALL)
    {
        const struct dj_tests_box* box = (const struct dj_tests_box*)_x.box;

        if (box->refuses)
        {
            return false;
        }

        x = box->held;
    }

    // an inline exact number
    if ( (x.kind != D_MATH_VALUE_INTEGER) &&
         (x.kind != D_MATH_VALUE_RATIONAL) )
    {
        return false;
    }

    *_out = d_tests_exact_of(x.exact.num, x.exact.den);

    return true;
}

// d_tests_order_real
//   -1, 0 or 1 as the exact _x is below, equal to or above _f; 2 where _f is
// a NaN. Where |_f|'s binade lies past |_x|'s, or below it, the magnitudes
// are ordered by it; otherwise |n| 2^s against m d 2^(e + s), exactly.
static int
d_tests_order_real(
    struct dj_tests_exact _x,
    long double           _f
)
{
    const int x_sign = d_tests_big_is_zero(_x.n)
                           ? 0
                           : ((_x.n.negative != 0) ? -1 : 1);
    const int f_sign = (_f > 0.0L) ? 1 : ((_f < 0.0L) ? -1 : 0);
    const int x_bits = d_tests_big_bits(_x.n) - d_tests_big_bits(_x.d);

    int         exponent = 0;
    int         order    = 0;
    long double fraction = 0.0L;

    // NaN
    if (_f != _f)
    {
        return 2;
    }

    // a zero, or unlike signs
    if ( (x_sign == 0) ||
         (f_sign == 0) ||
         (x_sign != f_sign) )
    {
        return (x_sign < f_sign) ? -1 : ((x_sign > f_sign) ? 1 : 0);
    }

    // an infinity: past every exact number
    if ( (_f > LDBL_MAX) ||
         (_f < -LDBL_MAX) )
    {
        return (f_sign > 0) ? -1 : 1;
    }

    fraction = frexpl((_f < 0.0L) ? -_f : _f, &exponent);

    // |x| lies between 2^(x_bits - 1) and 2^(x_bits + 1), |f| between
    // 2^(exponent - 1) and 2^exponent
    if ((exponent - 1) >= (x_bits + 1))
    {
        order = -1;
    }
    else if (exponent <= (x_bits - 1))
    {
        order = 1;
    }
    else
    {
        const struct dj_tests_parts parts =
            d_tests_parts_of(fraction, exponent, LDBL_MANT_DIG);
        const int shift = (parts.exponent < 0) ? -parts.exponent : 0;

        order = d_tests_big_cmp_mag(
            d_tests_big_shl(d_tests_big_abs(_x.n), shift),
            d_tests_big_shl(d_tests_big_mul(parts.mantissa, _x.d),
                            parts.exponent + shift));
    }

    // the magnitudes' order, turned for two negatives
    order = (x_sign > 0) ? order : -order;

    return (order < 0) ? -1 : ((order > 0) ? 1 : 0);
}

// d_tests_order_of
//   the order -1, 0, 1 or 2 as value.h's order.
static enum dj_math_value_order
d_tests_order_of(
    int _order
)
{
    return (_order == 2) ? D_MATH_VALUE_UNORDERED
                         : ( (_order < 0) ? D_MATH_VALUE_LESS
                                          : ( (_order > 0)
                                                  ? D_MATH_VALUE_GREATER
                                                  : D_MATH_VALUE_EQUAL ) );
}


//==============================================================================
// 4.  CHECKS AND REPORTS
//==============================================================================


// 4.1    Reports
//------------------------------------------------------------------------------

// d_tests_value_text
//   _x for a report, into _buffer (at least 2 * D_TESTS_TEXT_ROOM): its
// kind and value, and its flags.
static const char*
d_tests_value_text(
    struct dj_math_value _x,
    char*                _buffer
)
{
    char num[24];
    char den[24];
    char big[D_TESTS_TEXT_ROOM];

    // by kind
    if (d_tests_is_wide(_x))
    {
        sprintf(_buffer, "wide %s",
                d_tests_big_text(d_tests_big_of_integer(d_tests_held(_x)),
                                 big));
    }
    else if (d_tests_kind_of(_x) >= 0)
    {
        sprintf(_buffer, "%s", _x.type->name);
    }
    else if (_x.kind == D_MATH_VALUE_BOXED)
    {
        sprintf(_buffer, "a boxed value of no kind the harness knows");
    }
    else if (_x.kind == D_MATH_VALUE_NONE)
    {
        sprintf(_buffer, "none");
    }
    else if (_x.kind == D_MATH_VALUE_ERROR)
    {
        sprintf(_buffer, "error %d", _x.code);
    }
    else if (_x.kind == D_MATH_VALUE_BOOLEAN)
    {
        sprintf(_buffer, "%s", (_x.code != 0) ? "true" : "false");
    }
    else if (_x.kind == D_MATH_VALUE_INTEGER)
    {
        sprintf(_buffer, "integer %s", d_tests_text(_x.exact.num, num));
    }
    else if (_x.kind == D_MATH_VALUE_RATIONAL)
    {
        sprintf(_buffer, "rational %s/%s", d_tests_text(_x.exact.num, num),
                d_tests_text(_x.exact.den, den));
    }
    else
    {
        sprintf(_buffer, "%s %.21Lg", d_math_value_kind_name(_x), _x.real);
    }

    // the flags, where there are any
    if (_x.flags != 0u)
    {
        sprintf(_buffer + strlen(_buffer), " (flags %u)", _x.flags);
    }

    return _buffer;
}

// d_tests_expect
//   counts a check, and reports the first failures with their operands and
// the result last shown.
static void
d_tests_expect(
    bool                 _ok,
    const char*          _what,
    struct dj_math_value _a,
    struct dj_math_value _b,
    int                  _line
)
{
    char text[3][2 * D_TESTS_TEXT_ROOM];

    ++g_checks;

    // the first failures, with their operands
    if (!_ok)
    {
        ++g_failures;

        if (g_failures <= 40L)
        {
            printf("  FAIL line %d: %s (%s; %s) gave %s\n", _line, _what,
                   d_tests_value_text(_a, text[0]),
                   d_tests_value_text(_b, text[1]),
                   d_tests_value_text(g_shown, text[2]));
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what, a, b)                                         \
    d_tests_expect((ok), (what), (a), (b), __LINE__)

// d_tests_expect_detail
//   counts a check, and reports the first failures with a text of their own.
static void
d_tests_expect_detail(
    bool        _ok,
    const char* _what,
    const char* _detail,
    int         _line
)
{
    ++g_checks;

    // the first failures
    if (!_ok)
    {
        ++g_failures;

        if (g_failures <= 40L)
        {
            printf("  FAIL line %d: %s (%s)\n", _line, _what, _detail);
        }
    }

    return;
}

#define D_TESTS_EXPECT_DETAIL(ok, what, detail)                                \
    d_tests_expect_detail((ok), (what), (detail), __LINE__)

// 4.2    Values
//------------------------------------------------------------------------------

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

// d_tests_same_real
//   whether _a and _b are the same floating-point value: both NaN, or equal
// with the same sign, -0 and +0 told apart.
static bool
d_tests_same_real(
    long double _a,
    long double _b
)
{
    // NaN
    if ( (_a != _a) ||
         (_b != _b) )
    {
        return ( (_a != _a) &&
                 (_b != _b) );
    }

    // a zero: the same sign too
    if ( (_a == 0.0L) &&
         (_b == 0.0L) )
    {
        return (d_tests_is_plus_zero(_a) == d_tests_is_plus_zero(_b));
    }

    return (_a == _b);
}

// d_tests_same
//   whether _a and _b, inline, are the same value: the same kind, flags and
// fields, and no table or box.
static bool
d_tests_same(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    return ( (_a.kind == _b.kind) &&
             (_a.kind != D_MATH_VALUE_BOXED) &&
             (_a.flags == _b.flags) &&
             (_a.code == _b.code) &&
             (_a.exact.num == _b.exact.num) &&
             (_a.exact.den == _b.exact.den) &&
             (d_tests_same_real(_a.real, _b.real)) &&
             (_a.type == NULL) &&
             (_b.type == NULL) &&
             (_a.box == NULL) &&
             (_b.box == NULL) );
}

// d_tests_is_fresh
//   whether _r is a value of the wide kind under the table _type, with
// _flags, its other fields 0, in a box of its own with one holder, its
// integer in the one form.
static bool
d_tests_is_fresh(
    struct dj_math_value             _r,
    const struct dj_math_boxed_kind* _type,
    unsigned                         _flags
)
{
    return ( (d_tests_is_wide(_r)) &&
             (_r.type == _type) &&
             (_r.flags == _flags) &&
             (_r.code == 0) &&
             (_r.exact.num == 0) &&
             (_r.exact.den == 1) &&
             (d_tests_is_plus_zero(_r.real)) &&
             (d_tests_holders(_r) == 1L) &&
             (d_tests_wide_in_one_form(d_tests_held(_r))) );
}

// d_tests_is_error
//   whether _r is the error _code, with _flags, its other fields 0.
static bool
d_tests_is_error(
    struct dj_math_value     _r,
    enum dj_math_value_error _code,
    unsigned                 _flags
)
{
    struct dj_math_value want = d_math_value_from_error(_code);

    want.flags = _flags;

    return d_tests_same(_r, want);
}

// d_tests_is_float_kind
//   whether _x is an inline float, double or long double.
static bool
d_tests_is_float_kind(
    struct dj_math_value _x
)
{
    return ( (_x.kind == D_MATH_VALUE_FLOAT) ||
             (_x.kind == D_MATH_VALUE_DOUBLE) ||
             (_x.kind == D_MATH_VALUE_LONG_DOUBLE) );
}

// d_tests_is_other
//   whether _x is none, an error or a boolean.
static bool
d_tests_is_other(
    struct dj_math_value _x
)
{
    return ( (_x.kind == D_MATH_VALUE_NONE) ||
             (_x.kind == D_MATH_VALUE_ERROR) ||
             (_x.kind == D_MATH_VALUE_BOOLEAN) );
}


//==============================================================================
// 5.  WHAT A RESULT MUST BE
//==============================================================================


// 5.1    The exact result
//------------------------------------------------------------------------------

// dj_tests_outcome
//   enum: what an operation of the kind's must give.
enum dj_tests_outcome
{
    D_TESTS_BOXED    = 0,  // a wide integer, in a box of its own
    D_TESTS_RATIONAL = 1,  // an inline rational
    D_TESTS_NEAREST  = 2,  // the nearest long double, flagged inexact
    D_TESTS_POLE     = 3,  // a pole
    D_TESTS_INVALID  = 4   // invalid: 0 / 0
};

// dj_tests_expected
//   struct: an operation's outcome and its exact value, not reduced.
struct dj_tests_expected
{
    enum dj_tests_outcome outcome;  // what the result must be
    struct dj_tests_exact value;    // its exact value
};

// d_tests_expected_of
//   what _a _op _b must give: its exact value, as the reference computes it,
// and its outcome, sorted by its reduced form. An integer is one whose
// denominator divides its numerator, which is where their greatest common
// divisor is the denominator; it fits 256 bits where |n| < d 2^256.
static struct dj_tests_expected
d_tests_expected_of(
    enum dj_math_value_op _op,
    struct dj_tests_exact _a,
    struct dj_tests_exact _b
)
{
    struct dj_tests_expected result = { D_TESTS_BOXED, _a };
    bool                     whole  = false;

    // the exact value; over 0, a pole or invalid
    if (_op == D_MATH_VALUE_OP_ADD)
    {
        result.value = d_tests_exact_add(_a, _b);
    }
    else if (_op == D_MATH_VALUE_OP_SUB)
    {
        result.value = d_tests_exact_add(_a, d_tests_exact_neg(_b));
    }
    else if (_op == D_MATH_VALUE_OP_MUL)
    {
        result.value = d_tests_exact_mul(_a, _b);
    }
    else if (d_tests_big_is_zero(_b.n))
    {
        result.outcome = d_tests_big_is_zero(_a.n) ? D_TESTS_INVALID
                                                   : D_TESTS_POLE;

        return result;
    }
    else
    {
        result.value = d_tests_exact_mul(_a, d_tests_exact_inv(_b));
    }

    // 0
    if (d_tests_big_is_zero(result.value.n))
    {
        return result;
    }

    whole = ( (d_tests_big_cmp_mag(result.value.d, d_tests_one()) == 0) ||
              (d_tests_big_cmp_mag(d_tests_big_gcd(result.value.n,
                                                   result.value.d),
                                   result.value.d) == 0) );

    // an integer, within 256 bits or not; a rational, in the family or not
    if (whole)
    {
        result.outcome =
            (d_tests_big_cmp_mag(result.value.n,
                                 d_tests_big_shl(result.value.d, 256)) < 0)
                ? D_TESTS_BOXED
                : D_TESTS_NEAREST;
    }
    else
    {
        result.outcome = d_tests_exact_fits(result.value) ? D_TESTS_RATIONAL
                                                          : D_TESTS_NEAREST;
    }

    return result;
}

// d_tests_is_result
//   whether _r is what _want says, with the operands' flags _flags, a box
// of the kind's made under the table _type.
static bool
d_tests_is_result(
    struct dj_math_value             _r,
    struct dj_tests_expected         _want,
    unsigned                         _flags,
    const struct dj_math_boxed_kind* _type
)
{
    struct dj_math_value blank = d_math_value_none();

    // by outcome
    switch (_want.outcome)
    {
        case D_TESTS_BOXED:
            return ( (d_tests_is_fresh(_r, _type, _flags)) &&
                     (d_tests_exact_cmp(
                          d_tests_exact_from(
                              d_tests_big_of_integer(d_tests_held(_r)),
                              d_tests_one()),
                          _want.value) == 0) );

        case D_TESTS_RATIONAL:
            blank.kind  = D_MATH_VALUE_RATIONAL;
            blank.flags = _flags;
            blank.exact = _r.exact;

            return ( (d_tests_same(_r, blank)) &&
                     (_r.exact.den != 1) &&
                     (d_tests_in_one_form(_r.exact)) &&
                     (d_tests_exact_cmp(d_tests_exact_of(_r.exact.num,
                                                         _r.exact.den),
                                        _want.value) == 0) );

        case D_TESTS_NEAREST:
            blank.kind  = D_MATH_VALUE_LONG_DOUBLE;
            blank.flags = _flags | D_MATH_VALUE_FLAG_INEXACT;
            blank.real  = _r.real;

            return ( (d_tests_same(_r, blank)) &&
                     (d_tests_nearest_ld((_want.value.n.negative != 0),
                                         d_tests_big_abs(_want.value.n),
                                         _want.value.d, _r.real)) );

        case D_TESTS_POLE:
            return d_tests_is_error(_r, D_MATH_VALUE_ERROR_POLE, _flags);

        default:
            return d_tests_is_error(_r, D_MATH_VALUE_ERROR_INVALID, _flags);
    }
}

// 5.2    Operations
//------------------------------------------------------------------------------

// d_tests_apply
//   value.h's _a _op _b.
static struct dj_math_value
d_tests_apply(
    enum dj_math_value_op _op,
    struct dj_math_value  _a,
    struct dj_math_value  _b
)
{
    // the operation
    switch (_op)
    {
        case D_MATH_VALUE_OP_ADD:
            return d_math_value_add(_a, _b);

        case D_MATH_VALUE_OP_SUB:
            return d_math_value_sub(_a, _b);

        case D_MATH_VALUE_OP_MUL:
            return d_math_value_mul(_a, _b);

        default:
            return d_math_value_div(_a, _b);
    }
}

// d_tests_converted
//   the wide integer _x as the inline number of kind _to, converted as the
// unit converts it; any other _x as it is.
static struct dj_math_value
d_tests_converted(
    struct dj_math_value    _x,
    enum dj_math_value_kind _to
)
{
    // not a wide integer
    if (!d_tests_is_wide(_x))
    {
        return _x;
    }

    // to the type
    if (_to == D_MATH_VALUE_FLOAT)
    {
        volatile float f = d_math_wide_to_f(d_tests_held(_x), NULL);

        return d_math_value_from_f(f);
    }

    if (_to == D_MATH_VALUE_DOUBLE)
    {
        volatile double d = d_math_wide_to_d(d_tests_held(_x), NULL);

        return d_math_value_from_d(d);
    }

    return d_math_value_from_ld(d_math_wide_to_ld(d_tests_held(_x), NULL));
}

// d_tests_operation_name
//   the operation's sign, for a report.
static const char*
d_tests_operation_name(
    enum dj_math_value_op _op
)
{
    static const char* const names[6] = {
        "a + b", "a - b", "a * b", "a / b", "-a", "|a|"
    };

    return names[(int)_op];
}


//==============================================================================
// 6.  THE OPERANDS
//==============================================================================
// The wide integers, in a list of their own; every other operand in a second.
// Each list holds its values, and gives them back at the end.


static struct dj_math_value g_wides[D_TESTS_LIST_MAX];
static int                  g_wide_count = 0;
static struct dj_math_value g_others[D_TESTS_LIST_MAX];
static int                  g_other_count = 0;

// d_tests_wide_value
//   the wide integer _x as a value, with _flags.
static struct dj_math_value
d_tests_wide_value(
    struct dj_tests_big _x,
    unsigned            _flags
)
{
    struct dj_math_value result =
        d_math_value_from_wide(d_tests_integer_of_big(_x));

    result.flags = _flags;

    return result;
}

// d_tests_add_wide
//   _x, and -_x, to the wide integers, with _flags.
static void
d_tests_add_wide(
    struct dj_tests_big _x,
    unsigned            _flags
)
{
    int sign = 0;

    for (sign = 0; sign < 2; ++sign)
    {
        // 0 once
        if ( (sign == 1) &&
             (d_tests_big_is_zero(_x)) )
        {
            break;
        }

        if (g_wide_count < D_TESTS_LIST_MAX)
        {
            g_wides[g_wide_count] = d_tests_wide_value(
                (sign == 0) ? _x : d_tests_big_neg(_x), _flags);
            D_TESTS_EXPECT(d_tests_is_wide(g_wides[g_wide_count]),
                           "an operand made", g_wides[g_wide_count],
                           d_math_value_none());
            ++g_wide_count;
        }
    }

    return;
}

// d_tests_add_other
//   _x to the other operands.
static void
d_tests_add_other(
    struct dj_math_value _x
)
{
    if (g_other_count < D_TESTS_LIST_MAX)
    {
        g_others[g_other_count] = _x;
        ++g_other_count;
    }

    return;
}

// d_tests_flagged
//   _x with _flags.
static struct dj_math_value
d_tests_flagged(
    struct dj_math_value _x,
    unsigned             _flags
)
{
    struct dj_math_value result = _x;

    result.flags = _flags;

    return result;
}

// d_tests_make_wides
//   the wide integers: about the powers of two that matter and products of
// small primes, both signs, and some flagged.
static void
d_tests_make_wides(void)
{
    static const long small[8] = { 0L, 1L, 2L, 3L, 5L, 7L, 10L, 255L };
    static const int  powers[13] = { 15, 16, 24, 31, 32, 53, 62, 63, 64,
                                     113, 127, 128, 255 };

    int i = 0;

    for (i = 0; i < 8; ++i)
    {
        d_tests_add_wide(d_tests_big_of_umax((dj_math_umax)small[i], false),
                         0u);
    }

    // each power of two, and one either side of it; the top one below 2^256
    for (i = 0; i < 13; ++i)
    {
        d_tests_add_wide(d_tests_power(powers[i]), 0u);
        d_tests_add_wide(d_tests_plus(d_tests_power(powers[i]), 1L), 0u);
        d_tests_add_wide(d_tests_plus(d_tests_power(powers[i]), -1L), 0u);
    }

    // the ties of 24, 53 and 64 bits, and the float's threshold for an
    // infinity, 2^128 - 2^103, and below it
    d_tests_add_wide(d_tests_plus(d_tests_power(24), 3L), 0u);
    d_tests_add_wide(d_tests_plus(d_tests_power(53), 3L), 0u);
    d_tests_add_wide(d_tests_plus(d_tests_power(64), 3L), 0u);
    d_tests_add_wide(d_tests_minus(d_tests_power(128), d_tests_power(103)),
                     0u);
    d_tests_add_wide(d_tests_plus(d_tests_minus(d_tests_power(128),
                                                d_tests_power(103)), -1L),
                     0u);

    // products of small primes, for the greatest common divisors
    d_tests_add_wide(d_tests_raised(6UL, 99), 0u);
    d_tests_add_wide(d_tests_raised(3UL, 161), 0u);
    d_tests_add_wide(d_tests_raised(10UL, 76), 0u);
    d_tests_add_wide(d_tests_raised(6UL, 50), 0u);

    // the top
    d_tests_add_wide(d_tests_plus(d_tests_power(256), -1L), 0u);
    d_tests_add_wide(d_tests_minus(d_tests_power(256), d_tests_power(192)),
                     0u);
    d_tests_add_wide(d_tests_plus(d_tests_power(255), -19L), 0u);

    // flagged
    d_tests_add_wide(d_tests_big_of_umax(12u, false),
                     D_MATH_VALUE_FLAG_INEXACT);
    d_tests_add_wide(d_tests_power(200), D_MATH_VALUE_FLAG_INEXACT);

    return;
}

// d_tests_make_others
//   every other operand: inline integers and rationals about the family's
// ends; floating-point values about 2^256, zeros, infinities and NaN;
// errors, booleans and none; and the harness's kinds.
static void
d_tests_make_others(void)
{
    const dj_math_imax quarter = (dj_math_imax)((dj_math_umax)1
                                                << (D_TESTS_W - 2));
    const dj_math_imax half    = (dj_math_imax)((dj_math_umax)1
                                                << (D_TESTS_W / 2));
    volatile float     zero_f  = 0.0f;

    // integers
    d_tests_add_other(d_math_value_from_int(0));
    d_tests_add_other(d_math_value_from_int(1));
    d_tests_add_other(d_math_value_from_int(-1));
    d_tests_add_other(d_math_value_from_int(2));
    d_tests_add_other(d_math_value_from_int(-3));
    d_tests_add_other(d_math_value_from_int(7));
    d_tests_add_other(d_math_value_from_int(D_TESTS_MOST));
    d_tests_add_other(d_math_value_from_int(D_TESTS_LEAST));
    d_tests_add_other(d_math_value_from_int(D_TESTS_MOST - 1));
    d_tests_add_other(d_math_value_from_int(quarter));
    d_tests_add_other(d_math_value_from_int(-half));
    d_tests_add_other(d_tests_flagged(d_math_value_from_int(5),
                                      D_MATH_VALUE_FLAG_INEXACT));

    // rationals
    d_tests_add_other(d_math_value_from_ratio(1, 2));
    d_tests_add_other(d_math_value_from_ratio(-1, 2));
    d_tests_add_other(d_math_value_from_ratio(2, 3));
    d_tests_add_other(d_math_value_from_ratio(-7, 3));
    d_tests_add_other(d_math_value_from_ratio(1, D_TESTS_MOST));
    d_tests_add_other(d_math_value_from_ratio(D_TESTS_MOST, 2));
    d_tests_add_other(d_math_value_from_ratio(D_TESTS_MOST, 6));
    d_tests_add_other(d_math_value_from_ratio(D_TESTS_LEAST, 3));
    d_tests_add_other(d_math_value_from_ratio(-D_TESTS_MOST,
                                              D_TESTS_MOST - 1));
    d_tests_add_other(d_math_value_from_ratio(3, half));
    d_tests_add_other(d_math_value_from_ratio(half + 1, half - 1));
    d_tests_add_other(d_tests_flagged(d_math_value_from_ratio(1, 3),
                                      D_MATH_VALUE_FLAG_INEXACT));

    // floating-point values
    d_tests_add_other(d_math_value_from_f(0.0f));
    d_tests_add_other(d_math_value_from_f(-0.0f));
    d_tests_add_other(d_math_value_from_f(1.5f));
    d_tests_add_other(d_math_value_from_f(FLT_MAX));
    d_tests_add_other(d_math_value_from_f((float)HUGE_VAL));
    d_tests_add_other(d_math_value_from_f(zero_f / zero_f));
    d_tests_add_other(d_math_value_from_d(-2.0));
    d_tests_add_other(d_math_value_from_d(1e300));
    d_tests_add_other(d_math_value_from_d(ldexp(1.0, 256)));
    d_tests_add_other(d_math_value_from_d(-ldexp(1.0, 255)));
    d_tests_add_other(d_math_value_from_ld(ldexpl(1.0L, 256)));
    d_tests_add_other(d_math_value_from_ld(ldexpl(1.0L, 64) + 1.0L));
    d_tests_add_other(d_math_value_from_ld(-(long double)HUGE_VAL));
    d_tests_add_other(d_math_value_from_ld(1.0L / 3.0L));
    d_tests_add_other(d_tests_flagged(d_math_value_from_d(0.5),
                                      D_MATH_VALUE_FLAG_INEXACT));

    // errors, booleans and none
    d_tests_add_other(d_math_value_none());
    d_tests_add_other(d_math_value_from_bool(false));
    d_tests_add_other(d_math_value_from_bool(true));
    d_tests_add_other(d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN));
    d_tests_add_other(d_math_value_from_error(D_MATH_VALUE_ERROR_POLE));
    d_tests_add_other(d_tests_flagged(
        d_math_value_from_error(D_MATH_VALUE_ERROR_MEMORY),
        D_MATH_VALUE_FLAG_INEXACT));

    // the harness's kinds
    d_tests_add_other(d_tests_new_box(D_TESTS_SMALL, d_math_value_from_int(5),
                                      false));
    d_tests_add_other(d_tests_new_box(D_TESTS_SMALL,
                                      d_math_value_from_int(D_TESTS_LEAST),
                                      false));
    d_tests_add_other(d_tests_new_box(D_TESTS_SMALL, d_math_value_from_int(0),
                                      false));
    d_tests_add_other(d_tests_new_box(D_TESTS_SMALL, d_math_value_from_int(9),
                                      true));
    d_tests_add_other(d_tests_new_box(D_TESTS_BLUR,
                                      d_math_value_from_ld(2.5L), false));
    d_tests_add_other(d_tests_new_box(D_TESTS_TOKEN, d_math_value_none(),
                                      false));

    return;
}

// d_tests_release_operands
//   every operand given back.
static void
d_tests_release_operands(void)
{
    int i = 0;

    for (i = 0; i < g_wide_count; ++i)
    {
        d_math_value_release(&g_wides[i]);
    }

    for (i = 0; i < g_other_count; ++i)
    {
        d_math_value_release(&g_others[i]);
    }

    g_wide_count  = 0;
    g_other_count = 0;

    return;
}


//==============================================================================
// 7.  ONE OPERAND
//==============================================================================


// 7.1    Queries
//------------------------------------------------------------------------------

// d_tests_queries
//   _x's kind as value.h and the unit tell it: its rank, its name, a number
// and exact; of the kind; and its integer read back.
static void
d_tests_queries(
    struct dj_math_value _x
)
{
    const struct dj_math_wide held = d_tests_held(_x);

    struct dj_math_wide read = d_math_wide_from_imax(-1);

    D_TESTS_EXPECT(d_math_value_rank(_x) == D_TESTS_RANK_WIDE, "its rank", _x,
                   d_math_value_none());
    D_TESTS_EXPECT(strcmp(d_math_value_kind_name(_x), D_TESTS_NAME_WIDE) == 0,
                   "its name", _x, d_math_value_none());
    D_TESTS_EXPECT( (d_math_value_is_number(_x)) &&
                    (d_math_value_is_exact(_x)) &&
                    (d_math_value_is_wide(_x)),
                    "a number, exact, and a wide integer", _x,
                    d_math_value_none());
    D_TESTS_EXPECT( (d_math_value_to_wide(&read, _x)) &&
                    (d_math_wide_eq(read, held)),
                    "its integer, read back", _x, d_math_value_none());

    return;
}

// 7.2    Signs
//------------------------------------------------------------------------------

// d_tests_sign
//   -_x where _negate, else |_x|: a box of its own, the integer's sign turned
// or cleared, with _x's flags, its holders as they were.
static void
d_tests_sign(
    struct dj_math_value _x,
    bool                 _negate
)
{
    const long                live    = g_live;
    const long                holders = d_tests_holders(_x);
    const struct dj_tests_big value   =
        d_tests_big_of_integer(d_tests_held(_x));

    struct dj_tests_expected want = {
        D_TESTS_BOXED, d_tests_exact_from(value, d_tests_one()) };
    struct dj_math_value     r    = (_negate) ? d_math_value_neg(_x)
                                              : d_math_value_abs(_x);

    want.value.n = (_negate) ? d_tests_big_neg(value) : d_tests_big_abs(value);
    g_shown      = r;

    D_TESTS_EXPECT(d_tests_is_result(r, want, _x.flags, _x.type),
                   (_negate) ? "-a" : "|a|", _x, d_math_value_none());
    D_TESTS_EXPECT( (g_live == (live + 1L)) &&
                    (d_tests_holders(_x) == holders),
                    "one new box, and the operand's holders as they were",
                    _x, d_math_value_none());

    d_tests_drop(&r);
    g_shown = d_math_value_none();

    D_TESTS_EXPECT( (g_live == live) &&
                    (r.kind == D_MATH_VALUE_NONE),
                    "the result given back", _x, d_math_value_none());

    return;
}

// 7.3    Conversions
//------------------------------------------------------------------------------

// d_tests_threshold
//   the float's threshold for an infinity: FLT_MAX and half its last place,
// 2^MAX_EXP - 2^(MAX_EXP - P - 1), which rounds, ties to even, to 2^MAX_EXP.
static struct dj_tests_big
d_tests_threshold(void)
{
    return d_tests_minus(d_tests_power(FLT_MAX_EXP),
                         d_tests_power(FLT_MAX_EXP - FLT_MANT_DIG - 1));
}

// d_tests_nearest_float
//   whether _f, finite, is the float nearest the integer of magnitude _n
// and the sign _negative: math_tests_exact.h's oracle, but for the largest
// float, whose neighbour above, 2^MAX_EXP, the C library cannot give, a
// virtual one; there the integer must also lie below the threshold, where
// it would round to an infinity.
static bool
d_tests_nearest_float(
    bool                _negative,
    struct dj_tests_big _n,
    float               _f
)
{
    const float magnitude = (_negative) ? -_f : _f;

    struct dj_tests_parts above   = { { { 0UL }, 0 }, 0 };
    float                 below   = 0.0f;
    float                 f_r     = 0.0f;
    float                 f_below = 0.0f;
    int                   e_r     = 0;
    int                   e_below = 0;

    // below the largest: the oracle as it is
    if (magnitude != FLT_MAX)
    {
        return d_tests_nearest_f(_negative, _n, d_tests_one(), _f);
    }

    below          = nextafterf(FLT_MAX, 0.0f);
    f_r            = frexpf(FLT_MAX, &e_r);
    f_below        = frexpf(below, &e_below);
    above.mantissa = d_tests_power(FLT_MANT_DIG - 1);
    above.exponent = FLT_MAX_EXP - FLT_MANT_DIG + 1;

    return ( (d_tests_big_cmp_mag(_n, d_tests_threshold()) < 0) &&
             (d_tests_nearest(_n, d_tests_one(),
                              d_tests_parts_of(f_r, e_r, FLT_MANT_DIG),
                              d_tests_parts_of(f_below, e_below,
                                               FLT_MANT_DIG),
                              above)) );
}

// d_tests_convert_entry
//   the kind's conversion of _x to _to, called through its table: true, and
// the inline value of that kind whose real is _real, its direction how it
// stands to _x, exactly.
static void
d_tests_convert_entry(
    struct dj_math_value    _x,
    enum dj_math_value_kind _to,
    long double             _real
)
{
    const struct dj_tests_exact exact =
        d_tests_exact_from(d_tests_big_of_integer(d_tests_held(_x)),
                           d_tests_one());

    struct dj_math_value out       = d_math_value_from_bool(true);
    int                  direction = 7;
    bool                 ok        =
        _x.type->convert(&out, &direction, _x, _to);

    g_shown = out;

    D_TESTS_EXPECT( (ok) &&
                    (out.kind == _to) &&
                    (out.flags == 0u) &&
                    (d_tests_same_real(out.real, _real)) &&
                    (direction == -d_tests_order_real(exact, _real)),
                    "its conversion's value and direction", _x,
                    d_math_value_none());
    g_shown = d_math_value_none();

    return;
}

// d_tests_conversions
//   _x to float, double and long double: the nearest value, past the
// largest float an infinity, and each direction exact; to an exact kind,
// the integer where it fits the family; and to anything else, nothing.
static void
d_tests_conversions(
    struct dj_math_value _x
)
{
    const struct dj_tests_big value    =
        d_tests_big_of_integer(d_tests_held(_x));
    const struct dj_tests_big most     = d_tests_big_of(D_TESTS_MOST);
    const struct dj_tests_big least    = d_tests_big_of(D_TESTS_LEAST);
    const bool                negative = (value.negative != 0);
    const bool                fits     =
        ( (d_tests_big_cmp(value, most) <= 0) &&
          (d_tests_big_cmp(value, least) >= 0) );

    float                f         = 0.0f;
    double               d         = 0.0;
    long double          ld        = 0.0L;
    struct dj_rational   r         = { 3, 7 };
    struct dj_math_value out       = d_math_value_from_bool(true);
    int                  direction = 7;
    int                  k         = 0;
    bool                 ok        = false;

    // a float: the nearest, or an infinity at the threshold and past it
    ok = d_math_value_to_f(&f, _x);
    D_TESTS_EXPECT( (ok) &&
                    ( ( (f <= FLT_MAX) &&
                        (f >= -FLT_MAX) &&
                        (d_tests_nearest_float(negative,
                                               d_tests_big_abs(value), f)) ) ||
                      ( ( (f > FLT_MAX) ||
                          (f < -FLT_MAX) ) &&
                        ((f < 0.0f) == negative) &&
                        (d_tests_big_cmp_mag(value, d_tests_threshold())
                             >= 0) ) ||
                      ( (f == 0.0f) &&
                        (d_tests_big_is_zero(value)) ) ),
                    "the nearest float", _x, d_math_value_none());

    // a double and a long double: the nearest
    ok = d_math_value_to_d(&d, _x);
    D_TESTS_EXPECT( (ok) &&
                    ( (d_tests_nearest_d(negative, d_tests_big_abs(value),
                                         d_tests_one(), d)) ||
                      ( (d == 0.0) &&
                        (d_tests_big_is_zero(value)) ) ),
                    "the nearest double", _x, d_math_value_none());

    ok = d_math_value_to_ld(&ld, _x);
    D_TESTS_EXPECT( (ok) &&
                    ( (d_tests_nearest_ld(negative, d_tests_big_abs(value),
                                          d_tests_one(), ld)) ||
                      ( (ld == 0.0L) &&
                        (d_tests_big_is_zero(value)) ) ),
                    "the nearest long double", _x, d_math_value_none());

    // through the table, with the direction: 0 is +0
    d_tests_convert_entry(_x, D_MATH_VALUE_FLOAT, (long double)f);
    d_tests_convert_entry(_x, D_MATH_VALUE_DOUBLE, (long double)d);
    d_tests_convert_entry(_x, D_MATH_VALUE_LONG_DOUBLE, ld);
    D_TESTS_EXPECT( (!d_tests_big_is_zero(value)) ||
                    ( (d_tests_is_plus_zero((long double)f)) &&
                      (d_tests_is_plus_zero((long double)d)) &&
                      (d_tests_is_plus_zero(ld)) ),
                    "0 converts to +0", _x, d_math_value_none());

    // an exact kind: the integer where it fits, nothing touched otherwise
    ok = d_math_value_to_rational(&r, _x);
    D_TESTS_EXPECT( ( (fits) &&
                      (ok) &&
                      (r.den == 1) &&
                      (d_tests_big_cmp(d_tests_big_of(r.num), value) == 0) ) ||
                    ( (!fits) &&
                      (!ok) &&
                      (r.num == 3) &&
                      (r.den == 7) ),
                    "a rational where it fits", _x, d_math_value_none());

    for (k = (int)D_MATH_VALUE_INTEGER; k <= (int)D_MATH_VALUE_RATIONAL; ++k)
    {
        out       = d_math_value_from_bool(true);
        direction = 7;
        ok        = _x.type->convert(&out, &direction, _x,
                                     (enum dj_math_value_kind)k);
        g_shown   = out;

        D_TESTS_EXPECT( ( (fits) &&
                          (ok) &&
                          (direction == 0) &&
                          (out.kind == D_MATH_VALUE_INTEGER) &&
                          (out.flags == 0u) &&
                          (d_tests_big_cmp(d_tests_big_of(out.exact.num),
                                           value) == 0) &&
                          (out.exact.den == 1) ) ||
                        ( (!fits) &&
                          (!ok) &&
                          (direction == 7) &&
                          (out.kind == D_MATH_VALUE_BOOLEAN) ),
                        "to an exact kind, where it fits", _x,
                        d_math_value_none());
    }

    // nothing else
    for (k = (int)D_MATH_VALUE_NONE; k <= (int)D_MATH_VALUE_BOXED; ++k)
    {
        if ( (k >= (int)D_MATH_VALUE_INTEGER) &&
             (k <= (int)D_MATH_VALUE_LONG_DOUBLE) )
        {
            continue;
        }

        out       = d_math_value_from_bool(true);
        direction = 7;
        ok        = _x.type->convert(&out, &direction, _x,
                                     (enum dj_math_value_kind)k);
        g_shown   = out;

        D_TESTS_EXPECT( (!ok) &&
                        (direction == 7) &&
                        (out.kind == D_MATH_VALUE_BOOLEAN),
                        "to no number, nothing", _x, d_math_value_none());
    }

    g_shown = d_math_value_none();

    return;
}

// 7.4    Text
//------------------------------------------------------------------------------

// d_tests_texts
//   _x's text, through the kind's table: the reference's digits, its length
// returned whatever the room, and cut short as snprintf cuts it.
static void
d_tests_texts(
    struct dj_math_value _x
)
{
    char   want[D_TESTS_TEXT_ROOM];
    char   got[D_TESTS_TEXT_ROOM];
    size_t length  = 0u;
    size_t room[5] = { 0u, 1u, 2u, 0u, 0u };
    size_t size    = 0u;
    int    i       = 0;
    bool   ok      = false;

    (void)d_tests_big_text(d_tests_big_of_integer(d_tests_held(_x)), want);
    length  = strlen(want);
    room[3] = length;
    room[4] = length + 1u;

    // the whole text
    memset(got, '#', sizeof(got));
    size = _x.type->text(got, sizeof(got), _x);
    D_TESTS_EXPECT( (size == length) &&
                    (strcmp(got, want) == 0),
                    "its text", _x, d_math_value_none());

    // cut short: what fits before the NUL, and nothing past the room
    for (i = 0; i < 5; ++i)
    {
        memset(got, '#', sizeof(got));
        size = _x.type->text(got, room[i], _x);
        ok   = (size == length);

        if (room[i] == 0u)
        {
            ok = ( (ok) &&
                   (got[0] == '#') );
        }
        else
        {
            const size_t kept = (length < (room[i] - 1u)) ? length
                                                          : (room[i] - 1u);

            ok = ( (ok) &&
                   (strncmp(got, want, kept) == 0) &&
                   (got[kept] == '\0') &&
                   (got[room[i]] == '#') );
        }

        D_TESTS_EXPECT(ok, "its text, cut short", _x, d_math_value_none());
    }

    return;
}

// 7.5    Holders
//------------------------------------------------------------------------------

// d_tests_holds
//   retain makes a copy that holds the same box once more, and release gives
// that hold back; neither allocates or frees.
static void
d_tests_holds(
    struct dj_math_value _x
)
{
    const long live    = g_live;
    const long holders = d_tests_holders(_x);

    struct dj_math_value copy = d_math_value_retain(_x);

    D_TESTS_EXPECT( (copy.box == _x.box) &&
                    (copy.type == _x.type) &&
                    (copy.flags == _x.flags) &&
                    (d_tests_holders(_x) == (holders + 1L)) &&
                    (g_live == live),
                    "retained: one holder more", _x, d_math_value_none());

    d_math_value_release(&copy);

    D_TESTS_EXPECT( (copy.kind == D_MATH_VALUE_NONE) &&
                    (d_tests_holders(_x) == holders) &&
                    (g_live == live),
                    "released: one holder fewer", _x, d_math_value_none());

    return;
}

// d_tests_ones
//   each wide integer alone.
static void
d_tests_ones(void)
{
    int i = 0;

    for (i = 0; i < g_wide_count; ++i)
    {
        d_tests_queries(g_wides[i]);
        d_tests_sign(g_wides[i], true);
        d_tests_sign(g_wides[i], false);
        d_tests_conversions(g_wides[i]);
        d_tests_texts(g_wides[i]);
        d_tests_holds(g_wides[i]);
    }

    return;
}


//==============================================================================
// 8.  TWO OPERANDS
//==============================================================================


// 8.1    What a pair must give
//------------------------------------------------------------------------------

// d_tests_reads
//   the conversions the kind must ask of _x's kind to read it: one, for a
// small; none for anything else.
static long
d_tests_reads(
    struct dj_math_value _x
)
{
    return (d_tests_kind_of(_x) == D_TESTS_SMALL) ? 1L : 0L;
}

// d_tests_harness_kind
//   the harness's kind among _a and _b, -1 for none.
static int
d_tests_harness_kind(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    return (d_tests_kind_of(_a) >= 0) ? d_tests_kind_of(_a)
                                      : d_tests_kind_of(_b);
}

// d_tests_two_op
//   _a _op _b, one of them a wide integer, against what it must give: an
// error passed through, a domain error for a boolean or none, value.h's
// arithmetic on the integer converted for a float, double or long double,
// the exact result for an exact operand, and a domain error for an operand
// the kind cannot read. The operands' holders as they were, one new box
// where the result is the kind's, and only the reads asked of the
// harness's kinds.
static void
d_tests_two_op(
    enum dj_math_value_op _op,
    struct dj_math_value  _a,
    struct dj_math_value  _b
)
{
    const struct dj_math_value   wide      = d_tests_is_wide(_a) ? _a : _b;
    const unsigned               flags     = _a.flags | _b.flags;
    const long                   live      = g_live;
    const long                   a_holders = d_tests_holders(_a);
    const long                   b_holders = d_tests_holders(_b);
    const struct dj_tests_counts before    = d_tests_counted();
    const char*                  what      = d_tests_operation_name(_op);

    struct dj_tests_exact    ea    = d_tests_exact_of(0, 1);
    struct dj_tests_exact    eb    = d_tests_exact_of(0, 1);
    struct dj_tests_expected want  = { D_TESTS_BOXED, ea };
    struct dj_math_value     other = d_math_value_none();
    struct dj_math_value     r     = d_tests_apply(_op, _a, _b);
    long                     boxes = 0L;

    g_shown = r;

    // an error, the first; a boolean or none
    if ( (_a.kind == D_MATH_VALUE_ERROR) ||
         (_b.kind == D_MATH_VALUE_ERROR) )
    {
        other       = (_a.kind == D_MATH_VALUE_ERROR) ? _a : _b;
        other.flags = flags;
        D_TESTS_EXPECT(d_tests_same(r, other), what, _a, _b);
    }
    else if ( (d_tests_is_other(_a)) ||
              (d_tests_is_other(_b)) )
    {
        D_TESTS_EXPECT(d_tests_is_error(r, D_MATH_VALUE_ERROR_DOMAIN, flags),
                       what, _a, _b);
    }
    // a float, double or long double: the integer converted to its type
    else if ( (d_tests_is_float_kind(_a)) ||
              (d_tests_is_float_kind(_b)) )
    {
        const enum dj_math_value_kind to =
            d_tests_is_float_kind(_a) ? _a.kind : _b.kind;

        other       = d_tests_apply(_op, d_tests_converted(_a, to),
                                    d_tests_converted(_b, to));
        other.flags = flags;
        D_TESTS_EXPECT(d_tests_same(r, other), what, _a, _b);
    }
    // two exact operands: the exact result
    else if ( (d_tests_value_exact(_a, &ea)) &&
              (d_tests_value_exact(_b, &eb)) )
    {
        want  = d_tests_expected_of(_op, ea, eb);
        boxes = (want.outcome == D_TESTS_BOXED) ? 1L : 0L;
        D_TESTS_EXPECT(d_tests_is_result(r, want, flags, wide.type), what,
                       _a, _b);
    }
    // an operand the kind cannot read
    else
    {
        D_TESTS_EXPECT(d_tests_is_error(r, D_MATH_VALUE_ERROR_DOMAIN, flags),
                       what, _a, _b);
    }

    // the storage, and the calls into the harness's kinds
    D_TESTS_EXPECT( (d_tests_holders(_a) == a_holders) &&
                    (d_tests_holders(_b) == b_holders) &&
                    (g_live == (live + boxes)),
                    "the operands' holders as they were, and the result's box",
                    _a, _b);
    D_TESTS_EXPECT(d_tests_calls_are(&before, d_tests_harness_kind(_a, _b),
                                     d_tests_reads(_a) + d_tests_reads(_b)),
                   "only a small read, once", _a, _b);

    d_tests_drop(&r);
    g_shown = d_math_value_none();

    D_TESTS_EXPECT(g_live == live, "the result given back", _a, _b);

    return;
}

// d_tests_two_cmp
//   _a against _b, one of them a wide integer: unordered beside an error, a
// boolean, none, a NaN or an operand the kind cannot read; otherwise the
// exact order. Nothing allocated, and eq as cmp says.
static void
d_tests_two_cmp(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    const long                   live   = g_live;
    const struct dj_tests_counts before = d_tests_counted();

    struct dj_tests_exact    ea    = d_tests_exact_of(0, 1);
    struct dj_tests_exact    eb    = d_tests_exact_of(0, 1);
    enum dj_math_value_order want  = D_MATH_VALUE_UNORDERED;
    enum dj_math_value_order order = d_math_value_cmp(_a, _b);

    g_shown = d_math_value_none();

    // beside a float kind: the integer against its value, exactly
    if ( (d_tests_is_float_kind(_a)) &&
         (d_tests_value_exact(_b, &eb)) )
    {
        want = d_internal_math_value_turn(
            d_tests_order_of(d_tests_order_real(eb, _a.real)));
    }
    else if ( (d_tests_is_float_kind(_b)) &&
              (d_tests_value_exact(_a, &ea)) )
    {
        want = d_tests_order_of(d_tests_order_real(ea, _b.real));
    }
    // two exact operands
    else if ( (d_tests_value_exact(_a, &ea)) &&
              (d_tests_value_exact(_b, &eb)) )
    {
        want = d_tests_order_of(d_tests_exact_cmp(ea, eb));
    }

    D_TESTS_EXPECT(order == want, "a against b", _a, _b);
    D_TESTS_EXPECT(d_math_value_eq(_a, _b) == (want == D_MATH_VALUE_EQUAL),
                   "a == b", _a, _b);
    D_TESTS_EXPECT( (g_live == live) &&
                    (d_tests_calls_are(&before, d_tests_harness_kind(_a, _b),
                                       2L * (d_tests_reads(_a) +
                                             d_tests_reads(_b)))),
                    "no box, and only a small read, once each", _a, _b);

    return;
}

// d_tests_two
//   every operation, and the comparison, on _a and _b.
static void
d_tests_two(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    int op = 0;

    for (op = (int)D_MATH_VALUE_OP_ADD; op <= (int)D_MATH_VALUE_OP_DIV; ++op)
    {
        d_tests_two_op((enum dj_math_value_op)op, _a, _b);
    }

    d_tests_two_cmp(_a, _b);

    return;
}

// d_tests_pairs
//   every pair of wide integers, and every wide integer with every other
// operand, both ways.
static void
d_tests_pairs(void)
{
    int i = 0;
    int j = 0;

    for (i = 0; i < g_wide_count; ++i)
    {
        for (j = 0; j < g_wide_count; ++j)
        {
            d_tests_two(g_wides[i], g_wides[j]);
        }

        for (j = 0; j < g_other_count; ++j)
        {
            d_tests_two(g_wides[i], g_others[j]);
            d_tests_two(g_others[j], g_wides[i]);
        }
    }

    return;
}


//==============================================================================
// 9.  STORAGE REFUSED
//==============================================================================
// Each operation that makes a box, with its box refused: a memory error, the
// operands' flags on it, nothing left behind; and one that makes none, with
// a refusal waiting, untouched by it.

// d_tests_refused_op
//   _a _op _b, one a wide integer and the other exact, with the next box
// refused.
static void
d_tests_refused_op(
    enum dj_math_value_op _op,
    struct dj_math_value  _a,
    struct dj_math_value  _b
)
{
    const struct dj_math_value wide    = d_tests_is_wide(_a) ? _a : _b;
    const unsigned             flags   = _a.flags | _b.flags;
    const long                 live    = g_live;
    const long                 refused = g_refused;

    struct dj_tests_exact    ea   = d_tests_exact_of(0, 1);
    struct dj_tests_exact    eb   = d_tests_exact_of(0, 1);
    struct dj_tests_expected want = { D_TESTS_BOXED, ea };
    struct dj_math_value     r    = d_math_value_none();

    (void)d_tests_value_exact(_a, &ea);
    (void)d_tests_value_exact(_b, &eb);
    want = d_tests_expected_of(_op, ea, eb);

    g_refuse = 0L;
    r        = d_tests_apply(_op, _a, _b);
    g_shown  = r;

    // a box refused: a memory error; otherwise the result, the refusal left
    if (want.outcome == D_TESTS_BOXED)
    {
        D_TESTS_EXPECT( (d_tests_is_error(r, D_MATH_VALUE_ERROR_MEMORY,
                                          flags)) &&
                        (g_refused == (refused + 1L)),
                        "its box refused: a memory error", _a, _b);
    }
    else
    {
        D_TESTS_EXPECT( (d_tests_is_result(r, want, flags, wide.type)) &&
                        (g_refused == refused) &&
                        (g_refuse == 0L),
                        "no box asked for", _a, _b);
    }

    D_TESTS_EXPECT(g_live == live, "nothing left behind", _a, _b);

    d_tests_drop(&r);
    g_shown  = d_math_value_none();
    g_refuse = -1L;

    return;
}

// d_tests_refused
//   each wide integer's negation, absolute value and copy with its box
// refused, and its arithmetic with a few exact operands.
static void
d_tests_refused(void)
{
    const struct dj_math_value exact[4] = {
        d_math_value_from_int(1), d_math_value_from_int(-3),
        d_math_value_from_ratio(1, 2), d_math_value_from_int(0)
    };

    struct dj_math_value r = d_math_value_none();
    int                  i = 0;
    int                  j = 0;
    int                  op = 0;

    for (i = 0; i < g_wide_count; ++i)
    {
        const struct dj_math_value x    = g_wides[i];
        const long                 live = g_live;

        // -x and |x|
        g_refuse = 0L;
        r        = d_math_value_neg(x);
        g_shown  = r;
        D_TESTS_EXPECT( (d_tests_is_error(r, D_MATH_VALUE_ERROR_MEMORY,
                                          x.flags)) &&
                        (g_live == live),
                        "-a, its box refused", x, d_math_value_none());

        g_refuse = 0L;
        r        = d_math_value_abs(x);
        g_shown  = r;
        D_TESTS_EXPECT( (d_tests_is_error(r, D_MATH_VALUE_ERROR_MEMORY,
                                          x.flags)) &&
                        (g_live == live),
                        "|a|, its box refused", x, d_math_value_none());

        // a new value of the integer
        g_refuse = 0L;
        r        = d_math_value_from_wide(d_tests_held(x));
        g_shown  = r;
        D_TESTS_EXPECT( (d_tests_is_error(r, D_MATH_VALUE_ERROR_MEMORY, 0u)) &&
                        (g_live == live),
                        "a value made, its box refused", x,
                        d_math_value_none());
        g_shown  = d_math_value_none();
        g_refuse = -1L;

        // the four operations, either way
        for (j = 0; j < 4; ++j)
        {
            for (op = (int)D_MATH_VALUE_OP_ADD;
                 op <= (int)D_MATH_VALUE_OP_DIV; ++op)
            {
                d_tests_refused_op((enum dj_math_value_op)op, x, exact[j]);
                d_tests_refused_op((enum dj_math_value_op)op, exact[j], x);
            }
        }
    }

    return;
}


//==============================================================================
// 10.  AT RANDOM
//==============================================================================


// 10.1    Drawing
//------------------------------------------------------------------------------

// d_tests_random
//   16 random bits.
static unsigned long
d_tests_random(void)
{
    g_state = ((g_state * 1103515245UL) + 12345UL) & 0xFFFFFFFFUL;

    return (g_state >> 16) & 0xFFFFUL;
}

// d_tests_random_below
//   a random int from 0 to _n - 1.
static int
d_tests_random_below(
    int _n
)
{
    return (int)(d_tests_random() % (unsigned long)_n);
}

// d_tests_random_big
//   a natural of _bits bits, its top bit set (0 for 0 bits): random limbs,
// or a quarter of the time limbs of all ones or all zeros.
static struct dj_tests_big
d_tests_random_big(
    int _bits
)
{
    const bool runs = (d_tests_random_below(4) == 0);

    struct dj_tests_big result = { { 0UL }, 0 };
    int                 i      = 0;

    for (i = 0; (i * 16) < _bits; ++i)
    {
        unsigned long limb = d_tests_random();

        if (runs)
        {
            limb = (d_tests_random_below(2) == 0) ? 0xFFFFUL : 0UL;
        }

        // the top limb: cut to the bits left, the top one set
        if (((i + 1) * 16) >= _bits)
        {
            const int left = _bits - (i * 16);

            limb = (limb & ((1UL << left) - 1UL)) | (1UL << (left - 1));
        }

        result.limb[i] = limb;
    }

    return result;
}

// d_tests_random_wide
//   a wide integer of a random length up to 256 bits, of either sign.
static struct dj_math_value
d_tests_random_wide(void)
{
    const struct dj_tests_big x =
        d_tests_random_big(d_tests_random_below(257));

    return d_tests_wide_value(
        (d_tests_random_below(2) == 0) ? x : d_tests_big_neg(x), 0u);
}

// d_tests_random_imax
//   an integer of the family of a random length below a word, either sign.
static dj_math_imax
d_tests_random_imax(void)
{
    const int bits = 1 + d_tests_random_below(D_TESTS_W - 1);

    dj_math_umax x = 0u;
    int          i = 0;

    for (i = 0; i < D_TESTS_W; i = i + 16)
    {
        x = (x << 16) | (dj_math_umax)d_tests_random();
    }

    x = x & (((dj_math_umax)1 << bits) - 1u);

    return (d_tests_random_below(2) == 0) ? (dj_math_imax)x
                                          : -(dj_math_imax)x;
}

// d_tests_random_other
//   an inline integer or rational, or a wide integer, drawn at random.
static struct dj_math_value
d_tests_random_other(void)
{
    const int choice = d_tests_random_below(3);

    dj_math_imax den = 0;

    // a wide integer, an integer, or a rational
    if (choice == 0)
    {
        return d_tests_random_wide();
    }

    if (choice == 1)
    {
        return d_math_value_from_int(d_tests_random_imax());
    }

    while (den == 0)
    {
        den = d_tests_random_imax();
    }

    return d_math_value_from_ratio(d_tests_random_imax(), den);
}

// 10.2    The trials
//------------------------------------------------------------------------------

// d_tests_random_trials
//   pairs drawn at random: a wide integer with a wide integer, an integer or
// a rational, both ways; products past 256 bits; and quotients with a large
// common factor.
static void
d_tests_random_trials(void)
{
    long trial = 0L;

    for (trial = 0L; trial < D_TESTS_TRIALS; ++trial)
    {
        struct dj_math_value x = d_tests_random_wide();
        struct dj_math_value y = d_tests_random_other();

        d_tests_two(x, y);
        d_tests_two(y, x);

        d_math_value_release(&x);
        d_math_value_release(&y);
    }

    // long operands: products past 256 bits, and quotients past the family
    for (trial = 0L; trial < (D_TESTS_TRIALS / 4L); ++trial)
    {
        struct dj_math_value x = d_tests_wide_value(
            d_tests_random_big(160 + d_tests_random_below(97)), 0u);
        struct dj_math_value y = d_tests_wide_value(
            d_tests_random_big(160 + d_tests_random_below(97)), 0u);

        d_tests_two(x, y);

        d_math_value_release(&x);
        d_math_value_release(&y);
    }

    // a common factor of up to 128 bits
    for (trial = 0L; trial < (D_TESTS_TRIALS / 4L); ++trial)
    {
        const struct dj_tests_big g =
            d_tests_random_big(1 + d_tests_random_below(128));
        const struct dj_tests_big p =
            d_tests_random_big(1 + d_tests_random_below(128));
        const struct dj_tests_big q =
            d_tests_random_big(1 + d_tests_random_below(128));

        struct dj_math_value x = d_tests_wide_value(d_tests_big_mul(g, p), 0u);
        struct dj_math_value y = d_tests_wide_value(d_tests_big_mul(g, q), 0u);

        d_tests_two_op(D_MATH_VALUE_OP_DIV, x, y);
        d_tests_two_op(D_MATH_VALUE_OP_MUL, x, y);

        d_math_value_release(&x);
        d_math_value_release(&y);
    }

    return;
}


//==============================================================================
// 11.  THE ROUNDING OF A RATIO
//==============================================================================
// The unit rounds a ratio of terms of up to 512 bits once, to any precision
// up to two words. Its mantissa must have exactly P bits; neither of its
// neighbours, one place down and one up, may be nearer the ratio, and on a
// tie its mantissa must be even; and its direction must be the exact order.


// d_tests_order_parts
//   -1, 0 or 1 as _n / _d is below, equal to or above m 2^e.
static int
d_tests_order_parts(
    struct dj_tests_big   _n,
    struct dj_tests_big   _d,
    struct dj_tests_parts _v
)
{
    const int shift = (_v.exponent < 0) ? -_v.exponent : 0;

    return d_tests_big_cmp_mag(
        d_tests_big_shl(_n, shift),
        d_tests_big_shl(d_tests_big_mul(_v.mantissa, _d),
                        _v.exponent + shift));
}

// d_tests_rounding_one
//   _n / _d, both positive and below 2^512, rounded by the unit to
// _precision bits, against the reference.
static void
d_tests_rounding_one(
    struct dj_tests_big _n,
    struct dj_tests_big _d,
    unsigned            _precision
)
{
    const int precision = (int)_precision;

    dj_math_wide_limb num[D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS] = { 0u };
    dj_math_wide_limb den[D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS] = { 0u };
    struct dj_internal_rational_rounded rounded = { { 0u, 0u }, 0, 0 };
    struct dj_tests_parts               r       = { { { 0UL }, 0 }, 0 };
    struct dj_tests_parts               below   = { { { 0UL }, 0 }, 0 };
    struct dj_tests_parts               above   = { { { 0UL }, 0 }, 0 };
    char                                detail[2 * D_TESTS_TEXT_ROOM];
    char                                n_text[D_TESTS_TEXT_ROOM];
    char                                d_text[D_TESTS_TEXT_ROOM];

    d_tests_limbs_of_big(_n, num, D_TESTS_RATIO);
    d_tests_limbs_of_big(_d, den, D_TESTS_RATIO);
    rounded = d_internal_math_value_wide_round_ratio(
        num, D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS,
        den, D_INTERNAL_MATH_VALUE_WIDE_RATIO_LIMBS, _precision);

    // the value and its neighbours: one place down, a binade down from its
    // bottom, and one place up
    r.mantissa = d_tests_big_of_wide(rounded.mantissa);
    r.exponent = rounded.exponent;

    if (d_tests_big_cmp_mag(r.mantissa, d_tests_power(precision - 1)) == 0)
    {
        below.mantissa = d_tests_plus(d_tests_power(precision), -1L);
        below.exponent = r.exponent - 1;
    }
    else
    {
        below.mantissa = d_tests_plus(r.mantissa, -1L);
        below.exponent = r.exponent;
    }

    above.mantissa = d_tests_plus(r.mantissa, 1L);
    above.exponent = r.exponent;

    sprintf(detail, "%s / %s to %d bits", d_tests_big_text(_n, n_text),
            d_tests_big_text(_d, d_text), precision);
    D_TESTS_EXPECT_DETAIL( (d_tests_big_bits(r.mantissa) == precision) &&
                           (d_tests_nearest(_n, _d, r, below, above)),
                           "a ratio rounded to the nearest", detail);
    D_TESTS_EXPECT_DETAIL(rounded.direction ==
                              -d_tests_order_parts(_n, _d, r),
                          "its rounding's direction", detail);

    return;
}

// d_tests_rounding_at
//   the ratios at _precision: the extremes of 512-bit terms; ties, exactly
// halfway, and a hair either side, the remainder past them as small as the
// terms allow; and ratios drawn at random.
static void
d_tests_rounding_at(
    unsigned _precision
)
{
    const int                 precision = (int)_precision;
    const struct dj_tests_big top       = d_tests_plus(d_tests_power(512), -1L);
    const struct dj_tests_big one       = d_tests_one();

    long trial = 0L;

    // the extremes
    d_tests_rounding_one(one, one, _precision);
    d_tests_rounding_one(one, top, _precision);
    d_tests_rounding_one(top, one, _precision);
    d_tests_rounding_one(top, d_tests_plus(top, -2L), _precision);
    d_tests_rounding_one(d_tests_plus(top, -2L), top, _precision);
    d_tests_rounding_one(top, d_tests_power(511), _precision);
    d_tests_rounding_one(d_tests_big_of_umax(3u, false), top, _precision);

    // ties and a hair either side: m + 1/2 over 1, as (2m + 1) / 2, and as
    // ((2m + 1) D + e) / 2D for a long D and e of -1, 0 or 1
    for (trial = 0L; trial < 40L; ++trial)
    {
        const struct dj_tests_big m =
            d_tests_random_big(precision - (int)(trial % 3L));
        const struct dj_tests_big odd =
            d_tests_plus(d_tests_big_twice(m), 1L);
        const struct dj_tests_big big_d =
            d_tests_random_big(1 + d_tests_random_below(512 - precision - 2));
        const struct dj_tests_big scaled = d_tests_big_mul(odd, big_d);
        const struct dj_tests_big twice_d = d_tests_big_twice(big_d);

        d_tests_rounding_one(odd, d_tests_big_of_umax(2u, false), _precision);
        d_tests_rounding_one(scaled, twice_d, _precision);
        d_tests_rounding_one(d_tests_plus(scaled, 1L), twice_d, _precision);
        d_tests_rounding_one(d_tests_plus(scaled, -1L), twice_d, _precision);
    }

    // at random
    for (trial = 0L; trial < D_TESTS_ROUND_TRIALS; ++trial)
    {
        const struct dj_tests_big n =
            d_tests_random_big(1 + d_tests_random_below(512));
        const struct dj_tests_big d =
            d_tests_random_big(1 + d_tests_random_below(512));

        d_tests_rounding_one(n, d, _precision);
    }

    return;
}

// d_tests_rounding
//   every precision a long double may have, up to two words: a float's, a
// double's, the x87's and binary128's, and this build's.
static void
d_tests_rounding(void)
{
    static const unsigned precisions[4] = { 24u, 53u, 64u, 113u };

    int i = 0;

    for (i = 0; i < 4; ++i)
    {
        if (precisions[i] <= (2u * D_INTERNAL_RATIONAL_WORD_BITS))
        {
            d_tests_rounding_at(precisions[i]);
        }
    }

    d_tests_rounding_at((unsigned)LDBL_MANT_DIG);

    return;
}


//==============================================================================
// 12.  THE TABLE
//==============================================================================


// 12.1    What it holds
//------------------------------------------------------------------------------

// the calls into a second copy of the kind's table, as another translation
// unit would hold it
static long g_copy_calls[D_TESTS_ENTRIES];

static void
d_tests_copy_retain(
    void* _box
)
{
    ++g_copy_calls[D_TESTS_RETAIN];
    d_math_value_wide_kind()->retain(_box);

    return;
}

static void
d_tests_copy_release(
    void* _box
)
{
    ++g_copy_calls[D_TESTS_RELEASE];
    d_math_value_wide_kind()->release(_box);

    return;
}

static struct dj_math_value
d_tests_copy_unary(
    enum dj_math_value_op _op,
    struct dj_math_value  _x
)
{
    ++g_copy_calls[D_TESTS_UNARY];

    return d_math_value_wide_kind()->unary(_op, _x);
}

static struct dj_math_value
d_tests_copy_binary(
    enum dj_math_value_op _op,
    struct dj_math_value  _a,
    struct dj_math_value  _b
)
{
    ++g_copy_calls[D_TESTS_BINARY];

    return d_math_value_wide_kind()->binary(_op, _a, _b);
}

static enum dj_math_value_order
d_tests_copy_cmp(
    struct dj_math_value _a,
    struct dj_math_value _b
)
{
    ++g_copy_calls[D_TESTS_CMP];

    return d_math_value_wide_kind()->cmp(_a, _b);
}

static bool
d_tests_copy_convert(
    struct dj_math_value*   _out,
    int*                    _direction,
    struct dj_math_value    _x,
    enum dj_math_value_kind _to
)
{
    ++g_copy_calls[D_TESTS_CONVERT];

    return d_math_value_wide_kind()->convert(_out, _direction, _x, _to);
}

static size_t
d_tests_copy_text(
    char*                _buffer,
    size_t               _size,
    struct dj_math_value _x
)
{
    ++g_copy_calls[D_TESTS_TEXT];

    return d_math_value_wide_kind()->text(_buffer, _size, _x);
}

static const struct dj_math_boxed_kind g_kind_copy = {
    D_TESTS_NAME_WIDE, D_TESTS_RANK_WIDE,
    D_MATH_BOXED_NUMBER | D_MATH_BOXED_EXACT,
    d_tests_copy_retain, d_tests_copy_release, d_tests_copy_unary,
    d_tests_copy_binary, d_tests_copy_cmp, d_tests_copy_convert,
    d_tests_copy_text, NULL
};

// a table like the wide integer's at a rank above it, which is another
// kind's
static const struct dj_math_boxed_kind g_kind_above = {
    "above", D_TESTS_RANK_WIDE + 10,
    D_MATH_BOXED_NUMBER | D_MATH_BOXED_EXACT,
    d_tests_copy_retain, d_tests_copy_release, d_tests_copy_unary,
    d_tests_copy_binary, d_tests_copy_cmp, d_tests_copy_convert,
    d_tests_copy_text, NULL
};

// d_tests_table_held
//   the table's name, rank, traits and entries, the same table on each
// call.
static void
d_tests_table_held(void)
{
    const struct dj_math_boxed_kind* table = d_math_value_wide_kind();

    D_TESTS_EXPECT( (table != NULL) &&
                    (table == d_math_value_wide_kind()) &&
                    (strcmp(table->name, D_TESTS_NAME_WIDE) == 0) &&
                    (table->rank == D_TESTS_RANK_WIDE) &&
                    (D_MATH_VALUE_RANK_WIDE == D_TESTS_RANK_WIDE) &&
                    (table->traits ==
                         (D_MATH_BOXED_NUMBER | D_MATH_BOXED_EXACT)) &&
                    (table->retain != NULL) &&
                    (table->release != NULL) &&
                    (table->unary != NULL) &&
                    (table->binary != NULL) &&
                    (table->cmp != NULL) &&
                    (table->convert != NULL) &&
                    (table->text != NULL),
                    "the kind's table", d_math_value_none(),
                    d_math_value_none());

    return;
}

// d_tests_table_copies
//   values under a second copy of the table mix with values under the
// first: one kind, the result of each under the table of the operand of the
// kind that handled it, and the copy's entries called where its values are.
static void
d_tests_table_copies(void)
{
    const long live = g_live;

    struct dj_math_value a = d_tests_wide_value(d_tests_power(200), 0u);
    struct dj_math_value b = d_tests_wide_value(d_tests_power(100), 0u);
    struct dj_math_value r = d_math_value_none();
    int                  e = 0;

    a.type = &g_kind_copy;

    for (e = 0; e < D_TESTS_ENTRIES; ++e)
    {
        g_copy_calls[e] = 0L;
    }

    // the copy's operand first: its table handles it, and the result is
    // under the copy
    r       = d_math_value_add(a, b);
    g_shown = r;
    D_TESTS_EXPECT( (d_tests_is_wide(r)) &&
                    (r.type == &g_kind_copy) &&
                    (g_copy_calls[D_TESTS_BINARY] == 1L),
                    "a copy's value first", a, b);
    d_tests_drop(&r);
    D_TESTS_EXPECT(g_copy_calls[D_TESTS_RELEASE] == 1L,
                   "a result under the copy, released through it", a, b);

    // the first table's operand first: the result under the first
    r       = d_math_value_sub(b, a);
    g_shown = r;
    D_TESTS_EXPECT( (d_tests_is_wide(r)) &&
                    (r.type == d_math_value_wide_kind()) &&
                    (g_copy_calls[D_TESTS_BINARY] == 1L) &&
                    (d_math_value_cmp(r, d_math_value_from_int(0)) ==
                     D_MATH_VALUE_LESS),
                    "the first table's value first", b, a);
    d_tests_drop(&r);

    // the copy's own entries: a comparison, a negation, a conversion
    D_TESTS_EXPECT( (d_math_value_cmp(a, b) == D_MATH_VALUE_GREATER) &&
                    (g_copy_calls[D_TESTS_CMP] == 1L),
                    "a copy's comparison", a, b);

    r       = d_math_value_neg(a);
    g_shown = r;
    D_TESTS_EXPECT( (d_tests_is_wide(r)) &&
                    (r.type == &g_kind_copy) &&
                    (g_copy_calls[D_TESTS_UNARY] == 1L),
                    "a copy's negation", a, b);
    d_tests_drop(&r);
    g_shown = d_math_value_none();

    D_TESTS_EXPECT( (d_math_value_cmp(d_math_value_from_d(1.0), a) ==
                     D_MATH_VALUE_LESS) &&
                    (g_copy_calls[D_TESTS_CONVERT] == 1L),
                    "a copy's conversion", a, b);

    d_math_value_release(&a);
    d_math_value_release(&b);

    D_TESTS_EXPECT( (g_live == live) &&
                    (g_copy_calls[D_TESTS_RELEASE] == 3L),
                    "every box given back, the copy's through it", a, b);

    return;
}

// 12.2    Its entries, asked what value.h never asks
//------------------------------------------------------------------------------

// d_tests_table_breaches
//   the entries called directly with what value.h never gives them: no
// value of the kind, an operation not theirs, a conversion to no number.
static void
d_tests_table_breaches(void)
{
    const struct dj_math_boxed_kind* table = d_math_value_wide_kind();
    const long                       live  = g_live;

    struct dj_math_value wide  = d_tests_wide_value(d_tests_power(70), 0u);
    struct dj_math_value three = d_math_value_from_int(3);
    struct dj_math_value out   = d_math_value_from_bool(true);
    struct dj_math_value r     = d_math_value_none();
    char                 text[8];
    int                  direction = 7;

    r = table->unary(D_MATH_VALUE_OP_NEG, three);
    D_TESTS_EXPECT(d_tests_is_error(r, D_MATH_VALUE_ERROR_DOMAIN, 0u),
                   "unary, no value of the kind", three, r);
    r = table->unary(D_MATH_VALUE_OP_ADD, wide);
    D_TESTS_EXPECT(d_tests_is_error(r, D_MATH_VALUE_ERROR_DOMAIN, 0u),
                   "unary, not its operation", wide, r);
    r = table->binary(D_MATH_VALUE_OP_ADD, three, three);
    D_TESTS_EXPECT(d_tests_is_error(r, D_MATH_VALUE_ERROR_DOMAIN, 0u),
                   "binary, no value of the kind", three, r);
    r = table->binary(D_MATH_VALUE_OP_NEG, wide, three);
    D_TESTS_EXPECT(d_tests_is_error(r, D_MATH_VALUE_ERROR_DOMAIN, 0u),
                   "binary, not its operation", wide, r);
    r = table->binary(D_MATH_VALUE_OP_ABS, three, wide);
    D_TESTS_EXPECT(d_tests_is_error(r, D_MATH_VALUE_ERROR_DOMAIN, 0u),
                   "binary, not its operation either", wide, r);
    r = table->binary(D_MATH_VALUE_OP_ADD, wide, d_math_value_from_d(1.5));
    D_TESTS_EXPECT(d_tests_is_error(r, D_MATH_VALUE_ERROR_DOMAIN, 0u),
                   "binary, an inline number it does not read", wide, r);
    r = table->binary(D_MATH_VALUE_OP_MUL, d_math_value_from_bool(true),
                      wide);
    D_TESTS_EXPECT(d_tests_is_error(r, D_MATH_VALUE_ERROR_DOMAIN, 0u),
                   "binary, a boolean", wide, r);
    D_TESTS_EXPECT(table->cmp(three, three) == D_MATH_VALUE_UNORDERED,
                   "cmp, no value of the kind", three, three);
    D_TESTS_EXPECT( (table->cmp(wide, d_math_value_none()) ==
                     D_MATH_VALUE_UNORDERED) &&
                    (table->cmp(d_math_value_from_f(2.0f), wide) ==
                     D_MATH_VALUE_UNORDERED),
                    "cmp, none, or an inline number it does not read", wide,
                    d_math_value_none());
    D_TESTS_EXPECT( (!table->convert(&out, &direction, three,
                                     D_MATH_VALUE_DOUBLE)) &&
                    (out.kind == D_MATH_VALUE_BOOLEAN) &&
                    (direction == 7),
                    "convert, no value of the kind", three, out);

    memset(text, '#', sizeof(text));
    D_TESTS_EXPECT( (table->text(text, sizeof(text), three) == 0u) &&
                    (text[0] == '\0') &&
                    (text[1] == '#'),
                    "text, no value of the kind", three,
                    d_math_value_none());
    memset(text, '#', sizeof(text));
    D_TESTS_EXPECT( (table->text(text, 0u, three) == 0u) &&
                    (text[0] == '#'),
                    "text, no value of the kind and no room", three,
                    d_math_value_none());

    d_math_value_release(&wide);

    D_TESTS_EXPECT(g_live == live, "nothing made by any of them", three,
                   three);

    return;
}

// 12.3    Construction and reading
//------------------------------------------------------------------------------

// d_tests_table_values
//   from_wide's negative 0 made 0; to_wide's exact integers, and nothing
// else; is_wide's values of the kind, and nothing else.
static void
d_tests_table_values(void)
{
    const struct dj_math_wide seven = d_math_wide_from_imax(7);

    struct dj_math_wide  zero = { { 0u }, true };
    struct dj_math_wide  read = seven;
    struct dj_math_value x    = d_math_value_from_wide(zero);
    struct dj_math_value fake = x;
    int                  i    = 0;

    g_shown = x;
    D_TESTS_EXPECT( (d_tests_is_fresh(x, d_math_value_wide_kind(), 0u)) &&
                    (d_tests_big_is_zero(
                         d_tests_big_of_integer(d_tests_held(x)))) &&
                    (!d_tests_held(x).negative),
                    "a negative 0 made 0", x, d_math_value_none());
    d_math_value_release(&x);
    g_shown = d_math_value_none();

    // to_wide: an inline integer, and a small's
    D_TESTS_EXPECT( (d_math_value_to_wide(&read, d_math_value_from_int(-12)))
                    && (d_math_wide_eq(read, d_math_wide_from_imax(-12))),
                    "an inline integer read as a wide integer",
                    d_math_value_from_int(-12), d_math_value_none());

    for (i = 0; i < g_other_count; ++i)
    {
        struct dj_tests_exact exact = d_tests_exact_of(0, 1);
        const bool            whole =
            ( (d_tests_value_exact(g_others[i], &exact)) &&
              (d_tests_big_cmp_mag(exact.d, d_tests_one()) == 0) );
        const bool            ok    = d_math_value_to_wide(&read, g_others[i]);

        D_TESTS_EXPECT( ( (whole) &&
                          (ok) &&
                          (d_tests_big_cmp(d_tests_big_of_integer(read),
                                           exact.n) == 0) ) ||
                        ( (!whole) &&
                          (!ok) ),
                        "read as a wide integer where it is an integer",
                        g_others[i], d_math_value_none());
        D_TESTS_EXPECT(!d_math_value_is_wide(g_others[i]),
                       "no wide integer", g_others[i], d_math_value_none());

        read = seven;
    }

    D_TESTS_EXPECT( (!d_math_value_to_wide(&read,
                                           d_math_value_from_ratio(1, 2))) &&
                    (d_math_wide_eq(read, seven)),
                    "a rational is none, and nothing written",
                    d_math_value_from_ratio(1, 2), d_math_value_none());

    // is_wide: a boxed value with no box, or no table, or another rank
    fake = d_tests_wide_value(d_tests_power(3), 0u);
    x    = fake;
    x.box = NULL;
    D_TESTS_EXPECT(!d_math_value_is_wide(x), "no box: no wide integer", fake,
                   d_math_value_none());
    x      = fake;
    x.type = NULL;
    D_TESTS_EXPECT(!d_math_value_is_wide(x), "no table: no wide integer",
                   fake, d_math_value_none());
    x      = fake;
    x.kind = D_MATH_VALUE_INTEGER;
    D_TESTS_EXPECT(!d_math_value_is_wide(x), "not boxed: no wide integer",
                   fake, d_math_value_none());
    x      = fake;
    x.type = &g_kind_small;
    D_TESTS_EXPECT(!d_math_value_is_wide(x), "a rank below: no wide integer",
                   fake, d_math_value_none());
    x      = fake;
    x.type = &g_kind_above;
    D_TESTS_EXPECT(!d_math_value_is_wide(x), "a rank above: no wide integer",
                   fake, d_math_value_none());
    d_math_value_release(&fake);

    return;
}


// 12.4    The internals, given buffers already written
//------------------------------------------------------------------------------
// The exact arithmetic writes every limb of its result, and reading an
// operand writes both its terms, whatever the buffers held before; where
// either fails, it writes nothing. Each is given a buffer full of a pattern.

// d_tests_dirty_ratio
//   a ratio full of the pattern: every limb 0xA5..., and negative.
static struct dj_internal_math_value_wide_ratio
d_tests_dirty_ratio(void)
{
    struct dj_internal_math_value_wide_ratio result = { false, { 0u }, { 0u } };
    int                                      i      = 0;

    for (i = 0; i < D_TESTS_RATIO; ++i)
    {
        result.num[i] = (dj_math_wide_limb)0xA5A5A5A5UL;
        result.den[i] = (dj_math_wide_limb)0x5A5A5A5AUL;
    }

    result.negative = true;

    return result;
}

// d_tests_same_ratio
//   whether _a and _b hold the same terms and sign, limb for limb.
static bool
d_tests_same_ratio(
    const struct dj_internal_math_value_wide_ratio* _a,
    const struct dj_internal_math_value_wide_ratio* _b
)
{
    int i = 0;

    for (i = 0; i < D_TESTS_RATIO; ++i)
    {
        if ( (_a->num[i] != _b->num[i]) ||
             (_a->den[i] != _b->den[i]) )
        {
            return false;
        }
    }

    return (_a->negative == _b->negative);
}

// d_tests_dirty_reads
//   each operand read into a term full of the pattern: both terms written
// where the kind reads it, and nothing otherwise.
static void
d_tests_dirty_reads(void)
{
    const struct dj_internal_math_value_wide_term dirty = {
        { { (dj_math_wide_limb)0xA5A5A5A5UL }, true }, 99u
    };

    int i = 0;

    for (i = 0; i < (g_wide_count + g_other_count); ++i)
    {
        const struct dj_math_value x = (i < g_wide_count)
                                          ? g_wides[i]
                                          : g_others[i - g_wide_count];

        struct dj_internal_math_value_wide_term term  = dirty;
        struct dj_tests_exact                   exact = d_tests_exact_of(0, 1);
        const bool readable = d_tests_value_exact(x,  &exact);
        const bool ok       = d_internal_math_value_wide_read(&term, x);

        D_TESTS_EXPECT( ( (readable) &&
                          (ok) &&
                          (d_tests_wide_in_one_form(term.num)) &&
                          (d_tests_exact_cmp(
                               d_tests_exact_from(
                                   d_tests_big_of_integer(term.num),
                                   d_tests_big_of_umax(term.den, false)),
                               exact) == 0) &&
                          (term.den == ( (x.kind == D_MATH_VALUE_RATIONAL)
                                             ? (dj_math_umax)x.exact.den
                                             : 1u )) ) ||
                        ( (!readable) &&
                          (!ok) &&
                          (d_math_wide_eq(term.num, dirty.num)) &&
                          (term.den == dirty.den) ),
                        "an operand read into a written term", x,
                        d_math_value_none());
    }

    return;
}

// d_tests_dirty_results
//   each operation on pairs of operands, its result written over the
// pattern: the same ratio as over zeros, and over 0 the pattern untouched.
static void
d_tests_dirty_results(void)
{
    const struct dj_internal_math_value_wide_ratio pattern =
        d_tests_dirty_ratio();

    int i  = 0;
    int j  = 0;
    int op = 0;

    for (i = 0; i < g_wide_count; i = i + 3)
    {
        for (j = 0; j < (g_wide_count + g_other_count); j = j + 2)
        {
            const struct dj_math_value y = (j < g_wide_count)
                                              ? g_wides[j]
                                              : g_others[j - g_wide_count];

            struct dj_internal_math_value_wide_term a =
                { { { 0u }, false }, 1u };
            struct dj_internal_math_value_wide_term b =
                { { { 0u }, false }, 1u };

            // operands the kind reads only
            if ( (!d_internal_math_value_wide_read(&a, g_wides[i])) ||
                 (!d_internal_math_value_wide_read(&b, y)) )
            {
                continue;
            }

            for (op = (int)D_MATH_VALUE_OP_ADD;
                 op <= (int)D_MATH_VALUE_OP_DIV; ++op)
            {
                struct dj_internal_math_value_wide_ratio clean =
                    { false, { 0u }, { 0u } };
                struct dj_internal_math_value_wide_ratio dirty = pattern;
                const enum dj_math_wide_status first =
                    d_internal_math_value_wide_exact(
                        &clean, (enum dj_math_value_op)op, a, b);
                const enum dj_math_wide_status second =
                    d_internal_math_value_wide_exact(
                        &dirty, (enum dj_math_value_op)op, a, b);

                D_TESTS_EXPECT( (first == second) &&
                                ( ( (first == D_MATH_WIDE_OK) &&
                                    (d_tests_same_ratio(&clean, &dirty)) ) ||
                                  ( (first != D_MATH_WIDE_OK) &&
                                    (d_tests_same_ratio(&pattern,
                                                        &dirty)) ) ),
                                "a result written over the pattern",
                                g_wides[i], y);
            }
        }
    }

    return;
}


//==============================================================================
// 13.  CONSTANT EVALUATION
//==============================================================================
// From C++14 the exact arithmetic, the order and the rounding are constant
// expressions: each is checked here at compile time, and the same calls at
// run time.

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// d_tests_ce_term
//   _num / _den as a term.
static constexpr struct dj_internal_math_value_wide_term
d_tests_ce_term(
    dj_math_imax _num,
    dj_math_imax _den
)
{
    struct dj_internal_math_value_wide_term result = { { { 0u }, false }, 1u };

    result.num = d_math_wide_from_imax(_num);
    result.den = (dj_math_umax)_den;

    return result;
}

// d_tests_ce_top
//   2^255 as a term.
static constexpr struct dj_internal_math_value_wide_term
d_tests_ce_top(void)
{
    struct dj_internal_math_value_wide_term result = { { { 0u }, false }, 1u };

    result.num.limb[D_MATH_WIDE_LIMBS - 1u] =
        (dj_math_wide_limb)((dj_math_wide_limb)1u
                            << (D_MATH_WIDE_LIMB_BITS - 1u));

    return result;
}

// d_tests_ce_carry
//   2^255 + 2^255 is 2^256, a limb past a wide integer's.
static constexpr bool
d_tests_ce_carry(void)
{
    struct dj_internal_math_value_wide_ratio r = { false, { 0u }, { 0u } };

    (void)d_internal_math_value_wide_exact(&r, D_MATH_VALUE_OP_ADD,
                                           d_tests_ce_top(),
                                           d_tests_ce_top());

    return ( (r.num[D_MATH_WIDE_LIMBS] == 1u) &&
             (r.num[D_MATH_WIDE_LIMBS - 1u] == 0u) &&
             (r.den[0] == 1u) &&
             (!r.negative) );
}

// d_tests_ce_quotient
//   6 / -4 is -3/2.
static constexpr bool
d_tests_ce_quotient(void)
{
    struct dj_internal_math_value_wide_ratio r = { false, { 0u }, { 0u } };

    (void)d_internal_math_value_wide_exact(&r, D_MATH_VALUE_OP_DIV,
                                           d_tests_ce_term(6, 1),
                                           d_tests_ce_term(-4, 1));

    return ( (r.num[0] == 3u) &&
             (r.num[1] == 0u) &&
             (r.den[0] == 2u) &&
             (r.den[1] == 0u) &&
             (r.negative) );
}

// d_tests_ce_pole
//   1 / 0.
static constexpr enum dj_math_wide_status
d_tests_ce_pole(void)
{
    struct dj_internal_math_value_wide_ratio r = { false, { 0u }, { 0u } };

    return d_internal_math_value_wide_exact(&r, D_MATH_VALUE_OP_DIV,
                                            d_tests_ce_term(1, 1),
                                            d_tests_ce_term(0, 1));
}

// d_tests_ce_order
//   1/3 against 1/2.
static constexpr int
d_tests_ce_order(void)
{
    return d_internal_math_value_wide_order(d_tests_ce_term(1, 3),
                                            d_tests_ce_term(1, 2));
}

// d_tests_ce_third
//   1/3 as the nearest long double.
static constexpr long double
d_tests_ce_third(void)
{
    struct dj_internal_math_value_wide_ratio r = { false, { 0u }, { 0u } };

    r.num[0] = 1u;
    r.den[0] = 3u;

    return d_internal_math_value_wide_nearest(&r);
}

// d_tests_ce_square
//   (2^255)^2 / 1, 2^510, as the nearest long double; and 2^510 by repeated
// squaring.
static constexpr long double
d_tests_ce_square(void)
{
    struct dj_internal_math_value_wide_ratio r = { false, { 0u }, { 0u } };

    (void)d_internal_math_value_wide_exact(&r, D_MATH_VALUE_OP_MUL,
                                           d_tests_ce_top(),
                                           d_tests_ce_top());

    return d_internal_math_value_wide_nearest(&r);
}

static constexpr long double
d_tests_ce_power(
    int _k
)
{
    long double result = 1.0L;
    int         i      = 0;

    for (i = 0; i < _k; ++i)
    {
        result = result * 2.0L;
    }

    return result;
}

static_assert(d_tests_ce_carry(),
              "2^255 + 2^255 must carry into a limb past the integer's");
static_assert(d_tests_ce_quotient(), "6 / -4 must be -3/2");
static_assert(d_tests_ce_pole() == D_MATH_WIDE_POLE, "1 / 0 must be a pole");
static_assert(d_tests_ce_order() == -1, "1/3 must be below 1/2");
static_assert(d_tests_ce_third() == (1.0L / 3.0L),
              "1/3 must round as the compiler rounds it");
static_assert(d_tests_ce_square() == d_tests_ce_power(510),
              "(2^255)^2 must be 2^510 exactly");
static_assert(!d_math_value_is_wide(d_math_value_from_int(3)),
              "an inline integer is no wide integer");

// d_tests_constant_evaluation
//   the same calls at run time.
static void
d_tests_constant_evaluation(void)
{
    D_TESTS_EXPECT( (d_tests_ce_carry()) &&
                    (d_tests_ce_quotient()) &&
                    (d_tests_ce_pole() == D_MATH_WIDE_POLE) &&
                    (d_tests_ce_order() == -1) &&
                    (d_tests_ce_third() == (1.0L / 3.0L)) &&
                    (d_tests_ce_square() == d_tests_ce_power(510)),
                    "the constant expressions, at run time",
                    d_math_value_none(), d_math_value_none());

    return;
}

#endif  // C++14


int
main(void)
{
    g_shown = d_math_value_none();

    printf("t_value_wide: limbs of %d bits, words of %d, long double of %d\n",
           D_TESTS_H, D_TESTS_W, (int)LDBL_MANT_DIG);

    d_tests_make_wides();
    d_tests_make_others();
    d_tests_table_held();
    d_tests_table_copies();
    d_tests_table_breaches();
    d_tests_table_values();
    d_tests_dirty_reads();
    d_tests_dirty_results();
    d_tests_ones();
    d_tests_pairs();
    d_tests_refused();
    d_tests_random_trials();
    d_tests_rounding();
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif
    d_tests_release_operands();

    // every box given back, and the reference within its limbs
    D_TESTS_EXPECT( (g_live == 0L) &&
                    (g_kind_live == 0L),
                    "every box given back", d_math_value_none(),
                    d_math_value_none());
    D_TESTS_EXPECT(!g_reference_overrun, "the reference's 768 bits held",
                   d_math_value_none(), d_math_value_none());

    printf("t_value_wide: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0L) ? 0 : 1;
}
