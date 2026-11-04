/*******************************************************************************
* djinterp [c/math]                                              t_value_boxed.c
*
*   Conformance harness for value.h's boxed kinds (c/math/value.h): dispatch
* through a kind's table, ownership, and the inline paths a boxed operand
* takes, with four kinds the harness registers itself.
*   Three of them mirror inline values, each boxed at a rank of its own:
* "small" holds an integer, at rank 150, between the integer's and the
* rational's; "exact" an integer or a rational, at 250, between the
* rational's and the float's; "inexact" a long double, at 600, past the long
* double's. Each computes with value.h on the values it holds, so value.h's
* inline arithmetic is the reference: an operation with boxed operands must
* give what it gives on the inline values they stand for, with every flag,
* held in a box where the kind that handled it keeps such a result (small an
* integer, exact an exact number, inexact a long double) and inline
* otherwise. Their conversions to float, double and long double are
* rational.h's, or C's for a long double, each rounding's direction found
* exactly. The fourth kind, "pair", at 1000, holds two values of any kinds
* and is no number: it computes part by part.
*   Each check also counts the calls into the tables. The operand of the
* higher rank must handle an operation, a boxed one through one call of its
* own entry; where an inline number is the higher, the boxed operand's
* conversion must be called once, to that number's kind, or for a
* comparison to an exact kind or the long double. An error, a boolean or
* none must be settled with no call at all. A kind handed an operand the
* protocol would not give it counts a breach.
*   Ownership: every box is counted. An operation leaves its operands' holds
* as they were, and its result holds new boxes only, which releasing it frees;
* retain and release add and drop one hold; and once the harness has
* released everything, no box is left. Where storage is refused, a result
* is a memory error, and nothing leaks. Two copies of one table, as two
* translation units would have in C, are one kind.
*   The operands: 69 inline values, and 57 boxed ones: mirrors of some of
* them, some flagged and two sharing a box, and five pairs. Each pair of
* operands with a boxed one among them, for every operation and the
* comparison; each operand alone, for neg, abs, the conversions, the
* queries, and retain and release. Then random pairs of exact numbers,
* boxed and not, with the floating-point values nearest them and their
* neighbours. From C++14 the new queries are also checked as constant
* expressions.
*   A kind's exact entry, asked only of a number whose traits leave its
* exactness to each value (S325), is checked with kinds of each trait; and
* value.h's count of a box's holders, atomic as cfg_value.h resolves it
* (S326).
*   The same file compiles as C++ (-x c++), at any level from C++98.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/math/t_value_boxed.c -o t_value_boxed -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_value_boxed.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.31
*******************************************************************************/
// std
#include <float.h>   // FLT_MAX, DBL_MAX, LDBL_MAX
#include <math.h>    // nextafterf, nextafter, nextafterl
#include <stdio.h>   // printf, sprintf
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, strcmp, strlen
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/value.h"  // unit under test
// tests
#include "./math_tests_exact.h"  // the exact reference


// the family's ends, and 2^(w/2), where the product of two terms passes a word
#define D_TESTS_MOST   D_MATH_IMAX_MAX
#define D_TESTS_LEAST  D_MATH_IMAX_MIN
#define D_TESTS_HALF                                                           \
    ((d_math_imax)((d_math_umax)1 << D_INTERNAL_RATIONAL_HALF_BITS))

// the random trials
#define D_TESTS_TRIALS  4000L

// the harness's kinds, as it numbers them, and their ranks
#define D_TESTS_SMALL    0
#define D_TESTS_EXACT    1
#define D_TESTS_INEXACT  2
#define D_TESTS_PAIR     3
#define D_TESTS_KINDS    4

#define D_TESTS_RANK_SMALL    150
#define D_TESTS_RANK_EXACT    250
#define D_TESTS_RANK_INEXACT  600
#define D_TESTS_RANK_PAIR     1000

// a table's entries, as the harness counts the calls into them
#define D_TESTS_RETAIN   0
#define D_TESTS_RELEASE  1
#define D_TESTS_UNARY    2
#define D_TESTS_BINARY   3
#define D_TESTS_CMP      4
#define D_TESTS_CONVERT  5
#define D_TESTS_TEXT     6
#define D_TESTS_ENTRIES  7

// the room for a value's text
#define D_TESTS_TEXT_ROOM  400

static long g_checks   = 0;
static long g_failures = 0;

// the calls into each kind's entries, and each kind's last conversion's kind
static long                   g_calls[D_TESTS_KINDS][D_TESTS_ENTRIES];
static enum d_math_value_kind g_target[D_TESTS_KINDS];

// boxes held and made; operands the protocol should not have given a kind;
// allocations to come before one is refused, -1 for none
static long g_live     = 0;
static long g_made     = 0;
static long g_breaches = 0;
static long g_refuse   = -1;


//==============================================================================
// 1.  THE KINDS
//==============================================================================
// A box holds two values: a mirror kind's in part[0], a pair's in both. Its
// holders are counted; the last release frees it and its parts.

// d_tests_box
//   struct: a box of one of the harness's kinds.
struct d_tests_box
{
    long                refs;     // its holders
    int                 kind;     // D_TESTS_SMALL ... D_TESTS_PAIR
    struct d_math_value part[2];  // what it holds
};

// the entries, defined below
static void
d_tests_box_retain(void* _box);
static void
d_tests_box_release(void* _box);
static struct d_math_value
d_tests_mirror_unary(enum d_math_value_op _op,
                     struct d_math_value  _x);
static struct d_math_value
d_tests_box_small_binary(enum d_math_value_op _op,
                         struct d_math_value  _a,
                         struct d_math_value  _b);
static struct d_math_value
d_tests_box_exact_binary(enum d_math_value_op _op,
                         struct d_math_value  _a,
                         struct d_math_value  _b);
static struct d_math_value
d_tests_box_inexact_binary(enum d_math_value_op _op,
                           struct d_math_value  _a,
                           struct d_math_value  _b);
static enum d_math_value_order
d_tests_box_small_cmp(struct d_math_value _a,
                      struct d_math_value _b);
static enum d_math_value_order
d_tests_box_exact_cmp(struct d_math_value _a,
                      struct d_math_value _b);
static enum d_math_value_order
d_tests_box_inexact_cmp(struct d_math_value _a,
                        struct d_math_value _b);
static bool
d_tests_mirror_convert(struct d_math_value*   _out,
                       int*                   _direction,
                       struct d_math_value    _x,
                       enum d_math_value_kind _to);
static struct d_math_value
d_tests_pair_unary(enum d_math_value_op _op,
                   struct d_math_value  _x);
static struct d_math_value
d_tests_pair_binary(enum d_math_value_op _op,
                    struct d_math_value  _a,
                    struct d_math_value  _b);
static enum d_math_value_order
d_tests_pair_cmp(struct d_math_value _a,
                 struct d_math_value _b);
static bool
d_tests_pair_convert(struct d_math_value*   _out,
                     int*                   _direction,
                     struct d_math_value    _x,
                     enum d_math_value_kind _to);
static size_t
d_tests_box_text(char*               _buffer,
                 size_t              _size,
                 struct d_math_value _x);

// the four tables; and a second copy of exact's, as a second translation
// unit would hold in C
static const struct d_math_boxed_kind g_kind_small = {
    "small", D_TESTS_RANK_SMALL, D_MATH_BOXED_NUMBER | D_MATH_BOXED_EXACT,
    d_tests_box_retain, d_tests_box_release, d_tests_mirror_unary,
    d_tests_box_small_binary, d_tests_box_small_cmp, d_tests_mirror_convert,
    d_tests_box_text, NULL
};

static const struct d_math_boxed_kind g_kind_exact = {
    "exact", D_TESTS_RANK_EXACT, D_MATH_BOXED_NUMBER | D_MATH_BOXED_EXACT,
    d_tests_box_retain, d_tests_box_release, d_tests_mirror_unary,
    d_tests_box_exact_binary, d_tests_box_exact_cmp, d_tests_mirror_convert,
    d_tests_box_text, NULL
};

static const struct d_math_boxed_kind g_kind_inexact = {
    "inexact", D_TESTS_RANK_INEXACT, D_MATH_BOXED_NUMBER,
    d_tests_box_retain, d_tests_box_release, d_tests_mirror_unary,
    d_tests_box_inexact_binary, d_tests_box_inexact_cmp, d_tests_mirror_convert,
    d_tests_box_text, NULL
};

static const struct d_math_boxed_kind g_kind_pair = {
    "pair", D_TESTS_RANK_PAIR, 0u,
    d_tests_box_retain, d_tests_box_release, d_tests_pair_unary,
    d_tests_pair_binary, d_tests_pair_cmp, d_tests_pair_convert,
    d_tests_box_text, NULL
};

static const struct d_math_boxed_kind g_kind_exact_copy = {
    "exact", D_TESTS_RANK_EXACT, D_MATH_BOXED_NUMBER | D_MATH_BOXED_EXACT,
    d_tests_box_retain, d_tests_box_release, d_tests_mirror_unary,
    d_tests_box_exact_binary, d_tests_box_exact_cmp, d_tests_mirror_convert,
    d_tests_box_text, NULL
};

static const struct d_math_boxed_kind* const g_tables[D_TESTS_KINDS] = {
    &g_kind_small, &g_kind_exact, &g_kind_inexact, &g_kind_pair
};


// 1.1    Boxes
//------------------------------------------------------------------------------

// d_tests_box_of
//   _x's box: _x is of one of the harness's kinds.
static struct d_tests_box*
d_tests_box_of(
    struct d_math_value _x
)
{
    return (struct d_tests_box*)_x.box;
}

// d_tests_index
//   which of the harness's kinds _x is, known by its rank as the kinds know
// their own; -1 for anything else, a boxed value without a table or a box
// among them.
static int
d_tests_index(
    struct d_math_value _x
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

    // by rank
    for (k = 0; k < D_TESTS_KINDS; ++k)
    {
        if (_x.type->rank == g_tables[k]->rank)
        {
            return k;
        }
    }

    return -1;
}

// d_tests_drop
//   a result given back, and none in its place; one that is boxed but none
// of the harness's, which only a fault can make, is left as it is, since
// releasing it would read what is not there.
static void
d_tests_drop(
    struct d_math_value* _x
)
{
    // whole, or not
    if ( (_x->kind != D_MATH_VALUE_BOXED) ||
         (d_tests_index(*_x) >= 0) )
    {
        d_math_value_release(_x);
    }

    *_x = d_math_value_none();

    return;
}

// d_tests_new_box
//   a value of the harness's kind _kind, under _table, holding _first and
// _second, with one holder; the box takes the caller's holds on them. A
// memory error, taking nothing, where storage is refused.
static struct d_math_value
d_tests_new_box(
    int                             _kind,
    const struct d_math_boxed_kind* _table,
    struct d_math_value             _first,
    struct d_math_value             _second
)
{
    struct d_math_value result = d_math_value_none();
    struct d_tests_box* box    = NULL;

    // refused on purpose
    if (g_refuse == 0)
    {
        g_refuse = -1;

        return d_math_value_from_error(D_MATH_VALUE_ERROR_MEMORY);
    }

    if (g_refuse > 0)
    {
        --g_refuse;
    }

    box = (struct d_tests_box*)malloc(sizeof(struct d_tests_box));

    // refused by the system
    if (box == NULL)
    {
        return d_math_value_from_error(D_MATH_VALUE_ERROR_MEMORY);
    }

    box->refs    = 1;
    box->kind    = _kind;
    box->part[0] = _first;
    box->part[1] = _second;
    ++g_live;
    ++g_made;

    result.kind = D_MATH_VALUE_BOXED;
    result.type = _table;
    result.box  = box;

    return result;
}

static void
d_tests_box_retain(
    void* _box
)
{
    struct d_tests_box* box = (struct d_tests_box*)_box;

    ++g_calls[box->kind][D_TESTS_RETAIN];
    ++box->refs;

    return;
}

static void
d_tests_box_release(
    void* _box
)
{
    struct d_tests_box* box = (struct d_tests_box*)_box;

    ++g_calls[box->kind][D_TESTS_RELEASE];
    --box->refs;

    // the last holder: its parts, then the box
    if (box->refs == 0)
    {
        d_tests_drop(&box->part[0]);
        d_tests_drop(&box->part[1]);
        free(box);
        --g_live;
    }

    return;
}

// d_tests_refs
//   the holders of _x's box; 0 for an inline value.
static long
d_tests_refs(
    struct d_math_value _x
)
{
    return (d_tests_index(_x) >= 0) ? d_tests_box_of(_x)->refs : 0L;
}

// d_tests_boxes_in
//   the boxes _x holds: its own, and a pair's parts'.
static long
d_tests_boxes_in(
    struct d_math_value _x
)
{
    // inline, or none of the harness's
    if (d_tests_index(_x) < 0)
    {
        return 0L;
    }

    return 1L + d_tests_boxes_in(d_tests_box_of(_x)->part[0]) +
           d_tests_boxes_in(d_tests_box_of(_x)->part[1]);
}

// 1.2    The mirror kinds
//------------------------------------------------------------------------------

// d_tests_apply
//   value.h's _a _op _b.
static struct d_math_value
d_tests_apply(
    enum d_math_value_op _op,
    struct d_math_value  _a,
    struct d_math_value  _b
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

// d_tests_rank_of
//   _x's rank, as the harness states the ranks: the inline numbers' 100 to
// 500, a boxed kind's own, 0 for none, an error or a boolean.
static int
d_tests_rank_of(
    struct d_math_value _x
)
{
    // by kind
    switch (_x.kind)
    {
        case D_MATH_VALUE_BOXED:
            return _x.type->rank;

        case D_MATH_VALUE_INTEGER:
            return 100;

        case D_MATH_VALUE_RATIONAL:
            return 200;

        case D_MATH_VALUE_FLOAT:
            return 300;

        case D_MATH_VALUE_DOUBLE:
            return 400;

        case D_MATH_VALUE_LONG_DOUBLE:
            return 500;

        default:
            return 0;
    }
}

// d_tests_keeps
//   whether the kind _kind keeps a result like _r in a box: small an
// integer, exact an integer or a rational, inexact a long double.
static bool
d_tests_keeps(
    int                 _kind,
    struct d_math_value _r
)
{
    // by kind
    if (_kind == D_TESTS_SMALL)
    {
        return (_r.kind == D_MATH_VALUE_INTEGER);
    }

    if (_kind == D_TESTS_EXACT)
    {
        return ( (_r.kind == D_MATH_VALUE_INTEGER) ||
                 (_r.kind == D_MATH_VALUE_RATIONAL) );
    }

    return ( (_kind == D_TESTS_INEXACT) &&
             (_r.kind == D_MATH_VALUE_LONG_DOUBLE) );
}

// d_tests_mirror_result
//   the result _r as the kind _kind gives it: in a box of its own, with _r's
// flags on the value, where it keeps such results; as it is otherwise.
static struct d_math_value
d_tests_mirror_result(
    int                 _kind,
    struct d_math_value _r
)
{
    struct d_math_value held   = _r;
    struct d_math_value result = _r;

    // kept
    if (d_tests_keeps(_kind, _r))
    {
        held.flags   = 0u;
        result       = d_tests_new_box(_kind, g_tables[_kind], held,
                                       d_math_value_none());
        result.flags = _r.flags;
    }

    return result;
}

// d_tests_inline_of
//   the inline number a kind of rank _rank computes with for the operand _x:
// an inline number as it is; a value of its own kind, what its box holds; a
// lower boxed number, through that kind's table, exactly where it is exact,
// else as the nearest long double. Anything else is an operand the protocol
// would not give the kind: a breach, and none.
static struct d_math_value
d_tests_inline_of(
    struct d_math_value _x,
    int                 _rank
)
{
    struct d_math_value result    = d_math_value_none();
    int                 direction = 0;

    // an inline number, no higher
    if (_x.kind != D_MATH_VALUE_BOXED)
    {
        if ( (d_math_value_is_number(_x)) &&
             (d_tests_rank_of(_x) <= _rank) )
        {
            return _x;
        }

        ++g_breaches;

        return result;
    }

    // its own kind
    if (_x.type->rank == _rank)
    {
        return d_tests_box_of(_x)->part[0];
    }

    // a lower boxed number, through its table
    if ( (_x.type->rank < _rank) &&
         ((_x.type->traits & D_MATH_BOXED_NUMBER) != 0u) &&
         ( ( ((_x.type->traits & D_MATH_BOXED_EXACT) != 0u) &&
             (_x.type->convert(&result, &direction, _x,
                               D_MATH_VALUE_RATIONAL)) ) ||
           (_x.type->convert(&result, &direction, _x,
                             D_MATH_VALUE_LONG_DOUBLE)) ) )
    {
        return result;
    }

    ++g_breaches;

    return d_math_value_none();
}

// d_tests_mirror_binary
//   a mirror kind's _a _op _b: value.h's, on the inline numbers they stand
// for.
static struct d_math_value
d_tests_mirror_binary(
    int                  _kind,
    enum d_math_value_op _op,
    struct d_math_value  _a,
    struct d_math_value  _b
)
{
    const int rank = g_tables[_kind]->rank;

    ++g_calls[_kind][D_TESTS_BINARY];

    // one operand of this kind
    if ( (d_tests_index(_a) != _kind) &&
         (d_tests_index(_b) != _kind) )
    {
        ++g_breaches;
    }

    return d_tests_mirror_result(
        _kind, d_tests_apply(_op, d_tests_inline_of(_a, rank),
                             d_tests_inline_of(_b, rank)));
}

// d_tests_mirror_cmp
//   a mirror kind's _a against _b: value.h's, on the inline numbers.
static enum d_math_value_order
d_tests_mirror_cmp(
    int                 _kind,
    struct d_math_value _a,
    struct d_math_value _b
)
{
    const int rank = g_tables[_kind]->rank;

    ++g_calls[_kind][D_TESTS_CMP];

    // one operand of this kind
    if ( (d_tests_index(_a) != _kind) &&
         (d_tests_index(_b) != _kind) )
    {
        ++g_breaches;
    }

    return d_math_value_cmp(d_tests_inline_of(_a, rank),
                            d_tests_inline_of(_b, rank));
}

static struct d_math_value
d_tests_box_small_binary(
    enum d_math_value_op _op,
    struct d_math_value  _a,
    struct d_math_value  _b
)
{
    return d_tests_mirror_binary(D_TESTS_SMALL, _op, _a, _b);
}

static struct d_math_value
d_tests_box_exact_binary(
    enum d_math_value_op _op,
    struct d_math_value  _a,
    struct d_math_value  _b
)
{
    return d_tests_mirror_binary(D_TESTS_EXACT, _op, _a, _b);
}

static struct d_math_value
d_tests_box_inexact_binary(
    enum d_math_value_op _op,
    struct d_math_value  _a,
    struct d_math_value  _b
)
{
    return d_tests_mirror_binary(D_TESTS_INEXACT, _op, _a, _b);
}

static enum d_math_value_order
d_tests_box_small_cmp(
    struct d_math_value _a,
    struct d_math_value _b
)
{
    return d_tests_mirror_cmp(D_TESTS_SMALL, _a, _b);
}

static enum d_math_value_order
d_tests_box_exact_cmp(
    struct d_math_value _a,
    struct d_math_value _b
)
{
    return d_tests_mirror_cmp(D_TESTS_EXACT, _a, _b);
}

static enum d_math_value_order
d_tests_box_inexact_cmp(
    struct d_math_value _a,
    struct d_math_value _b
)
{
    return d_tests_mirror_cmp(D_TESTS_INEXACT, _a, _b);
}

// d_tests_mirror_unary
//   a mirror kind's -_x or |_x|: value.h's, on what it holds.
static struct d_math_value
d_tests_mirror_unary(
    enum d_math_value_op _op,
    struct d_math_value  _x
)
{
    const int                 kind = d_tests_box_of(_x)->kind;
    const struct d_math_value held = d_tests_box_of(_x)->part[0];

    ++g_calls[kind][D_TESTS_UNARY];

    // the two operations
    if (_op == D_MATH_VALUE_OP_NEG)
    {
        return d_tests_mirror_result(kind, d_math_value_neg(held));
    }

    if (_op == D_MATH_VALUE_OP_ABS)
    {
        return d_tests_mirror_result(kind, d_math_value_abs(held));
    }

    ++g_breaches;

    return d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
}

// d_tests_direction
//   how _r, a rounding of the inline number _held, stands to it: -1 below,
// 0 equal, 1 above; 0 for a NaN. An exact number by the exact reference.
static int
d_tests_direction(
    long double         _r,
    struct d_math_value _held
)
{
    // a NaN
    if (_r != _r)
    {
        return 0;
    }

    // exact: the reference's order of the number against _r, turned
    if (d_math_value_is_exact(_held))
    {
        return -d_tests_exact_cmp_real(_held.exact, _r);
    }

    return (_r > _held.real) ? 1 : ((_r < _held.real) ? -1 : 0);
}

// d_tests_mirror_convert
//   a mirror kind's conversion: to an exact kind, an exact value as it is;
// to a floating-point type, rational.h's rounding of an exact one, or C's
// conversion of a long double, and the rounding's direction.
static bool
d_tests_mirror_convert(
    struct d_math_value*   _out,
    int*                   _direction,
    struct d_math_value    _x,
    enum d_math_value_kind _to
)
{
    const int                 kind  = d_tests_box_of(_x)->kind;
    const struct d_math_value held  = d_tests_box_of(_x)->part[0];
    const bool                exact = d_math_value_is_exact(held);

    ++g_calls[kind][D_TESTS_CONVERT];
    g_target[kind] = _to;

    // to an exact kind
    if ( (_to == D_MATH_VALUE_INTEGER) ||
         (_to == D_MATH_VALUE_RATIONAL) )
    {
        if (!exact)
        {
            return false;
        }

        *_out       = held;
        *_direction = 0;

        return true;
    }

    // to a floating-point type, each value stored in its type
    if (_to == D_MATH_VALUE_FLOAT)
    {
        volatile float f = (exact) ? d_rational_to_f(held.exact)
                                   : (float)held.real;

        *_out = d_math_value_from_f(f);
    }
    else if (_to == D_MATH_VALUE_DOUBLE)
    {
        volatile double d = (exact) ? d_rational_to_d(held.exact)
                                    : (double)held.real;

        *_out = d_math_value_from_d(d);
    }
    else if (_to == D_MATH_VALUE_LONG_DOUBLE)
    {
        *_out = d_math_value_from_ld((exact) ? d_rational_to_ld(held.exact)
                                             : held.real);
    }
    else
    {
        ++g_breaches;

        return false;
    }

    *_direction = d_tests_direction(_out->real, held);

    return true;
}

// 1.3    The pair
//------------------------------------------------------------------------------

// d_tests_part
//   part _i of _x where it is a pair; _x itself otherwise.
static struct d_math_value
d_tests_part(
    struct d_math_value _x,
    int                 _i
)
{
    return (d_tests_index(_x) == D_TESTS_PAIR) ? d_tests_box_of(_x)->part[_i]
                                               : _x;
}

// d_tests_pair_of
//   a pair of _first and _second, with holds of its own on them; a memory
// error, and the holds given back, where storage is refused.
static struct d_math_value
d_tests_pair_of(
    struct d_math_value _first,
    struct d_math_value _second
)
{
    struct d_math_value first  = d_math_value_retain(_first);
    struct d_math_value second = d_math_value_retain(_second);
    struct d_math_value result =
        d_tests_new_box(D_TESTS_PAIR, &g_kind_pair, first, second);

    // no storage: the holds given back
    if (result.kind != D_MATH_VALUE_BOXED)
    {
        d_math_value_release(&first);
        d_math_value_release(&second);
    }

    return result;
}

// d_tests_pair_made
//   a pair of the results _first and _second, taking them; a memory error,
// and both released, where storage is refused.
static struct d_math_value
d_tests_pair_made(
    struct d_math_value _first,
    struct d_math_value _second
)
{
    struct d_math_value result =
        d_tests_new_box(D_TESTS_PAIR, &g_kind_pair, _first, _second);

    // no storage: the results given back
    if (result.kind != D_MATH_VALUE_BOXED)
    {
        d_tests_drop(&_first);
        d_tests_drop(&_second);
    }

    return result;
}

static struct d_math_value
d_tests_pair_binary(
    enum d_math_value_op _op,
    struct d_math_value  _a,
    struct d_math_value  _b
)
{
    struct d_math_value first  = d_math_value_none();
    struct d_math_value second = d_math_value_none();

    ++g_calls[D_TESTS_PAIR][D_TESTS_BINARY];

    // one operand a pair
    if ( (d_tests_index(_a) != D_TESTS_PAIR) &&
         (d_tests_index(_b) != D_TESTS_PAIR) )
    {
        ++g_breaches;
    }

    first  = d_tests_apply(_op, d_tests_part(_a, 0), d_tests_part(_b, 0));
    second = d_tests_apply(_op, d_tests_part(_a, 1), d_tests_part(_b, 1));

    return d_tests_pair_made(first, second);
}

static struct d_math_value
d_tests_pair_unary(
    enum d_math_value_op _op,
    struct d_math_value  _x
)
{
    const struct d_tests_box* box = d_tests_box_of(_x);

    ++g_calls[D_TESTS_PAIR][D_TESTS_UNARY];

    // the two operations
    if (_op == D_MATH_VALUE_OP_NEG)
    {
        return d_tests_pair_made(d_math_value_neg(box->part[0]),
                                 d_math_value_neg(box->part[1]));
    }

    if (_op == D_MATH_VALUE_OP_ABS)
    {
        return d_tests_pair_made(d_math_value_abs(box->part[0]),
                                 d_math_value_abs(box->part[1]));
    }

    ++g_breaches;

    return d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
}

// d_tests_pair_cmp
//   two pairs are equal where their parts are; nothing else is ordered.
static enum d_math_value_order
d_tests_pair_cmp(
    struct d_math_value _a,
    struct d_math_value _b
)
{
    ++g_calls[D_TESTS_PAIR][D_TESTS_CMP];

    // one operand a pair
    if ( (d_tests_index(_a) != D_TESTS_PAIR) &&
         (d_tests_index(_b) != D_TESTS_PAIR) )
    {
        ++g_breaches;
    }

    return ( (d_tests_index(_a) == D_TESTS_PAIR) &&
             (d_tests_index(_b) == D_TESTS_PAIR) &&
             (d_math_value_eq(d_tests_part(_a, 0), d_tests_part(_b, 0))) &&
             (d_math_value_eq(d_tests_part(_a, 1), d_tests_part(_b, 1))) )
               ? D_MATH_VALUE_EQUAL
               : D_MATH_VALUE_UNORDERED;
}

// d_tests_pair_convert
//   a pair is no number: value.h must never ask.
static bool
d_tests_pair_convert(
    struct d_math_value*   _out,
    int*                   _direction,
    struct d_math_value    _x,
    enum d_math_value_kind _to
)
{
    ++g_calls[D_TESTS_PAIR][D_TESTS_CONVERT];
    g_target[D_TESTS_PAIR] = _to;
    (void)_out;
    (void)_direction;
    (void)_x;

    return false;
}

// 1.4    Text
//------------------------------------------------------------------------------

// d_tests_inline_text
//   an inline value as text, into _buffer, returned: none, false, true,
// "error n", an integer's digits, "n/d", or a floating-point value's 21
// significant digits.
static const char*
d_tests_inline_text(
    struct d_math_value _x,
    char*               _buffer
)
{
    char num[24];
    char den[24];

    // by kind
    if (_x.kind == D_MATH_VALUE_NONE)
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
        sprintf(_buffer, "%s", d_tests_text(_x.exact.num, num));
    }
    else if (_x.kind == D_MATH_VALUE_RATIONAL)
    {
        sprintf(_buffer, "%s/%s", d_tests_text(_x.exact.num, num),
                d_tests_text(_x.exact.den, den));
    }
    else
    {
        sprintf(_buffer, "%.21Lg", _x.real);
    }

    return _buffer;
}

// d_tests_put
//   _text into _buffer as snprintf puts it: at most _size characters, the
// NUL among them; its whole length.
static size_t
d_tests_put(
    char*       _buffer,
    size_t      _size,
    const char* _text
)
{
    const size_t length = strlen(_text);

    // room for some of it
    if (_size > 0u)
    {
        const size_t count = (length < (_size - 1u)) ? length : (_size - 1u);

        memcpy(_buffer, _text, count);
        _buffer[count] = '\0';
    }

    return length;
}

// d_tests_any_text
//   any value as its text, into _buffer (D_TESTS_TEXT_ROOM), returned: a
// boxed one through its table.
static const char*
d_tests_any_text(
    struct d_math_value _x,
    char*               _buffer
)
{
    // boxed, whole or not; or inline
    if (_x.kind == D_MATH_VALUE_BOXED)
    {
        if (d_tests_index(_x) < 0)
        {
            sprintf(_buffer, "(a boxed value of no kind of the harness's)");
        }
        else
        {
            (void)_x.type->text(_buffer, D_TESTS_TEXT_ROOM, _x);
        }

        return _buffer;
    }

    return d_tests_inline_text(_x, _buffer);
}

// d_tests_box_text
//   a mirror kind's value as the value it holds; a pair as "(first, second)".
static size_t
d_tests_box_text(
    char*               _buffer,
    size_t              _size,
    struct d_math_value _x
)
{
    const struct d_tests_box* box = d_tests_box_of(_x);

    char text[D_TESTS_TEXT_ROOM];
    char first[D_TESTS_TEXT_ROOM];
    char second[D_TESTS_TEXT_ROOM];

    ++g_calls[box->kind][D_TESTS_TEXT];

    // a pair, or a mirror
    if (box->kind == D_TESTS_PAIR)
    {
        sprintf(text, "(%.180s, %.180s)",
                d_tests_any_text(box->part[0], first),
                d_tests_any_text(box->part[1], second));
    }
    else
    {
        (void)d_tests_inline_text(box->part[0], text);
    }

    return d_tests_put(_buffer, _size, text);
}


//==============================================================================
// 2.  REPORTING AND WHAT A RESULT MUST BE
//==============================================================================

// d_tests_value_text
//   _x for a report, into _buffer (at least 2 * D_TESTS_TEXT_ROOM): its
// kind's name, its text, its flags.
static const char*
d_tests_value_text(
    struct d_math_value _x,
    char*               _buffer
)
{
    char text[D_TESTS_TEXT_ROOM];

    sprintf(_buffer, "%s %s",
            (d_tests_index(_x) >= 0) ? _x.type->name
                                     : ( (_x.kind == D_MATH_VALUE_BOXED)
                                             ? "boxed"
                                             : "inline" ),
            d_tests_any_text(_x, text));

    // the flags, where there are any
    if (_x.flags != 0u)
    {
        sprintf(_buffer + strlen(_buffer), " (flags %u)", _x.flags);
    }

    return _buffer;
}

// d_tests_expect
//   counts a check, and reports the first few failures with their operands.
static void
d_tests_expect(
    bool                _ok,
    const char*         _what,
    struct d_math_value _a,
    struct d_math_value _b,
    int                 _line
)
{
    char text[2][2 * D_TESTS_TEXT_ROOM];

    ++g_checks;

    // the first failures, with their operands
    if (!_ok)
    {
        ++g_failures;

        if (g_failures <= 40)
        {
            printf("  FAIL line %d: %s (%s; %s)\n", _line, _what,
                   d_tests_value_text(_a, text[0]),
                   d_tests_value_text(_b, text[1]));
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what, a, b)                                         \
    d_tests_expect((ok), (what), (a), (b), __LINE__)

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
//   whether _a and _b are the same value: the same kind, flags and fields;
// boxed, of the same kind of the harness's, holding the same values.
static bool
d_tests_same(
    struct d_math_value _a,
    struct d_math_value _b
)
{
    // the fields
    if ( (_a.kind != _b.kind)                        ||
         (_a.flags != _b.flags)                      ||
         (_a.code != _b.code)                        ||
         (_a.exact.num != _b.exact.num)              ||
         (_a.exact.den != _b.exact.den)              ||
         (!d_tests_same_real(_a.real, _b.real)) )
    {
        return false;
    }

    // inline: no table, no box
    if (_a.kind != D_MATH_VALUE_BOXED)
    {
        return ( (_a.type == NULL) &&
                 (_b.type == NULL) &&
                 (_a.box == NULL) &&
                 (_b.box == NULL) );
    }

    return ( (d_tests_index(_a) >= 0) &&
             (d_tests_index(_b) >= 0) &&
             (d_tests_index(_a) == d_tests_index(_b)) &&
             (d_tests_same(d_tests_box_of(_a)->part[0],
                           d_tests_box_of(_b)->part[0])) &&
             (d_tests_same(d_tests_box_of(_a)->part[1],
                           d_tests_box_of(_b)->part[1])) );
}

// d_tests_is_boxed_as
//   whether _r is a value of the harness's kind _kind, its fields outside
// the box 0 but its flags, _flags, its box new and alone.
static bool
d_tests_is_boxed_as(
    struct d_math_value _r,
    int                 _kind,
    unsigned            _flags
)
{
    return ( (_r.kind == D_MATH_VALUE_BOXED) &&
             (d_tests_index(_r) == _kind) &&
             (_r.flags == _flags) &&
             (_r.code == 0) &&
             (_r.exact.num == 0) &&
             (_r.exact.den == 1) &&
             (d_tests_is_plus_zero(_r.real)) &&
             (_r.box != NULL) &&
             (d_tests_box_of(_r)->refs == 1) );
}

// d_tests_is_kept
//   whether _r is _want as the kind _kind gives it: in a box, holding _want
// without its flags, which are on _r, where the kind keeps such a result;
// _want itself otherwise.
static bool
d_tests_is_kept(
    struct d_math_value _r,
    int                 _kind,
    struct d_math_value _want
)
{
    struct d_math_value held = _want;

    // inline
    if (!d_tests_keeps(_kind, _want))
    {
        return d_tests_same(_r, _want);
    }

    held.flags = 0u;

    return ( (d_tests_is_boxed_as(_r, _kind, _want.flags)) &&
             (d_tests_same(d_tests_box_of(_r)->part[0], held)) &&
             (d_tests_same(d_tests_box_of(_r)->part[1],
                           d_math_value_none())) );
}

// d_tests_plain
//   the inline value a mirror kind's _x stands for, with _x's flags; an
// inline _x itself.
static struct d_math_value
d_tests_plain(
    struct d_math_value _x
)
{
    struct d_math_value result = _x;

    // what the box holds
    if (_x.kind == D_MATH_VALUE_BOXED)
    {
        result       = d_tests_box_of(_x)->part[0];
        result.flags = _x.flags;
    }

    return result;
}

// d_tests_handler
//   the kind the protocol has handle an operation on _a and _b, both numbers
// or boxed, one boxed: the harness's index of the operand of the higher
// rank where it is boxed, a boxed one where the ranks tie; -1 where an
// inline number is the higher.
static int
d_tests_handler(
    struct d_math_value _a,
    struct d_math_value _b
)
{
    const int a_rank = d_tests_rank_of(_a);
    const int b_rank = d_tests_rank_of(_b);

    // the boxed operand as high as the other
    if ( (_a.kind == D_MATH_VALUE_BOXED) &&
         (a_rank >= b_rank) )
    {
        return d_tests_index(_a);
    }

    if ( (_b.kind == D_MATH_VALUE_BOXED) &&
         (b_rank >= a_rank) )
    {
        return d_tests_index(_b);
    }

    return -1;
}

// d_tests_is_other
//   whether _x is none, an error or a boolean.
static bool
d_tests_is_other(
    struct d_math_value _x
)
{
    return ( (_x.kind == D_MATH_VALUE_NONE)  ||
             (_x.kind == D_MATH_VALUE_ERROR) ||
             (_x.kind == D_MATH_VALUE_BOOLEAN) );
}

// d_tests_count
//   struct: the counters at one moment.
struct d_tests_count
{
    long                   calls[D_TESTS_KINDS][D_TESTS_ENTRIES];  // the
                                                    // calls into the tables
    enum d_math_value_kind target[D_TESTS_KINDS];   // each kind's last
                                                    // conversion's kind
    long                   live;                    // the boxes held
    long                   breaches;                // the breaches
};

// d_tests_counted
//   the counters now.
static struct d_tests_count
d_tests_counted(void)
{
    struct d_tests_count result;

    memcpy(result.calls, g_calls, sizeof(g_calls));
    memcpy(result.target, g_target, sizeof(g_target));
    result.live     = g_live;
    result.breaches = g_breaches;

    return result;
}

// d_tests_calls
//   the calls into the kind _kind's entry _entry from _before to _after.
static long
d_tests_calls(
    const struct d_tests_count* _before,
    const struct d_tests_count* _after,
    int                         _kind,
    int                         _entry
)
{
    return _after->calls[_kind][_entry] - _before->calls[_kind][_entry];
}

// d_tests_calls_only
//   whether, from _before to _after, the only calls into any table are
// _count calls into the entry _entry of the kind _kind; _kind -1 for none
// at all.
static bool
d_tests_calls_only(
    const struct d_tests_count* _before,
    const struct d_tests_count* _after,
    int                         _kind,
    int                         _entry,
    long                        _count
)
{
    int k = 0;
    int e = 0;

    // every kind's every entry
    for (k = 0; k < D_TESTS_KINDS; ++k)
    {
        for (e = 0; e < D_TESTS_ENTRIES; ++e)
        {
            const long calls = d_tests_calls(_before, _after, k, e);

            if (calls != ( ( (k == _kind) &&
                             (e == _entry) ) ? _count : 0L ))
            {
                return false;
            }
        }
    }

    return true;
}

// d_tests_entry_calls_only
//   whether, from _before to _after, the entry _entry was called _count
// times in the kind _kind and never in another; the other entries are not
// counted.
static bool
d_tests_entry_calls_only(
    const struct d_tests_count* _before,
    const struct d_tests_count* _after,
    int                         _kind,
    int                         _entry,
    long                        _count
)
{
    int k = 0;

    // each kind
    for (k = 0; k < D_TESTS_KINDS; ++k)
    {
        if (d_tests_calls(_before, _after, k, _entry) !=
            ((k == _kind) ? _count : 0L))
        {
            return false;
        }
    }

    return true;
}

// d_tests_no_holds_moved
//   whether, from _before to _after, no hold was taken or given back in any
// table.
static bool
d_tests_no_holds_moved(
    const struct d_tests_count* _before,
    const struct d_tests_count* _after
)
{
    int k = 0;

    // each kind
    for (k = 0; k < D_TESTS_KINDS; ++k)
    {
        if ( (d_tests_calls(_before, _after, k, D_TESTS_RETAIN) != 0) ||
             (d_tests_calls(_before, _after, k, D_TESTS_RELEASE) != 0) )
        {
            return false;
        }
    }

    return true;
}


//==============================================================================
// 3.  ONE OPERATION, CHECKED
//==============================================================================

// d_tests_check_settled
//   whether _r is the error or domain error value.h must give _a _op _b
// itself, where an operand is none, an error or a boolean.
static bool
d_tests_check_settled(
    struct d_math_value _r,
    struct d_math_value _a,
    struct d_math_value _b
)
{
    const unsigned flags = _a.flags | _b.flags;

    // an error passes through, the first of two
    if ( (_a.kind == D_MATH_VALUE_ERROR) ||
         (_b.kind == D_MATH_VALUE_ERROR) )
    {
        const struct d_math_value first =
            (_a.kind == D_MATH_VALUE_ERROR) ? _a : _b;

        return ( (_r.kind == D_MATH_VALUE_ERROR) &&
                 (_r.code == first.code) &&
                 (_r.flags == flags) &&
                 (_r.type == NULL) &&
                 (_r.box == NULL) );
    }

    // a boolean or none: a domain error
    return ( (_r.kind == D_MATH_VALUE_ERROR) &&
             (_r.code == (int)D_MATH_VALUE_ERROR_DOMAIN) &&
             (_r.flags == flags) &&
             (_r.type == NULL) &&
             (_r.box == NULL) );
}

// d_tests_check_pair_result
//   whether _r is a pair of _first and _second, flagged _flags, as the pair
// kind gives it; both released after.
static bool
d_tests_check_pair_result(
    struct d_math_value _r,
    struct d_math_value _first,
    struct d_math_value _second,
    unsigned            _flags
)
{
    const bool ok = ( (d_tests_is_boxed_as(_r, D_TESTS_PAIR, _flags)) &&
                      (d_tests_same(d_tests_box_of(_r)->part[0], _first)) &&
                      (d_tests_same(d_tests_box_of(_r)->part[1], _second)) );

    d_tests_drop(&_first);
    d_tests_drop(&_second);

    return ok;
}

// d_tests_check_binary
//   _a _op _b, one of them boxed, against what it must be, one check: the
// result, the kind that handled it, the holds and the boxes.
static void
d_tests_check_binary(
    enum d_math_value_op _op,
    struct d_math_value  _a,
    struct d_math_value  _b
)
{
    static const char* const names[4] = { "add", "sub", "mul", "div" };

    const long     a_refs = d_tests_refs(_a);
    const long     b_refs = d_tests_refs(_b);
    const unsigned flags  = _a.flags | _b.flags;

    struct d_tests_count before;
    struct d_tests_count after;
    struct d_math_value  r;
    bool                 ok = false;

    // only where a boxed operand is: the rest is t_value.c's
    if ( (_a.kind != D_MATH_VALUE_BOXED) &&
         (_b.kind != D_MATH_VALUE_BOXED) )
    {
        return;
    }

    before = d_tests_counted();
    r      = d_tests_apply(_op, _a, _b);
    after  = d_tests_counted();

    // none, an error or a boolean: settled by value.h, no table called
    if ( (d_tests_is_other(_a)) ||
         (d_tests_is_other(_b)) )
    {
        ok = ( (d_tests_check_settled(r, _a, _b)) &&
               (d_tests_calls_only(&before, &after, -1, 0, 0L)) );
    }
    // a pair: the pair's, part by part (its parts may call any kind)
    else if ( (d_tests_index(_a) == D_TESTS_PAIR) ||
              (d_tests_index(_b) == D_TESTS_PAIR) )
    {
        ok = ( (d_tests_calls(&before, &after, D_TESTS_PAIR,
                              D_TESTS_BINARY) >= 1L) &&
               (d_tests_check_pair_result(
                   r,
                   d_tests_apply(_op, d_tests_part(_a, 0),
                                 d_tests_part(_b, 0)),
                   d_tests_apply(_op, d_tests_part(_a, 1),
                                 d_tests_part(_b, 1)),
                   flags)) );
    }
    // two numbers: the operand of the higher rank
    else
    {
        const struct d_math_value want =
            d_tests_apply(_op, d_tests_plain(_a), d_tests_plain(_b));
        const int handler = d_tests_handler(_a, _b);

        // a boxed kind's one call, its result kept or not
        if (handler >= 0)
        {
            ok = ( (d_tests_entry_calls_only(&before, &after, handler,
                                             D_TESTS_BINARY, 1L)) &&
                   (d_tests_is_kept(r, handler, want)) );
        }
        // an inline number's: the boxed operand converted to its kind
        else
        {
            const struct d_math_value boxed =
                (_a.kind == D_MATH_VALUE_BOXED) ? _a : _b;
            const struct d_math_value other =
                (_a.kind == D_MATH_VALUE_BOXED) ? _b : _a;
            const int low = d_tests_index(boxed);

            ok = ( (d_tests_calls_only(&before, &after, low,
                                       D_TESTS_CONVERT, 1L)) &&
                   (after.target[low] == other.kind) &&
                   (d_tests_same(r, want)) );
        }
    }

    // the operands' holds as they were, the result's boxes new
    ok = ( (ok) &&
           (d_tests_no_holds_moved(&before, &after)) &&
           (d_tests_refs(_a) == a_refs) &&
           (d_tests_refs(_b) == b_refs) &&
           (after.live == before.live + d_tests_boxes_in(r)) &&
           (after.breaches == before.breaches) );

    d_tests_drop(&r);

    D_TESTS_EXPECT( (ok) &&
                    (g_live == before.live),
                    names[_op], _a, _b);

    return;
}

// d_tests_check_unary
//   neg and abs of a boxed _x against what they must be, one check.
static void
d_tests_check_unary(
    struct d_math_value _x
)
{
    const int  kind = d_tests_index(_x);
    const long refs = d_tests_refs(_x);

    struct d_tests_count before;
    struct d_tests_count after;
    struct d_math_value  negated;
    struct d_math_value  absolute;
    bool                 ok = false;

    // only a boxed value
    if (_x.kind != D_MATH_VALUE_BOXED)
    {
        return;
    }

    before   = d_tests_counted();
    negated  = d_math_value_neg(_x);
    absolute = d_math_value_abs(_x);
    after    = d_tests_counted();

    // a pair, part by part (its parts may call any kind); a mirror, kept
    // or not
    if (kind == D_TESTS_PAIR)
    {
        ok = ( (d_tests_calls(&before, &after, D_TESTS_PAIR,
                              D_TESTS_UNARY) >= 2L) &&
               (d_tests_check_pair_result(
                   negated, d_math_value_neg(d_tests_part(_x, 0)),
                   d_math_value_neg(d_tests_part(_x, 1)), _x.flags)) &&
               (d_tests_check_pair_result(
                   absolute, d_math_value_abs(d_tests_part(_x, 0)),
                   d_math_value_abs(d_tests_part(_x, 1)), _x.flags)) );
    }
    else
    {
        ok = ( (d_tests_calls_only(&before, &after, kind, D_TESTS_UNARY,
                                   2L)) &&
               (d_tests_is_kept(negated, kind,
                                d_math_value_neg(d_tests_plain(_x)))) &&
               (d_tests_is_kept(absolute, kind,
                                d_math_value_abs(d_tests_plain(_x)))) );
    }

    ok = ( (ok) &&
           (d_tests_no_holds_moved(&before, &after)) &&
           (d_tests_refs(_x) == refs) &&
           (after.live == before.live + d_tests_boxes_in(negated) +
                          d_tests_boxes_in(absolute)) &&
           (after.breaches == before.breaches) );

    d_tests_drop(&negated);
    d_tests_drop(&absolute);

    D_TESTS_EXPECT( (ok) &&
                    (g_live == before.live),
                    "neg and abs", _x, _x);

    return;
}

// d_tests_check_compare
//   cmp both ways and eq on (_a, _b), one of them boxed, against what they
// must be, one check; and which kind compared them.
static void
d_tests_check_compare(
    struct d_math_value _a,
    struct d_math_value _b
)
{
    enum d_math_value_order want  = D_MATH_VALUE_UNORDERED;
    enum d_math_value_order got   = D_MATH_VALUE_UNORDERED;
    enum d_math_value_order back  = D_MATH_VALUE_UNORDERED;
    enum d_math_value_order turn  = D_MATH_VALUE_UNORDERED;
    struct d_tests_count    before;
    struct d_tests_count    after;
    bool                    ok    = false;

    // only where a boxed operand is
    if ( (_a.kind != D_MATH_VALUE_BOXED) &&
         (_b.kind != D_MATH_VALUE_BOXED) )
    {
        return;
    }

    before = d_tests_counted();
    got    = d_math_value_cmp(_a, _b);
    after  = d_tests_counted();

    // none, an error or a boolean: no order, no table called
    if ( (d_tests_is_other(_a)) ||
         (d_tests_is_other(_b)) )
    {
        want = D_MATH_VALUE_UNORDERED;
        ok   = d_tests_calls_only(&before, &after, -1, 0, 0L);
    }
    // a pair: equal to a pair with equal parts, else unordered (its parts
    // may call any kind)
    else if ( (d_tests_index(_a) == D_TESTS_PAIR) ||
              (d_tests_index(_b) == D_TESTS_PAIR) )
    {
        ok   = (d_tests_calls(&before, &after, D_TESTS_PAIR, D_TESTS_CMP) >=
                1L);
        want = ( (d_tests_index(_a) == D_TESTS_PAIR) &&
                 (d_tests_index(_b) == D_TESTS_PAIR) &&
                 (d_math_value_eq(d_tests_part(_a, 0),
                                  d_tests_part(_b, 0))) &&
                 (d_math_value_eq(d_tests_part(_a, 1),
                                  d_tests_part(_b, 1))) )
                   ? D_MATH_VALUE_EQUAL
                   : D_MATH_VALUE_UNORDERED;
    }
    // two numbers: as the inline numbers they stand for, exactly
    else
    {
        const int handler = d_tests_handler(_a, _b);

        want = d_math_value_cmp(d_tests_plain(_a), d_tests_plain(_b));

        // a boxed kind's one call; or the boxed operand converted, exactly
        // for an exact number, to the nearest long double for the others
        if (handler >= 0)
        {
            ok = d_tests_entry_calls_only(&before, &after, handler,
                                          D_TESTS_CMP, 1L);
        }
        else
        {
            const struct d_math_value boxed =
                (_a.kind == D_MATH_VALUE_BOXED) ? _a : _b;
            const struct d_math_value other =
                (_a.kind == D_MATH_VALUE_BOXED) ? _b : _a;
            const int low = d_tests_index(boxed);

            ok = ( (d_tests_calls_only(&before, &after, low,
                                       D_TESTS_CONVERT, 1L)) &&
                   (after.target[low] == (d_math_value_is_exact(other)
                                              ? D_MATH_VALUE_RATIONAL
                                              : D_MATH_VALUE_LONG_DOUBLE)) );
        }
    }

    ok   = ( (ok) &&
             (d_tests_no_holds_moved(&before, &after)) &&
             (after.live == before.live) &&
             (after.breaches == before.breaches) );
    back = d_math_value_cmp(_b, _a);
    turn = (want == D_MATH_VALUE_LESS)
               ? D_MATH_VALUE_GREATER
               : ( (want == D_MATH_VALUE_GREATER) ? D_MATH_VALUE_LESS
                                                  : want );

    D_TESTS_EXPECT( (ok) &&
                    (got == want) &&
                    (back == turn) &&
                    (d_math_value_eq(_a, _b) == (want == D_MATH_VALUE_EQUAL)),
                    "cmp and eq", _a, _b);

    return;
}

// d_tests_check_convert
//   to_f, to_d, to_ld and to_rational of a boxed _x, one check: a mirror
// kind's conversion called once for each, to that type, and giving what the
// inline number it stands for gives; to_rational of an inexact kind, and
// every conversion of a pair, refused without a call, the destination as
// it was.
static void
d_tests_check_convert(
    struct d_math_value _x
)
{
    const int kind = d_tests_index(_x);

    float                f    = 12345.0f;
    double               d    = 12345.0;
    long double          ld   = 12345.0L;
    struct d_rational    r    = { 12345, 0 };
    float                wf   = 12345.0f;
    double               wd   = 12345.0;
    long double          wld  = 12345.0L;
    struct d_rational    wr   = { 12345, 0 };
    struct d_tests_count before;
    struct d_tests_count after;
    bool                 ok   = true;

    // only a boxed value
    if (_x.kind != D_MATH_VALUE_BOXED)
    {
        return;
    }

    // a pair: no number, no call
    if (kind == D_TESTS_PAIR)
    {
        before = d_tests_counted();
        ok     = ( (!d_math_value_to_f(&f, _x)) &&
                   (!d_math_value_to_d(&d, _x)) &&
                   (!d_math_value_to_ld(&ld, _x)) &&
                   (!d_math_value_to_rational(&r, _x)) &&
                   (f == 12345.0f) &&
                   (d == 12345.0) &&
                   (ld == 12345.0L) &&
                   (r.num == 12345) &&
                   (r.den == 0) );
        after  = d_tests_counted();
        ok     = ( (ok) &&
                   (d_tests_calls_only(&before, &after, -1, 0, 0L)) );

        D_TESTS_EXPECT(ok, "a pair converts to nothing", _x, _x);

        return;
    }

    // float, double, long double: one call each, to that type
    before = d_tests_counted();
    ok     = d_math_value_to_f(&f, _x);
    after  = d_tests_counted();
    ok     = ( (ok) &&
               (d_tests_calls_only(&before, &after, kind, D_TESTS_CONVERT,
                                   1L)) &&
               (after.target[kind] == D_MATH_VALUE_FLOAT) );
    before = d_tests_counted();
    ok     = ( (d_math_value_to_d(&d, _x)) &&
               (ok) );
    after  = d_tests_counted();
    ok     = ( (ok) &&
               (d_tests_calls_only(&before, &after, kind, D_TESTS_CONVERT,
                                   1L)) &&
               (after.target[kind] == D_MATH_VALUE_DOUBLE) );
    before = d_tests_counted();
    ok     = ( (d_math_value_to_ld(&ld, _x)) &&
               (ok) );
    after  = d_tests_counted();
    ok     = ( (ok) &&
               (d_tests_calls_only(&before, &after, kind, D_TESTS_CONVERT,
                                   1L)) &&
               (after.target[kind] == D_MATH_VALUE_LONG_DOUBLE) );

    // the values the inline number gives
    (void)d_math_value_to_f(&wf, d_tests_plain(_x));
    (void)d_math_value_to_d(&wd, d_tests_plain(_x));
    (void)d_math_value_to_ld(&wld, d_tests_plain(_x));
    ok = ( (ok) &&
           (d_tests_same_real((long double)f, (long double)wf)) &&
           (d_tests_same_real((long double)d, (long double)wd)) &&
           (d_tests_same_real(ld, wld)) );

    // to_rational: exact kinds, one call; inexact, none
    if (d_math_value_to_rational(&wr, d_tests_plain(_x)))
    {
        before = d_tests_counted();
        ok     = ( (d_math_value_to_rational(&r, _x)) &&
                   (ok) );
        after  = d_tests_counted();
        ok     = ( (ok) &&
                   (r.num == wr.num) &&
                   (r.den == wr.den) &&
                   (d_tests_calls_only(&before, &after, kind,
                                       D_TESTS_CONVERT, 1L)) &&
                   (after.target[kind] == D_MATH_VALUE_RATIONAL) );
    }
    else
    {
        before = d_tests_counted();
        ok     = ( (!d_math_value_to_rational(&r, _x)) &&
                   (ok) );
        after  = d_tests_counted();
        ok     = ( (ok) &&
                   (r.num == 12345) &&
                   (r.den == 0) &&
                   (d_tests_calls_only(&before, &after, -1, 0, 0L)) );
    }

    D_TESTS_EXPECT( (ok) &&
                    (g_breaches == 0),
                    "to_f, to_d, to_ld, to_rational", _x, _x);

    return;
}

// d_tests_check_queries
//   is_number, is_exact, rank and kind_name of any _x against the
// harness's own statement of them, one check.
static void
d_tests_check_queries(
    struct d_math_value _x
)
{
    static const char* const inline_names[8] = {
        "none", "error", "boolean", "integer", "rational", "float", "double",
        "long double"
    };
    static const char* const boxed_names[D_TESTS_KINDS] = {
        "small", "exact", "inexact", "pair"
    };

    const int kind = d_tests_index(_x);

    bool        number = false;
    bool        exact  = false;
    const char* name   = NULL;

    // the harness's statement
    if (_x.kind == D_MATH_VALUE_BOXED)
    {
        number = (kind != D_TESTS_PAIR);
        exact  = ( (kind == D_TESTS_SMALL) ||
                   (kind == D_TESTS_EXACT) );
        name   = boxed_names[kind];
    }
    else
    {
        number = ( (_x.kind >= D_MATH_VALUE_INTEGER) &&
                   (_x.kind <= D_MATH_VALUE_LONG_DOUBLE) );
        exact  = ( (_x.kind == D_MATH_VALUE_INTEGER) ||
                   (_x.kind == D_MATH_VALUE_RATIONAL) );
        name   = inline_names[_x.kind];
    }

    D_TESTS_EXPECT( (d_math_value_is_number(_x) == number) &&
                    (d_math_value_is_exact(_x) == exact) &&
                    (d_math_value_rank(_x) == d_tests_rank_of(_x)) &&
                    (strcmp(d_math_value_kind_name(_x), name) == 0),
                    "is_number, is_exact, rank, kind_name", _x, _x);

    return;
}

// d_tests_check_holds
//   retain and release of any _x, one check: a copy of _x, its box held once
// more, then given back, the copy left none.
static void
d_tests_check_holds(
    struct d_math_value _x
)
{
    const bool boxed = (_x.kind == D_MATH_VALUE_BOXED);
    const int  kind  = d_tests_index(_x);
    const long refs  = d_tests_refs(_x);

    struct d_tests_count before = d_tests_counted();
    struct d_math_value  copy   = d_math_value_retain(_x);
    struct d_tests_count after  = d_tests_counted();
    bool                 ok     = false;

    // the copy, and one hold more
    ok = ( (d_tests_same(copy, _x)) &&
           (copy.box == _x.box) &&
           (copy.type == _x.type) &&
           (d_tests_refs(_x) == refs + (boxed ? 1L : 0L)) &&
           (d_tests_calls_only(&before, &after, kind, D_TESTS_RETAIN,
                               boxed ? 1L : 0L)) );

    // given back: the hold, and the copy none
    before = d_tests_counted();
    d_math_value_release(&copy);
    d_math_value_release(NULL);
    after  = d_tests_counted();

    ok = ( (ok) &&
           (d_tests_refs(_x) == refs) &&
           (d_tests_same(copy, d_math_value_none())) &&
           (d_tests_calls_only(&before, &after, kind, D_TESTS_RELEASE,
                               boxed ? 1L : 0L)) &&
           (after.live == before.live) );

    D_TESTS_EXPECT(ok, "retain and release", _x, _x);

    return;
}


//==============================================================================
// 4.  THE OPERANDS
//==============================================================================

// the operands, each held once by the harness
#define D_TESTS_MAX_VALUES  160

static struct d_math_value g_values[D_TESTS_MAX_VALUES];
static int                 g_count = 0;

// d_tests_add_value
//   _x among the operands; the harness's hold on it is the operands'.
static void
d_tests_add_value(
    struct d_math_value _x
)
{
    // room for it, or given back
    if (g_count < D_TESTS_MAX_VALUES)
    {
        g_values[g_count] = _x;
        ++g_count;
    }
    else
    {
        d_math_value_release(&_x);
    }

    return;
}

// d_tests_boxed_small, d_tests_boxed_exact, d_tests_boxed_inexact
//   a value of a mirror kind standing for an integer, an exact number, or a
// long double.
static struct d_math_value
d_tests_boxed_small(
    d_math_imax _n
)
{
    return d_tests_new_box(D_TESTS_SMALL, &g_kind_small,
                           d_math_value_from_int(_n), d_math_value_none());
}

static struct d_math_value
d_tests_boxed_exact(
    struct d_math_value _x
)
{
    return d_tests_new_box(D_TESTS_EXACT, &g_kind_exact, _x,
                           d_math_value_none());
}

static struct d_math_value
d_tests_boxed_inexact(
    long double _x
)
{
    return d_tests_new_box(D_TESTS_INEXACT, &g_kind_inexact,
                           d_math_value_from_ld(_x), d_math_value_none());
}

// d_tests_flagged
//   _x with the inexact flag.
static struct d_math_value
d_tests_flagged(
    struct d_math_value _x
)
{
    _x.flags = D_MATH_VALUE_FLAG_INEXACT;

    return _x;
}

// d_tests_make_values
//   the operands: inline values of every kind, and the boxed ones.
static void
d_tests_make_values(void)
{
    // each in the one form; integers as n/1
    static const d_math_imax exact[][2] = {
        { 0, 1 },          { 1, 1 },               { -1, 1 },
        { 2, 1 },          { -3, 1 },              { 7, 1 },
        { 1, 2 },          { -1, 3 },              { 2, 3 },
        { -5, 6 },
        { D_TESTS_MOST,      1                },
        { D_TESTS_LEAST,     1                },
        { D_TESTS_MOST - 1,  1                },
        { D_TESTS_LEAST + 1, 1                },
        { D_TESTS_MOST,      3                },
        { 1,                 D_TESTS_MOST     },
        { D_TESTS_HALF,      1                },
        { D_TESTS_HALF + 1,  1                },
        { D_TESTS_LEAST / 2, 1                },
        { D_TESTS_HALF + 1,  D_TESTS_HALF - 1 }
    };
    const int exact_count = (int)(sizeof(exact) / sizeof(exact[0]));

    volatile float       zero_f  = 0.0f;
    volatile double      zero_d  = 0.0;
    volatile long double zero_ld = 0.0L;
    int                  i       = 0;

    // the exact numbers, inline, in exact's box, and in small's for integers
    for (i = 0; i < exact_count; ++i)
    {
        const struct d_math_value x =
            d_math_value_from_ratio(exact[i][0], exact[i][1]);

        d_tests_add_value(x);
        d_tests_add_value(d_tests_boxed_exact(x));

        if (x.kind == D_MATH_VALUE_INTEGER)
        {
            d_tests_add_value(d_tests_boxed_small(x.exact.num));
        }
    }

    // float and double
    d_tests_add_value(d_math_value_from_f(0.0f));
    d_tests_add_value(d_math_value_from_f(-zero_f));
    d_tests_add_value(d_math_value_from_f(1.0f));
    d_tests_add_value(d_math_value_from_f(0.5f));
    d_tests_add_value(d_math_value_from_f(1.0f / 3.0f));
    d_tests_add_value(d_math_value_from_f(-2.5f));
    d_tests_add_value(d_math_value_from_f(16777217.0f));
    d_tests_add_value(d_math_value_from_f(1.0e30f));
    d_tests_add_value(d_math_value_from_f(FLT_MAX));
    d_tests_add_value(d_math_value_from_f(1.0f / zero_f));
    d_tests_add_value(d_math_value_from_f(zero_f / zero_f));
    d_tests_add_value(d_math_value_from_d(0.0));
    d_tests_add_value(d_math_value_from_d(-zero_d));
    d_tests_add_value(d_math_value_from_d(1.0));
    d_tests_add_value(d_math_value_from_d(0.1));
    d_tests_add_value(d_math_value_from_d(-2.5));
    d_tests_add_value(d_math_value_from_d(9007199254740992.0));
    d_tests_add_value(d_math_value_from_d(9223372036854775808.0));
    d_tests_add_value(d_math_value_from_d(1.0e300));
    d_tests_add_value(d_math_value_from_d(DBL_MAX));
    d_tests_add_value(d_math_value_from_d(-1.0 / zero_d));
    d_tests_add_value(d_math_value_from_d(zero_d / zero_d));

    // long double, inline and in inexact's box
    {
        const long double longs[] = {
            0.0L, -zero_ld, 1.0L, -1.0L, 0.5L, 1.0L / 3.0L, -2.5L,
            9007199254740993.0L, 1.0e300L, LDBL_MAX, 1.0L / zero_ld,
            -1.0L / zero_ld, zero_ld / zero_ld
        };
        const int long_count = (int)(sizeof(longs) / sizeof(longs[0]));

        for (i = 0; i < long_count; ++i)
        {
            d_tests_add_value(d_math_value_from_ld(longs[i]));
            d_tests_add_value(d_tests_boxed_inexact(longs[i]));
        }
    }

    // the nearest float, double and long double of rationals that none of
    // them equals: where a comparison must not settle for the rounding
    {
        const struct d_math_value odd[2] = {
            d_math_value_from_ratio(-1, 3),
            d_math_value_from_ratio(D_TESTS_MOST, 3)
        };

        float       near_f  = 0.0f;
        double      near_d  = 0.0;
        long double near_ld = 0.0L;

        for (i = 0; i < 2; ++i)
        {
            (void)d_math_value_to_f(&near_f, odd[i]);
            (void)d_math_value_to_d(&near_d, odd[i]);
            (void)d_math_value_to_ld(&near_ld, odd[i]);
            d_tests_add_value(d_math_value_from_f(near_f));
            d_tests_add_value(d_math_value_from_d(near_d));
            d_tests_add_value(d_math_value_from_ld(near_ld));
            d_tests_add_value(d_tests_boxed_inexact(near_ld));
        }
    }

    // the other kinds
    d_tests_add_value(d_math_value_none());
    d_tests_add_value(d_math_value_from_bool(false));
    d_tests_add_value(d_math_value_from_bool(true));
    d_tests_add_value(d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN));
    d_tests_add_value(d_math_value_from_error(D_MATH_VALUE_ERROR_POLE));
    d_tests_add_value(d_math_value_from_error(D_MATH_VALUE_ERROR_MEMORY));

    // flagged: an exact sum past the family, an error made from it, and
    // boxed values; one shares its box with an operand already there
    {
        const struct d_math_value past =
            d_math_value_add(d_math_value_from_int(D_TESTS_MOST),
                             d_math_value_from_int(1));
        const struct d_math_value third =
            d_tests_boxed_exact(d_math_value_from_ratio(1, 3));

        d_tests_add_value(past);
        d_tests_add_value(d_math_value_add(past, d_math_value_none()));
        d_tests_add_value(d_tests_flagged(d_tests_boxed_small(5)));
        d_tests_add_value(d_tests_flagged(d_tests_boxed_inexact(0.25L)));
        d_tests_add_value(third);
        d_tests_add_value(d_tests_flagged(d_math_value_retain(third)));
    }

    // pairs: numbers inline and boxed, a non-number and an error, and a pair
    // within a pair
    {
        struct d_math_value one  = d_math_value_from_int(1);
        struct d_math_value half =
            d_tests_boxed_exact(d_math_value_from_ratio(1, 2));
        struct d_math_value two  = d_tests_boxed_small(2);
        struct d_math_value tiny = d_tests_boxed_inexact(0.25L);
        struct d_math_value in   = d_tests_pair_of(one, two);

        d_tests_add_value(d_tests_pair_of(one, half));
        d_tests_add_value(d_tests_pair_of(two, tiny));
        d_tests_add_value(d_tests_pair_of(
            d_math_value_from_ratio(-1, 3), d_math_value_from_d(0.5)));
        d_tests_add_value(d_tests_flagged(d_tests_pair_of(
            d_math_value_from_bool(true),
            d_math_value_from_error(D_MATH_VALUE_ERROR_POLE))));
        d_tests_add_value(d_tests_pair_of(in, half));
        d_math_value_release(&half);
        d_math_value_release(&two);
        d_math_value_release(&tiny);
        d_math_value_release(&in);
    }

    return;
}

// d_tests_release_values
//   the operands given back.
static void
d_tests_release_values(void)
{
    int i = 0;

    // each
    for (i = 0; i < g_count; ++i)
    {
        d_math_value_release(&g_values[i]);
    }

    g_count = 0;

    return;
}


//==============================================================================
// 5.  THE CHECKS
//==============================================================================

// d_tests_pairs
//   every operand alone, and each pair with a boxed one, for every
// operation and the comparison.
static void
d_tests_pairs(void)
{
    int i  = 0;
    int j  = 0;
    int op = 0;

    // each operand
    for (i = 0; i < g_count; ++i)
    {
        d_tests_check_queries(g_values[i]);
        d_tests_check_holds(g_values[i]);
        d_tests_check_unary(g_values[i]);
        d_tests_check_convert(g_values[i]);

        // with each
        for (j = 0; j < g_count; ++j)
        {
            for (op = (int)D_MATH_VALUE_OP_ADD;
                 op <= (int)D_MATH_VALUE_OP_DIV; ++op)
            {
                d_tests_check_binary((enum d_math_value_op)op, g_values[i],
                                     g_values[j]);
            }

            d_tests_check_compare(g_values[i], g_values[j]);
        }
    }

    return;
}

// d_tests_named
//   results worth naming.
static void
d_tests_named(void)
{
    const struct d_math_value third = d_math_value_from_ratio(1, 3);
    const struct d_math_value sixth = d_math_value_from_ratio(1, 6);

    struct d_math_value boxed_third = d_tests_boxed_exact(third);
    struct d_math_value copy_sixth  =
        d_tests_new_box(D_TESTS_EXACT, &g_kind_exact_copy, sixth,
                        d_math_value_none());
    struct d_math_value most        = d_tests_boxed_small(D_TESTS_MOST);
    struct d_math_value tiny        = d_tests_boxed_inexact(0.25L);
    struct d_math_value r           = d_math_value_none();
    long double         near        = 0.0L;

    // a boxed 1/3 plus an inline 1/6: exact's box holding 1/2
    r = d_math_value_add(boxed_third, sixth);
    D_TESTS_EXPECT( (d_tests_is_boxed_as(r, D_TESTS_EXACT, 0u)) &&
                    (d_tests_box_of(r)->part[0].exact.num == 1) &&
                    (d_tests_box_of(r)->part[0].exact.den == 2),
                    "exact 1/3 + 1/6 is exact 1/2", boxed_third, sixth);
    d_tests_drop(&r);

    // two copies of one table, as two translation units have them: one kind
    r = d_math_value_add(boxed_third, copy_sixth);
    D_TESTS_EXPECT( (d_tests_is_boxed_as(r, D_TESTS_EXACT, 0u)) &&
                    (d_tests_box_of(r)->part[0].exact.num == 1) &&
                    (d_tests_box_of(r)->part[0].exact.den == 2) &&
                    (g_breaches == 0),
                    "two copies of exact's table are one kind", boxed_third,
                    copy_sixth);
    d_tests_drop(&r);

    // small's MAX + 1: the long double, flagged, inline (G2)
    r = d_math_value_add(most, d_math_value_from_int(1));
    D_TESTS_EXPECT( (r.kind == D_MATH_VALUE_LONG_DOUBLE) &&
                    (r.flags == D_MATH_VALUE_FLAG_INEXACT) &&
                    (r.real == (long double)D_TESTS_MOST + 1.0L),
                    "small MAX + 1 is a flagged long double", most, most);
    d_tests_drop(&r);

    // a boxed 1/3 plus the double 0.5: the double's, inline, each step
    // stored in a double
    {
        double          third_d = 0.0;
        volatile double sum     = 0.0;

        (void)d_math_value_to_d(&third_d, third);
        sum = third_d + 0.5;
        r   = d_math_value_add(boxed_third, d_math_value_from_d(0.5));
        D_TESTS_EXPECT( (r.kind == D_MATH_VALUE_DOUBLE) &&
                        (r.real == (long double)sum) &&
                        (r.type == NULL),
                        "exact 1/3 + 0.5 is a double", boxed_third, r);
        d_tests_drop(&r);
    }

    // a boxed 1/3 against its own nearest long double: not equal, and on
    // the side the rounding says
    (void)d_math_value_to_ld(&near, third);
    D_TESTS_EXPECT( (d_math_value_cmp(boxed_third, d_math_value_from_ld(near))
                     == d_math_value_cmp(third, d_math_value_from_ld(near))) &&
                    (!d_math_value_eq(boxed_third, d_math_value_from_ld(near))),
                    "exact 1/3 against its nearest long double", boxed_third,
                    d_math_value_from_ld(near));

    // inexact 0.25 times exact 1/3: inexact's box
    r = d_math_value_mul(tiny, boxed_third);
    D_TESTS_EXPECT( (d_tests_is_boxed_as(r, D_TESTS_INEXACT, 0u)) &&
                    (d_tests_box_of(r)->part[0].real ==
                     0.25L * d_rational_to_ld(third.exact)),
                    "inexact 0.25 * exact 1/3 is inexact", tiny, boxed_third);
    d_tests_drop(&r);

    // a pair times 2, part by part
    {
        struct d_math_value pair =
            d_tests_pair_of(d_math_value_from_int(1), boxed_third);

        r = d_math_value_mul(pair, d_math_value_from_int(3));
        D_TESTS_EXPECT( (d_tests_is_boxed_as(r, D_TESTS_PAIR, 0u)) &&
                        (d_tests_box_of(r)->part[0].kind ==
                         D_MATH_VALUE_INTEGER) &&
                        (d_tests_box_of(r)->part[0].exact.num == 3) &&
                        (d_tests_is_boxed_as(d_tests_box_of(r)->part[1],
                                             D_TESTS_EXACT, 0u)) &&
                        (d_tests_box_of(d_tests_box_of(r)->part[1])
                             ->part[0].exact.num == 1) &&
                        (d_tests_box_of(d_tests_box_of(r)->part[1])
                             ->part[0].exact.den == 1),
                        "(1, exact 1/3) * 3 is (3, exact 1)", pair, r);
        d_tests_drop(&r);
        d_math_value_release(&pair);
    }

    // an error passes a boxed operand by; none beside one is a domain error
    r = d_math_value_sub(d_math_value_from_error(D_MATH_VALUE_ERROR_POLE),
                         boxed_third);
    D_TESTS_EXPECT( (r.kind == D_MATH_VALUE_ERROR) &&
                    (r.code == (int)D_MATH_VALUE_ERROR_POLE),
                    "a pole less exact 1/3 is the pole", r, boxed_third);
    r = d_math_value_div(boxed_third, d_math_value_none());
    D_TESTS_EXPECT( (r.kind == D_MATH_VALUE_ERROR) &&
                    (r.code == (int)D_MATH_VALUE_ERROR_DOMAIN),
                    "exact 1/3 over none is a domain error", boxed_third, r);

    d_math_value_release(&boxed_third);
    d_math_value_release(&copy_sixth);
    d_math_value_release(&most);
    d_math_value_release(&tiny);

    return;
}

// d_tests_memory
//   storage refused: a kind's result is a memory error, with the operands'
// flags, and nothing leaks; a pair whose parts' or own storage is refused.
static void
d_tests_memory(void)
{
    const long live = g_live;

    struct d_math_value two   = d_tests_flagged(d_tests_boxed_small(2));
    struct d_math_value third =
        d_tests_boxed_exact(d_math_value_from_ratio(1, 3));
    struct d_math_value tiny  = d_tests_boxed_inexact(0.25L);
    struct d_math_value pair  = d_tests_pair_of(two, third);
    struct d_math_value r     = d_math_value_none();
    long                held  = 0;
    int                 k     = 0;

    // each mirror kind's own result refused
    g_refuse = 0;
    r        = d_math_value_add(two, d_math_value_from_int(1));
    D_TESTS_EXPECT( (r.kind == D_MATH_VALUE_ERROR) &&
                    (r.code == (int)D_MATH_VALUE_ERROR_MEMORY) &&
                    (r.flags == D_MATH_VALUE_FLAG_INEXACT),
                    "small 2 + 1, refused", two, r);
    g_refuse = 0;
    r        = d_math_value_neg(third);
    D_TESTS_EXPECT( (r.kind == D_MATH_VALUE_ERROR) &&
                    (r.code == (int)D_MATH_VALUE_ERROR_MEMORY),
                    "-exact 1/3, refused", third, r);
    g_refuse = 0;
    r        = d_math_value_mul(tiny, d_math_value_from_d(2.0));
    D_TESTS_EXPECT( (r.kind == D_MATH_VALUE_ERROR) &&
                    (r.code == (int)D_MATH_VALUE_ERROR_MEMORY),
                    "inexact 0.25 * 2.0, refused", tiny, r);

    // the pair plus 1: its first part, its second, then its own box refused
    for (k = 0; k < 3; ++k)
    {
        held     = g_live;
        g_refuse = k;
        r        = d_math_value_add(pair, d_math_value_from_int(1));

        if (k < 2)
        {
            D_TESTS_EXPECT( (d_tests_is_boxed_as(r, D_TESTS_PAIR, 0u)) &&
                            (d_tests_box_of(r)->part[k].kind ==
                             D_MATH_VALUE_ERROR) &&
                            (d_tests_box_of(r)->part[k].code ==
                             (int)D_MATH_VALUE_ERROR_MEMORY) &&
                            (d_tests_box_of(r)->part[1 - k].kind ==
                             D_MATH_VALUE_BOXED),
                            "(small 2, exact 1/3) + 1, a part refused", pair,
                            r);
        }
        else
        {
            D_TESTS_EXPECT( (r.kind == D_MATH_VALUE_ERROR) &&
                            (r.code == (int)D_MATH_VALUE_ERROR_MEMORY) &&
                            (r.flags == 0u) &&
                            (g_live == held),
                            "(small 2, exact 1/3) + 1, the pair refused",
                            pair, r);
        }

        d_tests_drop(&r);
        D_TESTS_EXPECT(g_live == held, "a refused pair's boxes given back",
                       pair, r);
    }

    // a pair made with its storage refused: its parts' holds given back
    held     = d_tests_refs(third);
    g_refuse = 0;
    r        = d_tests_pair_of(two, third);
    D_TESTS_EXPECT( (r.kind == D_MATH_VALUE_ERROR) &&
                    (d_tests_refs(third) == held),
                    "a pair refused its storage", two, third);

    d_math_value_release(&pair);
    d_math_value_release(&two);
    d_math_value_release(&third);
    d_math_value_release(&tiny);
    D_TESTS_EXPECT( (g_live == live) &&
                    (g_refuse == -1),
                    "storage refused: every box given back", r, r);

    return;
}

// d_tests_text_entries
//   each kind's text, through its table, and its room respected.
static void
d_tests_text_entries(void)
{
    struct d_math_value small = d_tests_boxed_small(-42);
    struct d_math_value exact =
        d_tests_boxed_exact(d_math_value_from_ratio(-5, 6));
    struct d_math_value tiny  = d_tests_boxed_inexact(0.25L);
    struct d_math_value in    =
        d_tests_pair_of(d_math_value_from_int(1), small);
    struct d_math_value pair  = d_tests_pair_of(in, exact);

    char   text[D_TESTS_TEXT_ROOM];
    char   room[8];
    size_t length = 0u;

    // each kind's text
    D_TESTS_EXPECT( (small.type->text(text, sizeof(text), small) == 3u) &&
                    (strcmp(text, "-42") == 0) &&
                    (exact.type->text(text, sizeof(text), exact) == 4u) &&
                    (strcmp(text, "-5/6") == 0) &&
                    (tiny.type->text(text, sizeof(text), tiny) == 4u) &&
                    (strcmp(text, "0.25") == 0) &&
                    (pair.type->text(text, sizeof(text), pair) == 16u) &&
                    (strcmp(text, "((1, -42), -5/6)") == 0),
                    "text: -42, -5/6, 0.25, ((1, -42), -5/6)", small, pair);

    // its room: two characters and the NUL, then none
    room[0] = 'x';
    room[3] = 'y';
    length  = small.type->text(room, 3u, small);
    D_TESTS_EXPECT( (length == 3u) &&
                    (strcmp(room, "-4") == 0) &&
                    (room[3] == 'y') &&
                    (small.type->text(room + 4, 0u, small) == 3u),
                    "text: a buffer too short", small, small);

    d_math_value_release(&pair);
    d_math_value_release(&in);
    d_math_value_release(&small);
    d_math_value_release(&exact);
    d_math_value_release(&tiny);

    return;
}

// Exactness by value, and holders: a kind whose values are exact or not one
// by one says which by its exact entry, asked only of a number whose traits
// do not say it for all; and the holders' count, value.h's for every kind.

// the box of the kinds below: whether it is exact; and the calls of the
// exact entry
struct d_tests_marked
{
    bool exact;
};

static long g_exact_calls = 0;

// d_tests_marked_exact
//   the exact entry: the box's mark, each call counted.
static bool
d_tests_marked_exact(
    struct d_math_value _x
)
{
    ++g_exact_calls;

    return ((const struct d_tests_marked*)_x.box)->exact;
}

// d_tests_marked_kind
//   a kind of the traits _traits and the exact entry _exact, its other
// entries the pair's, never called here.
static struct d_math_boxed_kind
d_tests_marked_kind(
    unsigned              _traits,
    d_math_boxed_exact_fn _exact
)
{
    struct d_math_boxed_kind kind = g_kind_pair;

    kind.name   = "marked";
    kind.rank   = 700;
    kind.traits = _traits;
    kind.exact  = _exact;

    return kind;
}

// d_tests_marked_value
//   a value of _kind holding _box, made by hand: no holder is counted.
static struct d_math_value
d_tests_marked_value(
    const struct d_math_boxed_kind* _kind,
    struct d_tests_marked*          _box
)
{
    struct d_math_value x = d_math_value_none();

    x.kind = D_MATH_VALUE_BOXED;
    x.type = _kind;
    x.box  = _box;

    return x;
}

// d_tests_exactness
//   d_math_value_is_exact of a boxed value: the exact entry's answer for a
// number whose traits say no D_MATH_BOXED_EXACT, asked once; true for one
// whose traits say it, and false for no number or for no entry, none of
// them asking; and d_math_boxed_hold and d_math_boxed_unhold, one holder
// more and one fewer, the last said, atomically as cfg_value.h resolves.
static void
d_tests_exactness(void)
{
    const struct d_math_value none = d_math_value_none();

    struct d_tests_marked    exact     = { true };
    struct d_tests_marked    inexact   = { false };
    struct d_math_boxed_kind by_value  =
        d_tests_marked_kind(D_MATH_BOXED_NUMBER, d_tests_marked_exact);
    struct d_math_boxed_kind no_entry  =
        d_tests_marked_kind(D_MATH_BOXED_NUMBER, NULL);
    struct d_math_boxed_kind all_exact =
        d_tests_marked_kind(D_MATH_BOXED_NUMBER | D_MATH_BOXED_EXACT,
                            d_tests_marked_exact);
    struct d_math_boxed_kind no_number =
        d_tests_marked_kind(0u, d_tests_marked_exact);
    size_t                   holders   = 1u;
    bool                     last_1    = true;
    bool                     last_2    = false;

    // asked of each value: its mark, once each
    g_exact_calls = 0;
    D_TESTS_EXPECT( (d_math_value_is_number(
                         d_tests_marked_value(&by_value, &exact))) &&
                    (d_math_value_is_exact(
                         d_tests_marked_value(&by_value, &exact))) &&
                    (!d_math_value_is_exact(
                         d_tests_marked_value(&by_value, &inexact))) &&
                    (g_exact_calls == 2),
                    "exact: the entry's answer, value by value, asked once "
                    "each", none, none);

    // not asked: the traits say it, or no number, or no entry
    g_exact_calls = 0;
    D_TESTS_EXPECT( (d_math_value_is_exact(
                         d_tests_marked_value(&all_exact, &inexact))) &&
                    (!d_math_value_is_exact(
                         d_tests_marked_value(&no_number, &exact))) &&
                    (!d_math_value_is_number(
                         d_tests_marked_value(&no_number, &exact))) &&
                    (!d_math_value_is_exact(
                         d_tests_marked_value(&no_entry, &exact))) &&
                    (g_exact_calls == 0),
                    "exact: the traits' answer for every value, none for no "
                    "number or no entry, and the entry not asked",
                    none, none);

    // the harness's own kinds and the wide integer have no entry
    D_TESTS_EXPECT( (g_kind_small.exact == NULL) &&
                    (g_kind_exact.exact == NULL) &&
                    (g_kind_inexact.exact == NULL) &&
                    (g_kind_pair.exact == NULL),
                    "exact: no entry in a table written without one",
                    none, none);

    // the holders: one more, then the last two given back
    d_math_boxed_hold(&holders);
    last_1 = d_math_boxed_unhold(&holders);
    last_2 = d_math_boxed_unhold(&holders);
    D_TESTS_EXPECT( (!last_1) &&
                    (last_2) &&
                    (holders == 0u),
                    "holders: 1, 2, 1, 0, the last said", none, none);

    // the knob on by default, where the build does not set it
#ifndef D_TESTS_HOLDERS_SET
    D_TESTS_EXPECT(D_CFG_IS_ON(D_CFG_MATH_VALUE_ATOMIC_HOLDERS),
                   "holders: the knob on by default", none, none);
#endif

    // atomic as cfg_value.h resolves it
    D_TESTS_EXPECT( (D_MATH_BOXED_HOLDERS_ATOMIC ==
                     ( (D_INTERNAL_MATH_VALUE_HOLDERS != 0) ? 1 : 0 )) &&
                    ( (D_CFG_IS_OFF(D_CFG_MATH_VALUE_ATOMIC_HOLDERS))
                          ? (D_MATH_BOXED_HOLDERS_ATOMIC == 0)
                          : (D_MATH_BOXED_HOLDERS_ATOMIC ==
                             ( ( (D_CFG_IS_ON(
                                      D_ENV_COMPILER_HAS_ATOMIC_BUILTINS)) ||
                                 (D_CFG_IS_ON(D_ENV_COMPILER_MSVC_FAMILY)) )
                                   ? 1 : 0 )) ),
                    "holders: atomic where the knob is on and the compiler "
                    "can", none, none);

    return;
}

static unsigned long g_state = 27182818UL;

// d_tests_random16
//   16 bits from a 32-bit linear congruential generator: the same in every
// build.
static unsigned long
d_tests_random16(void)
{
    g_state = ((g_state * 1103515245UL) + 12345UL) & 0xFFFFFFFFUL;

    return (g_state >> 16) & 0xFFFFUL;
}

// d_tests_random_bits
//   a d_math_umax below 2^_bits, _bits at most the word's.
static d_math_umax
d_tests_random_bits(
    unsigned _bits
)
{
    d_math_umax value = 0u;
    unsigned    have  = 0u;

    // sixteen bits at a time, then the excess shifted away
    while (have < _bits)
    {
        value = (value << 16) | (d_math_umax)d_tests_random16();
        have  = have + 16u;
    }

    return value >> (have - _bits);
}

// d_tests_random_exact
//   an exact value: a numerator below 2^n, either sign, over a denominator
// below 2^m, each length drawn up to w - 1; one time in four an integer.
static struct d_math_value
d_tests_random_exact(void)
{
    const unsigned    width = D_INTERNAL_RATIONAL_WORD_BITS;
    const unsigned    nbits = (unsigned)(d_tests_random16() % width);
    const unsigned    dbits =
        1u + (unsigned)(d_tests_random16() % (width - 1u));
    const bool        whole = ((d_tests_random16() & 3UL) == 0UL);
    const d_math_umax n     = d_tests_random_bits(nbits);
    const d_math_umax drawn = d_tests_random_bits(dbits);
    const bool        neg   = ((d_tests_random16() & 1UL) != 0UL);
    const d_math_umax d     = ( (whole) ||
                                (drawn == 0u) ) ? 1u : drawn;

    return d_math_value_from_ratio(neg ? -(d_math_imax)n : (d_math_imax)n,
                                   (d_math_imax)d);
}

// d_tests_random_near
//   a floating-point value at or next to _x's nearest one: its float, double
// or long double as _type is 0, 1 or 2, and as _step is 0, 1 or 2, that
// value, the next one up, or the next one down.
static struct d_math_value
d_tests_random_near(
    struct d_math_value _x,
    unsigned long       _type,
    unsigned long       _step
)
{
    float       f  = 0.0f;
    double      d  = 0.0;
    long double ld = 0.0L;

    // by type
    if (_type == 0UL)
    {
        (void)d_math_value_to_f(&f, _x);
        f = (_step == 1UL) ? nextafterf(f, FLT_MAX)
                           : ( (_step == 2UL) ? nextafterf(f, -FLT_MAX) : f );

        return d_math_value_from_f(f);
    }

    if (_type == 1UL)
    {
        (void)d_math_value_to_d(&d, _x);
        d = (_step == 1UL) ? nextafter(d, DBL_MAX)
                           : ( (_step == 2UL) ? nextafter(d, -DBL_MAX) : d );

        return d_math_value_from_d(d);
    }

    (void)d_math_value_to_ld(&ld, _x);
    ld = (_step == 1UL) ? nextafterl(ld, LDBL_MAX)
                        : ( (_step == 2UL) ? nextafterl(ld, -LDBL_MAX) : ld );

    return d_math_value_from_ld(ld);
}

// d_tests_random
//   D_TESTS_TRIALS draws: two exact numbers x and y, x's numerator n, and a
// floating-point value at or next to x's nearest; as values, x in exact's
// box, y inline and boxed, n in small's box, the floating-point value
// inline and in inexact's box. Each pair with a boxed one, both ways, for
// every operation and the comparison.
static void
d_tests_random(void)
{
    long trial = 0;
    int  i     = 0;
    int  j     = 0;
    int  op    = 0;

    // each draw
    for (trial = 0; trial < D_TESTS_TRIALS; ++trial)
    {
        const struct d_math_value x    = d_tests_random_exact();
        const struct d_math_value y    = d_tests_random_exact();
        const unsigned long       type = d_tests_random16() % 3UL;
        const unsigned long       step = d_tests_random16() % 3UL;
        const struct d_math_value near = d_tests_random_near(x, type, step);

        long double         as_ld     = 0.0L;
        struct d_math_value values[7];

        (void)d_math_value_to_ld(&as_ld, near);
        values[0] = d_tests_boxed_exact(x);
        values[1] = y;
        values[2] = d_tests_boxed_exact(y);
        values[3] = d_tests_boxed_small(x.exact.num);
        values[4] = near;
        values[5] = d_tests_boxed_inexact(as_ld);
        values[6] = d_math_value_from_int(x.exact.num);

        for (i = 0; i < 7; ++i)
        {
            for (j = 0; j < 7; ++j)
            {
                for (op = (int)D_MATH_VALUE_OP_ADD;
                     op <= (int)D_MATH_VALUE_OP_DIV; ++op)
                {
                    d_tests_check_binary((enum d_math_value_op)op, values[i],
                                         values[j]);
                }

                d_tests_check_compare(values[i], values[j]);
            }
        }

        for (i = 0; i < 7; ++i)
        {
            d_math_value_release(&values[i]);
        }
    }

    return;
}


//==============================================================================
// 6.  CONSTANT EVALUATION
//==============================================================================
// From C++14 the new queries, and retain and release of an inline value,
// are constant expressions, and so is an inline comparison through the new
// dispatch.

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

static constexpr bool
d_tests_constexpr_same_text(
    const char* _a,
    const char* _b
)
{
    // character by character
    while ( (*_a != '\0') &&
            (*_a == *_b) )
    {
        ++_a;
        ++_b;
    }

    return (*_a == *_b);
}

static constexpr struct d_math_value
d_tests_constexpr_released()
{
    struct d_math_value x = d_math_value_retain(d_math_value_from_ratio(1, 3));

    d_math_value_release(&x);

    return x;
}

static_assert( (d_math_value_rank(d_math_value_from_int(1)) == 100) &&
               (d_math_value_rank(d_math_value_from_ratio(1, 2)) == 200) &&
               (d_math_value_rank(d_math_value_from_f(0.5f)) == 300) &&
               (d_math_value_rank(d_math_value_from_d(0.5)) == 400) &&
               (d_math_value_rank(d_math_value_from_ld(0.5L)) == 500) &&
               (d_math_value_rank(d_math_value_none()) == 0),
               "the ranks as constant expressions");
static_assert(d_tests_constexpr_same_text(
                  d_math_value_kind_name(d_math_value_from_ratio(1, 3)),
                  "rational"),
              "kind_name as a constant expression");
static_assert( (d_tests_constexpr_released().kind == D_MATH_VALUE_NONE) &&
               (!d_math_value_is_number(d_tests_constexpr_released())),
               "retain and release of an inline value as constant "
               "expressions");
static_assert(d_math_value_eq(d_math_value_from_ratio(1, 2),
                              d_math_value_from_d(0.5)),
              "1/2 equals 0.5 as a constant expression");

// d_tests_constant_evaluation
//   the same at run time.
static void
d_tests_constant_evaluation(void)
{
    const struct d_math_value released = d_tests_constexpr_released();

    D_TESTS_EXPECT( (d_math_value_rank(d_math_value_from_ld(0.5L)) == 500) &&
                    (d_tests_constexpr_same_text(
                        d_math_value_kind_name(d_math_value_from_ratio(1, 3)),
                        "rational")) &&
                    (released.kind == D_MATH_VALUE_NONE) &&
                    (d_math_value_eq(d_math_value_from_ratio(1, 2),
                                     d_math_value_from_d(0.5))),
                    "the constant expressions, at run time", released,
                    released);

    return;
}

#endif  // C++14


int
main(void)
{
    const struct d_math_value none = d_math_value_none();

    // the ranks value.h states, as the harness states them
    D_TESTS_EXPECT( (D_MATH_VALUE_RANK_INTEGER == 100) &&
                    (D_MATH_VALUE_RANK_RATIONAL == 200) &&
                    (D_MATH_VALUE_RANK_FLOAT == 300) &&
                    (D_MATH_VALUE_RANK_DOUBLE == 400) &&
                    (D_MATH_VALUE_RANK_LONG_DOUBLE == 500),
                    "the inline numbers' ranks", none, none);

    d_tests_make_values();
    d_tests_pairs();
    d_tests_named();
    d_tests_memory();
    d_tests_text_entries();
    d_tests_exactness();
    d_tests_random();
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_evaluation();
#endif
    d_tests_release_values();

    // every box given back; no breach; the reference never needed more
    D_TESTS_EXPECT( (g_live == 0) &&
                    (g_made > 0),
                    "every box given back", none, none);
    D_TESTS_EXPECT(g_breaches == 0, "no kind given an operand it should "
                   "not have been", none, none);
    D_TESTS_EXPECT(!g_reference_overrun, "the reference's 256 bits held",
                   none, none);

    printf("t_value_boxed: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
